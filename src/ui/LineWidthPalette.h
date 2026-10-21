#ifndef LINE_WIDTH_PALETTE_H
#define LINE_WIDTH_PALETTE_H

#include <View.h>


class BBitmap;


// MacPaint's line palette: "no border" (a gray sample, for the shape tools)
// and sizes 1, 2, 4 and 8, with a check mark at the current choice.
// Posts kMsgSetLineWidth to the window.
class LineWidthPalette : public BView {
public:
								LineWidthPalette();
	virtual						~LineWidthPalette();

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);

			void				SetLineWidth(int width, bool border);

private:
			void				_Render();

			BBitmap*			fBitmap;
			int					fWidth;
			bool				fBorder;
};


#endif	// LINE_WIDTH_PALETTE_H
