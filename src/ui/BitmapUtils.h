#ifndef BITMAP_UTILS_H
#define BITMAP_UTILS_H

#include <SupportDefs.h>

#include "Canvas.h"


class BBitmap;


static const uint32 kBlackPixel = 0xff000000;
static const uint32 kWhitePixel = 0xffffffff;


// All bitmaps are B_RGB32; the caller owns them.

// A size x size swatch tiled with the pattern.
BBitmap*	NewPatternBitmap(const hp::Pattern& pattern, int size);

// One string per row, '#' black and anything else white.
BBitmap*	NewBitmapFromArt(const char* const* rows, int width, int height);

void		SetBitmapPixel(BBitmap* bitmap, int x, int y, bool black);


#endif	// BITMAP_UTILS_H
