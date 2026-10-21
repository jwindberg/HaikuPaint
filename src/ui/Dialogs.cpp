#include "Dialogs.h"

#include <Bitmap.h>
#include <Button.h>
#include <View.h>

#include <algorithm>

#include "BitmapUtils.h"
#include "Geometry.h"
#include "Messages.h"
#include "Selection.h"


static const uint32 kMsgOK = 'ok  ';
static const uint32 kMsgCancel = 'cncl';	// also Escape
static const uint32 kMsgNone = 'none';


// MacPaint's buttons are 18 pixels tall; Haiku's want more room, so they
// keep MacPaint's left edges and widths and size their own height.
static BButton*
AddButton(BView* parent, int left, int top, int right, const char* label,
	uint32 what)
{
	BButton* button = new BButton(BRect(left, top, right - 1, top + 17),
		label, label, new BMessage(what));
	parent->AddChild(button);
	float width, height;
	button->GetPreferredSize(&width, &height);
	button->ResizeTo(right - left - 1, height);
	return button;
}


static void
ConfigureDialog(BWindow* window, BRect parentFrame)
{
	window->CenterIn(parentFrame);
	window->AddShortcut(B_ESCAPE, 0, new BMessage(kMsgCancel));
}


// #pragma mark - PatternEditWindow


// EditPat: the fat bits in (16, 16)-(80, 80), each 7 x 7 on an 8 pixel
// grid, and the sample in (96, 16)-(160, 80).
class PatternEditWindow::EditorView : public BView {
public:
	EditorView(BRect frame, const hp::Pattern& pattern)
		:
		BView(frame, "editor", B_FOLLOW_ALL, B_WILL_DRAW),
		fPattern(pattern),
		fBitmap(NULL),
		fTracking(false),
		fPaintWhite(false)
	{
		SetViewColor(255, 255, 255);
		_Render();
	}

	virtual ~EditorView()
	{
		delete fBitmap;
	}

	const hp::Pattern& Pattern() const { return fPattern; }

	virtual void Draw(BRect updateRect)
	{
		DrawBitmap(fBitmap, BPoint(0, 0));
	}

	virtual void MouseDown(BPoint where)
	{
		hp::Point bit;
		if (!_BitAt(where, &bit))
			return;
		// Like MacPaint: start on a black bit to paint white.
		fPaintWhite = fPattern.Bit(bit.x, bit.y);
		fTracking = true;
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
		_SetBit(bit);
	}

	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage)
	{
		hp::Point bit;
		if (fTracking && _BitAt(where, &bit))
			_SetBit(bit);
	}

	virtual void MouseUp(BPoint where)
	{
		fTracking = false;
	}

private:
	bool _BitAt(BPoint where, hp::Point* bit) const
	{
		if (!fTracking && (where.x < 16 || where.x >= 80 || where.y < 16
				|| where.y >= 80))
			return false;
		bit->x = std::max(0, std::min(7, ((int)where.x - 16) / 8));
		bit->y = std::max(0, std::min(7, ((int)where.y - 16) / 8));
		return true;
	}

	void _SetBit(hp::Point bit)
	{
		uint8_t mask = 0x80 >> bit.x;
		uint8_t& row = fPattern.rows[bit.y];
		uint8_t newRow = fPaintWhite ? row & ~mask : row | mask;
		if (newRow == row)
			return;
		row = newRow;
		_Render();
		Invalidate();
	}

	void _Render()
	{
		BRect bounds = Bounds();
		hp::Canvas canvas((int)bounds.Width() + 1, (int)bounds.Height() + 1);
		FrameQDRect(canvas, 15, 15, 81, 81);
		for (int y = 0; y < 8; y++) {
			for (int x = 0; x < 8; x++) {
				if (fPattern.Bit(x, y)) {
					PaintQDRect(canvas, 16 + x * 8, 16 + y * 8,
						16 + x * 8 + 7, 16 + y * 8 + 7);
				}
			}
		}
		FrameQDRect(canvas, 95, 15, 161, 81);
		PaintQDRect(canvas, 96, 16, 160, 80, fPattern);

		delete fBitmap;
		fBitmap = NewBitmapFromCanvas(canvas);
	}

	hp::Pattern		fPattern;
	BBitmap*		fBitmap;
	bool			fTracking;
	bool			fPaintWhite;
};


