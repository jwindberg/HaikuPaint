#include "CanvasView.h"

#include <Bitmap.h>
#include <ScrollBar.h>

#include <algorithm>
#include <cmath>


static const uint32 kBlackPixel = 0xff000000;
static const uint32 kWhitePixel = 0xffffffff;


static BRect
RectFrom(BPoint a, BPoint b)
{
	return BRect(std::min(a.x, b.x), std::min(a.y, b.y),
		std::max(a.x, b.x), std::max(a.y, b.y));
}


CanvasView::CanvasView()
	:
	BView("canvas", B_WILL_DRAW | B_FRAME_EVENTS),
	fBitmap(new BBitmap(BRect(0, 0, fCanvas.Width() - 1,
		fCanvas.Height() - 1), B_RGB32)),
	fTool(Tool::Pencil),
	fTracking(false),
	fPencilBlack(true)
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

	// Rubber band while dragging out a line.
	if (fTracking && fTool == Tool::Line) {
		SetHighColor(0, 0, 0);
		StrokeLine(fStart, fLast);
	}
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
	int x = (int)floorf(where.x);
	int y = (int)floorf(where.y);

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
			// TODO: use the selected pattern once there is a pattern palette.
			fCanvas.FloodFill(x, y, hp::kBlackPattern);
			_CanvasChanged(_CanvasBounds());
			return;
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
	if (!fTracking)
		return;

	switch (fTool) {
		case Tool::Pencil:
			fCanvas.DrawLine((int)floorf(fLast.x), (int)floorf(fLast.y),
				(int)floorf(where.x), (int)floorf(where.y), fPencilBlack);
			_CanvasChanged(RectFrom(fLast, where));
			break;
		case Tool::Line:
		{
			BRect dirty = RectFrom(fStart, fLast) | RectFrom(fStart, where);
			dirty.InsetBy(-1, -1);
			Invalidate(dirty);
			break;
		}
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
		fCanvas.DrawLine((int)floorf(fStart.x), (int)floorf(fStart.y),
			(int)floorf(where.x), (int)floorf(where.y), true);
		_CanvasChanged(RectFrom(fStart, fLast) | RectFrom(fStart, where));
	}
	fTracking = false;
}


void
CanvasView::Clear()
{
	fCanvas.Clear();
	_CanvasChanged(_CanvasBounds());
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
	int left = std::max(0, (int)floorf(dirty.left));
	int top = std::max(0, (int)floorf(dirty.top));
	int right = std::min(fCanvas.Width() - 1, (int)ceilf(dirty.right));
	int bottom = std::min(fCanvas.Height() - 1, (int)ceilf(dirty.bottom));

	uint8* bits = (uint8*)fBitmap->Bits();
	int32 bytesPerRow = fBitmap->BytesPerRow();
	for (int y = top; y <= bottom; y++) {
		uint32* row = (uint32*)(bits + y * bytesPerRow);
		for (int x = left; x <= right; x++)
			row[x] = fCanvas.Get(x, y) ? kBlackPixel : kWhitePixel;
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
