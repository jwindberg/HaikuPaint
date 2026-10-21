#ifndef DIALOGS_H
#define DIALOGS_H

#include <Messenger.h>
#include <Window.h>

#include "Canvas.h"


// MacPaint's Goodies dialogs, laid out from its DLOG/DITL resources. Both
// are modal and send their result to the target when you click OK.


// Edit Pattern (DLOG 2003): an 8 x 8 grid of fat bits and a sample. Click
// or drag in the grid to set and clear bits. OK sends kMsgPatternEdited.
class PatternEditWindow : public BWindow {
public:
								PatternEditWindow(BRect parentFrame,
									int index, const hp::Pattern& pattern,
									BMessenger target);

	virtual	void				MessageReceived(BMessage* message);

private:
			class EditorView;

			EditorView*			fEditor;
			int					fIndex;
			BMessenger			fTarget;
};


// Brush Mirrors (DLOG 2005): four mirror lines through a square; click one
// to toggle it. OK sends kMsgSetMirrors; None turns them all off.
class MirrorWindow : public BWindow {
public:
								MirrorWindow(BRect parentFrame, int mirrors,
									BMessenger target);

	virtual	void				MessageReceived(BMessage* message);

private:
			class MirrorView;

			MirrorView*			fMirrorView;
			BMessenger			fTarget;
};


// Show Page: the whole page at a third of its size, with a gray frame for
// the part in the window. Drag the frame to choose what to look at; drag
// anywhere else to move the whole drawing on the page. OK sends
// kMsgShowPageDone.
class ShowPageWindow : public BWindow {
public:
								ShowPageWindow(BRect parentFrame,
									const hp::Canvas& page,
									hp::Rect visible, BMessenger target);

	virtual	void				MessageReceived(BMessage* message);

private:
			class PageView;

			PageView*			fPageView;
			hp::Rect			fVisible;
			int					fPageWidth;
			int					fPageHeight;
			BMessenger			fTarget;
};


#endif	// DIALOGS_H
