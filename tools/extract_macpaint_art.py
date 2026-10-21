#!/usr/bin/env python3
"""Extract MacPaint's original artwork from a 400K MFS floppy image.

Reads, without running anything from the disk:
  - System file, PAT# 0: the standard 38 patterns MacPaint 1.x loads
    (MacPaint.p: patListID = 0 { use system patlist }).
  - MacPaint, FONT 31884 (= font 249 at 12 pt, "PaintFont"): the tool
    icons, cursors, check mark and brush shapes, drawn as characters.

Writes, relative to the project root:
  src/engine/Patterns.cpp
  src/ui/PaintFont.cpp

Usage: python3 -I tools/extract_macpaint_art.py <disk.dsk> [project-root]
"""
import os
import struct
import sys

PAINT_FONT_ID = 249 * 128 + 12
PATTERN_LIST_ID = 0


# --- MFS volume ----------------------------------------------------------

class MFSVolume:
    def __init__(self, data):
        self.data = data
        mdb = data[1024:1024 + 512]
        (sig, _cr, _bk, _atrb, _nfiles, self.dir_start, self.dir_len,
         self.num_blocks, self.block_size, _clp, self.alloc_start,
         _nxt, _free) = struct.unpack('>HIIHHHHHIIHIH', mdb[:36])
        if sig != 0xD2D7:
            raise ValueError('not an MFS volume')
        # 12-bit allocation map entries for blocks 2.., which can run past
        # the 512-byte MDB sector.
        raw = data[1024 + 64:1024 + 64 + (self.num_blocks * 12 + 15) // 8 + 2]
        self.block_map = {}
        for i in range(self.num_blocks):
            bit = i * 12
            word = (raw[bit // 8] << 8) | raw[bit // 8 + 1]
            self.block_map[i + 2] = (word >> 4) if bit % 8 == 0 \
                else (word & 0xFFF)
        self.files = list(self._directory())

    def _directory(self):
        for block in range(self.dir_start, self.dir_start + self.dir_len):
            buf = self.data[block * 512:(block + 1) * 512]
            pos = 0
            while pos + 51 <= 512 and buf[pos] & 0x80:
                _num, dstart, dlen, _dpy, rstart, rlen = struct.unpack(
                    '>IHIIHI', buf[pos + 18:pos + 38])
                name = buf[pos + 51:pos + 51 + buf[pos + 50]]
                yield {'name': name.decode('mac_roman'),
                       'data': (dstart, dlen), 'rsrc': (rstart, rlen)}
                pos += 51 + buf[pos + 50]
                pos += pos & 1

    def resource_fork(self, name):
        f = next(f for f in self.files if f['name'] == name)
        block, length = f['rsrc']
        out = bytearray()
        while block not in (0, 1) and len(out) < length:
            off = self.alloc_start * 512 + (block - 2) * self.block_size
            out += self.data[off:off + self.block_size]
            block = self.block_map[block]
        return bytes(out[:length])


def resource(fork, rtype, rid):
    data_off, map_off = struct.unpack('>II', fork[:8])
    rmap = fork[map_off:]
    type_list = rmap[struct.unpack('>H', rmap[24:26])[0]:]
    for t in range(struct.unpack('>h', type_list[:2])[0] + 1):
        entry = type_list[2 + t * 8:10 + t * 8]
        if entry[:4].decode('mac_roman') != rtype:
            continue
        count, ref_off = struct.unpack('>hH', entry[4:8])
        for r in range(count + 1):
            ref = type_list[ref_off + r * 12:ref_off + r * 12 + 12]
            if struct.unpack('>h', ref[:2])[0] == rid:
                start = data_off + int.from_bytes(ref[5:8], 'big')
                length = struct.unpack('>I', fork[start:start + 4])[0]
                return fork[start + 4:start + 4 + length]
    raise KeyError(f'{rtype} {rid}')


# --- Resource decoding ---------------------------------------------------

def decode_patterns(data):
    count = struct.unpack('>H', data[:2])[0]
    return [data[2 + i * 8:10 + i * 8] for i in range(count)]


def decode_font(data):
    (_type, first, last, _widmax, kernmax, _ndesc, _frw, height, owtloc,
     _ascent, _descent, _leading, rowwords) = struct.unpack('>13h', data[:26])
    rowbytes = rowwords * 2
    strike = data[26:26 + rowbytes * height]
    count = last - first + 3
    loc_at = 26 + rowbytes * height
    loc = struct.unpack(f'>{count}H', data[loc_at:loc_at + count * 2])
    # owTLoc is in words from its own field (byte 16). PaintFont's table
    # stops one entry short of the usual sentinel, so read what is there.
    ow_at = 16 + owtloc * 2
    ow_count = min(count, (len(data) - ow_at) // 2)
    ow = struct.unpack(f'>{ow_count}H', data[ow_at:ow_at + ow_count * 2])

    glyphs = []
    for code in range(first, last + 1):
        i = code - first
        if ow[i] == 0xFFFF:
            continue
        rows = []
        for y in range(height):
            row = strike[y * rowbytes:(y + 1) * rowbytes]
            rows.append(''.join(
                '#' if (row[x // 8] >> (7 - x % 8)) & 1 else '.'
                for x in range(loc[i], loc[i + 1])))
        glyphs.append({'code': code, 'rows': rows,
                       'offset': (ow[i] >> 8) + kernmax,
                       'advance': ow[i] & 0xFF})
    return height, glyphs


# --- Output --------------------------------------------------------------

HEADER = """\
// Generated by tools/extract_macpaint_art.py from
// {source}.
// Do not edit; re-run the script instead.
"""


def write_patterns(path, patterns):
    lines = [HEADER.format(source="the System file's PAT# 0, the pattern "
                           'list MacPaint 1.x uses'),
             '#include "Patterns.h"', '', '', 'namespace hp {', '', '',
             '// Rows top to bottom; MSB is the leftmost pixel. MacPaint '
             'shows 0-18 in its',
             '// top row and 19-37 in its bottom row.',
             'const Pattern kPatterns[kPatternCount] = {']
    for i, p in enumerate(patterns):
        hexes = ', '.join(f'0x{b:02x}' for b in p)
        lines.append(f'\t{{{{{hexes}}}}},\t// {i}')
    lines += ['};', '', '', '}\t// namespace hp', '']
    with open(path, 'w') as f:
        f.write('\n'.join(lines))


def write_font(path, height, glyphs):
    lines = [HEADER.format(source="MacPaint's PaintFont (FONT 249, 12 pt)"),
             '#include "PaintFont.h"', '', '#include <cstddef>', '', '']
    for g in glyphs:
        lines.append(f"static const char* const kGlyph{g['code']}[] = {{")
        lines += [f'\t"{row}",' for row in g['rows']]
        lines += ['};', '']
    lines += ['', 'static const Glyph kGlyphs[] = {']
    for g in glyphs:
        width = len(g['rows'][0])
        lines.append(f"\t{{{g['code']}, {width}, {g['offset']}, "
                     f"{g['advance']}, kGlyph{g['code']}}},")
    lines += ['};', '', '',
              'const Glyph*',
              'PaintFontGlyph(int code)',
              '{',
              '\tfor (const Glyph& glyph : kGlyphs) {',
              '\t\tif (glyph.code == code)',
              '\t\t\treturn &glyph;',
              '\t}',
              '\treturn NULL;',
              '}', '']
    assert height == 16, 'PaintFont.h assumes kPaintFontHeight == 16'
    with open(path, 'w') as f:
        f.write('\n'.join(lines))


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    root = sys.argv[2] if len(sys.argv) > 2 else \
        os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    with open(sys.argv[1], 'rb') as f:
        volume = MFSVolume(f.read())

    patterns = decode_patterns(
        resource(volume.resource_fork('System'), 'PAT#', PATTERN_LIST_ID))
    if len(patterns) != 38:
        sys.exit(f'expected 38 patterns, found {len(patterns)}')
    height, glyphs = decode_font(
        resource(volume.resource_fork('MacPaint'), 'FONT', PAINT_FONT_ID))

    write_patterns(os.path.join(root, 'src/engine/Patterns.cpp'), patterns)
    write_font(os.path.join(root, 'src/ui/PaintFont.cpp'), height, glyphs)
    print(f'Wrote {len(patterns)} patterns and {len(glyphs)} glyphs.')


if __name__ == '__main__':
    main()
