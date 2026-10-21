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

			// Fills the 4-connected region of same-colored pixels containing
			// (x, y). The pattern is aligned to canvas coordinates, as in
			// MacPaint, so adjacent fills line up seamlessly.
			void				FloodFill(int x, int y,
									const Pattern& pattern);

			const std::vector<uint8_t>& Pixels() const { return fPixels; }

private:
			size_t				_Index(int x, int y) const
									{ return size_t(y) * fWidth + x; }

			int					fWidth;
			int					fHeight;
			std::vector<uint8_t> fPixels;
};


}	// namespace hp

#endif	// HP_CANVAS_H
