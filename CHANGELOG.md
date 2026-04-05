# Changelog

All notable FeatherDoc changes should be recorded in this file.

This project follows a pragmatic `MAJOR.MINOR.PATCH` release model oriented
around clear API behavior, MSVC buildability, diagnostics quality, and core-path
performance.

## [Unreleased]

### Added

- `create_empty()` for creating a new in-memory `.docx` document without an
  existing template archive.
- `save_as(path)` semantics that keep `Document::path()` unchanged.
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
  more explicitly, including encrypted `.docx`, header/footer APIs, equations,
  and table creation.

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
