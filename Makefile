## Haiku build, using the standard makefile-engine. Run `make` on Haiku.
## The binary lands in objects.<arch>-<compiler>-release/HaikuPaint.
## (On macOS/Linux, use `make -C tests` to test the engine instead.)

NAME = HaikuPaint
TYPE = APP
APP_MIME_SIG = application/x-vnd.HaikuPaint

SRCS = \
	src/engine/Canvas.cpp \
	src/engine/Patterns.cpp \
	src/ui/App.cpp \
	src/ui/BitmapUtils.cpp \
	src/ui/CanvasView.cpp \
	src/ui/LineWidthPalette.cpp \
	src/ui/MainWindow.cpp \
	src/ui/PatternPalette.cpp \
	src/ui/ToolPalette.cpp

RDEFS = HaikuPaint.rdef

LIBS = be $(STDCPPLIBS)

LOCAL_INCLUDE_PATHS = src/engine src/ui

OPTIMIZE := FULL
WARNINGS = ALL
SYMBOLS :=
DEBUGGER :=

## C++17 is gcc 13's default; COMPILER_FLAGS would also reach the C
## preprocessor that handles the .rdef, which warns about -std=c++17.
COMPILER_FLAGS =

## Include the makefile-engine
DEVEL_DIRECTORY := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine
