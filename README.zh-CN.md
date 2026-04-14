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

如果想把 section page setup 的 sample 输出和 CLI 改写链路一起回归：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_section_page_setup_regression.ps1
```

如果想专门回归 `PAGE` / `NUMPAGES` 字段的 Word 渲染结果：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_page_number_fields_regression.ps1
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
方便自动化消费最新任务。

`run_section_page_setup_regression.ps1` 会同时构建
`featherdoc_sample_section_page_setup` 和 `featherdoc_cli`，在
`output/section-page-setup-regression/` 下产出 `api-sample` 与
`cli-rewrite` 两组结果。除了 Word 渲染证据，它还会把每组 case 的
CLI inspection JSON 一起落盘，方便对照“机器读到的页面设置”和
“Word 最终渲染出来的版式”是否一致。

`run_page_number_fields_regression.ps1` 会同时构建
`featherdoc_sample_page_number_fields`、
`featherdoc_sample_section_page_setup` 和 `featherdoc_cli`，在
`output/page-number-fields-regression/` 下产出 `api-sample` 与
`cli-append` 两组结果。每组 case 还会额外落一份 `field_summary.json`，
方便对照 Word 最终渲染出来的页码字段和 DOCX 中检测到的
`PAGE` / `NUMPAGES` 计数是否一致。

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
featherdoc_cli inspect-styles input.docx
featherdoc_cli inspect-styles input.docx --style Strong --json
featherdoc_cli inspect-styles input.docx --style Strong --usage
featherdoc_cli inspect-numbering input.docx
featherdoc_cli inspect-numbering input.docx --instance 1
featherdoc_cli inspect-numbering input.docx --definition 1 --json
featherdoc_cli inspect-page-setup input.docx
featherdoc_cli inspect-page-setup input.docx --section 1 --json
featherdoc_cli set-section-page-setup input.docx 1 --orientation landscape --width 15840 --height 12240 --margin-top 720 --output rotated.docx --json
featherdoc_cli inspect-bookmarks input.docx
featherdoc_cli inspect-bookmarks input.docx --part header --index 0 --bookmark header_rows --json
featherdoc_cli inspect-images input.docx
featherdoc_cli inspect-images input.docx --relationship-id rId5 --image-entry-name word/media/image1.png --json
featherdoc_cli inspect-images input.docx --part header --index 0 --image 0 --json
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
featherdoc_cli append-page-number-field input.docx --part section-header --section 1 --output page-number.docx --json
featherdoc_cli append-total-pages-field input.docx --part section-footer --section 1 --kind first --output total-pages.docx --json
featherdoc_cli validate-template input.docx --part body --slot customer:text --slot line_items:table_rows --json
featherdoc_cli validate-template input.docx --part header --index 0 --slot header_title:text --slot header_rows:table_rows --json
featherdoc_cli validate-template input.docx --part section-footer --section 1 --kind first --slot footer_company:text --slot footer_note:block:optional
```

`inspect-page-setup` 用来只读检查 section 上显式声明的纸张尺寸、方向、
页边距和起始页码；`set-section-page-setup` 用来直接写回这些字段。它支持
部分更新：如果目标 section 本来就有显式 page setup，没有传入的字段会保留原值；
如果目标还是隐式 final section，则第一次写入时需要把几何字段补齐，CLI 会自动
物化可编辑的 `w:sectPr` / `w:pgSz` / `w:pgMar` 节点。

`append-page-number-field` / `append-total-pages-field` 用来往目标模板部件
追加 Word 的 `PAGE` / `NUMPAGES` 简单字段。它们沿用
`inspect-bookmarks` / `validate-template` 的目标选择方式：
`--part body|header|footer|section-header|section-footer`，普通页眉页脚用
`--index`，分节页眉页脚用 `--section` 和可选的
`--kind default|first|even`。如果目标页眉/页脚部件还不存在，CLI 会先自动
物化可写部件，再追加字段；配合 `--json` 可以拿到解析后的 `part`、
`part_index`、`section`、`kind`、`entry_name` 和 `field`。

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
- 内联图片与浮动图片（支持基础包围方式、包围距离和裁剪）
- 页眉、页脚、分节复制 / 插入 / 移动 / 删除
- 列表、基础样式引用、样式目录检查、最小样式定义和样式 look 编辑

