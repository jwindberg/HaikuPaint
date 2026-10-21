#ifndef CANVAS_VIEW_H
#define CANVAS_VIEW_H

#include <View.h>

#include <optional>
#include <vector>

#include "Canvas.h"
#include "Geometry.h"
#include "Messages.h"
#include "Selection.h"


class BBitmap;
class BCursor;
class BMessageRunner;


// Shows an hp::Canvas and turns mouse input into tool operations, following
// MacPaint's EditDoc and the tool procedures it calls. All pixel logic
// belongs in the engine; this class only translates events and keeps
// fBitmap (what the app_server draws) in sync with the canvas.
//
// MacPaint's modifier keys map to Haiku as Option -> Ctrl and
// Command -> Alt.
class CanvasView : public BView {
public:
								CanvasView();
	virtual						~CanvasView();

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float newWidth, float newHeight);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				MouseUp(BPoint where);

			void				SetTool(Tool tool);
			void				SetPattern(const hp::Pattern& pattern)
									{ fPattern = pattern; }
			void				SetLineWidth(int width, bool border);
			void				SetBrush(int brush);
			int					Brush() const { return fBrush; }

			// Clears the selection if there is one, else the whole page.
			void				Clear();

			// MacPaint's one level of undo, which undoing again redoes.
			bool				CanUndo() const { return fCanUndo; }
			void				Undo();

			// Selections and the clipboard.
			bool				HasSelection() const
									{ return fSelection.has_value(); }
			void				Cut();
			void				Copy();
			void				Paste();
			void				SelectAll();
			void				ApplyToSelection(SelectionCommand command);

			// The document.
			const hp::Canvas&	Image() const { return fCanvas; }
			void				SetImage(const hp::Canvas& image);
			bool				IsDirty() const { return fDirty; }
			void				SetDirty(bool dirty) { fDirty = dirty; }

private:
	enum DragMode {
		kDragNone,
		kDragMarquee,		// making a rectangular selection
		kDragLasso,			// drawing a lasso loop
		kDragMove,			// moving the selection
		kDragCopy,			// moving a copy, leaving the original (Option)
		kDragTrail,			// leaving copies along the way (Option+Command)
		kDragStretch		// stretching a rect selection (Command)
	};

			// Called before anything changes the canvas: saves the undo
			// image and marks the document dirty.
			void				_BeginOperation();

			// Live previews: the canvas is restored from fUndo (the image
			// before this operation) under the previous preview before each
			// new one is drawn, as MacPaint does with its alternate buffer.
			void				_BeginPreview();
			void				_RestorePreview();
			void				_EndPreview(hp::Rect drawn);

			void				_DragTo(hp::Point point);
			void				_DrawShape(hp::Point end);
			void				_DrawLine(hp::Point end);
			void				_PolygonClick(hp::Point point);
			void				_DrawPolygonPreview(hp::Point end);
			void				_FinishPolygon(bool closed);
			void				_FinishFilledFreeform();

			// Selections. While one floats, fCanvas shows it stamped over
			// fBase, the page underneath.
			void				_SelectionMouseDown(hp::Point point);
			void				_BeginSelectionDrag(hp::Point point);
			void				_DragSelection(hp::Point point);
			void				_EndSelectionDrag(hp::Point point);
			void				_Lift();
			void				_PlaceSelection(hp::Selection selection);
			void				_SelectionChanged();
			void				_AcceptSelection();
			void				_ClearSelection();
			bool				_InSelection(hp::Point point) const;

			// Overlays: the marching ants and the lasso trail. They're drawn
			// into fBitmap only, never into the canvas.
			void				_SetOverlay(std::vector<hp::Point> points,
									bool ants);
			void				_AppendOverlay(
									const std::vector<hp::Point>& points);

			hp::Point			_PenPoint(hp::Point point) const;
			hp::Rect			_PenBounds(hp::Point a, hp::Point b) const;
			hp::Rect			_PointsBounds(
									const std::vector<hp::Point>& points) const;
			std::vector<hp::Point> _PenPoints() const;
			const hp::Pattern&	_LinePattern() const;
			bool				_ShiftDown() const;
			bool				_OptionDown() const;
			bool				_CommandDown() const;

			void				_Erase(hp::Point from, hp::Point to);
			void				_CanvasChanged(hp::Rect dirty);
			void				_SyncBitmap(BRect dirty);
			void				_UpdateScrollBars();
			void				_UpdateCursor();
			hp::Rect			_CanvasBounds() const;
			hp::Rect			_VisibleBounds() const;

			hp::Canvas			fCanvas;
			hp::Canvas			fUndo;
			bool				fCanUndo;
			bool				fDirty;
			BBitmap*			fBitmap;

			Tool				fTool;
			hp::Pattern			fPattern;
			int					fLineWidth;
			bool				fBorder;
			int					fBrush;
			hp::Mask16			fBrushMask;
			hp::Mask16			fSprayMask;

			bool				fTracking;
			bool				fPencilBlack;
			hp::Point			fStart;
			hp::Point			fLast;
			hp::AxisLock		fAxisLock;
			hp::Rect			fPreviewRect;

			// Freeform, polygon and lasso vertices, in mouse coordinates.
			std::vector<hp::Point> fPoints;
			bool				fPolygonActive;

			std::optional<hp::Selection> fSelection;
			std::vector<hp::Point> fSelectionEdges;	// relative to its rect
			hp::Canvas			fBase;
			bool				fFloating;
			DragMode			fDrag;
			hp::Rect			fDragStartRect;
			hp::Selection		fStretchSource;
			hp::Point			fAnchor;
			bool				fKeepWidth;
			bool				fKeepHeight;
			bool				fInSelection;

			std::vector<hp::Point> fOverlay;
			bool				fOverlayAnts;
			hp::Rect			fOverlayBounds;
			int					fAntsPhase;
			BMessageRunner*		fAntsRunner;

			// MacPaint's cursors, from PaintFont.
			BCursor*			fPencilCursor;
			BCursor*			fFillCursor;
			BCursor*			fEraserCursor;
			BCursor*			fSprayCursor;
			BCursor*			fBrushCursor;
			BCursor*			fLassoCursor;
			BCursor*			fSelectCursor;
			BCursor*			fArrowCursor;
			BCursor*			fLineCursors[4];	// sizes 1, 2, 4, 8
};


#endif	// CANVAS_VIEW_H
