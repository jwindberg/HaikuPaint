#include "Canvas.h"

#include <algorithm>
#include <cstdlib>
#include <utility>


namespace hp {


const Pattern kBlackPattern
	= {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
const Pattern kWhitePattern
	= {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};


Canvas::Canvas(int width, int height)
	:
	fWidth(width),
	fHeight(height),
	fPixels(size_t(width) * height, 0)
{
}


bool
Canvas::Contains(int x, int y) const
{
	return x >= 0 && y >= 0 && x < fWidth && y < fHeight;
}


bool
Canvas::Get(int x, int y) const
{
	return Contains(x, y) && fPixels[_Index(x, y)] != 0;
}


void
Canvas::Set(int x, int y, bool black)
{
	if (Contains(x, y))
		fPixels[_Index(x, y)] = black ? 1 : 0;
}


void
Canvas::Clear()
{
	std::fill(fPixels.begin(), fPixels.end(), 0);
}


void
Canvas::DrawLine(int x0, int y0, int x1, int y1, bool black)
{
	// Bresenham, all octants.
	int dx = std::abs(x1 - x0);
	int dy = -std::abs(y1 - y0);
	int sx = x0 < x1 ? 1 : -1;
	int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;

	for (;;) {
		Set(x0, y0, black);
		if (x0 == x1 && y0 == y1)
			break;
		int e2 = 2 * err;
		if (e2 >= dy) {
			err += dy;
			x0 += sx;
		}
		if (e2 <= dx) {
			err += dx;
			y0 += sy;
		}
	}
}


void
Canvas::FloodFill(int x, int y, const Pattern& pattern)
{
	if (!Contains(x, y))
		return;

	// Find the whole region first, then paint it. Painting while searching
	// would loop forever when the pattern contains the seed color.
	const uint8_t target = fPixels[_Index(x, y)];
	std::vector<uint8_t> inRegion(fPixels.size(), 0);
	std::vector<std::pair<int, int>> stack;
	stack.push_back({x, y});

	while (!stack.empty()) {
		auto [px, py] = stack.back();
		stack.pop_back();
		if (!Contains(px, py))
			continue;
		size_t index = _Index(px, py);
		if (inRegion[index] || fPixels[index] != target)
			continue;
		inRegion[index] = 1;
		stack.push_back({px + 1, py});
		stack.push_back({px - 1, py});
		stack.push_back({px, py + 1});
		stack.push_back({px, py - 1});
	}

	for (int py = 0; py < fHeight; py++) {
		for (int px = 0; px < fWidth; px++) {
			size_t index = _Index(px, py);
			if (inRegion[index])
				fPixels[index] = pattern.Bit(px, py) ? 1 : 0;
		}
	}
}


}	// namespace hp
