# HaikuPaint

A MacPaint-style 1-bit paint program for Haiku, written in C++ against the
native Be API.

## Layout
```
src/engine/   Pure C++17 drawing engine. No Haiku headers. Unit tested anywhere.
src/ui/       Haiku app: BApplication, BWindow, CanvasView (Be API).
tests/        Engine tests; run with `make -C tests` (macOS, Linux or Haiku).
docs/         SPEC (what MacPaint did), ROADMAP (milestones), REFERENCES.
reference/    Put the CHM MacPaint/QuickDraw source here (git-ignored).
scripts/      Sync and build helpers between the Mac and Haiku.
```

## Building on Haiku
```sh
pkgman install rsync      # only needed for the sync scripts
make                      # uses the makefile-engine; output in objects.*-release/
./objects.*-release/HaikuPaint
```
`make` needs the development tools (`gcc`, `makefile_engine`). They're
included in a standard Haiku install. If they're missing, run
`pkgman install haiku_devel makefile_engine`.

## Installing
On Haiku, `scripts/make-package.sh` builds `HaikuPaint-<version>.hpkg` in the
project folder; install it with `pkgman install ./HaikuPaint-*.hpkg`.
HaikuPaint then appears in the Deskbar's Applications menu, and opens
MacPaint documents from Tracker.

## Translations
User-visible strings are wrapped in `B_TRANSLATE()`. After changing them,
run `make catkeys` on Haiku to update `locales/en.catkeys`, add the new
strings to each `locales/<language>.catkeys`, and `make bindcatalogs`.
HaikuPaint comes in English, German, French, Spanish and Swedish.

## Engine tests (anywhere)
```sh
make -C tests
```

## Workflow: Haiku laptop → SSH → Mac
The project lives on the Mac at `~/Projects/haiku/HaikuPaint`. Claude Code
runs there too.

1. **On Haiku**, open Terminal and run `ssh jwindberg@<mac-ip>`, then
   `cd ~/Projects/haiku/HaikuPaint` and `claude`.
   (On the Mac, turn on System Settings → General → Sharing → Remote Login.)
2. **To build and run the app**, use one of these:
   - **Pull from Haiku** (simplest, needs nothing extra on the Haiku side):
     in a second Haiku Terminal, run
     `MAC_HOST=jwindberg@<mac-ip> ~/HaikuPaint/scripts/pull-from-mac.sh run`.
     For the very first copy, run
     `rsync -az jwindberg@<mac-ip>:Projects/haiku/HaikuPaint/ ~/HaikuPaint/`.
   - **Push from the Mac** (lets Claude compile and see errors itself): turn
     on sshd on Haiku, then run `HAIKU_HOST=user@<haiku-ip> scripts/haiku-build.sh`
     on the Mac.
3. GUI apps can't be displayed over SSH, so always launch HaikuPaint on the
   Haiku desktop.

## License
HaikuPaint's code is under the MIT license; see `LICENSE`. MacPaint's
artwork, described below, is not.

## Credits
MacPaint was written by Bill Atkinson at Apple (1984). HaikuPaint's code is
new, but its patterns (the System file's `PAT# 0`), its tool icons and
cursors (MacPaint's PaintFont), and its application, document and About
box icons (MacPaint's `ICN#` and `ICON` resources) are MacPaint's original
artwork, © Apple. `tools/extract_macpaint_art.py` regenerates
`src/engine/Patterns.cpp`, `src/ui/PaintFont.cpp`, `src/ui/MacIcons.cpp`
and `icons.rdef` from a MacPaint 1.x floppy image. Behavior and layout
were studied in the MacPaint source code released by the Computer History
Museum, which isn't included here.
