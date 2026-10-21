# Roadmap

Keep each milestone shippable. Engine work comes with tests in
`tests/test_canvas.cpp`.

## M0: Skeleton (done)
- [x] Engine: 1-bit `hp::Canvas`, lines, pattern flood fill, host tests
- [x] Haiku app: window, menus, scrollable canvas
- [x] Tools (from a menu): pencil, line, paint bucket

## M1: Palettes (done)
- [x] Tool palette view (a grid of icon buttons, left side)
- [x] Pattern palette with 38 patterns (bottom); the paint bucket uses the
      selected pattern
- [x] Original artwork: patterns (System PAT# 0) and PaintFont icons and
      cursors, extracted by `tools/extract_macpaint_art.py`; palettes follow
      MacPaint's geometry
- [x] Line width selector (the line tool uses it)
- [x] Eraser; double-clicking the eraser clears the page

## M2: Shapes and brushes (done, pending testing on Haiku)
- [x] Engine: thick lines, rectangles, ovals and rounded rectangles, hollow
      and filled with a pattern
- [x] Exact live previews (snapshot and redraw, like MacPaint's alternate
      buffer); Shift constraints for shapes, lines, polygon edges, the pencil
      and the brushes
- [x] "No border" entry at the top of the line width palette (shapes only)
- [x] Paintbrush with MacPaint's 32 brush shapes (double-click the brush);
      spray can
- [x] Polygon and freeform shapes, hollow and filled

## M3: Undo, files (done, pending testing on Haiku)
- [x] One-level undo that toggles, like MacPaint's (Edit > Undo, Alt+Z)
- [x] MacPaint documents as the native format (`src/engine/MacPaintFile`):
      PackBits, version 2 header with patterns, MacBinary on open
- [x] File > New / Open / Save / Save As; Open converts anything a
      translator reads to black and white; File > Export as PNG
- [x] MIME type image/x-macpaint (.mac, .pntg, .pnt), "unsaved changes"
      prompt for New, Open and Quit
- [x] Document icon for MacPaint files (MacPaint's own ICN# 129, M6)
- [x] Use the patterns saved in a document (with Edit Pattern, M5)
- [x] Dither instead of threshold when converting color images (M6)

## M4: Selections (done, pending testing on Haiku)
- [x] Marquee: select, move, Ctrl-drag to copy (MacPaint's Option),
      Ctrl+Alt-drag to leave a trail, Alt-drag to stretch (Command; Shift
      keeps the aspect), Backspace to clear; double-click selects the window
- [x] Lasso with MacPaint's shrink-to-fit (CalcMask), transparent moves
- [x] Marching ants (GetSelPat) around rects and lasso edges
- [x] Clipboard: Cut, Copy and Paste; HaikuPaint's own format keeps lasso
      transparency, an archived BBitmap ("image/bitmap") goes along for
      other apps, and pasted bitmaps from them are converted
- [x] Edit transforms: Invert, Fill, Trace Edges (Shift: shadow), Flip
      horizontal/vertical, Rotate
- [x] Ctrl draws lines and borders in the current pattern (JamLine)
- [x] Command (Alt) as OR mode for painting (JamLine/JamFill's patOr, M6)

## M5: Goodies (done, pending testing on Haiku)
- [x] Goodies menu: Grid, FatBits, Show Page, Edit Pattern, Brush Shape,
      Brush Mirrors (no Introduction or Short Cuts screens)
- [x] Grid: 8 pixel snapping for MacPaint's cheapSet tools and selection
      moves
- [x] FatBits zoom mode with the actual size preview; double-click the
      pencil
- [x] Grabber tool; Show Page with page shifting; double-click the grabber
- [x] Edit Pattern (double-click a pattern); patterns saved per document
- [x] Brush mirrors (MapSym)
- [x] Text tool with Font, FontSize and Style menus, live until accepted

## M6: Finishing touches (done, pending testing on Haiku)
- [x] OR mode: Alt paints only a pattern's black pixels (patOr), for lines,
      shapes, polygons, freeforms, the brush and the spray can
- [x] File > Revert, with MacPaint's alert
- [x] About HaikuPaint, after MacPaint's About box with the "BILL ICON"
- [x] MacPaint's application icon (ICN# 128) for the app, its document icon
      (ICN# 129) for image/x-macpaint
- [x] Floyd-Steinberg dithering for opened and pasted images
      (`src/engine/Dither`)
- [x] File > Page setup and Print, through the print kit; 8 x 10 inches,
      scaled down to fit
- [x] MIT license; `scripts/make-package.sh` builds an .hpkg

## Later
- [ ] Localization (Haiku's Locale Kit)
- [ ] Submit the package to HaikuPorts (a recipe in haikuports)
- [ ] MacPaint's Print Catalog (thumbnails of several documents)
