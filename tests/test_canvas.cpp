// Host-side unit tests for the drawing engine. No framework on purpose:
// it must build with nothing but a C++17 compiler, on macOS, Linux or Haiku.

#include "Canvas.h"
#include "MacPaintFile.h"
#include "Patterns.h"
#include "Selection.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>


static int sFailures = 0;

#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, \
				__LINE__, #condition); \
			sFailures++; \
		} \
	} while (false)


static int
CountBlack(const hp::Canvas& canvas)
{
	int count = 0;
	for (uint8_t pixel : canvas.Pixels())
		count += pixel;
	return count;
}


static void
TestNewCanvasIsWhite()
{
	hp::Canvas canvas;
	CHECK(canvas.Width() == 576);
	CHECK(canvas.Height() == 720);
	CHECK(CountBlack(canvas) == 0);
}


static void
TestOutOfBoundsIsSafe()
{
	hp::Canvas canvas(10, 10);
	canvas.Set(-1, 0, true);
	canvas.Set(10, 10, true);
	CHECK(CountBlack(canvas) == 0);
	CHECK(!canvas.Get(-5, -5));
}


static void
TestLines()
{
	hp::Canvas canvas(20, 20);
	canvas.DrawLine(0, 0, 9, 0, true);
	CHECK(CountBlack(canvas) == 10);
	CHECK(canvas.Get(0, 0) && canvas.Get(9, 0));

	canvas.Clear();
	canvas.DrawLine(9, 9, 0, 0, true);
	CHECK(CountBlack(canvas) == 10);
	CHECK(canvas.Get(5, 5));

	// A single point is still drawn.
	canvas.Clear();
	canvas.DrawLine(3, 4, 3, 4, true);
	CHECK(CountBlack(canvas) == 1);
}


static void
TestFloodFillStopsAtBorder()
{
	hp::Canvas canvas(20, 20);
	// Box outline from (5,5) to (10,10): 6x6 border, 4x4 interior.
	canvas.DrawLine(5, 5, 10, 5, true);
	canvas.DrawLine(10, 5, 10, 10, true);
	canvas.DrawLine(10, 10, 5, 10, true);
	canvas.DrawLine(5, 10, 5, 5, true);
	int border = CountBlack(canvas);
	CHECK(border == 20);

	canvas.FloodFill(7, 7, hp::kBlackPattern);
	CHECK(CountBlack(canvas) == border + 16);
	CHECK(!canvas.Get(0, 0));
}


static void
TestFloodFillWithPatternTerminates()
{
	// A checkerboard contains both colors; a naive fill would never end.
	const hp::Pattern checker
		= {{0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55}};
	hp::Canvas canvas(16, 16);
	canvas.FloodFill(0, 0, checker);
	CHECK(CountBlack(canvas) == 16 * 16 / 2);
	CHECK(canvas.Get(0, 0) && !canvas.Get(1, 0) && canvas.Get(1, 1));
}


static void
TestFillRect()
{
	hp::Canvas canvas(20, 20);
	// Corners in any order, inclusive.
	canvas.FillRect(5, 6, 2, 3, hp::kBlackPattern);
	CHECK(CountBlack(canvas) == 4 * 4);
	CHECK(canvas.Get(2, 3) && canvas.Get(5, 6) && !canvas.Get(6, 6));

	// Clipped at the edges.
	canvas.Clear();
	canvas.FillRect(-10, -10, 1, 1, hp::kBlackPattern);
	CHECK(CountBlack(canvas) == 4);
}


