#include "MainWindow.h"

#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollView.h>

#include "CanvasView.h"


enum {
	kMsgClear			= 'clr ',
	kMsgToolPencil		= 'tpen',
	kMsgToolLine		= 'tlin',
	kMsgToolPaintBucket	= 'tbkt'
};


MainWindow::MainWindow()
	:
	BWindow(BRect(100, 100, 700, 600), "HaikuPaint", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
{
	fCanvasView = new CanvasView();
	BScrollView* scrollView = new BScrollView("scroller", fCanvasView, 0,
		true, true, B_NO_BORDER);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(_BuildMenuBar())
		.Add(scrollView);
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgClear:
			fCanvasView->Clear();
			break;
		case kMsgToolPencil:
			fCanvasView->SetTool(Tool::Pencil);
			break;
		case kMsgToolLine:
			fCanvasView->SetTool(Tool::Line);
			break;
		case kMsgToolPaintBucket:
			fCanvasView->SetTool(Tool::PaintBucket);
			break;
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

	// Temporary until there is a real tool palette (see docs/ROADMAP.md).
	BMenu* toolMenu = new BMenu("Tools");
	toolMenu->SetRadioMode(true);
	BMenuItem* pencilItem = new BMenuItem("Pencil",
		new BMessage(kMsgToolPencil), '1');
	pencilItem->SetMarked(true);
	toolMenu->AddItem(pencilItem);
	toolMenu->AddItem(new BMenuItem("Line", new BMessage(kMsgToolLine), '2'));
	toolMenu->AddItem(new BMenuItem("Paint bucket",
		new BMessage(kMsgToolPaintBucket), '3'));
	menuBar->AddItem(toolMenu);

	return menuBar;
}
