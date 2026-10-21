#ifndef MAC_ICONS_H
#define MAC_ICONS_H

#include <cstdint>


// MacPaint's own icons, generated into MacIcons.cpp by
// tools/extract_macpaint_art.py: 32 x 32, one bit deep, 4 bytes per row,
// MSB leftmost (QuickDraw's ICON and ICN# formats).
static const int kMacIconSize = 32;
static const int kMacIconBytes = kMacIconSize * kMacIconSize / 8;

extern const uint8_t kAppIcon[kMacIconBytes];
extern const uint8_t kAppIconMask[kMacIconBytes];
extern const uint8_t kDocumentIcon[kMacIconBytes];
extern const uint8_t kDocumentIconMask[kMacIconBytes];
extern const uint8_t kAboutIcon[kMacIconBytes];		// the "BILL ICON"


#endif	// MAC_ICONS_H
