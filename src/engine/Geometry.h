#ifndef HP_GEOMETRY_H
#define HP_GEOMETRY_H

#include <algorithm>
#include <cstdlib>


namespace hp {


struct Point {
	int x;
	int y;

	bool operator==(const Point& other) const
		{ return x == other.x && y == other.y; }
	bool operator!=(const Point& other) const
		{ return !(*this == other); }
};


// QuickDraw semantics: right and bottom are exclusive, so a rect from
// (10, 10) to (20, 20) covers 10 x 10 pixels.
struct Rect {
	int left;
	int top;
	int right;
	int bottom;

	int Width() const { return right - left; }
	int Height() const { return bottom - top; }
	bool IsEmpty() const { return right <= left || bottom <= top; }

	Rect InsetBy(int dx, int dy) const
		{ return {left + dx, top + dy, right - dx, bottom - dy}; }
	Rect Union(const Rect& other) const
	{
		if (IsEmpty())
			return other;
		if (other.IsEmpty())
			return *this;
		return {std::min(left, other.left), std::min(top, other.top),
			std::max(right, other.right), std::max(bottom, other.bottom)};
	}
};


// QuickDraw's Pt2Rect: the rect spanned by two corners, in any order.
inline Rect
RectFromPoints(Point a, Point b)
{
	return {std::min(a.x, b.x), std::min(a.y, b.y), std::max(a.x, b.x),
		std::max(a.y, b.y)};
}


// MacPaint's NearPt: within tolerance horizontally and vertically.
inline bool
NearPoint(Point a, Point b, int tolerance)
{
	return std::abs(a.x - b.x) <= tolerance
		&& std::abs(a.y - b.y) <= tolerance;
}


// MacPaint's Constrain, used with Shift held: makes the drag square (for
// shapes) or diagonal. With allowHV, a drag that is mostly horizontal or
// vertical snaps to that axis instead (for lines and polygon edges).
inline Point
Constrain(Point start, Point p, bool allowHV)
{
	int dx = p.x - start.x;
	int dy = p.y - start.y;
	int absDx = std::abs(dx);
	int absDy = std::abs(dy);
	int delta = std::min(absDx, absDy);
	Point result = {start.x + (dx < 0 ? -delta : delta),
		start.y + (dy < 0 ? -delta : delta)};

	if (allowHV) {
		if (absDx > 2 * absDy)
			result = {p.x, start.y};
		if (absDy > 2 * absDx)
			result = {start.x, p.y};
	}
	return result;
}


// MacPaint's InitConstrain/HVConstrain, used with Shift held by the pencil
// and brushes: locks to whichever axis the stroke first moves along.
class AxisLock {
public:
	explicit AxisLock(Point start = {0, 0})
		:
		fStart(start),
		fHorizontal(true),
		fVertical(true)
	{
	}

	Point Apply(Point p)
	{
		if (fHorizontal && fVertical) {
			// Still choosing a direction.
			int dx = std::abs(p.x - fStart.x);
			int dy = std::abs(p.y - fStart.y);
			if (dx > dy && dx > 1)
				fVertical = false;
			if (dy > dx && dy > 1)
				fHorizontal = false;
		}
		if (fHorizontal)
			p.y = fStart.y;
		if (fVertical)
			p.x = fStart.x;
		return p;
	}

private:
	Point	fStart;
	bool	fHorizontal;
	bool	fVertical;
};


}	// namespace hp

#endif	// HP_GEOMETRY_H
