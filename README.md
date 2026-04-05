# FeatherDoc

FeatherDoc is a modernized C++ library for reading and writing Microsoft Word
`.docx` files.

## Highlights

- CMake 3.20+
- C++20
- MSVC-friendly build setup
- Lightweight paragraph/run/table traversal API
- MSVC-safe XML parsing on `open()`
- Streamed ZIP rewrite path on `save()`

## Build

```bash
cmake -S . -B build
cmake --build build
```

Top-level builds enable `BUILD_CLI` by default, so the `featherdoc_cli`
utility is built alongside the library unless you pass `-DBUILD_CLI=OFF`.

## Build With MSVC

Open an `x64` Visual Studio Developer Command Prompt first, or initialize the
environment with `VsDevCmd.bat -arch=x64 -host_arch=x64`, then run:

```bat
cmake -S . -B build-msvc-nmake -G "NMake Makefiles" -DBUILD_TESTING=ON -DBUILD_SAMPLES=ON -DBUILD_CLI=ON
cmake --build build-msvc-nmake
ctest --test-dir build-msvc-nmake --output-on-failure --timeout 60
```

## CLI

`featherdoc_cli` is a small command-line wrapper around the current
section-aware header/footer APIs.

```bash
featherdoc_cli inspect-sections input.docx
featherdoc_cli insert-section input.docx 1 --no-inherit --output inserted.docx
featherdoc_cli copy-section-layout input.docx 0 2 --output copied.docx
featherdoc_cli move-section input.docx 2 0 --output reordered.docx
featherdoc_cli remove-section input.docx 3 --output trimmed.docx
featherdoc_cli show-section-header input.docx 1 --kind even
featherdoc_cli set-section-footer input.docx 0 --text "Page 1" --output footer.docx
featherdoc_cli set-section-header input.docx 2 --kind even --text-file header.txt
```

`inspect-sections` prints the current section count together with per-section
header/footer attachment flags for `default`, `first`, and `even` references.
The mutating commands save in place by default; pass `--output <path>` to write
to a separate `.docx`.

`show-section-header` / `show-section-footer` print the referenced paragraphs
one line per paragraph. `set-section-header` / `set-section-footer` rewrite the
target part as plain paragraphs from `--text` or `--text-file`, and create the
requested section reference automatically when it does not exist yet.

## Install

```bash
cmake --install build --prefix install
```

The installed package now also carries repository-facing metadata and legal
files under `share/FeatherDoc`, including `CHANGELOG.md`, `README.md`,
`LICENSE`, `LICENSE.upstream-mit`, `NOTICE`, and `LEGAL.md`.

## Use From CMake

```cmake
list(APPEND CMAKE_PREFIX_PATH "/path/to/FeatherDoc/install")
find_package(FeatherDoc CONFIG REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE FeatherDoc::FeatherDoc)
```

The generated package config also exposes:

- `FeatherDoc_VERSION`
- `FeatherDoc_DESCRIPTION`
- `FeatherDoc_PACKAGE_DATA_DIR`

## Quick Start

```cpp
#include <featherdoc.hpp>
#include <iostream>

int main() {
    featherdoc::Document doc("file.docx");
    if (const auto error = doc.open()) {
        const auto& error_info = doc.last_error();
        std::cerr << error.message();
        if (!error_info.detail.empty()) {
            std::cerr << ": " << error_info.detail;
        }
        if (!error_info.entry_name.empty()) {
            std::cerr << " [entry=" << error_info.entry_name << ']';
        }
        if (error_info.xml_offset.has_value()) {
            std::cerr << " [xml_offset=" << *error_info.xml_offset << ']';
        }
        std::cerr << '\n';
        return 1;
    }

    for (auto paragraph : doc.paragraphs()) {
        std::string text;
        for (auto run : paragraph.runs()) {
            text += run.get_text();
        }
        std::cout << text << '\n';
    }

    for (auto table : doc.tables()) {
        for (auto row : table.rows()) {
            for (auto cell : row.cells()) {
                for (auto paragraph : cell.paragraphs()) {
                    std::string text;
                    for (auto run : paragraph.runs()) {
                        text += run.get_text();
                    }
                    std::cout << text << '\n';
                }
            }
        }
    }

    return 0;
}
```

