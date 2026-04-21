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
bundle。这个同步流程现在也会自动吸收同一 task root 下的 curated visual
bundle task，而不是只处理最早那几条固定链路，因此不需要为了补写 bundle
结论重新跑整条 preflight：

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1
```

如果你需要手动覆盖自动推断出的 gate / release 路径，则改用：

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\sync_visual_review_verdict.ps1 `
    -GateSummaryJson .\output\word-visual-release-gate\report\gate_summary.json
```

## Word 可视化验证

如果你想验证模板表格 CLI 在 `--bookmark` 路径下的真实页面效果，可运行：

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\run_template_table_cli_bookmark_visual_regression.ps1
```

这个回归会生成一个仅包含正文的 baseline 文档，其中有一个保留表格，
以及一个同时支持“表前书签”和“表内书签”定位的目标表格；随后分别通过
`set-template-table-row-texts` 与
`set-template-table-cell-block-texts` 的 `--bookmark` 形式做变更，再导出
Word 渲染前后截图和 contact sheet，输出到
`output/template-table-cli-bookmark-visual-regression/`。

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
    -DocxPath .\path\to\target.docx `
    -Mode review-only
```

任务目录会带上：

- `task_prompt.md`
- `task_manifest.json`
- `evidence/`
- `report/`
- `repair/`

并维护 `output/word-visual-smoke/tasks/` 下的 latest pointer，
方便自动化消费最新任务，例如：

- `latest_fixed-grid-regression-bundle_task.json`
- `latest_template-table-cli-selector-visual-regression-bundle_task.json`

如果你不想自己解析这些稳定指针，可以直接运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\open_latest_word_review_task.ps1

powershell -ExecutionPolicy Bypass -File .\scripts\open_latest_word_review_task.ps1 `
    -SourceKind template-table-cli-selector-visual-regression-bundle `
    -PrintPrompt
```

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

`featherdoc_cli` 当前已经覆盖分节、样式、编号、页面设置、书签、图片和模板
部件等多类检查与编辑流程。

```bash
featherdoc_cli inspect-sections input.docx
featherdoc_cli inspect-sections input.docx --json
featherdoc_cli inspect-styles input.docx --style Strong --json
featherdoc_cli inspect-runs input.docx 1 --run 0 --json
featherdoc_cli inspect-template-runs input.docx 1 --run 0 --json
featherdoc_cli inspect-numbering input.docx --definition 1 --json
featherdoc_cli inspect-page-setup input.docx --section 1 --json
featherdoc_cli inspect-bookmarks input.docx --part header --index 0 --bookmark header_rows --json
featherdoc_cli inspect-images input.docx --relationship-id rId5 --json
featherdoc_cli ensure-table-style input.docx ReportTable --name "Report Table" --based-on TableGrid --output styled.docx --json
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
featherdoc_cli remove-footer-part input.docx 0 --output footers-pruned.docx
featherdoc_cli append-page-number-field input.docx --part section-header --section 1 --output page-number.docx --json
featherdoc_cli set-template-table-from-json report.docx --bookmark line_items_table --patch-file row_patch.json --output report-updated.docx --json
featherdoc_cli set-template-tables-from-json report.docx --patch-file multi_table_patch.json --output report-updated.docx --json
featherdoc_cli validate-template input.docx --part body --slot customer:text --slot line_items:table_rows --json
```

上面的命令块是代表性示例，不是完整 CLI 清单。更完整的命令列表和字
段说明请优先看 `docs/index.rst`；如果你同时需要仓库级背景说明，再配合
[README.md](README.md) 一起看会更顺。

补充命令族示例：

```bash
# 段落、run、样式、编号
featherdoc_cli inspect-paragraphs input.docx --paragraph 4 --json
featherdoc_cli set-paragraph-style input.docx 4 Heading2 --output styled-paragraph.docx --json
featherdoc_cli clear-paragraph-style input.docx 4 --output cleared-paragraph-style.docx --json
featherdoc_cli set-run-style input.docx 4 1 Strong --output styled-run.docx --json
featherdoc_cli clear-run-style input.docx 4 1 --output cleared-run-style.docx --json
featherdoc_cli set-run-font-family input.docx 4 1 Consolas --output font-run.docx --json
featherdoc_cli clear-run-font-family input.docx 4 1 --output cleared-run-font.docx --json
featherdoc_cli set-run-language input.docx 4 1 en-US --output language-run.docx --json
featherdoc_cli clear-run-language input.docx 4 1 --output cleared-run-language.docx --json
featherdoc_cli ensure-paragraph-style input.docx ReviewHeading --name "Review Heading" --based-on Heading1 --output ensured-paragraph-style.docx --json
featherdoc_cli ensure-character-style input.docx ReviewStrong --name "Review Strong" --based-on Strong --output ensured-character-style.docx --json
featherdoc_cli ensure-numbering-definition input.docx --definition-name OutlineReview --numbering-level 0:decimal:1:%1. --output numbering.docx --json
featherdoc_cli set-paragraph-numbering input.docx 6 --definition 12 --level 0 --output numbered.docx --json
featherdoc_cli set-paragraph-style-numbering input.docx Heading2 --definition-name HeadingReview --numbering-level 0:decimal:1:%1. --style-level 1 --output style-numbering.docx --json
featherdoc_cli clear-paragraph-style-numbering input.docx Heading2 --output cleared-style-numbering.docx --json
featherdoc_cli set-paragraph-list input.docx 6 --kind bullet --level 1 --output bulleted.docx --json
featherdoc_cli restart-paragraph-list input.docx 10 --kind decimal --level 0 --output restarted-list.docx --json
featherdoc_cli clear-paragraph-list input.docx 10 --output cleared-list.docx --json