static void
TestStrokeLineSquarePen()
{
	hp::Canvas canvas(40, 40);
	// A 3x3 pen hangs down-right: a horizontal stroke from x 10 to 19
	// covers x 10..21, y 5..7.
	canvas.StrokeLine(10, 5, 19, 5, 3, hp::kBlackPattern);
	CHECK(CountBlack(canvas) == 12 * 3);
	CHECK(canvas.Get(10, 5) && canvas.Get(21, 7));
	CHECK(!canvas.Get(9, 5) && !canvas.Get(10, 8));

	// Pen size 1 matches DrawLine.
	hp::Canvas thin(40, 40);
	thin.DrawLine(1, 2, 30, 17, true);
	canvas.Clear();
	canvas.StrokeLine(1, 2, 30, 17, 1, hp::kBlackPattern);
	CHECK(canvas.Pixels() == thin.Pixels());
}


static void
TestEraserStroke()
{
	// Erasing is a stroke with the white pattern.
	hp::Canvas canvas(40, 40);
	canvas.FillRect(0, 0, 39, 39, hp::kBlackPattern);
	canvas.StrokeLine(0, 0, 0, 0, 16, hp::kWhitePattern);
	CHECK(CountBlack(canvas) == 40 * 40 - 16 * 16);
	CHECK(!canvas.Get(15, 15) && canvas.Get(16, 16));
}


static void
TestPatternsAreDistinct()
{
	CHECK(std::memcmp(&hp::kPatterns[hp::kDefaultPatternIndex],
		&hp::kBlackPattern, sizeof(hp::Pattern)) == 0);
	CHECK(std::memcmp(&hp::kPatterns[hp::kWhitePatternIndex],
		&hp::kWhitePattern, sizeof(hp::Pattern)) == 0);

	// Also catches a short initializer list: missing entries would be
	// zero-filled and duplicate white.
	for (int i = 0; i < hp::kPatternCount; i++) {
		for (int j = i + 1; j < hp::kPatternCount; j++) {
			if (std::memcmp(&hp::kPatterns[i], &hp::kPatterns[j],
					sizeof(hp::Pattern)) == 0) {
				std::fprintf(stderr, "patterns %d and %d are identical\n",
					i, j);
				sFailures++;
			}
		}
	}
}


static void
TestFillAndFrameRect()
{
	hp::Canvas canvas(40, 40);
	// Exclusive right and bottom, as in QuickDraw.
	canvas.FillShape(hp::Shape::Rect, {10, 10, 20, 15}, hp::kBlackPattern);
	CHECK(CountBlack(canvas) == 10 * 5);
	CHECK(canvas.Get(19, 14) && !canvas.Get(20, 14) && !canvas.Get(19, 15));

	// A 2 pixel frame stays inside the rect.
	canvas.Clear();
	canvas.FrameShape(hp::Shape::Rect, {10, 10, 20, 20}, 2,
		hp::kBlackPattern);
	CHECK(CountBlack(canvas) == 10 * 10 - 6 * 6);
	CHECK(canvas.Get(10, 10) && canvas.Get(11, 11) && !canvas.Get(12, 12));
	CHECK(canvas.Get(19, 19) && !canvas.Get(20, 20));

	// A pen wider than half the rect fills it.
	canvas.Clear();
	canvas.FrameShape(hp::Shape::Rect, {0, 0, 6, 6}, 4, hp::kBlackPattern);
	CHECK(CountBlack(canvas) == 36);
}


