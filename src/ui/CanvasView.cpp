#include "CanvasView.h"

#include <Bitmap.h>
#include <Cursor.h>
#include <InterfaceDefs.h>
#include <Looper.h>
#include <ScrollBar.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

#include "BitmapUtils.h"
#include "PaintFont.h"


// MacPaint's eraser is its 16 x 16 cursor box, centered on the hotspot.
static const int kEraserSize = 16;

// MacPaint's PolyLine/CreatePoly limit.
static const size_t kMaxPolygonPoints = 1000;

static const uint32 kMsgUpdateCursor = 'ucrs';


static hp::Point
PointOf(BPoint where)
{
	return {(int)floorf(where.x), (int)floorf(where.y)};
}


// Pixels a 16 x 16 mask stamped at point can touch.
static hp::Rect
MaskBounds(hp::Point point)
{
	return {point.x - 8, point.y - 8, point.x + 8, point.y + 8};
}


static bool
IsShapeTool(Tool tool)
{
	return tool >= Tool::Rect && tool <= Tool::FilledOval;
}


static bool
IsPolygonTool(Tool tool)
{
	return tool == Tool::Polygon || tool == Tool::FilledPolygon;
}


CanvasView::CanvasView()
	:
	BView("canvas", B_WILL_DRAW | B_FRAME_EVENTS),
	fCanUndo(false),
	fDirty(false),
	fBitmap(new BBitmap(BRect(0, 0, fCanvas.Width() - 1,
		fCanvas.Height() - 1), B_RGB32)),
	fTool(Tool::Pencil),
	fPattern(hp::kBlackPattern),
	fLineWidth(1),
	fBorder(true),
	fBrush(kDefaultBrush),
	fBrushMask(MaskFromGlyph(kGlyphFirstBrush + kDefaultBrush)),
	fSprayMask(MaskFromGlyph(kGlyphSpray)),
	fTracking(false),
	fPencilBlack(true),
	fStart({0, 0}),
	fLast({0, 0}),
	fPreviewRect({0, 0, 0, 0}),
	fPolygonActive(false)
{
	_SyncBitmap(BRect(0, 0, fCanvas.Width() - 1, fCanvas.Height() - 1));

	// Glyphs, masks and hotspots from MacPaint's SetToolCursor. Cursors
	// without a mask used an empty glyph there.
	fPencilCursor = NewGlyphCursor('L', kGlyphPencilMask, 3, 16);
	fFillCursor = NewGlyphCursor('I', kGlyphFillMask, 13, 16);
	fEraserCursor = NewGlyphCursor(kGlyphEraserBox, kGlyphAllOnes, 8, 8);
	fSprayCursor = NewGlyphCursor(kGlyphSpray, 0, 8, 8);
	fBrushCursor = NewGlyphCursor(kGlyphFirstBrush + fBrush, 0, 8, 8);
	const int lineGlyphs[] = {kGlyphLineCursor1, kGlyphLineCursor2,
		kGlyphLineCursor4, kGlyphLineCursor8};
	for (int i = 0; i < 4; i++)
		fLineCursors[i] = NewGlyphCursor(lineGlyphs[i], 0, 8, 8);
}


CanvasView::~CanvasView()
{
	delete fBitmap;
	delete fPencilCursor;
	delete fFillCursor;
	delete fEraserCursor;
	delete fSprayCursor;
	delete fBrushCursor;
	for (BCursor* cursor : fLineCursors)
		delete cursor;
}


void
CanvasView::AttachedToWindow()
{
	// The area outside the document, visible when the window is larger.
	SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_2_TINT));
	_UpdateScrollBars();

	// Setting the cursor now can reach the app_server before this view
	// does, and be lost; do it once the window is running instead.
	Looper()->PostMessage(kMsgUpdateCursor, this);
}


void
CanvasView::MessageReceived(BMessage* message)
{
	if (message->what == kMsgUpdateCursor)
		_UpdateCursor();
	else
		BView::MessageReceived(message);
}


void
CanvasView::Draw(BRect updateRect)
{
	BRect rect = updateRect
		& BRect(0, 0, fCanvas.Width() - 1, fCanvas.Height() - 1);
	if (rect.IsValid())
		DrawBitmap(fBitmap, rect, rect);
}


