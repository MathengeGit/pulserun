"""Builds the Checkpoint 1 evidence PDF from the rendered screenshots.

Usage (needs Python 3 + Pillow):  python tools/make_evidence_pdf.py
Reads docs/screenshots/*.png and writes docs/PulseRun_Checkpoint1_Evidence.pdf
"""
import os
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHOTS = os.path.join(ROOT, "docs", "screenshots")
OUT = os.path.join(ROOT, "docs", "PulseRun_Checkpoint1_Evidence.pdf")

PAGE_W, PAGE_H = 1754, 1240  # A4 landscape at 150 dpi

MEMBERS = [
    "Wachira Njoroge", "Stacy Akinyi", "Okeyo Marie Hoppe Solini",
    "Mathenge Gitahi", "Diana Mucheke", "Kimani Anise Wambui",
]

PAGES = [
    ("01_lines.png", "Line algorithms",
     "DDA, Bresenham (all 8 octants), dashed lines and thick lines.",
     "Source: src/raster/line.cpp, demos/demo_lines.cpp",
     "Used in PulseRun for: ground line, scrolling grid ticks, saw spokes, UI frames, jump-trail segments."),
    ("02_circles.png", "Circles, ellipses and arcs",
     "Midpoint circle, midpoint ellipse (two regions), filled versions, arcs and thick arcs.",
     "Source: src/raster/circle.cpp, demos/demo_circles.cpp",
     "Used in PulseRun for: jump orbs, jump pads, difficulty faces, death shock-wave rings, rounded UI corners."),
    ("03_polygons.png", "Polygon fill and transforms",
     "Scanline fill (edge table + active edge table, even-odd), point-in-polygon, rotate/scale/translate.",
     "Source: src/raster/polygon.cpp, demos/demo_polygons.cpp",
     "Used in PulseRun for: cube, spikes, blocks, saws, skyline, hills; point-in-polygon will drive spike collision."),
    ("04_clipping.png", "Clipping",
     "Cohen-Sutherland line clipping and Sutherland-Hodgman polygon clipping against a rectangular window.",
     "Source: src/raster/clip.cpp, demos/demo_clipping.cpp",
     "Used in PulseRun for: ground tick marks clipped to the ground band, death shards clipped to the viewport."),
    ("05_curves.png", "Bezier curves and splines",
     "Cubic Bezier with the De Casteljau construction shown, Catmull-Rom spline, and the game application.",
     "Source: src/raster/curve.cpp, demos/demo_curves.cpp",
     "Used in PulseRun for: jump arc trail, parallax hills, portal edges, logo pulse line."),
    ("06_scene_gameplay.png", "Gameplay frame",
     "Cube mid-jump with trail, spikes, blocks, saw, orb, pad, portal, parallax city and hills, HUD.",
     "Source: demos/demo_scenes.cpp, src/game/sprites.cpp",
     "Every shape is drawn by the algorithms on the previous pages."),
    ("07_scene_menu.png", "Main menu",
     "Logo with ECG pulse line, play button, icon buttons, parallax background.",
     "Source: demos/demo_scenes.cpp, src/ui/ui.cpp",
     "Rounded rectangles are lines + arcs + midpoint-circle corner profiles; text uses our bitmap font."),
    ("08_scene_level_select.png", "Level select",
     "Four level cards: three unlocked with difficulty faces and best-progress bars, one locked.",
     "Source: demos/demo_scenes.cpp, src/ui/ui.cpp",
     "Matches the project scope: levels 1-3 committed, level 4 stretch goal."),
    ("09_scene_death.png", "Death frame",
     "Crash on a spike: shards flying out (clipped polygons), expanding shock-wave circles, retry prompt.",
     "Source: demos/demo_scenes.cpp, src/game/sprites.cpp",
     "Shards use polygon fill + Sutherland-Hodgman clipping; rings use the midpoint circle algorithm."),
]


def font(size, bold=False):
    names = ["DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf", "arialbd.ttf" if bold else "arial.ttf"]
    for base in ["/usr/share/fonts/truetype/dejavu/", "C:/Windows/Fonts/", ""]:
        for n in names:
            try:
                return ImageFont.truetype(base + n, size)
            except OSError:
                pass
    return ImageFont.load_default()


def wrap(draw, text, fnt, max_w):
    words, lines, cur = text.split(), [], ""
    for w in words:
        t = (cur + " " + w).strip()
        if draw.textlength(t, font=fnt) <= max_w:
            cur = t
        else:
            lines.append(cur)
            cur = w
    if cur:
        lines.append(cur)
    return lines


def cover():
    img = Image.new("RGB", (PAGE_W, PAGE_H), (12, 14, 34))
    d = ImageDraw.Draw(img)
    d.text((120, 200), "PulseRun", font=font(120, True), fill=(0, 238, 255))
    d.text((120, 350), "Computer Graphics Project - Checkpoint 1: Technical Foundation", font=font(44), fill=(235, 240, 255))
    d.text((120, 450), "Evidence of graphical output (pure C++ renderer)", font=font(36), fill=(160, 170, 215))
    d.text((120, 600), "Group members", font=font(38, True), fill=(255, 74, 200))
    for i, m in enumerate(MEMBERS):
        d.text((120, 670 + i * 56), m, font=font(34), fill=(235, 240, 255))
    d.text((120, 1100), "github.com/MathengeGit/pulserun", font=font(32), fill=(160, 170, 215))
    return img


def page(fname, title, l1, l2, l3):
    img = Image.new("RGB", (PAGE_W, PAGE_H), (255, 255, 255))
    d = ImageDraw.Draw(img)
    d.text((80, 50), title, font=font(54, True), fill=(20, 24, 60))
    shot = Image.open(os.path.join(SHOTS, fname)).convert("RGB")
    w = 1500
    h = int(shot.height * w / shot.width)
    shot = shot.resize((w, h), Image.LANCZOS)
    x = (PAGE_W - w) // 2
    d.rectangle([x - 4, 140 - 4, x + w + 4, 140 + h + 4], outline=(90, 98, 150), width=4)
    img.paste(shot, (x, 140))
    y = 140 + h + 40
    f = font(30)
    for text in (l1, l2, l3):
        for line in wrap(d, text, f, PAGE_W - 160):
            d.text((80, y), line, font=f, fill=(40, 44, 80))
            y += 42
        y += 6
    d.text((80, PAGE_H - 70), fname, font=font(24), fill=(130, 136, 170))
    return img


def main():
    pages = [cover()] + [page(*p) for p in PAGES]
    pages[0].save(OUT, "PDF", resolution=150.0, quality=95, save_all=True, append_images=pages[1:])
    print("Wrote", OUT, "(%d pages)" % len(pages))


if __name__ == "__main__":
    main()
