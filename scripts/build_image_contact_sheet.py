import argparse
from pathlib import Path
import sys

try:
    from PIL import Image, ImageDraw
except ModuleNotFoundError as exc:
    print("Pillow is required to build image contact sheets.", file=sys.stderr)
    raise SystemExit(2) from exc


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build a labeled contact sheet from images.")
    parser.add_argument("--output", required=True, type=Path, help="Output PNG path")
    parser.add_argument(
        "--images", required=True, nargs="+", type=Path, help="Input image paths"
    )
    parser.add_argument(
        "--labels", nargs="*", default=None, help="Optional labels matching image order"
    )
    parser.add_argument("--columns", default=2, type=int, help="Grid columns")
    parser.add_argument(
        "--thumb-width", default=420, type=int, help="Thumbnail width for each cell"
    )
    return parser.parse_args()


def build_contact_sheet(
    image_paths: list[Path],
    labels: list[str],
    output_path: Path,
    columns: int,
    thumb_width: int,
) -> None:
    margin = 24
    caption_height = 44
    background = (248, 248, 248)
    caption_fill = (20, 20, 20)

    prepared: list[Image.Image] = []
    cell_height = 0

    for image_path, label in zip(image_paths, labels):
        image = Image.open(image_path).convert("RGB")
        aspect_ratio = image.height / image.width
        resized = image.resize(
            (thumb_width, int(thumb_width * aspect_ratio)),
            Image.Resampling.LANCZOS,
        )

        canvas = Image.new(
            "RGB", (thumb_width, resized.height + caption_height), color=(255, 255, 255)
        )
        canvas.paste(resized, (0, 0))
        draw = ImageDraw.Draw(canvas)
        draw.text((12, resized.height + 12), label, fill=caption_fill)
        prepared.append(canvas)
        cell_height = max(cell_height, canvas.height)

    rows = (len(prepared) + columns - 1) // columns
    sheet_width = margin + columns * (thumb_width + margin)
    sheet_height = margin + rows * (cell_height + margin)
    contact_sheet = Image.new("RGB", (sheet_width, sheet_height), color=background)

    for index, thumbnail in enumerate(prepared):
        row = index // columns
        column = index % columns
        x = margin + column * (thumb_width + margin)
        y = margin + row * (cell_height + margin)
        contact_sheet.paste(thumbnail, (x, y))

    output_path.parent.mkdir(parents=True, exist_ok=True)
    contact_sheet.save(output_path)


def main() -> int:
    args = parse_args()

    if args.labels is None or len(args.labels) == 0:
        labels = [path.stem for path in args.images]
    else:
        labels = args.labels

    if len(labels) != len(args.images):
        print("The number of labels must match the number of images.", file=sys.stderr)
        return 2

    build_contact_sheet(
        image_paths=args.images,
        labels=labels,
        output_path=args.output,
        columns=max(1, args.columns),
        thumb_width=max(160, args.thumb_width),
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
