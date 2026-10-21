#include "BrushWindow.h"

#include <Bitmap.h>
#include <View.h>

#include <algorithm>

#include "BitmapUtils.h"
#include "Messages.h"
#include "PaintFont.h"


// From MacPaint's ChooseBrush and its DLOG 2004 (288 x 160).
static const int kWidth = 288;
static const int kHeight = 160;
static const int kLeft = 16;
static const int kTop = 16;
static const int kSpace = 32;
static const int kRows = 4;
static const int kColumns = 8;


class BrushView : public BView {
public:
	BrushView(int brush, BMessenger target)
		:
		BView("brushes", B_WILL_DRAW),
		fTarget(target),
		fBitmap(NULL)
	{
		SetExplicitMinSize(BSize(kWidth - 1, kHeight - 1));
		SetExplicitMaxSize(BSize(kWidth - 1, kHeight - 1));
		_Render(brush);
	}

	virtual ~BrushView()
	{
		delete fBitmap;
	}

	virtual void AttachedToWindow()
	{
		MakeFocus();
	}

	virtual void Draw(BRect updateRect)
	{
		DrawBitmap(fBitmap, BPoint(0, 0));
	}

	virtual void MouseDown(BPoint where)
	{
		int row = ((int)where.y - kTop) / kSpace;
		int column = ((int)where.x - kLeft) / kSpace;
		row = std::max(0, std::min(row, kRows - 1));
		column = std::max(0, std::min(column, kColumns - 1));

		BMessage message(kMsgSetBrush);
		message.AddInt32("brush", row + column * kRows);
		fTarget.SendMessage(&message);
		Window()->PostMessage(B_QUIT_REQUESTED);
	}

	virtual void KeyDown(const char* bytes, int32 numBytes)
	{
		if (bytes[0] == B_ESCAPE)
			Window()->PostMessage(B_QUIT_REQUESTED);
		else
			BView::KeyDown(bytes, numBytes);
	}

private:
	void _Render(int brush)
	{
		hp::Canvas canvas(kWidth, kHeight);
		for (int column = 0; column < kColumns; column++) {
			for (int row = 0; row < kRows; row++) {
				const Glyph* glyph = PaintFontGlyph(kGlyphFirstBrush + row
					+ column * kRows);
				if (glyph != NULL) {
					DrawGlyph(canvas, *glyph,
						kLeft + kSpace / 2 - 8 + column * kSpace,
						kTop + kSpace / 2 - 8 + row * kSpace);
				}
			}
		}

		// InvBrush: a 2 pixel XOR frame around the current brush's cell.
		int left = kLeft + (brush / kRows) * kSpace;
		int top = kTop + (brush % kRows) * kSpace;
		InvertQDRect(canvas, left, top, left + kSpace, top + 2);
		InvertQDRect(canvas, left, top + kSpace - 2, left + kSpace,
			top + kSpace);
		InvertQDRect(canvas, left, top + 2, left + 2, top + kSpace - 2);
		InvertQDRect(canvas, left + kSpace - 2, top + 2, left + kSpace,
			top + kSpace - 2);

		fBitmap = NewBitmapFromCanvas(canvas);
	}

	BMessenger	fTarget;
	BBitmap*	fBitmap;
};


BrushWindow::BrushWindow(BRect parentFrame, int brush, BMessenger target)
	:
	BWindow(BRect(0, 0, kWidth - 1, kHeight - 1), "Brush Shape",
		B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	BView* view = new BrushView(brush, target);
	view->SetViewColor(255, 255, 255);
	view->ResizeTo(kWidth - 1, kHeight - 1);
	AddChild(view);
	CenterIn(parentFrame);
}
