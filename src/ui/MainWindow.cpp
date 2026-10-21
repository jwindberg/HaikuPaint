#include "MainWindow.h"

#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollView.h>

#include "CanvasView.h"
#include "LineWidthPalette.h"
#include "Messages.h"
#include "PatternPalette.h"
#include "Patterns.h"
#include "ToolPalette.h"


MainWindow::MainWindow()
	:
	BWindow(BRect(100, 100, 750, 650), "HaikuPaint", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE
			| B_AUTO_UPDATE_SIZE_LIMITS)
{
	fCanvasView = new CanvasView();
	fCanvasView->SetPattern(hp::kPatterns[hp::kDefaultPatternIndex]);
	BScrollView* scrollView = new BScrollView("scroller", fCanvasView, 0,
		true, true, B_NO_BORDER);

	fToolPalette = new ToolPalette();
	fLineWidthPalette = new LineWidthPalette();
	fPatternPalette = new PatternPalette();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(_BuildMenuBar())
		.AddGroup(B_HORIZONTAL, 0)
			.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
				.SetInsets(B_USE_SMALL_SPACING)
				.Add(fToolPalette)
				.Add(fLineWidthPalette)
				.AddGlue()
			.End()
			.Add(scrollView)
		.End()
		.Add(fPatternPalette);
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
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
			if (message->FindInt32("width", &width) == B_OK) {
				fCanvasView->SetLineWidth(width);
				fLineWidthPalette->SetLineWidth(width);
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


BMenuBar*
MainWindow::_BuildMenuBar()
{
	BMenuBar* menuBar = new BMenuBar("menubar");

	BMenu* fileMenu = new BMenu("File");
	fileMenu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED),
		'Q'));
	menuBar->AddItem(fileMenu);

	BMenu* editMenu = new BMenu("Edit");
	editMenu->AddItem(new BMenuItem("Clear", new BMessage(kMsgClear)));
	menuBar->AddItem(editMenu);

	return menuBar;
}
