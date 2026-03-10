#!/usr/bin/env python3

from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "ships.png"
OUT_DIR = ROOT / "assets" / "generated"
OUT_PATH = OUT_DIR / "ships.labels.png"

TILE = 8
SCALE = 12


def cell_label(x: int, y: int) -> str:
    if 0 <= x <= 2 and 0 <= y <= 4:
        row = y
        part = ["left", "center", "right"][x]
        return f"player_{row}_{part}"

    if 4 <= x <= 9 and 0 <= y <= 5:
        return f"enemy_small_r{y}_c{x - 4}"

    if 4 <= x <= 9 and 6 <= y <= 9:
        block_x = (x - 4) // 2
        block_y = (y - 6) // 2
        sub_x = (x - 4) % 2
        sub_y = (y - 6) % 2
        corner = {
            (0, 0): "tl",
            (1, 0): "tr",
            (0, 1): "bl",
            (1, 1): "br",
        }[(sub_x, sub_y)]
        return f"enemy_big_r{block_y}_c{block_x}_{corner}"

    return f"tile_{x}_{y}"


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    image = Image.open(SOURCE).convert("RGBA")
    width, height = image.size
    scaled = image.resize((width * SCALE, height * SCALE), Image.Resampling.NEAREST)
    draw = ImageDraw.Draw(scaled)
    font = ImageFont.load_default()

    for tile_y in range(height // TILE):
        for tile_x in range(width // TILE):
            px = tile_x * TILE * SCALE
            py = tile_y * TILE * SCALE
            draw.rectangle(
                [px, py, px + TILE * SCALE, py + TILE * SCALE],
                outline=(255, 255, 255, 120),
                width=1,
            )

            label = f"{tile_x},{tile_y}\n{cell_label(tile_x, tile_y)}"
            draw.multiline_text(
                (px + 2, py + 2),
                label,
                fill=(255, 255, 255, 255),
                font=font,
                spacing=1,
                stroke_width=1,
                stroke_fill=(0, 0, 0, 255),
            )

    scaled.save(OUT_PATH)
    print(OUT_PATH)


if __name__ == "__main__":
    main()