## 当前限制

- 不支持加密或受密码保护的 `.docx`
- 还没有高层公式（OMML）typed API
- 暂无高级表格样式属性编辑和浮动表格定位 API
- 暂无继承感知的样式重构 / 更高层的 numbering-style 抽象 API

更细的限制列表请看 [README.md](README.md) 里的 `Current Limitations`。

## 样式目录检查

现在可以通过 `list_styles()` 和 `find_style()` 读取当前文档的样式目录摘要，
包括样式类型、显示名、`basedOn`、默认 / 自定义 / 半隐藏 / 快速样式等标记。
如果源文档里还没有 `word/styles.xml`，`create_empty()` 生成的默认样式目录也可以直接读取。
当段落样式本身已经带有编号元数据时，`style_summary::numbering` 现在也会直接带出
解析后的定义字段和对应的编号 `instance` 摘要。

CLI 现在也提供同样的只读入口：

```bash
featherdoc_cli inspect-styles report.docx
featherdoc_cli inspect-styles report.docx --style BodyText --json
featherdoc_cli inspect-styles report.docx --style BodyText --usage
```

如果段落样式上带有 `w:numPr`，`inspect-styles` 还会继续解析它引用的编号实例，
JSON 输出里会附带具体的 `instance` 摘要，文本输出也会显示 override 数量。
当你对单个样式额外传入 `--usage` 时，CLI 还会统计
`word/document.xml`、页眉和页脚中引用该样式的段落、运行和表格数量。
JSON 和文本输出都会同时给出总计，以及 `body`、`header`、`footer`
三段聚合明细。
此外还会给出 `hits` 清单，用来标明每次命中来自哪个 part，以及它在该 part
里的文档顺序编号。正文命中现在也会直接在 `hits[*].section_index`
里给出所属 section，便于把主文档里的命中回溯到具体节。
如果命中来自页眉或页脚，`hits[*].section_index` 会保持为空，
`hits[*].references` 则继续列出它被哪些 `section_index` 以
`default` / `first` / `even` 方式引用，便于识别共享 part。

```cpp
const auto styles = doc.list_styles();
const auto heading = doc.find_style("Heading1");

auto paragraph = doc.paragraphs();
doc.set_paragraph_style(paragraph, "Heading1");
```

现在也可以用 `ensure_paragraph_style()`、`ensure_character_style()`、
`ensure_table_style()` 以声明式方式创建或更新最小样式定义，同时保留该样式节点上
未被这组 API 覆盖的无关 XML。

```cpp
featherdoc::paragraph_style_definition body_style{};
body_style.name = "Body Text";
body_style.based_on = std::string{"Normal"};
body_style.run_font_family = std::string{"Segoe UI"};
body_style.run_language = std::string{"en-US"};
body_style.is_quick_format = true;

doc.ensure_paragraph_style("BodyText", body_style);
```

## 分节页面设置检查

现在也可以通过 `get_section_page_setup(section_index)` 只读读取某个 section
上显式声明的页面方向、纸张尺寸、页边距和起始页码。返回的
`section_page_setup` 会包含 `orientation`、`width_twips`、`height_twips`、
`margins` 和可选的 `page_number_start`。如果该 section 没有显式的
`w:sectPr` / `w:pgSz` / `w:pgMar`，API 会返回 `std::nullopt`。

```cpp
for (std::size_t i = 0; i < doc.section_count(); ++i) {
    const auto page_setup = doc.get_section_page_setup(i);
    if (!page_setup.has_value()) {
        continue;
    }

    std::cout << i << ": "
              << (page_setup->orientation ==
                          featherdoc::page_orientation::landscape
                      ? "landscape"
                      : "portrait")
              << " " << page_setup->width_twips << "x"
              << page_setup->height_twips << '\n';
}
```

