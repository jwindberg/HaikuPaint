#ifndef HP_MACPAINT_FILE_H
#define HP_MACPAINT_FILE_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Canvas.h"


namespace hp {


// MacPaint documents (Mac file type 'PNTG'), as read and written by
// MacPaint.p (ReadDoc, BlankDoc):
//   - a 512-byte header: version (4 bytes, big endian; 2 and up carry
//     patterns), the document's 38 patterns (304 bytes), then zeros;
//   - 720 scanlines of 72 bytes (576 pixels, 1 = black), each compressed
//     separately with PackBits.
// Files copied off a Mac often carry a 128-byte MacBinary header first;
// reading skips it.

static constexpr int kMacPaintWidth = 576;
static constexpr int kMacPaintHeight = 720;
static constexpr int kMacPaintRowBytes = kMacPaintWidth / 8;
static constexpr int kMacPaintHeaderSize = 512;


// Reads a MacPaint document into canvas, which must be 576 x 720. Returns
// false, with a reason in error, if the data isn't a usable MacPaint file.
// If patterns is given, the document's 38 patterns are copied there and
// *hasPatterns says whether it had any (version 2 and up do).
bool					ReadMacPaint(const uint8_t* data, size_t size,
							Canvas& canvas, std::string* error = nullptr,
							Pattern* patterns = nullptr,
							bool* hasPatterns = nullptr);

// Encodes canvas (576 x 720) as a version 2 MacPaint document with the
// given 38 patterns.
std::vector<uint8_t>	WriteMacPaint(const Canvas& canvas,
							const Pattern* patterns);

// Where the MacPaint data starts: 128 for a MacBinary file of type PNTG,
// otherwise 0.
size_t					MacPaintDataOffset(const uint8_t* data, size_t size);

// Apple's PackBits, exposed for testing.
std::vector<uint8_t>	PackBits(const uint8_t* data, size_t size);

// Unpacks into exactly outSize bytes from data, starting at *position and
// advancing it. Returns false if the data ends early.
bool					UnpackBits(const uint8_t* data, size_t size,
							size_t* position, uint8_t* out, size_t outSize);


}	// namespace hp

#endif	// HP_MACPAINT_FILE_H
