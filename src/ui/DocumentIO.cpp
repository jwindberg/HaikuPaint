#include "DocumentIO.h"

#include <Bitmap.h>
#include <BitmapStream.h>
#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <MimeType.h>
#include <NodeInfo.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>

#include <algorithm>
#include <memory>
#include <string>
#include <string.h>
#include <strings.h>
#include <vector>

#include "App.h"
#include "BitmapUtils.h"
#include "MacPaintFile.h"
#include "Patterns.h"


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
		mime.SetShortDescription("MacPaint image");
	if (mime.GetLongDescription(text) != B_OK)
		mime.SetLongDescription("MacPaint document");
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


// Converts a translated image to MacPaint's black and white, top left.
static status_t
ImageFromBitmap(const BBitmap* source, hp::Canvas& image)
{
	BBitmap rgba(source->Bounds(), B_RGBA32);
	status_t status = rgba.ImportBits(source);
	if (status != B_OK)
		return status;

	image.Clear();
	int width = std::min((int)source->Bounds().Width() + 1, image.Width());
	int height = std::min((int)source->Bounds().Height() + 1,
		image.Height());
	const uint8* bits = (const uint8*)rgba.Bits();
	for (int y = 0; y < height; y++) {
		const uint8* pixel = bits + y * rgba.BytesPerRow();
		for (int x = 0; x < width; x++, pixel += 4) {
			// B_RGBA32 is B, G, R, A in memory. Transparent counts as
			// white; otherwise threshold the luminance.
			int luminance = (114 * pixel[0] + 587 * pixel[1]
				+ 299 * pixel[2]) / 1000;
			image.Set(x, y, pixel[3] >= 128 && luminance < 128);
		}
	}
	return B_OK;
}


status_t
LoadDocument(const entry_ref& ref, hp::Canvas& image, bool* isMacPaint,
	BString* error)
{
	BFile file(&ref, B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK) {
		error->SetToFormat("Couldn't open the file: %s", strerror(status));
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

	std::string reason;
	bool triedMacPaint = false;
	if (!data.empty() && LooksLikeMacPaint(file, ref, data)) {
		triedMacPaint = true;
		if (hp::ReadMacPaint(data.data(), data.size(), image, &reason)) {
			*isMacPaint = true;
			return B_OK;
		}
	}

	std::unique_ptr<BBitmap> bitmap(BTranslationUtils::GetBitmap(&ref));
	if (bitmap && ImageFromBitmap(bitmap.get(), image) == B_OK) {
		*isMacPaint = false;
		return B_OK;
	}

	// Files that lost their type and name, e.g. copied off an old disk.
	if (!triedMacPaint && !data.empty()
		&& hp::ReadMacPaint(data.data(), data.size(), image, &reason)) {
		*isMacPaint = true;
		return B_OK;
	}

	if (triedMacPaint)
		error->SetTo(reason.c_str());
	else
		error->SetTo("HaikuPaint can't read this kind of file.");
	return B_ERROR;
}


status_t
SaveMacPaint(const BEntry& entry, const hp::Canvas& image, BString* error)
{
	BFile file(&entry, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status == B_OK) {
		std::vector<uint8_t> data = hp::WriteMacPaint(image, hp::kPatterns);
		ssize_t written = file.Write(data.data(), data.size());
		if (written < 0)
			status = written;
		else if ((size_t)written != data.size())
			status = B_IO_ERROR;
	}
	if (status != B_OK) {
		error->SetToFormat("Couldn't save the document: %s",
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
		error->SetToFormat("Couldn't export the image: %s",
			strerror(status));
		return status;
	}

	BNodeInfo(&file).SetType("image/png");
	return B_OK;
}