void
CanvasView::FrameResized(float newWidth, float newHeight)
{
	BView::FrameResized(newWidth, newHeight);
	_UpdateScrollBars();
}


void
CanvasView::KeyDown(const char* bytes, int32 numBytes)
{
	// As in MacPaint, any key ends a polygon in progress.
	if (fPolygonActive) {
		_FinishPolygon(false);
		return;
	}
	BView::KeyDown(bytes, numBytes);
}


void
CanvasView::MouseDown(BPoint where)
{
	MakeFocus();
	SetMouseEventMask(B_POINTER_EVENTS,
		B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
	hp::Point point = PointOf(where);
	fTracking = true;

	// A polygon spans several clicks; vertices are added on mouse up.
	if (IsPolygonTool(fTool)) {
		if (!fPolygonActive) {
			_BeginOperation();
			_BeginPreview();
			fPoints.assign(1, point);
			fPolygonActive = true;
		}
		return;
	}

	fStart = fLast = point;
	fAxisLock = hp::AxisLock(point);
	_BeginOperation();

	switch (fTool) {
		case Tool::PaintBucket:
			fCanvas.FloodFill(point.x, point.y, fPattern);
			_CanvasChanged(_CanvasBounds());
			fTracking = false;
			break;

		case Tool::Spray:
			fCanvas.StampMask(fSprayMask, point.x, point.y, fPattern);
			_CanvasChanged(MaskBounds(point));
			break;

		case Tool::Brush:
			fCanvas.StampMask(fBrushMask, point.x, point.y, fPattern);
			_CanvasChanged(MaskBounds(point));
			break;

		case Tool::Pencil:
			// Like MacPaint: starting on a black pixel draws white.
			fPencilBlack = !fCanvas.Get(point.x, point.y);
			fCanvas.Set(point.x, point.y, fPencilBlack);
			_CanvasChanged({point.x, point.y, point.x + 1, point.y + 1});
			break;

		case Tool::Eraser:
			_Erase(point, point);
			break;

		case Tool::Freeform:
		case Tool::FilledFreeform:
		{
			if (fTool == Tool::FilledFreeform) {
				_BeginPreview();
				fPoints.assign(1, point);
			}
			hp::Point pen = _PenPoint(point);
			fCanvas.StrokeLine(pen.x, pen.y, pen.x, pen.y, fLineWidth,
				hp::kBlackPattern);
			hp::Rect drawn = _PenBounds(point, point);
			fPreviewRect = fPreviewRect.Union(drawn);
			_CanvasChanged(drawn);
			break;
		}

		default:
			// Line and the shape tools.
			_BeginPreview();
			_DragTo(point);
			break;
	}
}


void
CanvasView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	hp::Point point = PointOf(where);

	// Belt and braces for the startup case above.
	if (transit == B_ENTERED_VIEW)
		_UpdateCursor();

	// The edge being placed follows the pointer, button down or not.
	if (fPolygonActive) {
		if (_ShiftDown())
			point = hp::Constrain(fPoints.back(), point, true);
		_DrawPolygonPreview(point);
		return;
	}

	if (!fTracking)
		return;

	switch (fTool) {
		case Tool::Pencil:
			if (_ShiftDown())
				point = fAxisLock.Apply(point);
			fCanvas.DrawLine(fLast.x, fLast.y, point.x, point.y,
				fPencilBlack);
			_CanvasChanged(_PenBounds(fLast, point));
			fLast = point;
			break;

		case Tool::Brush:
		case Tool::Spray:
		{
			// MacPaint's BrushPaint: the brush paints along the path, the
			// spray can only where the pointer has moved to.
			if (_ShiftDown())
				point = fAxisLock.Apply(point);
			int moved = std::abs(point.x - fLast.x)
				+ std::abs(point.y - fLast.y);
			if (fTool == Tool::Brush && moved > 1) {
				fCanvas.StrokeMask(fBrushMask, fLast.x, fLast.y, point.x,
					point.y, fPattern);
				_CanvasChanged(MaskBounds(fLast).Union(MaskBounds(point)));
				fLast = point;
			} else if (fTool == Tool::Spray && moved > 2) {
				fCanvas.StampMask(fSprayMask, point.x, point.y, fPattern);
				_CanvasChanged(MaskBounds(point));
				fLast = point;
			}
			break;
		}

		case Tool::Eraser:
			_Erase(fLast, point);
			fLast = point;
			break;

		case Tool::Freeform:
		case Tool::FilledFreeform:
		{
			// HollowCurve and GetRgn: add a segment once the pointer has
			// moved far enough.
			bool farEnough = fTool == Tool::Freeform
				? !hp::NearPoint(fLast, point, 3)
				: std::abs(point.x - fLast.x) + std::abs(point.y - fLast.y)
					> 1;
			if (!farEnough || fPoints.size() >= kMaxPolygonPoints)
				break;
			hp::Point from = _PenPoint(fLast);
			hp::Point to = _PenPoint(point);
			fCanvas.StrokeLine(from.x, from.y, to.x, to.y, fLineWidth,
				hp::kBlackPattern);
			hp::Rect drawn = _PenBounds(fLast, point);
			fPreviewRect = fPreviewRect.Union(drawn);
			_CanvasChanged(drawn);
			if (fTool == Tool::FilledFreeform)
				fPoints.push_back(point);
			fLast = point;
			break;
		}

		case Tool::PaintBucket:
			break;

		default:
			_DragTo(point);
			break;
	}
}


