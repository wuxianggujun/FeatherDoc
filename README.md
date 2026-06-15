# FeatherDoc

[![Windows MSVC CI](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/windows-msvc.yml/badge.svg?branch=dev)](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/windows-msvc.yml)
[![Linux CMake CI](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/linux-cmake.yml/badge.svg?branch=dev)](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/linux-cmake.yml)
[![macOS CMake CI](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/macos-cmake.yml/badge.svg?branch=dev)](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/macos-cmake.yml)

[Simplified Chinese](README.zh-CN.md) | English

FeatherDoc is a C++20 library for creating, reading, editing, inspecting, and
rewriting Microsoft Word `.docx` files.

This README is intentionally short. Detailed API pages, workflows, release
governance, PDF notes, and visual validation material live in the documentation
site.

## Documentation

- English documentation: <https://wuxianggujun.github.io/FeatherDoc/en/>
- Simplified Chinese documentation: <https://wuxianggujun.github.io/FeatherDoc/zh-CN/>
- English API reference: <https://wuxianggujun.github.io/FeatherDoc/en/api/>
- Simplified Chinese API reference: <https://wuxianggujun.github.io/FeatherDoc/zh-CN/api/>
- English getting started: <https://wuxianggujun.github.io/FeatherDoc/en/getting_started.html>
- Simplified Chinese getting started: <https://wuxianggujun.github.io/FeatherDoc/zh-CN/getting_started.html>
- PDF workflows: <https://wuxianggujun.github.io/FeatherDoc/en/api/pdf_workflow.html>
- Source docs: [`docs/index.rst`](docs/index.rst)
- Documentation maintenance: [`docs/documentation_maintenance_zh.rst`](docs/documentation_maintenance_zh.rst)
- Script task index: [`docs/script_task_index_zh.rst`](docs/script_task_index_zh.rst)

## What You Can Build

- Create and edit `.docx` packages.
- Edit paragraphs, runs, tables, rows, cells, styles, numbering, sections,
  headers, and footers.
- Fill bookmarks and content controls by tag or alias.
- Replace template slots with text, paragraphs, tables, rows, and images.
- Inspect fields, hyperlinks, comments, footnotes, endnotes, revisions, images,
  styles, numbering, and semantic document differences.
- Use `featherdoc_cli` for scriptable inspection and one-shot rewrites.

## Quick Start

```bash
cmake -S . -B build
cmake --build build
cmake --install build --prefix install
```

Run the project-template smoke path that validates the Chinese invoice
template, renders it from business JSON, and writes review artifacts:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_project_template_smoke.ps1 `
  -ManifestPath .\samples\project_template_smoke.manifest.json `
  -BuildDir .\build `
  -OutputDir .\output\project-template-smoke `
  -SkipBuild `
  -SkipVisualSmoke
```

Minimal C++ usage:

```cpp
#include <featherdoc.hpp>

int main() {
    featherdoc::Document doc{"template.docx"};
    if (doc.open()) {
        return 1;
    }

    doc.body_template().replace_content_control_text_by_tag("customer", "Ada");
    return doc.save_as("output.docx") ? 1 : 0;
}
```

## Branch Policy

- `dev` is the active development branch.
- `master` is kept as the stable/release branch.
- Codex/local automation work should continue directly on the current `dev`
  branch by default. Do not create `codex/*` or other task branches unless the
  maintainer explicitly requests a new branch.
- Release tags should be created only after version, changelog, CI, and release
  checks are aligned.

## Support / GPT Access

- GPT access and promotion link: <https://shop.input.im/?code=fbe6f3d5>
- Sponsor QR codes are kept below.

## Sponsor

If this project helps your work, you can support ongoing maintenance via the
following support QR codes.

<p align="center">
  <img src="sponsor/zhifubao.jpg" alt="Alipay QR Code" width="220" />
  <img src="sponsor/weixin.png" alt="WeChat Appreciation QR Code" width="220" />
</p>
<p align="center">
  <sub>Left: Alipay. Right: WeChat Appreciation.</sub>
</p>

## License

This fork should be described as source-available rather than open source for
its fork-specific modifications. See [`LICENSE`](LICENSE),
[`LICENSE.upstream-mit`](LICENSE.upstream-mit), and [`NOTICE`](NOTICE).
