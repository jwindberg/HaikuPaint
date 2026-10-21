#include "MainWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <Directory.h>
#include <FilePanel.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollView.h>

#include "BrushWindow.h"
#include "CanvasView.h"
#include "Dialogs.h"
#include "DocumentIO.h"
#include "LineWidthPalette.h"
#include "Messages.h"
#include "PatternPalette.h"
#include "Patterns.h"
#include "PrintCatalog.h"
#include "Printing.h"
#include "Symmetry.h"
#include "ToolPalette.h"

#include <cstring>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


enum {
	kMsgNew				= 'new ',
	kMsgOpen			= 'open',
	kMsgSave			= 'save',
	kMsgSaveAs			= 'svas',
	kMsgRevert			= 'rvrt',
	kMsgPageSetup		= 'pgst',
	kMsgPrint			= 'prnt',
	kMsgPrintCatalog	= 'pcat',
	kMsgCatalogFolder	= 'pcfd',
	kMsgExport			= 'expt',
	kMsgSaveRequested	= 'svrq',
	kMsgExportRequested	= 'exrq',
	kMsgUndo			= 'undo',
	kMsgSelectionCommand = 'scmd',	// "command" int32 (a SelectionCommand)
	kMsgSetFontFamily	= 'sfam',	// "family" string
	kMsgSetFontSize		= 'ssiz',	// "size" float
	kMsgSetTextFace		= 'sfac',	// "face" int32: 0 plain, 1-5 as the menu
	kMsgSetAlignment	= 'salg'	// "alignment" int32 (a TextAlignment)
};

// MacPaint's FontSize menu.
static const float kFontSizes[] = {9, 10, 12, 14, 18, 24, 36, 48, 72};



MainWindow::MainWindow()
	:
	BWindow(BRect(100, 100, 750, 650), B_TRANSLATE("untitled"),
		B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE
			| B_AUTO_UPDATE_SIZE_LIMITS),
	fPatternIndex(hp::kDefaultPatternIndex),
	fMirrors(0),
	fHasRef(false),
	fName(B_TRANSLATE("untitled")),
	fPendingAction(kPendingNone),
	fPrintSettings(NULL)
{
	std::memcpy(fPatterns, hp::kPatterns, sizeof(fPatterns));
	fCanvasView = new CanvasView();
	fCanvasView->SetPattern(fPatterns[fPatternIndex]);
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
	fExportPanel->Window()->SetTitle(B_TRANSLATE("HaikuPaint: Export as PNG"));
	BMessage catalogMessage(kMsgCatalogFolder);
	fCatalogPanel = new BFilePanel(B_OPEN_PANEL, &messenger, NULL,
		B_DIRECTORY_NODE, false, &catalogMessage);
	fCatalogPanel->Window()->SetTitle(B_TRANSLATE("HaikuPaint: Print catalog"));
	fCatalogPanel->SetButtonLabel(B_DEFAULT_BUTTON, B_TRANSLATE("Print"));
}


