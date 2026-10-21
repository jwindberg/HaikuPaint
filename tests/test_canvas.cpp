// Host-side unit tests for the drawing engine. No framework on purpose:
// it must build with nothing but a C++17 compiler, on macOS, Linux or Haiku.

#include "Canvas.h"
#include "Patterns.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>


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
	CHECK(std::memcmp(&hp::kPatterns[0], &hp::kWhitePattern,
		sizeof(hp::Pattern)) == 0);
	CHECK(std::memcmp(&hp::kPatterns[hp::kDefaultPatternIndex],
		&hp::kBlackPattern, sizeof(hp::Pattern)) == 0);

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

	if (sFailures != 0) {
		std::fprintf(stderr, "%d check(s) failed\n", sFailures);
		return EXIT_FAILURE;
	}
	std::printf("All engine tests passed.\n");
	return EXIT_SUCCESS;
}
