#include "CanvasView.h"

#include <Bitmap.h>
#include <Clipboard.h>
#include <Cursor.h>
#include <InterfaceDefs.h>
#include <Looper.h>
#include <MessageRunner.h>
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
static const uint32 kMsgMarchAnts = 'ants';

// MacPaint stepped its ants every tick (1/60 s); a little slower is kinder
// to the app_server.
static const bigtime_t kAntsInterval = 40000;

// HaikuPaint's own clipboard data, which keeps lasso transparency. An
// archived BBitmap under "image/bitmap" goes along for other apps.
static const char* kSelectionClipboardType
	= "application/x-vnd.HaikuPaint-selection";


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


static hp::Rect
BoundsOf(const std::vector<hp::Point>& points)
{
	if (points.empty())
		return {0, 0, 0, 0};
	hp::Rect bounds = {points[0].x, points[0].y, points[0].x + 1,
		points[0].y + 1};
	for (const hp::Point& point : points) {
		bounds.left = std::min(bounds.left, point.x);
		bounds.top = std::min(bounds.top, point.y);
		bounds.right = std::max(bounds.right, point.x + 1);
		bounds.bottom = std::max(bounds.bottom, point.y + 1);
	}
	return bounds;
}


// The pixels of a 1 pixel line, for the lasso's trail.
static void
AddLinePixels(hp::Point from, hp::Point to, std::vector<hp::Point>& out)
{
	int dx = std::abs(to.x - from.x);
	int dy = -std::abs(to.y - from.y);
	int sx = from.x < to.x ? 1 : -1;
	int sy = from.y < to.y ? 1 : -1;
	int err = dx + dy;
	for (;;) {
		out.push_back(from);
		if (from == to)
			break;
		int e2 = 2 * err;
		if (e2 >= dy) {
			err += dy;
			from.x += sx;
		}
		if (e2 <= dx) {
			err += dx;
			from.y += sy;
		}
	}
}


// The frame of rect, as marching ants walk it.
static std::vector<hp::Point>
FramePixels(hp::Rect rect)
{
	std::vector<hp::Point> pixels;
	if (rect.IsEmpty())
		return pixels;
	for (int x = rect.left; x < rect.right; x++) {
		pixels.push_back({x, rect.top});
		if (rect.Height() > 1)
			pixels.push_back({x, rect.bottom - 1});
	}
	for (int y = rect.top + 1; y < rect.bottom - 1; y++) {
		pixels.push_back({rect.left, y});
		if (rect.Width() > 1)
			pixels.push_back({rect.right - 1, y});
	}
	return pixels;
}


