#ifndef BITMAP_UTILS_H
#define BITMAP_UTILS_H

#include <SupportDefs.h>

#include "Canvas.h"


class BBitmap;
class BCursor;
struct Glyph;


static const uint32 kBlackPixel = 0xff000000;
static const uint32 kWhitePixel = 0xffffffff;


void		SetBitmapPixel(BBitmap* bitmap, int x, int y, bool black);

// A B_RGB32 copy of the canvas; the caller owns it.
BBitmap*	NewBitmapFromCanvas(const hp::Canvas& canvas);

// Draws a PaintFont glyph with the pen at (penX, top); see PaintFont.h.
// Like QuickDraw's srcOr, or srcXor when invert is true.
void		DrawGlyph(hp::Canvas& canvas, const Glyph& glyph, int penX,
				int top, bool invert = false);

// Rectangle helpers with QuickDraw semantics: right and bottom are
// exclusive, so MacPaint's coordinates can be used as they are.
void		PaintQDRect(hp::Canvas& canvas, int left, int top, int right,
				int bottom, const hp::Pattern& pattern = hp::kBlackPattern);
void		FrameQDRect(hp::Canvas& canvas, int left, int top, int right,
				int bottom);
void		InvertQDRect(hp::Canvas& canvas, int left, int top, int right,
				int bottom);

// A palette's white box with a black frame and MacPaint's one pixel drop
// shadow; the canvas must be one pixel larger than the box each way.
void		DrawPaletteBox(hp::Canvas& canvas);

// A PaintFont glyph drawn at pen (0, 0) into 16 x 16 bits, the way MacPaint
// builds its cursors; brushes paint through these.
hp::Mask16	MaskFromGlyph(int code);

// A 16 x 16 cursor from PaintFont glyphs drawn at pen (0, 0), as MacPaint's
// SetToolCursor does. mask may be NULL. Pixels set in data but not in mask
// inverted the screen on the Mac; they show black here.
BCursor*	NewGlyphCursor(int dataCode, int maskCode, int hotX, int hotY);


#endif	// BITMAP_UTILS_H
