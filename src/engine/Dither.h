#ifndef HP_DITHER_H
#define HP_DITHER_H

#include <cstdint>
#include <vector>

#include "Canvas.h"


namespace hp {


// Converts a grayscale image (0 black ... 255 white, width * height bytes,
// row by row) to black and white with Floyd-Steinberg error diffusion,
// scanning alternate rows in opposite directions. Pure black and white
// pass through unchanged, so line art stays crisp. The result goes in the
// top left of canvas, cropped to it; the rest of canvas is cleared.
void	DitherGray(const std::vector<uint8_t>& gray, int width, int height,
			Canvas& canvas);


}	// namespace hp

#endif	// HP_DITHER_H