// MacPaint's GetSelPat: $F8 rotated right by the phase, and one bit further
// left on each row, giving diagonal stripes that march as the phase steps.
static bool
AntBit(int x, int y, int phase)
{
	uint8 row = (uint8)((0xF8 >> phase) | (0xF8 << (8 - phase)));
	int shift = y & 7;
	row = (uint8)((row << shift) | (row >> (8 - shift)));
	return (row >> (7 - (x & 7))) & 1;
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


static bool
IsSelectionTool(Tool tool)
{
	return tool == Tool::Lasso || tool == Tool::Select;
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
	fPolygonActive(false),
	fBase(0, 0),
	fFloating(false),
	fDrag(kDragNone),
	fDragStartRect({0, 0, 0, 0}),
	fAnchor({0, 0}),
	fKeepWidth(false),
	fKeepHeight(false),
	fInSelection(false),
	fOverlayAnts(false),
	fOverlayBounds({0, 0, 0, 0}),
	fAntsPhase(0),
	fAntsRunner(NULL)
{
	_SyncBitmap(BRect(0, 0, fCanvas.Width() - 1, fCanvas.Height() - 1));

	// Glyphs, masks and hotspots from MacPaint's SetToolCursor. Cursors
	// without a mask used an empty glyph there.
	fPencilCursor = NewGlyphCursor('L', kGlyphPencilMask, 3, 16);
	fFillCursor = NewGlyphCursor('I', kGlyphFillMask, 13, 16);
	fEraserCursor = NewGlyphCursor(kGlyphEraserBox, kGlyphAllOnes, 8, 8);
	fSprayCursor = NewGlyphCursor(kGlyphSpray, 0, 8, 8);
	fBrushCursor = NewGlyphCursor(kGlyphFirstBrush + fBrush, 0, 8, 8);
	fLassoCursor = NewGlyphCursor('E', 0, 2, 15);
	fSelectCursor = NewGlyphCursor('r', 's', 8, 8);
	fArrowCursor = new BCursor(B_CURSOR_ID_SYSTEM_DEFAULT);
	const int lineGlyphs[] = {kGlyphLineCursor1, kGlyphLineCursor2,
		kGlyphLineCursor4, kGlyphLineCursor8};
	for (int i = 0; i < 4; i++)
		fLineCursors[i] = NewGlyphCursor(lineGlyphs[i], 0, 8, 8);
}


CanvasView::~CanvasView()
{
	delete fAntsRunner;
	delete fBitmap;
	delete fPencilCursor;
	delete fFillCursor;
	delete fEraserCursor;
	delete fSprayCursor;
	delete fBrushCursor;
	delete fLassoCursor;
	delete fSelectCursor;
	delete fArrowCursor;
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

	BMessage march(kMsgMarchAnts);
	fAntsRunner = new BMessageRunner(BMessenger(this), &march,
		kAntsInterval);
}


void
CanvasView::DetachedFromWindow()
{
	delete fAntsRunner;
	fAntsRunner = NULL;
}


void
CanvasView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgUpdateCursor:
			_UpdateCursor();
			break;

		case kMsgMarchAnts:
		{
			if (!fOverlayAnts || fOverlay.empty())
				break;
			fAntsPhase = (fAntsPhase + 1) & 7;
			for (const hp::Point& point : fOverlay) {
				if (fCanvas.Contains(point.x, point.y)) {
					SetBitmapPixel(fBitmap, point.x, point.y,
						AntBit(point.x, point.y, fAntsPhase));
				}
			}
			const hp::Rect& bounds = fOverlayBounds;
			Invalidate(BRect(bounds.left, bounds.top, bounds.right - 1,
				bounds.bottom - 1));
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
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
	// Backspace clears the selection, as in MacPaint.
	if (fSelection && (bytes[0] == B_BACKSPACE || bytes[0] == B_DELETE)) {
		_ClearSelection();
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

	if (IsSelectionTool(fTool)) {
		_SelectionMouseDown(point);
		return;
	}

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
				_LinePattern());
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

	// Belt and braces for the startup cursor case in AttachedToWindow().
	if (transit == B_ENTERED_VIEW)
		_UpdateCursor();

	// The arrow over a selection, as in MacPaint's TrackCursor.
	bool inSelection = fDrag == kDragNone && _InSelection(point);
	if (inSelection != fInSelection) {
		fInSelection = inSelection;
		_UpdateCursor();
	}

	// The edge being placed follows the pointer, button down or not.
	if (fPolygonActive) {
		if (_ShiftDown())
			point = hp::Constrain(fPoints.back(), point, true);
		_DrawPolygonPreview(point);
		return;
	}

	if (!fTracking)
		return;

	if (IsSelectionTool(fTool)) {
		switch (fDrag) {
			case kDragMarquee:
			{
				hp::Rect rect = hp::RectFromPoints(fStart, point);
				rect.right++;
				rect.bottom++;
				_SetOverlay(FramePixels(rect), true);
				break;
			}
			case kDragLasso:
				if (std::abs(point.x - fLast.x) + std::abs(point.y - fLast.y)
						> 1 && fPoints.size() < kMaxPolygonPoints) {
					std::vector<hp::Point> pixels;
					AddLinePixels(fLast, point, pixels);
					_AppendOverlay(pixels);
					fPoints.push_back(point);
					fLast = point;
				}
				break;
			case kDragNone:
				break;
			default:
				_DragSelection(point);
				break;
		}
		return;
	}

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
				_LinePattern());
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

	if (IsSelectionTool(fTool))
		_EndSelectionDrag(point);
	else if (fTool == Tool::FilledFreeform)
		_FinishFilledFreeform();
	else if (fTool == Tool::Line || IsShapeTool(fTool))
		_DragTo(point);
}


void
CanvasView::SetTool(Tool tool)
{
	if (fPolygonActive)
		_FinishPolygon(false);
	_AcceptSelection();
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
	if (fSelection) {
		_ClearSelection();
		return;
	}
	_BeginOperation();
	fCanvas.Clear();
	_CanvasChanged(_CanvasBounds());
}


void
CanvasView::Undo()
{
	if (fPolygonActive)
		_FinishPolygon(false);
	_AcceptSelection();
	if (!fCanUndo)
		return;

	// Swapping makes the next Undo a redo, as in MacPaint.
	std::swap(fCanvas, fUndo);
	fDirty = true;
	_CanvasChanged(_CanvasBounds());
}


void
CanvasView::Cut()
{
	if (!fSelection)
		return;
	Copy();
	_ClearSelection();
}


void
CanvasView::Copy()
{
	if (!fSelection || !be_clipboard->Lock())
		return;

	be_clipboard->Clear();
	BMessage* clip = be_clipboard->Data();
	std::vector<uint8_t> data = hp::EncodeSelection(*fSelection);
	clip->AddData(kSelectionClipboardType, B_MIME_TYPE, data.data(),
		data.size());

	// For other apps; unselected lasso pixels go white.
	hp::Canvas pixels = fSelection->pixels;
	for (int y = 0; y < pixels.Height(); y++) {
		for (int x = 0; x < pixels.Width(); x++) {
			if (!fSelection->mask.Get(x, y))
				pixels.Set(x, y, false);
		}
	}
	BBitmap* bitmap = NewBitmapFromCanvas(pixels);
	BMessage archive;
	if (bitmap->Archive(&archive) == B_OK)
		clip->AddMessage("image/bitmap", &archive);
	delete bitmap;

	be_clipboard->Commit();
	be_clipboard->Unlock();
}


void
CanvasView::Paste()
{
	hp::Selection pasted;
	bool found = false;
	if (be_clipboard->Lock()) {
		BMessage* clip = be_clipboard->Data();
		const void* data;
		ssize_t size;
		BMessage archive;
		if (clip->FindData(kSelectionClipboardType, B_MIME_TYPE, &data,
				&size) == B_OK) {
			found = hp::DecodeSelection((const uint8_t*)data, size, &pasted);
		} else if (clip->FindMessage("image/bitmap", &archive) == B_OK) {
			BBitmap bitmap(&archive);
			if (bitmap.InitCheck() == B_OK) {
				BRect bounds = bitmap.Bounds();
				hp::Canvas pixels(std::min((int)bounds.Width() + 1,
						fCanvas.Width()),
					std::min((int)bounds.Height() + 1, fCanvas.Height()));
				if (CanvasFromBitmap(&bitmap, pixels) == B_OK) {
					pasted = hp::SelectRect(pixels,
						{0, 0, pixels.Width(), pixels.Height()});
					found = true;
				}
			}
		}
		be_clipboard->Unlock();
	}
	if (!found)
		return;

	// CenterScrap: centered in the window.
	hp::Rect visible = _VisibleBounds();
	int width = pasted.rect.Width();
	int height = pasted.rect.Height();
	int left = (visible.left + visible.right - width) / 2;
	int top = (visible.top + visible.bottom - height) / 2;
	pasted.rect = {left, top, left + width, top + height};

	if (fPolygonActive)
		_FinishPolygon(false);
	_AcceptSelection();
	_BeginOperation();
	_PlaceSelection(std::move(pasted));
}


void
CanvasView::SelectAll()
{
	// MacPaint selects the whole window.
	if (fPolygonActive)
		_FinishPolygon(false);
	_AcceptSelection();
	hp::Selection selection = hp::SelectRect(fCanvas, _VisibleBounds());
	if (selection.rect.IsEmpty())
		return;
	fSelection = std::move(selection);
	fFloating = false;
	_SelectionChanged();
}


void
CanvasView::ApplyToSelection(SelectionCommand command)
{
	if (!fSelection)
		return;

	_BeginOperation();
	_Lift();
	hp::Rect old = fSelection->rect;
	switch (command) {
		case SelectionCommand::Invert:
			hp::Invert(*fSelection);
			break;
		case SelectionCommand::Fill:
			hp::Fill(*fSelection, fPattern);
			break;
		case SelectionCommand::TraceEdges:
			hp::TraceEdges(*fSelection, false);
			break;
		case SelectionCommand::TraceEdgesShadow:
			hp::TraceEdges(*fSelection, true);
			break;
		case SelectionCommand::FlipHorizontal:
			hp::FlipHorizontal(*fSelection);
			break;
		case SelectionCommand::FlipVertical:
			hp::FlipVertical(*fSelection);
			break;
		case SelectionCommand::Rotate:
			hp::RotateLeft(*fSelection);
			break;
	}

	fCanvas.CopyRect(fBase, old);
	hp::Stamp(fCanvas, *fSelection);
	_SelectionChanged();
	_CanvasChanged(old.Union(fSelection->rect));
}


void
CanvasView::SetImage(const hp::Canvas& image)
{
	fPolygonActive = false;
	fPoints.clear();
	fTracking = false;
	_AcceptSelection();
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
		_LinePattern());
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
		fCanvas.FrameShape(shape, rect, fLineWidth, _LinePattern());
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
	fCanvas.StrokePolyline(pen, false, fLineWidth, _LinePattern());

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
			fBorder ? _LinePattern() : fPattern);
	} else
		fCanvas.StrokePolyline(pen, closed, fLineWidth, _LinePattern());
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
		fBorder ? _LinePattern() : fPattern);
	_EndPreview(_PointsBounds(fPoints));
	fPoints.clear();
}


