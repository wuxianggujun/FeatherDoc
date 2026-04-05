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

## Build With MSVC

Open an `x64` Visual Studio Developer Command Prompt first, or initialize the
environment with `VsDevCmd.bat -arch=x64 -host_arch=x64`, then run:

```bat
cmake -S . -B build-msvc-nmake -G "NMake Makefiles" -DBUILD_TESTING=ON -DBUILD_SAMPLES=ON
cmake --build build-msvc-nmake
ctest --test-dir build-msvc-nmake --output-on-failure --timeout 60
```

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
- The current public API reads body paragraphs, runs, and tables, but does not
  expose dedicated header/footer editing APIs.
- Word equations (`OMML`) are not surfaced through a typed equation API.
- Existing tables can be traversed, but there is no high-level API for creating
  new tables programmatically yet.
- Bookmark-targeted batch replacement is not exposed as a dedicated API yet.

## Bundled Dependencies

- `pugixml` `1.15`
- `kuba--/zip` `0.3.8`
- `doctest` `2.5.1`

## Documentation

- Changelog: `CHANGELOG.md`
- Sphinx docs entry: `docs/index.rst`
- Project identity guide: `docs/project_identity_zh.rst`
- Initial audit notes: `docs/project_audit_zh.rst`
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