PatternEditWindow::PatternEditWindow(BRect parentFrame, int index,
	const hp::Pattern& pattern, BMessenger target)
	:
	BWindow(BRect(0, 0, 175, 127), "Edit pattern", B_MODAL_WINDOW_LOOK,
		B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS),
	fIndex(index),
	fTarget(target)
{
	fEditor = new EditorView(Bounds(), pattern);
	AddChild(fEditor);
	BButton* ok = AddButton(fEditor, 16, 92, 80, "OK", kMsgOK);
	AddButton(fEditor, 96, 92, 160, "Cancel", kMsgCancel);
	SetDefaultButton(ok);
	ConfigureDialog(this, parentFrame);
}


void
PatternEditWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgOK:
		{
			BMessage edited(kMsgPatternEdited);
			edited.AddInt32("index", fIndex);
			edited.AddData("pattern", B_RAW_TYPE, fEditor->Pattern().rows,
				8);
			fTarget.SendMessage(&edited);
			Quit();
			break;
		}
		case kMsgCancel:
			Quit();
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


// #pragma mark - MirrorWindow


// Symmetry: a 112 pixel square at (32, 16), its middle at (88, 72), with a
// line for each mirror, 3 pixels thick when it's on.
class MirrorWindow::MirrorView : public BView {
public:
	MirrorView(BRect frame, int mirrors)
		:
		BView(frame, "mirrors", B_FOLLOW_ALL, B_WILL_DRAW),
		fMirrors(mirrors),
		fBitmap(NULL)
	{
		SetViewColor(255, 255, 255);
		_Render();
	}

	virtual ~MirrorView()
	{
		delete fBitmap;
	}

	int Mirrors() const { return fMirrors; }

	virtual void Draw(BRect updateRect)
	{
		DrawBitmap(fBitmap, BPoint(0, 0));
	}

	virtual void MouseDown(BPoint where)
	{
		hp::Point click = {(int)where.x, (int)where.y};
		if (click.x < kLeft || click.x >= kLeft + kSize || click.y < kTop
			|| click.y >= kTop + kSize)
			return;

		// Snap the click to the nearest of the four lines.
		hp::Point middle = {kLeft + kSize / 2, kTop + kSize / 2};
		hp::Point snapped = hp::Constrain(middle, click, true);
		if (snapped.x == middle.x)
			fMirrors ^= kMirrorVertical;
		else if (snapped.y == middle.y)
			fMirrors ^= kMirrorHorizontal;
		else if (snapped.x - middle.x == snapped.y - middle.y)
			fMirrors ^= kMirrorDiagonal;
		else
			fMirrors ^= kMirrorAntiDiagonal;
		_Render();
		Invalidate();
	}

private:
	static const int kLeft = 32;
	static const int kTop = 16;
	static const int kSize = 112;

	void _Line(hp::Canvas& canvas, int mirror, hp::Point from, hp::Point to)
	{
		// PenSize(3, 3); Moov(-1, -1) for a mirror that's on.
		if (fMirrors & mirror) {
			canvas.StrokeLine(from.x - 1, from.y - 1, to.x - 1, to.y - 1, 3,
				hp::kBlackPattern);
		} else
			canvas.DrawLine(from.x, from.y, to.x, to.y, true);
	}

	void _Render()
	{
		BRect bounds = Bounds();
		int width = (int)bounds.Width() + 1;
		int height = (int)bounds.Height() + 1;

		hp::Canvas lines(width, height);
		int middleX = kLeft + kSize / 2;
		int middleY = kTop + kSize / 2;
		_Line(lines, kMirrorVertical, {middleX, kTop},
			{middleX, kTop + kSize});
		_Line(lines, kMirrorHorizontal, {kLeft, middleY},
			{kLeft + kSize, middleY});
		_Line(lines, kMirrorDiagonal, {kLeft, kTop},
			{kLeft + kSize, kTop + kSize});
		_Line(lines, kMirrorAntiDiagonal, {kLeft, kTop + kSize},
			{kLeft + kSize, kTop});

		// The lines are clipped to inside the frame.
		hp::Canvas canvas(width, height);
		FrameQDRect(canvas, kLeft, kTop, kLeft + kSize, kTop + kSize);
		canvas.CopyRect(lines, {kLeft + 1, kTop + 1, kLeft + kSize - 1,
			kTop + kSize - 1});

		delete fBitmap;
		fBitmap = NewBitmapFromCanvas(canvas);
	}

	int			fMirrors;
	BBitmap*	fBitmap;
};


