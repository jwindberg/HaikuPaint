#ifndef MESSAGES_H
#define MESSAGES_H

#include <SupportDefs.h>


// MacPaint's tool numbers (MacPaint.p: lassoTool = 0 ...), which are also
// the tools' positions in the palette, two per row.
enum class Tool : int32 {
	Lasso,
	Select,
	Grabber,
	Text,
	PaintBucket,
	Spray,
	Brush,
	Pencil,
	Line,
	Eraser,
	Rect,
	FilledRect,
	RoundRect,
	FilledRoundRect,
	Oval,
	FilledOval,
	Freeform,
	FilledFreeform,
	Polygon,
	FilledPolygon
};

static const int kToolCount = 20;

// Tools HaikuPaint doesn't have yet show in the palette but can't be
// chosen.
inline bool
IsToolImplemented(Tool tool)
{
	switch (tool) {
		case Tool::Grabber:
		case Tool::Text:
			return false;
		default:
			return true;
	}
}


// Messages the palettes post to MainWindow, which forwards them to the
// canvas and keeps the palettes' highlights in sync.
enum {
	kMsgClear			= 'clr ',
	kMsgSetTool			= 'stol',	// "tool" int32 (a Tool)
	kMsgSetPattern		= 'spat',	// "index" int32 (into hp::kPatterns)
	kMsgSetLineWidth	= 'slwd',	// "width" int32 (pixels), "border" bool
									// (false: MacPaint's "no border")
	kMsgChooseBrush		= 'cbsh',	// opens the brush shape chooser
	kMsgSetBrush		= 'sbsh',	// "brush" int32 (0-31, as theBrush)
	kMsgSelectAll		= 'sall'	// also: double-click the marquee
};


// Edit menu operations on the selection.
enum class SelectionCommand {
	Invert,
	Fill,
	TraceEdges,
	TraceEdgesShadow,
	FlipHorizontal,
	FlipVertical,
	Rotate
};


#endif	// MESSAGES_H
