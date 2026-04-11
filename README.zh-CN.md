# FeatherDoc

[![Windows MSVC CI](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/windows-msvc.yml/badge.svg?branch=master)](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/windows-msvc.yml)

[English](README.md) | 简体中文

FeatherDoc 是一个面向现代 C++ 的 Microsoft Word `.docx` 读写与编辑库。

它当前重点覆盖：

- 现代 CMake / C++20 工程化集成
- 对 MSVC 友好的构建、测试与安装导出链路
- 段落、Run、表格、图片、列表、页眉页脚与模板部件的轻量级编辑 API
- 对既有 `.docx` 的 reopen-after-save 持续编辑能力
- 基于真实 Microsoft Word 渲染结果的截图级可视化验证

> 这份文件提供中文入口、构建方式、验证流程和项目级说明。  
> 更完整的 API 细节、更多可运行 sample 和边界行为说明，仍以
> [README.md](README.md) 和 `docs/index.rst` 为准。

## 亮点

- CMake 3.20+
- C++20
- 默认支持 MSVC / Windows 构建
- 顶层构建默认启用 `featherdoc_cli`
- 已覆盖 fixed-grid 表格的 `merge_right()` / `merge_down()` /
  `unmerge_right()` / `unmerge_down()` 宽度闭环验证
- 提供 Word 截图级 smoke / release gate / AI review task 打包脚本

## 构建

```bash
cmake -S . -B build
cmake --build build
```

顶层构建默认开启 `BUILD_CLI`，除非显式传入 `-DBUILD_CLI=OFF`，
否则会同时构建 `featherdoc_cli`。

## MSVC 构建

请先打开 `x64` 的 Visual Studio Developer Command Prompt，或先执行：

```bat
VsDevCmd.bat -arch=x64 -host_arch=x64
```

然后运行：

```bat
cmake -S . -B build-msvc-nmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DBUILD_SAMPLES=ON -DBUILD_CLI=ON
cmake --build build-msvc-nmake
ctest --test-dir build-msvc-nmake --output-on-failure --timeout 60
```

## 发布前总检查

如果你想在 Windows 上一次性跑完整的发布前检查，直接执行：

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1
```

这个总控脚本会串联：

- MSVC build / test
- `cmake --install` + `find_package(FeatherDoc)` smoke
- Word visual release gate

脚本结束后，输出目录里会生成：

- `START_HERE.md`
- `report/ARTIFACT_GUIDE.md`
- `report/REVIEWER_CHECKLIST.md`
- `report/release_handoff.md`
- `report/release_body.zh-CN.md`
- `report/release_summary.zh-CN.md`

如果截图级 review 结论是在后续补写的，优先执行最短同步命令，把最终
visual verdict 一次性回写到 gate summary、release summary 和 release note
bundle，而不是整条 preflight 重新跑一遍：

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1
```

如果你需要手动覆盖自动推断出的 gate / release 路径，则改用：

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\sync_visual_review_verdict.ps1 `
    -GateSummaryJson .\output\word-visual-release-gate\report\gate_summary.json
```

## Word 可视化验证

如果只想跑基础 Word 冒烟检查：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1
```

如果只想检查 fixed-grid merge / unmerge 四件套：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_fixed_grid_merge_unmerge_regression.ps1
```

如果希望在证据生成后，立即打包成 AI 可消费的 review task：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_fixed_grid_merge_unmerge_regression.ps1 `
    -PrepareReviewTask `
    -ReviewMode review-only
```

如果你已经有一个目标 `.docx`，想把它打包成稳定的 AI 复核任务目录：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\prepare_word_review_task.ps1 `
    -DocxPath C:\path\to\target.docx `
    -Mode review-only
```

任务目录会带上：

- `task_prompt.md`
- `task_manifest.json`
- `evidence/`
- `report/`
- `repair/`

并维护 `output/word-visual-smoke/tasks/` 下的 latest pointer，
方便自动化消费最新任务。

## 渲染示例

下面这些图都来自当前验证流程的真实 Word 渲染证据，直接保存在仓库里。
相比旧式 sample 截图，这一组图更直观地展示了 FeatherDoc 当前的能力面：
既能看到 fixed-grid 宽度闭环，也能看到中文业务模板的最终输出效果。

<p align="center">
  <img src="docs/assets/readme/visual-smoke-contact-sheet.png" alt="完整的 Word visual smoke 联系图" width="900" />
