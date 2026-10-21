#include "Selection.h"

#include <cstring>
#include <utility>


namespace hp {


Selection::Selection()
	:
	rect({0, 0, 0, 0}),
	pixels(0, 0),
	mask(0, 0),
	lasso(false)
{
}


bool
Selection::Contains(Point point) const
{
	return point.x >= rect.left && point.x < rect.right
		&& point.y >= rect.top && point.y < rect.bottom
		&& mask.Get(point.x - rect.left, point.y - rect.top);
}


std::vector<Point>
Selection::Edges() const
{
	std::vector<Point> edges;
	for (int y = 0; y < mask.Height(); y++) {
		for (int x = 0; x < mask.Width(); x++) {
			if (mask.Get(x, y) && (!mask.Get(x - 1, y) || !mask.Get(x + 1, y)
					|| !mask.Get(x, y - 1) || !mask.Get(x, y + 1)))
				edges.push_back({rect.left + x, rect.top + y});
		}
	}
	return edges;
}


// A copy of rect from the page; white where rect is off the page.
static Canvas
CopyRegion(const Canvas& page, Rect rect)
{
	Canvas region(rect.Width(), rect.Height());
	for (int y = 0; y < rect.Height(); y++) {
		for (int x = 0; x < rect.Width(); x++)
			region.Set(x, y, page.Get(rect.left + x, rect.top + y));
	}
	return region;
}


Selection
SelectRect(const Canvas& page, Rect rect)
{
	Rect clipped = {std::max(rect.left, 0), std::max(rect.top, 0),
		std::min(rect.right, page.Width()), std::min(rect.bottom,
			page.Height())};

	Selection selection;
	if (clipped.IsEmpty())
		return selection;

	selection.rect = clipped;
	selection.pixels = CopyRegion(page, clipped);
	selection.mask = Canvas(clipped.Width(), clipped.Height());
	selection.mask.FillRect(0, 0, clipped.Width() - 1, clipped.Height() - 1,
		kBlackPattern);
	return selection;
}


bool
SelectLasso(const Canvas& page, const std::vector<Point>& loop,
	Selection* selection)
{
	if (loop.size() < 3)
		return false;

	// The pixels the loop covers, grown by one so the corner is outside it.
	Rect bounds = {loop[0].x, loop[0].y, loop[0].x, loop[0].y};
	for (const Point& point : loop) {
		bounds.left = std::min(bounds.left, point.x);
		bounds.top = std::min(bounds.top, point.y);
		bounds.right = std::max(bounds.right, point.x);
		bounds.bottom = std::max(bounds.bottom, point.y);
	}
	bounds = {bounds.left - 1, bounds.top - 1, bounds.right + 2,
		bounds.bottom + 2};

	// The region inside the loop, including the loop itself.
	std::vector<Point> local;
	for (const Point& point : loop)
		local.push_back({point.x - bounds.left, point.y - bounds.top});
	Canvas region(bounds.Width(), bounds.Height());
	region.FillPolygon(local, kBlackPattern);
	region.StrokePolyline(local, true, 1, kBlackPattern);

	// The page inside the region, on white.
	Canvas work(bounds.Width(), bounds.Height());
	for (int y = 0; y < work.Height(); y++) {
		for (int x = 0; x < work.Width(); x++) {
			if (region.Get(x, y))
				work.Set(x, y, page.Get(bounds.left + x, bounds.top + y));
		}
	}

	// CalcMask: flood the white reachable from the corner; everything it
	// can't reach is selected. Filling black from the corner marks it.
	Canvas reached = work;
	reached.FloodFill(0, 0, kBlackPattern);

	Rect trimmed = {work.Width(), work.Height(), 0, 0};
	for (int y = 0; y < work.Height(); y++) {
		for (int x = 0; x < work.Width(); x++) {
			if (reached.Get(x, y) && !work.Get(x, y))
				continue;
			trimmed.left = std::min(trimmed.left, x);
			trimmed.top = std::min(trimmed.top, y);
			trimmed.right = std::max(trimmed.right, x + 1);
			trimmed.bottom = std::max(trimmed.bottom, y + 1);
		}
	}
	if (trimmed.IsEmpty())
		return false;

	Selection result;
	result.rect = {bounds.left + trimmed.left, bounds.top + trimmed.top,
		bounds.left + trimmed.right, bounds.top + trimmed.bottom};
	result.pixels = CopyRegion(page, result.rect);
	result.mask = Canvas(trimmed.Width(), trimmed.Height());
	for (int y = 0; y < trimmed.Height(); y++) {
		for (int x = 0; x < trimmed.Width(); x++) {
			int wx = trimmed.left + x;
			int wy = trimmed.top + y;
			result.mask.Set(x, y, !reached.Get(wx, wy) || work.Get(wx, wy));
		}
	}
	result.lasso = true;
	*selection = std::move(result);
	return true;
}


void
Stamp(Canvas& page, const Selection& selection)
{
	for (int y = 0; y < selection.rect.Height(); y++) {
		for (int x = 0; x < selection.rect.Width(); x++) {
			if (selection.mask.Get(x, y)) {
				page.Set(selection.rect.left + x, selection.rect.top + y,
					selection.pixels.Get(x, y));
			}
		}
	}
}


void
EraseUnder(Canvas& page, const Selection& selection)
{
	for (int y = 0; y < selection.rect.Height(); y++) {
		for (int x = 0; x < selection.rect.Width(); x++) {
			if (selection.mask.Get(x, y))
				page.Set(selection.rect.left + x, selection.rect.top + y,
					false);
		}
	}
}


void
Invert(Selection& selection)
{
	for (int y = 0; y < selection.rect.Height(); y++) {
		for (int x = 0; x < selection.rect.Width(); x++) {
			if (selection.mask.Get(x, y))
				selection.pixels.Set(x, y, !selection.pixels.Get(x, y));
		}
	}
}


void
Fill(Selection& selection, const Pattern& pattern)
{
	// Aligned to the page, like every other pattern fill.
	for (int y = 0; y < selection.rect.Height(); y++) {
		for (int x = 0; x < selection.rect.Width(); x++) {
			if (selection.mask.Get(x, y)) {
				selection.pixels.Set(x, y, pattern.Bit(
					selection.rect.left + x, selection.rect.top + y));
			}
		}
	}
}


// The canvas with pixel (x, y) taken from (map(x, y)) of the source.
template<typename Map>
static Canvas
Remapped(const Canvas& source, int width, int height, Map map)
{
	Canvas result(width, height);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			Point from = map(x, y);
			result.Set(x, y, source.Get(from.x, from.y));
		}
	}
	return result;
}