MainWindow::~MainWindow()
{
	delete fOpenPanel;
	delete fSavePanel;
	delete fExportPanel;
	delete fCatalogPanel;
	delete fPrintSettings;
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

		case kMsgPageSetup:
			ConfigurePage(&fPrintSettings);
			break;

		case kMsgPrint:
			PrintPage(fCanvasView->Image(), fName.String(), &fPrintSettings);
			break;

		case kMsgPrintCatalog:
			// Start in the document's folder, as MacPaint catalogs its disk.
			if (fHasRef) {
				BEntry entry(&fRef);
				BEntry parent;
				entry_ref folder;
				if (entry.GetParent(&parent) == B_OK
					&& parent.GetRef(&folder) == B_OK)
					fCatalogPanel->SetPanelDirectory(&folder);
			}
			fCatalogPanel->Show();
			break;

		case kMsgCatalogFolder:
		{
			entry_ref folder;
			if (message->FindRef("refs", &folder) != B_OK)
				break;
			BString heading;
			heading.SetToFormat(B_TRANSLATE("MacPaint documents in %s"),
				folder.name);
			std::vector<hp::Canvas> pages = BuildCatalogPages(folder,
				heading);
			if (pages.empty()) {
				_ShowError(B_TRANSLATE(
					"There are no MacPaint documents in this folder."));
				break;
			}
			PrintPages(pages, heading.String(), &fPrintSettings);
			break;
		}

		case kMsgRevert:
		{
			if (!fHasRef)
				break;
			// MacPaint's Revert alert.
			BAlert* alert = new BAlert(B_TRANSLATE("Revert"),
				B_TRANSLATE("Revert to the last version saved?"),
				B_TRANSLATE("Cancel"), B_TRANSLATE("OK"), NULL,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->SetShortcut(0, B_ESCAPE);
			if (alert->Go() == 1)
				_Open(fRef);
			break;
		}

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
				fPatternIndex = index;
				fCanvasView->SetPattern(fPatterns[index]);
				fPatternPalette->SetPatternIndex(index);
			}
			break;
		}

		case kMsgEditPattern:
		{
			PatternEditWindow* window = new PatternEditWindow(Frame(),
				fPatternIndex, fPatterns[fPatternIndex], BMessenger(this));
			window->Show();
			break;
		}

		case kMsgPatternEdited:
		{
			int32 index;
			const void* data;
			ssize_t size;
			if (message->FindInt32("index", &index) != B_OK
				|| index < 0 || index >= hp::kPatternCount
				|| message->FindData("pattern", B_RAW_TYPE, &data, &size)
					!= B_OK || size != 8)
				break;
			hp::Pattern patterns[hp::kPatternCount];
			std::memcpy(patterns, fPatterns, sizeof(patterns));
			std::memcpy(patterns[index].rows, data, 8);
			_SetPatterns(patterns);
			// The patterns are saved with the document.
			fCanvasView->SetDirty(true);
			break;
		}

		case kMsgChooseMirrors:
		{
			MirrorWindow* window = new MirrorWindow(Frame(), fMirrors,
				BMessenger(this));
			window->Show();
			break;
		}

		case kMsgSetMirrors:
			fMirrors = message->GetInt32("mirrors", 0);
			fCanvasView->SetSymmetry(hp::MapSymmetry(
				(fMirrors & kMirrorVertical) != 0,
				(fMirrors & kMirrorHorizontal) != 0,
				(fMirrors & kMirrorDiagonal) != 0,
				(fMirrors & kMirrorAntiDiagonal) != 0));
			fMirrorsItem->SetMarked(fMirrors != 0);
			break;

		case kMsgSetFontFamily:
		{
			const char* family;
			if (message->FindString("family", &family) == B_OK) {
				fTextStyle.family = family;
				_TextStyleChanged();
			}
			break;
		}

		case kMsgSetFontSize:
			fTextStyle.size = message->GetFloat("size", 12);
			_TextStyleChanged();
			break;

		case kMsgSetTextFace:
			switch (message->GetInt32("face", 0)) {
				case 0:
					fTextStyle.bold = fTextStyle.italic = false;
					fTextStyle.underline = fTextStyle.outline = false;
					fTextStyle.shadow = false;
					break;
				case 1:
					fTextStyle.bold = !fTextStyle.bold;
					break;
				case 2:
					fTextStyle.italic = !fTextStyle.italic;
					break;
				case 3:
					fTextStyle.underline = !fTextStyle.underline;
					break;
				case 4:
					fTextStyle.outline = !fTextStyle.outline;
					break;
				case 5:
					fTextStyle.shadow = !fTextStyle.shadow;
					break;
			}
			_TextStyleChanged();
			break;

		case kMsgSetAlignment:
			fTextStyle.alignment
				= (TextAlignment)message->GetInt32("alignment", 0);
			_TextStyleChanged();
			break;

		case kMsgToggleGrid:
			fGridItem->SetMarked(!fGridItem->IsMarked());
			fCanvasView->SetGrid(fGridItem->IsMarked());
			break;

		case kMsgToggleFatBits:
			fCanvasView->SetFatBits(!fCanvasView->IsFatBits());
			fFatBitsItem->SetMarked(fCanvasView->IsFatBits());
			break;

		case kMsgShowPage:
		{
			// As in MacPaint, Show Page leaves FatBits.
			fCanvasView->SetFatBits(false);
			fFatBitsItem->SetMarked(false);
			ShowPageWindow* window = new ShowPageWindow(Frame(),
				fCanvasView->Image(), fCanvasView->VisibleRect(),
				BMessenger(this));
			window->Show();
			break;
		}

		case kMsgShowPageDone:
		{
			BPoint shift = message->GetPoint("shift", BPoint(0, 0));
			BPoint origin = message->GetPoint("origin", BPoint(0, 0));
			fCanvasView->OffsetPage((int)shift.x, (int)shift.y);
			fCanvasView->ScrollToPage({(int)origin.x, (int)origin.y});
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
	fRevertItem->SetEnabled(fHasRef && fCanvasView->IsDirty());

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

	BMenu* fileMenu = new BMenu(B_TRANSLATE("File"));
	BMenuItem* aboutItem = new BMenuItem(
		B_TRANSLATE("About HaikuPaint" B_UTF8_ELLIPSIS),
		new BMessage(B_ABOUT_REQUESTED));
	aboutItem->SetTarget(be_app);
	fileMenu->AddItem(aboutItem);
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("New"), new BMessage(kMsgNew), 'N'));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Open" B_UTF8_ELLIPSIS),
		new BMessage(kMsgOpen), 'O'));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Save"), new BMessage(kMsgSave), 'S'));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Save as" B_UTF8_ELLIPSIS),
		new BMessage(kMsgSaveAs), 'S', B_SHIFT_KEY));
	fRevertItem = new BMenuItem(B_TRANSLATE("Revert"), new BMessage(kMsgRevert));
	fileMenu->AddItem(fRevertItem);
	fileMenu->AddItem(new BMenuItem(
		B_TRANSLATE("Export as PNG" B_UTF8_ELLIPSIS),
		new BMessage(kMsgExport)));
	fileMenu->AddSeparatorItem();
	// No shortcut: Alt+P is MacPaint's Plain, and MacPaint had none.
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Page setup" B_UTF8_ELLIPSIS),
		new BMessage(kMsgPageSetup)));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Print" B_UTF8_ELLIPSIS),
		new BMessage(kMsgPrint)));
	fileMenu->AddItem(new BMenuItem(
		B_TRANSLATE("Print catalog" B_UTF8_ELLIPSIS),
		new BMessage(kMsgPrintCatalog)));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED),
		'Q'));
	menuBar->AddItem(fileMenu);

	// MacPaint's Edit menu, plus Select all.
	BMenu* editMenu = new BMenu(B_TRANSLATE("Edit"));
	fUndoItem = new BMenuItem(B_TRANSLATE("Undo"), new BMessage(kMsgUndo), 'Z');
	editMenu->AddItem(fUndoItem);
	editMenu->AddSeparatorItem();

	BMenuItem* item = new BMenuItem(B_TRANSLATE("Cut"), new BMessage(B_CUT), 'X');
	editMenu->AddItem(item);
	fSelectionItems.push_back(item);
	item = new BMenuItem(B_TRANSLATE("Copy"), new BMessage(B_COPY), 'C');
	editMenu->AddItem(item);
	fSelectionItems.push_back(item);
	fPasteItem = new BMenuItem(B_TRANSLATE("Paste"), new BMessage(B_PASTE), 'V');
	editMenu->AddItem(fPasteItem);
	editMenu->AddItem(new BMenuItem(B_TRANSLATE("Clear"), new BMessage(kMsgClear)));
	editMenu->AddItem(new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(B_SELECT_ALL), 'A'));
	editMenu->AddSeparatorItem();

	struct {
		const char*			label;
		SelectionCommand	command;
		char				shortcut;
	} commands[] = {
		{B_TRANSLATE_MARK("Invert"), SelectionCommand::Invert},
		{B_TRANSLATE_MARK("Fill"), SelectionCommand::Fill},
		{B_TRANSLATE_MARK("Trace edges"), SelectionCommand::TraceEdges, 'E'},
		{B_TRANSLATE_MARK("Flip horizontal"), SelectionCommand::FlipHorizontal},
		{B_TRANSLATE_MARK("Flip vertical"), SelectionCommand::FlipVertical},
		{B_TRANSLATE_MARK("Rotate"), SelectionCommand::Rotate},
	};
	for (const auto& entry : commands) {
		BMessage* message = new BMessage(kMsgSelectionCommand);
		message->AddInt32("command", (int32)entry.command);
		item = new BMenuItem(B_TRANSLATE_NOCOLLECT(entry.label), message,
			entry.shortcut);
		editMenu->AddItem(item);
		fSelectionItems.push_back(item);
	}
	menuBar->AddItem(editMenu);

	// MacPaint's Goodies, without its Introduction and Short Cuts screens.
	BMenu* goodiesMenu = new BMenu(B_TRANSLATE("Goodies"));
	fGridItem = new BMenuItem(B_TRANSLATE("Grid"), new BMessage(kMsgToggleGrid));
	goodiesMenu->AddItem(fGridItem);
	fFatBitsItem = new BMenuItem(B_TRANSLATE("FatBits"),
		new BMessage(kMsgToggleFatBits));
	goodiesMenu->AddItem(fFatBitsItem);
	goodiesMenu->AddItem(new BMenuItem(B_TRANSLATE("Show page"),
		new BMessage(kMsgShowPage)));
	goodiesMenu->AddSeparatorItem();
	goodiesMenu->AddItem(new BMenuItem(
		B_TRANSLATE("Edit pattern" B_UTF8_ELLIPSIS),
		new BMessage(kMsgEditPattern)));
	goodiesMenu->AddItem(new BMenuItem(
		B_TRANSLATE("Brush shape" B_UTF8_ELLIPSIS),
		new BMessage(kMsgChooseBrush)));
	fMirrorsItem = new BMenuItem(
		B_TRANSLATE("Brush mirrors" B_UTF8_ELLIPSIS),
		new BMessage(kMsgChooseMirrors));
	goodiesMenu->AddItem(fMirrorsItem);
	menuBar->AddItem(goodiesMenu);

	_AddTextMenus(menuBar);

	return menuBar;
}


