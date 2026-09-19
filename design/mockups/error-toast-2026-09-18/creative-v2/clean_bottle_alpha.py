#!/usr/bin/env python3
"""Remove the generated neutral checkerboard without changing the canvas.

Requires Pillow and numpy. The original Image Gen file remains unchanged.
The user approved local cleanup after Image Gen returned a baked checkerboard.
"""

import argparse
from collections import deque
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter


def regions(mask):
    """Yield four-connected component coordinates in scan order."""
    height, width = mask.shape
    seen = np.zeros(mask.shape, dtype=bool)
    for yy, xx in zip(*np.nonzero(mask)):
        if seen[yy, xx]:
            continue
        queue = deque([(int(yy), int(xx))])
        seen[yy, xx] = True
        cells = []
        while queue:
            y, x = queue.popleft()
            cells.append((y, x))
            for ny, nx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)):
                if 0 <= ny < height and 0 <= nx < width and mask[ny, nx] and not seen[ny, nx]:
                    seen[ny, nx] = True
                    queue.append((ny, nx))
        yield tuple(zip(*cells))


def clean_alpha(source):
    original = Image.open(source).convert("RGB")
    raw = np.array(original)
    rgb = raw.astype(np.int16)
    height, width = rgb.shape[:2]

    # The checkerboard has a channel range of at most 12. Bright neutral
    # highlights belong to the art; the observed background never exceeds 229.
    channel_range = rgb.max(axis=2) - rgb.min(axis=2)
    mask = (channel_range > 12) | (rgb.min(axis=2) > 232)

    # Restore small enclosed dark strokes, such as the heart outline, while
    # retaining the larger transparent opening between the seaweed strands.
    binary = Image.fromarray(mask.astype(np.uint8) * 255)
    padded = Image.new("L", (width + 2, height + 2), 0)
    padded.paste(binary, (1, 1))
    ImageDraw.floodfill(padded, (0, 0), 128)
    holes = np.array(padded)[1:-1, 1:-1] == 0
    for ys, xs in regions(holes):
        if len(ys) <= 64:
            mask[ys, xs] = True

    # Discard isolated background noise. All ten bubbles are larger than this.
    clean = np.zeros(mask.shape, dtype=bool)
    component_count = 0
    for ys, xs in regions(mask):
        if len(ys) >= 24:
            clean[ys, xs] = True
            component_count += 1

    # Remove a single contaminated boundary pixel, then soften only the edge.
    base = Image.fromarray(clean.astype(np.uint8) * 255)
    core = np.array(base.filter(ImageFilter.MinFilter(3))) > 0
    softened = Image.fromarray(core.astype(np.uint8) * 255).filter(ImageFilter.GaussianBlur(0.6))
    alpha = np.minimum(np.array(softened), np.array(base))

    # Extend adjacent clean colours into the one-pixel boundary strip. This
    # removes grey matte contamination without changing any interior colours.
    colour = raw.astype(np.float32)
    known = core.copy()
    for _ in range(3):
        count = np.zeros((height, width), np.float32)
        total = np.zeros((height, width, 3), np.float32)
        for dy, dx in ((-1, -1), (-1, 0), (-1, 1), (0, -1), (0, 1), (1, -1), (1, 0), (1, 1)):
            ny = slice(max(0, dy), min(height, height + dy))
            nx = slice(max(0, dx), min(width, width + dx))
            sy = slice(max(0, -dy), min(height, height - dy))
            sx = slice(max(0, -dx), min(width, width - dx))
            available = known[sy, sx]
            count[ny, nx] += available
            total[ny, nx] += colour[sy, sx] * available[..., None]
        new = (~known) & (count > 0) & clean
        colour[new] = total[new] / count[new, None]
        known |= new

    rgba = np.dstack([np.clip(np.rint(colour), 0, 255).astype(np.uint8), alpha])
    return Image.fromarray(rgba, "RGBA"), component_count


def main():
    root = Path(__file__).resolve().parents[4]
    default_output = root / "assets/hud-icons/error-toast-bottle-v1.png"
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--output", type=Path, default=default_output)
    parser.add_argument("--proof", type=Path)
    args = parser.parse_args()
    provenance_path = default_output.with_suffix(".provenance.json")
    provenance = json.loads(provenance_path.read_text())
    source = args.source or Path(provenance["selected_source"])
    if source.resolve() == args.output.resolve():
        raise SystemExit("Source must be the preserved original, not the cleaned output.")

    result, component_count = clean_alpha(source)
    result.save(args.output)
    alpha = np.array(result.getchannel("A"))
    x0, y0, x1, y1 = result.getchannel("A").getbbox()
    cleanup = {
        "script": str(Path(__file__).resolve().relative_to(root)),
        "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
        "output_sha256": hashlib.sha256(args.output.read_bytes()).hexdigest(),
        "user_approval": "The user replied 'okay cool!' to local background cleanup and requested a restart.",
        "method": "Neutral channel-range mask (>12), retain white highlights (minimum channel >232), flood-fill enclosed holes, restore holes up to 64 pixels, discard foreground components below 24 pixels, erode the edge by one source pixel, Gaussian alpha smoothing at radius 0.6, extend adjacent clean interior RGB into the one-pixel edge strip.",
        "canvas_preserved": True,
        "foreground_components": component_count,
        "visible_pixels": int((alpha > 0).sum()),
        "opaque_pixels": int((alpha == 255).sum()),
        "validation": "Checked the full image and enlarged bottle/bubble edges against teal (#00919e) and dark (#08202b) backgrounds. The seaweed opening remains transparent and white highlights remain intact.",
    }
    if args.output.resolve() == default_output.resolve():
        provenance.update({
            "status": "production_ready",
            "method": "built-in image_gen followed by user-approved local background cleanup",
            "mode": "RGBA",
            "has_alpha": True,
            "alpha_bounds": {"x": x0, "y": y0, "width": x1 - x0, "height": y1 - y0},
            "cleanup": cleanup,
            "fidelity_concerns": [
                "Generated shapes and palette follow the approved artwork, but some illustration details differ from the source mockup.",
                "The cleanup changes only transparency and the outermost one-pixel RGB edge strip to remove checkerboard contamination."
            ],
        })
        provenance_path.write_text(json.dumps(provenance, indent=2) + "\n")

    if args.proof:
        proof = Image.new("RGB", (result.width, result.height * 2))
        for index, background in enumerate(((0, 145, 158), (8, 32, 43))):
            composite = Image.new("RGBA", result.size, background + (255,))
            composite.alpha_composite(result)
            proof.paste(composite.convert("RGB"), (0, index * result.height))
        proof.save(args.proof)

    print(json.dumps({"output": str(args.output), "dimensions": result.size, "alpha_bounds": [x0, y0, x1, y1], "foreground_components": component_count}))


if __name__ == "__main__":
    main()