`Run` represents WordprocessingML text runs, not whole lines. If one logical line
is split into multiple runs, concatenate `run.get_text()` values inside a
paragraph before printing. Text inside tables is traversed through
`doc.tables() -> rows() -> cells() -> paragraphs()`.

## Formatting Flags

```cpp
paragraph.add_run("bold text", featherdoc::formatting_flag::bold);
paragraph.add_run(
    "mixed style",
    featherdoc::formatting_flag::bold |
        featherdoc::formatting_flag::italic |
        featherdoc::formatting_flag::underline
);
```

`Document` now uses `std::filesystem::path`, and both `open()` and `save()`
return `std::error_code` so callers can inspect the failure reason directly.

```cpp
if (const auto error = doc.save()) {
    const auto& error_info = doc.last_error();
    std::cerr << error.message() << '\n';
    std::cerr << error_info.detail << '\n';
    return 1;
}
```

`last_error()` exposes structured context for the most recent failure:

- `code`: the same `std::error_code` returned by `open()` / `save()`
- `detail`: richer human-readable explanation
- `entry_name`: failing ZIP entry when relevant
- `xml_offset`: parse offset for malformed XML

Use `save_as(path)` when you want to keep the current source document path
unchanged and write the modified document to a different output file.

```cpp
if (const auto error = doc.save_as("copy.docx")) {
    std::cerr << error.message() << '\n';
    return 1;
}
```

Use `replace_bookmark_text(name, replacement)` when you want to rewrite the
content enclosed by a named bookmark range. The method returns the number of
bookmark ranges replaced.

```cpp
if (doc.replace_bookmark_text("customer_name", "FeatherDoc User") == 0) {
    std::cerr << "bookmark not found\n";
    return 1;
}
```

Use `header_count()`, `footer_count()`, `header_paragraphs(index)`, and
`footer_paragraphs(index)` when you need to read or edit paragraphs stored in
existing header/footer parts.

```cpp
for (std::size_t i = 0; i < doc.header_count(); ++i) {
    for (auto paragraph = doc.header_paragraphs(i); paragraph.has_next();
         paragraph.next()) {
        for (auto run = paragraph.runs(); run.has_next(); run.next()) {
            std::cout << run.get_text() << '\n';
        }
    }
}
```

Use `ensure_header_paragraphs()` and `ensure_footer_paragraphs()` when you need
to create and attach a default header/footer to the document's body-level
section properties before editing it.

```cpp
auto header = doc.ensure_header_paragraphs();
header.add_run("Generated header");

auto footer = doc.ensure_footer_paragraphs();
footer.add_run("Page 1");
```

Use `section_count()`, `section_header_paragraphs(section_index, kind)`, and
`section_footer_paragraphs(section_index, kind)` when you need to resolve the
existing header/footer reference attached to a specific section.

```cpp
for (std::size_t i = 0; i < doc.section_count(); ++i) {
    auto header = doc.section_header_paragraphs(i);
    if (header.has_next()) {
        std::cout << header.runs().get_text() << '\n';
    }
}
```

Use `ensure_section_header_paragraphs(section_index, kind)` and
`ensure_section_footer_paragraphs(section_index, kind)` when you need to create
and attach a missing section-specific header/footer reference before editing it.
When `kind` is `first_page` or `even_page`, FeatherDoc also enables the
required WordprocessingML switches (`w:titlePg` or
`word/settings.xml` -> `w:evenAndOddHeaders`) automatically.

```cpp
auto even_header = doc.ensure_section_header_paragraphs(
    1, featherdoc::section_reference_kind::even_page);
even_header.add_run("Even page header");

auto first_footer = doc.ensure_section_footer_paragraphs(
    1, featherdoc::section_reference_kind::first_page);
first_footer.add_run("First page footer");
```