void
FlipHorizontal(Selection& selection)
{
	int width = selection.rect.Width();
	int height = selection.rect.Height();
	auto map = [width](int x, int y) { return Point{width - 1 - x, y}; };
	selection.pixels = Remapped(selection.pixels, width, height, map);
	selection.mask = Remapped(selection.mask, width, height, map);
}


void
FlipVertical(Selection& selection)
{
	int width = selection.rect.Width();
	int height = selection.rect.Height();
	auto map = [height](int x, int y) { return Point{x, height - 1 - y}; };
	selection.pixels = Remapped(selection.pixels, width, height, map);
	selection.mask = Remapped(selection.mask, width, height, map);
}


void
RotateLeft(Selection& selection)
{
	// Counterclockwise, as MacPaint's Rotate: the right edge becomes the
	// top. New pixel (x, y) comes from old (width - 1 - y, x).
	int width = selection.rect.Width();
	int height = selection.rect.Height();
	auto map = [width](int x, int y) { return Point{width - 1 - y, x}; };
	selection.pixels = Remapped(selection.pixels, height, width, map);
	selection.mask = Remapped(selection.mask, height, width, map);

	Rect& rect = selection.rect;
	int left = rect.left + (width - height) / 2;
	int top = rect.top + (height - width) / 2;
	rect = {left, top, left + height, top + width};
}


// Grows the selection by the given margins, unselected and white.
static void
Pad(Selection& selection, int left, int top, int right, int bottom)
{
	const Rect& old = selection.rect;
	Rect rect = {old.left - left, old.top - top, old.right + right,
		old.bottom + bottom};
	Canvas pixels(rect.Width(), rect.Height());
	Canvas mask(rect.Width(), rect.Height());
	for (int y = 0; y < old.Height(); y++) {
		for (int x = 0; x < old.Width(); x++) {
			pixels.Set(left + x, top + y, selection.pixels.Get(x, y));
			mask.Set(left + x, top + y, selection.mask.Get(x, y));
		}
	}
	selection.rect = rect;
	selection.pixels = std::move(pixels);
	selection.mask = std::move(mask);
}


// One of TraceEdges' CopyBits(..., srcOr) smears: ORs the source area of
// the canvas into itself, offset by (dx, dy).
static void
Smear(Canvas& canvas, const Rect& source, int dx, int dy)
{
	Canvas copy = canvas;
	for (int y = source.top; y < source.bottom; y++) {
		for (int x = source.left; x < source.right; x++) {
			if (copy.Get(x, y))
				canvas.Set(x + dx, y + dy, true);
		}
	}
}


