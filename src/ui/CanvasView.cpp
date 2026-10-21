#include "CanvasView.h"

#include <Bitmap.h>
#include <ScrollBar.h>

#include <algorithm>
#include <cmath>

#include "BitmapUtils.h"


static const int kEraserSize = 16;


static BRect
RectFrom(BPoint a, BPoint b)
{
	return BRect(std::min(a.x, b.x), std::min(a.y, b.y),
		std::max(a.x, b.x), std::max(a.y, b.y));
}


static int
PixelOf(float coordinate)
{
	return (int)floorf(coordinate);
}


CanvasView::CanvasView()
	:
	BView("canvas", B_WILL_DRAW | B_FRAME_EVENTS),
	fBitmap(new BBitmap(BRect(0, 0, fCanvas.Width() - 1,
		fCanvas.Height() - 1), B_RGB32)),
	fTool(Tool::Pencil),
	fPattern(hp::kBlackPattern),
	fLineWidth(1),
	fTracking(false),
	fPencilBlack(true),
	fShowEraser(false)
{
	_SyncBitmap(_CanvasBounds());
}


CanvasView::~CanvasView()
{
	delete fBitmap;
}


void
CanvasView::AttachedToWindow()
{
	// The area outside the document, visible when the window is larger.
	SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_2_TINT));
	_UpdateScrollBars();
}


void
CanvasView::Draw(BRect updateRect)
{
	BRect rect = updateRect & _CanvasBounds();
	if (rect.IsValid())
		DrawBitmap(fBitmap, rect, rect);

	SetHighColor(0, 0, 0);

	// Rubber band while dragging out a line. The engine's square pen hangs
	// down-right of each point, while app_server centers its pen, so shift
	// the preview to match the final result.
	if (fTracking && fTool == Tool::Line) {
		float offset = (fLineWidth - 1) / 2.0f;
		SetPenSize(fLineWidth);
		StrokeLine(fStart + BPoint(offset, offset),
			fLast + BPoint(offset, offset));
		SetPenSize(1);
	}

	if (fShowEraser && fTool == Tool::Eraser)
		StrokeRect(_EraserFrame(fEraserWhere));
}


void
CanvasView::FrameResized(float newWidth, float newHeight)
{
	BView::FrameResized(newWidth, newHeight);
	_UpdateScrollBars();
}


void
CanvasView::MouseDown(BPoint where)
{
	int x = PixelOf(where.x);
	int y = PixelOf(where.y);

	switch (fTool) {
		case Tool::Pencil:
			// Like MacPaint: starting on a black pixel draws white.
			fPencilBlack = !fCanvas.Get(x, y);
			fCanvas.Set(x, y, fPencilBlack);
			_CanvasChanged(BRect(where, where));
			break;
		case Tool::Line:
			break;
		case Tool::PaintBucket:
			fCanvas.FloodFill(x, y, fPattern);
			_CanvasChanged(_CanvasBounds());
			return;
		case Tool::Eraser:
			_Erase(where, where);
			break;
	}

	SetMouseEventMask(B_POINTER_EVENTS,
		B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
	fTracking = true;
	fStart = fLast = where;
}


void
CanvasView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (fTool == Tool::Eraser) {
		// The eraser outline follows the pointer, even when not drawing.
		if (fShowEraser)
			Invalidate(_EraserFrame(fEraserWhere));
		fShowEraser = transit != B_EXITED_VIEW && transit != B_OUTSIDE_VIEW;
		fEraserWhere = where;
		if (fShowEraser)
			Invalidate(_EraserFrame(where));
	}

	if (!fTracking)
		return;

	switch (fTool) {
		case Tool::Pencil:
			fCanvas.DrawLine(PixelOf(fLast.x), PixelOf(fLast.y),
				PixelOf(where.x), PixelOf(where.y), fPencilBlack);
			_CanvasChanged(RectFrom(fLast, where));
			break;
		case Tool::Line:
			Invalidate(_LineFrame(fStart, fLast) | _LineFrame(fStart, where));
			break;
		case Tool::Eraser:
			_Erase(fLast, where);
			break;
		case Tool::PaintBucket:
			break;
	}
	fLast = where;
}


void
CanvasView::MouseUp(BPoint where)
{
	if (!fTracking)
		return;

	if (fTool == Tool::Line) {
		fCanvas.StrokeLine(PixelOf(fStart.x), PixelOf(fStart.y),
			PixelOf(where.x), PixelOf(where.y), fLineWidth,
			hp::kBlackPattern);
		fTracking = false;	// before the redraw, so the rubber band goes away
		_CanvasChanged(_LineFrame(fStart, fLast) | _LineFrame(fStart, where));
	}
	fTracking = false;
}


void
CanvasView::SetTool(Tool tool)
{
	if (fShowEraser && fTool == Tool::Eraser && tool != Tool::Eraser) {
		fShowEraser = false;
		Invalidate(_EraserFrame(fEraserWhere));
	}
	fTool = tool;
}


void
CanvasView::Clear()
{
	fCanvas.Clear();
	_CanvasChanged(_CanvasBounds());
}


void
CanvasView::_Erase(BPoint from, BPoint to)
{
	// Center the eraser square on the pointer.
	int half = kEraserSize / 2;
	fCanvas.StrokeLine(PixelOf(from.x) - half, PixelOf(from.y) - half,
		PixelOf(to.x) - half, PixelOf(to.y) - half, kEraserSize,
		hp::kWhitePattern);
	_CanvasChanged(_EraserFrame(from) | _EraserFrame(to));
}


BRect
CanvasView::_EraserFrame(BPoint where) const
{
	float left = PixelOf(where.x) - kEraserSize / 2;
	float top = PixelOf(where.y) - kEraserSize / 2;
	return BRect(left, top, left + kEraserSize - 1, top + kEraserSize - 1);
}


BRect
CanvasView::_LineFrame(BPoint from, BPoint to) const
{
	BRect frame = RectFrom(from, to);
	frame.right += fLineWidth;
	frame.bottom += fLineWidth;
	frame.InsetBy(-1, -1);
	return frame;
}


void
CanvasView::_CanvasChanged(BRect dirty)
{
	dirty.InsetBy(-1, -1);
	dirty = dirty & _CanvasBounds();
	if (!dirty.IsValid())
		return;
	_SyncBitmap(dirty);
	Invalidate(dirty);
}


void
CanvasView::_SyncBitmap(BRect dirty)
{
	int left = std::max(0, PixelOf(dirty.left));
	int top = std::max(0, PixelOf(dirty.top));
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


BRect
CanvasView::_CanvasBounds() const
{
	return BRect(0, 0, fCanvas.Width() - 1, fCanvas.Height() - 1);
}
