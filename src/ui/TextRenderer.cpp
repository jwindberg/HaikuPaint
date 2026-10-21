#include "TextRenderer.h"

#include <Bitmap.h>
#include <Font.h>
#include <View.h>

#include <algorithm>
#include <cmath>

#include "Selection.h"


TextStyle::TextStyle()
	:
	size(12),
	bold(false),
	italic(false),
	underline(false),
	outline(false),
	shadow(false),
	alignment(TextAlignment::Left)
{
	font_family family;
	font_style style;
	be_plain_font->GetFamilyAndStyle(&family, &style);
	this->family = family;
}


TextLayout::TextLayout()
	:
	bounds({0, 0, 0, 0}),
	mask(0, 0),
	caret({0, 0}),
	caretHeight(0)
{
}


static BFont
FontFor(const TextStyle& style)
{
	BFont font;
	font.SetFamilyAndStyle(style.family.String(), NULL);
	font.SetSize(style.size);

	uint16 face = 0;
	if (style.bold)
		face |= B_BOLD_FACE;
	if (style.italic)
		face |= B_ITALIC_FACE;
	if (style.underline)
		face |= B_UNDERSCORE_FACE;
	font.SetFace(face != 0 ? face : B_REGULAR_FACE);

	// One bit deep, like QuickDraw's text.
	font.SetFlags(B_DISABLE_ANTIALIASING);
	return font;
}


TextLayout
RenderText(const std::vector<BString>& lines, const TextStyle& style,
	hp::Point origin, bool grid)
{
	BFont font = FontFor(style);
	font_height height;
	font.GetHeight(&height);
	int ascent = (int)ceilf(height.ascent);
	int descent = (int)ceilf(height.descent);
	int lineHeight = ascent + descent + (int)ceilf(height.leading);
	if (grid)
		lineHeight = hp::TruncToGrid(lineHeight + 7);

	// Where each line starts, aligned around the click point.
	std::vector<int> lefts;
	std::vector<int> widths;
	for (const BString& line : lines) {
		int width = (int)ceilf(font.StringWidth(line.String()));
		int left = origin.x;
		if (style.alignment == TextAlignment::Middle)
			left -= width / 2;
		else if (style.alignment == TextAlignment::Right)
			left -= width;
		lefts.push_back(left);
		widths.push_back(width);
	}

	TextLayout layout;
	int count = (int)lines.size();
	int lastBaseline = origin.y + (count - 1) * lineHeight;
	layout.caret = {count > 0 ? lefts.back() + widths.back() : origin.x,
		lastBaseline - ascent};
	layout.caretHeight = ascent + descent;
	if (count == 0)
		return layout;

	// Room for italic overhang, bold, and the outline and shadow smears.
	int pad = (int)(style.size / 2) + 4;
	int left = *std::min_element(lefts.begin(), lefts.end());
	int right = left;
	for (int i = 0; i < count; i++)
		right = std::max(right, lefts[i] + widths[i]);
	layout.bounds = {left - pad, origin.y - ascent - pad, right + pad,
		lastBaseline + descent + pad};

	BRect frame(0, 0, layout.bounds.Width() - 1,
		layout.bounds.Height() - 1);
	BBitmap bitmap(frame, B_RGB32, true);
	BView* view = new BView(frame, "text", B_FOLLOW_NONE, 0);
	bitmap.AddChild(view);
	if (bitmap.Lock()) {
		view->SetHighColor(0, 0, 0);
		view->SetLowColor(255, 255, 255);
		view->FillRect(frame, B_SOLID_LOW);
		view->SetFont(&font);
		for (int i = 0; i < count; i++) {
			view->DrawString(lines[i].String(),
				BPoint(lefts[i] - layout.bounds.left,
					origin.y + i * lineHeight - layout.bounds.top));
		}
		view->Sync();
		bitmap.Unlock();
	}

	layout.mask = hp::Canvas(layout.bounds.Width(), layout.bounds.Height());
	const uint8* bits = (const uint8*)bitmap.Bits();
	for (int y = 0; y < layout.mask.Height(); y++) {
		const uint8* pixel = bits + y * bitmap.BytesPerRow();
		for (int x = 0; x < layout.mask.Width(); x++, pixel += 4)
			layout.mask.Set(x, y, pixel[1] < 128);
	}

	// QuickDraw's outline and shadow styles are hollow letters, which is
	// what Trace Edges makes.
	if (style.outline || style.shadow) {
		hp::Selection text;
		text.rect = layout.bounds;
		text.pixels = layout.mask;
		text.mask = hp::Canvas(text.rect.Width(), text.rect.Height());
		text.mask.FillRect(0, 0, text.rect.Width() - 1,
			text.rect.Height() - 1, hp::kBlackPattern);
		hp::TraceEdges(text, style.shadow);
		layout.mask = text.pixels;
	}
	return layout;
}
