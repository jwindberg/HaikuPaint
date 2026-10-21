#include "LineWidthPalette.h"

#include <Window.h>

#include <cmath>

#include "Messages.h"


// MacPaint's line sizes. Its "no border" choice for shapes comes with the
// shape tools.
static const int kWidths[] = {1, 2, 4, 8};
static const int kWidthCount = sizeof(kWidths) / sizeof(kWidths[0]);

// Matches the tool palette: two 28 pixel cells.
static const float kPaletteWidth = 56;
static const float kRowHeight = 14;
static const float kSampleInset = 8;


LineWidthPalette::LineWidthPalette()
	:
	BView("line widths", B_WILL_DRAW),
	fWidth(kWidths[0])
{
	BSize size(kPaletteWidth, kWidthCount * kRowHeight);
	SetExplicitMinSize(size);
	SetExplicitMaxSize(size);
}


void
LineWidthPalette::Draw(BRect updateRect)
{
	for (int i = 0; i < kWidthCount; i++) {
		BRect frame = _RowFrame(i);
		bool selected = kWidths[i] == fWidth;

		if (selected)
			SetHighColor(0, 0, 0);
		else
			SetHighColor(255, 255, 255);
		FillRect(frame);

		if (selected)
			SetHighColor(255, 255, 255);
		else
			SetHighColor(0, 0, 0);
		float top = floorf((frame.top + frame.bottom - kWidths[i]) / 2) + 1;
		FillRect(BRect(frame.left + kSampleInset, top,
			frame.right - kSampleInset, top + kWidths[i] - 1));
	}

	SetHighColor(0, 0, 0);
	StrokeRect(Bounds());
}


void
LineWidthPalette::MouseDown(BPoint where)
{
	for (int i = 0; i < kWidthCount; i++) {
		if (!_RowFrame(i).Contains(where))
			continue;

		BMessage message(kMsgSetLineWidth);
		message.AddInt32("width", kWidths[i]);
		Window()->PostMessage(&message);
		return;
	}
}


void
LineWidthPalette::SetLineWidth(int width)
{
	if (width == fWidth)
		return;
	fWidth = width;
	Invalidate();
}


BRect
LineWidthPalette::_RowFrame(int index) const
{
	return BRect(0, index * kRowHeight, kPaletteWidth,
		(index + 1) * kRowHeight);
}
