#ifndef DOCUMENT_IO_H
#define DOCUMENT_IO_H

#include <String.h>
#include <SupportDefs.h>

#include "Canvas.h"


class BEntry;
struct entry_ref;


// HaikuPaint's native format is MacPaint's own (Mac file type 'PNTG').
extern const char* kMacPaintMimeType;

// Registers kMacPaintMimeType with the system if it isn't known yet.
void		InstallMacPaintMimeType();

// A quick check, without reading the whole file: whether its type, name or
// MacBinary header says it's a MacPaint document.
bool		IsMacPaintDocument(const entry_ref& ref);

// Loads a document into image (576 x 720) and its 38 patterns into
// patterns. MacPaint files are read as they are (with the standard patterns
// if they have none of their own); anything a Translation Kit translator
// can read is converted to black and white and placed at the top left,
// cropped to fit, with the standard patterns. isMacPaint says which
// happened, so callers know whether saving may overwrite the file.
status_t	LoadDocument(const entry_ref& ref, hp::Canvas& image,
				hp::Pattern* patterns, bool* isMacPaint, BString* error);

// Writes image and its patterns as a MacPaint document, creating or
// replacing the file.
status_t	SaveMacPaint(const BEntry& entry, const hp::Canvas& image,
				const hp::Pattern* patterns, BString* error);

// Writes image as a PNG through the Translation Kit.
status_t	ExportPNG(const BEntry& entry, const hp::Canvas& image,
				BString* error);


#endif	// DOCUMENT_IO_H
