#ifndef PATTERN_PALETTE_H
#define PATTERN_PALETTE_H

#include <View.h>

#include "Patterns.h"


class BBitmap;


// MacPaint's pattern palette: a sample of the current pattern, then the
// document's 38 patterns in two rows of 19. Posts kMsgSetPattern to the
// window; double-clicking a pattern also posts kMsgEditPattern.
class PatternPalette : public BView {
public:
								PatternPalette();
	virtual						~PatternPalette();

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);

			void				SetPatternIndex(int index);
			void				SetPatterns(const hp::Pattern* patterns);

private:
			void				_Render();

			BBitmap*			fBitmap;
			int					fIndex;
			hp::Pattern			fPatterns[hp::kPatternCount];
};


#endif	// PATTERN_PALETTE_H
