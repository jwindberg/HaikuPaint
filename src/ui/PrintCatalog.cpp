#include "PrintCatalog.h"

#include <Directory.h>
#include <Entry.h>

#include <algorithm>

#include "BitmapUtils.h"
#include "DocumentIO.h"
#include "Patterns.h"
#include "TextRenderer.h"


// From PrintCatalog and GetCatPic: bands of 144 pixels, five to a page,
// each with five pictures 72 x 90 at 112 pixel steps from x 28, 32 pixels
// down, with the name centered 16 pixels below.
static const int kPerRow = 5;
static const int kRowsPerPage = 5;
static const int kBandHeight = 144;
static const int kReduction = 8;


// Draws text in black with its first baseline at (x, y).
static void
DrawText(hp::Canvas& page, const BString& text, hp::Point at,
	TextAlignment alignment)
{
	TextStyle style;
	style.alignment = alignment;
	TextLayout layout = RenderText({text}, style, at, false);
	for (int y = 0; y < layout.mask.Height(); y++) {
		for (int x = 0; x < layout.mask.Width(); x++) {
			if (layout.mask.Get(x, y))
				page.Set(layout.bounds.left + x, layout.bounds.top + y, true);
		}
	}
}


std::vector<hp::Canvas>
BuildCatalogPages(const entry_ref& folder, const BString& heading)
{
	// The folder's MacPaint documents, by name.
	BDirectory directory(&folder);
	std::vector<entry_ref> refs;
	entry_ref ref;
	while (directory.GetNextRef(&ref) == B_OK) {
		BEntry entry(&ref, true);
		if (entry.IsFile() && IsMacPaintDocument(ref))
			refs.push_back(ref);
	}
	std::sort(refs.begin(), refs.end(),
		[](const entry_ref& a, const entry_ref& b) {
			return strcasecmp(a.name, b.name) < 0;
		});

	std::vector<hp::Canvas> pages;
	int slot = 0;
	for (const entry_ref& document : refs) {
		hp::Canvas image;
		hp::Pattern patterns[hp::kPatternCount];
		bool isMacPaint = false;
		BString error;
		if (LoadDocument(document, image, patterns, &isMacPaint, &error)
				!= B_OK || !isMacPaint)
			continue;

		int onPage = slot % (kPerRow * kRowsPerPage);
		if (onPage == 0) {
			pages.push_back(hp::Canvas());
			DrawText(pages.back(), heading, {28, 10}, TextAlignment::Left);
		}
		hp::Canvas& page = pages.back();

		int left = 28 + 112 * (onPage % kPerRow);
		int top = (onPage / kPerRow) * kBandHeight + 32;
		hp::Canvas small = hp::Reduced(image, kReduction,
			kReduction * kReduction / 4);
		for (int y = 0; y < small.Height(); y++) {
			for (int x = 0; x < small.Width(); x++)
				page.Set(left + x, top + y, small.Get(x, y));
		}
		FrameQDRect(page, left, top, left + small.Width(),
			top + small.Height());
		DrawText(page, document.name,
			{left + small.Width() / 2, top + small.Height() + 16},
			TextAlignment::Middle);
		slot++;
	}
	return pages;
}