static void
TestOvalAndRoundRect()
{
	hp::Canvas canvas(60, 60);
	canvas.FillShape(hp::Shape::Oval, {0, 0, 40, 20}, hp::kBlackPattern);
	// Center filled, corners empty, extremes of the axes reached.
	CHECK(canvas.Get(20, 10) && !canvas.Get(0, 0) && !canvas.Get(39, 19));
	CHECK(canvas.Get(0, 10) && canvas.Get(39, 9) && canvas.Get(20, 0));
	// Symmetric left/right and top/bottom.
	bool symmetric = true;
	for (int y = 0; y < 20; y++) {
		for (int x = 0; x < 40; x++) {
			if (canvas.Get(x, y) != canvas.Get(39 - x, y)
				|| canvas.Get(x, y) != canvas.Get(x, 19 - y))
				symmetric = false;
		}
	}
	CHECK(symmetric);
	// Roughly pi * a * b.
	int area = CountBlack(canvas);
	CHECK(area > 600 && area < 660);

	// A hollow oval is a ring: the center stays white.
	canvas.Clear();
	canvas.FrameShape(hp::Shape::Oval, {0, 0, 40, 40}, 1, hp::kBlackPattern);
	CHECK(!canvas.Get(20, 20) && canvas.Get(0, 20) && canvas.Get(20, 39));

	// Round rect: square-cornered in the middle of each edge, rounded at
	// the corners.
	canvas.Clear();
	canvas.FillShape(hp::Shape::RoundRect, {0, 0, 50, 50},
		hp::kBlackPattern);
	CHECK(!canvas.Get(0, 0) && !canvas.Get(49, 49));
	CHECK(canvas.Get(25, 0) && canvas.Get(0, 25) && canvas.Get(49, 25));
	CHECK(canvas.Get(9, 0) && canvas.Get(4, 4));
}


static void
TestFillPolygon()
{
	hp::Canvas canvas(30, 30);
	// A square given as a polygon covers the same pixels as the rect.
	canvas.FillPolygon({{5, 5}, {15, 5}, {15, 15}, {5, 15}},
		hp::kBlackPattern);
	CHECK(CountBlack(canvas) == 100);
	CHECK(canvas.Get(5, 5) && canvas.Get(14, 14) && !canvas.Get(15, 15));

	// A right triangle covers about half a square.
	canvas.Clear();
	canvas.FillPolygon({{0, 0}, {20, 0}, {0, 20}}, hp::kBlackPattern);
	int area = CountBlack(canvas);
	CHECK(area >= 190 && area <= 210);
	CHECK(canvas.Get(1, 1) && !canvas.Get(18, 18));

	// Fewer than three points: nothing.
	canvas.Clear();
	canvas.FillPolygon({{0, 0}, {10, 10}}, hp::kBlackPattern);
	CHECK(CountBlack(canvas) == 0);
}


static void
TestStampAndStrokeMask()
{
	hp::Mask16 dot = {};
	dot.rows[8] = 0x0080;	// a single pixel at (8, 8)
	hp::Canvas canvas(40, 40);
	canvas.StampMask(dot, 20, 20, hp::kBlackPattern);
	CHECK(CountBlack(canvas) == 1 && canvas.Get(20, 20));

	// Only masked pixels change: stamping white leaves the rest black.
	hp::Mask16 full;
	for (uint16_t& row : full.rows)
		row = 0xffff;
	canvas.FillRect(0, 0, 39, 39, hp::kBlackPattern);
	canvas.StampMask(dot, 5, 5, hp::kWhitePattern);
	CHECK(CountBlack(canvas) == 40 * 40 - 1 && !canvas.Get(5, 5));
	canvas.StampMask(full, 20, 20, hp::kWhitePattern);
	CHECK(!canvas.Get(12, 12) && !canvas.Get(27, 27));
	CHECK(canvas.Get(11, 11) && canvas.Get(28, 28));

	// Clipped at the edges.
	canvas.Clear();
	canvas.StampMask(full, 0, 0, hp::kBlackPattern);
	CHECK(CountBlack(canvas) == 8 * 8);

	// A stroke stamps each step after the start: a horizontal stroke of
	// a single-pixel mask from x 10 to 20 sets x 11..20.
	canvas.Clear();
	canvas.StrokeMask(dot, 10, 30, 20, 30, hp::kBlackPattern);
	CHECK(CountBlack(canvas) == 10 && !canvas.Get(10, 30)
		&& canvas.Get(20, 30));
	canvas.Clear();
	canvas.StrokeMask(dot, 20, 10, 10, 0, hp::kBlackPattern);
	CHECK(CountBlack(canvas) == 10 && canvas.Get(10, 0)
		&& canvas.Get(15, 5));
}


