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
- [ ] Option key: draw borders and lines in the current pattern (JamLine)

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

## M4: Selections
- [ ] Marquee: select, move, Option-drag to copy, Delete to clear
- [ ] Clipboard: Cut, Copy and Paste through BClipboard
- [ ] Lasso with shrink-to-fit (study the reference source)
- [ ] Edit transforms: Invert, Fill, Flip, Rotate, Trace Edges

## M5: Goodies
- [ ] FatBits zoom mode
- [ ] Grabber tool and Show Page
- [ ] Text tool with font, size and style
- [ ] Brush mirrors

## Later
- [ ] Localization (Haiku's Locale Kit)
- [ ] Package as an .hpkg, submit to HaikuPorts
