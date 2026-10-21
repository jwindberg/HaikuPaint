#include "ToolPalette.h"

#include <Bitmap.h>
#include <Window.h>

#include "BitmapUtils.h"


static const int kIconSize = 16;
static const int kCellSize = 28;
static const int kColumns = 2;


static const char* const kPencilIcon[] = {
	"................",
	"................",
	"...........#....",
	"..........#.#...",
	".........#...#..",
	"........#...#...",
	".......#...#....",
	"......#...#.....",
	".....#...#......",
	"....#...#.......",
	"....##.#........",
	"...#.##.........",
	"...###..........",
	"..#.............",
	"................",
	"................",
};

static const char* const kLineIcon[] = {
	"................",
	"................",
	".............#..",
	"............#...",
	"...........#....",
	"..........#.....",
	".........#......",
	"........#.......",
	".......#........",
	"......#.........",
	".....#..........",
	"....#...........",
	"...#............",
	"..#.............",
	"................",
	"................",
};

static const char* const kPaintBucketIcon[] = {
	"................",
	"................",
	".....######.....",
	"....#......#....",
	"....#......#....",
	"...#........#...",
	"...##########...",
	"...#........##..",
	"....#......#..#.",
	"....########..#.",
	"....#......#..#.",
	"....#......#....",
	".....#....#...#.",
	".....######.....",
	"................",
	"................",
};

static const char* const kEraserIcon[] = {
	"................",
	"................",
	"................",
	"................",
	".....#########..",
	"....#.......##..",
	"...#.......#.#..",
	"..#########..#..",
	"..#.......#..#..",
	"..#.......#..#..",
	"..#.......#.#...",
	"..#.......##....",
	"..#########.....",
	"................",
	"................",
	"................",
};


struct ToolInfo {
	Tool				tool;
	const char* const*	icon;
};

static const ToolInfo kTools[] = {
	{Tool::Pencil, kPencilIcon},
	{Tool::Line, kLineIcon},
	{Tool::PaintBucket, kPaintBucketIcon},
	{Tool::Eraser, kEraserIcon},
};

static const int kToolCount = sizeof(kTools) / sizeof(kTools[0]);
static const int kRows = (kToolCount + kColumns - 1) / kColumns;


static BBitmap*
NewInvertedBitmap(const BBitmap* source)
{
	BBitmap* bitmap = new BBitmap(source);
	uint32* pixel = (uint32*)bitmap->Bits();
	int32 count = bitmap->BitsLength() / 4;
	for (int32 i = 0; i < count; i++)
		pixel[i] = pixel[i] == kBlackPixel ? kWhitePixel : kBlackPixel;
	return bitmap;
}


ToolPalette::ToolPalette()
	:
	BView("tools", B_WILL_DRAW),
	fTool(Tool::Pencil)
{
	for (const ToolInfo& info : kTools) {
		BBitmap* icon = NewBitmapFromArt(info.icon, kIconSize, kIconSize);
		fIcons.push_back(icon);
		fSelectedIcons.push_back(NewInvertedBitmap(icon));
	}

	// Cells share their borders, so the grid is one pixel wider than the
	// cells; BSize already counts that extra pixel.
	BSize size(kColumns * kCellSize, kRows * kCellSize);
	SetExplicitMinSize(size);
	SetExplicitMaxSize(size);
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
}


ToolPalette::~ToolPalette()
{
	for (BBitmap* icon : fIcons)
		delete icon;
	for (BBitmap* icon : fSelectedIcons)
		delete icon;
}


void
ToolPalette::Draw(BRect updateRect)
{
	for (int i = 0; i < kToolCount; i++) {
		BRect frame = _CellFrame(i);
		bool selected = kTools[i].tool == fTool;

		if (selected)
			SetHighColor(0, 0, 0);
		else
			SetHighColor(255, 255, 255);
		FillRect(frame);
		SetHighColor(0, 0, 0);
		StrokeRect(frame);

		float inset = (kCellSize - kIconSize) / 2;
		DrawBitmap(selected ? fSelectedIcons[i] : fIcons[i],
			BPoint(frame.left + inset, frame.top + inset));
	}
}


void
ToolPalette::MouseDown(BPoint where)
{
	for (int i = 0; i < kToolCount; i++) {
		if (!_CellFrame(i).Contains(where))
			continue;

		int32 clicks = 1;
		if (BMessage* current = Window()->CurrentMessage())
			current->FindInt32("clicks", &clicks);

		if (kTools[i].tool == Tool::Eraser && clicks >= 2) {
			Window()->PostMessage(kMsgClear);
			return;
		}

		BMessage message(kMsgSetTool);
		message.AddInt32("tool", (int32)kTools[i].tool);
		Window()->PostMessage(&message);
		return;
	}
}


void
ToolPalette::SetTool(Tool tool)
{
	if (tool == fTool)
		return;
	fTool = tool;
	Invalidate();
}


BRect
ToolPalette::_CellFrame(int index) const
{
	int column = index % kColumns;
	int row = index / kColumns;
	return BRect(column * kCellSize, row * kCellSize,
		(column + 1) * kCellSize, (row + 1) * kCellSize);
}