void
CanvasView::MouseUp(BPoint where)
{
	hp::Point point = PointOf(where);
	if (fPolygonActive) {
		if (_ShiftDown())
			point = hp::Constrain(fPoints.back(), point, true);
		_PolygonClick(point);
		fTracking = false;
		return;
	}

	if (!fTracking)
		return;
	fTracking = false;

	if (fTool == Tool::FilledFreeform)
		_FinishFilledFreeform();
	else if (fTool == Tool::Line || IsShapeTool(fTool))
		_DragTo(point);
}


void
CanvasView::SetTool(Tool tool)
{
	if (fPolygonActive)
		_FinishPolygon(false);
	fTool = tool;
	_UpdateCursor();
}


void
CanvasView::SetLineWidth(int width, bool border)
{
	fLineWidth = width;
	fBorder = border;
	_UpdateCursor();
}


void
CanvasView::SetBrush(int brush)
{
	if (brush < 0 || brush >= kBrushCount)
		return;
	fBrush = brush;
	fBrushMask = MaskFromGlyph(kGlyphFirstBrush + brush);

	// Switch to the new cursor before deleting the one in use.
	BCursor* oldCursor = fBrushCursor;
	fBrushCursor = NewGlyphCursor(kGlyphFirstBrush + brush, 0, 8, 8);
	_UpdateCursor();
	delete oldCursor;
}


void
CanvasView::Clear()
{
	if (fPolygonActive)
		_FinishPolygon(false);
	_BeginOperation();
	fCanvas.Clear();
	_CanvasChanged(_CanvasBounds());
}


void
CanvasView::Undo()
{
	if (fPolygonActive)
		_FinishPolygon(false);
	if (!fCanUndo)
		return;

	// Swapping makes the next Undo a redo, as in MacPaint.
	std::swap(fCanvas, fUndo);
	fDirty = true;
	_CanvasChanged(_CanvasBounds());
}


void
CanvasView::SetImage(const hp::Canvas& image)
{
	fPolygonActive = false;
	fPoints.clear();
	fTracking = false;
	fCanvas = image;
	fCanUndo = false;
	fDirty = false;
	_CanvasChanged(_CanvasBounds());
}


void
CanvasView::_BeginOperation()
{
	fUndo = fCanvas;
	fCanUndo = true;
	fDirty = true;
}


void
CanvasView::_BeginPreview()
{
	fPreviewRect = {0, 0, 0, 0};
}


void
CanvasView::_RestorePreview()
{
	fCanvas.CopyRect(fUndo, fPreviewRect);
}


void
CanvasView::_EndPreview(hp::Rect drawn)
{
	hp::Rect dirty = fPreviewRect.Union(drawn);
	fPreviewRect = drawn;
	_CanvasChanged(dirty);
}


