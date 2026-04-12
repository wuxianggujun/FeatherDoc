# Changelog

All notable FeatherDoc changes should be recorded in this file.

This project follows a pragmatic `MAJOR.MINOR.PATCH` release model oriented
around clear API behavior, MSVC buildability, diagnostics quality, and core-path
performance.

## [Unreleased]

### Fixed

- Added `scripts/assert_release_material_safety.ps1`, wired it into
  `write_release_note_bundle.ps1`, and taught `package_release_assets.ps1` to
  sanitize repo-local absolute paths inside staged text/JSON metadata before
  auditing and zipping public release materials.

### Documentation

- Added a Chinese historical-release backfill playbook, linked it from the
  Sphinx docs homepage and release policy, and recorded the current
  `2026-04-12` recovery baseline for the `v1.0.0` to `v1.6.4` release set.
- Cleaned the Sphinx documentation build by adding a hidden docs toctree,
  fixing existing reStructuredText syntax issues, adding the docs static
  directory placeholder, and ignoring `docs/_build/` in Git.

## [1.6.4] - 2026-04-11

### Added

- Added `scripts/package_release_assets.ps1` so a single release-candidate
  summary can now stage and package the public MSVC install ZIP, the
  screenshot gallery ZIP, and the release-evidence ZIP, with optional
  `gh release upload --clobber` publishing to the matching GitHub release.

### Fixed

- Release-facing metadata bundles now emit repository-relative paths across
  `START_HERE.md`, `ARTIFACT_GUIDE.md`, `REVIEWER_CHECKLIST.md`,
  `release_handoff.md`, `final_review.md`, and `gate_final_review.md` instead
  of leaking machine-local `C:\Users\...` paths back into public release
  drafts and reviewer handoff material.
- Visual-verdict sync and release-candidate final-review regeneration now keep
  those reviewer-facing evidence paths relative even after a later screenshot
  review refreshes the bundle.

## [1.6.3] - 2026-04-11

### Changed

- Refreshed the public README and Sphinx gallery so the repository now shows
  standalone Word-rendered `merge_right()` and `merge_down()` fixed-grid
  closure pages alongside the aggregate quartet signoff bundle and the Chinese
  invoice template output.
- Rebuilt `README.zh-CN.md` as a readable UTF-8 Simplified Chinese entry point
  instead of shipping a garbled document.
- Expanded release-facing quickstarts, templates, and README-gallery refresh
  scripts so installed `visual-validation/` assets now include the standalone
  fixed-grid closure pages and the Chinese business-template preview.

## [1.6.2] - 2026-04-11

### Fixed

- Public Chinese release-note generation now emits install-tree relative paths
  such as `share/FeatherDoc/...` plus repository-relative evidence paths instead
  of leaking machine-local absolute paths into `release_body.zh-CN.md` and
  `release_summary.zh-CN.md`.
- Replaced the release-note relative-path helper with a PowerShell-compatible
  implementation so local release-preflight runs no longer depend on
  `System.IO.Path.GetRelativePath()` availability.
- Added regression coverage for the public release-note bundle so the generated
  Chinese release body now fails tests if it contains the local install prefix
  or local working directory.

## [1.6.1] - 2026-04-11

### Added

- Added standalone fixed-grid merge/unmerge samples plus a bundled Word-rendered
  regression flow for `merge_right()`, `merge_down()`, `unmerge_right()`, and
  `unmerge_down()` so reopened tables now have screenshot-backed width-closure
  repros and review tasks.
- Added `fixed_grid_regression_bundle` and `docs_conf_version_sync` checks so
  the release candidate suite now covers reopened fixed-layout table workflows
  and guards Sphinx docs metadata drift from `CMakeLists.txt`.

### Changed

- Release metadata generation now persists `release_version` in the preflight
  summary and refreshes `release_handoff.md` / `release_body.zh-CN.md` against
  the matching `CHANGELOG.md` release section when available, instead of always
  falling back to `Unreleased`.
- Refreshed `README.md`, `README.zh-CN.md`, `docs/index.rst`, and installed
  visual-validation assets to highlight the reopened fixed-layout column-width
  Word rendering and fixed-grid aggregate evidence more directly.

### Fixed

- `docs/conf.py` now resolves `version` / `release` directly from the root
  `CMakeLists.txt`, preventing stale documentation headers during release prep.

## [1.6.0] - 2026-04-11

### Added

- Added `README.zh-CN.md` as a repository-level Simplified Chinese README,
  linked it from `README.md`, documented the bilingual entry in
  `docs/index.rst`, refreshed the docs homepage with current Word-rendered
  validation previews, added `scripts/refresh_readme_visual_assets.ps1` for
  repository-gallery refresh, taught `run_word_visual_release_gate.ps1` and
  `run_release_candidate_checks.ps1` to optionally trigger that refresh in the
  same run, and installed both README files under `share/FeatherDoc`.
