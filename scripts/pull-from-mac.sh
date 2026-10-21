#!/bin/sh
# Run on Haiku: fetch the latest project files from the Mac, then build.
# Needs only ssh (built into Haiku) and rsync (pkgman install rsync).
#
#   MAC_HOST=jwindberg@<mac-ip> scripts/pull-from-mac.sh
#   MAC_HOST=jwindberg@<mac-ip> scripts/pull-from-mac.sh run   # build + launch

set -e

if [ -z "$MAC_HOST" ]; then
	echo "Set MAC_HOST, e.g. MAC_HOST=jwindberg@192.168.7.75 $0" >&2
	exit 1
fi

MAC_DIR="${MAC_DIR:-Projects/haiku/HaikuPaint}"
LOCAL_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# Deliberately no -t: copied files get the receiver's current time, so make
# still works when the two clocks disagree. --checksum skips unchanged files.
rsync -rlpz --checksum \
	--exclude '.git/' \
	--exclude 'objects.*/' \
	--exclude 'reference/' \
	--exclude 'tests/test_canvas' \
	--exclude 'package-stage/' \
	--exclude '*.hpkg' \
	"$MAC_HOST:$MAC_DIR/" "$LOCAL_DIR/"

cd "$LOCAL_DIR"
make

if [ "$1" = "run" ]; then
	./objects.*-release/HaikuPaint &
fi