static void
TestCopyRect()
{
	hp::Canvas snapshot(20, 20);
	snapshot.FillRect(0, 0, 19, 19, hp::kBlackPattern);
	hp::Canvas canvas(20, 20);
	canvas.CopyRect(snapshot, {5, 5, 10, 8});
	CHECK(CountBlack(canvas) == 5 * 3);
	canvas.CopyRect(snapshot, {-5, -5, 100, 100});
	CHECK(CountBlack(canvas) == 400);
}


static void
TestConstraints()
{
	// Square drag.
	hp::Point p = hp::Constrain({10, 10}, {30, 15}, false);
	CHECK(p == (hp::Point{15, 15}));
	p = hp::Constrain({10, 10}, {0, 40}, false);
	CHECK(p == (hp::Point{0, 20}));

	// With allowHV, mostly-horizontal snaps to horizontal.
	p = hp::Constrain({10, 10}, {40, 12}, true);
	CHECK(p == (hp::Point{40, 10}));
	p = hp::Constrain({10, 10}, {11, 50}, true);
	CHECK(p == (hp::Point{10, 50}));
	p = hp::Constrain({10, 10}, {20, 18}, true);
	CHECK(p == (hp::Point{18, 18}));

	// AxisLock waits until the stroke has moved more than a pixel, then
	// locks to that axis.
	hp::AxisLock lock({10, 10});
	CHECK(lock.Apply({11, 11}) == (hp::Point{10, 10}));
	CHECK(lock.Apply({15, 12}) == (hp::Point{15, 10}));
	CHECK(lock.Apply({15, 40}) == (hp::Point{15, 10}));
}


static bool
RoundTrips(const std::vector<uint8_t>& data)
{
	std::vector<uint8_t> packed = hp::PackBits(data.data(), data.size());
	std::vector<uint8_t> unpacked(data.size());
	size_t position = 0;
	return hp::UnpackBits(packed.data(), packed.size(), &position,
			unpacked.data(), unpacked.size())
		&& position == packed.size() && unpacked == data;
}


static void
TestPackBits()
{
	// MacPaint's BlankDoc writes each blank scanline as -71, 0.
	std::vector<uint8_t> blank(72, 0);
	std::vector<uint8_t> packed = hp::PackBits(blank.data(), blank.size());
	CHECK(packed.size() == 2 && (int8_t)packed[0] == -71 && packed[1] == 0);

	// The classic example from Apple's Technical Note TN1023.
	const uint8_t sample[] = {0xAA, 0xAA, 0xAA, 0x80, 0x00, 0x2A, 0xAA, 0xAA,
		0xAA, 0xAA, 0x80, 0x00, 0x2A, 0x22, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
		0xAA, 0xAA, 0xAA, 0xAA};
	const uint8_t tnPacked[] = {0xFE, 0xAA, 0x02, 0x80, 0x00, 0x2A, 0xFD,
		0xAA, 0x03, 0x80, 0x00, 0x2A, 0x22, 0xF7, 0xAA};
	uint8_t out[sizeof(sample)];
	size_t position = 0;
	CHECK(hp::UnpackBits(tnPacked, sizeof(tnPacked), &position, out,
		sizeof(out)));
	CHECK(std::memcmp(out, sample, sizeof(sample)) == 0);
	CHECK(RoundTrips(std::vector<uint8_t>(sample, sample + sizeof(sample))));

	// Runs and literals longer than one packet, and random data.
	CHECK(RoundTrips(std::vector<uint8_t>(300, 0x55)));
	std::mt19937 random(1984);
	for (int trial = 0; trial < 200; trial++) {
		std::vector<uint8_t> data(random() % 400);
		for (uint8_t& byte : data)
			byte = random() % 4 == 0 ? 0 : (uint8_t)random();
		if (!RoundTrips(data)) {
			std::fprintf(stderr, "PackBits round trip failed, trial %d\n",
				trial);
			sFailures++;
			break;
		}
	}

	// Truncated input fails instead of reading past the end.
	position = 0;
	CHECK(!hp::UnpackBits(tnPacked, 5, &position, out, sizeof(out)));
}