# 正文表格与行列元数据
featherdoc_cli inspect-tables input.docx --table 0 --json
featherdoc_cli inspect-table-cells input.docx 0 --row 1 --cell 1 --json
featherdoc_cli inspect-table-rows input.docx 0 --row 1 --json
featherdoc_cli set-table-cell-text input.docx 0 1 1 --text "Updated" --output cell-text.docx --json
featherdoc_cli set-table-cell-fill input.docx 0 1 1 FFE699 --output cell-fill.docx --json
featherdoc_cli clear-table-cell-fill input.docx 0 1 1 --output cell-fill-cleared.docx --json
featherdoc_cli set-table-cell-vertical-alignment input.docx 0 1 1 center --output cell-align.docx --json
featherdoc_cli clear-table-cell-vertical-alignment input.docx 0 1 1 --output cell-align-cleared.docx --json
featherdoc_cli set-table-cell-text-direction input.docx 0 1 1 top_to_bottom_right_to_left --output cell-text-direction.docx --json
featherdoc_cli clear-table-cell-text-direction input.docx 0 1 1 --output cell-text-direction-cleared.docx --json
featherdoc_cli set-table-cell-width input.docx 0 1 1 2400 --output cell-width.docx --json
featherdoc_cli clear-table-cell-width input.docx 0 1 1 --output cell-width-cleared.docx --json
featherdoc_cli set-table-cell-margin input.docx 0 1 1 left 160 --output cell-margin.docx --json
featherdoc_cli clear-table-cell-margin input.docx 0 1 1 left --output cell-margin-cleared.docx --json
featherdoc_cli set-table-cell-border input.docx 0 1 1 right --style single --size 8 --color FF0000 --output cell-border.docx --json
featherdoc_cli clear-table-cell-border input.docx 0 1 1 right --output cell-border-cleared.docx --json
featherdoc_cli set-table-row-height input.docx 0 1 720 exact --output row-height.docx --json
featherdoc_cli clear-table-row-height input.docx 0 1 --output row-height-cleared.docx --json
featherdoc_cli set-table-row-cant-split input.docx 0 1 --output row-cant-split.docx --json
featherdoc_cli clear-table-row-cant-split input.docx 0 1 --output row-cant-split-cleared.docx --json
featherdoc_cli set-table-row-repeat-header input.docx 0 0 --output row-repeat-header.docx --json
featherdoc_cli clear-table-row-repeat-header input.docx 0 0 --output row-repeat-header-cleared.docx --json
featherdoc_cli append-table-row input.docx 0 --cell-count 3 --output row-appended.docx --json
featherdoc_cli insert-table-row-before input.docx 0 1 --output row-inserted-before.docx --json
featherdoc_cli insert-table-row-after input.docx 0 1 --output row-inserted-after.docx --json
featherdoc_cli remove-table-row input.docx 0 1 --output row-removed.docx --json
featherdoc_cli merge-table-cells input.docx 0 0 0 --direction right --count 2 --output merged.docx --json
featherdoc_cli unmerge-table-cells input.docx 0 0 0 --direction right --output unmerged.docx --json

