#include "BitmapUtils.h"

#include <Bitmap.h>


void
SetBitmapPixel(BBitmap* bitmap, int x, int y, bool black)
{
	uint8* bits = (uint8*)bitmap->Bits();
	uint32* row = (uint32*)(bits + y * bitmap->BytesPerRow());
	row[x] = black ? kBlackPixel : kWhitePixel;
}


BBitmap*
NewPatternBitmap(const hp::Pattern& pattern, int size)
{
	BBitmap* bitmap = new BBitmap(BRect(0, 0, size - 1, size - 1), B_RGB32);
	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++)
			SetBitmapPixel(bitmap, x, y, pattern.Bit(x, y));
	}
	return bitmap;
}


BBitmap*
NewBitmapFromArt(const char* const* rows, int width, int height)
{
	BBitmap* bitmap = new BBitmap(BRect(0, 0, width - 1, height - 1),
		B_RGB32);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++)
			SetBitmapPixel(bitmap, x, y, rows[y][x] == '#');
	}
	return bitmap;
}
