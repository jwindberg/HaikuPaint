#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <String.h>

#include <vector>

#include "Canvas.h"
#include "Geometry.h"


// MacPaint's Font, FontSize and Style menus, applied to the text tool.
enum class TextAlignment {
	Left,
	Middle,
	Right
};

struct TextStyle {
	BString				family;
	float				size;
	bool				bold;
	bool				italic;
	bool				underline;
	bool				outline;
	bool				shadow;
	TextAlignment		alignment;

						TextStyle();
};


// A block of text as MacPaint's UpdateText draws it: line by line from the
// click point, which is the first baseline, aligned around it horizontally.
struct TextLayout {
	hp::Rect			bounds;		// on the page
	hp::Canvas			mask;		// bounds-sized; set where text is black
	hp::Point			caret;		// top of the caret, after the last line
	int					caretHeight;

						TextLayout();
};

// Renders the lines in black, one bit deep. With grid, line spacing rounds
// up to a multiple of 8, as in MacPaint.
TextLayout	RenderText(const std::vector<BString>& lines,
				const TextStyle& style, hp::Point origin, bool grid);


#endif	// TEXT_RENDERER_H
