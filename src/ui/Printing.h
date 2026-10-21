#ifndef PRINTING_H
#define PRINTING_H

#include <SupportDefs.h>

#include <vector>

#include "Canvas.h"


class BMessage;


// Prints the page through Haiku's print kit. MacPaint's page is 576 x 720
// pixels at 72 dpi, 8 x 10 inches, which prints at that size when it fits
// the printable area and is scaled down to fit when it doesn't.
//
// settings holds the page setup between calls (NULL until there is one);
// these functions replace it when the user changes it.
status_t	ConfigurePage(BMessage** settings);
status_t	PrintPage(const hp::Canvas& page, const char* name,
				BMessage** settings);
status_t	PrintPages(const std::vector<hp::Canvas>& pages, const char* name,
				BMessage** settings);


#endif	// PRINTING_H
