"""Render the Cornell box with each camera model and tile the results into one figure.

Usage (from the repository root, after building the `render` target):

    python docs/figures/camera_models/make_camera_models.py --render build/render

Writes docs/figures/camera_models.png. Requires Pillow.
"""

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

HERE = Path(__file__).resolve().parent
OUT = HERE.parent / "camera_models.png"

CELL_W, CELL_H, LABEL_H = 400, 300, 34
PANELS = [
    ("pinhole", "Pinhole (perspective)"),
    ("thin_lens", "Thin lens (depth of field)"),
    ("orthographic", "Orthographic"),
    ("equirectangular", "Equirectangular (360°)"),
]


def font(size):
    for name in ("segoeui.ttf", "DejaVuSans.ttf", "arial.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


def render(exe, name, out_dir):
    out = out_dir / f"{name}.png"
    subprocess.run([str(exe), "-c", str(HERE / f"{name}.json"), "-o", str(out), "--no-progress"],
                   check=True, stdout=subprocess.DEVNULL)
    return Image.open(out).convert("RGB")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--render", default="build/render", help="path to the render executable")
    args = ap.parse_args()

    sheet = Image.new("RGB", (CELL_W * 2, (CELL_H + LABEL_H) * 2), (24, 24, 27))
    draw = ImageDraw.Draw(sheet)
    label_font = font(18)

    with tempfile.TemporaryDirectory() as tmp:
        for n, (name, label) in enumerate(PANELS):
            img = render(args.render, name, Path(tmp))
            if img.size != (CELL_W, CELL_H):
                # fit inside the cell (equirectangular is 2:1), keep the aspect ratio
                scale = min(CELL_W / img.width, CELL_H / img.height)
                img = img.resize((round(img.width * scale), round(img.height * scale)), Image.LANCZOS)
            x = (n % 2) * CELL_W
            y = (n // 2) * (CELL_H + LABEL_H)
            sheet.paste(img, (x + (CELL_W - img.width) // 2, y + CELL_H - img.height))
            draw.text((x + 12, y + CELL_H + 6), label, fill=(226, 232, 240), font=label_font)

    sheet.save(OUT)
    print("wrote", OUT)


if __name__ == "__main__":
    sys.exit(main())