void
CanvasView::_DragTo(hp::Point point)
{
	if (fTool == Tool::Line) {
		// StraightLine: Shift snaps to horizontal, vertical or 45 degrees.
		if (_ShiftDown())
			point = hp::Constrain(fStart, point, true);
		_DrawLine(point);
	} else {
		// CreateShape: Shift makes squares and circles.
		if (_ShiftDown())
			point = hp::Constrain(fStart, point, false);
		_DrawShape(point);
	}
}


void
CanvasView::_DrawLine(hp::Point end)
{
	_RestorePreview();
	hp::Point from = _PenPoint(fStart);
	hp::Point to = _PenPoint(end);
	fCanvas.StrokeLine(from.x, from.y, to.x, to.y, fLineWidth,
		hp::kBlackPattern);
	_EndPreview(_PenBounds(fStart, end));
}


void
CanvasView::_DrawShape(hp::Point end)
{
	// CreateShape: grow the dragged rect so the border is centered on it.
	hp::Rect rect = hp::RectFromPoints(fStart, end);
	int half = fLineWidth / 2;
	rect = {rect.left - half, rect.top - half,
		rect.right + fLineWidth - half, rect.bottom + fLineWidth - half};

	// The shape tools come in hollow/filled pairs, in Shape order.
	int pair = (int)fTool - (int)Tool::Rect;
	hp::Shape shape = (hp::Shape)(pair / 2);
	bool filled = pair % 2 == 1;

	_RestorePreview();
	if (filled)
		fCanvas.FillShape(shape, rect, fPattern);
	if (fBorder || !filled)
		fCanvas.FrameShape(shape, rect, fLineWidth, hp::kBlackPattern);
	_EndPreview(rect);
}


void
CanvasView::_PolygonClick(hp::Point point)
{
	// CreatePoly: done when a vertex lands near the start (closing the
	// polygon) or near the previous vertex (a double-click).
	hp::Point previous = fPoints.back();
	fPoints.push_back(point);

	bool closed = false;
	bool done = fPoints.size() >= kMaxPolygonPoints;
	if (fPoints.size() >= 3) {
		int tolerance = 4 * fLineWidth;
		closed = hp::NearPoint(point, fPoints.front(), tolerance);
		done = done || closed || hp::NearPoint(point, previous, tolerance);
	}

	if (done)
		_FinishPolygon(closed);
	else
		_DrawPolygonPreview(point);
}


void
CanvasView::_DrawPolygonPreview(hp::Point end)
{
	_RestorePreview();
	std::vector<hp::Point> pen = _PenPoints();
	pen.push_back(_PenPoint(end));
	fCanvas.StrokePolyline(pen, false, fLineWidth, hp::kBlackPattern);

	std::vector<hp::Point> points = fPoints;
	points.push_back(end);
	_EndPreview(_PointsBounds(points));
}


void
CanvasView::_FinishPolygon(bool closed)
{
	_RestorePreview();
	std::vector<hp::Point> pen = _PenPoints();
	if (fTool == Tool::FilledPolygon) {
		// Filled polygons are always closed; without a border the outline
		// is drawn in the fill pattern.
		fCanvas.FillPolygon(fPoints, fPattern);
		fCanvas.StrokePolyline(pen, true, fLineWidth,
			fBorder ? hp::kBlackPattern : fPattern);
	} else
		fCanvas.StrokePolyline(pen, closed, fLineWidth, hp::kBlackPattern);
	_EndPreview(_PointsBounds(fPoints));

	fPolygonActive = false;
	fPoints.clear();
}


void
CanvasView::_FinishFilledFreeform()
{
	// GetRgn: close the curve, fill it, then redraw its outline.
	_RestorePreview();
	fCanvas.FillPolygon(fPoints, fPattern);
	fCanvas.StrokePolyline(_PenPoints(), true, fLineWidth,
		fBorder ? hp::kBlackPattern : fPattern);
	_EndPreview(_PointsBounds(fPoints));
	fPoints.clear();
}


hp::Point
CanvasView::_PenPoint(hp::Point point) const
{
	// MacPaint centers its pen on the pointer: MoveTo(h - halfLineSize, ...).
	int half = fLineWidth / 2;
	return {point.x - half, point.y - half};
}


