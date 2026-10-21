#include "App.h"

#include <Entry.h>

#include "Dialogs.h"
#include "DocumentIO.h"
#include "MainWindow.h"


// Must match the signature in HaikuPaint.rdef.
const char* kAppSignature = "application/x-vnd.HaikuPaint";


App::App()
	:
	BApplication(kAppSignature)
{
	// Created now, so files passed at launch, which can arrive before
	// ReadyToRun(), have somewhere to go.
	fWindow = new MainWindow();
}


void
App::AboutRequested()
{
	(new AboutWindow())->Show();
}


void
App::ArgvReceived(int32 argc, char** argv)
{
	BMessage refs(B_REFS_RECEIVED);
	for (int32 i = 1; i < argc; i++) {
		entry_ref ref;
		if (get_ref_for_path(argv[i], &ref) == B_OK)
			refs.AddRef("refs", &ref);
	}
	if (refs.HasRef("refs"))
		RefsReceived(&refs);
}


void
App::RefsReceived(BMessage* message)
{
	// One document window, as in MacPaint: open the first file.
	fWindow->PostMessage(message);
}


void
App::ReadyToRun()
{
	InstallMacPaintMimeType();
	fWindow->Show();
}


int
main()
{
	App app;
	app.Run();
	return 0;
}
