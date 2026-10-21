#include "MainWindow.h"

#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollView.h>

#include "BrushWindow.h"
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
