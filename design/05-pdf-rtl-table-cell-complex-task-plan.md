# PDF RTL table cell complex task plan

本文件是 RTL table cell 后续推进的执行源。简单 digit / punctuation 矩阵已经闭环；
接下来只做复杂 cell 语义、raw-preservation 和验证收口，不继续无边界扩展
table-cell normalizer。

## Scope

In:
- 固定复杂 RTL table cell 的语义边界。
- 补齐 multi-cluster guard 的显式测试。
- 把设计决策同步到 roadmap / execution plan / build runbook。
- 跑 PDF 定向、广域、回归和视觉门禁静态验证。

Out:
- 不扩大 paragraph / line 级 bidi normalization。
- 不新增通用 bidi 重排器。
- 不放开非 LTR glyph-id CID writer guard。
- 不 stage，不 commit。

## Execution checklist

- [x] Read this document before editing.
- [x] Freeze simple boundary expansion in docs: Hebrew / Arabic core、ASCII digits、
      Arabic-Indic digits、Extended Arabic-Indic / Persian digits 和常见 ASCII
      punctuation 已闭环，后续新增必须先证明新的 Unicode class 或 PDFium raw 形态。
- [x] Add explicit carriage-return multi-cluster negative coverage for
      `normalize_rtl_table_cell_text_for_testing()` alongside existing newline
      guard coverage.
- [x] Document the complex-cell decision table:
      pure single-cluster cells may normalize; split Hebrew newline cells stay raw;
      Arabic split pure RTL may normalize only when PDFium produces a single raw
      cluster; mixed-bidi and combining-mark cells stay raw.
- [x] Confirm release gate taxonomy remains clear:
      `rtl-table-cell-normalization` for controlled simple cases and
      `rtl-table-cell-raw-preservation` for complex raw-preservation cases.
- [x] Run directed PDF Unicode roundtrip validation with `--timeout 60`.
- [x] Run PDF broad validation 6/6 with `--timeout 60`.
- [x] Run `pdf_regression_` with `--timeout 60`.
- [x] Run visual release gate static checks with `--timeout 60`.
- [x] Render or reuse the latest relevant contact sheet and inspect it when
      visual output changed.
- [x] Run `git diff --check` and `git status --short --branch`.

## Latest validation

- Directed:
  `cmd /s /c '"D:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat" && cmake --build .bpdf-roundtrip-msvc --target pdf_unicode_font_roundtrip_tests && ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_unicode_font_roundtrip$" --output-on-failure --timeout 60'`
  passed.
- Broad:
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font|unicode_font_roundtrip|import_structure|import_failure|import_table_heuristic)$" --output-on-failure --timeout 60`
  passed 6/6.
- Regression:
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60`
  passed 40/40.
- Visual gate static:
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_visual_release_gate_(style|text_shaping)_baselines$" --output-on-failure --timeout 60`
  passed 2/2.
- Contact sheet:
  `output\pdf-rtl-complex-task-plan-split-pure-rtl\contact-sheet.png`
  inspected; the split pure RTL complex baseline is non-empty and has no visible
  clipping, overlap, or table grid drift.
- Repository checks:
  `git diff --check` passed with only LF/CRLF warnings; `git status --short --branch`
  confirmed the expected modified files plus this untracked task document, with
  no staged changes.

## Complex-cell decision table

- Pure single-cluster RTL cell: may normalize when PDFium raw text is one
  reversible visual cluster and has no digits, combining marks, LTR text, `\n`,
  or `\r`.
- Controlled digit / punctuation cell: may normalize only for the locked
  Hebrew / Arabic core matrices covering ASCII digits, Arabic-Indic digits,
  Extended Arabic-Indic / Persian digits, and the documented ASCII punctuation
  set.
- Split Hebrew pure RTL cell: stays raw when PDFium emits multi-cluster text
  containing `\n` or `\r`; current helper returns false before any reversal.
- Split Arabic pure RTL cell: may normalize only when PDFium collapses the cell
  to a single raw cluster such as `مالس `; do not infer Hebrew behavior from
  Arabic behavior or the reverse.
- Split mixed-bidi cell: stays raw. The test suite records raw span order and
  geometry, but does not restore logical text.
- Combining-mark cell: stays raw until mark ownership is modeled at the glyph /
  cluster level. Hebrew niqqud and Arabic harakat fixtures are raw-preservation
  baselines, not normalization baselines.

## Release gate taxonomy

- `rtl-table-cell-normalization`: simple, controlled normalization baselines
  where logical output is expected.
- `rtl-table-cell-raw-preservation`: complex baselines whose purpose is to keep
  PDFium raw text, raw spans, table geometry, and import parity stable.
- A new RTL table cell baseline must choose one taxonomy label before it is
  added to `run_pdf_visual_release_gate.ps1`.

## Phase 2 checklist

- [x] Strengthen split pure RTL import assertions:
      Hebrew split pure RTL default import and raw import both stay equal to
      PDFium raw cell text.
- [x] Strengthen Arabic split pure RTL import assertions:
      default import equals normalized logical text, raw import does not contain
      logical text and keeps the PDFium raw core.
- [x] Record Phase 2 validation commands and results in this document.

## Phase 2 validation

- Directed:
  `cmd /s /c '"D:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat" && cmake --build .bpdf-roundtrip-msvc --target pdf_unicode_font_roundtrip_tests && ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_unicode_font_roundtrip$" --output-on-failure --timeout 60'`
  passed.
- Broad:
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font|unicode_font_roundtrip|import_structure|import_failure|import_table_heuristic)$" --output-on-failure --timeout 60`
  passed 6/6.
- Regression:
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60`
  passed 40/40.
- Visual gate static:
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_visual_release_gate_(style|text_shaping)_baselines$" --output-on-failure --timeout 60`
  passed 2/2.
- Contact sheet:
  `output\pdf-rtl-complex-task-plan-phase2-arabic-split\contact-sheet.png`
  inspected; the Arabic split pure RTL baseline is non-empty and has no visible
  clipping, overlap, or table grid drift.

## Phase 3 checklist

- [x] Add import parity coverage for divergent complex normalization:
      when Arabic split pure RTL normalizes only the target cell, all other
      imported table cells must stay identical between default and raw import.
- [x] Verify the new import parity assertion with directed
      `pdf_unicode_font_roundtrip`.
- [x] Run broad PDF validation, regression validation, visual gate static
      validation, and `git diff --check`.

## Phase 3 validation

- Directed:
  `cmd /s /c '"D:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat" && cmake --build .bpdf-roundtrip-msvc --target pdf_unicode_font_roundtrip_tests && ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_unicode_font_roundtrip$" --output-on-failure --timeout 60'`
  passed.
- Broad:
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font|unicode_font_roundtrip|import_structure|import_failure|import_table_heuristic)$" --output-on-failure --timeout 60`
  passed 6/6.
- Regression:
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60`
  passed 40/40.
- Visual gate static:
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_visual_release_gate_(style|text_shaping)_baselines$" --output-on-failure --timeout 60`
  passed 2/2.
- Contact sheet:
  `output\pdf-rtl-complex-task-plan-phase3-arabic-split\contact-sheet.png`
  inspected; the Arabic split pure RTL baseline is non-empty and has no visible
  clipping, overlap, or table grid drift.
- Repository checks:
  `git diff --check` passed with only LF/CRLF warnings.

## Completion criteria

- All checklist items above are marked complete.
- No temporary diagnostics remain.
- All test commands that are `ctest` include `--timeout 60`.
- The final response reports tests, contact sheet path when used, and
  `git diff --check` result.
