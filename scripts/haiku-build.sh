#!/bin/sh
# Run on the Mac: copy the project to the Haiku machine and build it there.
# Lets Claude Code (running on the Mac) compile the real Be API code.
#
# Requires sshd running on Haiku, rsync installed there (pkgman install rsync)
# and key-based ssh login. Haiku's only account is "user".
#
#   HAIKU_HOST=user@<haiku-ip> scripts/haiku-build.sh          # build
#   HAIKU_HOST=user@<haiku-ip> scripts/haiku-build.sh clean    # any make target

set -e

if [ -z "$HAIKU_HOST" ]; then
	echo "Set HAIKU_HOST, e.g. HAIKU_HOST=user@192.168.7.109 $0" >&2
	exit 1
fi

HAIKU_DIR="${HAIKU_DIR:-HaikuPaint}"
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# Deliberately no -t: copied files get the receiver's current time, so make
# still works when the two clocks disagree. --checksum skips unchanged files.
rsync -rlpz --checksum --delete \
	--exclude '.git/' \
	--exclude 'objects.*/' \
	--exclude 'reference/' \
	--exclude 'tests/test_canvas' \
	--exclude 'package-stage/' \
	--exclude '*.hpkg' \
	"$PROJECT_DIR/" "$HAIKU_HOST:$HAIKU_DIR/"

ssh "$HAIKU_HOST" "cd $HAIKU_DIR && make $*"
