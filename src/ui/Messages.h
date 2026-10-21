#ifndef MESSAGES_H
#define MESSAGES_H

#include <SupportDefs.h>


enum class Tool : int32 {
	Pencil,
	Line,
	PaintBucket,
	Eraser
};


// Messages the palettes post to MainWindow, which forwards them to the
// canvas and keeps the palettes' highlights in sync.
enum {
	kMsgClear			= 'clr ',
	kMsgSetTool			= 'stol',	// "tool" int32 (a Tool)
	kMsgSetPattern		= 'spat',	// "index" int32 (into hp::kPatterns)
	kMsgSetLineWidth	= 'slwd'	// "width" int32 (pixels)
};


#endif	// MESSAGES_H
