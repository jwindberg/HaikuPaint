#ifndef CANVAS_VIEW_H
#define CANVAS_VIEW_H

#include <View.h>

#include "Canvas.h"


class BBitmap;


enum class Tool {
	Pencil,
	Line,
	PaintBucket
};


// Shows an hp::Canvas and turns mouse input into tool operations.
// All pixel logic belongs in the engine; this class only translates events
// and keeps fBitmap (what the app_server draws) in sync with the canvas.
class CanvasView : public BView {
public:
								CanvasView();
	virtual						~CanvasView();

	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float newWidth, float newHeight);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				MouseUp(BPoint where);

			void				SetTool(Tool tool) { fTool = tool; }
			void				Clear();

private:
			void				_CanvasChanged(BRect dirty);
			void				_SyncBitmap(BRect dirty);
			void				_UpdateScrollBars();
			BRect				_CanvasBounds() const;

			hp::Canvas			fCanvas;
			BBitmap*			fBitmap;
			Tool				fTool;
			bool				fTracking;
			bool				fPencilBlack;
			BPoint				fStart;
			BPoint				fLast;
};


#endif	// CANVAS_VIEW_H