static void
TestMacPaintFile()
{
	hp::Canvas canvas;
	canvas.FillShape(hp::Shape::Oval, {100, 100, 300, 250}, hp::kPatterns[3]);
	canvas.DrawLine(0, 0, 575, 719, true);
	canvas.Set(575, 0, true);

	std::vector<uint8_t> file = hp::WriteMacPaint(canvas, hp::kPatterns);
	CHECK(file.size() > 512 && file[3] == 2);
	CHECK(std::memcmp(&file[4], hp::kPatterns, 38 * 8) == 0);

	hp::Canvas loaded;
	std::string error;
	CHECK(hp::ReadMacPaint(file.data(), file.size(), loaded, &error));
	CHECK(loaded.Pixels() == canvas.Pixels());

	// A blank document is exactly BlankDoc's 512 + 720 * 2 bytes.
	hp::Canvas blank;
	CHECK(hp::WriteMacPaint(blank, hp::kPatterns).size() == 512 + 720 * 2);

	// With a MacBinary header in front.
	std::vector<uint8_t> macBinary(128, 0);
	macBinary[1] = 5;
	std::memcpy(&macBinary[2], "Hello", 5);
	std::memcpy(&macBinary[65], "PNTGMPNT", 8);
	macBinary.insert(macBinary.end(), file.begin(), file.end());
	CHECK(hp::MacPaintDataOffset(macBinary.data(), macBinary.size()) == 128);
	hp::Canvas fromMacBinary;
	CHECK(hp::ReadMacPaint(macBinary.data(), macBinary.size(), fromMacBinary));
	CHECK(fromMacBinary.Pixels() == canvas.Pixels());
	CHECK(hp::MacPaintDataOffset(file.data(), file.size()) == 0);

	// Damaged and foreign files are rejected.
	hp::Canvas scratch;
	CHECK(!hp::ReadMacPaint(file.data(), file.size() / 2, scratch, &error));
	CHECK(!error.empty());
	CHECK(!hp::ReadMacPaint(file.data(), 100, scratch));
	std::vector<uint8_t> png = file;
	std::memcpy(png.data(), "\x89PNG", 4);
	CHECK(!hp::ReadMacPaint(png.data(), png.size(), scratch));
}


static void
TestRectSelection()
{
	hp::Canvas page(40, 40);
	page.FillRect(10, 10, 14, 14, hp::kBlackPattern);

	// Clipped to the page; everything inside is selected, white too.
	hp::Selection selection = hp::SelectRect(page, {8, 8, 50, 20});
	CHECK(selection.rect.left == 8 && selection.rect.right == 40);
	CHECK(!selection.lasso && selection.Contains({30, 15}));
	CHECK(selection.pixels.Get(2, 2) && !selection.pixels.Get(0, 0));

	// Moving: erase under, then stamp at the new place, opaquely.
	hp::Canvas moved = page;
	hp::Selection small = hp::SelectRect(page, {10, 10, 15, 15});
	hp::EraseUnder(moved, small);
	CHECK(CountBlack(moved) == 0);
	small.rect = {20, 20, 25, 25};
	hp::Stamp(moved, small);
	CHECK(CountBlack(moved) == 25 && moved.Get(20, 20) && moved.Get(24, 24));

	// Edges of a rect selection are its frame.
	CHECK(small.Edges().size() == 16);

	CHECK(hp::SelectRect(page, {50, 50, 60, 60}).rect.IsEmpty());
}


