#include "MainWindow.h"

#include <Alert.h>
#include <Clipboard.h>
#include <Directory.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollView.h>

#include "BrushWindow.h"
#include "CanvasView.h"
#include "DocumentIO.h"
#include "LineWidthPalette.h"
#include "Messages.h"
#include "PatternPalette.h"
#include "Patterns.h"
#include "ToolPalette.h"


enum {
	kMsgNew				= 'new ',
	kMsgOpen			= 'open',
	kMsgSave			= 'save',
	kMsgSaveAs			= 'svas',
	kMsgExport			= 'expt',
	kMsgSaveRequested	= 'svrq',
	kMsgExportRequested	= 'exrq',
	kMsgUndo			= 'undo',
	kMsgSelectionCommand = 'scmd'	// "command" int32 (a SelectionCommand)
};

static const char* kUntitled = "untitled";


MainWindow::MainWindow()
	:
	BWindow(BRect(100, 100, 750, 650), kUntitled, B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE
			| B_AUTO_UPDATE_SIZE_LIMITS),
	fHasRef(false),
	fName(kUntitled),
	fPendingAction(kPendingNone)
{
	fCanvasView = new CanvasView();
	fCanvasView->SetPattern(hp::kPatterns[hp::kDefaultPatternIndex]);
	BScrollView* scrollView = new BScrollView("scroller", fCanvasView, 0,
		true, true, B_NO_BORDER);

	fToolPalette = new ToolPalette();
	fLineWidthPalette = new LineWidthPalette();
	fPatternPalette = new PatternPalette();

	// As in MacPaint: tools above the line palette on the left, patterns
	// below the document.
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(_BuildMenuBar())
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.SetInsets(B_USE_SMALL_SPACING)
			.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
				.Add(fToolPalette)
				.AddGlue()
				.Add(fLineWidthPalette)
			.End()
			.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
				.Add(scrollView)
				.AddGroup(B_HORIZONTAL, 0)
					.Add(fPatternPalette)
					.AddGlue()
				.End()
			.End()
		.End();

	// The panels copy the messenger and message.
	BMessenger messenger(this);
	fOpenPanel = new BFilePanel(B_OPEN_PANEL, &messenger, NULL, B_FILE_NODE,
		false);
	BMessage saveMessage(kMsgSaveRequested);
	fSavePanel = new BFilePanel(B_SAVE_PANEL, &messenger, NULL, B_FILE_NODE,
		false, &saveMessage);
	BMessage exportMessage(kMsgExportRequested);
	fExportPanel = new BFilePanel(B_SAVE_PANEL, &messenger, NULL,
		B_FILE_NODE, false, &exportMessage);
	fExportPanel->Window()->SetTitle("HaikuPaint: Export as PNG");
}


