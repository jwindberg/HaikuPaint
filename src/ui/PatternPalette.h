#ifndef PATTERN_PALETTE_H
#define PATTERN_PALETTE_H

#include <View.h>

#include <vector>


class BBitmap;


// The current pattern in a large swatch, then a two-row grid of all
// hp::kPatterns. Posts kMsgSetPattern to the window.
class PatternPalette : public BView {
public:
								PatternPalette();
	virtual						~PatternPalette();

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);

			void				SetPatternIndex(int index);

private:
			BRect				_CurrentFrame() const;
			BRect				_CellFrame(int index) const;

			std::vector<BBitmap*> fSwatches;
			BBitmap*			fCurrentSwatch;
			int					fIndex;
};


#endif	// PATTERN_PALETTE_H
