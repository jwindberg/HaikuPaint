#ifndef TOOL_PALETTE_H
#define TOOL_PALETTE_H

#include <View.h>

#include <vector>

#include "Messages.h"


class BBitmap;


// MacPaint-style grid of tool icons; the selected tool is shown inverted.
// Posts kMsgSetTool to the window. Double-clicking the eraser posts
// kMsgClear, as in MacPaint.
class ToolPalette : public BView {
public:
								ToolPalette();
	virtual						~ToolPalette();

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);

			void				SetTool(Tool tool);

private:
			BRect				_CellFrame(int index) const;

			std::vector<BBitmap*> fIcons;
			std::vector<BBitmap*> fSelectedIcons;
			Tool				fTool;
};


#endif	// TOOL_PALETTE_H
