"""Build man/figures/logo.svg (design A of the z3r logo canvas) with its
lettering converted to outlines, so the logo renders identically without the
Space Grotesk font installed.

Usage: python tools/make_logo.py SpaceGrotesk[wght].ttf man/figures/logo.svg

Needs fontTools. Space Grotesk is licensed under the SIL Open Font License.
"""
import sys

from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont

GROUND = "#12233F"
ACCENT = "#2BB3A3"
HIGHLIGHT = "#F2B33D"
EDGE = "#3B5478"


def text_path(font, text, size, weight_font_units_tracking, cx, baseline):
    """SVG path data for `text`, centered at cx on the given baseline."""
    glyph_set = font.getGlyphSet()
    cmap = font.getBestCmap()
    upm = font["head"].unitsPerEm
    scale = size / upm
    names = [cmap[ord(ch)] for ch in text]
    advances = [font["hmtx"][n][0] for n in names]
    tracking = weight_font_units_tracking
    total = sum(advances) + tracking * (len(names) - 1)
    x = cx / scale - total / 2
    paths = []
    for name, adv in zip(names, advances):
        pen = SVGPathPen(glyph_set)
        # Font units have y up; flip into SVG coordinates.
        tpen = TransformPen(pen, (scale, 0, 0, -scale, x * scale, baseline))
        glyph_set[name].draw(tpen)
        paths.append(pen.getCommands())
        x += adv + tracking
    return " ".join(paths)


def main(font_path, out_path):
    bold = instantiateVariableFont(TTFont(font_path), {"wght": 700})
    medium = instantiateVariableFont(TTFont(font_path), {"wght": 500})
    word = text_path(bold, "z3r", 132, -30, 260, 420)
    tagline = text_path(medium, "SAT · UNSAT · OPTIMIZE", 22, 270, 260, 470)
    svg = f"""<svg xmlns="http://www.w3.org/2000/svg" width="520" height="600" viewBox="0 0 520 600" role="img" aria-label="z3r logo">
  <title>z3r</title>
  <polygon points="260,8 512,154 512,446 260,592 8,446 8,154" fill="{GROUND}" stroke="{ACCENT}" stroke-width="14" stroke-linejoin="round"/>
  <polygon points="260,34 489,167 489,433 260,566 31,433 31,167" fill="none" stroke="#FFFFFF" stroke-opacity="0.08" stroke-width="2"/>
  <g stroke="{EDGE}" stroke-width="5" stroke-linecap="round" fill="none">
    <line x1="260" y1="118" x2="186" y2="196"/>
    <line x1="186" y1="196" x2="148" y2="270"/>
    <line x1="186" y1="196" x2="224" y2="270"/>
    <line x1="334" y1="196" x2="372" y2="270"/>
  </g>
  <g stroke="{ACCENT}" stroke-width="7" stroke-linecap="round" fill="none">
    <line x1="260" y1="118" x2="334" y2="196"/>
    <line x1="334" y1="196" x2="296" y2="270"/>
  </g>
  <g fill="{GROUND}" stroke="{EDGE}" stroke-width="5">
    <circle cx="186" cy="196" r="17"/>
    <circle cx="148" cy="270" r="15"/>
    <circle cx="224" cy="270" r="15"/>
    <circle cx="372" cy="270" r="15"/>
  </g>
  <g stroke="{EDGE}" stroke-width="4" stroke-linecap="round">
    <line x1="140" y1="262" x2="156" y2="278"/>
    <line x1="156" y1="262" x2="140" y2="278"/>
    <line x1="216" y1="262" x2="232" y2="278"/>
    <line x1="232" y1="262" x2="216" y2="278"/>
    <line x1="364" y1="262" x2="380" y2="278"/>
    <line x1="380" y1="262" x2="364" y2="278"/>
  </g>
  <circle cx="260" cy="118" r="19" fill="{ACCENT}"/>
  <circle cx="334" cy="196" r="17" fill="{ACCENT}"/>
  <circle cx="296" cy="270" r="22" fill="{HIGHLIGHT}"/>
  <polyline points="286,271 293,279 307,262" fill="none" stroke="{GROUND}" stroke-width="5" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="{word}" fill="#FFFFFF"/>
  <path d="{tagline}" fill="{ACCENT}"/>
</svg>
"""
    with open(out_path, "w") as fh:
        fh.write(svg)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
