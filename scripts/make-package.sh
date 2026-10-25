#!/bin/sh
# Run on Haiku: builds HaikuPaint and packages it as an .hpkg in the
# project folder. Install it with: pkgman install ./HaikuPaint-*.hpkg

set -e

cd "$(dirname "$0")/.."
make
make bindcatalogs

VERSION=2.0.1
REVISION=1
ARCH=$(getarch)
STAGE=package-stage
PACKAGE="HaikuPaint-$VERSION-$REVISION-$ARCH.hpkg"

rm -rf "$STAGE" "$PACKAGE"
mkdir -p "$STAGE/apps" "$STAGE/data/deskbar/menu/Applications" \
	"$STAGE/documentation/packages/haikupaint"
cp objects.*-release/HaikuPaint "$STAGE/apps/"
ln -s ../../../../apps/HaikuPaint \
	"$STAGE/data/deskbar/menu/Applications/HaikuPaint"
cp LICENSE README.md "$STAGE/documentation/packages/haikupaint/"

cat > "$STAGE/.PackageInfo" <<EOF
name			haikupaint
version			$VERSION-$REVISION
architecture	$ARCH
summary			"A re-creation of MacPaint for Haiku"
description		"HaikuPaint is the 1984 MacPaint, rebuilt as a native Haiku \
application: every tool, MacPaint's original patterns, icons and cursors, \
FatBits, the Goodies, and real MacPaint documents."
packager		"jwindberg <jwindberg@gmail.com>"
vendor			"jwindberg"
licenses		{ "MIT" }
copyrights		{
	"2026 jwindberg"
	"1983-1984 Apple Computer, Inc. (MacPaint artwork)"
}
provides		{
	haikupaint = $VERSION
	app:HaikuPaint = $VERSION
}
requires		{
	haiku
}
urls			{
	"https://github.com/jwindberg/HaikuPaint"
}
EOF

package create -C "$STAGE" "$PACKAGE"
rm -rf "$STAGE"
echo "Created $PACKAGE"