MainWindow::~MainWindow()
{
	delete fOpenPanel;
	delete fSavePanel;
	delete fExportPanel;
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgNew:
			if (_ConfirmDiscard(kPendingNew))
				_New();
			break;

		case kMsgOpen:
			fOpenPanel->Show();
			break;

		case B_REFS_RECEIVED:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK
				&& _ConfirmDiscard(kPendingOpen, &ref))
				_Open(ref);
			break;
		}

		case kMsgSave:
			_Save();
			break;

		case kMsgSaveAs:
			_ShowSavePanel();
			break;

		case kMsgExport:
		{
			BString name(fName);
			int32 dot = name.FindLast('.');
			if (dot > 0)
				name.Truncate(dot);
			name << ".png";
			fExportPanel->SetSaveText(name.String());
			fExportPanel->Show();
			break;
		}

		case kMsgSaveRequested:
			_SaveRequested(message, false);
			break;

		case kMsgExportRequested:
			_SaveRequested(message, true);
			break;

		case B_CANCEL:
			// A file panel closed. After a save this arrives once the
			// pending action has already run.
			if (message->GetInt32("old_what", 0) == kMsgSaveRequested)
				fPendingAction = kPendingNone;
			break;

		case kMsgUndo:
			fCanvasView->Undo();
			break;

		case B_CUT:
			fCanvasView->Cut();
			break;

		case B_COPY:
			fCanvasView->Copy();
			break;

		case B_PASTE:
			fCanvasView->Paste();
			break;

		case B_SELECT_ALL:
		case kMsgSelectAll:
			fCanvasView->SelectAll();
			break;

		case kMsgSelectionCommand:
		{
			SelectionCommand command
				= (SelectionCommand)message->GetInt32("command", 0);
			// As in MacPaint, Shift makes Trace Edges add a shadow.
			if (command == SelectionCommand::TraceEdges
				&& (modifiers() & B_SHIFT_KEY) != 0)
				command = SelectionCommand::TraceEdgesShadow;
			fCanvasView->ApplyToSelection(command);
			break;
		}

		case kMsgClear:
			fCanvasView->Clear();
			break;

		case kMsgSetTool:
		{
			int32 tool;
			if (message->FindInt32("tool", &tool) == B_OK) {
				fCanvasView->SetTool((Tool)tool);
				fToolPalette->SetTool((Tool)tool);
			}
			break;
		}

		case kMsgSetPattern:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK
				&& index >= 0 && index < hp::kPatternCount) {
				fCanvasView->SetPattern(hp::kPatterns[index]);
				fPatternPalette->SetPatternIndex(index);
			}
			break;
		}

		case kMsgSetLineWidth:
		{
			int32 width;
			bool border = message->GetBool("border", true);
			if (message->FindInt32("width", &width) == B_OK) {
				fCanvasView->SetLineWidth(width, border);
				fLineWidthPalette->SetLineWidth(width, border);
			}
			break;
		}

		case kMsgChooseBrush:
		{
			BrushWindow* window = new BrushWindow(Frame(),
				fCanvasView->Brush(), BMessenger(this));
			window->Show();
			break;
		}

		case kMsgSetBrush:
		{
			int32 brush;
			if (message->FindInt32("brush", &brush) == B_OK)
				fCanvasView->SetBrush(brush);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
MainWindow::MenusBeginning()
{
	fUndoItem->SetEnabled(fCanvasView->CanUndo());

	bool selection = fCanvasView->HasSelection();
	for (BMenuItem* item : fSelectionItems)
		item->SetEnabled(selection);

	bool canPaste = false;
	if (be_clipboard->Lock()) {
		BMessage* clip = be_clipboard->Data();
		canPaste = clip->HasData("application/x-vnd.HaikuPaint-selection",
				B_MIME_TYPE)
			|| clip->HasMessage("image/bitmap");
		be_clipboard->Unlock();
	}
	fPasteItem->SetEnabled(canPaste);

	BWindow::MenusBeginning();
}


bool
MainWindow::QuitRequested()
{
	return _ConfirmDiscard(kPendingQuit);
}


BMenuBar*
MainWindow::_BuildMenuBar()
{
	BMenuBar* menuBar = new BMenuBar("menubar");

	BMenu* fileMenu = new BMenu("File");
	fileMenu->AddItem(new BMenuItem("New", new BMessage(kMsgNew), 'N'));
	fileMenu->AddItem(new BMenuItem("Open" B_UTF8_ELLIPSIS,
		new BMessage(kMsgOpen), 'O'));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Save", new BMessage(kMsgSave), 'S'));
	fileMenu->AddItem(new BMenuItem("Save as" B_UTF8_ELLIPSIS,
		new BMessage(kMsgSaveAs), 'S', B_SHIFT_KEY));
	fileMenu->AddItem(new BMenuItem("Export as PNG" B_UTF8_ELLIPSIS,
		new BMessage(kMsgExport)));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED),
		'Q'));
	menuBar->AddItem(fileMenu);

	// MacPaint's Edit menu, plus Select all.
	BMenu* editMenu = new BMenu("Edit");
	fUndoItem = new BMenuItem("Undo", new BMessage(kMsgUndo), 'Z');
	editMenu->AddItem(fUndoItem);
	editMenu->AddSeparatorItem();

	BMenuItem* item = new BMenuItem("Cut", new BMessage(B_CUT), 'X');
	editMenu->AddItem(item);
	fSelectionItems.push_back(item);
	item = new BMenuItem("Copy", new BMessage(B_COPY), 'C');
	editMenu->AddItem(item);
	fSelectionItems.push_back(item);
	fPasteItem = new BMenuItem("Paste", new BMessage(B_PASTE), 'V');
	editMenu->AddItem(fPasteItem);
	editMenu->AddItem(new BMenuItem("Clear", new BMessage(kMsgClear)));
	editMenu->AddItem(new BMenuItem("Select all",
		new BMessage(B_SELECT_ALL), 'A'));
	editMenu->AddSeparatorItem();

	struct {
		const char*			label;
		SelectionCommand	command;
	} commands[] = {
		{"Invert", SelectionCommand::Invert},
		{"Fill", SelectionCommand::Fill},
		{"Trace edges", SelectionCommand::TraceEdges},
		{"Flip horizontal", SelectionCommand::FlipHorizontal},
		{"Flip vertical", SelectionCommand::FlipVertical},
		{"Rotate", SelectionCommand::Rotate},
	};
	for (const auto& entry : commands) {
		BMessage* message = new BMessage(kMsgSelectionCommand);
		message->AddInt32("command", (int32)entry.command);
		item = new BMenuItem(entry.label, message);
		editMenu->AddItem(item);
		fSelectionItems.push_back(item);
	}
	menuBar->AddItem(editMenu);

	return menuBar;
}