- Added `VISUAL_VALIDATION.md` and `VISUAL_VALIDATION.zh-CN.md`, linked the
  preview galleries back to those repro notes, and installed the same
  screenshot assets plus repro guides under `share/FeatherDoc` so release
  artifacts now keep the screenshot -> script -> review-task entry visible.
- Added `VISUAL_VALIDATION_QUICKSTART.md` and
  `VISUAL_VALIDATION_QUICKSTART.zh-CN.md` as install-facing shortcut guides so
  `share/FeatherDoc` now exposes a direct preview PNG -> repro command ->
  review-task path without making users scan the longer README first, and now
  also points directly at the README-gallery refresh step.
- Added `RELEASE_ARTIFACT_TEMPLATE.md`,
  `RELEASE_ARTIFACT_TEMPLATE.zh-CN.md`, and
  `scripts/write_release_artifact_handoff.ps1` so release-preflight output can
  be turned into a release-facing handoff note that points back to installed
  preview PNGs, repro commands, and evidence files.
- Updated `scripts/run_release_candidate_checks.ps1` to reuse an already active
  MSVC developer environment, and updated `windows-msvc.yml` to upload a
  `windows-msvc-release-metadata` artifact carrying `share/FeatherDoc` plus a
  CI-side `release_handoff.md` / summary bundle for release-note drafting.
- Added `scripts/write_release_body_zh.ps1` so the release-preflight summary
  can also emit a directly editable Chinese release-body draft
  (`release_body.zh-CN.md`) alongside `release_handoff.md`, with the draft's
  "highlights" section seeded from `CHANGELOG.md` `Unreleased` entries.
- Updated `scripts/write_release_body_zh.ps1`,
  `scripts/run_release_candidate_checks.ps1`, and
  `scripts/write_release_artifact_handoff.ps1` so release-preflight now also
  emits a shorter `release_summary.zh-CN.md` draft, wires that path into the
  report bundle, and carries the short summary through handoff metadata and
  installed release-note guidance.
- Added `scripts/write_release_note_bundle.ps1` as the one-shot post-review
  refresh entry, and updated the quickstarts, templates, README/docs release
  notes, and handoff regeneration path to refresh `release_handoff.md`,
  `release_body.zh-CN.md`, and `release_summary.zh-CN.md` together after a
  later screenshot-backed visual verdict update.
- Added `scripts/sync_visual_review_verdict.ps1` so screenshot-backed task
  verdicts can now be promoted back into `gate_summary.json`,
  release-preflight `summary.json`, `gate_final_review.md`, `final_review.md`,
  and the regenerated release-note bundle without rerunning the full Windows
  preflight.
- Added `scripts/sync_latest_visual_review_verdict.ps1` as the shortest
  post-review helper so the newest document/fixed-grid task pointers can now
  auto-resolve the matching gate summary and release-preflight summary before
  delegating to the explicit verdict-sync path.
- Added a dedicated fixed-grid merge/unmerge regression bundle around
  `scripts/run_fixed_grid_merge_unmerge_regression.ps1`, covering
  `merge_right()`, `merge_down()`, `unmerge_right()`, and `unmerge_down()` via
  standalone runnable samples, aggregate Word-rendered contact sheets,
  screenshot-backed review-task packaging, and repository README gallery
  refresh for the fixed-grid quartet.
- Added `scripts/write_release_artifact_guide.ps1` so the generated report
  bundle now includes an `ARTIFACT_GUIDE.md` index that points artifact
  browsers at the installed `share/FeatherDoc` entry points plus the release
  summary/body/handoff files before they dive into the raw evidence set.
- Added `scripts/write_release_reviewer_checklist.ps1` so the report bundle now
  also emits a `REVIEWER_CHECKLIST.md` with a fixed three-step reviewer flow
  covering draft review, verification state, and publish-or-refresh handoff.
- Added `scripts/write_release_metadata_start_here.ps1`, a summary-root
  `START_HERE.md`, and an artifact-root `RELEASE_METADATA_START_HERE.md`, then
  updated the README/docs/templates so release metadata now has one consistent
  start-here entry before `ARTIFACT_GUIDE.md` and `REVIEWER_CHECKLIST.md`.
- Added `table_style_look`, `Table::style_look()`, `set_style_look(...)`, and
  `clear_style_look()` for editing `w:tblLook` on existing tables so first/last
  row or column emphasis plus row/column banding can be retuned without
  dropping to raw XML.
- Added `Document::restart_paragraph_list(...)` so managed bullet or decimal
  numbering can restart from a fresh `w:num` instance while adjacent list items
  continue the restarted sequence instead of falling back to the shared default
  instance.
