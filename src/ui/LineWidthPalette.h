#ifndef LINE_WIDTH_PALETTE_H
#define LINE_WIDTH_PALETTE_H

#include <View.h>


// A column of line samples; the selected width is shown inverted.
// Posts kMsgSetLineWidth to the window.
class LineWidthPalette : public BView {
public:
								LineWidthPalette();

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);

			void				SetLineWidth(int width);

private:
			BRect				_RowFrame(int index) const;

			int					fWidth;
};


#endif	// LINE_WIDTH_PALETTE_H
