#!/usr/bin/env python3
"""Regenerates the app icons in app/resources/ (requires Pillow).

    python3 scripts/make_icons.py

The icon is a small glowing light, a glimmer, on a night-blue page with a few
faint ruled lines. Outputs:
  glimmer.png   512x512, window icon, Linux desktop/AppImage
  glimmer.ico   16-256 px, Windows executable icon
  glimmer.icns  macOS bundle icon
"""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

SIZE = 1024
OUT = Path(__file__).resolve().parent.parent / "app" / "resources"


def lerp(a, b, t):
    return tuple(round(x + (y - x) * t) for x, y in zip(a, b))


def build() -> Image.Image:
    # Background: rounded square with a vertical night gradient.
    top, bottom = (32, 30, 74), (14, 16, 40)
    gradient = Image.new("RGB", (SIZE, SIZE))
    draw = ImageDraw.Draw(gradient)
    for y in range(SIZE):
        draw.line([(0, y), (SIZE, y)], fill=lerp(top, bottom, y / SIZE))
    mask = Image.new("L", (SIZE, SIZE), 0)
    ImageDraw.Draw(mask).rounded_rectangle([40, 40, SIZE - 40, SIZE - 40], radius=220, fill=255)
    icon = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    icon.paste(gradient, (0, 0), mask)

    # Faint ruled lines, like a diary page.
    lines = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    lines_draw = ImageDraw.Draw(lines)
    for y in (620, 720, 820):
        lines_draw.rounded_rectangle([210, y, 814, y + 14], radius=7, fill=(170, 175, 230, 70))
    icon = Image.alpha_composite(icon, Image.composite(lines, Image.new("RGBA", lines.size), mask))

    # The glimmer: a wide soft glow, a tighter halo, then a bright core.
    cx, cy = 560, 400
    for radius, colour, blur in ((300, (255, 214, 102, 110), 110), (150, (255, 232, 150, 190), 45)):
        layer = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
        ImageDraw.Draw(layer).ellipse([cx - radius, cy - radius, cx + radius, cy + radius], fill=colour)
        layer = layer.filter(ImageFilter.GaussianBlur(blur))
        icon = Image.alpha_composite(icon, Image.composite(layer, Image.new("RGBA", layer.size), mask))
    core = ImageDraw.Draw(icon)
    core.ellipse([cx - 62, cy - 62, cx + 62, cy + 62], fill=(255, 250, 225, 255))

    # Two distant sparkles.
    for x, y, r in ((300, 250, 16), (760, 220, 11)):
        core.ellipse([x - r, y - r, x + r, y + r], fill=(255, 240, 190, 200))
    return icon


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    icon = build()
    icon.resize((512, 512), Image.LANCZOS).save(OUT / "glimmer.png", optimize=True)
    icon.save(OUT / "glimmer.ico", sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    icon.save(OUT / "glimmer.icns")
    print(f"Wrote icons to {OUT}")


if __name__ == "__main__":
    main()
