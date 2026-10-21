#include "ToolPalette.h"

#include <Bitmap.h>
#include <Window.h>

#include "BitmapUtils.h"
#include "PaintFont.h"


// MacPaint.p's toolLeft/Right/Top/Bottom/Mid and toolSpace, relative to
// the palette's top left corner.
static const int kWidth = 53;
static const int kHeight = 221;
static const int kMid = 26;
static const int kRowSpace = 22;
static const int kRows = kToolCount / 2;


ToolPalette::ToolPalette()
	:
	BView("tools", B_WILL_DRAW),
	fBitmap(NULL),
	fTool(Tool::Pencil)
{
	// One extra pixel each way for the drop shadow; BSize counts from 0.
	BSize size(kWidth, kHeight);
	SetExplicitMinSize(size);
	SetExplicitMaxSize(size);
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	_Render();
}


ToolPalette::~ToolPalette()
{
	delete fBitmap;
}


void
ToolPalette::Draw(BRect updateRect)
{
	DrawBitmap(fBitmap, BPoint(0, 0));
}


void
ToolPalette::MouseDown(BPoint where)
{
	if (where.x < 1 || where.x >= kWidth - 1 || where.y < 1
		|| where.y >= kHeight - 1)
		return;

	int row = ((int)where.y - 1) / kRowSpace;
	Tool tool = (Tool)(2 * row + (where.x > kMid ? 1 : 0));
	if (!IsToolImplemented(tool))
		return;

	// Double-clicks, as in MacPaint: the eraser clears the page, the brush
	// opens the brush shapes.
	int32 clicks = 1;
	if (BMessage* current = Window()->CurrentMessage())
		current->FindInt32("clicks", &clicks);
	if (clicks >= 2 && tool == Tool::Eraser) {
		Window()->PostMessage(kMsgClear);
		return;
	}
	BMessage message(kMsgSetTool);
	message.AddInt32("tool", (int32)tool);
	Window()->PostMessage(&message);

	if (clicks >= 2 && tool == Tool::Brush)
		Window()->PostMessage(kMsgChooseBrush);
	if (clicks >= 2 && tool == Tool::Select)
		Window()->PostMessage(kMsgSelectAll);
}


void
ToolPalette::SetTool(Tool tool)
{
	if (tool == fTool)
		return;
	fTool = tool;
	_Render();
	Invalidate();
}


void
ToolPalette::_Render()
{
	// Follows MacPaint's DrawTool and InvTool.
	hp::Canvas canvas(kWidth + 1, kHeight + 1);
	DrawPaletteBox(canvas);
	canvas.DrawLine(kMid, 0, kMid, kHeight, true);

	int v = 0;
	int code = kGlyphFirstTool;
	for (int row = 0; row < kRows; row++) {
		for (int column = 0; column < 2; column++, code++) {
			const Glyph* glyph = PaintFontGlyph(code);
			if (glyph == NULL)
				continue;
			int cellLeft = column == 0 ? 0 : kMid;
			DrawGlyph(canvas, *glyph, cellLeft + 14 - glyph->advance / 2,
				v + 4);
		}
		v += kRowSpace;
		if (row != kRows - 1)
			canvas.DrawLine(0, v, kWidth - 1, v, true);
	}

	int selected = (int)fTool;
	int top = 1 + (selected / 2) * kRowSpace;
	int bottom = top + kRowSpace - 1;
	if (selected % 2 == 1)
		InvertQDRect(canvas, kMid + 1, top, kWidth - 1, bottom);
	else
		InvertQDRect(canvas, 1, top, kMid, bottom);

	delete fBitmap;
	fBitmap = NewBitmapFromCanvas(canvas);
}