- Added `TableCell::remove()` so lightweight table editing can delete one
  standalone column across the whole table while refusing to remove the last
  remaining column or any column that intersects a horizontal merge span.
- Added `TableCell::insert_cell_before()` and `insert_cell_after()` so
  lightweight table editing can insert one safe cloned column across the whole
  table while expanding `w:tblGrid`, clearing copied merge markup on the new
  cells, and refusing insertion points that land inside horizontal merge spans.
- Added `TableCell::unmerge_right()` and `unmerge_down()` so lightweight table
  editing can split one pure horizontal merged cell back into standalone
  siblings and remove one full vertical merge chain while keeping restored
  cells editable through the same wrappers.
- Added `Table::column_width_twips()`, `set_column_width_twips(...)`, and
  `clear_column_width()` so existing tables can edit `w:tblGrid/w:gridCol`
  widths directly and persist explicit per-column twip widths without dropping
  to raw XML. Fixed-layout tables now also mirror complete grid widths back
  into cell `w:tcW` values, including when `set_layout_mode(fixed)` is applied
  after the grid widths are already in place, while clearing one grid column
  width removes stale `w:tcW` hints from cells that still cover that column.
  Switching back to `autofit` or clearing `tblLayout` keeps the saved
  `w:gridCol` / `w:tcW` widths as preferred widths instead of discarding them.
  `set_width_twips(...)` and `clear_width()` likewise only change `w:tblW`
  and leave the saved grid/cell preferred widths intact.
  Manual `TableCell::set_width_twips(...)` / `clear_width()` edits remain
  possible, but the next fixed-grid synchronization pass rewrites those cells
  back to the grid-derived widths.
  Reopened legacy fixed-layout tables with stale `w:tcW` values can be
  normalized the same way by reapplying `set_layout_mode(fixed)`.
  The same normalization also runs when those reopened fixed-layout tables go
  through explicit span or column edits such as `merge_right()`,
  `unmerge_right()`, `set_column_width_twips(...)`, `insert_cell_before()` /
  `insert_cell_after()`, or `TableCell::remove()`.
  `clear_column_width()` follows the same reopened fixed-layout path for the
  cleared grid column, but only clears `w:tcW` on cells that still cover that
  column instead of re-normalizing unrelated stale cell widths.
  Plain non-grid edits such as `TableCell::set_text(...)`,
  `set_fill_color(...)`, and `set_border(...)` do not trigger that
  normalization on their own.
  The same is true for row-only structural edits:
  `TableRow::insert_row_before()`, `insert_row_after()`, and `remove()` keep
  reopened stale `w:tcW` values as-is instead of re-normalizing them from
  `w:tblGrid`.
- Added `samples/sample_edit_existing_table_style_look.cpp` as a runnable
  workflow that reopens a saved `.docx`, updates `tblLook` on an existing
  table, and preserves the underlying style reference.
- Added `samples/sample_restart_paragraph_list.cpp` as a runnable workflow that
  reopens a saved `.docx`, restarts an existing decimal list at `1.`, and
  continues the restarted sequence in later paragraphs.
- Added `samples/sample_remove_table_column.cpp` as a runnable workflow that
  reopens a saved `.docx`, removes a temporary middle column from an existing
  table, and continues editing the surviving result column through the same
  wrapper.
- Added `samples/sample_insert_table_column.cpp` as a runnable workflow that
  reopens a saved `.docx`, inserts one cloned column after the base column,
  inserts another before the result column, and keeps editing the new cells.
- Added `samples/sample_unmerge_table_cells.cpp` as a runnable workflow that
  reopens a saved `.docx`, splits one horizontal merge and one vertical merge,
  and continues editing the restored cells.
- Added `samples/sample_edit_existing_table_column_widths.cpp` as a runnable
  workflow that reopens a saved `.docx`, rewrites `w:tblGrid` column widths on
  an existing table, and makes the resulting narrow/medium/wide split easy to
  verify in Word.

### Fixed

- Fixed PowerShell parsing in `scripts/run_word_visual_smoke.ps1` and
  `scripts/run_fixed_grid_merge_unmerge_regression.ps1` so the generated review
  checklist text no longer trips `pwsh` on Markdown backticks during real local
  Word-rendered validation runs.

### Validation

- Validated table style-look edits with MinGW build + `ctest` and a Word visual
  smoke pass for `featherdoc_sample_edit_existing_table_style_look`, including
  a final XML check that the saved table keeps `w:tblStyle` and writes the
  expected `w:tblLook`.
- Validated paragraph-list restart edits with MinGW build + `ctest` and a Word
  visual smoke pass for `featherdoc_sample_restart_paragraph_list`, confirming
  the rendered document shows two decimal sequences that both begin at `1.`.