Use `assign_section_header_paragraphs(section_index, header_index, kind)` and
`assign_section_footer_paragraphs(section_index, footer_index, kind)` when you
need multiple sections to reuse an existing header/footer part instead of
creating a new one. Each call only rebinds the requested `kind`, so reuse
across multiple kinds on the same section needs one call per kind.

```cpp
auto shared_header = doc.assign_section_header_paragraphs(1, 0);
shared_header.runs().set_text("Shared header");

auto shared_footer = doc.assign_section_footer_paragraphs(1, 0);
shared_footer.runs().set_text("Shared footer");

auto shared_first_footer = doc.assign_section_footer_paragraphs(
    1, 0, featherdoc::section_reference_kind::first_page);
shared_first_footer.runs().set_text("Shared footer");
```

Use `remove_section_header_reference(section_index, kind)` and
`remove_section_footer_reference(section_index, kind)` to detach a specific
section-level reference without touching other kinds already attached to the
same section.

```cpp
doc.remove_section_header_reference(1);
doc.remove_section_footer_reference(
    1, featherdoc::section_reference_kind::first_page);
```

When a header/footer part is no longer referenced from `document.xml`,
`save()` / `save_as()` automatically omit the orphaned part together with the
matching `document.xml.rels` relationship and `[Content_Types].xml` override.
Removing the last first-page or even-page reference also drops `w:titlePg` or
`w:evenAndOddHeaders` when that flag is no longer needed.

Use `remove_header_part(index)` and `remove_footer_part(index)` when you want to
drop one loaded header/footer part entirely. The matching section references are
detached in memory, `header_count()` / `footer_count()` shrink immediately, and
the orphaned ZIP entries are omitted on the next save.

```cpp
doc.remove_header_part(1);
doc.remove_footer_part(1);
```

Use `copy_section_header_references(source_section, target_section)` and
`copy_section_footer_references(source_section, target_section)` when one
section should adopt another section's current header/footer reference layout.
The target side is replaced for that reference family, so stale `first` / `even`
references are removed automatically.

```cpp
doc.copy_section_header_references(0, 1);
doc.copy_section_footer_references(0, 1);
```

Use `replace_section_header_text(section_index, replacement, kind)` and
`replace_section_footer_text(section_index, replacement, kind)` when you want
to rewrite a section-specific header/footer as plain paragraph text in one step.
The replacement text is split on newlines, and the requested reference is
created automatically when it is missing.

```cpp
doc.replace_section_header_text(0, "Title line\nSubtitle line");
doc.replace_section_footer_text(
    0, "First page footer", featherdoc::section_reference_kind::first_page);
```

Use `append_section(inherit_header_footer)` to append a new final section at the
end of the document. By default it inherits the previous final section's
header/footer reference layout; passing `false` appends the new section without
copying those references.

```cpp
doc.append_section();
doc.append_section(false);
```

Use `insert_section(section_index, inherit_header_footer)` to insert a new
section after an existing section. By default the inserted section inherits the
referenced section's current header/footer reference layout; passing `false`
creates the new section break without copying those references.

```cpp
doc.insert_section(0);
doc.insert_section(1, false);
```

Use `remove_section(section_index)` to remove one section while preserving the
document content around it. Removing a non-final section collapses its break so
that content flows into the following section; removing the final section makes
the previous section become the new final section.

```cpp
doc.remove_section(1);
```

Use `move_section(source_section_index, target_section_index)` to reorder whole
sections. The section content and its header/footer reference layout move
together, and `target_section_index` is the final index of the moved section
after reordering.

```cpp
doc.move_section(2, 0);
```

Use `create_empty()` when you want to build a new `.docx` document from scratch
without relying on an existing template archive.

```cpp
featherdoc::Document doc("new-file.docx");
if (const auto error = doc.create_empty()) {
    std::cerr << error.message() << '\n';
    return 1;
}

doc.paragraphs().add_run("Hello FeatherDoc");
if (const auto error = doc.save()) {
    std::cerr << error.message() << '\n';
    return 1;
}
```

