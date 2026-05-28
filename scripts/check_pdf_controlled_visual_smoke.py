#!/usr/bin/env python3
"""Validate already-rendered controlled PDF visual smoke artifacts.

This checker is intentionally read-only: it does not render PDFs, invoke CTest,
create virtual environments, install dependencies, or run a build. It validates
the PNG and text evidence that was already produced by a controlled smoke run.
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
import zlib
from pathlib import Path


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def resolve_path(value: str, base: Path) -> Path:
    path = Path(value)
    if not path.is_absolute():
        path = base / path
    return path


def paeth_predictor(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def read_png_metrics(path: Path) -> dict:
    data = path.read_bytes()
    if not data.startswith(PNG_SIGNATURE):
        raise ValueError(f"Not a PNG file: {path}")

    offset = len(PNG_SIGNATURE)
    width = height = bit_depth = color_type = interlace = None
    idat = bytearray()

    while offset + 8 <= len(data):
        length = struct.unpack(">I", data[offset : offset + 4])[0]
        chunk_type = data[offset + 4 : offset + 8]
        chunk_data = data[offset + 8 : offset + 8 + length]
        offset += 12 + length

        if chunk_type == b"IHDR":
            (
                width,
                height,
                bit_depth,
                color_type,
                _compression,
                _filter,
                interlace,
            ) = struct.unpack(">IIBBBBB", chunk_data)
        elif chunk_type == b"IDAT":
            idat.extend(chunk_data)
        elif chunk_type == b"IEND":
            break

    if width is None or height is None:
        raise ValueError(f"PNG is missing IHDR: {path}")
    if bit_depth != 8 or interlace != 0:
        raise ValueError(
            f"Unsupported PNG format for smoke validation: bit_depth={bit_depth} "
            f"interlace={interlace} path={path}"
        )

    bytes_per_pixel_by_color_type = {
        0: 1,  # grayscale
        2: 3,  # RGB
        4: 2,  # grayscale + alpha
        6: 4,  # RGBA
    }
    if color_type not in bytes_per_pixel_by_color_type:
        raise ValueError(f"Unsupported PNG color type {color_type}: {path}")

    bpp = bytes_per_pixel_by_color_type[color_type]
    stride = width * bpp
    raw = zlib.decompress(bytes(idat))
    expected_length = (stride + 1) * height
    if len(raw) != expected_length:
        raise ValueError(
            f"Unexpected PNG data size for {path}: {len(raw)} != {expected_length}"
        )

    previous = bytearray(stride)
    cursor = 0
    nonwhite = 0
    dark = 0
    total_pixels = width * height

    for _row_index in range(height):
        filter_type = raw[cursor]
        cursor += 1
        current = bytearray(raw[cursor : cursor + stride])
        cursor += stride

        if filter_type == 1:
            for i in range(bpp, stride):
                current[i] = (current[i] + current[i - bpp]) & 0xFF
        elif filter_type == 2:
            for i in range(stride):
                current[i] = (current[i] + previous[i]) & 0xFF
        elif filter_type == 3:
            for i in range(stride):
                left = current[i - bpp] if i >= bpp else 0
                up = previous[i]
                current[i] = (current[i] + ((left + up) >> 1)) & 0xFF
        elif filter_type == 4:
            for i in range(stride):
                left = current[i - bpp] if i >= bpp else 0
                up = previous[i]
                upper_left = previous[i - bpp] if i >= bpp else 0
                current[i] = (
                    current[i] + paeth_predictor(left, up, upper_left)
                ) & 0xFF
        elif filter_type != 0:
            raise ValueError(f"Unsupported PNG filter {filter_type}: {path}")

        if color_type == 0:
            for value in current:
                if value <= 248:
                    nonwhite += 1
                if value < 80:
                    dark += 1
        elif color_type == 2:
            for i in range(0, stride, 3):
                r, g, b = current[i], current[i + 1], current[i + 2]
                if r <= 248 or g <= 248 or b <= 248:
                    nonwhite += 1
                if r < 80 and g < 80 and b < 80:
                    dark += 1
        elif color_type == 4:
            for i in range(0, stride, 2):
                value, alpha = current[i], current[i + 1]
                if alpha != 0 and value <= 248:
                    nonwhite += 1
                if alpha != 0 and value < 80:
                    dark += 1
        elif color_type == 6:
            for i in range(0, stride, 4):
                r, g, b, alpha = current[i], current[i + 1], current[i + 2], current[i + 3]
                if alpha != 0 and (r <= 248 or g <= 248 or b <= 248):
                    nonwhite += 1
                if alpha != 0 and r < 80 and g < 80 and b < 80:
                    dark += 1

        previous = current

    return {
        "path": str(path),
        "width": width,
        "height": height,
        "color_type": color_type,
        "nonwhite_pixel_count": nonwhite,
        "dark_pixel_count": dark,
        "nonwhite_ratio": round(nonwhite / total_pixels, 8),
        "dark_ratio": round(dark / total_pixels, 8),
        "bytes": path.stat().st_size,
    }


def add_check(checks: list[dict], name: str, passed: bool, message: str, **details) -> None:
    entry = {
        "name": name,
        "status": "pass" if passed else "fail",
        "message": message,
    }
    if details:
        entry["details"] = details
    checks.append(entry)


def validate_case(
    root: Path,
    case_name: str,
    expected_text: list[str],
    min_width: int,
    min_height: int,
    min_nonwhite_ratio: float,
) -> dict:
    case_dir = root / case_name
    checks: list[dict] = []
    png_metrics: list[dict] = []

    summary_path = case_dir / "summary.json"
    text_summary_path = case_dir / "text-summary.json"
    text_path = case_dir / "text.txt"

    if not summary_path.is_file():
        add_check(checks, "summary_json_exists", False, "summary.json is missing.")
        return {"case": case_name, "passed": False, "checks": checks, "png_metrics": []}

    summary = json.loads(summary_path.read_text(encoding="utf-8"))
    add_check(checks, "summary_json_exists", True, "summary.json exists.", path=str(summary_path))

    input_pdf = resolve_path(str(summary.get("input_pdf", "")), case_dir)
    add_check(
        checks,
        "input_pdf_exists",
        input_pdf.is_file(),
        "Input PDF referenced by summary exists.",
        path=str(input_pdf),
    )

    page_count = int(summary.get("page_count", 0) or 0)
    pages = [resolve_path(str(value), case_dir) for value in summary.get("pages", [])]
    add_check(
        checks,
        "page_count_matches_pages",
        page_count > 0 and page_count == len(pages),
        "summary page_count matches rendered page PNG count.",
        page_count=page_count,
        page_path_count=len(pages),
    )

    contact_sheet = resolve_path(str(summary.get("contact_sheet", "")), case_dir)
    png_paths = pages + [contact_sheet]
    for png_path in png_paths:
        if not png_path.is_file():
            add_check(
                checks,
                "png_exists",
                False,
                "Referenced PNG is missing.",
                path=str(png_path),
            )
            continue

        metrics = read_png_metrics(png_path)
        png_metrics.append(metrics)
        add_check(
            checks,
            "png_decoded",
            True,
            "PNG decoded successfully.",
            path=str(png_path),
            width=metrics["width"],
            height=metrics["height"],
        )
        add_check(
            checks,
            "png_min_dimensions",
            metrics["width"] >= min_width and metrics["height"] >= min_height,
            "PNG dimensions satisfy the controlled smoke threshold.",
            path=str(png_path),
            width=metrics["width"],
            height=metrics["height"],
            min_width=min_width,
            min_height=min_height,
        )
        add_check(
            checks,
            "png_nonblank",
            metrics["nonwhite_ratio"] >= min_nonwhite_ratio,
            "PNG has enough non-white pixels to prove it is not blank.",
            path=str(png_path),
            nonwhite_ratio=metrics["nonwhite_ratio"],
            min_nonwhite_ratio=min_nonwhite_ratio,
        )

    add_check(
        checks,
        "text_summary_exists",
        text_summary_path.is_file(),
        "text-summary.json exists.",
        path=str(text_summary_path),
    )
    add_check(
        checks,
        "text_output_exists",
        text_path.is_file(),
        "text.txt exists.",
        path=str(text_path),
    )

    text = text_path.read_text(encoding="utf-8") if text_path.is_file() else ""
    missing_text = [value for value in expected_text if value not in text]
    add_check(
        checks,
        "expected_text_present",
        not missing_text,
        "Expected text-layer snippets are present.",
        missing_text=missing_text,
        expected_text=expected_text,
    )

    text_summary_page_count = None
    if text_summary_path.is_file():
        text_summary = json.loads(text_summary_path.read_text(encoding="utf-8"))
        text_summary_page_count = int(text_summary.get("page_count", 0) or 0)
    add_check(
        checks,
        "text_page_count_matches",
        text_summary_page_count == page_count,
        "text-summary page_count matches render summary page_count.",
        page_count=page_count,
        text_summary_page_count=text_summary_page_count,
    )

    passed = all(check["status"] == "pass" for check in checks)
    return {
        "case": case_name,
        "passed": passed,
        "checks": checks,
        "png_metrics": png_metrics,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Validate existing controlled PDF visual smoke artifacts."
    )
    parser.add_argument("--root", required=True, type=Path)
    parser.add_argument("--case", action="append", dest="cases", default=[])
    parser.add_argument("--expect-text", action="append", dest="expected_text", default=[])
    parser.add_argument("--output", type=Path)
    parser.add_argument("--min-width", type=int, default=100)
    parser.add_argument("--min-height", type=int, default=100)
    parser.add_argument("--min-nonwhite-ratio", type=float, default=0.0001)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = args.root.resolve()
    checks: list[dict] = []

    add_check(
        checks,
        "root_exists",
        root.is_dir(),
        "Controlled PDF visual smoke root exists.",
        path=str(root),
    )
    if not root.is_dir():
        summary = {
            "schema": "featherdoc.pdf_controlled_visual_smoke_check.v1",
            "passed": False,
            "root": str(root),
            "checks": checks,
            "cases": [],
        }
        write_summary(args.output, summary)
        return 1

    cases = args.cases or sorted(
        child.name for child in root.iterdir() if child.is_dir() and (child / "summary.json").is_file()
    )
    add_check(
        checks,
        "case_count_nonzero",
        bool(cases),
        "At least one controlled smoke case was selected.",
        case_count=len(cases),
        cases=cases,
    )

    case_results = [
        validate_case(
            root=root,
            case_name=case_name,
            expected_text=args.expected_text,
            min_width=args.min_width,
            min_height=args.min_height,
            min_nonwhite_ratio=args.min_nonwhite_ratio,
        )
        for case_name in cases
    ]

    passed = all(check["status"] == "pass" for check in checks) and all(
        result["passed"] for result in case_results
    )
    summary = {
        "schema": "featherdoc.pdf_controlled_visual_smoke_check.v1",
        "passed": passed,
        "status": "pass" if passed else "fail",
        "root": str(root),
        "case_count": len(case_results),
        "checks": checks,
        "cases": case_results,
    }
    write_summary(args.output, summary)
    if args.output:
        print(f"PDF controlled visual smoke summary: {args.output}")
    print(f"PDF controlled visual smoke status: {summary['status']}")
    return 0 if passed else 1


def write_summary(output: Path | None, summary: dict) -> None:
    if output is None:
        print(json.dumps(summary, ensure_ascii=False, indent=2))
        return
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    raise SystemExit(main())
