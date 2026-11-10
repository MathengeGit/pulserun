"""Export the game's 5x7 bitmap font (src/raster/font.cpp) to JSON for the website.

The site draws its headlines with the same glyph rows the C++ renderer uses, so the
page and the game share one typeface. Run: python3 web/export_font.py src/raster/font.cpp out.json
"""
import json
import re
import sys

src = open(sys.argv[1], encoding="utf-8").read()
glyphs = {}
for m in re.finditer(r"\{'(\\.|.)',\s*\{([^}]*)\}\}", src):
    ch = m.group(1)
    if ch.startswith("\\"):
        ch = ch[1:]
    rows = [int(v.strip().replace("0b", ""), 2) for v in m.group(2).split(",") if v.strip()]
    if len(rows) == 7:
        glyphs[ch] = rows
json.dump({"width": 5, "height": 7, "glyphs": glyphs}, open(sys.argv[2], "w"), separators=(",", ":"))
print(f"exported {len(glyphs)} glyphs")