- Validated table-column removal edits with MinGW build + `ctest` and a Word
  visual smoke pass for `featherdoc_sample_remove_table_column`, confirming
  the rendered document keeps only the surviving two columns and the saved
  table shrinks to two `w:gridCol` entries while dropping the removed
  column's saved grid width.
- Validated safe table-column insertion edits with targeted `unit_tests` cases
  plus the Word visual smoke sample, confirming inserted columns stay aligned,
  inherit the source-side `w:gridCol` width after
  `insert_cell_before()` / `insert_cell_after()`, and still behave correctly
  after insertion at a merged-block boundary.
- Validated table-cell unmerge edits with targeted `unit_tests` cases plus the
  Word visual smoke sample, confirming `unmerge_right()` and `unmerge_down()`
  restore standalone cells without leftover merge markup in the saved XML.
- Validated table grid-column width edits with targeted `unit_tests` cases plus
  the Word visual smoke sample, confirming `Table::set_column_width_twips(...)`
  updates `w:tblGrid/w:gridCol`, `clear_column_width()` removes one saved
  `w:w` attribute, and the rendered layout keeps a visible narrow/medium/wide
  split.

## [1.5.0] - 2026-04-09

### Added

- Added `TemplatePart::paragraphs()`, `tables()`, and `append_table(...)` so
  existing body, header, and footer parts can use direct paragraph/table
  traversal and lightweight table creation/editing without dropping back to the
  owning `Document`.
- Added `TemplatePart::append_image(...)` and
  `TemplatePart::append_floating_image(...)` so existing body, header, and
  footer parts can append new inline or anchored images without dropping back
  to the owning `Document`.
- Added `TemplatePart::append_paragraph(...)` so existing body, header, and
  footer parts can append a new paragraph directly and continue editing the
  returned `Paragraph` handle without dropping back to paragraph cursor
  bookkeeping.
- Added `Paragraph::insert_paragraph_before()` so existing body, header,
  footer, and table-cell paragraph handles can prepend a new sibling paragraph
  directly before the current anchor without dropping to manual XML ordering.
- Added `Paragraph::insert_paragraph_like_before()` and
  `Paragraph::insert_paragraph_like_after()` so existing body, header, footer,
  and table-cell paragraph handles can clone paragraph-level properties such as
  styles, bidi flags, and list numbering into new sibling paragraphs without
  copying the anchor paragraph's body content.
- Added `Run::insert_run_before()` and `Run::insert_run_after()` so existing
  body, header, footer, and table-cell paragraphs can insert sibling runs
  around the current anchor run while keeping the anchor handle usable.
- Added `Run::insert_run_like_before()` and `Run::insert_run_like_after()` so
  existing body, header, footer, and table-cell paragraphs can clone the
  current anchor run's formatting properties into a new empty sibling run
  without copying the anchor's body content.
- Added `TableRow::insert_row_before()` for cloning the current row structure
  into a new empty row directly above it, while conservatively refusing rows
  that participate in vertical merge chains.
- Added `TableRow::insert_row_after()` for cloning the current row structure
  into a new empty row directly below it, while conservatively refusing rows
  that participate in vertical merge chains.
- Added `Table::insert_table_before()` and `Table::insert_table_after()` for
  inserting new sibling tables directly around an existing body/header/footer
  table while retargeting the wrapper to the inserted table.
- Added `Table::insert_table_like_before()` and `Table::insert_table_like_after()`
  for cloning an existing body/header/footer table's structure and
  table/row/cell formatting into a new empty sibling table while retargeting
  the wrapper to the inserted clone.
- Added `Table::cell_spacing_twips()`, `set_cell_spacing_twips()`, and
  `clear_cell_spacing()` for editing `w:tblCellSpacing` on existing tables
  without dropping to raw XML.
- Added `samples/sample_remove_table.cpp` as a runnable workflow that reopens
  a saved `.docx`, removes a temporary middle table, and continues editing the
  following table through the same wrapper.
- Added `samples/sample_insert_table_around_existing.cpp` as a runnable
  workflow that reopens a saved `.docx`, inserts new tables directly before and
  after an existing anchor table, and continues editing the surrounding tables
  through the returned wrappers.
- Added `samples/sample_insert_table_like_existing.cpp` as a runnable workflow
  that reopens a saved `.docx`, duplicates an existing table's layout and
  styling into new empty sibling tables, fills the clones, and keeps the
  original anchor table editable.
- Added `samples/sample_edit_existing_table_spacing.cpp` as a runnable workflow
  that reopens a saved `.docx`, adds visible inter-cell spacing to an existing
  table, and saves the updated layout back out.
- Added `samples/sample_edit_existing_part_tables.cpp` as a runnable workflow
  that creates header tables, reopens the saved `.docx`, removes a temporary
  middle header table, and continues editing the following table through the
  same wrapper.
