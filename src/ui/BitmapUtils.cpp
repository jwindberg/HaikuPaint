#include "BitmapUtils.h"

#include <Bitmap.h>
#include <Cursor.h>

#include <algorithm>

#include "PaintFont.h"


static const int kCursorSize = 16;


void
SetBitmapPixel(BBitmap* bitmap, int x, int y, bool black)
{
	uint8* bits = (uint8*)bitmap->Bits();
	uint32* row = (uint32*)(bits + y * bitmap->BytesPerRow());
	row[x] = black ? kBlackPixel : kWhitePixel;
}


BBitmap*
NewBitmapFromCanvas(const hp::Canvas& canvas)
{
	BBitmap* bitmap = new BBitmap(BRect(0, 0, canvas.Width() - 1,
		canvas.Height() - 1), B_RGB32);
	for (int y = 0; y < canvas.Height(); y++) {
		for (int x = 0; x < canvas.Width(); x++)
			SetBitmapPixel(bitmap, x, y, canvas.Get(x, y));
	}
	return bitmap;
}


status_t
CanvasFromBitmap(const BBitmap* source, hp::Canvas& canvas)
{
	BBitmap rgba(source->Bounds(), B_RGBA32);
	status_t status = rgba.ImportBits(source);
	if (status != B_OK)
		return status;

	canvas.Clear();
	int width = std::min((int)source->Bounds().Width() + 1, canvas.Width());
	int height = std::min((int)source->Bounds().Height() + 1,
		canvas.Height());
	const uint8* bits = (const uint8*)rgba.Bits();
	for (int y = 0; y < height; y++) {
		const uint8* pixel = bits + y * rgba.BytesPerRow();
		for (int x = 0; x < width; x++, pixel += 4) {
			// B_RGBA32 is B, G, R, A in memory.
			int luminance = (114 * pixel[0] + 587 * pixel[1]
				+ 299 * pixel[2]) / 1000;
			canvas.Set(x, y, pixel[3] >= 128 && luminance < 128);
		}
	}
	return B_OK;
}


void
DrawGlyph(hp::Canvas& canvas, const Glyph& glyph, int penX, int top,
	bool invert)
{
	int left = penX + glyph.offset;
	for (int y = 0; y < kPaintFontHeight; y++) {
		for (int x = 0; x < glyph.width; x++) {
			if (glyph.rows[y][x] != '#')
				continue;
			bool black = invert ? !canvas.Get(left + x, top + y) : true;
			canvas.Set(left + x, top + y, black);
		}
	}
}


void
PaintQDRect(hp::Canvas& canvas, int left, int top, int right, int bottom,
	const hp::Pattern& pattern)
{
	if (right > left && bottom > top)
		canvas.FillRect(left, top, right - 1, bottom - 1, pattern);
}


void
FrameQDRect(hp::Canvas& canvas, int left, int top, int right, int bottom)
{
	canvas.DrawLine(left, top, right - 1, top, true);
	canvas.DrawLine(left, bottom - 1, right - 1, bottom - 1, true);
	canvas.DrawLine(left, top, left, bottom - 1, true);
	canvas.DrawLine(right - 1, top, right - 1, bottom - 1, true);
}


void
InvertQDRect(hp::Canvas& canvas, int left, int top, int right, int bottom)
{
	for (int y = top; y < bottom; y++) {
		for (int x = left; x < right; x++)
			canvas.Set(x, y, !canvas.Get(x, y));
	}
}


void
DrawPaletteBox(hp::Canvas& canvas)
{
	// OffsetRect(r, 1, 1); PaintRect(r); OffsetRect(r, -1, -1);
	// EraseRect(r); FrameRect(r);
	int right = canvas.Width() - 1;
	int bottom = canvas.Height() - 1;
	PaintQDRect(canvas, 1, 1, right + 1, bottom + 1);
	PaintQDRect(canvas, 0, 0, right, bottom, hp::kWhitePattern);
	FrameQDRect(canvas, 0, 0, right, bottom);
}


hp::Mask16
MaskFromGlyph(int code)
{
	hp::Canvas canvas(kCursorSize, kCursorSize);
	if (const Glyph* glyph = PaintFontGlyph(code))
		DrawGlyph(canvas, *glyph, 0, 0);

	hp::Mask16 mask = {};
	for (int y = 0; y < kCursorSize; y++) {
		for (int x = 0; x < kCursorSize; x++) {
			if (canvas.Get(x, y))
				mask.rows[y] |= 0x8000 >> x;
		}
	}
	return mask;
}


BCursor*
NewGlyphCursor(int dataCode, int maskCode, int hotX, int hotY)
{
	hp::Canvas data(kCursorSize, kCursorSize);
	hp::Canvas mask(kCursorSize, kCursorSize);
	if (const Glyph* glyph = PaintFontGlyph(dataCode))
		DrawGlyph(data, *glyph, 0, 0);
	if (const Glyph* glyph = PaintFontGlyph(maskCode))
		DrawGlyph(mask, *glyph, 0, 0);

	BBitmap bitmap(BRect(0, 0, kCursorSize - 1, kCursorSize - 1), B_RGBA32);
	uint8* bits = (uint8*)bitmap.Bits();
	for (int y = 0; y < kCursorSize; y++) {
		uint32* row = (uint32*)(bits + y * bitmap.BytesPerRow());
		for (int x = 0; x < kCursorSize; x++) {
			if (data.Get(x, y))
				row[x] = kBlackPixel;
			else if (mask.Get(x, y))
				row[x] = kWhitePixel;
			else
				row[x] = 0;		// transparent
		}
	}

	// The Mac allowed a hotspot one past the edge (the pencil's is at
	// v = 16); clamp it onto the cursor.
	BPoint hotspot(std::min(hotX, kCursorSize - 1),
		std::min(hotY, kCursorSize - 1));
	return new BCursor(&bitmap, hotspot);
}
