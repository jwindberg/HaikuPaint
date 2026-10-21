#include "DocumentIO.h"

#include <Bitmap.h>
#include <BitmapStream.h>
#include <Catalog.h>
#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <MimeType.h>
#include <NodeInfo.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <string.h>
#include <strings.h>
#include <vector>

#include "App.h"
#include "BitmapUtils.h"
#include "MacIcons.h"
#include "MacPaintFile.h"
#include "Patterns.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DocumentIO"


const char* kMacPaintMimeType = "image/x-macpaint";

// Large enough for any real MacPaint file, which is under 60 KB.
static const off_t kMaxMacPaintFileSize = 1024 * 1024;


void
InstallMacPaintMimeType()
{
	// The registrar may already have created the type from the app's
	// file_types resource, so fill in whatever is missing rather than
	// stopping when it exists.
	BMimeType mime(kMacPaintMimeType);
	if (!mime.IsInstalled())
		mime.Install();

	char text[B_MIME_TYPE_LENGTH];
	if (mime.GetShortDescription(text) != B_OK)
		mime.SetShortDescription(B_TRANSLATE("MacPaint image"));
	if (mime.GetLongDescription(text) != B_OK)
		mime.SetLongDescription(B_TRANSLATE("MacPaint document"));
	if (mime.GetPreferredApp(text) != B_OK)
		mime.SetPreferredApp(kAppSignature);

	BMessage extensions;
	if (mime.GetFileExtensions(&extensions) != B_OK
		|| !extensions.HasString("extensions")) {
		extensions.MakeEmpty();
		extensions.AddString("extensions", "mac");
		extensions.AddString("extensions", "pntg");
		extensions.AddString("extensions", "pnt");
		mime.SetFileExtensions(&extensions);
	}

	// MacPaint's own document icon, unless the type has one already.
	BBitmap existing(BRect(0, 0, 31, 31), B_CMAP8);
	if (mime.GetIcon(&existing, B_LARGE_ICON) != B_OK) {
		const struct {
			icon_size	which;
			int			size;
		} icons[] = {{B_LARGE_ICON, 32}, {B_MINI_ICON, 16}};
		for (const auto& icon : icons) {
			std::unique_ptr<BBitmap> bitmap(NewMacIconBitmap(kDocumentIcon,
				kDocumentIconMask, icon.size));
			mime.SetIcon(bitmap.get(), icon.which);
		}
	}
}


// Whether the file's type, name or MacBinary header says MacPaint.
static bool
LooksLikeMacPaint(BFile& file, const entry_ref& ref,
	const std::vector<uint8_t>& data)
{
	char type[B_MIME_TYPE_LENGTH];
	BNodeInfo info(&file);
	if (info.GetType(type) == B_OK && strcasecmp(type, kMacPaintMimeType) == 0)
		return true;

	BString name(ref.name);
	name.ToLower();
	if (name.EndsWith(".mac") || name.EndsWith(".pntg")
		|| name.EndsWith(".pnt"))
		return true;

	return hp::MacPaintDataOffset(data.data(), data.size()) != 0;
}


bool
IsMacPaintDocument(const entry_ref& ref)
{
	BFile file(&ref, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return false;

	// Enough for a MacBinary header and the MacPaint header after it.
	std::vector<uint8_t> start(128 + hp::kMacPaintHeaderSize);
	ssize_t read = file.ReadAt(0, start.data(), start.size());
	start.resize(read > 0 ? read : 0);
	return LooksLikeMacPaint(file, ref, start);
}


status_t
LoadDocument(const entry_ref& ref, hp::Canvas& image, hp::Pattern* patterns,
	bool* isMacPaint, BString* error)
{
	std::memcpy(patterns, hp::kPatterns, sizeof(hp::kPatterns));

	BFile file(&ref, B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK) {
		error->SetToFormat(B_TRANSLATE("Couldn't open the file: %s"),
			strerror(status));
		return status;
	}

	off_t size = 0;
	file.GetSize(&size);
	std::vector<uint8_t> data;
	if (size > 0 && size <= kMaxMacPaintFileSize) {
		data.resize(size);
		if (file.ReadAt(0, data.data(), size) != size)
			data.clear();
	}

	// The engine's reasons (MacPaintFile.cpp), listed here so they get
	// collected for translation; they're translated where shown, below.
	B_TRANSLATE_MARK_VOID("The canvas isn't MacPaint's size.");
	B_TRANSLATE_MARK_VOID("The file is too short to be a MacPaint document.");
	B_TRANSLATE_MARK_VOID("This doesn't look like a MacPaint document.");
	B_TRANSLATE_MARK_VOID("The MacPaint document is damaged or incomplete.");

	std::string reason;
	bool triedMacPaint = false;
	if (!data.empty() && LooksLikeMacPaint(file, ref, data)) {
		triedMacPaint = true;
		if (hp::ReadMacPaint(data.data(), data.size(), image, &reason,
				patterns)) {
			*isMacPaint = true;
			return B_OK;
		}
	}

	std::unique_ptr<BBitmap> bitmap(BTranslationUtils::GetBitmap(&ref));
	if (bitmap && CanvasFromBitmap(bitmap.get(), image) == B_OK) {
		*isMacPaint = false;
		return B_OK;
	}

	// Files that lost their type and name, e.g. copied off an old disk.
	if (!triedMacPaint && !data.empty()
		&& hp::ReadMacPaint(data.data(), data.size(), image, &reason,
				patterns)) {
		*isMacPaint = true;
		return B_OK;
	}

	if (triedMacPaint)
		error->SetTo(B_TRANSLATE_NOCOLLECT(reason.c_str()));
	else
		error->SetTo(B_TRANSLATE("HaikuPaint can't read this kind of file."));
	return B_ERROR;
}


status_t
SaveMacPaint(const BEntry& entry, const hp::Canvas& image,
	const hp::Pattern* patterns, BString* error)
{
	BFile file(&entry, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status == B_OK) {
		std::vector<uint8_t> data = hp::WriteMacPaint(image, patterns);
		ssize_t written = file.Write(data.data(), data.size());
		if (written < 0)
			status = written;
		else if ((size_t)written != data.size())
			status = B_IO_ERROR;
	}
	if (status != B_OK) {
		error->SetToFormat(B_TRANSLATE("Couldn't save the document: %s"),
			strerror(status));
		return status;
	}

	BNodeInfo(&file).SetType(kMacPaintMimeType);
	return B_OK;
}


status_t
ExportPNG(const BEntry& entry, const hp::Canvas& image, BString* error)
{
	BFile file(&entry, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status == B_OK) {
		// The stream owns the bitmap and deletes it.
		BBitmapStream stream(NewBitmapFromCanvas(image));
		status = BTranslatorRoster::Default()->Translate(&stream, NULL, NULL,
			&file, B_PNG_FORMAT);
	}
	if (status != B_OK) {
		error->SetToFormat(B_TRANSLATE("Couldn't export the image: %s"),
			strerror(status));
		return status;
	}

	BNodeInfo(&file).SetType("image/png");
	return B_OK;
}