void
CanvasView::_SelectionMouseDown(hp::Point point)
{
	// EditDoc: inside the selection, drag it; elsewhere, make a new one.
	if (fSelection && _InSelection(point)) {
		_BeginSelectionDrag(point);
		return;
	}

	_AcceptSelection();
	fStart = fLast = point;
	if (fTool == Tool::Select) {
		fDrag = kDragMarquee;
		_SetOverlay(FramePixels({point.x, point.y, point.x + 1,
			point.y + 1}), true);
	} else {
		fDrag = kDragLasso;
		fPoints.assign(1, point);
		_SetOverlay({point}, false);
	}
}


void
CanvasView::_BeginSelectionDrag(hp::Point point)
{
	// DragImage's modes.
	bool option = _OptionDown();
	bool command = _CommandDown();
	if (option && command)
		fDrag = kDragTrail;
	else if (option)
		fDrag = kDragCopy;
	else if (command && !fSelection->lasso)
		fDrag = kDragStretch;
	else
		fDrag = kDragMove;

	_BeginOperation();
	if (fDrag == kDragCopy || fDrag == kDragTrail) {
		// The current state, original included, becomes the page.
		fBase = fCanvas;
		fFloating = true;
	} else
		_Lift();

	fStart = point;
	fLast = {-100000, -100000};
	fDragStartRect = fSelection->rect;
	fAxisLock = hp::AxisLock(point);

	if (fDrag == kDragStretch) {
		// Grabbing an outer third anchors the opposite side; the middle
		// third keeps that dimension. The middle of both just moves.
		const hp::Rect& rect = fDragStartRect;
		int third = rect.Width() / 3;
		fKeepWidth = false;
		if (point.x < rect.left + third)
			fAnchor.x = rect.right;
		else if (point.x > rect.right - third)
			fAnchor.x = rect.left;
		else
			fKeepWidth = true;

		third = rect.Height() / 3;
		fKeepHeight = false;
		if (point.y < rect.top + third)
			fAnchor.y = rect.bottom;
		else if (point.y > rect.bottom - third)
			fAnchor.y = rect.top;
		else
			fKeepHeight = true;

		if (fKeepWidth && fKeepHeight)
			fDrag = kDragMove;
		else
			fStretchSource = *fSelection;
	}
	_UpdateCursor();
}


