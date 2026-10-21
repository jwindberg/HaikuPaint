#ifndef HP_PATTERNS_H
#define HP_PATTERNS_H

#include "Canvas.h"


namespace hp {


// The pattern palette: 38 patterns, like MacPaint's. This is HaikuPaint's own
// set (grays, lines, textures), not Apple's originals; see docs/ROADMAP.md.
static constexpr int kPatternCount = 38;
extern const Pattern kPatterns[kPatternCount];

// Index of solid black, the initial selection.
static constexpr int kDefaultPatternIndex = 1;


}	// namespace hp

#endif	// HP_PATTERNS_H
