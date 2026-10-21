// Host-side unit tests for the drawing engine. No framework on purpose:
// it must build with nothing but a C++17 compiler, on macOS, Linux or Haiku.

#include "Canvas.h"

#include <cstdio>
#include <cstdlib>


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


int
main()
{
	TestNewCanvasIsWhite();
	TestOutOfBoundsIsSafe();
	TestLines();
	TestFloodFillStopsAtBorder();
	TestFloodFillWithPatternTerminates();

	if (sFailures != 0) {
		std::fprintf(stderr, "%d check(s) failed\n", sFailures);
		return EXIT_FAILURE;
	}
	std::printf("All engine tests passed.\n");
	return EXIT_SUCCESS;
}
