#include "Printing.h"

#include <Bitmap.h>
#include <PrintJob.h>
#include <View.h>
#include <Window.h>

#include <algorithm>

#include "BitmapUtils.h"


// Draws a page into the print job, scaled to fit if it must be.
class PrintView : public BView {
public:
	PrintView(BRect frame)
		:
		BView(frame, "print", B_FOLLOW_NONE, B_WILL_DRAW),
		fBitmap(NULL)
	{
	}

	virtual ~PrintView()
	{
		delete fBitmap;
	}

	void SetPage(const hp::Canvas& page)
	{
		delete fBitmap;
		fBitmap = NewBitmapFromCanvas(page);
	}

	virtual void Draw(BRect updateRect)
	{
		if (fBitmap == NULL)
			return;
		BRect source = fBitmap->Bounds();
		BRect bounds = Bounds();
		float scale = std::min(1.0f,
			std::min((bounds.Width() + 1) / (source.Width() + 1),
				(bounds.Height() + 1) / (source.Height() + 1)));
		BRect dest(0, 0, (source.Width() + 1) * scale - 1,
			(source.Height() + 1) * scale - 1);
		DrawBitmap(fBitmap, source, dest);
	}

private:
	BBitmap*	fBitmap;
};


static void
StoreSettings(BPrintJob& job, BMessage** settings)
{
	delete *settings;
	*settings = job.Settings();
}


status_t
ConfigurePage(BMessage** settings)
{
	BPrintJob job("HaikuPaint");
	if (*settings != NULL)
		job.SetSettings(new BMessage(**settings));
	status_t status = job.ConfigPage();
	if (status == B_OK)
		StoreSettings(job, settings);
	return status;
}


status_t
PrintPage(const hp::Canvas& page, const char* name, BMessage** settings)
{
	return PrintPages(std::vector<hp::Canvas>(1, page), name, settings);
}


status_t
PrintPages(const std::vector<hp::Canvas>& pages, const char* name,
	BMessage** settings)
{
	if (pages.empty())
		return B_BAD_VALUE;

	BPrintJob job(name);
	if (*settings != NULL)
		job.SetSettings(new BMessage(**settings));
	else {
		status_t status = job.ConfigPage();
		if (status != B_OK)
			return status;
	}
	status_t status = job.ConfigJob();
	if (status != B_OK)
		return status;
	StoreSettings(job, settings);

	job.BeginJob();
	if (!job.CanContinue())
		return B_CANCELED;

	// DrawView() needs a view in a running window; a hidden one will do.
	BRect printable = job.PrintableRect();
	BRect frame(0, 0, printable.Width(), printable.Height());
	BWindow* window = new BWindow(frame, "Printing", B_NO_BORDER_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL, B_AVOID_FOCUS);
	PrintView* view = new PrintView(frame);
	window->AddChild(view);
	window->Hide();
	window->Show();

	bool printed = true;
	for (const hp::Canvas& page : pages) {
		if (window->Lock()) {
			view->SetPage(page);
			job.DrawView(view, frame, BPoint(0, 0));
			window->Unlock();
		}
		job.SpoolPage();
		// Checked before committing: a committed job can't continue either.
		if (!job.CanContinue()) {
			printed = false;
			break;
		}
	}
	if (printed)
		job.CommitJob();

	if (window->Lock())
		window->Quit();
	return printed ? B_OK : B_CANCELED;
}