void
CanvasView::_DragSelection(hp::Point point)
{
	if (fDrag != kDragStretch && _ShiftDown())
		point = fAxisLock.Apply(point);

	// Leaving a trail spaces the copies out by the line size.
	int tolerance = 0;
	if (fDrag == kDragTrail)
		tolerance = fLineWidth == 1 ? 1 : 2 * fLineWidth;
	if (hp::NearPoint(point, fLast, tolerance))
		return;
	fLast = point;

	hp::Rect old = fSelection->rect;
	if (fDrag == kDragStretch) {
		hp::Point start = fStart;
		if (_ShiftDown()) {
			// Keep the aspect ratio.
			start = hp::Constrain(fAnchor, start, false);
			point = hp::Constrain(fAnchor, point, false);
		}
		hp::Rect from = hp::RectFromPoints(fAnchor, start);
		hp::Rect to = hp::RectFromPoints(fAnchor, point);
		if (fKeepWidth) {
			to.left = from.left;
			to.right = from.right;
		}
		if (fKeepHeight) {
			to.top = from.top;
			to.bottom = from.bottom;
		}
		hp::Rect rect = hp::MapRect(fDragStartRect, from, to);
		if (rect.IsEmpty())
			return;
		*fSelection = hp::Stretched(fStretchSource, rect);
	} else {
		// Pinned so the selection stays on the page (or, if stretched
		// bigger than the page, at least its top left corner does).
		const hp::Rect& rect = fDragStartRect;
		int dx = std::max(-rect.left, std::min(point.x - fStart.x,
			fCanvas.Width() - rect.right));
		int dy = std::max(-rect.top, std::min(point.y - fStart.y,
			fCanvas.Height() - rect.bottom));
		fSelection->rect = {rect.left + dx, rect.top + dy, rect.right + dx,
			rect.bottom + dy};
	}

	if (fDrag != kDragTrail)
		fCanvas.CopyRect(fBase, old);
	hp::Stamp(fCanvas, *fSelection);
	if (fDrag == kDragTrail)
		hp::Stamp(fBase, *fSelection);

	if (fDrag == kDragStretch)
		_SelectionChanged();
	else {
		std::vector<hp::Point> edges;
		for (const hp::Point& edge : fSelectionEdges) {
			edges.push_back({fSelection->rect.left + edge.x,
				fSelection->rect.top + edge.y});
		}
		_SetOverlay(std::move(edges), true);
	}
	_CanvasChanged(old.Union(fSelection->rect));
}


