#!/bin/sh
# Run on the Mac: copy the project to the Haiku machine and build it there.
# Lets Claude Code (running on the Mac) compile the real Be API code.
#
# Requires sshd running on Haiku and rsync installed there (pkgman install rsync).
#
#   HAIKU_HOST=user@192.168.7.65 scripts/haiku-build.sh          # build
#   HAIKU_HOST=user@192.168.7.65 scripts/haiku-build.sh clean    # any make target

set -e

HAIKU_HOST="${HAIKU_HOST:-user@192.168.7.65}"
HAIKU_DIR="${HAIKU_DIR:-HaikuPaint}"
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

rsync -az --delete \
	--exclude '.git/' \
	--exclude 'objects.*/' \
	--exclude 'reference/' \
	--exclude 'tests/test_canvas' \
	"$PROJECT_DIR/" "$HAIKU_HOST:$HAIKU_DIR/"

ssh "$HAIKU_HOST" "cd $HAIKU_DIR && make $*"