static void
TestLassoShrinksToFit()
{
	// A ring with a white hole, a separate dot, on white.
	hp::Canvas page(60, 60);
	page.FrameShape(hp::Shape::Rect, {10, 10, 20, 20}, 2, hp::kBlackPattern);
	page.Set(40, 40, true);

	// A loose loop around the ring only.
	hp::Selection selection;
	CHECK(hp::SelectLasso(page, {{2, 2}, {30, 2}, {30, 30}, {2, 30}},
		&selection));
	CHECK(selection.lasso);
	// Shrunk to the ring's bounds.
	CHECK(selection.rect.left == 10 && selection.rect.top == 10
		&& selection.rect.right == 20 && selection.rect.bottom == 20);
	// The enclosed white hole is selected too; the dot outside is not.
	CHECK(selection.Contains({15, 15}) && selection.Contains({10, 10}));
	CHECK(!selection.Contains({40, 40}));

	// Moving it is transparent outside the mask: a lasso selection of a
	// diagonal line doesn't carry white over its neighbors.
	hp::Canvas diagonal(30, 30);
	diagonal.DrawLine(0, 0, 9, 9, true);
	hp::Selection line;
	CHECK(hp::SelectLasso(diagonal, {{0, 0}, {12, 0}, {12, 12}, {0, 12}},
		&line));
	hp::Canvas target(30, 30);
	target.FillRect(0, 0, 29, 29, hp::kBlackPattern);
	line.rect = {15, 15, 15 + line.rect.Width(), 15 + line.rect.Height()};
	hp::Stamp(target, line);
	CHECK(CountBlack(target) == 30 * 30);

	// A loop around nothing selects nothing; a click isn't a loop.
	hp::Canvas empty(30, 30);
	CHECK(!hp::SelectLasso(empty, {{1, 1}, {20, 1}, {20, 20}}, &selection));
	CHECK(!hp::SelectLasso(page, {{5, 5}}, &selection));
}


static void
TestSelectionTransforms()
{
	// An L shape in a 4 x 3 selection:
	//   #...
	//   #...
	//   ##..
	hp::Canvas page(20, 20);
	page.Set(0, 0, true);
	page.Set(0, 1, true);
	page.Set(0, 2, true);
	page.Set(1, 2, true);
	hp::Selection base = hp::SelectRect(page, {0, 0, 4, 3});

	hp::Selection flipped = base;
	hp::FlipHorizontal(flipped);
	CHECK(flipped.pixels.Get(3, 0) && flipped.pixels.Get(2, 2)
		&& !flipped.pixels.Get(0, 0));
	hp::FlipVertical(flipped);
	CHECK(flipped.pixels.Get(3, 2) && flipped.pixels.Get(2, 0));

	// Rotating left: 4 x 3 becomes 3 x 4 around the same center, and the
	// bottom row (##..) becomes the left column, read upward.
	hp::Selection rotated = base;
	hp::RotateLeft(rotated);
	CHECK(rotated.rect.Width() == 3 && rotated.rect.Height() == 4);
	CHECK(rotated.pixels.Get(0, 3) && rotated.pixels.Get(1, 3)
		&& rotated.pixels.Get(2, 3) && rotated.pixels.Get(2, 2));
	CHECK(!rotated.pixels.Get(0, 0));
	hp::Selection fourTimes = base;
	for (int i = 0; i < 4; i++)
		hp::RotateLeft(fourTimes);
	CHECK(fourTimes.pixels.Pixels() == base.pixels.Pixels());

	hp::Selection inverted = base;
	hp::Invert(inverted);
	CHECK(!inverted.pixels.Get(0, 0) && inverted.pixels.Get(3, 0));

	hp::Selection filled = base;
	hp::Fill(filled, hp::kBlackPattern);
	CHECK(CountBlack(filled.pixels) == 12);

	// Stretching doubles each pixel.
	hp::Selection stretched = hp::Stretched(base, {0, 0, 8, 6});
	CHECK(stretched.pixels.Get(0, 0) && stretched.pixels.Get(1, 1)
		&& stretched.pixels.Get(3, 5) && !stretched.pixels.Get(4, 5));

	hp::Rect mapped = hp::MapRect({10, 10, 20, 20}, {0, 0, 10, 10},
		{0, 0, 20, 40});
	CHECK(mapped.left == 20 && mapped.top == 40 && mapped.right == 40
		&& mapped.bottom == 80);
}


