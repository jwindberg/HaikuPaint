#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Entry.h>
#include <String.h>
#include <Window.h>


class BFilePanel;
class BMenuBar;
class BMenuItem;
class CanvasView;
class LineWidthPalette;
class PatternPalette;
class ToolPalette;


class MainWindow : public BWindow {
public:
								MainWindow();
	virtual						~MainWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MenusBeginning();
	virtual	bool				QuitRequested();

private:
	// What to do once an "unsaved changes" save finishes.
	enum PendingAction {
		kPendingNone,
		kPendingNew,
		kPendingOpen,
		kPendingQuit
	};

			BMenuBar*			_BuildMenuBar();

			bool				_ConfirmDiscard(PendingAction action,
									const entry_ref* ref = NULL);
			void				_RunPendingAction();

			void				_New();
			void				_Open(const entry_ref& ref);
			bool				_Save();
			void				_ShowSavePanel();
			void				_SaveRequested(BMessage* message,
									bool exporting);
			void				_SetDocumentName(const char* name);
			void				_ShowError(const BString& text);

			CanvasView*			fCanvasView;
			ToolPalette*		fToolPalette;
			LineWidthPalette*	fLineWidthPalette;
			PatternPalette*		fPatternPalette;
			BMenuItem*			fUndoItem;

			BFilePanel*			fOpenPanel;
			BFilePanel*			fSavePanel;
			BFilePanel*			fExportPanel;

			// The file Save writes to; only set for MacPaint documents, so
			// an opened PNG is never overwritten in MacPaint format.
			entry_ref			fRef;
			bool				fHasRef;
			BString				fName;

			PendingAction		fPendingAction;
			entry_ref			fPendingRef;
};


#endif	// MAIN_WINDOW_H
