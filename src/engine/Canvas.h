/*
 * HaikuPaint drawing engine.
 *
 * Pure C++17: no Haiku headers allowed in src/engine, so it can be built and
 * unit tested on any machine (see tests/).
 */
#ifndef HP_CANVAS_H
#define HP_CANVAS_H

#include <cstdint>
#include <vector>

#include "Geometry.h"


namespace hp {


// 8x8 1-bit pattern, one byte per row, MSB is the leftmost pixel
// (the same layout as a QuickDraw Pattern).
struct Pattern {
	uint8_t rows[8];

	bool Bit(int x, int y) const
		{ return (rows[y & 7] >> (7 - (x & 7))) & 1; }
};

extern const Pattern kBlackPattern;
extern const Pattern kWhitePattern;


// A 16 x 16 1-bit mask (MacPaint's Bits16): one word per row, MSB leftmost.
// Brushes and the spray can paint the current pattern through one.
struct Mask16 {
	uint16_t rows[16];

	bool Bit(int x, int y) const
		{ return (rows[y] >> (15 - x)) & 1; }
};


enum class Shape {
	Rect,
	RoundRect,
	Oval
};

// MacPaint's corner: the diameter of a round rect's corner curves.
static constexpr int kRoundRectCorner = 18;

// The points MacPaint's BrushLine stamps a brush at going from (x0, y0) to
// (x1, y1): one per step, not including the start.
std::vector<Point>	MaskStrokePoints(int x0, int y0, int x1, int y1);


// A 1-bit image. true/1 is black, false/0 is white.
class Canvas {
public:
	// A full MacPaint document is 576 x 720 pixels (8 x 10 inches at 72 dpi).
	static constexpr int kDefaultWidth = 576;
	static constexpr int kDefaultHeight = 720;

								Canvas(int width = kDefaultWidth,
									int height = kDefaultHeight);

			int					Width() const { return fWidth; }
			int					Height() const { return fHeight; }
			bool				Contains(int x, int y) const;

			// Out-of-bounds reads return white; out-of-bounds writes are
			// ignored, so tools can draw past the edges safely.
			bool				Get(int x, int y) const;
			void				Set(int x, int y, bool black);
			void				Clear();

			void				DrawLine(int x0, int y0, int x1, int y1,
									bool black);

			// Fills the rectangle (inclusive coordinates, in any order).
			void				FillRect(int x0, int y0, int x1, int y1,
									const Pattern& pattern);

			// Drags a penSize x penSize square pen along the line. Like a
			// QuickDraw pen, it hangs down and to the right of each point.
			void				StrokeLine(int x0, int y0, int x1, int y1,
									int penSize, const Pattern& pattern);

			// Fills the 4-connected region of same-colored pixels containing
			// (x, y). The pattern is aligned to canvas coordinates, as in
			// MacPaint, so adjacent fills line up seamlessly.
			void				FloodFill(int x, int y,
									const Pattern& pattern);

			// QuickDraw's PaintRect/PaintRoundRect/PaintOval: fills the
			// shape inscribed in rect.
			void				FillShape(Shape shape, const Rect& rect,
									const Pattern& pattern);

			// QuickDraw's FrameRect/FrameRoundRect/FrameOval: the shape
			// minus the same shape inset by penSize, so the border lies
			// inside rect.
			void				FrameShape(Shape shape, const Rect& rect,
									int penSize, const Pattern& pattern);

			// Fills the polygon (implicitly closed) using the even-odd
			// rule, testing pixel centers.
			void				FillPolygon(const std::vector<Point>& points,
									const Pattern& pattern);

			// StrokeLine through consecutive points; also from the last
			// point back to the first when closed.
			void				StrokePolyline(
									const std::vector<Point>& points,
									bool closed, int penSize,
									const Pattern& pattern);

			// MacPaint's DrawBrush: paints the pattern wherever the mask is
			// set, with the mask centered on (centerX, centerY), i.e. its
			// top left at (centerX - 8, centerY - 8). Other pixels are left
			// alone.
			void				StampMask(const Mask16& mask, int centerX,
									int centerY, const Pattern& pattern);

			// MacPaint's BrushLine: stamps along the line from (x0, y0) to
			// (x1, y1), one stamp per step, not including the start.
			void				StrokeMask(const Mask16& mask, int x0, int y0,
									int x1, int y1, const Pattern& pattern);

			// Copies the pixels of rect from source, which must be the same
			// size. Used to restore a snapshot under a live preview.
			void				CopyRect(const Canvas& source, Rect rect);

			// Moves the whole image by (dx, dy), as Show Page's page shift
			// does: what moves off the page is lost, and white comes in.
			void				Offset(int dx, int dy);

			const std::vector<uint8_t>& Pixels() const { return fPixels; }

private:
			size_t				_Index(int x, int y) const
									{ return size_t(y) * fWidth + x; }
			Rect				_Clip(const Rect& rect) const;

			int					fWidth;
			int					fHeight;
			std::vector<uint8_t> fPixels;
};


}	// namespace hp

#endif	// HP_CANVAS_H