static void
TestTraceEdges()
{
	// A 2 x 2 black square in the middle of a rect selection becomes a
	// ring around where it was.
	hp::Canvas page(10, 10);
	page.FillRect(4, 4, 5, 5, hp::kBlackPattern);
	hp::Selection selection = hp::SelectRect(page, {0, 0, 10, 10});
	hp::TraceEdges(selection, false);
	CHECK(!selection.pixels.Get(4, 4) && !selection.pixels.Get(5, 5));
	CHECK(selection.pixels.Get(3, 4) && selection.pixels.Get(6, 5)
		&& selection.pixels.Get(4, 3) && selection.pixels.Get(5, 6));
	CHECK(!selection.pixels.Get(7, 4) && !selection.pixels.Get(2, 4));

	// The shadow version reaches two pixels right and down.
	hp::Selection shadowed = hp::SelectRect(page, {0, 0, 10, 10});
	hp::TraceEdges(shadowed, true);
	CHECK(shadowed.pixels.Get(7, 4) && shadowed.pixels.Get(4, 7));
	CHECK(!shadowed.pixels.Get(2, 4));

	// A lasso selection grows to make room for the outline.
	hp::Selection lasso;
	CHECK(hp::SelectLasso(page, {{1, 1}, {8, 1}, {8, 8}, {1, 8}}, &lasso));
	CHECK(lasso.rect.Width() == 2);
	hp::TraceEdges(lasso, false);
	CHECK(lasso.rect.Width() == 4 && lasso.Contains({3, 4})
		&& lasso.Contains({6, 5}));
}


static void
TestSelectionClipboardFormat()
{
	hp::Canvas page(30, 30);
	page.FrameShape(hp::Shape::Oval, {2, 2, 20, 14}, 1, hp::kBlackPattern);
	hp::Selection selection;
	CHECK(hp::SelectLasso(page, {{0, 0}, {25, 0}, {25, 20}, {0, 20}},
		&selection));

	std::vector<uint8_t> data = hp::EncodeSelection(selection);
	hp::Selection decoded;
	CHECK(hp::DecodeSelection(data.data(), data.size(), &decoded));
	CHECK(decoded.lasso && decoded.rect.left == 0
		&& decoded.rect.Width() == selection.rect.Width()
		&& decoded.rect.Height() == selection.rect.Height());
	CHECK(decoded.pixels.Pixels() == selection.pixels.Pixels());
	CHECK(decoded.mask.Pixels() == selection.mask.Pixels());

	CHECK(!hp::DecodeSelection(data.data(), data.size() - 1, &decoded));
	data[0] = 'X';
	CHECK(!hp::DecodeSelection(data.data(), data.size(), &decoded));
}


int
main()
{
	TestNewCanvasIsWhite();
	TestOutOfBoundsIsSafe();
	TestLines();
	TestFloodFillStopsAtBorder();
	TestFloodFillWithPatternTerminates();
	TestFillRect();
	TestStrokeLineSquarePen();
	TestEraserStroke();
	TestPatternsAreDistinct();
	TestFillAndFrameRect();
	TestOvalAndRoundRect();
	TestFillPolygon();
	TestStampAndStrokeMask();
	TestCopyRect();
	TestConstraints();
	TestPackBits();
	TestMacPaintFile();
	TestRectSelection();
	TestLassoShrinksToFit();
	TestSelectionTransforms();
	TestTraceEdges();
	TestSelectionClipboardFormat();

	if (sFailures != 0) {
		std::fprintf(stderr, "%d check(s) failed\n", sFailures);
		return EXIT_FAILURE;
	}
	std::printf("All engine tests passed.\n");
	return EXIT_SUCCESS;
}
