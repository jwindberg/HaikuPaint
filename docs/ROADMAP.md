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
- [ ] Document icon (HVIF) for MacPaint files
- [ ] Use the patterns saved in a document (needs pattern editing)
- [ ] Dither instead of threshold when converting color images

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
- [ ] Command (Alt) as OR mode for painting (JamLine/JamFill's patOr)

## M5: Goodies
- [ ] FatBits zoom mode
- [ ] Grabber tool and Show Page
- [ ] Text tool with font, size and style
- [ ] Brush mirrors

## Later
- [ ] Localization (Haiku's Locale Kit)
- [ ] Package as an .hpkg, submit to HaikuPorts