bool
MainWindow::_ConfirmDiscard(PendingAction action, const entry_ref* ref)
{
	// Returns true when the caller may go ahead now. If the user wants to
	// save a document that has no file yet, the save panel opens and the
	// action runs once the save is done.
	if (!fCanvasView->IsDirty())
		return true;

	BString text;
	text.SetToFormat("Save changes to the document \"%s\"?", fName.String());
	BAlert* alert = new BAlert("Unsaved changes", text.String(), "Cancel",
		"Don't save", "Save", B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
		B_WARNING_ALERT);
	alert->SetShortcut(0, B_ESCAPE);
	switch (alert->Go()) {
		case 0:
			return false;
		case 1:
			return true;
		default:
			break;
	}

	if (fHasRef)
		return _Save();

	fPendingAction = action;
	if (ref != NULL)
		fPendingRef = *ref;
	_ShowSavePanel();
	return false;
}


void
MainWindow::_RunPendingAction()
{
	PendingAction action = fPendingAction;
	fPendingAction = kPendingNone;
	switch (action) {
		case kPendingNew:
			_New();
			break;
		case kPendingOpen:
			_Open(fPendingRef);
			break;
		case kPendingQuit:
			PostMessage(B_QUIT_REQUESTED);
			break;
		case kPendingNone:
			break;
	}
}


void
MainWindow::_New()
{
	fCanvasView->SetImage(hp::Canvas());
	fHasRef = false;
	_SetDocumentName(kUntitled);
}


void
MainWindow::_Open(const entry_ref& ref)
{
	hp::Canvas image;
	bool isMacPaint = false;
	BString error;
	if (LoadDocument(ref, image, &isMacPaint, &error) != B_OK) {
		_ShowError(error);
		return;
	}

	fCanvasView->SetImage(image);
	fHasRef = isMacPaint;
	if (isMacPaint)
		fRef = ref;
	_SetDocumentName(ref.name);
}


bool
MainWindow::_Save()
{
	if (!fHasRef) {
		_ShowSavePanel();
		return false;
	}

	BString error;
	BEntry entry(&fRef);
	if (SaveMacPaint(entry, fCanvasView->Image(), &error) != B_OK) {
		_ShowError(error);
		return false;
	}
	fCanvasView->SetDirty(false);
	return true;
}


void
MainWindow::_ShowSavePanel()
{
	// Suggest a MacPaint name, also for an image opened from another format.
	BString name(fName);
	if (!fHasRef) {
		int32 dot = name.FindLast('.');
		if (dot > 0)
			name.Truncate(dot);
		name << ".mac";
	}
	fSavePanel->SetSaveText(name.String());
	fSavePanel->Show();
}


void
MainWindow::_SaveRequested(BMessage* message, bool exporting)
{
	entry_ref directoryRef;
	const char* name;
	if (message->FindRef("directory", &directoryRef) != B_OK
		|| message->FindString("name", &name) != B_OK)
		return;

	BDirectory directory(&directoryRef);
	BEntry entry(&directory, name);
	BString error;

	if (exporting) {
		if (ExportPNG(entry, fCanvasView->Image(), &error) != B_OK)
			_ShowError(error);
		return;
	}

	if (SaveMacPaint(entry, fCanvasView->Image(), &error) != B_OK) {
		_ShowError(error);
		fPendingAction = kPendingNone;
		return;
	}
	entry.GetRef(&fRef);
	fHasRef = true;
	fCanvasView->SetDirty(false);
	_SetDocumentName(name);
	_RunPendingAction();
}


void
MainWindow::_SetDocumentName(const char* name)
{
	fName = name;
	SetTitle(name);
}


void
MainWindow::_ShowError(const BString& text)
{
	BAlert* alert = new BAlert("Error", text.String(), "OK", NULL, NULL,
		B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert->Go(NULL);
}
