#include "MacPaintFile.h"

#include <cstring>

#include "Patterns.h"


namespace hp {


static const size_t kMacBinaryHeaderSize = 128;


static void
SetError(std::string* error, const char* message)
{
	if (error != nullptr)
		*error = message;
}


size_t
MacPaintDataOffset(const uint8_t* data, size_t size)
{
	// MacBinary: byte 0 is zero, byte 1 the file name length (1-63), and
	// bytes 65-68 the Mac file type.
	if (size >= kMacBinaryHeaderSize + kMacPaintHeaderSize && data[0] == 0
		&& data[1] >= 1 && data[1] <= 63
		&& std::memcmp(data + 65, "PNTG", 4) == 0)
		return kMacBinaryHeaderSize;
	return 0;
}


std::vector<uint8_t>
PackBits(const uint8_t* data, size_t size)
{
	std::vector<uint8_t> out;
	size_t i = 0;
	while (i < size) {
		// Length of the run of identical bytes starting here.
		size_t run = 1;
		while (i + run < size && run < 128 && data[i + run] == data[i])
			run++;

		if (run >= 2) {
			// Repeat: -(count - 1), then the byte.
			out.push_back((uint8_t)(int8_t)(1 - (int)run));
			out.push_back(data[i]);
			i += run;
			continue;
		}

		// Literal: count - 1, then the bytes, up to the next run of two.
		size_t start = i;
		while (i < size && i - start < 128) {
			if (i + 1 < size && data[i + 1] == data[i])
				break;
			i++;
		}
		out.push_back((uint8_t)(i - start - 1));
		out.insert(out.end(), data + start, data + i);
	}
	return out;
}


bool
UnpackBits(const uint8_t* data, size_t size, size_t* position, uint8_t* out,
	size_t outSize)
{
	size_t written = 0;
	while (written < outSize) {
		if (*position >= size)
			return false;
		int8_t flag = (int8_t)data[(*position)++];
		if (flag == -128)
			continue;

		if (flag >= 0) {
			size_t count = (size_t)flag + 1;
			if (*position + count > size)
				return false;
			for (size_t i = 0; i < count && written < outSize; i++)
				out[written++] = data[*position + i];
			*position += count;
		} else {
			size_t count = (size_t)(1 - flag);
			if (*position >= size)
				return false;
			uint8_t value = data[(*position)++];
			for (size_t i = 0; i < count && written < outSize; i++)
				out[written++] = value;
		}
	}
	return true;
}


bool
ReadMacPaint(const uint8_t* data, size_t size, Canvas& canvas,
	std::string* error)
{
	if (canvas.Width() != kMacPaintWidth
		|| canvas.Height() != kMacPaintHeight) {
		SetError(error, "The canvas isn't MacPaint's size.");
		return false;
	}

	size_t offset = MacPaintDataOffset(data, size);
	if (size < offset + kMacPaintHeaderSize) {
		SetError(error, "The file is too short to be a MacPaint document.");
		return false;
	}

	// The version is 0 to 3 in real files; anything else means this is
	// some other kind of file.
	uint32_t version = (uint32_t)data[offset] << 24
		| (uint32_t)data[offset + 1] << 16 | (uint32_t)data[offset + 2] << 8
		| data[offset + 3];
	if (version > 3) {
		SetError(error, "This doesn't look like a MacPaint document.");
		return false;
	}

	// The scanlines run continuously; MacPaint packs each one separately,
	// but some writers didn't, and unpacking straight through handles both.
	std::vector<uint8_t> bits(kMacPaintRowBytes * kMacPaintHeight);
	size_t position = offset + kMacPaintHeaderSize;
	if (!UnpackBits(data, size, &position, bits.data(), bits.size())) {
		SetError(error, "The MacPaint document is damaged or incomplete.");
		return false;
	}

	for (int y = 0; y < kMacPaintHeight; y++) {
		for (int x = 0; x < kMacPaintWidth; x++) {
			uint8_t byte = bits[y * kMacPaintRowBytes + x / 8];
			canvas.Set(x, y, (byte >> (7 - x % 8)) & 1);
		}
	}
	return true;
}


std::vector<uint8_t>
WriteMacPaint(const Canvas& canvas, const Pattern* patterns)
{
	std::vector<uint8_t> out(kMacPaintHeaderSize, 0);
	out[3] = 2;		// version 2: has patterns
	for (int i = 0; i < kPatternCount; i++)
		std::memcpy(&out[4 + i * 8], patterns[i].rows, 8);

	uint8_t row[kMacPaintRowBytes];
	for (int y = 0; y < kMacPaintHeight; y++) {
		std::memset(row, 0, sizeof(row));
		for (int x = 0; x < kMacPaintWidth && x < canvas.Width(); x++) {
			if (canvas.Get(x, y))
				row[x / 8] |= 0x80 >> (x % 8);
		}
		std::vector<uint8_t> packed = PackBits(row, sizeof(row));
		out.insert(out.end(), packed.begin(), packed.end());
	}
	return out;
}


}	// namespace hp
