#include "PatternPalette.h"

#include <Bitmap.h>
#include <Window.h>

#include "BitmapUtils.h"
#include "Messages.h"
#include "Patterns.h"


static const int kRows = 2;
static const int kColumns = (hp::kPatternCount + kRows - 1) / kRows;
// Cells share borders: each frame is kCellSize + 1 pixels, with a
// kCellSize - 1 pixel swatch inside.
static const int kCellSize = 17;
static const int kSwatchSize = kCellSize - 1;
static const int kCurrentSwatchSize = kRows * kCellSize - 1;
static const float kMargin = 4;
static const float kGap = 8;


PatternPalette::PatternPalette()
	:
	BView("patterns", B_WILL_DRAW),
	fCurrentSwatch(NULL),
	fIndex(-1)
{
	for (int i = 0; i < hp::kPatternCount; i++)
		fSwatches.push_back(NewPatternBitmap(hp::kPatterns[i], kSwatchSize));
	SetPatternIndex(hp::kDefaultPatternIndex);

	float width = _CellFrame(hp::kPatternCount - 1).right + kMargin;
	float height = kRows * kCellSize + 2 * kMargin;
	SetExplicitMinSize(BSize(width, height));
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, height));
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
}


PatternPalette::~PatternPalette()
{
	for (BBitmap* swatch : fSwatches)
		delete swatch;
	delete fCurrentSwatch;
}


void
PatternPalette::Draw(BRect updateRect)
{
	SetHighColor(0, 0, 0);

	BRect current = _CurrentFrame();
	StrokeRect(current);
	DrawBitmap(fCurrentSwatch, BPoint(current.left + 1, current.top + 1));

	for (int i = 0; i < hp::kPatternCount; i++) {
		BRect frame = _CellFrame(i);
		StrokeRect(frame);
		DrawBitmap(fSwatches[i], BPoint(frame.left + 1, frame.top + 1));
	}
}


void
PatternPalette::MouseDown(BPoint where)
{
	for (int i = 0; i < hp::kPatternCount; i++) {
		if (!_CellFrame(i).Contains(where))
			continue;

		BMessage message(kMsgSetPattern);
		message.AddInt32("index", i);
		Window()->PostMessage(&message);
		return;
	}
}


void
PatternPalette::SetPatternIndex(int index)
{
	if (index == fIndex || index < 0 || index >= hp::kPatternCount)
		return;
	fIndex = index;
	delete fCurrentSwatch;
	fCurrentSwatch = NewPatternBitmap(hp::kPatterns[index],
		kCurrentSwatchSize);
	Invalidate(_CurrentFrame());
}


BRect
PatternPalette::_CurrentFrame() const
{
	return BRect(kMargin, kMargin, kMargin + kRows * kCellSize,
		kMargin + kRows * kCellSize);
}


BRect
PatternPalette::_CellFrame(int index) const
{
	float left = _CurrentFrame().right + kGap + (index / kRows) * kCellSize;
	float top = kMargin + (index % kRows) * kCellSize;
	return BRect(left, top, left + kCellSize, top + kCellSize);
}
