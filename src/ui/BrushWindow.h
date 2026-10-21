#ifndef BRUSH_WINDOW_H
#define BRUSH_WINDOW_H

#include <Messenger.h>
#include <Window.h>


// MacPaint's Brush Shape dialog: the 32 brushes from PaintFont, with the
// current one framed. A click sends kMsgSetBrush to the target and closes
// the window; Escape just closes it.
class BrushWindow : public BWindow {
public:
								BrushWindow(BRect parentFrame, int brush,
									BMessenger target);
};


#endif	// BRUSH_WINDOW_H
