#include "Dither.h"

#include <algorithm>


namespace hp {


void
DitherGray(const std::vector<uint8_t>& gray, int width, int height,
	Canvas& canvas)
{
	canvas.Clear();
	if (width <= 0 || height <= 0
		|| gray.size() < (size_t)width * height)
		return;

	// Work in 1/16ths so the 7, 3, 5 and 1 weights stay integers. Two rows
	// of error, with a pixel of margin on each side.
	std::vector<int> current(width + 2, 0);
	std::vector<int> next(width + 2, 0);

	for (int y = 0; y < height; y++) {
		bool leftToRight = y % 2 == 0;
		int step = leftToRight ? 1 : -1;
		std::fill(next.begin(), next.end(), 0);

		for (int i = 0; i < width; i++) {
			int x = leftToRight ? i : width - 1 - i;
			int value = gray[(size_t)y * width + x] * 16 + current[x + 1];
			bool black = value < 128 * 16;
			int error = value - (black ? 0 : 255 * 16);
			if (x < canvas.Width() && y < canvas.Height())
				canvas.Set(x, y, black);

			// Spread the error ahead on this row and onto the next one.
			current[x + 1 + step] += error * 7 / 16;
			next[x + 1 - step] += error * 3 / 16;
			next[x + 1] += error * 5 / 16;
			next[x + 1 + step] += error * 1 / 16;
		}
		current.swap(next);
	}
}


}	// namespace hp
