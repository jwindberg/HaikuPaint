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