hp::Rect
CanvasView::_PenBounds(hp::Point a, hp::Point b) const
{
	hp::Rect rect = hp::RectFromPoints(a, b);
	int half = fLineWidth / 2;
	return {rect.left - half, rect.top - half,
		rect.right - half + fLineWidth, rect.bottom - half + fLineWidth};
}


hp::Rect
CanvasView::_PointsBounds(const std::vector<hp::Point>& points) const
{
	hp::Rect bounds = {0, 0, 0, 0};
	for (const hp::Point& point : points)
		bounds = bounds.Union(_PenBounds(point, point));
	return bounds;
}


std::vector<hp::Point>
CanvasView::_PenPoints() const
{
	std::vector<hp::Point> pen;
	for (const hp::Point& point : fPoints)
		pen.push_back(_PenPoint(point));
	return pen;
}


bool
CanvasView::_ShiftDown() const
{
	return (modifiers() & B_SHIFT_KEY) != 0;
}


void
CanvasView::_Erase(hp::Point from, hp::Point to)
{
	// Center the eraser square on the pointer.
	int half = kEraserSize / 2;
	fCanvas.StrokeLine(from.x - half, from.y - half, to.x - half,
		to.y - half, kEraserSize, hp::kWhitePattern);
	hp::Rect rect = hp::RectFromPoints(from, to);
	_CanvasChanged({rect.left - half, rect.top - half,
		rect.right + half, rect.bottom + half});
}


void
CanvasView::_CanvasChanged(hp::Rect dirty)
{
	if (dirty.IsEmpty())
		return;
	BRect rect(dirty.left, dirty.top, dirty.right - 1, dirty.bottom - 1);
	rect.InsetBy(-1, -1);
	rect = rect & BRect(0, 0, fCanvas.Width() - 1, fCanvas.Height() - 1);
	if (!rect.IsValid())
		return;
	_SyncBitmap(rect);
	Invalidate(rect);
}


void
CanvasView::_SyncBitmap(BRect dirty)
{
	int left = std::max(0, (int)floorf(dirty.left));
	int top = std::max(0, (int)floorf(dirty.top));
	int right = std::min(fCanvas.Width() - 1, (int)ceilf(dirty.right));
	int bottom = std::min(fCanvas.Height() - 1, (int)ceilf(dirty.bottom));

	for (int y = top; y <= bottom; y++) {
		for (int x = left; x <= right; x++)
			SetBitmapPixel(fBitmap, x, y, fCanvas.Get(x, y));
	}
}


void
CanvasView::_UpdateScrollBars()
{
	BRect bounds = Bounds();
	float visibleWidth = bounds.Width() + 1;
	float visibleHeight = bounds.Height() + 1;

	if (BScrollBar* bar = ScrollBar(B_HORIZONTAL)) {
		float range = std::max(0.0f, fCanvas.Width() - visibleWidth);
		bar->SetRange(0, range);
		bar->SetProportion(std::min(1.0f, visibleWidth / fCanvas.Width()));
		bar->SetSteps(8, visibleWidth);
	}
	if (BScrollBar* bar = ScrollBar(B_VERTICAL)) {
		float range = std::max(0.0f, fCanvas.Height() - visibleHeight);
		bar->SetRange(0, range);
		bar->SetProportion(std::min(1.0f, visibleHeight / fCanvas.Height()));
		bar->SetSteps(8, visibleHeight);
	}
}


void
CanvasView::_UpdateCursor()
{
	BCursor* cursor;
	switch (fTool) {
		case Tool::Pencil:
			cursor = fPencilCursor;
			break;
		case Tool::PaintBucket:
			cursor = fFillCursor;
			break;
		case Tool::Eraser:
			cursor = fEraserCursor;
			break;
		case Tool::Spray:
			cursor = fSprayCursor;
			break;
		case Tool::Brush:
			cursor = fBrushCursor;
			break;
		default:
		{
			// Line, shapes, freeform and polygon: a crosshair as thick as
			// the line.
			int index = fLineWidth >= 8 ? 3 : fLineWidth >= 4 ? 2
				: fLineWidth >= 2 ? 1 : 0;
			cursor = fLineCursors[index];
			break;
		}
	}
	SetViewCursor(cursor, true);
}


hp::Rect
CanvasView::_CanvasBounds() const
{
	return {0, 0, fCanvas.Width(), fCanvas.Height()};
}