MirrorWindow::MirrorWindow(BRect parentFrame, int mirrors, BMessenger target)
	:
	BWindow(BRect(0, 0, 175, 175), "Brush mirrors", B_MODAL_WINDOW_LOOK,
		B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS),
	fTarget(target)
{
	fMirrorView = new MirrorView(Bounds(), mirrors);
	AddChild(fMirrorView);
	BButton* ok = AddButton(fMirrorView, 16, 140, 80, "OK", kMsgOK);
	AddButton(fMirrorView, 96, 140, 160, "None", kMsgNone);
	SetDefaultButton(ok);
	ConfigureDialog(this, parentFrame);
}


// #pragma mark - ShowPageWindow


static const int kPageScale = 3;
static const int kPageMargin = 16;


// ShowPage: the small page in a frame at (16, 16), the window's part of it
// framed in gray, drawn in XOR.
class ShowPageWindow::PageView : public BView {
public:
	PageView(BRect frame, const hp::Canvas& page, hp::Rect visible)
		:
		BView(frame, "page", B_FOLLOW_ALL, B_WILL_DRAW),
		fSmall(page.Width() / kPageScale, page.Height() / kPageScale),
		fWindow(hp::MapRect(visible, {0, 0, page.Width(), page.Height()},
			{0, 0, page.Width() / kPageScale, page.Height() / kPageScale})),
		fOldWindow(fWindow),
		fShift({0, 0}),
		fDrag(kDragNone),
		fBitmap(NULL)
	{
		SetViewColor(255, 255, 255);

		// Reduce 3 x 3 blocks; three or more black pixels make a black one,
		// which keeps light grays visible.
		for (int y = 0; y < fSmall.Height(); y++) {
			for (int x = 0; x < fSmall.Width(); x++) {
				int black = 0;
				for (int dy = 0; dy < kPageScale; dy++) {
					for (int dx = 0; dx < kPageScale; dx++) {
						black += page.Get(x * kPageScale + dx,
							y * kPageScale + dy);
					}
				}
				fSmall.Set(x, y, black >= 3);
			}
		}
		_Render();
	}

	virtual ~PageView()
	{
		delete fBitmap;
	}

	// What OK applies, in full size page pixels.
	hp::Point Shift() const
	{
		// "always shift to nearest 8 to keep patterns aligned"
		return {((kPageScale * fShift.x) / 8) * 8,
			((kPageScale * fShift.y) / 8) * 8};
	}

	hp::Point Scroll() const
	{
		return {kPageScale * (fWindow.left - fOldWindow.left),
			kPageScale * (fWindow.top - fOldWindow.top)};
	}

	virtual void Draw(BRect updateRect)
	{
		DrawBitmap(fBitmap, BPoint(0, 0));
	}

	virtual void MouseDown(BPoint where)
	{
		hp::Point point = _Local(where);
		if (point.x >= fWindow.left && point.x < fWindow.right
			&& point.y >= fWindow.top && point.y < fWindow.bottom)
			fDrag = kDragWindow;
		else if (point.x >= 0 && point.x < fSmall.Width() && point.y >= 0
			&& point.y < fSmall.Height())
			fDrag = kDragPage;
		else
			return;

		fLast = point;
		fLock = hp::AxisLock(point);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	}

	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage)
	{
		if (fDrag == kDragNone)
			return;
		hp::Point point = _Local(where);
		if ((modifiers() & B_SHIFT_KEY) != 0)
			point = fLock.Apply(point);
		int dx = point.x - fLast.x;
		int dy = point.y - fLast.y;
		fLast = point;
		if (dx == 0 && dy == 0)
			return;

		if (fDrag == kDragWindow) {
			// Pinned inside the small page.
			int width = fWindow.Width();
			int height = fWindow.Height();
			int left = std::max(0, std::min(fWindow.left + dx,
				fSmall.Width() - width));
			int top = std::max(0, std::min(fWindow.top + dy,
				fSmall.Height() - height));
			fWindow = {left, top, left + width, top + height};
		} else {
			fShift.x += dx;
			fShift.y += dy;
		}
		_Render();
		Invalidate();
	}

	virtual void MouseUp(BPoint where)
	{
		fDrag = kDragNone;
	}

