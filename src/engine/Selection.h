#ifndef HP_SELECTION_H
#define HP_SELECTION_H

#include <vector>

#include "Canvas.h"
#include "Geometry.h"


namespace hp {


// A selected piece of the page, as made by MacPaint's selection rectangle
// or lasso. The pixels are a copy; the page itself is only changed by
// Stamp() and EraseUnder().
struct Selection {
	Rect				rect;		// where it is on the page
	Canvas				pixels;		// rect-sized
	Canvas				mask;		// rect-sized; set where selected
	bool				lasso;		// lasso selections are transparent
									// outside the mask when moved

	Selection();

	// Whether the point (page coordinates) is on the selection.
	bool				Contains(Point point) const;

	// The pixels MacPaint marches its ants over: the inner edge of the
	// mask, which for a rectangle is its frame. Page coordinates.
	std::vector<Point>	Edges() const;
};


// A rectangular selection of the page, clipped to it.
Selection	SelectRect(const Canvas& page, Rect rect);

// MacPaint's Lasso: selects what the loop encloses, shrunk to fit. Black
// pixels and the white areas they fully enclose are selected; white
// background connected to the outside of the loop is not. Returns false if
// nothing would be selected.
bool		SelectLasso(const Canvas& page, const std::vector<Point>& loop,
				Selection* selection);

// Paints the selection onto the page: rect selections opaquely, lasso
// selections only where the mask is set.
void		Stamp(Canvas& page, const Selection& selection);

// Whitens the page under the selection's mask: the hole left behind when a
// selection is moved or cleared.
void		EraseUnder(Canvas& page, const Selection& selection);

// Edit menu operations, on the selection's pixels (and mask where it
// matters). The selection keeps its center when rotated.
void		Invert(Selection& selection);
void		Fill(Selection& selection, const Pattern& pattern);
void		FlipHorizontal(Selection& selection);
void		FlipVertical(Selection& selection);
void		RotateLeft(Selection& selection);

// MacPaint's Trace Edges: outlines black areas by smearing them one pixel
// each way and removing the original. shadow (Shift in MacPaint) smears two
// pixels right and down instead, for an outline with a drop shadow.
void		TraceEdges(Selection& selection, bool shadow);

// The selection scaled to fill rect (nearest neighbor), for stretching.
Selection	Stretched(const Selection& source, Rect rect);

// QuickDraw's MapRect: rect mapped from one frame of reference to another.
Rect		MapRect(Rect rect, Rect from, Rect to);

// HaikuPaint's clipboard format: keeps the mask, so lasso selections paste
// transparent. The position isn't stored; pasting places it.
std::vector<uint8_t>	EncodeSelection(const Selection& selection);
bool		DecodeSelection(const uint8_t* data, size_t size,
				Selection* selection);


}	// namespace hp

#endif	// HP_SELECTION_H