## Performance Notes

- `open()` now keeps XML buffer ownership on the FeatherDoc side before parsing,
  which avoids cross-library allocator mismatches in shared-library builds.
- `save()` now streams `document.xml` directly into the output archive instead
  of materializing one large intermediate string.
- `save()` also copies non-XML ZIP entries chunk-by-chunk, which avoids loading
  each entry into heap memory before writing it back.

## Current Limitations

- Password-protected or encrypted `.docx` files are not supported yet.
- Section-specific header/footer references can now be created and rebound
  through `ensure_section_header_paragraphs()` /
  `ensure_section_footer_paragraphs()` and
  `assign_section_header_paragraphs()` / `assign_section_footer_paragraphs()`,
  and removed through `remove_section_header_reference()` /
  `remove_section_footer_reference()`. Whole parts can also be dropped through
  `remove_header_part()` / `remove_footer_part()`, and section reference
  layouts can be copied through `copy_section_header_references()` /
  `copy_section_footer_references()`. New sections can now be appended or
  inserted after an existing section through `append_section()` /
  `insert_section()`, removed through `remove_section()`, and reordered through
  `move_section()`, but there is still no high-level API for part reordering.
- Word equations (`OMML`) are not surfaced through a typed equation API.
- Existing tables can be traversed, but there is no high-level API for creating
  new tables programmatically yet.

## Source Layout

The core implementation is now split into focused translation units instead of
living in a single large `.cpp` file:

- `src/document.cpp`: `Document` open/save flow, archive handling, and error reporting
- `src/paragraph.cpp`: paragraph traversal, run creation, and paragraph insertion
- `src/run.cpp`: run traversal and text read/write behavior
- `src/table.cpp`: table, row, and cell traversal helpers
- `src/xml_helpers.cpp` / `src/xml_helpers.hpp`: internal XML helper utilities shared by the modules
- `src/constants.cpp`: exported constants and error-category plumbing
- `cli/featherdoc_cli.cpp`: minimal section-layout inspection and editing utility

This layout keeps archive I/O, XML navigation, and public API objects easier to
reason about and extend independently.

## Bundled Dependencies

- `pugixml` `1.15`
- `kuba--/zip` `0.3.8`
- `doctest` `2.5.1`

## Documentation

- Changelog: `CHANGELOG.md`
- Sphinx docs entry: `docs/index.rst`
- Project identity guide: `docs/project_identity_zh.rst`
- Initial audit notes: `docs/project_audit_zh.rst`
- Upstream issue triage: `docs/upstream_issue_triage_zh.rst`
- Release policy guide: `docs/release_policy_zh.rst`
- Chinese license guide: `docs/licensing_zh.rst`
- Repository legal notes: `LEGAL.md`
- Distribution notice summary: `NOTICE`

## Project Direction

FeatherDoc should be treated as its own actively-shaped fork rather than a
passive mirror of the historical upstream project.

- Modern C++ and clearer API semantics take priority over preserving obsolete
  compatibility patterns.
- MSVC buildability is a real support target, not a best-effort afterthought.
- Error diagnostics, save/open behavior, and core-path performance are first
  class concerns.
- Project positioning, licensing, documentation, and repository metadata follow
  the current FeatherDoc direction.

## Sponsor

If this project helps your work, you can support ongoing maintenance via the
following payment QR codes.

### Alipay

![Alipay QR Code](sponsor/zhifubao.jpg)

### WeChat Pay

![WeChat Pay QR Code](sponsor/weixin.jpg)

## License

This fork should be described as source-available rather than open source for
its fork-specific modifications.

- Fork-specific FeatherDoc modifications are distributed under the
  non-commercial source-available terms in `LICENSE`.
- Upstream DuckX-derived portions still keep the original MIT text preserved in
  `LICENSE.upstream-mit`.
- Bundled third-party dependencies keep their own original licenses.
- Practical Chinese reading guide: `docs/licensing_zh.rst`
- Short repository legal guide: `LEGAL.md`
- Distribution notice file: `NOTICE`
