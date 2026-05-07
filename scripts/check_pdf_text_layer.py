import argparse
import json
from pathlib import Path
import sys

try:
    import fitz
except ModuleNotFoundError as exc:
    print(
        "PyMuPDF is required to inspect PDF text layers. Install it in a local environment first.",
        file=sys.stderr,
    )
    raise SystemExit(2) from exc


def collapse_whitespace(text: str) -> str:
    return " ".join(text.split())


def remove_whitespace(text: str) -> str:
    return "".join(text.split())


def extract_pdf_text(input_pdf: Path) -> tuple[str, list[str]]:
    document = fitz.open(input_pdf)
    page_texts: list[str] = []
    page_blocks: list[str] = []

    for page_index, page in enumerate(document, start=1):
        page_text = page.get_text("text")
        page_texts.append(page_text)
        page_blocks.append(f"=== Page {page_index:02d} ===\n{page_text.rstrip()}")

    return "\n".join(page_texts), page_blocks


def find_matches(expected_texts: list[str], combined_text: str) -> tuple[list[str], list[str]]:
    collapsed_text = collapse_whitespace(combined_text)
    stripped_text = remove_whitespace(combined_text)
    search_haystacks = (combined_text, collapsed_text, stripped_text)

    matched: list[str] = []
    missing: list[str] = []

    # 同时保留原文、折叠空白和去空白三种形态，尽量稳住 PDF
    # 断行或排版带来的文本层差异。
    for expected in expected_texts:
        expected_variants = (
            expected,
            collapse_whitespace(expected),
            remove_whitespace(expected),
        )
        if any(
            variant in haystack
            for variant in expected_variants
            for haystack in search_haystacks
        ):
            matched.append(expected)
        else:
            missing.append(expected)

    return matched, missing


def main() -> int:
    parser = argparse.ArgumentParser(description="Check whether a PDF text layer is searchable.")
    parser.add_argument("--input", required=True, type=Path, help="Input PDF path")
    parser.add_argument(
        "--expect-text",
        action="append",
        default=[],
        help="Expected text snippet. May be passed multiple times.",
    )
    parser.add_argument(
        "--output-text",
        type=Path,
        help="Optional path for the extracted text preview.",
    )
    parser.add_argument(
        "--summary",
        type=Path,
        help="Optional path for the JSON summary output.",
    )
    args = parser.parse_args()

    combined_text, page_blocks = extract_pdf_text(args.input)
    matched_text, missing_text = find_matches(args.expect_text, combined_text)

    if args.output_text:
        args.output_text.parent.mkdir(parents=True, exist_ok=True)
        args.output_text.write_text(
            "\n\n".join(page_blocks).rstrip() + "\n",
            encoding="utf-8",
        )

    summary = {
        "input_pdf": str(args.input.resolve()),
        "page_count": len(page_blocks),
        "expected_text": args.expect_text,
        "matched_text": matched_text,
        "missing_text": missing_text,
        "output_text": str(args.output_text.resolve()) if args.output_text else None,
    }

    if args.summary:
        args.summary.parent.mkdir(parents=True, exist_ok=True)
        args.summary.write_text(
            json.dumps(summary, ensure_ascii=False, indent=2),
            encoding="utf-8",
        )

    if missing_text:
        print("Missing expected PDF text layer content:", file=sys.stderr)
        for item in missing_text:
            print(f"- {item}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
