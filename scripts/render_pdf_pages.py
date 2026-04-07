import argparse
import json
from pathlib import Path
import sys

try:
    import fitz
except ModuleNotFoundError as exc:
    print(
        "PyMuPDF is required to render PDF pages. Install it in a local environment first.",
        file=sys.stderr,
    )
    raise SystemExit(2) from exc

from PIL import Image, ImageDraw


def render_pages(input_pdf: Path, output_dir: Path, dpi: int) -> list[Path]:
    scale = dpi / 72.0
    document = fitz.open(input_pdf)
    output_dir.mkdir(parents=True, exist_ok=True)

    page_paths: list[Path] = []
    for page_index, page in enumerate(document, start=1):
        pixmap = page.get_pixmap(matrix=fitz.Matrix(scale, scale), alpha=False)
        page_path = output_dir / f"page-{page_index:02d}.png"
        pixmap.save(page_path)
        page_paths.append(page_path)

    return page_paths


def build_contact_sheet(page_paths: list[Path], output_path: Path) -> None:
    if not page_paths:
        return

    margin = 24
    caption_height = 36
    thumbnail_width = 360

    thumbnails: list[Image.Image] = []
    thumbnail_height = 0
    for page_number, page_path in enumerate(page_paths, start=1):
        image = Image.open(page_path).convert("RGB")
        aspect_ratio = image.height / image.width
        resized = image.resize(
            (thumbnail_width, int(thumbnail_width * aspect_ratio)),
            Image.Resampling.LANCZOS,
        )

        canvas = Image.new(
            "RGB",
            (thumbnail_width, resized.height + caption_height),
            color=(255, 255, 255),
        )
        canvas.paste(resized, (0, 0))
        draw = ImageDraw.Draw(canvas)
        draw.text((12, resized.height + 10), f"Page {page_number}", fill=(0, 0, 0))
        thumbnails.append(canvas)
        thumbnail_height = max(thumbnail_height, canvas.height)

    contact_sheet = Image.new(
        "RGB",
        (
            margin + len(thumbnails) * (thumbnail_width + margin),
            thumbnail_height + margin * 2,
        ),
        color=(248, 248, 248),
    )

    current_x = margin
    for thumbnail in thumbnails:
        contact_sheet.paste(thumbnail, (current_x, margin))
        current_x += thumbnail_width + margin

    output_path.parent.mkdir(parents=True, exist_ok=True)
    contact_sheet.save(output_path)


def write_summary(
    input_pdf: Path, page_paths: list[Path], contact_sheet_path: Path, output_path: Path
) -> None:
    payload = {
        "input_pdf": str(input_pdf.resolve()),
        "page_count": len(page_paths),
        "pages": [str(path.resolve()) for path in page_paths],
        "contact_sheet": str(contact_sheet_path.resolve()),
    }
    output_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Render a PDF to PNG pages.")
    parser.add_argument("--input", required=True, type=Path, help="Input PDF path")
    parser.add_argument(
        "--output-dir", required=True, type=Path, help="Directory for page PNG output"
    )
    parser.add_argument(
        "--summary", required=True, type=Path, help="Path to the summary JSON output"
    )
    parser.add_argument(
        "--contact-sheet",
        required=True,
        type=Path,
        help="Path to the combined preview PNG",
    )
    parser.add_argument("--dpi", default=144, type=int, help="Render DPI")
    args = parser.parse_args()

    page_paths = render_pages(args.input, args.output_dir, args.dpi)
    build_contact_sheet(page_paths, args.contact_sheet)
    write_summary(args.input, page_paths, args.contact_sheet, args.summary)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
