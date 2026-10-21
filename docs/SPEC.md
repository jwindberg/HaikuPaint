# HaikuPaint feature spec

Target: the feel of MacPaint 1.x (1984–85), rebuilt as a native Haiku app.
These notes are from general knowledge of MacPaint. **Check each detail
against the CHM source, screenshots or an emulator (Mini vMac) before treating
it as exact.** Mark items as verified once you've checked them.

## Document
- 1-bit (black and white) bitmap, 576 × 720 pixels (8 × 10 in at 72 dpi).
- Only part of the page shows at once in the original. HaikuPaint can show
  more, but should keep the fixed page size.
- One level of undo, covering the last operation.

## Tools (original palette, roughly in order)
| Tool | Behavior notes |
|---|---|
| Lasso | Freehand selection that shrinks to fit the enclosed non-white pixels. The hardest tool; study the source. |
| Marquee (selection rectangle) | Rectangular selection. Drag to move. Option-drag copies, Shift constrains the direction, Command-drag stretches. |
| Grabber (hand) | Scrolls the page inside the window. |
| Text | Type text in a chosen font, size and style. Stays editable until you click elsewhere. |
| Paint bucket | Flood-fills the region under the click with the current pattern. Pattern aligned to the page. |
| Spray can | Sprays random dots in the current pattern. |
| Paintbrush | Paints with the current brush shape and pattern. |
| Pencil | 1-pixel lines. Draws white if the stroke starts on a black pixel. Shift constrains the direction. |
| Line | Rubber-band straight line using the current line width. Shift constrains to 45° angles. |
| Eraser | Square eraser. Double-clicking it erases the whole page. |
| Rectangle, rounded rectangle, oval, polygon, freeform | Hollow and filled variants. The filled ones use the current pattern, and the border uses the line width. Shift constrains to square or circle. |

## Palettes and settings
- **Pattern palette:** 38 8×8 patterns along the bottom of the window, which
  you can edit. Take the actual bit patterns from the reference source rather
  than guessing.
- **Line width:** several widths, including a dotted or "none" choice for
  borders.
- **Brush shapes:** a set of brush shapes (choose by double-clicking the
  brush tool).
- **Brush mirrors:** draw symmetrically across horizontal, vertical or
  diagonal axes.

## Goodies and other menus
- **FatBits:** a zoomed pixel-editing view, with a small actual-size preview.
- **Show page:** a scaled-down view of the whole page, for moving the visible
  area.
- **Edit menu:** Undo, Cut, Copy, Paste, Clear, Invert, Fill, Trace Edges,
  Flip Horizontal or Vertical, Rotate.
- **Font, FontSize and Style menus** for the text tool.

## Haiku-native additions (not in the original)
- Load and save through the Translation Kit (PNG, BMP and others), plus a
  native file format and MIME type.
- Drag and drop with Tracker.
- Window resizing, and a scrollable view of the whole page.
- A MacPaint file import, maybe later (the format is a 512-byte header plus
  PackBits-compressed rows).