- Added `samples/sample_edit_existing_part_append_images.cpp` as a runnable
  workflow that seeds body/header/footer content, reopens the saved `.docx`,
  and appends new inline or floating images through `TemplatePart` handles.
- Added `samples/sample_edit_existing_part_paragraphs.cpp` as a runnable
  workflow that seeds body/header/footer content, reopens the saved `.docx`,
  and appends new paragraphs through `TemplatePart` handles.
- Added `samples/sample_insert_paragraph_before.cpp` as a runnable workflow
  that seeds body/header/footer anchor paragraphs, reopens the saved `.docx`,
  and inserts new paragraphs directly before those existing anchors.
- Added `samples/sample_insert_paragraph_like_existing.cpp` as a runnable
  workflow that seeds styled body/header/footer anchor paragraphs, reopens the
  saved `.docx`, and clones paragraph formatting into new sibling paragraphs
  around those existing anchors.
- Added `samples/sample_insert_run_around_existing.cpp` as a runnable workflow
  that seeds body/header/footer anchor runs, reopens the saved `.docx`, and
  inserts new runs directly around those existing anchors.
- Added `samples/sample_insert_run_like_existing.cpp` as a runnable workflow
  that seeds styled body/header/footer anchor runs, reopens the saved `.docx`,
  and clones run formatting into new sibling runs around those existing
  anchors.
- Added `samples/sample_insert_table_row_before.cpp` as a runnable workflow
  that reopens a saved `.docx`, inserts a formatted row above an existing row,
  and saves the edited result back out.
- Added `samples/sample_insert_table_row.cpp` as a runnable workflow that
  reopens a saved `.docx`, inserts a formatted row in the middle of an
  existing table, and saves the edited result back out.
- Added `remove_bookmark_block(...)` across `Document` and `TemplatePart` as a
  high-level helper for deleting standalone bookmark placeholder paragraphs in
  body, header, or footer parts without having to pass an explicit empty
  replacement list.
- Added `samples/sample_remove_bookmark_block.cpp` as a runnable template edit
  flow for visually confirming that a standalone bookmark paragraph is removed
  cleanly from the rendered output.
- Added `remove_drawing_image(...)` and `remove_inline_image(...)` across
  `Document` and `TemplatePart` so existing inline or anchored images can be
  deleted while automatically pruning orphaned media parts on the next save.
- Added `samples/sample_remove_images.cpp` as a runnable workflow that creates,
  removes, and re-saves existing drawing-backed images for manual review.

### Validation

- Revalidated the table-removal and row-insertion editing flows with MSVC
  build + `ctest` and Word visual smoke passes for
  `featherdoc_sample_remove_table`,
  `featherdoc_sample_insert_table_around_existing`,
  `featherdoc_sample_edit_existing_part_tables`, and
  `featherdoc_sample_insert_table_row_before` before cutting `1.5.0`.
- Revalidated styled table-clone insertion with MSVC build + `ctest` and a
  Word visual smoke pass for `featherdoc_sample_insert_table_like_existing`
  before cutting `1.5.0`.
- Revalidated table cell-spacing edits with MinGW build + `ctest` and a Word
  visual smoke pass for `featherdoc_sample_edit_existing_table_spacing`
  before cutting `1.5.0`.
- Revalidated template-part image append flows with MSVC build + `ctest` and a
  Word visual smoke pass for
  `featherdoc_sample_edit_existing_part_append_images`, including a final
  manual review of the generated PDF/PNG evidence after correcting header
  floating-image placement before cutting `1.5.0`.
- Revalidated template-part paragraph append flows with MSVC build + `ctest`
  and a Word visual smoke pass for
  `featherdoc_sample_edit_existing_part_paragraphs` before cutting `1.5.0`.
- Revalidated paragraph insertion-before flows with MSVC build + `ctest` and a
  Word visual smoke pass for `featherdoc_sample_insert_paragraph_before`
  before cutting `1.5.0`.
- Revalidated paragraph property-clone insertion flows with MinGW build +
  `ctest` and a Word visual smoke pass for
  `featherdoc_sample_insert_paragraph_like_existing` before cutting `1.5.0`.
- Revalidated run insertion-around-anchor flows with MinGW build + `ctest` and
  a Word visual smoke pass for `featherdoc_sample_insert_run_around_existing`
  before cutting `1.5.0`.
- Revalidated run property-clone insertion flows with MinGW build + `ctest`
  and a Word visual smoke pass for
  `featherdoc_sample_insert_run_like_existing` before cutting `1.5.0`.

## [1.4.0] - 2026-04-08

### Added

- Expanded existing `.docx` editing workflows across paragraphs, runs,
  tables, bookmarks, headers/footers, and drawing-backed images, including
  safe `TableRow::remove()` behavior that preserves vertical merge restarts
  when deleting the first row of a merged block.
