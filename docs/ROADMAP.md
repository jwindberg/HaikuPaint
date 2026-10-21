# Roadmap

Keep each milestone shippable. Engine work comes with tests in
`tests/test_canvas.cpp`.

## M0: Skeleton (done)
- [x] Engine: 1-bit `hp::Canvas`, lines, pattern flood fill, host tests
- [x] Haiku app: window, menus, scrollable canvas
- [x] Tools (from a menu): pencil, line, paint bucket

## M1: Palettes
- [ ] Tool palette view (a grid of icon buttons, left side)
- [ ] Pattern palette with the 38 original patterns (bottom); the paint
      bucket uses the selected pattern
- [ ] Line width selector
- [ ] Eraser; double-clicking the eraser clears the page

## M2: Shapes and brushes
- [ ] Engine: thick lines, rectangles, ovals and rounded rectangles, hollow
      and filled with a pattern
- [ ] Rubber-band previews for all shape tools; Shift constraints
- [ ] Paintbrush with brush shapes; spray can
- [ ] Polygon and freeform shapes

## M3: Undo, files
- [ ] One-level undo (snapshot of the canvas before each operation)
- [ ] File > New / Open / Save / Save As through the Translation Kit
- [ ] Native MIME type, document icon, "unsaved changes" prompt on quit

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
- [ ] Import MacPaint files (PackBits)
- [ ] Localization (Haiku's Locale Kit)
- [ ] Package as an .hpkg, submit to HaikuPorts
