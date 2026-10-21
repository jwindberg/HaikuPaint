#ifndef CANVAS_VIEW_H
#define CANVAS_VIEW_H

#include <View.h>

#include "Canvas.h"
#include "Messages.h"


class BBitmap;


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

			void				SetTool(Tool tool);
			void				SetPattern(const hp::Pattern& pattern)
									{ fPattern = pattern; }
			void				SetLineWidth(int width)
									{ fLineWidth = width; }
			void				Clear();

private:
			void				_CanvasChanged(BRect dirty);
			void				_SyncBitmap(BRect dirty);
			void				_UpdateScrollBars();
			void				_Erase(BPoint from, BPoint to);
			BRect				_EraserFrame(BPoint where) const;
			BRect				_LineFrame(BPoint from, BPoint to) const;
			BRect				_CanvasBounds() const;

			hp::Canvas			fCanvas;
			BBitmap*			fBitmap;
			Tool				fTool;
			hp::Pattern			fPattern;
			int					fLineWidth;
			bool				fTracking;
			bool				fPencilBlack;
			BPoint				fStart;
			BPoint				fLast;
			bool				fShowEraser;
			BPoint				fEraserWhere;
};


#endif	// CANVAS_VIEW_H
