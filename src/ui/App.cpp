#include "App.h"

#include "MainWindow.h"


// Must match the signature in HaikuPaint.rdef.
const char* kAppSignature = "application/x-vnd.HaikuPaint";


App::App()
	:
	BApplication(kAppSignature)
{
}


void
App::ReadyToRun()
{
	MainWindow* window = new MainWindow();
	window->Show();
}


int
main()
{
	App app;
	app.Run();
	return 0;
}
