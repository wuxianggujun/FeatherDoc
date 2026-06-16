# FeatherDoc

[![Windows MSVC CI](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/windows-msvc.yml/badge.svg?branch=dev)](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/windows-msvc.yml)
[![Linux CMake CI](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/linux-cmake.yml/badge.svg?branch=dev)](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/linux-cmake.yml)
[![macOS CMake CI](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/macos-cmake.yml/badge.svg?branch=dev)](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/macos-cmake.yml)

[English](README.md) | 简体中文

FeatherDoc 是一个 C++20 `.docx` 文档库，用于创建、读取、编辑、检查和重写
Microsoft Word 文档。

这个 README 只保留仓库入口。详细 API、功能说明、发布治理、PDF 研究和可视化验证
都放到文档站里维护，避免 README 继续变成一整篇大文档。

## 文档入口

- 中文文档：<https://wuxianggujun.github.io/FeatherDoc/zh-CN/>
- 英文文档：<https://wuxianggujun.github.io/FeatherDoc/en/>
- 中文 API 参考：<https://wuxianggujun.github.io/FeatherDoc/zh-CN/api/>
- 英文 API 参考：<https://wuxianggujun.github.io/FeatherDoc/en/api/>
- 中文快速开始：<https://wuxianggujun.github.io/FeatherDoc/zh-CN/getting_started.html>
- 英文快速开始：<https://wuxianggujun.github.io/FeatherDoc/en/getting_started.html>
- Word 文档处理工作流：<https://wuxianggujun.github.io/FeatherDoc/zh-CN/word_document_workflow.html>
- PDF 工作流：<https://wuxianggujun.github.io/FeatherDoc/zh-CN/api/pdf_workflow.html>
- 文档源码：[`docs/index.rst`](docs/index.rst)
- 文档维护：[`docs/documentation_maintenance_zh.rst`](docs/documentation_maintenance_zh.rst)
- 脚本任务索引：[`docs/script_task_index_zh.rst`](docs/script_task_index_zh.rst)

## 可以用来做什么

- 创建和编辑 `.docx` 文档包。
- 编辑段落、run、表格、行、单元格、样式、编号、节、页眉和页脚。
- 按书签、content control tag 或 alias 填充模板。
- 将模板槽位替换为文本、段落、表格、表格行和图片。
- 检查字段、超链接、批注、脚注、尾注、修订、图片、样式、编号和语义差异。
- 使用 `featherdoc_cli` 做脚本化检查和一次性改写。

## 快速开始

```bash
cmake -S . -B build
cmake --build build
cmake --install build --prefix install
```

运行项目模板 smoke 路径，验证中文发票模板、按业务 JSON 渲染文档，
并写出审查产物：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_project_template_smoke.ps1 `
  -ManifestPath .\samples\project_template_smoke.manifest.json `
  -BuildDir .\build `
  -OutputDir .\output\project-template-smoke `
  -SkipBuild `
  -SkipVisualSmoke
```

最小 C++ 示例：

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

## 分支策略

- `dev` 是当前主开发分支。
- `master` 保留为稳定/发布分支。
- Codex / 本地自动化默认直接在当前 `dev` 分支推进；除非维护者明确要求，
  不要主动创建 `codex/*` 或其它任务分支。
- 只有版本号、CHANGELOG、CI 和 release checks 对齐后，才创建正式 release tag。

## GPT 推广链接

GPT 访问与推广链接：<https://shop.input.im/?code=fbe6f3d5>

## 赞助

如果这个项目对你的工作有帮助，可以通过下列收款码支持持续维护。

<p align="center">
  <img src="sponsor/zhifubao.jpg" alt="支付宝收款码" width="220" />
  <img src="sponsor/weixin.png" alt="微信赞赏码" width="220" />
</p>
<p align="center">
  <sub>左：支付宝；右：微信赞赏。</sub>
</p>

## 许可

这个 fork 的新增修改应描述为 source-available，而不是完整开源。详见
[`LICENSE`](LICENSE)、[`LICENSE.upstream-mit`](LICENSE.upstream-mit) 和
[`NOTICE`](NOTICE)。
