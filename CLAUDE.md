# HaikuPaint: notes for Claude

A MacPaint 1.x re-creation as a native Haiku app. C++17 with the Be API.
Read `docs/SPEC.md` for target behavior and `docs/ROADMAP.md` for what's
next. Tick roadmap boxes when you finish items.

## Where things run
- This repo lives on the user's **Mac**, and you run there. The user usually
  connects over SSH from a Haiku laptop.
- **The Be API code (`src/ui/`) cannot be compiled on the Mac.** There are
  no Haiku headers here. To compile it:
  - `scripts/haiku-build.sh` rsyncs to the Haiku machine and runs `make`
    there (needs `HAIKU_HOST` and sshd on Haiku). Try it. If ssh fails, ask
    the user to build on Haiku and paste the errors.
- `make -C tests` builds and runs the engine tests on the Mac. Run it after
  every engine change.
- You can't see the GUI. Ask the user to try UI changes, and be specific
  about what to click and what should happen.

## Architecture rules
- `src/engine/` is pure, portable C++17: **no Haiku or OS headers**. All
  pixel logic goes here (drawing, fills, selections, undo, file encoding),
  in namespace `hp`, with tests in `tests/test_canvas.cpp` (add new test
  files to `tests/Makefile` if you split them).
- `src/ui/` only translates Be API events into engine calls and copies the
  canvas into a `BBitmap` for display. Keep the logic out of it.
- New `.cpp` files must be added to `SRCS` in the top-level `Makefile`.
- Image load and save goes through the Translation Kit (add `translation` to
  `LIBS`). Don't write PNG code by hand.
- User-visible strings go through the Locale Kit: `B_TRANSLATE()` with a
  `B_TRANSLATION_CONTEXT` per file (after the includes). New or changed
  strings need `make catkeys` on Haiku and entries in every
  `locales/*.catkeys`; the engine's error messages are translated in
  `DocumentIO.cpp` and must match `MacPaintFile.cpp` word for word.

## Style (Haiku coding guidelines)
- Tabs (4 wide), 80 columns. Member variables use the `fName` prefix, private
  methods `_Name()`, statics `sName`, constants `kName`.
- The return type goes on its own line in function definitions. Braces open
  on the same line, except for function bodies.
- Message constants are four-character codes (`'tpen'`).
- Match the existing files; `src/ui/CanvasView.cpp` is a good model.

## Reference source: licensing rule
`reference/` may hold Apple's MacPaint/QuickDraw source from the Computer
History Museum. Its license allows study only. **Read it to understand
behavior and algorithms, then write original code. Never translate it line by
line or paste its code or comments into this project.** Never commit anything
from `reference/`.
