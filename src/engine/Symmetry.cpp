#include "Symmetry.h"


namespace hp {


uint8_t
MapSymmetry(bool vertical, bool horizontal, bool diagonal, bool antiDiagonal)
{
	// MapSym's table, indexed by vh * 8 + hv * 4 + v * 2 + h. (MacPaint's
	// hSym mirrors across the vertical line, vSym across the horizontal.)
	static const uint8_t kTable[16] = {
		0x80, 0xC0, 0xA0, 0xF0,
		0x88, 0xFF, 0xFF, 0xFF,
		0x81, 0xFF, 0xFF, 0xFF,
		0x99, 0xFF, 0xFF, 0xFF
	};
	int index = (antiDiagonal ? 8 : 0) + (diagonal ? 4 : 0)
		+ (horizontal ? 2 : 0) + (vertical ? 1 : 0);
	return kTable[index];
}


std::vector<Point>
SymmetricPoints(Point point, Point center, uint8_t symmetry)
{
	int h = point.x - center.x;
	int v = point.y - center.y;

	// One reflection per bit, from bit 7 (the point itself) down.
	const Point reflections[8] = {
		{h, v}, {-h, v}, {h, -v}, {-h, -v},
		{v, h}, {-v, h}, {v, -h}, {-v, -h}
	};

	std::vector<Point> points;
	points.push_back(point);
	for (int bit = 1; bit < 8; bit++) {
		if (symmetry & (0x80 >> bit)) {
			points.push_back({center.x + reflections[bit].x,
				center.y + reflections[bit].y});
		}
	}
	return points;
}


}	// namespace hp
