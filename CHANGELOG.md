# Changelog

All notable FeatherDoc changes should be recorded in this file.

This project follows a pragmatic `MAJOR.MINOR.PATCH` release model oriented
around clear API behavior, MSVC buildability, diagnostics quality, and core-path
performance.

## [Unreleased]

### Added

- `append_table(row_count, column_count)` for creating new body tables on
  editable documents created from either `open()` or `create_empty()`.
- `Table::append_row()` and `TableRow::append_cell()` for growing existing or
  newly created tables without dropping down to manual XML editing.
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
- `replace_bookmark_with_table(...)` and `replace_bookmark_with_image(...)`
  for replacing standalone bookmark paragraphs with generated table/image
  blocks.

### Changed

- Newly created or structurally extended tables now gain minimal `w:tblPr`,
  `w:tblGrid`, and `w:tcPr` scaffolding automatically so saved `.docx` output
  remains valid when appending rows or cells.
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

### Fixed

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