</p>
<p align="center">
  <sub>上图：当前 6 页 Word visual smoke 联系图，覆盖表格、分页、合并/拆分、文字方向、fixed-grid 列宽编辑以及 RTL/LTR/CJK 混排检查。</sub>
</p>
<p align="center">
  <img src="docs/assets/readme/fixed-grid-merge-right-page-01.png" alt="Word 渲染的 merge_right 固定网格样例页" width="200" />
  <img src="docs/assets/readme/fixed-grid-merge-down-page-01.png" alt="Word 渲染的 merge_down 固定网格样例页" width="200" />
  <img src="docs/assets/readme/fixed-grid-aggregate-contact-sheet.png" alt="fixed-grid merge 和 unmerge 回归联系图" width="200" />
  <img src="docs/assets/readme/sample-chinese-template-page-01.png" alt="Word 渲染的中文报价单模板样例页" width="200" />
</p>
<p align="center">
  <sub>下排从左到右：单功能 <code>merge_right()</code> 宽度闭环样例、单功能 <code>merge_down()</code> 宽度闭环样例、fixed-grid 四件套聚合签收图，以及中文报价单模板样例。前两张图分别来自 <code>sample_merge_right_fixed_grid.cpp</code> 和 <code>sample_merge_down_fixed_grid.cpp</code>，不需要读 XML 就能直接看出宽度是否收敛正确。最右图来自 <code>sample_chinese_template.cpp</code>，展示 CJK 字体元数据、表格布局和业务文档输出在 Word 里的真实结果。</sub>
</p>

如果你要重建这一组聚焦展示图，先构建并运行独立 sample。若你的生成器把
可执行文件放在别的位置，请把下面的可执行路径替换成自己的构建产物路径：

```powershell
cmake --build build-msvc-nmake --target `
    featherdoc_sample_merge_right_fixed_grid `
    featherdoc_sample_merge_down_fixed_grid `
    featherdoc_sample_chinese_template

.\build-msvc-nmake\featherdoc_sample_merge_right_fixed_grid.exe `
    .\output\sample-merge-right-fixed-grid\merge_right_fixed_grid.docx

.\build-msvc-nmake\featherdoc_sample_merge_down_fixed_grid.exe `
    .\output\sample-merge-down-fixed-grid\merge_down_fixed_grid.docx

.\build-msvc-nmake\featherdoc_sample_chinese_template.exe `
    .\samples\chinese_invoice_template.docx `
    .\output\sample-chinese-template\sample_chinese_invoice_output.docx

powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 `
    -InputDocx .\output\sample-merge-right-fixed-grid\merge_right_fixed_grid.docx `
    -OutputDir .\output\word-visual-sample-merge-right-fixed-grid

powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 `
    -InputDocx .\output\sample-merge-down-fixed-grid\merge_down_fixed_grid.docx `
    -OutputDir .\output\word-visual-sample-merge-down-fixed-grid

powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 `
    -InputDocx .\output\sample-chinese-template\sample_chinese_invoice_output.docx `
    -OutputDir .\output\word-visual-sample-chinese-template
```

如果你还要重建 fixed-grid 四件套聚合签收图并同时生成 review task：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_fixed_grid_merge_unmerge_regression.ps1 `
    -PrepareReviewTask `
    -ReviewMode review-only
```