void
MainWindow::_AddTextMenus(BMenuBar* menuBar)
{
	// Font: the installed families.
	fFontMenu = new BMenu(B_TRANSLATE("Font"));
	fFontMenu->SetRadioMode(true);
	int32 count = count_font_families();
	for (int32 i = 0; i < count; i++) {
		font_family family;
		if (get_font_family(i, &family) != B_OK)
			continue;
		BMessage* message = new BMessage(kMsgSetFontFamily);
		message->AddString("family", family);
		BMenuItem* item = new BMenuItem(family, message);
		item->SetMarked(fTextStyle.family == family);
		fFontMenu->AddItem(item);
	}
	menuBar->AddItem(fFontMenu);

	fSizeMenu = new BMenu(B_TRANSLATE("FontSize"));
	fSizeMenu->SetRadioMode(true);
	for (float size : kFontSizes) {
		BMessage* message = new BMessage(kMsgSetFontSize);
		message->AddFloat("size", size);
		BString label;
		label.SetToFormat(B_TRANSLATE("%d point"), (int)size);
		BMenuItem* item = new BMenuItem(label.String(), message);
		item->SetMarked(size == fTextStyle.size);
		fSizeMenu->AddItem(item);
	}
	menuBar->AddItem(fSizeMenu);

	// Style: MacPaint's shortcuts, except Outline and Shadow, whose O and S
	// are Open and Save on Haiku.
	BMenu* styleMenu = new BMenu(B_TRANSLATE("Style"));
	const struct {
		const char*	label;
		char		shortcut;
	} faces[] = {
		{B_TRANSLATE_MARK("Plain"), 'P'}, {B_TRANSLATE_MARK("Bold"), 'B'},
		{B_TRANSLATE_MARK("Italic"), 'I'},
		{B_TRANSLATE_MARK("Underline"), 'U'},
		{B_TRANSLATE_MARK("Outline"), 0}, {B_TRANSLATE_MARK("Shadow"), 0}
	};
	for (int i = 0; i < 6; i++) {
		BMessage* message = new BMessage(kMsgSetTextFace);
		message->AddInt32("face", i);
		fFaceItems[i] = new BMenuItem(B_TRANSLATE_NOCOLLECT(faces[i].label),
			message,
			faces[i].shortcut);
		styleMenu->AddItem(fFaceItems[i]);
	}
	styleMenu->AddSeparatorItem();
	const struct {
		const char*	label;
		char		shortcut;
	} alignments[] = {
		{B_TRANSLATE_MARK("Align left"), 'L'},
		{B_TRANSLATE_MARK("Align middle"), 'M'},
		{B_TRANSLATE_MARK("Align right"), 'R'}
	};
	for (int i = 0; i < 3; i++) {
		BMessage* message = new BMessage(kMsgSetAlignment);
		message->AddInt32("alignment", i);
		fAlignItems[i] = new BMenuItem(
			B_TRANSLATE_NOCOLLECT(alignments[i].label), message,
			alignments[i].shortcut);
		styleMenu->AddItem(fAlignItems[i]);
	}
	menuBar->AddItem(styleMenu);
	_TextStyleChanged();
}