# 模板部件、模板表格、书签、图片、页码字段
featherdoc_cli inspect-template-paragraphs input.docx --part header --index 0 --paragraph 0 --json
featherdoc_cli inspect-template-tables input.docx --part body --table 0 --json
featherdoc_cli inspect-template-table-rows input.docx 0 --row 1 --json
featherdoc_cli inspect-template-table-cells input.docx 0 --row 1 --cell 1 --json
featherdoc_cli set-template-table-row-texts input.docx 0 1 --row "SKU-1" --cell "2" --cell "$10" --output template-row-texts.docx --json
featherdoc_cli set-template-table-cell-block-texts input.docx 0 1 0 --row "Header" --cell "Line A" --cell "Line B" --output template-block-texts.docx --json
featherdoc_cli insert-template-table-column-before input.docx 0 1 1 --output template-column-before.docx --json
featherdoc_cli insert-template-table-column-after input.docx 0 1 1 --output template-column-after.docx --json
featherdoc_cli remove-template-table-column input.docx 0 1 1 --output template-column-removed.docx --json
featherdoc_cli append-template-table-row input.docx 0 --cell-count 3 --output template-row-appended.docx --json
featherdoc_cli insert-template-table-row-before input.docx 0 1 --output template-row-before.docx --json
featherdoc_cli insert-template-table-row-after input.docx 0 1 --output template-row-after.docx --json
featherdoc_cli remove-template-table-row input.docx 0 1 --output template-row-removed.docx --json
featherdoc_cli merge-template-table-cells input.docx 0 1 0 --direction right --count 2 --output template-merged.docx --json
featherdoc_cli unmerge-template-table-cells input.docx 0 1 0 --direction right --output template-unmerged.docx --json
featherdoc_cli replace-bookmark-text input.docx customer_name --text "Ada Lovelace" --output bookmark-text.docx --json
featherdoc_cli fill-bookmarks input.docx --set customer_name "Ada Lovelace" --set invoice_no INV-001 --output filled.docx --json
featherdoc_cli replace-bookmark-paragraphs input.docx notes --paragraph "Line one" --paragraph "Line two" --output bookmark-paragraphs.docx --json
featherdoc_cli replace-bookmark-table input.docx line_items --row "SKU-1" --cell "2" --cell "$10" --output bookmark-table.docx --json
featherdoc_cli replace-bookmark-table-rows input.docx line_items --row "SKU-2" --cell "4" --cell "$20" --output bookmark-table-rows.docx --json
featherdoc_cli remove-bookmark-block input.docx optional_section --output bookmark-block-removed.docx --json
featherdoc_cli set-bookmark-block-visibility input.docx optional_section --visible false --output bookmark-hidden.docx --json
featherdoc_cli apply-bookmark-block-visibility input.docx --hide optional_section --show totals --output bookmark-visibility.docx --json
featherdoc_cli replace-bookmark-image input.docx logo assets/logo.png --width 120 --height 40 --output bookmark-image.docx --json
featherdoc_cli replace-bookmark-floating-image input.docx hero assets/hero.png --width 320 --height 180 --horizontal-reference margin --vertical-reference paragraph --wrap-mode square --output bookmark-floating-image.docx --json
featherdoc_cli extract-image input.docx exported.png --relationship-id rId5 --json
featherdoc_cli replace-image input.docx replacement.png --relationship-id rId5 --output image-replaced.docx --json
featherdoc_cli remove-image input.docx --relationship-id rId5 --output image-removed.docx --json
featherdoc_cli append-image input.docx badge.png --width 96 --height 48 --output image-appended.docx --json
featherdoc_cli show-section-header input.docx 1 --kind even
featherdoc_cli show-section-footer input.docx 2 --json
featherdoc_cli set-section-header input.docx 2 --kind even --text-file header.txt --json
featherdoc_cli set-section-footer input.docx 0 --text "Page 1" --output footer.docx --json
featherdoc_cli append-total-pages-field input.docx --part section-footer --section 1 --kind first --output total-pages.docx --json
```

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

## 按场景找 API

如果你主要从中文 README 进入仓库，建议先按“你要解决什么问题”来找入口：

- 新建、打开、保存、排错：`create_empty()`、`open()`、`save()`、`save_as()`、`path()`、`last_error()`
- 编辑正文文本：`paragraphs()`、`runs()`、`set_text()`、段落 / run 的插入与删除
- 处理表格与版式：`append_table()`、插行 / 插列、`merge_right()`、`merge_down()`、`unmerge_*()`、列宽 / fixed-grid / 布局模式相关 API
- 做模板填充或书签校验：`list_bookmarks()`、`validate_template()`、`fill_bookmarks()`、`replace_bookmark_with_*()`、`body_template()` / `header_template()` / `footer_template()`
- 处理图片和页码字段：`append_image()`、`append_floating_image()`、`replace_*image()`、`append_page_number_field()`、`append_total_pages_field()`
- 处理分节、页眉页脚和页面设置：`inspect_sections()`、`get_section_page_setup()`、`set_section_page_setup()`、`ensure_*header*()`、`ensure_*footer*()`、`append_section()`、`insert_section()`、`move_section()`
- 处理样式、编号和语言元数据：`list_styles()`、`find_style()`、`ensure_*style()`、`ensure_numbering_definition()`、`set_paragraph_style_numbering()`
- 想做脚本化检查或一次性改写：优先看 CLI 的 `inspect-*`、`validate-template`、`append-page-number-field`、`set-section-page-setup`

如果你需要更完整的参数说明、可运行 sample 对照和边界行为说明，继续看
`docs/index.rst` 会更高效；它现在也补了按任务分组的 sample / CLI 导航。
英文 README 适合作为仓库级总入口。

## 按书签定位某一页的表格

如果你的真实需求是“改文档里某一页的某张表”，不要依赖渲染后的页码去反推
DOCX 结构位置。更稳的做法是：

1. 在目标表**内部**或目标表**前一个独立段落**放一个书签
2. CLI 用 `--bookmark <name>` 代替脆弱的 `<table-index>`
3. C++ 里先拿 `TemplatePart`，再用 `find_table_by_bookmark(...)`
   直接拿可编辑的 `Table`
4. 拿到 `Table` 之后，优先用 `find_row(...)`、`find_cell(...)`、
   `set_cell_text(...)`、`set_row_texts(...)` 这类按索引直达的入口，
   或者直接用 `set_rows_texts(...)`、`set_cell_block_texts(...)`
   一次性覆盖多行 / 一个矩形块，不需要再自己手写 `next()` 循环

CLI 示例：

```bash
featherdoc_cli inspect-template-table-rows report.docx --bookmark page3_target_table --json
featherdoc_cli set-template-table-cell-text report.docx --bookmark page3_target_table 1 2 --text "更新后的内容" --output report-updated.docx --json
featherdoc_cli set-template-table-row-texts report-updated.docx --bookmark page3_target_table 3 --row "商品A" --cell "3" --cell "99.00" --row "商品B" --cell "1" --cell "18.00" --output report-updated.docx --json
featherdoc_cli set-template-table-cell-block-texts report-updated.docx --bookmark page3_target_table 3 1 --row "华北" --cell "120" --row "华南" --cell "98" --output report-updated.docx --json
featherdoc_cli append-template-table-row report-updated.docx --bookmark page3_target_table --output report-updated.docx --json
```

C++ 示例：

```cpp
featherdoc::Document doc("report.docx");
doc.open();

auto part = doc.body_template();
auto table = part.find_table_by_bookmark("page3_target_table");
if (!table.has_value()) {
    throw std::runtime_error(doc.last_error().detail);
}

table->set_cell_text(1, 2, "更新后的内容");
table->set_row_texts(2, {"商品A", "3", "99.00"});
table->set_rows_texts(3, {{"商品B", "1", "18.00"}, {"商品C", "5", "7.50"}});

doc.save_as("report-updated.docx");
```

如果目标表在页眉 / 页脚 / 分节页眉页脚里，就把 `body_template()` 换成
`header_template()`、`footer_template()`、`section_header_template()` 或
`section_footer_template()`。

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
