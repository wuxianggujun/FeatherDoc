import argparse
import json
from pathlib import Path
from statistics import mean

from PIL import Image, ImageChops


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Summarize the visual pixel delta for table style quality look-only repair."
    )
    parser.add_argument("--before", required=True, type=Path, help="Before-rendered page PNG")
    parser.add_argument("--after", required=True, type=Path, help="After-rendered page PNG")
    parser.add_argument("--output-json", required=True, type=Path, help="Output JSON path")
    parser.add_argument("--output-text", required=True, type=Path, help="Output text path")
    return parser.parse_args()


def is_dark_blue(pixel: tuple[int, int, int]) -> bool:
    red, green, blue = pixel
    return red <= 70 and 45 <= green <= 120 and 80 <= blue <= 170 and blue > red and blue >= green


def iter_rgb_pixels(image: Image.Image):
    raw = image.tobytes()
    for index in range(0, len(raw), 3):
        yield raw[index], raw[index + 1], raw[index + 2]


def summarize(before_path: Path, after_path: Path) -> dict[str, object]:
    before = Image.open(before_path).convert("RGB")
    after = Image.open(after_path).convert("RGB")
    if before.size != after.size:
        raise ValueError(f"image size mismatch: before={before.size}, after={after.size}")

    diff = ImageChops.difference(before, after)
    diff_pixels = list(iter_rgb_pixels(diff))
    nonzero_diff_pixels = sum(1 for pixel in diff_pixels if pixel != (0, 0, 0))
    mean_diff = [mean(channel) for channel in zip(*diff_pixels)] if diff_pixels else [0.0, 0.0, 0.0]

    before_dark_blue_pixels = sum(1 for pixel in iter_rgb_pixels(before) if is_dark_blue(pixel))
    after_dark_blue_pixels = sum(1 for pixel in iter_rgb_pixels(after) if is_dark_blue(pixel))

    return {
        "size": list(before.size),
        "nonzero_diff_pixels": nonzero_diff_pixels,
        "mean_diff": mean_diff,
        "before_dark_blue_pixels": before_dark_blue_pixels,
        "after_dark_blue_pixels": after_dark_blue_pixels,
        "dark_blue_pixel_delta": after_dark_blue_pixels - before_dark_blue_pixels,
    }


def write_text_summary(summary: dict[str, object], output_path: Path) -> None:
    lines = [
        f"size=({summary['size'][0]}, {summary['size'][1]})",
        f"nonzero_diff_pixels={summary['nonzero_diff_pixels']}",
        f"mean_diff={summary['mean_diff']}",
        f"before_dark_blue_pixels={summary['before_dark_blue_pixels']}",
        f"after_dark_blue_pixels={summary['after_dark_blue_pixels']}",
        f"dark_blue_pixel_delta={summary['dark_blue_pixel_delta']}",
    ]
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    args = parse_args()
    summary = summarize(args.before, args.after)
    args.output_json.parent.mkdir(parents=True, exist_ok=True)
    args.output_json.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    write_text_summary(summary, args.output_text)

    if summary["after_dark_blue_pixels"] <= summary["before_dark_blue_pixels"]:
        raise SystemExit("after image should contain more dark-blue table-header pixels than before")
    if summary["nonzero_diff_pixels"] <= 0:
        raise SystemExit("before and after images did not differ")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