void
MainWindow::_TextStyleChanged()
{
	const TextStyle& style = fTextStyle;
	fFaceItems[0]->SetMarked(!style.bold && !style.italic && !style.underline
		&& !style.outline && !style.shadow);
	fFaceItems[1]->SetMarked(style.bold);
	fFaceItems[2]->SetMarked(style.italic);
	fFaceItems[3]->SetMarked(style.underline);
	fFaceItems[4]->SetMarked(style.outline);
	fFaceItems[5]->SetMarked(style.shadow);
	for (int i = 0; i < 3; i++)
		fAlignItems[i]->SetMarked((int)style.alignment == i);
	fCanvasView->SetTextStyle(style);
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
	text.SetToFormat(B_TRANSLATE("Save changes to the document \"%s\"?"),
		fName.String());
	BAlert* alert = new BAlert(B_TRANSLATE("Unsaved changes"), text.String(),
		B_TRANSLATE("Cancel"), B_TRANSLATE("Don't save"), B_TRANSLATE("Save"),
		B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
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
	_SetPatterns(hp::kPatterns);
	fHasRef = false;
	_SetDocumentName(B_TRANSLATE("untitled"));
}


void
MainWindow::_Open(const entry_ref& ref)
{
	hp::Canvas image;
	hp::Pattern patterns[hp::kPatternCount];
	bool isMacPaint = false;
	BString error;
	if (LoadDocument(ref, image, patterns, &isMacPaint, &error) != B_OK) {
		_ShowError(error);
		return;
	}

	fCanvasView->SetImage(image);
	_SetPatterns(patterns);
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
	if (SaveMacPaint(entry, fCanvasView->Image(), fPatterns, &error) != B_OK) {
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

	if (SaveMacPaint(entry, fCanvasView->Image(), fPatterns, &error) != B_OK) {
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
MainWindow::_SetPatterns(const hp::Pattern* patterns)
{
	std::memcpy(fPatterns, patterns, sizeof(fPatterns));
	fPatternPalette->SetPatterns(fPatterns);
	fCanvasView->SetPattern(fPatterns[fPatternIndex]);
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
	BAlert* alert = new BAlert(B_TRANSLATE("Error"), text.String(),
		B_TRANSLATE("OK"), NULL, NULL,
		B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert->Go(NULL);
}
