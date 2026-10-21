#include "LineWidthPalette.h"

#include <Bitmap.h>
#include <Window.h>

#include "BitmapUtils.h"
#include "Messages.h"
#include "PaintFont.h"


// MacPaint.p's lineLeft/Right/Top/Bottom, relative to the palette.
static const int kWidth = 53;
static const int kHeight = 77;

// QuickDraw's ltGray, used for the "no border" sample.
static const hp::Pattern kLightGray
	= {{0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22}};

// One row per choice, from DrawLine, ChooseLine and InvLnSize.
struct LineChoice {
	int		size;
	bool	border;
	int		sampleTop;
	int		sampleHeight;
	int		checkTop;
};

static const LineChoice kChoices[] = {
	{1, false, 12, 1, 5},	// no border
	{1, true, 23, 1, 15},
	{2, true, 33, 2, 26},
	{4, true, 44, 4, 38},
	{8, true, 57, 8, 52},
};


LineWidthPalette::LineWidthPalette()
	:
	BView("line widths", B_WILL_DRAW),
	fBitmap(NULL),
	fWidth(1),
	fBorder(true)
{
	BSize size(kWidth, kHeight);
	SetExplicitMinSize(size);
	SetExplicitMaxSize(size);
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	_Render();
}


LineWidthPalette::~LineWidthPalette()
{
	delete fBitmap;
}


void
LineWidthPalette::Draw(BRect updateRect)
{
	DrawBitmap(fBitmap, BPoint(0, 0));
}


void
LineWidthPalette::MouseDown(BPoint where)
{
	if (where.x < 0 || where.x >= kWidth || where.y < 0
		|| where.y >= kHeight)
		return;

	// MacPaint's ChooseLine.
	float v = where.y;
	int size = 1;
	bool border = v > 18;
	if (v > 28)
		size = 2;
	if (v > 39)
		size = 4;
	if (v > 52)
		size = 8;

	BMessage message(kMsgSetLineWidth);
	message.AddInt32("width", size);
	message.AddBool("border", border);
	Window()->PostMessage(&message);
}


void
LineWidthPalette::SetLineWidth(int width, bool border)
{
	if (width == fWidth && border == fBorder)
		return;
	fWidth = width;
	fBorder = border;
	_Render();
	Invalidate();
}


void
LineWidthPalette::_Render()
{
	hp::Canvas canvas(kWidth + 1, kHeight + 1);
	DrawPaletteBox(canvas);

	const Glyph* check = PaintFontGlyph(kGlyphCheck);
	for (const LineChoice& choice : kChoices) {
		PaintQDRect(canvas, 17, choice.sampleTop, kWidth - 7,
			choice.sampleTop + choice.sampleHeight,
			choice.border ? hp::kBlackPattern : kLightGray);

		bool selected = choice.border == fBorder
			&& (!fBorder || choice.size == fWidth);
		if (selected && check != NULL)
			DrawGlyph(canvas, *check, 2, choice.checkTop, true);
	}

	delete fBitmap;
	fBitmap = NewBitmapFromCanvas(canvas);
}
