#ifndef TOOL_PALETTE_H
#define TOOL_PALETTE_H

#include <View.h>

#include "Messages.h"


class BBitmap;


// MacPaint's tool palette: 20 icons from PaintFont in two columns, with the
// selected tool inverted. Posts kMsgSetTool to the window; double-clicking
// the eraser posts
// kMsgClear, the brush kMsgChooseBrush, the marquee kMsgSelectAll, the
// pencil kMsgToggleFatBits and the grabber kMsgShowPage, as in MacPaint.
class ToolPalette : public BView {
public:
								ToolPalette();
	virtual						~ToolPalette();

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);

			void				SetTool(Tool tool);

private:
			void				_Render();

			BBitmap*			fBitmap;
			Tool				fTool;
};


#endif	// TOOL_PALETTE_H