void
TraceEdges(Selection& selection, bool shadow)
{
	int delta = shadow ? 3 : 2;

	if (selection.lasso) {
		// Only what's selected gets outlined, and the outline needs room
		// around it.
		for (int y = 0; y < selection.rect.Height(); y++) {
			for (int x = 0; x < selection.rect.Width(); x++) {
				if (!selection.mask.Get(x, y))
					selection.pixels.Set(x, y, false);
			}
		}
		Pad(selection, 1, 1, delta - 1, delta - 1);
	}

	int width = selection.rect.Width();
	int height = selection.rect.Height();
	Rect source = {1, 1, width - delta + 1, height - delta + 1};
	if (source.IsEmpty())
		return;

	Canvas original = selection.pixels;
	Canvas& pixels = selection.pixels;
	Smear(pixels, source, -1, 0);
	Smear(pixels, source, delta - 1, 0);
	Smear(pixels, source, 0, -1);
	Smear(pixels, source, 0, delta - 1);

	// Punch out the original, leaving the outline.
	for (int y = source.top; y < source.bottom; y++) {
		for (int x = source.left; x < source.right; x++) {
			if (original.Get(x, y))
				pixels.Set(x, y, false);
		}
	}

	if (selection.lasso) {
		Smear(selection.mask, source, -1, 0);
		Smear(selection.mask, source, delta - 1, 0);
		Smear(selection.mask, source, 0, -1);
		Smear(selection.mask, source, 0, delta - 1);
	}
}


Selection
Stretched(const Selection& source, Rect rect)
{
	Selection result;
	if (rect.IsEmpty() || source.rect.IsEmpty())
		return result;

	int sourceWidth = source.rect.Width();
	int sourceHeight = source.rect.Height();
	auto map = [&](int x, int y) {
		return Point{(int)((2L * x + 1) * sourceWidth / (2L * rect.Width())),
			(int)((2L * y + 1) * sourceHeight / (2L * rect.Height()))};
	};
	result.rect = rect;
	result.pixels = Remapped(source.pixels, rect.Width(), rect.Height(), map);
	result.mask = Remapped(source.mask, rect.Width(), rect.Height(), map);
	result.lasso = source.lasso;
	return result;
}


Rect
MapRect(Rect rect, Rect from, Rect to)
{
	if (from.Width() == 0 || from.Height() == 0)
		return rect;

	auto mapX = [&](int x) {
		return to.left + (x - from.left) * to.Width() / from.Width();
	};
	auto mapY = [&](int y) {
		return to.top + (y - from.top) * to.Height() / from.Height();
	};
	return {mapX(rect.left), mapY(rect.top), mapX(rect.right),
		mapY(rect.bottom)};
}


// "HPSL", width and height (big endian 32 bit), the lasso flag, then one
// byte per pixel and one per mask pixel, row by row.
static const char kSelectionMagic[4] = {'H', 'P', 'S', 'L'};
static const size_t kSelectionHeaderSize = 13;
static const int kMaxSelectionSide = 4096;


static void
PutInt32(std::vector<uint8_t>& out, uint32_t value)
{
	for (int shift = 24; shift >= 0; shift -= 8)
		out.push_back((value >> shift) & 0xff);
}


static uint32_t
GetInt32(const uint8_t* data)
{
	return (uint32_t)data[0] << 24 | (uint32_t)data[1] << 16
		| (uint32_t)data[2] << 8 | data[3];
}


std::vector<uint8_t>
EncodeSelection(const Selection& selection)
{
	std::vector<uint8_t> out(kSelectionMagic, kSelectionMagic + 4);
	PutInt32(out, selection.rect.Width());
	PutInt32(out, selection.rect.Height());
	out.push_back(selection.lasso ? 1 : 0);
	const std::vector<uint8_t>& pixels = selection.pixels.Pixels();
	const std::vector<uint8_t>& mask = selection.mask.Pixels();
	out.insert(out.end(), pixels.begin(), pixels.end());
	out.insert(out.end(), mask.begin(), mask.end());
	return out;
}


bool
DecodeSelection(const uint8_t* data, size_t size, Selection* selection)
{
	if (size < kSelectionHeaderSize
		|| std::memcmp(data, kSelectionMagic, 4) != 0)
		return false;

	uint32_t width = GetInt32(data + 4);
	uint32_t height = GetInt32(data + 8);
	if (width == 0 || height == 0 || width > kMaxSelectionSide
		|| height > kMaxSelectionSide
		|| size != kSelectionHeaderSize + 2 * (size_t)width * height)
		return false;

	Selection result;
	result.rect = {0, 0, (int)width, (int)height};
	result.lasso = data[12] != 0;
	result.pixels = Canvas(width, height);
	result.mask = Canvas(width, height);
	const uint8_t* pixels = data + kSelectionHeaderSize;
	const uint8_t* mask = pixels + width * height;
	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			result.pixels.Set(x, y, pixels[y * width + x] != 0);
			result.mask.Set(x, y, mask[y * width + x] != 0);
		}
	}
	*selection = std::move(result);
	return true;
}


}	// namespace hp