- Added `floating_image_horizontal_reference`,
  `floating_image_vertical_reference`, `floating_image_options`,
  `append_floating_image(...)`, and
  `replace_bookmark_with_floating_image(...)` so body/header/footer parts can
  create or replace anchored `wp:anchor` drawings without dropping to manual
  XML editing.

### Documentation

- Added `samples/sample_edit_existing.cpp`,
  `samples/sample_edit_existing_part_images.cpp`, and
  `samples/sample_floating_images.cpp`, then documented the related edit,
  drawing-image, and floating-image workflows in `README.md` and
  `docs/index.rst`.
- Added a minimal Chinese/CJK quickstart example and runnable Chinese template
  samples to `README.md` and `docs/index.rst`, and refreshed the README
  screenshot gallery plus sponsor layout.

### Validation

- Added `scripts/run_install_find_package_smoke.ps1` together with a minimal
  `find_package(FeatherDoc CONFIG REQUIRED)` consumer smoke project for MSVC
  install/export validation.
- Revalidated the release candidate with MSVC build + `ctest`, install smoke,
  and Word visual smoke for the floating image sample before cutting `1.4.0`.

## [1.3.0] - 2026-04-07

### Added

- `Run::font_family()`, `east_asia_font_family()`, `set_font_family()`,
   `set_east_asia_font_family()`, and `clear_font_family()` for explicit
   `w:rFonts` editing with `eastAsia` coverage for Chinese/CJK text.
- `default_run_font_family()`, `default_run_east_asia_font_family()`,
  `set_default_run_font_family()`, `set_default_run_east_asia_font_family()`,
  `clear_default_run_font_family()`, and matching `style_run_*` APIs for
  editing `word/styles.xml` `docDefaults` plus style-level `w:rFonts`
  inheritance for Chinese/CJK text.
- `Run::language()`, `east_asia_language()`, `set_language()`,
  `set_east_asia_language()`, `clear_language()`, plus matching
  `default_run_*` and `style_run_*` language APIs for editing `w:lang`
  inheritance in `document.xml` and `styles.xml`.
- `scripts/run_word_visual_smoke.ps1` now validates generated smoke DOCX
  archives before opening them in Word and cross-checks PDF page count against
  `summary.json` plus rendered PNG output so stale build artifacts fail fast.
- Matching `bidi_language()` / `set_bidi_language()` plus `default_run_*` and
  `style_run_*` bidi language APIs for editing `w:lang/@w:bidi` alongside
  existing `val` and `eastAsia` language inheritance.

### Validation

- Extended `samples/visual_smoke_tables.cpp` with explicit Chinese/CJK review
  text rendered through `docDefaults` and `Strong` style inheritance
  (`Microsoft YaHei` for `eastAsia`, `zh-CN` / `ar-SA` for `w:lang`) so the
  Word-to-PDF-to-PNG smoke flow checks mixed-script and bidi layout as well as
  borders and table geometry.
- `prepare_word_review_task.ps1` now seeds collision-safe task packages from
  reusable review templates and emits script-oriented prompts that explicitly
  require AI reviewers to write findings back into `report/`.

## [1.2.0] - 2026-04-07

### Added

- `append_table(row_count, column_count)` for creating new body tables on
  editable documents created from either `open()` or `create_empty()`.
- `Table::append_row()` and `TableRow::append_cell()` for growing existing or
  newly created tables without dropping down to manual XML editing.
- `TableCell::set_width_twips()`, `width_twips()`, and `clear_width()` for
  explicit cell width editing.
- `TableCell::merge_right()` and `column_span()` for lightweight horizontal
  cell merging and span inspection.
- `Table::set_width_twips()`, `clear_width()`, `style_id()`,
  `set_style_id()`, `clear_style_id()`, and `set_border()` /
  `clear_border()` plus `TableCell::merge_down()` and
  `TableCell::set_border()` / `clear_border()` for table-level width/style
  references, vertical merges, and table/cell border editing.
- `Table::layout_mode()`, `set_layout_mode()`, and `clear_layout_mode()` for
  switching tables between `autofit` and fixed layout modes.
- `Table::alignment()`, `set_alignment()`, `clear_alignment()`,
  `indent_twips()`, `set_indent_twips()`, and `clear_indent()` for table
  placement editing.
- `Table::cell_margin_twips()`, `set_cell_margin_twips()`, and
  `clear_cell_margin()` for table-level default cell margin editing.
- `TableRow::height_twips()`, `height_rule()`, `set_height_twips()`, and
  `clear_height()` for explicit row height editing.
- `TableRow::cant_split()`, `set_cant_split()`, and `clear_cant_split()` for
  row pagination control.
- `TableRow::repeats_header()`, `set_repeats_header()`, and
  `clear_repeats_header()` for repeat-header row editing.
