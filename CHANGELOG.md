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

### Performance

- `open()` now parses `word/document.xml` in-place through `pugixml` buffer
  ownership transfer.
- `save()` / `save_as()` now stream `document.xml` and copy ZIP entries
  chunk-by-chunk instead of materializing whole archive entries in memory.

### Dependencies

- Updated bundled `pugixml` to `1.15`.
- Updated bundled `kuba--/zip` to `0.3.8`.
- Updated bundled `doctest` to `2.5.1`.

### Validation

- MSVC configuration, build, and test flow is documented and currently used as
  the main verification baseline.
