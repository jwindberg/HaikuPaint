#include "PatternPalette.h"

#include <Bitmap.h>
#include <Window.h>

#include "BitmapUtils.h"
#include "Messages.h"
#include "Patterns.h"


// MacPaint.p's patLeft/Right/Top/Bottom/Mid, patSpace, patRow and patStart,
// relative to the palette.
static const int kWidth = 423;
static const int kHeight = 35;
static const int kMid = 17;
static const int kSpace = 20;
static const int kPerRow = 19;
static const int kStart = 43;

static_assert(2 * kPerRow == hp::kPatternCount, "two rows of patterns");


PatternPalette::PatternPalette()
	:
	BView("patterns", B_WILL_DRAW),
	fBitmap(NULL),
	fIndex(hp::kDefaultPatternIndex)
{
	BSize size(kWidth, kHeight);
	SetExplicitMinSize(size);
	SetExplicitMaxSize(size);
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	_Render();
}


PatternPalette::~PatternPalette()
{
	delete fBitmap;
}


void
PatternPalette::Draw(BRect updateRect)
{
	DrawBitmap(fBitmap, BPoint(0, 0));
}


void
PatternPalette::MouseDown(BPoint where)
{
	int x = (int)where.x;
	int y = (int)where.y;
	if (x < kStart || x >= kStart + kPerRow * kSpace || y < 1
		|| y >= kHeight - 1)
		return;

	int index = (x - kStart) / kSpace + (y > kMid ? kPerRow : 0);
	BMessage message(kMsgSetPattern);
	message.AddInt32("index", index);
	Window()->PostMessage(&message);
}


void
PatternPalette::SetPatternIndex(int index)
{
	if (index == fIndex || index < 0 || index >= hp::kPatternCount)
		return;
	fIndex = index;
	_Render();
	Invalidate();
}


void
PatternPalette::_Render()
{
	// Follows MacPaint's DrawPat.
	hp::Canvas canvas(kWidth + 1, kHeight + 1);
	DrawPaletteBox(canvas);

	FrameQDRect(canvas, 8, 6, kStart - 7, kHeight - 6);
	PaintQDRect(canvas, 9, 7, kStart - 8, kHeight - 7, hp::kPatterns[fIndex]);

	PaintQDRect(canvas, kStart - 1, 1, kWidth - 1, kHeight - 1);
	int h = kStart;
	for (int i = 0; i < kPerRow; i++) {
		PaintQDRect(canvas, h, 1, h + kSpace - 1, kMid, hp::kPatterns[i]);
		PaintQDRect(canvas, h, kMid + 1, h + kSpace - 1, kHeight - 1,
			hp::kPatterns[i + kPerRow]);
		h += kSpace;
	}

	delete fBitmap;
	fBitmap = NewBitmapFromCanvas(canvas);
}