- `TableCell::fill_color()`, `set_fill_color()`, `clear_fill_color()`,
  `margin_twips()`, `set_margin_twips()`, and `clear_margin()` for basic cell
  shading and per-edge margin editing.
- `TableCell::vertical_alignment()`, `set_vertical_alignment()`, and
  `clear_vertical_alignment()` for cell vertical alignment editing.
- `samples/visual_smoke_tables.cpp` plus `scripts/run_word_visual_smoke.ps1`
  and `scripts/render_pdf_pages.py` for Word-driven visual smoke checks that
  export generated `.docx` output to PDF and page PNGs.
- `append_image(path)` and `append_image(path, width_px, height_px)` for
  appending inline body images backed by managed `word/media/*` parts.
- `set_paragraph_list(paragraph, kind, level)` and
  `clear_paragraph_list(paragraph)` for attaching and removing managed bullet
  and decimal list numbering on paragraphs.
- `set_paragraph_style(paragraph, style_id)` /
  `clear_paragraph_style(paragraph)` plus `set_run_style(run, style_id)` /
  `clear_run_style(run)` for attaching and removing paragraph/run style
  references.
- `fill_bookmarks(...)` plus `bookmark_text_binding` /
  `bookmark_fill_result` for batch bookmark text filling as a first high-level
  template API.
- `replace_bookmark_with_paragraphs(...)` for replacing standalone bookmark
  paragraphs with zero or more generated plain-text paragraphs.
- `replace_bookmark_with_table_rows(...)` for expanding a standalone bookmark
  paragraph inside a template table row into zero or more cloned rows whose
  cell bodies are rewritten from plain-text data.
- `replace_bookmark_with_table(...)` and `replace_bookmark_with_image(...)`
  for replacing standalone bookmark paragraphs with generated table/image
  blocks.
- `TemplatePart` plus `body_template()`, `header_template()`,
  `footer_template()`, `section_header_template()`, and
  `section_footer_template()` for reusing bookmark template APIs across
  existing body, header, and footer parts, including bookmark-based image
  replacement.
- `set_bookmark_block_visibility(...)` and
  `apply_bookmark_block_visibility(...)` plus
  `bookmark_block_visibility_binding` /
  `bookmark_block_visibility_result` for high-level conditional block handling
  backed by bookmark marker paragraphs across body, header, and footer
  template parts.

### Changed

- Newly created or structurally extended tables now gain minimal `w:tblPr`,
  `w:tblGrid`, and `w:tcPr` scaffolding automatically so saved `.docx` output
  remains valid when appending rows or cells.
- Table column counting now honors existing `w:gridSpan` values, which keeps
  later structural edits aligned after horizontal cell merges.
- Saving now emits newly appended image media parts together with matching
  `document.xml.rels` and `[Content_Types].xml` updates.
- Saving now emits managed `word/numbering.xml` content together with matching
  `document.xml.rels` and `[Content_Types].xml` updates when paragraph lists
  are used.
- Saving now emits managed `word/styles.xml` content together with matching
  `document.xml.rels` and `[Content_Types].xml` updates when paragraph/run
  style references are used.
- Bookmark replacement logic is now split out of `document.cpp` into a
  dedicated template-focused translation unit.
- Bookmark template operations now target any loaded XML content part, which
  lets existing header/footer parts reuse the same text, paragraph, table-row,
  table, and image bookmark replacement pipeline as `document.xml`.
- Header/footer image insertion now writes per-part `.rels` entries together
  with managed `word/media/*` parts so those images survive save/reopen
  roundtrips.

## [1.1.0] - 2026-04-06

### Added

- `create_empty()` for creating a new in-memory `.docx` document without an
  existing template archive.
- `save_as(path)` semantics that keep `Document::path()` unchanged.
- `header_count()` / `footer_count()` and indexed `header_paragraphs()` /
  `footer_paragraphs()` accessors for editing existing header/footer parts.
- `ensure_header_paragraphs()` / `ensure_footer_paragraphs()` for creating and
  attaching default header/footer parts to the body-level section properties.
- `section_count()` and section-aware `section_header_paragraphs()` /
  `section_footer_paragraphs()` accessors for resolving existing
  header/footer references per section.
- `ensure_section_header_paragraphs()` / `ensure_section_footer_paragraphs()`
  for creating missing default/first/even header/footer references on a
  specific section.
- `assign_section_header_paragraphs()` / `assign_section_footer_paragraphs()`
  for rebinding a section to an already loaded header/footer part.
- `remove_section_header_reference()` / `remove_section_footer_reference()`
  for detaching one section-specific header/footer reference kind.
- `remove_header_part()` / `remove_footer_part()` for dropping one loaded
  header/footer part and detaching every section reference that points to it.
- `copy_section_header_references()` / `copy_section_footer_references()` for
  making one section reuse another section's current header/footer reference
  layout in bulk.