要把这些渲染结果同步回仓库里的 README 预览 PNG，执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\refresh_readme_visual_assets.ps1
```

要从这些截图跳转到完整复现命令、review task 或发布前验证流程，建议先看：

- [VISUAL_VALIDATION_QUICKSTART.md](VISUAL_VALIDATION_QUICKSTART.md)
- [VISUAL_VALIDATION.md](VISUAL_VALIDATION.md)
- [VISUAL_VALIDATION.zh-CN.md](VISUAL_VALIDATION.zh-CN.md)

## CLI

`featherdoc_cli` 当前主要覆盖分节感知的页眉 / 页脚检查与编辑流程。

```bash
featherdoc_cli inspect-sections input.docx
featherdoc_cli inspect-sections input.docx --json
featherdoc_cli inspect-header-parts input.docx --json
featherdoc_cli inspect-footer-parts input.docx
featherdoc_cli insert-section input.docx 1 --no-inherit --output inserted.docx --json
featherdoc_cli copy-section-layout input.docx 0 2 --output copied.docx
featherdoc_cli move-section input.docx 2 0 --output reordered.docx
featherdoc_cli remove-section input.docx 3 --output trimmed.docx
featherdoc_cli assign-section-header input.docx 2 0 --kind even --output shared-header.docx --json
featherdoc_cli assign-section-footer input.docx 2 1 --output shared-footer.docx --json
featherdoc_cli remove-section-header input.docx 2 --kind even --output detached-header.docx
featherdoc_cli remove-section-footer input.docx 1 --kind first --output detached-footer.docx
featherdoc_cli remove-header-part input.docx 1 --output headers-pruned.docx
featherdoc_cli remove-footer-part input.docx 1 --output footers-pruned.docx
```

更完整的命令列表和字段说明请看 [README.md](README.md) 里的 `CLI` 章节。

## 安装

```bash
cmake --install build --prefix install
```

安装产物中的 `share/FeatherDoc` 会携带：

- `README.md` / `README.zh-CN.md`
- `VISUAL_VALIDATION*.md`
- `RELEASE_ARTIFACT_TEMPLATE*.md`
- `visual-validation/*.png`
- `CHANGELOG.md`
- `LICENSE` / `LICENSE.upstream-mit`
- `NOTICE` / `LEGAL.md`

## 通过 CMake 使用

```cmake
find_package(FeatherDoc CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE FeatherDoc::FeatherDoc)
```

如果你想验证安装导出链路是否健康，直接跑本仓库里的发布前 smoke，
它已经覆盖了 `find_package(FeatherDoc CONFIG REQUIRED)` 的最小消费者工程。

## 快速开始

下面这个例子打开现有 `.docx`，遍历正文段落和表格里的文本：

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

补充说明：

- `Run` 表示 WordprocessingML 的文本运行块，不等于“整行文本”
- 如果一个视觉上的自然行被拆成多个 run，需要先把 `run.get_text()` 拼起来
- 表格内文本需要通过 `doc.tables() -> rows() -> cells() -> paragraphs()` 访问

更完整的 API、更多可运行 sample 和复杂表格 / 模板 / 图片流程，请参考
[README.md](README.md) 和 `docs/index.rst`。

## 当前能力范围

当前 FeatherDoc 已覆盖以下高价值能力：

- 读取与保存既有 `.docx`
- 段落与 Run 的增删改
- 表格创建、插行、插列、删行、删列、合并、拆分、列宽与 fixed-grid 编辑
- 书签填充、模板表格扩展、条件块显隐
- 内联图片与浮动图片
- 页眉、页脚、分节复制 / 插入 / 移动 / 删除
- 列表、基础样式引用和样式 look 编辑

## 当前限制

- 不支持加密或受密码保护的 `.docx`
- 还没有高层公式（OMML）typed API
- 暂无高层的自定义表格样式定义编辑
- 暂无完整的样式目录检查 / 继承感知样式管理 API

更细的限制列表请看 [README.md](README.md) 里的 `Current Limitations`。

## 文档入口

仓库里当前的主要文档入口有：

- 英文主 README：[README.md](README.md)
- 中文 README：[README.zh-CN.md](README.zh-CN.md)
- Sphinx 文档首页：`docs/index.rst`
- 项目定位：`docs/project_identity_zh.rst`
- 项目审计记录：`docs/project_audit_zh.rst`
- 版本与发布策略：`docs/release_policy_zh.rst`
- Word 可视化工作流：`docs/automation/word_visual_workflow_zh.rst`
- 许可说明：`docs/licensing_zh.rst`
- 仓库法律说明：`LEGAL.md`

## 项目方向

FeatherDoc 现在应被视为一个持续演进的独立分支，而不是对历史上游项目的被动镜像。

- 现代 C++ 与更清晰的 API 语义优先
- MSVC 可构建性是正式支持目标
- 错误诊断、`open()` / `save()` 行为和核心路径性能是一等公民
- 文档、仓库元数据、许可与验证流程都按当前项目方向维护

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

这个 fork 的 fork-specific 修改部分应描述为 source-available，
而不是传统意义上的 open source。

- FeatherDoc fork-specific 修改遵循 `LICENSE` 中的非商用 source-available 条款
- 继承自上游 DuckX 的部分仍保留 `LICENSE.upstream-mit`
- 第三方依赖继续遵循各自原始许可证
- 中文阅读指引见 `docs/licensing_zh.rst`
- 仓库级法律摘要见 `LEGAL.md` 和 `NOTICE`