CLI 也提供了对应的只读入口：

```bash
featherdoc_cli inspect-page-setup report.docx
featherdoc_cli inspect-page-setup report.docx --section 1 --json
```

如果希望直接从命令行改 section 页面设置，可以使用：

```bash
featherdoc_cli set-section-page-setup report.docx 1 \
  --orientation landscape \
  --width 15840 \
  --height 12240 \
  --margin-top 720 \
  --margin-bottom 1080 \
  --margin-left 1440 \
  --margin-right 1440 \
  --margin-header 360 \
  --margin-footer 540 \
  --page-number-start 1 \
  --output report-landscape.docx \
  --json
```

如果需要直接修改 section 的页面设置，也可以调用
`set_section_page_setup(section_index, setup)`。它会更新目标 section 的
`w:pgSz`、`w:pgMar` 和 `w:pgNumType/@w:start`，同时尽量保留无关属性，
比如纸张代码、gutter 或页码格式；对于还没有显式 `w:sectPr` 的最终 section，
也会自动补齐可编辑节点。

```cpp
featherdoc::section_page_setup setup{};
setup.orientation = featherdoc::page_orientation::landscape;
setup.width_twips = 15840U;
setup.height_twips = 12240U;
setup.margins.top_twips = 720U;
setup.margins.bottom_twips = 1080U;
setup.margins.left_twips = 1440U;
setup.margins.right_twips = 1440U;
setup.margins.header_twips = 360U;
setup.margins.footer_twips = 540U;
setup.page_number_start = 1U;

doc.set_section_page_setup(1, setup);
```

完整示例可以参考 `samples/sample_section_page_setup.cpp`，它会生成一个
双 section 文档：第一页保持纵向，第二节切成横向附录。

## 自定义编号定义

除了现有的托管 `bullet` / `decimal` 段落列表，现在也可以先用
`ensure_numbering_definition()` 创建或刷新一个命名编号定义，再用
`set_paragraph_numbering()` 把它挂到目标段落上。

```cpp
featherdoc::numbering_definition outline{};
outline.name = "LegalOutline";
outline.levels = {
    {featherdoc::list_kind::decimal, 3U, 0U, "%1."},
    {featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
};

const auto numbering_id = doc.ensure_numbering_definition(outline);
if (numbering_id.has_value()) {
    auto paragraph = doc.paragraphs();
    doc.set_paragraph_numbering(paragraph, *numbering_id);
}
```

如果希望段落通过 paragraph style 继承编号，而不是逐段写入 `w:numPr`，
现在也可以用 `set_paragraph_style_numbering()` 先把编号定义挂到样式上。

```cpp
doc.ensure_paragraph_style("LegalHeading", body_style);

const auto numbering_id = doc.ensure_numbering_definition(outline);
if (numbering_id.has_value()) {
    doc.set_paragraph_style_numbering("LegalHeading", *numbering_id, 1U);
}
```

如果想把当前文档里的编号目录再读出来，现在也可以用
`list_numbering_definitions()` 获取完整摘要，或者用
`find_numbering_definition(definition_id)` 读取单个定义的层级和实例 id。
如果已经拿到某个 `numId`，还可以继续用
`find_numbering_instance(instance_id)` 反查它所属的定义和 override 摘要。

```cpp
const auto numbering = doc.list_numbering_definitions();
const auto outline_summary = numbering_id.has_value()
                                 ? doc.find_numbering_definition(*numbering_id)
                                 : std::nullopt;
const auto first_instance =
    outline_summary.has_value() && !outline_summary->instance_ids.empty()
        ? doc.find_numbering_instance(outline_summary->instance_ids.front())
        : std::nullopt;
```

CLI 也提供了对应的只读入口：

```bash
featherdoc_cli inspect-numbering report.docx
featherdoc_cli inspect-numbering report.docx --instance 1
featherdoc_cli inspect-numbering report.docx --definition 1 --json
```