void
CanvasView::_EndSelectionDrag(hp::Point point)
{
	DragMode drag = fDrag;
	fDrag = kDragNone;

	if (drag == kDragMarquee) {
		// SelDragImage: the rect includes the pixel under the pointer.
		hp::Rect rect = hp::RectFromPoints(fStart, point);
		_SetOverlay({}, false);
		if (rect.IsEmpty())
			return;
		rect.right++;
		rect.bottom++;
		hp::Selection selection = hp::SelectRect(fCanvas, rect);
		if (selection.rect.IsEmpty())
			return;
		fSelection = std::move(selection);
		fFloating = false;
		_SelectionChanged();
	} else if (drag == kDragLasso) {
		_SetOverlay({}, false);
		hp::Selection selection;
		if (hp::SelectLasso(fCanvas, fPoints, &selection)) {
			fSelection = std::move(selection);
			fFloating = false;
			_SelectionChanged();
		}
		fPoints.clear();
	}
	_UpdateCursor();
}


void
CanvasView::_Lift()
{
	// The first move or change leaves a white hole behind.
	if (fFloating)
		return;
	fBase = fCanvas;
	hp::EraseUnder(fBase, *fSelection);
	fFloating = true;
}


void
CanvasView::_PlaceSelection(hp::Selection selection)
{
	fSelection = std::move(selection);
	fBase = fCanvas;
	fFloating = true;
	hp::Stamp(fCanvas, *fSelection);
	_SelectionChanged();
	_CanvasChanged(fSelection->rect);
}


void
CanvasView::_SelectionChanged()
{
	fSelectionEdges.clear();
	for (const hp::Point& edge : fSelection->Edges()) {
		fSelectionEdges.push_back({edge.x - fSelection->rect.left,
			edge.y - fSelection->rect.top});
	}
	_SetOverlay(fSelection->Edges(), true);
}


void
CanvasView::_AcceptSelection()
{
	// AcceptEdits: the canvas already shows the selection where it is, so
	// accepting only forgets it.
	if (!fSelection)
		return;
	fSelection.reset();
	fSelectionEdges.clear();
	fBase = hp::Canvas(0, 0);
	fFloating = false;
	fDrag = kDragNone;
	fInSelection = false;
	_SetOverlay({}, false);
	_UpdateCursor();
}


