#ifndef PATTERN_PALETTE_H
#define PATTERN_PALETTE_H

#include <View.h>


class BBitmap;


// MacPaint's pattern palette: a sample of the current pattern, then
// hp::kPatterns in two rows of 19. Posts kMsgSetPattern to the window.
class PatternPalette : public BView {
public:
								PatternPalette();
	virtual						~PatternPalette();

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);

			void				SetPatternIndex(int index);

private:
			void				_Render();

			BBitmap*			fBitmap;
			int					fIndex;
};


#endif	// PATTERN_PALETTE_H
