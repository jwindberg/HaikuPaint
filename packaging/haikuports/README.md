# HaikuPorts recipe

`haiku-apps/haikupaint/haikupaint-2.0.0.recipe` builds HaikuPaint from the
`v2.0.0` GitHub release tarball. To submit it:

1. Fill in `CHECKSUM_SHA256` with the SHA-256 of
   `https://github.com/jwindberg/HaikuPaint/archive/refs/tags/v2.0.0.tar.gz`.
2. Copy the `haiku-apps/haikupaint` folder into a fork of
   https://github.com/haikuports/haikuports, test it with
   `haikuporter --build haikupaint`, and open a pull request.

HaikuPaint includes MacPaint's original artwork (patterns, icons, cursors),
which is © Apple. HaikuPorts reviewers may ask about that, since the
packages are redistributed; the code itself is MIT.