void
CanvasView::_ClearSelection()
{
	if (!fSelection)
		return;
	_BeginOperation();
	hp::Rect rect = fSelection->rect;
	if (fFloating)
		fCanvas.CopyRect(fBase, rect);
	else
		hp::EraseUnder(fCanvas, *fSelection);
	_AcceptSelection();
	_CanvasChanged(rect);
}


bool
CanvasView::_InSelection(hp::Point point) const
{
	if (!fSelection)
		return false;
	if (!fSelection->lasso) {
		const hp::Rect& rect = fSelection->rect;
		return point.x >= rect.left && point.x < rect.right
			&& point.y >= rect.top && point.y < rect.bottom;
	}
	// TrackCursor counts a pixel next to the lasso mask too.
	for (int dy = -1; dy <= 1; dy++) {
		for (int dx = -1; dx <= 1; dx++) {
			if (fSelection->Contains({point.x + dx, point.y + dy}))
				return true;
		}
	}
	return false;
}


void
CanvasView::_SetOverlay(std::vector<hp::Point> points, bool ants)
{
	// Put back the canvas under the old overlay pixels only; a big marquee
	// covers far more area than pixels.
	for (const hp::Point& point : fOverlay) {
		if (fCanvas.Contains(point.x, point.y)) {
			SetBitmapPixel(fBitmap, point.x, point.y,
				fCanvas.Get(point.x, point.y));
		}
	}
	hp::Rect dirty = fOverlayBounds;

	fOverlay = std::move(points);
	fOverlayAnts = ants;
	fOverlayBounds = BoundsOf(fOverlay);
	for (const hp::Point& point : fOverlay) {
		if (fCanvas.Contains(point.x, point.y)) {
			SetBitmapPixel(fBitmap, point.x, point.y,
				!fOverlayAnts || AntBit(point.x, point.y, fAntsPhase));
		}
	}

	dirty = dirty.Union(fOverlayBounds);
	if (!dirty.IsEmpty()) {
		Invalidate(BRect(dirty.left, dirty.top, dirty.right - 1,
			dirty.bottom - 1));
	}
}


void
CanvasView::_AppendOverlay(const std::vector<hp::Point>& points)
{
	fOverlay.insert(fOverlay.end(), points.begin(), points.end());
	fOverlayBounds = fOverlayBounds.Union(BoundsOf(points));
	_CanvasChanged(BoundsOf(points));
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


const hp::Pattern&
CanvasView::_LinePattern() const
{
	// JamLine: lines and borders are black, or the pattern with Option.
	return _OptionDown() ? fPattern : hp::kBlackPattern;
}


bool
CanvasView::_ShiftDown() const
{
	return (modifiers() & B_SHIFT_KEY) != 0;
}


bool
CanvasView::_OptionDown() const
{
	return (modifiers() & B_CONTROL_KEY) != 0;
}


bool
CanvasView::_CommandDown() const
{
	return (modifiers() & B_COMMAND_KEY) != 0;
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

	// The overlay goes on top, on screen only.
	for (const hp::Point& point : fOverlay) {
		if (point.x >= left && point.x <= right && point.y >= top
			&& point.y <= bottom) {
			SetBitmapPixel(fBitmap, point.x, point.y,
				fOverlayAnts ? AntBit(point.x, point.y, fAntsPhase) : true);
		}
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
	if (fInSelection || fDrag >= kDragMove) {
		SetViewCursor(fArrowCursor, true);
		return;
	}

	switch (fTool) {
		case Tool::Lasso:
			cursor = fLassoCursor;
			break;
		case Tool::Select:
			cursor = fSelectCursor;
			break;
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


hp::Rect
CanvasView::_VisibleBounds() const
{
	BRect bounds = Bounds();
	hp::Rect visible = {(int)floorf(bounds.left), (int)floorf(bounds.top),
		(int)floorf(bounds.right) + 1, (int)floorf(bounds.bottom) + 1};
	return {std::max(visible.left, 0), std::max(visible.top, 0),
		std::min(visible.right, fCanvas.Width()),
		std::min(visible.bottom, fCanvas.Height())};
}
