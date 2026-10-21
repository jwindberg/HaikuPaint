#ifndef HP_SYMMETRY_H
#define HP_SYMMETRY_H

#include <cstdint>
#include <vector>

#include "Geometry.h"


namespace hp {


// MacPaint's Brush Mirrors. Four mirror lines through a center point:
// vertical, horizontal and the two diagonals. MapSym turns the chosen ones
// into a byte saying which of the eight reflections of a point to paint;
// combinations it has no entry for paint all eight.

static constexpr uint8_t kNoSymmetry = 0x80;	// just the point itself

uint8_t		MapSymmetry(bool vertical, bool horizontal, bool diagonal,
				bool antiDiagonal);

// The point and its reflections around center that symmetry selects, in
// MacPaint's SymBrush order, starting with the point itself.
std::vector<Point>	SymmetricPoints(Point point, Point center,
						uint8_t symmetry);


}	// namespace hp

#endif	// HP_SYMMETRY_H