## 书签目录检查

现在可以通过 `list_bookmarks()` 和 `find_bookmark()` 做只读书签枚举，
在真正填充模板前先确认书签名、出现次数和占位符形态。每条
`bookmark_summary` 都会返回 `bookmark_name`、`occurrence_count` 以及推断出的
`kind`，例如 `text`、`block`、`table_rows`、`block_range`、`mixed`、
`malformed`。

```cpp
const auto bookmarks = doc.list_bookmarks();
const auto line_items = doc.find_bookmark("line_items");

if (line_items.has_value() &&
    line_items->kind == featherdoc::bookmark_kind::table_rows) {
    std::cout << "line_items 是一个表格行占位符\n";
}
```

CLI 现在也提供同样的只读入口：

```bash
featherdoc_cli inspect-bookmarks report.docx
featherdoc_cli inspect-bookmarks report.docx --part header --index 0 --bookmark header_rows --json
```

如果你要检查现有图片布局而不是书签元数据，现在也可以直接用
`inspect-images`。它会列出 body / header / footer / 分节页眉页脚部件里的
`drawing_images()` 结果，并且对锚定图片额外带出 `floating_options`
解析后的定位、环绕、裁剪信息。现在还可以先按图片的 `relationship_id`
和解析后的媒体包内路径筛选，再看整份列表或某一张图：

```bash
featherdoc_cli inspect-images report.docx
featherdoc_cli inspect-images report.docx --relationship-id rId5 --image-entry-name word/media/image1.png --json
featherdoc_cli inspect-images report.docx --part header --index 0 --image 0 --json
```

## 模板预检

现在可以先用 `validate_template()` 对正文模板做只读预检，再决定是否执行填充。
这个 API 会返回三类结果：

- `missing_required`：缺失的必填书签槽位
- `duplicate_bookmarks`：重复出现的书签名
- `malformed_placeholders`：占位符形态和目标槽位类型不匹配

```cpp
const auto validation = doc.validate_template({
    {"customer_name", featherdoc::template_slot_kind::text, true},
    {"line_item_row", featherdoc::template_slot_kind::table_rows, true},
    {"signature_block", featherdoc::template_slot_kind::block, false},
});

if (!validation) {
    for (const auto& missing : validation.missing_required) {
        std::cerr << "missing required slot: " << missing << '\n';
    }
}
```

如果你想直接看可运行样例，可以构建
`featherdoc_sample_template_validation`。它会在
`output/template-validation-visual` 下同时生成一份合法模板、一份故意带缺陷的模板，
以及 Markdown / JSON 验证报告，方便再接 Word 做可视化检查。

同样的检查现在也能直接走 CLI：

```bash
featherdoc_cli validate-template invoice.docx \
  --part body \
  --slot customer_name:text \
  --slot line_item_row:table_rows \
  --slot signature_block:block:optional \
  --json
```

同一套校验规则现在也能直接用于 `body_template()`、`header_template()`、
`footer_template()`、`section_header_template()`、`section_footer_template()`
返回的 `TemplatePart`。如果你想看 header/footer 的样例，可以构建
`featherdoc_sample_part_template_validation`，它会在
`output/part-template-validation-visual` 下生成一个“header 合法、footer 故意非法”
的 Word 样例和对应校验报告。

CLI 也支持同样的 part 级预检：

```bash
featherdoc_cli validate-template report.docx \
  --part section-footer \
  --section 0 \
  --slot footer_company:text \
  --slot footer_summary:block \
  --json
```

## 文档入口

仓库里当前的主要文档入口有：

- 英文主 README：[README.md](README.md)
- 中文 README：[README.zh-CN.md](README.zh-CN.md)
- Sphinx 文档首页：`docs/index.rst`
- 项目定位：`docs/project_identity_zh.rst`
- `v1.7.x` 路线图：`docs/v1_7_roadmap_zh.rst`
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
