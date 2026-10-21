#ifndef PRINT_CATALOG_H
#define PRINT_CATALOG_H

#include <String.h>

#include <vector>

#include "Canvas.h"


struct entry_ref;


// MacPaint's Print Catalog: every MacPaint document in a folder as a
// framed eighth-size picture with its name, 5 across and 5 down on each
// 576 x 720 page, under a heading. Returns no pages if the folder has no
// MacPaint documents.
std::vector<hp::Canvas>	BuildCatalogPages(const entry_ref& folder,
							const BString& heading);


#endif	// PRINT_CATALOG_H
