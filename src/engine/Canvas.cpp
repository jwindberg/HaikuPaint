#include "Canvas.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
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
	fPixels(size_t(width) * height, 0),
	fMode(PaintMode::Copy)
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


// Calls plot(x, y) for each point of a Bresenham line, all octants.
template<typename Plot>
static void
ForEachLinePoint(int x0, int y0, int x1, int y1, Plot plot)
{
	int dx = std::abs(x1 - x0);
	int dy = -std::abs(y1 - y0);
	int sx = x0 < x1 ? 1 : -1;
	int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;

	for (;;) {
		plot(x0, y0);
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
Canvas::DrawLine(int x0, int y0, int x1, int y1, bool black)
{
	ForEachLinePoint(x0, y0, x1, y1,
		[&](int x, int y) { Set(x, y, black); });
}


void
Canvas::FillRect(int x0, int y0, int x1, int y1, const Pattern& pattern)
{
	int left = std::max(0, std::min(x0, x1));
	int right = std::min(fWidth - 1, std::max(x0, x1));
	int top = std::max(0, std::min(y0, y1));
	int bottom = std::min(fHeight - 1, std::max(y0, y1));

	for (int y = top; y <= bottom; y++) {
		for (int x = left; x <= right; x++)
			_Paint(_Index(x, y), pattern.Bit(x, y));
	}
}


void
Canvas::StrokeLine(int x0, int y0, int x1, int y1, int penSize,
	const Pattern& pattern)
{
	if (penSize < 1)
		return;

	ForEachLinePoint(x0, y0, x1, y1, [&](int x, int y) {
		FillRect(x, y, x + penSize - 1, y + penSize - 1, pattern);
	});
}


// Whether the center of pixel (x, y) is inside the ellipse inscribed in rect.
static bool
InsideEllipse(const Rect& rect, int x, int y)
{
	double a = rect.Width() / 2.0;
	double b = rect.Height() / 2.0;
	double dx = (x + 0.5 - (rect.left + a)) / a;
	double dy = (y + 0.5 - (rect.top + b)) / b;
	return dx * dx + dy * dy <= 1.0;
}


// Whether pixel (x, y) is inside the shape inscribed in rect. corner is the
// round rect corner diameter.
static bool
InsideShape(Shape shape, const Rect& rect, int corner, int x, int y)
{
	if (rect.IsEmpty() || x < rect.left || x >= rect.right || y < rect.top
		|| y >= rect.bottom)
		return false;

	switch (shape) {
		case Shape::Rect:
			return true;
		case Shape::Oval:
			return InsideEllipse(rect, x, y);
		case Shape::RoundRect:
		{
			int cornerWidth = std::min(corner, rect.Width());
			int cornerHeight = std::min(corner, rect.Height());
			if (cornerWidth <= 0 || cornerHeight <= 0)
				return true;

			bool left = x < rect.left + cornerWidth / 2;
			bool right = x >= rect.right - cornerWidth / 2;
			bool top = y < rect.top + cornerHeight / 2;
			bool bottom = y >= rect.bottom - cornerHeight / 2;
			if (!(left || right) || !(top || bottom))
				return true;

			// In one of the four corners: test against that corner's oval.
			int cornerLeft = left ? rect.left : rect.right - cornerWidth;
			int cornerTop = top ? rect.top : rect.bottom - cornerHeight;
			return InsideEllipse({cornerLeft, cornerTop,
				cornerLeft + cornerWidth, cornerTop + cornerHeight}, x, y);
		}
	}
	return false;
}


// Floor division, also for negative numerators.
static long
FloorDiv(long numerator, long denominator)
{
	long quotient = numerator / denominator;
	if (numerator % denominator != 0 && (numerator < 0) != (denominator < 0))
		quotient--;
	return quotient;
}


Rect
Canvas::_Clip(const Rect& rect) const
{
	return {std::max(rect.left, 0), std::max(rect.top, 0),
		std::min(rect.right, fWidth), std::min(rect.bottom, fHeight)};
}


void
Canvas::FillShape(Shape shape, const Rect& rect, const Pattern& pattern)
{
	Rect clip = _Clip(rect);
	for (int y = clip.top; y < clip.bottom; y++) {
		for (int x = clip.left; x < clip.right; x++) {
			if (InsideShape(shape, rect, kRoundRectCorner, x, y))
				_Paint(_Index(x, y), pattern.Bit(x, y));
		}
	}
}


void
Canvas::FrameShape(Shape shape, const Rect& rect, int penSize,
	const Pattern& pattern)
{
	// As in QuickDraw, the inner shape's corners shrink with the pen.
	Rect inner = rect.InsetBy(penSize, penSize);
	int innerCorner = std::max(0, kRoundRectCorner - 2 * penSize);

	Rect clip = _Clip(rect);
	for (int y = clip.top; y < clip.bottom; y++) {
		for (int x = clip.left; x < clip.right; x++) {
			if (InsideShape(shape, rect, kRoundRectCorner, x, y)
				&& !InsideShape(shape, inner, innerCorner, x, y))
				_Paint(_Index(x, y), pattern.Bit(x, y));
		}
	}
}


void
Canvas::FillPolygon(const std::vector<Point>& points, const Pattern& pattern)
{
	if (points.size() < 3)
		return;

	int top = points[0].y;
	int bottom = points[0].y;
	for (const Point& point : points) {
		top = std::min(top, point.y);
		bottom = std::max(bottom, point.y);
	}
	top = std::max(top, 0);
	bottom = std::min(bottom, fHeight);

	std::vector<double> crossings;
	for (int y = top; y < bottom; y++) {
		// Where the scanline through the pixel centers crosses each edge.
		double center = y + 0.5;
		crossings.clear();
		for (size_t i = 0; i < points.size(); i++) {
			const Point& a = points[i];
			const Point& b = points[(i + 1) % points.size()];
			if ((a.y <= center) != (b.y <= center)) {
				crossings.push_back(a.x
					+ (center - a.y) * (b.x - a.x) / double(b.y - a.y));
			}
		}
		std::sort(crossings.begin(), crossings.end());

		// Even-odd: fill pixels whose centers lie between pairs.
		for (size_t i = 0; i + 1 < crossings.size(); i += 2) {
			int from = std::max(0, (int)std::ceil(crossings[i] - 0.5));
			int to = std::min(fWidth,
				(int)std::ceil(crossings[i + 1] - 0.5));
			for (int x = from; x < to; x++)
				_Paint(_Index(x, y), pattern.Bit(x, y));
		}
	}
}


void
Canvas::StrokePolyline(const std::vector<Point>& points, bool closed,
	int penSize, const Pattern& pattern)
{
	if (points.empty())
		return;
	if (points.size() == 1) {
		StrokeLine(points[0].x, points[0].y, points[0].x, points[0].y,
			penSize, pattern);
		return;
	}

	for (size_t i = 0; i + 1 < points.size(); i++) {
		StrokeLine(points[i].x, points[i].y, points[i + 1].x,
			points[i + 1].y, penSize, pattern);
	}
	if (closed) {
		StrokeLine(points.back().x, points.back().y, points[0].x,
			points[0].y, penSize, pattern);
	}
}


void
Canvas::StampMask(const Mask16& mask, int centerX, int centerY,
	const Pattern& pattern)
{
	int left = centerX - 8;
	int top = centerY - 8;
	for (int y = 0; y < 16; y++) {
		for (int x = 0; x < 16; x++) {
			if (mask.Bit(x, y) && Contains(left + x, top + y)) {
				_Paint(_Index(left + x, top + y),
					pattern.Bit(left + x, top + y));
			}
		}
	}
}


std::vector<Point>
MaskStrokePoints(int x0, int y0, int x1, int y1)
{
	// MacPaint steps in Fixed point from the center of the start pixel and
	// takes the integer part (HiWord) of each step.
	std::vector<Point> points;
	long dx = x1 - x0;
	long dy = y1 - y0;
	long steps = std::max(std::labs(dx), std::labs(dy));
	for (long step = 1; step <= steps; step++) {
		points.push_back({
			(int)FloorDiv(2L * x0 * steps + steps + 2 * dx * step,
				2 * steps),
			(int)FloorDiv(2L * y0 * steps + steps + 2 * dy * step,
				2 * steps)});
	}
	return points;
}


Canvas
Reduced(const Canvas& source, int factor, int minimumBlack)
{
	Canvas result(source.Width() / factor, source.Height() / factor);
	for (int y = 0; y < result.Height(); y++) {
		for (int x = 0; x < result.Width(); x++) {
			int black = 0;
			for (int dy = 0; dy < factor; dy++) {
				for (int dx = 0; dx < factor; dx++)
					black += source.Get(x * factor + dx, y * factor + dy);
			}
			result.Set(x, y, black >= minimumBlack);
		}
	}
	return result;
}


void
Canvas::StrokeMask(const Mask16& mask, int x0, int y0, int x1, int y1,
	const Pattern& pattern)
{
	for (const Point& point : MaskStrokePoints(x0, y0, x1, y1))
		StampMask(mask, point.x, point.y, pattern);
}


void
Canvas::CopyRect(const Canvas& source, Rect rect)
{
	if (source.fWidth != fWidth || source.fHeight != fHeight)
		return;

	Rect clip = _Clip(rect);
	if (clip.IsEmpty())
		return;
	for (int y = clip.top; y < clip.bottom; y++) {
		std::memcpy(&fPixels[_Index(clip.left, y)],
			&source.fPixels[_Index(clip.left, y)], clip.Width());
	}
}


void
Canvas::Offset(int dx, int dy)
{
	std::vector<uint8_t> moved(fPixels.size(), 0);
	for (int y = 0; y < fHeight; y++) {
		int fromY = y - dy;
		if (fromY < 0 || fromY >= fHeight)
			continue;
		for (int x = 0; x < fWidth; x++) {
			int fromX = x - dx;
			if (fromX >= 0 && fromX < fWidth)
				moved[_Index(x, y)] = fPixels[_Index(fromX, fromY)];
		}
	}
	fPixels.swap(moved);
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