private:
	enum {
		kDragNone,
		kDragWindow,
		kDragPage
	};

	hp::Point _Local(BPoint where) const
	{
		return {(int)where.x - kPageMargin - 1,
			(int)where.y - kPageMargin - 1};
	}

	void _Render()
	{
		BRect bounds = Bounds();
		hp::Canvas canvas((int)bounds.Width() + 1, (int)bounds.Height() + 1);
		int left = kPageMargin + 1;
		int top = kPageMargin + 1;
		FrameQDRect(canvas, kPageMargin, kPageMargin,
			left + fSmall.Width() + 1, top + fSmall.Height() + 1);

		// The page, moved by the shift so far, clipped to its box.
		for (int y = 0; y < fSmall.Height(); y++) {
			for (int x = 0; x < fSmall.Width(); x++) {
				canvas.Set(left + x, top + y,
					fSmall.Get(x - fShift.x, y - fShift.y));
			}
		}

		// PenMode(patXor); PenPat(gray); FrameRect(windRect)
		const hp::Pattern kGray
			= {{0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55}};
		hp::Rect frame = {left + fWindow.left, top + fWindow.top,
			left + fWindow.right, top + fWindow.bottom};
		for (int x = frame.left; x < frame.right; x++) {
			for (int y : {frame.top, frame.bottom - 1}) {
				if (kGray.Bit(x, y))
					canvas.Set(x, y, !canvas.Get(x, y));
			}
		}
		for (int y = frame.top + 1; y < frame.bottom - 1; y++) {
			for (int x : {frame.left, frame.right - 1}) {
				if (kGray.Bit(x, y))
					canvas.Set(x, y, !canvas.Get(x, y));
			}
		}

		delete fBitmap;
		fBitmap = NewBitmapFromCanvas(canvas);
	}

	hp::Canvas		fSmall;
	hp::Rect		fWindow;
	hp::Rect		fOldWindow;
	hp::Point		fShift;
	int				fDrag;
	hp::Point		fLast;
	hp::AxisLock	fLock;
	BBitmap*		fBitmap;
};


ShowPageWindow::ShowPageWindow(BRect parentFrame, const hp::Canvas& page,
	hp::Rect visible, BMessenger target)
	:
	BWindow(BRect(0, 0, page.Width() / kPageScale + 2 * kPageMargin + 1,
			page.Height() / kPageScale + 2 * kPageMargin + 50),
		"Show page", B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS),
	fVisible(visible),
	fPageWidth(page.Width()),
	fPageHeight(page.Height()),
	fTarget(target)
{
	fPageView = new PageView(Bounds(), page, visible);
	AddChild(fPageView);
	int buttonTop = page.Height() / kPageScale + 2 * kPageMargin + 4;
	int middle = (int)Bounds().Width() / 2;
	BButton* ok = AddButton(fPageView, kPageMargin, buttonTop, middle - 8,
		"OK", kMsgOK);
	AddButton(fPageView, middle + 8, buttonTop,
		(int)Bounds().Width() - kPageMargin, "Cancel", kMsgCancel);
	SetDefaultButton(ok);
	ConfigureDialog(this, parentFrame);
}


void
ShowPageWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgOK:
		{
			hp::Point scroll = fPageView->Scroll();
			hp::Point origin = {fVisible.left + scroll.x,
				fVisible.top + scroll.y};

			// ShowPage pins the window to the edges when it's within 3
			// pixels of them.
			const int pageWidth = std::max(0, fPageWidth - fVisible.Width());
			const int pageHeight = std::max(0,
				fPageHeight - fVisible.Height());
			if (origin.x < 3)
				origin.x = 0;
			if (origin.y < 3)
				origin.y = 0;
			if (origin.x > pageWidth - 3)
				origin.x = pageWidth;
			if (origin.y > pageHeight - 3)
				origin.y = pageHeight;

			hp::Point shift = fPageView->Shift();
			BMessage done(kMsgShowPageDone);
			done.AddPoint("shift", BPoint(shift.x, shift.y));
			done.AddPoint("origin", BPoint(origin.x, origin.y));
			fTarget.SendMessage(&done);
			Quit();
			break;
		}
		case kMsgCancel:
			Quit();
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
MirrorWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgOK:
		case kMsgNone:
		{
			// MacPaint's None turns every mirror off.
			BMessage result(kMsgSetMirrors);
			result.AddInt32("mirrors",
				message->what == kMsgOK ? fMirrorView->Mirrors() : 0);
			fTarget.SendMessage(&result);
			Quit();
			break;
		}
		case kMsgCancel:
			Quit();
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