- `append_section(inherit_header_footer)` for appending a new final section and
  optionally inheriting the previous section's header/footer reference layout.
- `insert_section(section_index, inherit_header_footer)` for inserting a new
  section after an existing section while optionally inheriting that section's
  header/footer reference layout.
- `remove_section(section_index)` for collapsing one section boundary while
  preserving the surrounding document content.
- `move_section(source_section_index, target_section_index)` for reordering
  whole sections while keeping their header/footer reference layouts attached.
- `featherdoc_cli` with `inspect-sections`, `insert-section`, `remove-section`,
  `move-section`, and `copy-section-layout` commands for minimal section layout
  inspection and editing from the command line.
- `replace_section_header_text()` / `replace_section_footer_text()` for
  rewriting one section-specific header/footer part as plain paragraphs,
  creating the requested reference when needed.
- `featherdoc_cli` `show-section-header`, `show-section-footer`,
  `set-section-header`, and `set-section-footer` commands for section-level
  text inspection and replacement.
- `featherdoc_cli inspect-sections --json` and `show-section-header/footer
  --json` output for machine-readable section layout and paragraph inspection.
- `--json` output for mutating `featherdoc_cli` section commands, including
  `insert-section`, `remove-section`, `move-section`,
  `copy-section-layout`, `set-section-header`, and `set-section-footer`.
- `featherdoc_cli` commands for rebinding and removing section header/footer
  references plus deleting loaded header/footer parts:
  `assign-section-header`, `assign-section-footer`,
  `remove-section-header`, `remove-section-footer`,
  `remove-header-part`, and `remove-footer-part`.
- `inspect-header-parts` / `inspect-footer-parts` commands for listing loaded
  header/footer part indexes, their relationship ids, referenced sections, and
  paragraph text in either plain text or JSON.
- Automatic `w:titlePg` / `w:evenAndOddHeaders` enablement plus
  `word/settings.xml` read/write support when first-page or even-page
  header/footer references are created.
- Automatic pruning of unreferenced header/footer parts, corresponding
  `document.xml.rels` relationships, and `[Content_Types].xml` overrides
  during `save()` / `save_as()` output.
- Removing the last first-page or even-page section reference now also clears
  stale `w:titlePg` / `w:evenAndOddHeaders` markers.
- `replace_bookmark_text(name, replacement)` for rewriting text enclosed by a
  named bookmark range.
- Structured error reporting through `last_error()` with `detail`,
  `entry_name`, and `xml_offset`.
- `NOTICE`, `LEGAL.md`, Chinese licensing guidance, project identity notes, and
  release policy documentation.

### Changed

- Project branding, exported CMake target names, installed headers, and package
  metadata were migrated from DuckX naming to FeatherDoc naming.
- `Document` now uses `std::filesystem::path`.
- `open()` and `save()` now report failures through `std::error_code`.
- Formatting flags now use `enum class`.
- Installed package metadata now includes legal and project-facing repository
  files under `share/FeatherDoc`.
- README and Sphinx docs now document the current unsupported feature surface
  more explicitly, including encrypted `.docx`, section header/footer
  part-reordering limits,
  equations, and table creation.
- Removed the leftover upstream DuckX image asset directory from the repository.

- `open()` now preserves a single custom `std::error_category` instance across
  Windows shared-library boundaries, so `document_errc` compares correctly in
  MSVC DLL builds.
- Windows shared builds now export/link correctly and copy runtime DLLs next to
  tests and samples, allowing MSVC `ctest` runs to execute without missing-DLL
  failures.
- `open()` now supports reading `.docx` files while Microsoft Word keeps the
  source file open on Windows.
- Paragraph and run traversal now stay on matching WordprocessingML siblings
  instead of drifting into unrelated nodes.
- Appending a paragraph to `w:body` now inserts it before `w:sectPr`, which
  avoids invalid document ordering and fixes `insert_paragraph_after()` save
  regressions.
- `open()` now reports password-protected or encrypted `.docx` files through an
  explicit FeatherDoc error code instead of a generic XML read failure.

### Performance

- `open()` now keeps ZIP-owned XML buffers on the FeatherDoc side before
  parsing, which avoids cross-library allocator mismatches in shared-library
  builds.
- `save()` / `save_as()` now stream `document.xml` and copy ZIP entries
  chunk-by-chunk instead of materializing whole archive entries in memory.

### Dependencies

- Updated bundled `pugixml` to `1.15`.
- Updated bundled `kuba--/zip` to `0.3.8`.
- Updated bundled `doctest` to `2.5.1`.

### Validation

- MSVC configuration, build, and test flow is documented and currently used as
  the main verification baseline.
- Added end-to-end CLI coverage for section inspection plus insert/copy/move/
  remove workflows.
