# FeatherDoc

[![Windows MSVC CI](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/windows-msvc.yml/badge.svg?branch=master)](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/windows-msvc.yml)
[![Linux CMake CI](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/linux-cmake.yml/badge.svg?branch=master)](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/linux-cmake.yml)
[![macOS CMake CI](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/macos-cmake.yml/badge.svg?branch=master)](https://github.com/wuxianggujun/FeatherDoc/actions/workflows/macos-cmake.yml)

[English](README.md) | 简体中文

FeatherDoc 是一个面向现代 C++ 的 Microsoft Word `.docx` 读写与编辑库。

它当前重点覆盖：

- 现代 CMake / C++20 工程化集成
- 对 MSVC 友好的构建、测试与安装导出链路
- 段落、Run、表格、图片、列表、样式、编号、页眉页脚与模板部件的轻量级编辑 API
- 编号 catalog、样式治理、模板 schema 和 content control 的脚本化检查入口
- 对既有 `.docx` 的 reopen-after-save 持续编辑能力
- 基于真实 Microsoft Word 渲染结果的截图级可视化验证

> 这份文件提供中文入口、构建方式、验证流程和项目级说明。  
> 更完整的 API 细节、更多可运行 sample 和边界行为说明，仍以
> [README.md](README.md) 和 `docs/index.rst` 为准。

## 亮点

- CMake 3.20+ / C++20
- 默认支持 MSVC / Windows 构建、测试、安装导出链路
- 顶层构建默认启用 `featherdoc_cli`
- 提供编号 catalog JSON import/export/check/patch/lint/diff 工作流
- 支持模板 schema 校验 / 导出、书签填充、content control schema slot、表单状态 mutation 与按 tag / alias 的纯文本及富内容替换 API
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

实验性的 PDF 字节写出模块默认关闭。只有在你要推进进程内
DOCX-to-PDF 路线时才需要显式开启：

```bash
cmake -S . -B build-pdf -DFEATHERDOC_BUILD_PDF=ON -DBUILD_SAMPLES=ON
cmake --build build-pdf --target featherdoc_pdfio_probe
```

这会构建 `FeatherDoc::Pdf`，拉取或使用 PDFio，同时保持主目标
`FeatherDoc::FeatherDoc` 不依赖 PDFio。

默认构建不会拉取 PDFio / PDFium，也不会安装 PDF 实验模块头文件。
PDF 能力目前只作为未来支持方向存在，不属于稳定 API。

当前这条实验性 PDF 路线，已经不只是“最小文本样例”了：它覆盖基础段落、
表格、基础样式、CJK 回退、字体度量、Unicode / ToUnicode 回环检查，以及一小批
regression 样本，但仍然明确属于实验状态。更完整的分页和图片覆盖还在推进中。

实验性的 PDF 读入方向同样默认关闭。它默认使用 PDFium 源码 checkout，
不会自动下载 PDFium：

```bash
gclient config --unmanaged https://pdfium.googlesource.com/pdfium.git
gclient sync --no-history --jobs 8

cmake -S . -B build-pdf-import \
  -DFEATHERDOC_BUILD_PDF_IMPORT=ON \
  -DFEATHERDOC_PDFIUM_SOURCE_DIR=/path/to/pdfium
```

源码 provider 会优先使用 PDFium checkout 自带的 `gn` / `ninja`。Windows 上
链接源码版 `pdfium.lib` 时，建议使用 MSVC `/MT`：

```bash
-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
```

如果 PDFium 没有自动找到 Visual Studio，可传
`-DFEATHERDOC_PDFIUM_VS_YEAR=2026` 和
`-DFEATHERDOC_PDFIUM_VS_INSTALL=/path/to/VisualStudio`。
完整编译手册见 `BUILDING_PDF.md`，设计背景见
`design/dependencies/pdfium.md`。

如果要继续使用预编译包，可以传
`-DFEATHERDOC_PDFIUM_PROVIDER=package -DPDFium_DIR=/path/to/pdfium`。

启用 PDF 读入后，`featherdoc_cli` 还会提供：

```bash
featherdoc_cli import-pdf input.pdf --output output.docx \
  [--import-table-candidates-as-tables] \
  [--min-table-continuation-confidence <count>] [--json]
```

默认会拒绝表格候选。需要时再加
`--import-table-candidates-as-tables`，把它们提升为 DOCX 表格。

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

如果同一轮已经完成截图审查，也可以把 visual gate 的
`-SmokeReviewVerdict`、`-FixedGridReviewVerdict`、
`-SectionPageSetupReviewVerdict`、`-PageNumberFieldsReviewVerdict`、
`-CuratedVisualReviewVerdict` 以及对应的 `*ReviewNote` 直接传给这个总控脚本，
它会透传到 visual gate，并把各 flow 的 seeded verdict 写入
`report/summary.json`，同时展示在 `report/final_review.md`。表格样式质量
before/after Word 渲染证据包默认不拖慢发布检查；需要时显式加
`-IncludeTableStyleQuality`，它会把 `table-style-quality` 作为 curated visual
regression bundle 纳入 visual gate 和 release summary。生成的
`START_HERE.md`、`ARTIFACT_GUIDE.md`、`REVIEWER_CHECKLIST.md`、
`release_handoff.md`、`release_body.zh-CN.md` 和 `release_summary.zh-CN.md`
也会同步列出 smoke、fixed-grid、section/page-number 和 curated visual verdict。
后续若人工补写了表格样式质量结论，`sync_latest_visual_review_verdict.ps1`
会自动吸收 `latest_table-style-quality-visual-regression-bundle_task.json`。
当 numbering catalog governance、table-layout delivery governance、
content-control data-binding governance 或 project-template delivery
readiness summary 已经生成时，可以把这些 JSON 通过
`-ReleaseBlockerRollupInputJson` 或 `-ReleaseBlockerRollupInputRoot` 交给同一个
总控脚本，也可以用 `-ReleaseBlockerRollupAutoDiscover` 自动收集默认的
`output/numbering-catalog-governance/summary.json`、
`output/table-layout-delivery-governance/summary.json`、
`output/content-control-data-binding-governance/summary.json` 和
`output/project-template-delivery-readiness/summary.json`。它会调用
`build_release_blocker_rollup_report.ps1`，写出
`report/release-blocker-rollup/summary.json` 和 Markdown，并把 rollup 状态、
来源数量、blocker / action / warning 计数以及自动发现输入同步进
`report/summary.json` 和 `report/final_review.md`。需要把最终 rollup 变成硬门禁时，
再加 `-ReleaseBlockerRollupFailOnBlocker` 或
`-ReleaseBlockerRollupFailOnWarning`。

如果发布前想先给 reviewer 一份只读交付总览，可以运行
`scripts/build_release_governance_handoff_report.ps1`。它会检查 `output/`
下默认的 numbering catalog governance、table layout delivery governance、
content-control data-binding governance 和 project-template delivery readiness
summary，缺失报告会附带重建命令，并写出
`output/release-governance-handoff/summary.json` 和 Markdown；随后同一批报告可继续交给
`run_release_candidate_checks.ps1 -ReleaseBlockerRollupAutoDiscover` 做最终门禁。
如果同时传入 `-IncludeReleaseBlockerRollup`，handoff 目录下还会生成嵌套的
`release-blocker-rollup/summary.json` 和 Markdown，方便 reviewer 在同一输出目录里查看最终
blocker/action 汇总。
同一能力也可以直接并入发布总控：
`run_release_candidate_checks.ps1 -ReleaseGovernanceHandoff` 会在
`report/release-governance-handoff/` 下生成 handoff，并把 loaded / missing 报告数量、
blocker / action / warning 计数写回 `report/summary.json` 和
`report/final_review.md`。需要一起归档最终 blocker/action 汇总时，加
`-ReleaseGovernanceHandoffIncludeRollup`；需要把缺失报告、blocker 或 warning 变成硬门禁时，
分别加 `-ReleaseGovernanceHandoffFailOnMissing`、
`-ReleaseGovernanceHandoffFailOnBlocker` 或
`-ReleaseGovernanceHandoffFailOnWarning`。
如果希望把四条最终治理报告也一起从既有 summary 编排出来，可以运行
`scripts/build_release_governance_pipeline_report.ps1`。它会消费 `output/` 下已经生成的
document skeleton rollup、numbering catalog manifest check、table layout rollup、
content-control data-binding governance、project-template onboarding governance 和
schema approval history，依次写出 numbering catalog governance、table layout delivery
governance、content-control data-binding governance、project-template delivery readiness、
release governance handoff 和最终 release blocker rollup；总览会落到
`output/release-governance-pipeline/summary.json` 和 Markdown。该脚本只读输入 summary，
不重跑 CLI、CMake、Word 或视觉自动化。
Linux/macOS CI 的 `release_smoke` 步骤也会把 release candidate blocker rollup、
release governance handoff、release governance pipeline 等 smoke 输出上传为 Actions
artifact，便于从远端构建直接下载审查证据。

脚本结束后，输出目录里会生成：

- `START_HERE.md`
- `report/ARTIFACT_GUIDE.md`
- `report/REVIEWER_CHECKLIST.md`
- `report/release_handoff.md`
- `report/release_body.zh-CN.md`
- `report/release_summary.zh-CN.md`

如果你还希望把模板 schema baseline 检查也并入这条发布前总检查，可以额外传入
`-TemplateSchemaInputDocx`、`-TemplateSchemaBaseline`，以及
`-TemplateSchemaSectionTargets` /
`-TemplateSchemaResolvedSectionTargets` 其中之一。这样脚本会把 template
schema gate 结果写进 `report/summary.json`，包括 committed schema 是否 lint
干净；一旦发现 schema 漂移，或者 baseline 本身存在需要修复的维护问题，就会让
整条 preflight 失败。

如果你想把仓库里已经登记到 manifest 的多份 template schema baseline 一次性
并入这条发布前总检查，则改传 `-TemplateSchemaManifestPath`。脚本会转而调用
`check_template_schema_manifest.ps1`，把 manifest gate 的状态、entry 数量、
drift 数量，以及 dirty baseline 数量一起写进 `report/summary.json`；只要其中
任一 baseline 漂移，或者任何 committed schema baseline 需要 repair，就会让
整条 preflight 失败。

如果你还想把真实项目模板回归也并入这条发布前总检查，可以再传
`-ProjectTemplateSmokeManifestPath`。脚本会调用
`run_project_template_smoke.ps1`，把 manifest 路径、summary 路径、entry /
failed 计数，以及聚合后的 project-template `visual_verdict` 一起写进
`report/summary.json`，并同步带到 `START_HERE.md`、`ARTIFACT_GUIDE.md`、
`REVIEWER_CHECKLIST.md` 和 `release_handoff.md`。如果希望发布前强制确认
仓库里所有已跟踪 `.docx` / `.dotx` 都已纳入 smoke manifest，或已经明确写进
`candidate_exclusions`，再加 `-ProjectTemplateSmokeRequireFullCoverage`。
脚本会把完整扫描写到 `project-template-smoke/candidate_discovery.json`，并把
registered / unregistered / excluded 计数同步进发布包。

如果你的目标不是做一次临时校验，而是把某份模板正式纳入仓库级 baseline，
优先使用 `register_template_schema_manifest_entry.ps1`。它会在需要时先准备
generated fixture，再冻结标准化 schema baseline，并把对应条目写入或更新到
`baselines/template-schema/manifest.json`，省掉手工改 manifest 的步骤。写入
manifest 前，它现在还会跑同一套 baseline gate；只要 schema 维护质量不干净，
或者文档与 schema 存在漂移，就会拒绝登记，除非你明确传入
`-SkipBaselineCheck`。

如果截图级 review 结论是在后续补写的，优先执行最短同步命令，把最终
visual verdict 一次性回写到 gate summary、release summary 和 release note
bundle。这个同步流程现在也会自动吸收同一 task root 下的 curated visual
bundle task，而不是只处理最早那几条固定链路，因此不需要为了补写 bundle
结论重新跑整条 preflight：

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1
```

刷新后的 `START_HERE.md`、`report/ARTIFACT_GUIDE.md`、
`report/REVIEWER_CHECKLIST.md`、`report/release_handoff.md`、
`report/release_body.zh-CN.md` 和 `report/release_summary.zh-CN.md`
现在会同时展示：

- 总 `visual verdict`
- `section page setup` verdict
- `page number fields` verdict
- 每个 curated visual regression bundle 的 verdict
- 对应 review task 路径，以及
  `open_latest_word_review_task.ps1 -SourceKind <bundle-key>-visual-regression-bundle`
  的快捷入口；表格样式质量流的专名入口是
  `open_latest_word_review_task.ps1 -SourceKind table-style-quality-visual-regression-bundle`，
  稳定指针是 `latest_table-style-quality-visual-regression-bundle_task.json`

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

如果你想单独验证浮动图片锚点的 z-order 真实叠放效果，可运行：

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\run_floating_image_z_order_visual_regression.ps1
```

这个回归会构建 `sample_floating_image_z_order_visual` 样例，先用 CLI 检查并提取
两张 anchored 图片，确认 z-order 和媒体内容都正确，再导出 Word 渲染后的首页
截图，方便你确认橙色浮动图片确实覆盖并显示在蓝色图片上方。输出目录为
`output/floating-image-z-order-visual-regression/`。

如果只想跑基础 Word 冒烟检查：

这条流程是本地验证工具，脚本实现位于 `scripts/`，不是 FeatherDoc 库 API。

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
相比早期 sample 截图，这一组图更直观地展示了 FeatherDoc 当前的能力面：
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

如果同一轮已经完成截图审查，可以给 `run_word_visual_smoke.ps1` 追加
`-ReviewVerdict pass`（或 `fail` / `undetermined`）和 `-ReviewNote`，这样
`review_result.json` 会直接记录 `status=reviewed`、`verdict` 与
`reviewed_at`，后续 verdict sync 可以直接消费。跑 release gate 时也可以用
`-SmokeReviewVerdict pass` 和 `-SmokeReviewNote`，把同一轮 smoke 截图结论
透传到 smoke 报告与 gate summary；fixed-grid 四联也能用
`-FixedGridReviewVerdict pass` 和 `-FixedGridReviewNote`，把结论直接种进生成的
fixed-grid review task。section page setup 和 page-number fields 两类 bundle 也可用
`-SectionPageSetupReviewVerdict/-SectionPageSetupReviewNote` 与
`-PageNumberFieldsReviewVerdict/-PageNumberFieldsReviewNote` 走同样流程。
curated visual regression bundle 也可以用 `-CuratedVisualReviewVerdict pass` 和
`-CuratedVisualReviewNote`，把同一轮结论种进每个生成的 curated bundle task，
并同步展示到 gate summary。

要把这些渲染结果同步回仓库里的 README 预览 PNG，执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\refresh_readme_visual_assets.ps1
```

要从这些截图跳转到完整复现命令、review task 或发布前验证流程，建议先看：

- [VISUAL_VALIDATION_QUICKSTART.md](VISUAL_VALIDATION_QUICKSTART.md)
- [VISUAL_VALIDATION.md](VISUAL_VALIDATION.md)
- [VISUAL_VALIDATION.zh-CN.md](VISUAL_VALIDATION.zh-CN.md)

## CLI

`featherdoc_cli` 当前已经覆盖分节、样式、编号、页面设置、书签、content control、
图片和模板部件等多类检查与编辑流程。

```bash
featherdoc_cli inspect-sections input.docx
featherdoc_cli inspect-sections input.docx --json
featherdoc_cli inspect-styles input.docx --style Strong --json
featherdoc_cli inspect-runs input.docx 1 --run 0 --json
featherdoc_cli inspect-template-runs input.docx 1 --run 0 --json
featherdoc_cli inspect-numbering input.docx --definition 1 --json
featherdoc_cli inspect-style-numbering input.docx --json
featherdoc_cli audit-style-numbering input.docx --fail-on-issue --json
featherdoc_cli repair-style-numbering input.docx --plan-only --json
featherdoc_cli repair-style-numbering input.docx --apply --output repaired-style-numbering.docx --json
featherdoc_cli repair-style-numbering input.docx --catalog-file numbering-catalog.json --apply --output catalog-repaired.docx --json
featherdoc_cli export-numbering-catalog input.docx --output numbering-catalog.json --json
featherdoc_cli check-numbering-catalog input.docx --catalog-file numbering-catalog.json --output numbering-catalog.generated.json --json
pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_report.ps1 -InputDocx .\input.docx -OutputDir .\output\document-skeleton-governance -BuildDir build-codex-clang-compat -SkipBuild
pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1 -InputRoot .\output\document-skeleton-governance -OutputDir .\output\document-skeleton-governance-rollup -FailOnIssue
pwsh -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_baseline.ps1 -InputDocx .\input.docx -CatalogFile .\numbering-catalog.json -GeneratedCatalogOutput .\numbering-catalog.generated.json -BuildDir build-codex-clang-compat -SkipBuild
pwsh -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_manifest.ps1 -ManifestPath .\baselines\numbering-catalog\manifest.json -BuildDir build-codex-clang-compat -OutputDir .\output\numbering-catalog-manifest-checks -SkipBuild
featherdoc_cli patch-numbering-catalog numbering-catalog.json --patch-file numbering-catalog.patch.json --output numbering-catalog.patched.json --json
featherdoc_cli lint-numbering-catalog numbering-catalog.patched.json --json
featherdoc_cli diff-numbering-catalog numbering-catalog.json numbering-catalog.patched.json --fail-on-diff --json
featherdoc_cli semantic-diff before.docx after.docx --fail-on-diff --json
featherdoc_cli import-numbering-catalog target.docx --catalog-file numbering-catalog.patched.json --output target-numbering.docx --json
featherdoc_cli inspect-page-setup input.docx --section 1 --json
featherdoc_cli inspect-bookmarks input.docx --part header --index 0 --bookmark header_rows --json
featherdoc_cli inspect-content-controls input.docx --tag customer_name --json
featherdoc_cli replace-content-control-text input.docx --tag customer_name --text "Ada Lovelace" --output content-control-text.docx --json
featherdoc_cli set-content-control-form-state input.docx --tag approved --checked false --clear-lock --output content-control-form.docx --json
featherdoc_cli inspect-images input.docx --relationship-id rId5 --json
featherdoc_cli ensure-table-style input.docx ReportTable --name "Report Table" --based-on TableGrid --style-text-color whole_table:333333 --style-bold first_row:true --style-italic first_row:true --style-font-size first_row:14 --style-font-family "first_row:Aptos Display" --style-east-asia-font-family first_row:SimHei --style-cell-vertical-alignment first_row:bottom --style-cell-text-direction first_row:top_to_bottom_right_to_left --style-paragraph-alignment first_row:right --style-paragraph-spacing-after first_row:120 --style-paragraph-line-spacing first_row:240:at_least --style-fill second_banded_rows:F2F2F2 --output styled.docx --json
featherdoc_cli inspect-table-style input.docx ReportTable --json
featherdoc_cli audit-table-style-regions input.docx --fail-on-issue --json
featherdoc_cli audit-table-style-inheritance input.docx --fail-on-issue --json
featherdoc_cli audit-table-style-quality input.docx --fail-on-issue --json
featherdoc_cli plan-table-style-quality-fixes input.docx --json
featherdoc_cli apply-table-style-quality-fixes input.docx --look-only --output quality-fixed.docx --json
powershell -ExecutionPolicy Bypass -File .\scripts\build_table_layout_delivery_report.ps1 -InputDocx .\input.docx -BuildDir build-codex-clang-compat -OutputDir .\output\table-layout-delivery-report -SkipBuild
powershell -ExecutionPolicy Bypass -File .\scripts\build_table_layout_delivery_rollup_report.ps1 -InputRoot .\output\table-layout-delivery-report -OutputDir .\output\table-layout-delivery-rollup -FailOnBlocker
powershell -ExecutionPolicy Bypass -File .\scripts\build_table_layout_delivery_governance_report.ps1 -InputJson .\output\table-layout-delivery-rollup\summary.json -OutputDir .\output\table-layout-delivery-governance -FailOnIssue -FailOnBlocker
powershell -ExecutionPolicy Bypass -File .\scripts\run_table_style_quality_visual_regression.ps1 -BuildDir build-codex-clang-compat -OutputDir output/table-style-quality-visual-regression -SkipBuild
powershell -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1 -SkipConfigure -SkipBuild -IncludeTableStyleQuality
featherdoc_cli check-table-style-look input.docx --fail-on-issue --json
featherdoc_cli repair-table-style-look input.docx --apply --output repaired-style-look.docx --json
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
featherdoc_cli move-header-part input.docx 1 0 --output headers-reordered.docx --json
featherdoc_cli remove-footer-part input.docx 0 --output footers-pruned.docx
featherdoc_cli move-footer-part input.docx 1 0 --output footers-reordered.docx --json
featherdoc_cli append-page-number-field input.docx --part section-header --section 1 --output page-number.docx --json
featherdoc_cli append-table-of-contents-field input.docx --part body --min-outline-level 1 --max-outline-level 3 --result-text "目录" --output toc.docx --json
featherdoc_cli append-field input.docx " AUTHOR " --part body --result-text "Ada Lovelace" --output author-field.docx --json
featherdoc_cli append-reference-field input.docx target_heading --part body --result-text "Referenced heading" --output ref.docx --json
featherdoc_cli append-page-reference-field input.docx target_heading --part body --relative-position --result-text "Page reference" --output page-ref.docx --json
featherdoc_cli append-style-reference-field input.docx "Heading 1" --part body --paragraph-number --result-text "Section heading" --output style-ref.docx --json
featherdoc_cli append-document-property-field input.docx Title --part body --result-text "Document title" --output doc-property.docx --json
featherdoc_cli append-date-field input.docx --part body --format "yyyy-MM-dd" --result-text "2026-05-01" --dirty --output date-field.docx --json
featherdoc_cli append-hyperlink-field input.docx https://example.com/report --part body --anchor target_heading --tooltip "Open target heading" --result-text "Open report" --locked --output hyperlink-field.docx --json
featherdoc_cli append-sequence-field input.docx Figure --part body --number-format ROMAN --restart 4 --result-text "IV" --output sequence.docx --json
featherdoc_cli replace-field input.docx 0 " SEQ Table \* ARABIC \r 1 " --part body --result-text "1" --output replaced-field.docx --json
featherdoc_cli append-caption input.docx Figure --part body --text "Architecture overview" --number-result "1" --output caption.docx --json
featherdoc_cli append-index-entry-field input.docx FeatherDoc --part body --subentry API --bookmark target_heading --cross-reference "See API" --output xe.docx --json
featherdoc_cli append-index-field input.docx --part body --columns 2 --result-text "Index placeholder" --output index.docx --json
featherdoc_cli append-complex-field input.docx --part body --instruction-before " IF " --nested-instruction " MERGEFIELD CustomerName " --nested-result-text "Ada" --instruction-after " = \"Ada\" \"Matched\" \"Other\" " --result-text "Matched" --output complex-field.docx --json
featherdoc_cli inspect-update-fields-on-open input.docx --json
featherdoc_cli set-update-fields-on-open input.docx --enable --output update-fields.docx --json
featherdoc_cli set-template-table-from-json report.docx --bookmark line_items_table --patch-file row_patch.json --output report-updated.docx --json
featherdoc_cli set-template-tables-from-json report.docx --patch-file multi_table_patch.json --output report-updated.docx --json
featherdoc_cli validate-template input.docx --part body --slot customer:text --slot line_items:table_rows --json
featherdoc_cli validate-template-schema input.docx --target section-header --section 0 --slot header_title:text --target section-footer --section 0 --slot footer_company:text --slot footer_summary:block --json
featherdoc_cli validate-template-schema input.docx --schema-file template-schema.json --json
featherdoc_cli export-template-schema input.docx --output template-schema.json --json
featherdoc_cli export-template-schema input.docx --section-targets --output section-template-schema.json --json
featherdoc_cli export-template-schema input.docx --resolved-section-targets --output resolved-section-template-schema.json --json
featherdoc_cli normalize-template-schema template-schema.json --output normalized-template-schema.json --json
featherdoc_cli lint-template-schema template-schema.json --json
featherdoc_cli repair-template-schema template-schema.json --output repaired-template-schema.json --json
featherdoc_cli merge-template-schema shared-template-schema.json invoice-template-schema.json --output merged-template-schema.json --json
featherdoc_cli patch-template-schema committed-template-schema.json --patch-file schema.patch.json --output patched-template-schema.json --json
featherdoc_cli preview-template-schema-patch committed-template-schema.json --patch-file schema.patch.json --output-patch schema.preview.patch.json --json
featherdoc_cli preview-template-schema-patch committed-template-schema.json generated-schema.json --output-patch schema.preview.patch.json --json
featherdoc_cli build-template-schema-patch committed-schema.json generated-schema.json --output schema.patch.json --json
featherdoc_cli diff-template-schema old-template-schema.json new-template-schema.json --json
featherdoc_cli diff-template-schema committed-schema.json generated-schema.json --fail-on-diff --json
featherdoc_cli check-template-schema input.docx --schema-file committed-schema.json --resolved-section-targets --output generated-schema.json --json
pwsh -ExecutionPolicy Bypass -File .\scripts\freeze_template_schema_baseline.ps1 -InputDocx .\template.docx -SchemaOutput .\template.schema.json -ResolvedSectionTargets
pwsh -ExecutionPolicy Bypass -File .\scripts\check_template_schema_baseline.ps1 -InputDocx .\template.docx -SchemaFile .\template.schema.json -ResolvedSectionTargets -GeneratedSchemaOutput .\generated-template.schema.json -RepairedSchemaOutput .\repaired-template.schema.json
pwsh -ExecutionPolicy Bypass -File .\scripts\check_template_schema_manifest.ps1 -ManifestPath .\baselines\template-schema\manifest.json -BuildDir build-codex-clang-compat -RepairedSchemaOutputDir .\output\template-schema-manifest-repairs
pwsh -ExecutionPolicy Bypass -File .\scripts\register_template_schema_manifest_entry.ps1 -Name template-name -InputDocx .\template.docx
pwsh -ExecutionPolicy Bypass -File .\scripts\onboard_project_template.ps1 -InputDocx .\samples\chinese_invoice_template.docx -TemplateName chinese-invoice -OutputDir .\output\project-template-onboarding\chinese-invoice -BuildDir build-codex-clang-compat -SkipBuild -SkipProjectTemplateSmoke
pwsh -ExecutionPolicy Bypass -File .\scripts\new_project_template_smoke_onboarding_plan.ps1 -ManifestPath .\samples\project_template_smoke.manifest.json -BuildDir build-codex-clang-compat
pwsh -ExecutionPolicy Bypass -File .\scripts\discover_project_template_smoke_candidates.ps1 -ManifestPath .\samples\project_template_smoke.manifest.json
pwsh -ExecutionPolicy Bypass -File .\scripts\discover_project_template_smoke_candidates.ps1 -ManifestPath .\samples\project_template_smoke.manifest.json -Json -IncludeRegistered -IncludeExcluded -OutputPath .\output\project-template-smoke\candidate_discovery.json -FailOnUnregistered
pwsh -ExecutionPolicy Bypass -File .\scripts\check_project_template_smoke_manifest.ps1 -ManifestPath .\samples\project_template_smoke.manifest.json -BuildDir build-codex-clang-compat -CheckPaths
pwsh -ExecutionPolicy Bypass -File .\scripts\describe_project_template_smoke_manifest.ps1 -ManifestPath .\samples\project_template_smoke.manifest.json -SummaryJson .\output\project-template-smoke\summary.json -BuildDir build-codex-clang-compat
pwsh -ExecutionPolicy Bypass -File .\scripts\register_project_template_smoke_manifest_entry.ps1 -Name contract-template -ManifestPath .\samples\project_template_smoke.manifest.json -InputDocx .\samples\chinese_invoice_template.docx -SchemaValidationFile .\baselines\template-schema\chinese_invoice_template.schema.json -SchemaBaselineFile .\baselines\template-schema\chinese_invoice_template.schema.json -VisualSmokeOutputDir .\output\project-template-smoke\contract-template-visual -ReplaceExisting
pwsh -ExecutionPolicy Bypass -File .\scripts\run_project_template_smoke.ps1 -ManifestPath .\samples\project_template_smoke.manifest.json -BuildDir build-codex-clang-compat -OutputDir output/project-template-smoke
pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_smoke_visual_verdict.ps1 -SummaryJson .\output\project-template-smoke\summary.json
pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_onboarding_governance_report.ps1 -InputRoot .\output\project-template-onboarding -InputRoot .\output\project-template-smoke-onboarding-plan -InputRoot .\output\project-template-smoke -OutputDir .\output\project-template-onboarding-governance -FailOnBlocker
pwsh -ExecutionPolicy Bypass -File .\scripts\write_project_template_schema_approval_history.ps1 -SummaryJsonDir .\output\project-template-smoke -Recurse -OutputJson .\output\project-template-schema-approval-history\history.json
pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_delivery_readiness_report.ps1 -InputJson .\output\project-template-onboarding-governance\summary.json,.\output\project-template-schema-approval-history\history.json -OutputDir .\output\project-template-delivery-readiness -FailOnBlocker
pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1 -InputRoot .\output\project-template-smoke -OutputDir .\output\schema-patch-confidence-calibration -FailOnPending
pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1 -InputRoot .\output\document-skeleton-governance -OutputDir .\output\document-skeleton-governance-rollup -FailOnIssue
pwsh -ExecutionPolicy Bypass -File .\scripts\build_numbering_catalog_governance_report.ps1 -InputJson .\output\document-skeleton-governance-rollup\summary.json,.\output\numbering-catalog-manifest-checks\summary.json -OutputDir .\output\numbering-catalog-governance -FailOnIssue -FailOnDrift -FailOnBlocker
pwsh -ExecutionPolicy Bypass -File .\scripts\build_table_layout_delivery_rollup_report.ps1 -InputRoot .\output\table-layout-delivery-report -OutputDir .\output\table-layout-delivery-rollup -FailOnBlocker
pwsh -ExecutionPolicy Bypass -File .\scripts\build_table_layout_delivery_governance_report.ps1 -InputJson .\output\table-layout-delivery-rollup\summary.json -OutputDir .\output\table-layout-delivery-governance -FailOnIssue -FailOnBlocker
pwsh -ExecutionPolicy Bypass -File .\scripts\build_release_blocker_rollup_report.ps1 -InputJson .\output\numbering-catalog-governance\summary.json,.\output\table-layout-delivery-governance\summary.json,.\output\project-template-delivery-readiness\summary.json -OutputDir .\output\release-blocker-rollup -FailOnBlocker
pwsh -ExecutionPolicy Bypass -File .\scripts\render_template_document.ps1 -InputDocx .\samples\chinese_invoice_template.docx -PlanPath .\samples\chinese_invoice_template.render_plan.json -OutputDocx .\output\rendered\invoice.docx -SummaryJson .\output\rendered\invoice.render.summary.json -BuildDir build-codex-clang-compat -SkipBuild
```

当 `preview-template-schema-patch` 写出 `--output-patch` 时，JSON summary 会包含 `output_patch_path`，方便自动化流程直接读取生成的 patch 文件。

如果你要把多份真实项目模板放到同一条 smoke 流里统一检查，可以直接用
`scripts/run_project_template_smoke.ps1`。manifest 里的每个 entry 既可以直
接指向仓库里的 `.docx`，也可以先通过 `prepare_sample_target` /
`prepare_argument` 生成输入文档，然后按需组合
`template_validations`、`schema_validation`、`schema_baseline`，以及可选
的 `visual_smoke`。脚本会同时产出每个 entry 的明细产物和聚合后的
`summary.json` / `summary.md`，便于一次性审阅整组模板。仓库内可直接运行
的例子见 `samples/project_template_smoke.manifest.json`。其中
`schema_baseline` 结果现在还会额外记录 committed schema 是否 lint 干净、
lint issue 数量，以及 baseline gate 生成的 repair 候选路径。

当多份真实模板已经跑过 onboarding、onboarding plan 或 project-template
smoke 后，可以继续运行
`scripts/build_project_template_onboarding_governance_report.ps1`。它只读取既有
`onboarding_summary.json`、`plan.json` 和 smoke `summary.json`，把
`schema_approval_state`、`release_blockers`、`action_items` 与人工复核建议聚合成
`featherdoc.project_template_onboarding_governance_report.v1` JSON / Markdown。
如果传入 `-FailOnBlocker`，仍存在未审批、待复核或被阻断的 schema approval 时会返回非零退出码，适合放进发布前 gate。
当 `scripts/write_project_template_schema_approval_history.ps1` 已经写出 schema
approval history 后，可以继续运行
`scripts/build_project_template_delivery_readiness_report.ps1`。它会把 onboarding
governance 的模板条目和最新 schema approval history gate 拼成
`featherdoc.project_template_delivery_readiness_report.v1`，并保留每个模板的
release blocker、action item、人工复核建议和历史审批状态。`-FailOnBlocker`
适合作为真实模板交付前的只读 gate。

如果需要从多次 smoke / approval history 里校准 schema patch 建议，可以运行
`scripts/write_schema_patch_confidence_calibration_report.ps1`。它会聚合
schema patch review 规模、approval outcome 和可选 `confidence` 元数据，输出
`featherdoc.schema_patch_confidence_calibration_report.v1`，并按置信度区间给出
approved / pending / rejected / invalid 的分布和保守的 `recommended_min_confidence`。
这个脚本是只读汇总入口，不替代人工审批；`-FailOnPending` 可用于阻止未决审批进入阈值收紧流程。

当多份单文档 document skeleton governance summary 已经生成后，可以先用
`scripts/build_document_skeleton_governance_rollup_report.ps1` 汇总为
`featherdoc.document_skeleton_governance_rollup_report.v1`，保留每份模板的
exemplar catalog 路径、样式编号 issue 汇总、release blocker 和 action item。
把这份 rollup 和 `scripts/check_numbering_catalog_manifest.ps1` 的 summary
一起交给 `scripts/build_numbering_catalog_governance_report.ps1`，可以得到
`featherdoc.numbering_catalog_governance_report.v1`，统一覆盖 exemplar catalog
覆盖情况、样式编号 issue、catalog baseline drift 和脏 baseline。表格版式侧也有同样的聚合层：
`scripts/build_table_layout_delivery_rollup_report.ps1` 会把多份
`featherdoc.table_layout_delivery_report.v1` 汇总成
`featherdoc.table_layout_delivery_rollup_report.v1`，保留 table style issue、
安全 `tblLook` 修复计数、浮动表格 preset plan 路径、release blocker 和 action item。
再把 rollup 交给 `scripts/build_table_layout_delivery_governance_report.ps1`，可输出
`featherdoc.table_layout_delivery_governance_report.v1`，把待执行的安全 `tblLook`
修复、人工 table-style 工作、浮动表格 plan 复核和视觉回归证据统一变成明确的交付
blocker / action item。当 numbering catalog governance、table layout delivery
governance、project template delivery readiness 等报告已经生成后，可以再用
`scripts/build_release_blocker_rollup_report.ps1` 统一汇总 release blocker 和 action
item。它会为不同来源的重复 blocker id 生成可追踪的 `composite_id`，输出
`featherdoc.release_blocker_rollup_report.v1`，并可通过 `-FailOnBlocker` /
`-FailOnWarning` 作为发布面板或 CI 的最后一层只读检查。
在进入最终 rollup 前，也可以先运行
`scripts/build_release_governance_handoff_report.ps1` 输出
`featherdoc.release_governance_handoff_report.v1`，把四条最终治理线的 loaded /
missing 状态、blocker/action/warning 计数和后续命令收束成一份 reviewer handoff。
需要同时归档最终 blocker/action 汇总时，加 `-IncludeReleaseBlockerRollup`。

如果你想先单独校验 manifest 契约，再决定要不要真正跑完整 smoke，可先
执行 `scripts/check_project_template_smoke_manifest.ps1`。示例 manifest 现
在也带了一个本地 `$schema` 指针，指向
`samples/project_template_smoke.manifest.schema.json`，这样支持 JSON
schema 的编辑器可以更早给出字段补全和结构报错。`run_project_template_smoke.ps1`
本身也会先做同样的前置校验，遇到坏 entry 会立刻中止，不会跑到中途才炸。

如果你要维护这份 manifest 本身，可以用
`scripts/describe_project_template_smoke_manifest.ps1` 看当前登记的 entry
和最近一次 smoke 结果，也可以用
`scripts/register_project_template_smoke_manifest_entry.ps1` 新增或更新单条
entry，而不是手改 JSON。常见场景直接传
`-SchemaValidationFile` / `-SchemaBaselineFile` 就够了；如果你的
`template_validations` 或 `schema_validation.targets` 比较复杂，也可以用
`-TemplateValidationsFile` 和 `-SchemaValidationTargetsFile` 从 JSON 数组文
件直接回填。真正接入项目模板前，可以先跑
`scripts/new_project_template_smoke_onboarding_plan.ps1` 生成一份不改 manifest
的接入计划。它会把候选发现、每个模板的
`freeze_template_schema_baseline.ps1` 命令、render-data workspace 准备命令、
业务数据完整性校验命令、`register_project_template_smoke_manifest_entry.ps1`
命令，以及最后的 smoke / strict preflight 命令统一写到 `plan.json`、
`plan.md` 和 `candidate_discovery.json`，方便先审阅 schema baseline 路径、
可填写的数据骨架工作区和 visual smoke 输出目录，再决定是否真正登记。计划里的
每个候选现在也会带 `schema_approval_state`、`release_blockers`、`action_items`
和 `manual_review_recommendations`；尚未运行 smoke 的候选会明确标记为
`status=not_evaluated`，提醒先生成并复核 `schema_patch_approval_items`。
也可以单独跑
`scripts/discover_project_template_smoke_candidates.ps1`，它会列出仓库里已
跟踪但还没登记的 `.docx` / `.dotx` 候选，并给出带唯一 entry 名的
`register_project_template_smoke_manifest_entry.ps1` 命令。如果要把它当覆盖率闸门，
加 `-FailOnUnregistered` 即可在仍有未登记候选时返回非零退出码；明确不是项目模板
的测试夹具可以放进 `candidate_exclusions`。示例 manifest 现在
已经把已提交的 `samples/chinese_invoice_template.docx` 作为真实模板 entry
接入，并启用了 schema validation、schema baseline 和 Word visual smoke。如果后续人工修改了某个 visual smoke 生成的
`review_result.json`，再跑一次
`scripts/sync_project_template_smoke_visual_verdict.ps1`，就能把 entry 级
`review_status` / `review_verdict`、顶层 `visual_verdict`，以及 pending /
undetermined 计数同步回 `summary.json` 和 `summary.md`。现在
`describe_project_template_smoke_manifest.ps1` 也会把这些最新的 visual
verdict 字段一起带出来，方便维护时快速确认。如果这份 smoke summary 已经接
进 `run_release_candidate_checks.ps1`，再额外传
`-ReleaseCandidateSummaryJson <report/summary.json> -RefreshReleaseBundle`，
还可以顺手把 release-candidate 的 `summary.json`、`final_review.md`、
`START_HERE.md`、`ARTIFACT_GUIDE.md`、`REVIEWER_CHECKLIST.md` 和
`release_handoff.md` 一起刷新掉，不需要重跑整条 preflight。

如果你要的是“单份模板 + 一份 JSON 数据计划 = 一份最终 docx”的入口，而不是
仓库级 manifest smoke，那么直接用 `scripts/render_template_document.ps1`。
它会把 `fill-bookmarks`、`replace-bookmark-paragraphs`、
`replace-bookmark-table-rows` 和 `apply-bookmark-block-visibility` 串成一
条可复现的渲染链路；只要计划里引用了不存在的书签，脚本就会直接失败，避免
模板漂移被静默吞掉。可直接参考
`samples/template_render_plan.schema.json` 和
`samples/chinese_invoice_template.render_plan.json`。

如果你不想手写第一版 render plan，可以先跑
`scripts/export_template_render_plan.ps1`。它会检查正文、header part、footer
part 的书签，把 `text` / `block` / `table_rows` / `block_range` 自动归类到
`bookmark_text`、`bookmark_paragraphs`、`bookmark_table_rows`、
`bookmark_block_visibility`，然后写出一份可直接编辑的草稿 JSON。默认占位值
会写成 `TODO: <bookmark_name>`，表格行草稿默认导出为空数组，方便你后续补业务
数据而不是从零拼结构。默认模式会把页眉/页脚书签导出成已加载的
`header[index]` / `footer[index]` 目标；如果你要直接按 section 生效视角编辑，
可以传 `-TargetMode resolved-section-targets`，让草稿写出带 `section` / `kind`
选择器的 `section-header` / `section-footer`，summary 里也会记录 `target_mode`。

如果你希望把业务数据和 render plan 草稿分开维护，可以再接
`scripts/patch_template_render_plan.ps1`。它会读取一份 base plan 和一份 patch
plan，并按 `bookmark_name` 加可选的 `part/index/section/kind` 选择器，把 patch
里的 `text`、`paragraphs`、`rows`、`visible` 覆盖到 base plan 对应条目上。
当书签名在草稿里唯一时，patch 可以只写 `bookmark_name + payload`；如果你传
`-RequireComplete`，脚本还会检查导出草稿里剩下的 `TODO` 和空表格行占位，避免
半成品 plan 被继续流到渲染阶段。可以直接参考
`samples/chinese_invoice_template.render_patch.json`。

如果你希望把“模板 + patch JSON = 最终 docx”压成一条命令，可以直接用
`scripts/render_template_document_from_patch.ps1`。它会顺序执行导出草稿、
套用 patch、渲染文档三步，并且只解析一次 `featherdoc_cli` 构建结果。patch
阶段默认开启 `-RequireComplete`，避免残留 `TODO` 或空表格行直接流入最终文
档；如果你还想保留中间产物做排查，也可以额外传
`-DraftPlanOutput` 和 `-PatchedPlanOutput`。如果 patch 需要直接命中 section
生效页眉/页脚，可以传 `-ExportTargetMode resolved-section-targets`，让导出步
生成 `section-header` / `section-footer` 选择器。

```bash
pwsh -ExecutionPolicy Bypass -File .\scripts\render_template_document_from_patch.ps1 -InputDocx .\samples\chinese_invoice_template.docx -PatchPlanPath .\samples\chinese_invoice_template.render_patch.json -OutputDocx .\output\rendered\invoice.from-patch.docx -SummaryJson .\output\rendered\invoice.from-patch.summary.json -DraftPlanOutput .\output\rendered\invoice.draft.render-plan.json -PatchedPlanOutput .\output\rendered\invoice.filled.render-plan.json -BuildDir build-codex-clang-compat -SkipBuild
```

如果你希望把编辑面继续往上抬，不再直接改 patch JSON，而是维护一份业务数据
JSON，可以用 `scripts/convert_render_data_to_patch_plan.ps1`。它会读取业务数
据和一份 mapping 文件，把每个 `source` 路径映射到
`bookmark_text`、`bookmark_paragraphs`、`bookmark_table_rows`、
`bookmark_block_visibility` 里的目标条目。转换 summary 会写出
`output_patch_path`，并在生成 patch 前校验 `part/index/section/kind` selector。
mapping 文件可直接参考 `samples/template_render_data_mapping.schema.json` 和
`samples/chinese_invoice_template.render_data_mapping.json`。

如果你已经有 render plan 草稿，但不想从零手写 mapping，也可以先跑
`scripts/export_render_data_mapping_draft.ps1`。它会保留书签目标和可选的
`part/index/section/kind` selector，并把每个 `source` 先生成为一个可编辑的
书签名路径草稿。

```bash
pwsh -ExecutionPolicy Bypass -File .\scripts\export_render_data_mapping_draft.ps1 -InputPlan .\samples\chinese_invoice_template.render_plan.json -OutputMapping .\output\rendered\invoice.render_data_mapping.draft.json -SummaryJson .\output\rendered\invoice.render_data_mapping.draft.summary.json -SourceRoot data
```

如果你想从 `.docx` 模板直接得到一个“可编辑数据工作区”，可以用
`scripts/prepare_template_render_data_workspace.ps1`。它会一次串起
`export_template_render_plan`、`export_render_data_mapping_draft`、
`export_render_data_skeleton` 和 `lint_render_data_mapping`，产出 render plan、
mapping 草稿、可填写的 data skeleton，以及总 summary。这个入口适合先让用户
编辑 JSON 数据，再用 `render_template_document_from_data.ps1` 生成最终文档。
现在 workspace 目录里还会自动附带一份 `START_HERE.zh-CN.md`，直接告诉使用者
应该先改哪个 JSON、如何先校验自己改对了，以及下一条该跑什么渲染命令。
如果 workspace 需要覆盖 section 生效页眉/页脚，可以在准备阶段传
`-ExportTargetMode resolved-section-targets`；summary、推荐校验/渲染命令、
workspace 校验入口和 workspace 渲染入口都会保留同一个 `export_target_mode`。

```bash
pwsh -ExecutionPolicy Bypass -File .\scripts\prepare_template_render_data_workspace.ps1 -InputDocx .\samples\chinese_invoice_template.docx -WorkspaceDir .\output\rendered\invoice.workspace -SummaryJson .\output\rendered\invoice.workspace\summary.json -BuildDir build-codex-clang-compat -SkipBuild
```

准备完 workspace 之后，可以直接用
`scripts/render_template_document_from_workspace.ps1`。它会从 workspace summary
或 workspace 目录里自动找到模板、mapping 和数据 JSON，把用户侧流程收敛成：

1. 先改 data JSON
2. 再用 `validate_render_data_mapping.ps1 -WorkspaceDir ... -RequireComplete` 确认
   `status=completed` 且 `remaining_placeholder_count=0`
3. 最后从 workspace 渲染最终 docx

如果 data JSON 里还保留脚手架生成的 `TODO:` 占位，它会先给出更直白的提示，
而不是直接抛底层 patch/render 错误。校验命令还可以通过 `-ReportMarkdown`
额外写一份人类可读报告，让使用者先打开 `.md` 看结论和未补完的槽位，
不必一开始就读 JSON summary；报告里的未补完槽位还会尽量反查 mapping，提示
建议检查的 data JSON 字段。

```bash
pwsh -ExecutionPolicy Bypass -File .\scripts\validate_render_data_mapping.ps1 -WorkspaceDir .\output\rendered\invoice.workspace -SummaryJson .\output\rendered\invoice.workspace\validation.summary.json -ReportMarkdown .\output\rendered\invoice.workspace\validation.report.md -BuildDir build-codex-clang-compat -SkipBuild -RequireComplete
```

```bash
pwsh -ExecutionPolicy Bypass -File .\scripts\render_template_document_from_workspace.ps1 -WorkspaceDir .\output\rendered\invoice.workspace -OutputDocx .\output\rendered\invoice.final.docx -SummaryJson .\output\rendered\invoice.workspace\render.summary.json
```

如果你的目标不是“用固定模板批量生成文档”，而是直接修改一个已有 `.docx`，
可以使用 `scripts/edit_document_from_plan.ps1`。它读取的是一份 **修改指令 JSON**，
不是把整份 Word 文档转换成 JSON；脚本会按顺序修改原始 `.docx`，最后输出新的
`.docx` 与 summary。这条路径更适合“已有文档 + 修改指令 = 新文档”。

```json
{
  "operations": [
    {
      "op": "replace_bookmark_text",
      "bookmark": "customer_name",
      "text": "上海羽文档科技有限公司"
    },
    {
      "op": "replace_bookmark_paragraphs",
      "bookmark": "note_lines",
      "paragraphs": ["第一段", "第二段"]
    },
    {
      "op": "replace_bookmark_table_rows",
      "bookmark": "line_item_row",
      "rows": [["服务", "说明", "金额"]]
    },
    {
      "op": "replace_bookmark_table",
      "bookmark": "metrics_table",
      "rows": [["指标", "值"], ["质量", "通过"]]
    },
    {
      "op": "remove_bookmark_block",
      "bookmark": "optional_note"
    },
    {
      "op": "replace_bookmark_image",
      "bookmark": "logo",
      "image_path": "assets/logo.png",
      "width": 120,
      "height": 40
    },
    {
      "op": "replace_bookmark_floating_image",
      "bookmark": "hero",
      "image_path": "assets/hero.png",
      "width": 320,
      "height": 180,
      "horizontal_reference": "margin",
      "vertical_reference": "paragraph",
      "wrap_mode": "square"
    },
    {
      "op": "apply_bookmark_block_visibility",
      "show": "totals",
      "hide": "optional_terms"
    },
    {
      "op": "replace_content_control_text",
      "content_control_tag": "order_no",
      "text": "INV-002"
    },
    {
      "op": "replace_content_control_paragraphs",
      "content_control_tag": "summary",
      "paragraphs": ["第一段摘要", "第二段摘要"]
    },
    {
      "op": "replace_content_control_table_rows",
      "content_control_alias": "Line Items",
      "rows": [["SKU-1", "Ready"], ["SKU-2", "Queued"]]
    },
    {
      "op": "set_content_control_form_state",
      "content_control_tag": "approved",
      "checked": false,
      "clear_lock": true
    },
    {
      "op": "set_table_cell_text",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "text": "4,000.00"
    },
    {
      "op": "set_table_cell_fill",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "background_color": "FFF2CC"
    },
    {
      "op": "set_table_cell_width",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "width_twips": 2400
    },
    {
      "op": "set_table_cell_margin",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "edge": "left",
      "margin_twips": 180
    },
    {
      "op": "set_table_cell_text_direction",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "direction": "top_to_bottom_right_to_left"
    },
    {
      "op": "set_table_cell_border",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "edge": "right",
      "style": "single",
      "thickness": 16,
      "color": "FF0000"
    },
    {
      "op": "set_table_row_height",
      "table_index": 0,
      "row_index": 3,
      "height_twips": 720,
      "rule": "exact"
    },
    {
      "op": "set_table_cell_vertical_alignment",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "alignment": "center"
    },
    {
      "op": "set_table_cell_horizontal_alignment",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "alignment": "right"
    },
    {
      "op": "replace_text",
      "find": "旧文本",
      "replace": "新文本"
    },
    {
      "op": "set_text_style",
      "text_contains": "新文本",
      "bold": true,
      "font_family": "Segoe UI",
      "east_asia_font_family": "Microsoft YaHei",
      "language": "en-US",
      "east_asia_language": "zh-CN",
      "color": "C00000",
      "font_size_points": 14
    },
    {
      "op": "delete_paragraph_contains",
      "text_contains": "临时说明"
    },
    {
      "op": "set_paragraph_horizontal_alignment",
      "text_contains": "第一段",
      "alignment": "center"
    },
    {
      "op": "set_paragraph_spacing",
      "text_contains": "第一段",
      "before_twips": 120,
      "after_twips": 240,
      "line_twips": 360,
      "line_rule": "exact"
    },
    {
      "op": "set_table_column_width",
      "table_index": 0,
      "column_index": 1,
      "width_twips": 5200
    },
    {
      "op": "set_table_style_id",
      "table_index": 0,
      "style_id": "TableGrid"
    },
    {
      "op": "set_table_style_look",
      "table_index": 0,
      "first_row": false,
      "last_row": true,
      "first_column": false,
      "last_column": true,
      "banded_rows": false,
      "banded_columns": true
    },
    {
      "op": "set_table_width",
      "table_index": 0,
      "width_twips": 7200
    },
    {
      "op": "set_table_layout_mode",
      "table_index": 0,
      "layout_mode": "fixed"
    },
    {
      "op": "set_table_alignment",
      "table_index": 0,
      "alignment": "center"
    },
    {
      "op": "set_table_indent",
      "table_index": 0,
      "indent_twips": 360
    },
    {
      "op": "set_table_cell_spacing",
      "table_index": 0,
      "cell_spacing_twips": 180
    },
    {
      "op": "set_table_default_cell_margin",
      "table_index": 0,
      "edge": "left",
      "margin_twips": 240
    },
    {
      "op": "set_table_border",
      "table_index": 0,
      "edge": "inside_horizontal",
      "style": "dashed",
      "size": 8,
      "color": "00AA00",
      "space": 2
    },
    {
      "op": "merge_table_cells",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 0,
      "direction": "right",
      "count": 2
    }
  ]
}
```

其中，单元格垂直对齐支持 `top` / `center` / `bottom` / `both`；
单元格和普通正文段落的水平对齐支持 `left` / `center` / `right` / `both`。
表格版式可以设置样式 id、总宽度、布局模式、水平对齐、缩进、
单元格间距、表格默认单元格边距、表格边框、显式单元格宽度和列宽，也可以按
`right` / `down` 方向合并或取消合并单元格；
单元格外观可以设置背景色、单边框样式、边框粗细、边框颜色；
文本样式可以设置加粗、字体颜色、字号、英文字体、中文字体和语言标签；
段落排版可以设置段前距、段后距和行距。
普通段落可以用 `paragraph_index` 精确定位，也可以用 `text_contains` 找到包含指定文本的段落。
如果文档没有书签，可以用 `replace_text` 做
普通正文替换；它按段落合并可见文字后查找，能覆盖 Word 把文字拆成多个 run 的常见情况。
如果要让“垂直居中”在 Word 里更明显，通常还要配合 `set_table_row_height` 给行一个明确高度。
如果编辑计划需要使用 CLI 已有的书签高级替换和条件块能力，可以用
`replace_bookmark_table`、`remove_bookmark_block`、`replace_bookmark_image`、
`replace_bookmark_floating_image`、`set_bookmark_block_visibility` 和
`apply_bookmark_block_visibility`。
如果编辑计划需要一次性填充书签文本，可以用 `fill_bookmarks`，它支持
`values` 对象或 `bindings` 数组来提供书签和值的对应关系。
如果编辑计划需要按内容控件定位，可以用
`replace_content_control_text`、`replace_content_control_paragraphs`、
`replace_content_control_table`、`replace_content_control_table_rows`、
`replace_content_control_image` 和 `set_content_control_form_state`，通过
`content_control_tag` 或 `content_control_alias` 选择目标；需要从内嵌
Custom XML 刷新数据绑定内容控件时，使用 `sync_content_controls_from_custom_xml`；`sync_content_control_from_custom_xml` 也可作为同一 edit-plan 别名。
需要清理审阅状态或批量维护批注时，可以用 `accept_all_revisions`、
`reject_all_revisions`、`set_comment_resolved`、`set_comment_metadata`、
`replace_comment` 和 `remove_comment`。批注类操作会转交给
`apply-review-mutation-plan`，因此可继续使用 `expected_text`、
`expected_comment_text`、`expected_resolved` 和 `expected_parent_index`
做预检保护；`replace_comment` 也接受 `text` 作为 `comment_text` 的简写。
需要一次性套用完整审阅变更计划时，可以使用
`apply_review_mutation_plan`；它接受 `plan_file` / `review_plan_file`，
也接受内联 `review_plan` / `operations`。
需要在编辑计划里创建修订时，可以使用 `append_insertion_revision`、
`append_deletion_revision`、`insert_run_revision_after`、
`delete_run_revision` 和 `replace_run_revision`。修订作者化操作支持可选
`author` 与 `date`；会产生新文本的操作需要 `text`。
`insert_paragraph_text_revision`、`delete_paragraph_text_revision` 和
`replace_paragraph_text_revision` 可用于段落内文本范围；
`insert_text_range_revision`、`delete_text_range_revision` 和
`replace_text_range_revision` 可用于跨段文本范围。这些范围操作支持
`paragraph_index` / `start_paragraph_index`、`text_offset` / `start_offset`、
`end_paragraph_index`、`end_offset`、`length`，并在对应 CLI 支持时接受
`expected_text` 预检保护。单条修订清理和元数据维护可用
`accept_revision`、`reject_revision` 和 `set_revision_metadata`。
需要新增批注或回复时，可以使用 `append_comment` 和
`append_comment_reply`。`append_comment` 通过 `selected_text` 或
`anchor_text` 锚定正文，`append_comment_reply` 通过
`parent_comment_index` 锚定父批注；批注正文可用 `comment_text` 或 `text`，
并支持可选 `author`、`initials` 和 `date`。
需要把批注锚定到段落内或跨段文本范围时，可以使用
`append_paragraph_text_comment` 和 `append_text_range_comment`；已有批注锚点可用
`set_paragraph_text_comment_range` 与 `set_text_range_comment_range` 重新定位。
需要在编辑计划里维护脚注或尾注时，可以使用 `append_footnote`、
`replace_footnote`、`remove_footnote` 以及 `append_endnote`、
`replace_endnote`、`remove_endnote`。追加操作需要 `reference_text` 与
`note_text`，也支持用 `selected_text` 或 `anchor_text` 作为
`reference_text` 的别名；替换和移除操作可通过 `note_index`、
`footnote_index`、`endnote_index` 或 `index` 定位目标，其中替换操作还需要
`note_text`。`text` 与 `body` 也可作为 `note_text` 的简写。
需要维护普通超链接 run 时，可以使用 `append_hyperlink`、
`replace_hyperlink` 和 `remove_hyperlink`。追加与替换操作接受 `text`，以及
`target`、`url` 或 `href`；移除操作可用 `hyperlink_index` 或 `index` 定位。
需要维护 Word 公式时，可以使用 `append_omml`、`replace_omml` 和
`remove_omml`。追加与替换操作接受 `xml`、`omml` 或 `omml_xml`；移除操作可用
`formula_index`、`omml_index` 或 `index` 定位。
需要维护正文、页眉、页脚或节级 part 里的普通 drawing 图片时，可以使用
`append_image`、`replace_image` 和 `remove_image`。`append_image` 支持和 CLI
`append-image` 一致的尺寸、浮动图、环绕和裁剪参数；`replace_image` 与
`remove_image` 可以通过 `image_index`、`relationship_id` 或
`image_entry_name` 定位已有图片。
需要通过编辑计划追加常用 Word 字段时，可以使用
`append_page_number_field`、`append_total_pages_field`、
`append_table_of_contents_field`、`append_document_property_field` 和
`append_date_field`。字段操作默认写入 `part: "body"`，也接受 `part`、
`index` / `part_index`、`section`、`kind`、`dirty` 和 `locked`；目录字段支持
`min_outline_level`、`max_outline_level`、`no_hyperlinks`、
`show_page_numbers_in_web_layout`、`no_outline_levels` 和 `result_text`。
文档属性字段与日期字段还支持 `result_text` 和
`preserve_formatting: false`，日期字段可用 `format` 或 `date_format` 指定
Word 日期格式。需要控制文档级打开时更新字段开关时，可以使用
`set_update_fields_on_open`、`enable_update_fields_on_open`、
`disable_update_fields_on_open` 或 `clear_update_fields_on_open`。
需要追加更丰富的字段对象时，可以使用
`append_page_reference_field`、`append_style_reference_field`、
`append_hyperlink_field`、`append_caption`、
`append_index_entry_field`、`append_index_field` 和
`append_complex_field`。页码/样式引用字段支持 `result_text`、
`relative_position` 和 `preserve_formatting: false`，其中页码引用还支持
`bookmark_name` / `bookmark` 与 `no_hyperlink`；超链接字段支持 `target`、
`anchor`、`tooltip` 和 `result_text`；题注支持
`label`、`caption_text` / `text`、`number_result_text`、`number_format`、
`restart` 和 `separator`；索引项字段支持 `entry_text`、`subentry`、
`bookmark`、`cross_reference`、`bold_page_number` 和
`italic_page_number`；索引字段支持 `columns`；复杂字段支持直接提供
`instruction`，或使用 `instruction_before` / `nested_instruction` /
`nested_result_text` / `instruction_after` 的嵌套形式。
当 edit-plan 需要通过成熟 CLI 统一处理默认 / 样式元数据、段落 / run
格式、编号或 section 页面设置时，可以使用
`set_default_run_properties`、`clear_default_run_properties`、
`set_style_run_properties`、`clear_style_run_properties`、
`set_paragraph_style_properties`、`clear_paragraph_style_properties`、
`set_paragraph_style_numbering`、`clear_paragraph_style_numbering`、
`set_paragraph_style`、`clear_paragraph_style`、`set_run_style`、
`clear_run_style`、`set_run_font_family`、`clear_run_font_family`、
`set_run_language`、`clear_run_language`、`set_paragraph_numbering`、
`set_paragraph_list`、`restart_paragraph_list`、`clear_paragraph_list` 和
`set_section_page_setup`。段落选择器使用 `paragraph_index` /
`paragraph`；run 级操作还需要 `run_index` / `run`；样式目标操作接受
`style_id` / `style`。默认 / 样式 run 属性写入支持
`font_family` / `font`、`east_asia_font_family` / `east_asia_font`、
`language` / `lang`、`east_asia_language` / `east_asia_lang`、
`bidi_language` / `bidi_lang`、`rtl` 和 `paragraph_bidi`；clear 变体既支持
同名布尔字段，也支持 `clear_*` 布尔字段，并额外支持
`primary_language`。段落样式属性写入支持 `based_on`、
`based_on_style`、`based_on_style_id`、`next_style`、`next_style_id`、
`next`、`outline_level` 或 `level`。段落样式编号支持
`definition_name` / `name`、`numbering_levels` / `levels` /
`definition_levels` 以及可选 `style_level` / `level`，其中每个级别既可以
直接写 CLI 字符串，也可以写成包含 `level`、`kind`、可选 `start` 和
`text_pattern` / `pattern` 的对象。段落编号继续支持 `definition`、
`definition_id`、`numbering_definition`、`numbering_definition_id` 或
`abstract_num_id`，并可附带 `level`；列表操作接受 `kind` 与可选
`level`；page setup 接受 `section_index` / `section`，以及
`orientation`、`width`、`height`、各类 `margin_*` 字段、
`page_number_start` 或 `clear_page_number_start`。分节与页眉页脚 part
也可以通过 `insert_section`、`remove_section` / `delete_section`、
`move_section`、`copy_section_layout`、`set_section_header`、
`set_section_footer`、`assign_section_header`、`assign_section_footer`、
`remove_section_header`、`remove_section_footer`、`remove_header_part`、
`remove_footer_part`、`move_header_part` 和 `move_footer_part` 进入
edit-plan。section 命令接受 `section_index` / `section`，移动和复制命令接受
`source_index` / `source` / `from_index` / `from` 以及
`target_index` / `target` / `to_index` / `to` /
`destination_index` / `destination`，页眉页脚引用类型接受 `kind` /
`reference_kind` / `section_part_kind` / `header_footer_kind`。设置页眉页脚时，
`text` / `content` 会写入临时文本文件，也可以直接传
`text_file` / `text_path` / `content_file` / `content_path`；`insert_section`
默认继承页眉页脚引用，可用 `no_inherit: true` 或
`inherit_header_footer: false` 关闭。`ensure_numbering_definition`
可以创建或更新编号定义，但不会自动绑定到样式。`ensure_paragraph_style`
和 `ensure_character_style` 可以按 `style_id` / `style` 创建或更新样式定义，
并支持 `name` / `style_name` / `display_name`、`based_on` /
`based_on_style` / `based_on_style_id`、`custom` / `is_custom`、
`semi_hidden` / `is_semi_hidden`、`unhide_when_used` /
`is_unhide_when_used`、`quick_format` / `is_quick_format`，以及与 run
属性命令相同的元数据字段。段落样式还支持 `next_style` /
`next_style_id` / `next`、`paragraph_bidi` 和 `outline_level` /
`level`。`materialize_style_run_properties` 会把某个样式解析后的 run
属性落成具体的样式 XML。`rebase_character_style_based_on` 和
`rebase_paragraph_style_based_on` 可以用 `based_on`、
`based_on_style`、`based_on_style_id`、`target_based_on`、
`target_style` 或 `target_style_id` 重新指定样式基准。`ensure_style_linked_numbering`
在相同的编号定义输入之外，还接受 `style_links` / `links`；每个关联既
可以写成 `StyleId:level` 字符串，也可以写成包含 `style` /
`style_id` 和 `level` 的对象。
样式重构命令也可以通过 `rename_style`、`merge_style`、
`apply_style_refactor`、`restore_style_merge` 和 `prune_unused_styles` 进入
edit-plan。`rename_style` 接受 `old_style_id` / `old_style` 和
`new_style_id` / `new_style`；`merge_style` 接受 `source_style_id` /
`source_style` 和 `target_style_id` / `target_style`；`apply_style_refactor`
接受 `plan_file` 或内联 `renames` / `merges` pair，并可选写出
`rollback_plan`。`restore_style_merge` 接受 `rollback_plan`，并可选使用
`entries`、`source_style_ids` 或 `target_style_ids` 过滤要恢复的 merge
rollback 项。
修复型样式 / 表格命令也可以通过 `apply_table_style_quality_fixes`、
`repair_table_style_look` 和 `repair_style_numbering` 进入 edit-plan；
`ensure_table_style` 也可以通过 edit-plan 进入；它接受 `style_id`
以及与 `ensure-table-style` 相同的表格样式目录和区域输入。
脚本会调用这些命令的安全应用模式，并把中间 DOCX 继续传给下一步。
`repair_style_numbering` 还可以传入 `catalog_file`。需要把保存好的编号
catalog 导入当前 edit-plan 文档时，使用 `import_numbering_catalog`；它接受
`catalog_file`、`catalog_path`、`numbering_catalog_file` 或
`numbering_catalog_path`。
单元格合并使用 `merge_table_cells`，取消合并使用 `unmerge_table_cells`；
合并可通过 `count` 指定跨几个单元格；需要删除整张正文表格时用
`remove_table`。

需要围绕现有正文表格新建空白同级表格时，使用 `insert_table_before` / `insert_table_after`。
需要克隆现有表格结构和样式但清空单元格文本时，使用 `insert_table_like_before` /
`insert_table_like_after`。
需要在选中正文表格后紧接着插入普通正文段落时，使用 `insert_paragraph_after_table`。
需要在编辑计划中设置或清除浮动表格 `w:tblpPr` 定位时，使用 `set_table_position` /
`clear_table_position`。需要回放 `plan-table-position-presets` 生成的 JSON
计划时，使用 `apply_table_position_plan`；它接受 `plan_file` /
`table_position_plan_file`，也接受内联 `table_position_plan` / `plan` 对象。
需要通过编辑计划追加、插入或删除正文表格行时，使用 `append_table_row`、
`insert_table_row_before`、`insert_table_row_after`、`remove_table_row`。
如果同样的行编辑需要作用在模板 part 表格上，可以使用
`append_template_table_row`、`insert_template_table_row_before`、
`insert_template_table_row_after`、`remove_template_table_row` 或
`delete_template_table_row`，并通过表格索引、书签或文本选择器定位目标表格。
需要通过同一套选择器克隆或删除模板 part 表格列时，使用
`insert_template_table_column_before`、`insert_template_table_column_after`、
`remove_template_table_column` 或 `delete_template_table_column`。
需要通过编辑计划更新模板表格整行文本、单元格文本或矩形文本块时，使用
`set_template_table_row_texts`、`set_template_table_cell_text` 或
`set_template_table_cell_block_texts`。
需要通过 JSON 合并或拆分模板 part 表格单元格时，使用
`merge_template_table_cells` 与 `unmerge_template_table_cells`。
需要把 CLI 的模板表格 JSON patch 格式纳入编辑计划时，使用
`set_template_table_from_json` 或 `set_template_tables_from_json`，分别应用到单表或批量表格选择器。
需要通过编辑计划控制正文表格行级布局元数据时，使用 `set_table_row_height`、
`clear_table_row_height`、`set_table_row_cant_split`、
`clear_table_row_cant_split`、`set_table_row_repeat_header`、
`clear_table_row_repeat_header`。
需要通过编辑计划控制单元格级外观元数据时，使用 `set_table_cell_fill`、
`clear_table_cell_fill`、`set_table_cell_width`、`clear_table_cell_width`、
`set_table_cell_margin`、`clear_table_cell_margin`、`set_table_cell_text_direction`、
`clear_table_cell_text_direction`、`set_table_cell_border`、`clear_table_cell_border`。
需要移除单元格直接外观或对齐覆盖时，也可以使用 `clear_table_cell_fill`、
`clear_table_cell_border`、`clear_table_cell_vertical_alignment`、
`clear_table_cell_horizontal_alignment`。
需要通过编辑计划插入或删除正文表格列时，使用 `insert_table_column_before`、
`insert_table_column_after`、`remove_table_column`。

```bash
pwsh -ExecutionPolicy Bypass -File .\scripts\edit_document_from_plan.ps1 -InputDocx .\samples\chinese_invoice_template.docx -EditPlan .\output\rendered\invoice.edit_plan.json -OutputDocx .\output\rendered\invoice.edited.docx -SummaryJson .\output\rendered\invoice.edit.summary.json -BuildDir build-codex-clang-compat -SkipBuild
```

如果要确认修改后的 `.docx` 不是“XML 里变了但页面效果没看过”，可以跑直接编辑流的
Word 可视化回归。它会先生成 edited DOCX，再用 Microsoft Word 导出 PDF、渲染 PNG，
并生成编辑前 / 编辑后的对比图：

```bash
pwsh -ExecutionPolicy Bypass -File .\scripts\run_edit_document_from_plan_visual_regression.ps1 -BuildDir build-codex-clang-compat -SkipBuild
```

```bash
pwsh -ExecutionPolicy Bypass -File .\scripts\convert_render_data_to_patch_plan.ps1 -DataPath .\samples\chinese_invoice_template.render_data.json -MappingPath .\samples\chinese_invoice_template.render_data_mapping.json -OutputPatch .\output\rendered\invoice.generated.render_patch.json -SummaryJson .\output\rendered\invoice.generated.render_patch.summary.json
```

编辑 mapping 时，如果想要更快反馈，可以先跑
`scripts/lint_render_data_mapping.ps1`。它不打开 `.docx` 模板，只检查
selector 组合、同一 category 内的重复目标，以及可选业务数据里的 `source`
路径和表格列路径是否能解析。

```bash
pwsh -ExecutionPolicy Bypass -File .\scripts\lint_render_data_mapping.ps1 -MappingPath .\samples\chinese_invoice_template.render_data_mapping.json -DataPath .\samples\chinese_invoice_template.render_data.json -SummaryJson .\output\rendered\invoice.mapping.lint.summary.json
```

在真正渲染之前，还可以先用
`scripts/validate_render_data_mapping.ps1` 校验整条
`template + mapping + data` 契约。这个脚本会先导出 render plan 草稿，再把
业务数据转换成 patch，最后把 patch 回放到草稿上；因此书签没映射、`source`
路径写错、重复命中同一槽位，或者 `-RequireComplete` 下仍然残留占位内容，都会在
生成最终 docx 之前直接失败。它也支持 `-ExportTargetMode
resolved-section-targets`，用于校验直接指向 section 生效页眉/页脚的 mapping；
如果从 workspace 运行且未显式传参，则会自动沿用 workspace summary 里的
`export_target_mode`，并写进 JSON summary 与 Markdown report。

```bash
pwsh -ExecutionPolicy Bypass -File .\scripts\validate_render_data_mapping.ps1 -InputDocx .\samples\chinese_invoice_template.docx -MappingPath .\samples\chinese_invoice_template.render_data_mapping.json -DataPath .\samples\chinese_invoice_template.render_data.json -SummaryJson .\output\rendered\invoice.validation.summary.json -DraftPlanOutput .\output\rendered\invoice.validation.draft.render-plan.json -GeneratedPatchOutput .\output\rendered\invoice.validation.generated.render_patch.json -PatchedPlanOutput .\output\rendered\invoice.validation.patched.render-plan.json -BuildDir build-codex-clang-compat -SkipBuild -RequireComplete
```

如果你想直接走“模板 + 业务数据 JSON + mapping = 最终 docx”，可以再用
`scripts/render_template_document_from_data.ps1`。它会按
“导出草稿 -> 生成 patch -> 回放 patch -> 渲染文档”这四步直接跑完整条链路，
并且默认沿用 patch 阶段的 `-RequireComplete` 语义；需要时也能通过
`-PatchPlanOutput`、`-DraftPlanOutput`、`-PatchedPlanOutput`
把中间产物保留下来。如果 mapping 需要覆盖 section 生效页眉/页脚，可以传
`-ExportTargetMode resolved-section-targets`，让导出和后续 patch 都保留
`section-header` / `section-footer` 选择器。

```bash
pwsh -ExecutionPolicy Bypass -File .\scripts\render_template_document_from_data.ps1 -InputDocx .\samples\chinese_invoice_template.docx -DataPath .\samples\chinese_invoice_template.render_data.json -MappingPath .\samples\chinese_invoice_template.render_data_mapping.json -OutputDocx .\output\rendered\invoice.from-data.docx -SummaryJson .\output\rendered\invoice.from-data.summary.json -PatchPlanOutput .\output\rendered\invoice.generated.render_patch.json -DraftPlanOutput .\output\rendered\invoice.draft.render-plan.json -PatchedPlanOutput .\output\rendered\invoice.filled.render-plan.json -BuildDir build-codex-clang-compat -SkipBuild
```

`move-header-part` / `move-footer-part` 用来重排当前文档里已加载的
header/footer part 索引顺序，同时保持各 section 继续指向原来的关系 id，
不会因为重排索引就把引用绑错。

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
featherdoc_cli inspect-default-run-properties input.docx --json
featherdoc_cli set-default-run-properties input.docx --font-family "Segoe UI" --east-asia-font-family "Microsoft YaHei" --language en-US --east-asia-language zh-CN --rtl true --output default-run-properties.docx --json
featherdoc_cli clear-default-run-properties input.docx --primary-language --rtl --output cleared-default-run-properties.docx --json
featherdoc_cli inspect-style-run-properties input.docx Normal --json
featherdoc_cli materialize-style-run-properties input.docx Normal --output materialized-style-run-properties.docx --json
featherdoc_cli set-style-run-properties input.docx Normal --font-family "Segoe UI" --east-asia-font-family "Microsoft YaHei" --language en-US --east-asia-language zh-CN --rtl true --paragraph-bidi true --output style-run-properties.docx --json
featherdoc_cli clear-style-run-properties input.docx Normal --primary-language --rtl --paragraph-bidi --output cleared-style-run-properties.docx --json
featherdoc_cli inspect-style-inheritance input.docx Normal --json
featherdoc_cli inspect-styles input.docx --usage --json
featherdoc_cli rename-style input.docx LegacyBody ReviewBody --output renamed-style.docx --json
featherdoc_cli merge-style input.docx LegacyBody ReviewBody --output merged-style.docx --json
featherdoc_cli plan-style-refactor input.docx --rename LegacyBody:ReviewBody --merge OldBody:Normal --output-plan style-refactor.plan.json --json
featherdoc_cli suggest-style-merges input.docx --output-plan style-merge-suggestions.json --json
featherdoc_cli suggest-style-merges input.docx --confidence-profile recommended --fail-on-suggestion --output-plan style-merge-suggestions.recommended.json --json
featherdoc_cli suggest-style-merges input.docx --min-confidence 90 --output-plan style-merge-suggestions.custom.json --json
featherdoc_cli suggest-style-merges input.docx --source-style DuplicateBodyC --target-style DuplicateBodyA --json
featherdoc_cli apply-style-refactor input.docx --plan-file style-refactor.plan.json --rollback-plan style-refactor.rollback.json --output refactored-styles.docx --json
featherdoc_cli apply-style-refactor input.docx --plan-file style-merge-suggestions.json --rollback-plan style-merge.rollback.json --output merged-styles.docx --json
featherdoc_cli restore-style-merge merged-styles.docx --rollback-plan style-merge.rollback.json --entry 0 --entry 2 --dry-run --json
featherdoc_cli restore-style-merge merged-styles.docx --rollback-plan style-merge.rollback.json --source-style OldBody --target-style Normal --dry-run --json
featherdoc_cli restore-style-merge merged-styles.docx --rollback-plan style-merge.rollback.json --output restored-styles.docx --json
featherdoc_cli plan-prune-unused-styles input.docx --json
featherdoc_cli prune-unused-styles input.docx --output pruned-styles.docx --json
featherdoc_cli inspect-paragraph-style-properties input.docx Heading1 --json
featherdoc_cli set-paragraph-style-properties input.docx Heading1 --next-style BodyText --outline-level 1 --output updated-paragraph-style-properties.docx --json
featherdoc_cli clear-paragraph-style-properties input.docx Heading1 --next-style --outline-level --output cleared-paragraph-style-properties.docx --json
featherdoc_cli rebase-character-style-based-on input.docx ReviewStrong Strong --output rebased-character-style.docx --json
featherdoc_cli rebase-paragraph-style-based-on input.docx Heading2 Normal --output rebased-paragraph-style.docx --json
featherdoc_cli ensure-paragraph-style input.docx ReviewHeading --name "Review Heading" --based-on Heading1 --output ensured-paragraph-style.docx --json
featherdoc_cli ensure-character-style input.docx ReviewStrong --name "Review Strong" --based-on Strong --output ensured-character-style.docx --json
featherdoc_cli ensure-numbering-definition input.docx --definition-name OutlineReview --numbering-level 0:decimal:1:%1. --output numbering.docx --json
featherdoc_cli inspect-style-numbering input.docx --json
featherdoc_cli audit-style-numbering input.docx --fail-on-issue --json
featherdoc_cli repair-style-numbering input.docx --plan-only --json
featherdoc_cli repair-style-numbering input.docx --apply --output repaired-style-numbering.docx --json
featherdoc_cli repair-style-numbering input.docx --catalog-file numbering-catalog.json --apply --output catalog-repaired.docx --json
featherdoc_cli export-numbering-catalog input.docx --output numbering-catalog.json --json
featherdoc_cli check-numbering-catalog input.docx --catalog-file numbering-catalog.json --output numbering-catalog.generated.json --json
pwsh -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_baseline.ps1 -InputDocx .\input.docx -CatalogFile .\numbering-catalog.json -GeneratedCatalogOutput .\numbering-catalog.generated.json -BuildDir build-codex-clang-compat -SkipBuild
pwsh -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_manifest.ps1 -ManifestPath .\baselines\numbering-catalog\manifest.json -BuildDir build-codex-clang-compat -OutputDir .\output\numbering-catalog-manifest-checks -SkipBuild
featherdoc_cli patch-numbering-catalog numbering-catalog.json --patch-file numbering-catalog.patch.json --output numbering-catalog.patched.json --json
featherdoc_cli lint-numbering-catalog numbering-catalog.patched.json --json
featherdoc_cli diff-numbering-catalog numbering-catalog.json numbering-catalog.patched.json --fail-on-diff --json
featherdoc_cli import-numbering-catalog target.docx --catalog-file numbering-catalog.patched.json --output target-numbering.docx --json
featherdoc_cli ensure-style-linked-numbering input.docx --definition-name HeadingReview --numbering-level 0:decimal:1:%1. --numbering-level 1:decimal:1:%1.%2. --style-link Heading1:0 --style-link Heading2:1 --output linked-style-numbering.docx --json
featherdoc_cli set-paragraph-numbering input.docx 6 --definition 12 --level 0 --output numbered.docx --json
featherdoc_cli set-paragraph-style-numbering input.docx Heading2 --definition-name HeadingReview --numbering-level 0:decimal:1:%1. --style-level 1 --output style-numbering.docx --json
featherdoc_cli clear-paragraph-style-numbering input.docx Heading2 --output cleared-style-numbering.docx --json
featherdoc_cli set-paragraph-list input.docx 6 --kind bullet --level 1 --output bulleted.docx --json
featherdoc_cli restart-paragraph-list input.docx 10 --kind decimal --level 0 --output restarted-list.docx --json
featherdoc_cli clear-paragraph-list input.docx 10 --output cleared-list.docx --json

# 正文表格与行列元数据
featherdoc_cli inspect-tables input.docx --table 0 --json
featherdoc_cli plan-table-position-presets input.docx --preset paragraph-callout --json
featherdoc_cli plan-table-position-presets input.docx --preset paragraph-callout --fail-on-change --json
featherdoc_cli plan-table-position-presets input.docx --preset paragraph-callout --output positioned.docx --output-plan table-position-plan.json --json
featherdoc_cli apply-table-position-plan table-position-plan.json --dry-run --json
featherdoc_cli apply-table-position-plan table-position-plan.json --json
# plan 输出 input_path、分类索引、table_fingerprints 和 recommended_* 模板命令；resolved_* 安全命令默认派生 --output，也可由 --output 覆盖；apply-table-position-plan --dry-run 可先校验指纹且不写文件，并返回 table_count / fingerprint_checked_count 等统计；缺少 table_fingerprints 的计划会被拒绝，指纹不匹配会输出变化字段，正式回放也会校验后再保存
# edit-plan 可通过 apply_table_position_plan 回放同一类计划，支持 plan_file 或内联 table_position_plan 对象
featherdoc_cli set-table-position input.docx 0 --preset page-corner --horizontal-offset 360 --bottom-from-text 288 --output table-position-preset.docx --json
featherdoc_cli set-table-position input.docx all --preset paragraph-callout --output table-position-all.docx --json
featherdoc_cli set-table-position input.docx 0 --table 1 --preset margin-anchor --output table-position-list.docx --json
featherdoc_cli set-table-position input.docx 0 --horizontal-reference page --horizontal-offset 720 --vertical-reference paragraph --vertical-offset -120 --left-from-text 144 --right-from-text 288 --top-from-text 72 --bottom-from-text 216 --overlap never --output table-position.docx --json
featherdoc_cli clear-table-position table-position-all.docx all --output table-position-all-cleared.docx --json
featherdoc_cli clear-table-position table-position-list.docx 0 --table 1 --output table-position-list-cleared.docx --json
featherdoc_cli clear-table-position table-position.docx 0 --output table-position-cleared.docx --json
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
featherdoc_cli set-table-column-width input.docx 0 1 2400 --output column-width.docx --json
featherdoc_cli clear-table-column-width input.docx 0 1 --output column-width-cleared.docx --json
featherdoc_cli set-table-style-id input.docx 0 TableGrid --output table-style-id.docx --json
featherdoc_cli clear-table-style-id input.docx 0 --output table-style-id-cleared.docx --json
featherdoc_cli set-table-style-look input.docx 0 --first-row false --last-row true --first-column false --last-column true --banded-rows false --banded-columns true --output table-style-look.docx --json
featherdoc_cli clear-table-style-look input.docx 0 --output table-style-look-cleared.docx --json
featherdoc_cli set-table-width input.docx 0 7200 --output table-width.docx --json
featherdoc_cli clear-table-width input.docx 0 --output table-width-cleared.docx --json
featherdoc_cli set-table-layout-mode input.docx 0 fixed --output table-layout-fixed.docx --json
featherdoc_cli clear-table-layout-mode input.docx 0 --output table-layout-cleared.docx --json
featherdoc_cli set-table-alignment input.docx 0 center --output table-aligned.docx --json
featherdoc_cli clear-table-alignment input.docx 0 --output table-alignment-cleared.docx --json
featherdoc_cli set-table-indent input.docx 0 360 --output table-indented.docx --json
featherdoc_cli clear-table-indent input.docx 0 --output table-indent-cleared.docx --json
featherdoc_cli set-table-cell-spacing input.docx 0 180 --output table-spacing.docx --json
featherdoc_cli clear-table-cell-spacing input.docx 0 --output table-spacing-cleared.docx --json
featherdoc_cli set-table-default-cell-margin input.docx 0 left 240 --output table-default-margin.docx --json
featherdoc_cli clear-table-default-cell-margin input.docx 0 left --output table-default-margin-cleared.docx --json
featherdoc_cli set-table-border input.docx 0 inside_horizontal --style dashed --size 8 --color 00AA00 --space 2 --output table-border.docx --json
featherdoc_cli clear-table-border input.docx 0 inside_horizontal --output table-border-cleared.docx --json
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
featherdoc_cli remove-table input.docx 0 --output table-removed.docx --json
featherdoc_cli insert-table-before input.docx 0 1 2 --output table-before.docx --json
featherdoc_cli insert-table-after input.docx 0 1 2 --output table-after.docx --json
featherdoc_cli insert-table-like-before input.docx 0 --output table-like-before.docx --json
featherdoc_cli insert-table-like-after input.docx 0 --output table-like-after.docx --json
featherdoc_cli insert-paragraph-after-table input.docx 0 --text "表格后说明" --output paragraph-after-table.docx --json
featherdoc_cli insert-table-column-before input.docx 0 1 1 --output column-before.docx --json
featherdoc_cli insert-table-column-after input.docx 0 1 1 --output column-after.docx --json
featherdoc_cli remove-table-column input.docx 0 1 1 --output column-removed.docx --json
featherdoc_cli merge-table-cells input.docx 0 0 0 --direction right --count 2 --output merged.docx --json
featherdoc_cli unmerge-table-cells input.docx 0 0 0 --direction right --output unmerged.docx --json

# 模板部件、模板表格、书签、图片、页码字段
featherdoc_cli inspect-template-paragraphs input.docx --part header --index 0 --paragraph 0 --json
featherdoc_cli inspect-content-controls input.docx --part body --alias "Customer Name" --json
featherdoc_cli semantic-diff before.docx after.docx --json
featherdoc_cli semantic-diff before.docx after.docx --index-alignment --json
featherdoc_cli replace-content-control-text input.docx --part body --alias "Customer Name" --text "Ada Lovelace" --output content-control-text.docx --json
featherdoc_cli set-content-control-form-state input.docx --part body --tag status --selected-item draft --lock sdtLocked --output content-control-form.docx --json
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
featherdoc_cli fill-bookmarks input.docx --set-file customer_name customer_name.txt --set-file invoice_no invoice_no.txt --output filled-from-files.docx --json
featherdoc_cli replace-bookmark-paragraphs input.docx notes --paragraph "Line one" --paragraph "Line two" --output bookmark-paragraphs.docx --json
featherdoc_cli replace-bookmark-paragraphs input.docx notes --paragraph-file note-1.txt --paragraph-file note-2.txt --output bookmark-paragraphs-files.docx --json
featherdoc_cli replace-bookmark-table input.docx line_items --row "SKU-1" --cell "2" --cell "$10" --output bookmark-table.docx --json
featherdoc_cli replace-bookmark-table-rows input.docx line_items --row "SKU-2" --cell "4" --cell "$20" --output bookmark-table-rows.docx --json
featherdoc_cli replace-bookmark-table-rows input.docx line_items --row-file sku.txt --cell-file qty.txt --cell-file price.txt --output bookmark-table-rows-files.docx --json
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
featherdoc_cli append-table-of-contents-field input.docx --part body --min-outline-level 1 --max-outline-level 3 --output toc.docx --json
featherdoc_cli append-field input.docx " AUTHOR " --part body --result-text "Ada Lovelace" --output author-field.docx --json
featherdoc_cli append-reference-field input.docx target_heading --part body --output ref.docx --json
featherdoc_cli append-page-reference-field input.docx target_heading --part body --relative-position --output page-ref.docx --json
featherdoc_cli append-date-field input.docx --part body --format "yyyy-MM-dd" --dirty --output date-field.docx --json
featherdoc_cli append-hyperlink-field input.docx https://example.com/report --part body --result-text "Open report" --locked --output hyperlink-field.docx --json
featherdoc_cli append-sequence-field input.docx Figure --part body --number-format ROMAN --restart 4 --output sequence.docx --json
featherdoc_cli replace-field input.docx 0 " SEQ Table \* ARABIC \r 1 " --part body --result-text "1" --output replaced-field.docx --json
featherdoc_cli append-caption input.docx Figure --part body --text "Architecture overview" --output caption.docx --json
featherdoc_cli append-index-field input.docx --part body --columns 2 --output index.docx --json
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
- 做模板填充、书签校验或 content control 检查 / 替换：`list_bookmarks()`、`validate_template()`、`fill_bookmarks()`、`replace_bookmark_with_*()`、`body_template()` / `header_template()` / `footer_template()`、`list_content_controls()`、`find_content_controls_by_*()`、`replace_content_control_text_by_*()`、`set_content_control_form_state_by_*()`、`replace_content_control_with_*()`
- 处理图片和字段：`append_image()`、`append_floating_image()`、`replace_*image()`、`list_fields()`、`append_field()`、`append_table_of_contents_field()`、`append_reference_field()`、`append_page_reference_field()`、`append_style_reference_field()`、`append_document_property_field()`、`append_date_field()`、`append_hyperlink_field()`、`append_sequence_field()`、`append_caption()`、`append_index_entry_field()`、`append_index_field()`、`append_complex_field()`、`replace_field()`、`append_page_number_field()`、`append_total_pages_field()`、`enable_update_fields_on_open()`、`clear_update_fields_on_open()`
- 处理超链接、审阅对象和 OMML 公式：`list_hyperlinks()`、`append_hyperlink()`、`replace_hyperlink()`、`remove_hyperlink()`、`list_footnotes()` / `append_footnote()` / `replace_footnote()` / `remove_footnote()`、`list_endnotes()` / `append_endnote()` / `replace_endnote()` / `remove_endnote()`、`list_comments()` / `append_comment()` / `replace_comment()` / `remove_comment()`、`list_revisions()`、`accept_*revision*()`、`reject_*revision*()`、`list_omml()`、`append_omml()`、`replace_omml()`、`remove_omml()`、`make_omml_*()`（含 `make_omml_delimiter()` / `make_omml_nary()`）
- 处理分节、页眉页脚和页面设置：`inspect_sections()`、`get_section_page_setup()`、`set_section_page_setup()`、`ensure_*header*()`、`ensure_*footer*()`、`append_section()`、`insert_section()`、`move_section()`、`copy_section_layout()`、`assign_section_*()`、`remove_section_*()`、`move_header_part()`、`move_footer_part()`
- 处理样式、编号和语言元数据：`list_styles()`、`find_style()`、`find_style_usage()`、`list_style_usage()`、`plan_style_refactor()`、`apply_style_refactor()`、`plan_style_refactor_restore()`、`restore_style_refactor()`、`rename_style()`、`merge_style()`、`plan_prune_unused_styles()`、`prune_unused_styles()`、`ensure_*style()`、`ensure_numbering_definition()`、`ensure_style_linked_numbering()`、`set_paragraph_style_numbering()`、`default_run_*()`、`style_run_*()`、`resolve_style_properties()`、`materialize_style_run_properties()`、`rebase_character_style_based_on()`、`set_paragraph_style_based_on()`、`set_paragraph_style_next_style()`、`set_paragraph_style_outline_level()`、`rebase_paragraph_style_based_on()`
- 想做脚本化检查或一次性改写：优先看 CLI 的 `inspect-*`、`inspect-hyperlinks`、`inspect-review`、`inspect-omml`、`validate-template`、`set-content-control-form-state`、`replace-content-control-*`、`append-hyperlink`、`accept-all-revisions`、`append-page-number-field`、`append-table-of-contents-field`、`append-field`、`append-reference-field`、`append-page-reference-field`、`append-style-reference-field`、`append-document-property-field`、`append-date-field`、`append-hyperlink-field`、`append-sequence-field`、`append-caption`、`append-index-entry-field`、`append-index-field`、`append-complex-field`、`replace-field`、`inspect-update-fields-on-open`、`set-update-fields-on-open`、`set-section-page-setup`

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
   `set_cell_text(...)`、`set_row_texts(...)` 这类按真实单元格索引直达的入口，
   或者直接用 `set_rows_texts(...)`、`set_cell_block_texts(...)`
   一次性覆盖多行 / 一个矩形块，不需要再自己手写 `next()` 循环
5. 如果模板行里有横向合并单元格，可以改用
   `find_cell_by_grid_column(...)` / `set_cell_text_by_grid_column(...)`
   按 Word 里看见的视觉列定位；命中的结果仍然是底层真实 `w:tc` 单元格

CLI 示例：

```bash
featherdoc_cli inspect-template-table-rows report.docx --bookmark page3_target_table --json
featherdoc_cli set-template-table-cell-text report.docx --bookmark page3_target_table 1 2 --text "更新后的内容" --output report-updated.docx --json
featherdoc_cli set-template-table-cell-text report.docx --bookmark page3_target_table 1 --grid-column 2 --text "按视觉列更新" --output report-updated.docx --json
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
table->set_cell_text_by_grid_column(1, 2, "按视觉列更新");
table->set_row_texts(2, {"商品A", "3", "99.00"});
table->set_rows_texts(3, {{"商品B", "1", "18.00"}, {"商品C", "5", "7.50"}});

doc.save_as("report-updated.docx");
```

如果目标表在页眉 / 页脚 / 分节页眉页脚里，就把 `body_template()` 换成
`header_template()`、`footer_template()`、`section_header_template()` 或
`section_footer_template()`。

如果你要一次性约束多个 part，可以直接用
`Document::validate_template_schema(...)`，把 body / header / footer /
section-header / section-footer 的要求组合到一个文档级 schema 里。

```cpp
const auto result = doc.validate_template_schema({
    {
        {featherdoc::template_schema_part_kind::section_header, std::nullopt, 0U,
         featherdoc::section_reference_kind::default_reference},
        {"header_title", featherdoc::template_slot_kind::text, true},
    },
    {
        {featherdoc::template_schema_part_kind::section_footer, std::nullopt, 0U,
         featherdoc::section_reference_kind::default_reference},
        {"footer_signature", featherdoc::template_slot_kind::text, true},
    },
});
```

可运行的文档级样例见
`samples/sample_template_schema_validation.cpp`，对应 target 是
`featherdoc_sample_template_schema_validation`。CLI 对应命令是
`featherdoc_cli validate-template-schema ...`，同时也支持
`--schema-file <path>` 读取可复用的 JSON schema 文件。

如果你要把一份真实模板接入项目流程，可以先用库级
`Document::onboard_template(...)` 做首轮诊断。它会复用 schema scan / validate / patch
review，返回扫描到的槽位、baseline 漂移、阻塞 issue 和下一步建议动作。
项目级入口 `scripts/onboard_project_template.ps1` 会额外在
`onboarding_summary.json` 和 `report/manual_review.md` 中写出
`schema_approval_state`、`release_blockers`、`action_items` 与
`manual_review_recommendations`，用于快速判断 schema approval 是否阻断发布、
应先复核 schema update candidate 还是补齐 render-data。
如果需要把多份 onboarding 结果、onboarding plan 和 project-template smoke
审批证据收拢成项目级发布判断，可以继续运行
`scripts/build_project_template_onboarding_governance_report.ps1`。它会输出稳定的
JSON / Markdown 治理报告，并能用 `-FailOnBlocker` 把未审批、待复核或被阻断的
schema approval 变成明确的发布前失败。

```cpp
featherdoc::Document doc("invoice-template.docx");
doc.open();

featherdoc::template_onboarding_options options{};
options.baseline_schema = existing_schema; // 首次接入可省略

const auto onboarding = doc.onboard_template(options);
if (!onboarding.has_value()) {
    throw std::runtime_error(doc.last_error().detail);
}
if (onboarding->has_errors()) {
    throw std::runtime_error(onboarding->issues.front().message);
}

if (onboarding->requires_schema_review()) {
    // 先复核 onboarding->schema_patch / onboarding->patch_review
}
```

`--schema-file` 既支持紧凑字符串 slot，例如 `"header_title:text"`，
也支持结构化 slot 对象：

```json
{
  "targets": [
    {
      "part": "section-header",
      "section": 0,
      "kind": "default",
      "slots": [
        { "bookmark": "header_title", "kind": "text" },
        { "bookmark_name": "header_note", "kind": "block", "required": true },
        { "bookmark": "header_rows", "kind": "table_rows", "count": 1 },
        { "content_control_tag": "order_no", "kind": "text" },
        { "content_control_alias": "Line Items", "kind": "table_rows" }
      ]
    }
  ]
}
```

仓库里现成的示例文件见 `samples/template_schema_validation.schema.json`。
slot 选择器可以继续用 `bookmark` / `bookmark_name` 指向书签，也可以用
`content_control_tag` 按内容控件 tag 指向 slot，或用
`content_control_alias` 按 alias 指向 slot。紧凑的 CLI `--slot` 形式中，
书签仍写成 `name:kind`；内容控件可以写成
`content_control_tag=order_no:text` 或
`content_control_alias=Line Items:table_rows`。

如果你不想手写第一版 schema，可以直接从现有模板导出一份初稿：

```bash
featherdoc_cli export-template-schema input.docx --output template-schema.json --json
featherdoc_cli validate-template-schema input.docx --schema-file template-schema.json --json
```

`export-template-schema` 默认会导出正文以及已加载的 `header[index]` /
`footer[index]` target。如果你希望直接导出成 `section-header` /
`section-footer` 的“直接引用”视角，可以加 `--section-targets`。如果你要
的是每个 section 实际生效的页眉/页脚视角，需要加
`--resolved-section-targets`；这种导出会额外带上
`resolved_from_section` / `linked_to_previous` 之类的元数据，同时仍然可以
直接回喂给 `validate-template-schema --schema-file ...`。这三种模式都会基于
`list_bookmarks()` 当前暴露的轻量书签分类来序列化可表示的书签 slot，
同时也会导出内容控件 slot：优先使用 tag，没有 tag 时回退到 alias。如果你想
把 schema 文件整理成稳定顺序，方便提交或 review，可以再跑一遍
`normalize-template-schema`；如果要比较两个 schema 版本的差异，可以直接用
`diff-template-schema` 查看 added / removed / changed targets。如果你希望在
review 或 CI 里先把 schema 维护质量 gate 住，可以直接用
`lint-template-schema`；它在 schema 干净时返回 `0`，发现重复 target 身份、
重复 slot 名、非规范顺序或残留 `entry_name` 元数据时返回 `1`。
如果你想把这些维护问题按安全、确定性的规则直接收敛掉，可以继续用
`repair-template-schema`；它会去掉 `entry_name`，按“后出现定义覆盖前出现定义”
的规则合并重复 target 身份与重复 slot 名，然后再把 target / slot 顺序规范化。
如果你需要把
一份共享 schema 和文档专用 schema 叠加成一个可提交结果，可以直接用
`merge-template-schema`；后面的文件会按 target 合并，并覆盖同一 bookmark 的
slot 定义，然后再输出规范化后的 schema。如果你需要在已提交的 schema 上做增删
改，可以直接用 `patch-template-schema`，通过 patch 文件里的
`upsert_targets`、`remove_targets`、`remove_slots` 和 `rename_slots`
来维护结果。如果你手里已经有一份 reviewed 的 left / right schema，希望自动整理
出一个可提交的 patch 文件，可以直接用 `build-template-schema-patch`；它仍然以
结果正确为第一优先级，但当某个 target 的完整身份保持不变时，会优先生成
slot 级的 `remove_slots`、`rename_slots` 和局部 `upsert_targets`。只有 target
身份发生变化时，才会退回到整 target 的 `remove_targets + upsert_targets`，
这样把 patch 回放到左侧 schema 上，依然能得到规范化后的右侧 schema。
C++ 侧也已经提供同一套内存级工作流：`template_schema_patch`、
`normalize_template_schema(...)`、`merge_template_schema(...)`、
`apply_template_schema_patch(...)` 和 `build_template_schema_patch(...)`，
下游工具不必为了做 schema 变更先绕一圈 CLI JSON 文件。
如果想把它直接接进 CI，当发现 schema 漂移时返回非零退出码，可以加
`--fail-on-diff`。如果你希望把“导出 + 规范化 + 对比 + gate” 合成一步，可以直接用
`check-template-schema`；它在完全匹配时返回 `0`，发现漂移时返回 `1`，并且
可以通过 `--output` 额外写出规范化后的 generated schema。如果你更希望用仓库
级脚本入口，可以直接用
`scripts/freeze_template_schema_baseline.ps1` 和
`scripts/check_template_schema_baseline.ps1`，它们会负责 CLI 的复用或构建。
其中 baseline wrapper 现在会先跑 `lint-template-schema`，再做文档与 baseline
的比对；如果 committed schema 需要清理，还可以用 `-RepairedSchemaOutput`
额外写出一份候选修复文件。`scripts/check_template_schema_manifest.ps1` 会把同样
的 gate 批量应用到 manifest 里的所有 entry 上，在 `summary.json` 里汇总
`dirty_baseline_count`，并且支持用 `-RepairedSchemaOutputDir` 输出逐条目的
repair 候选文件。

`patch-template-schema` 的 patch 文件本质上是一个 JSON 对象，可以包含下面这些
数组中的任意组合：

- `upsert_targets`：就是普通的 exported schema target，合并规则和
  `merge-template-schema` 一致，会按 target upsert，并覆盖同一 bookmark 的
  slot 定义
- `remove_targets`：按 target 的完整身份字段匹配并删除，匹配时会一起看
  `part`、`index` / `part_index`、`section`、`kind`、
  `resolved_from_section`、`linked_to_previous`
- `remove_slots`：和上面相同的 target 选择字段，再额外带
  `bookmark` / `bookmark_name`、`content_control_tag` 或
  `content_control_alias`；命中的 slot 会被删除，如果某个 target 被删空，
  会自动从结果里 prune 掉
- `rename_slots`：也是同样的 target 选择字段，再带旧 slot 选择器
  `bookmark` / `bookmark_name`、`content_control_tag` 或
  `content_control_alias`，以及同源的新名字 `new_bookmark` /
  `new_bookmark_name`、`new_content_control_tag` 或
  `new_content_control_alias`
- 选择器里的 `entry_name` 会被忽略，所以你可以把导出的 schema JSON 片段裁剪后
  直接拿来写 patch
- `{}` 也是合法的，表示 no-op patch；当两个 schema 已经等价时，
  `build-template-schema-patch` 就会输出它
- `build-template-schema-patch` 只有在 left / right target 身份完全一致，且
  旧 slot 与新 slot 的 shape 可以唯一对应时，才会输出 `rename_slots`；
  只要存在歧义，就会保守地退回删除 / 新增风格的 patch

一个可直接照抄的 patch 示例：

```json
{
  "remove_targets": [
    {
      "part": "section-footer",
      "section": 1,
      "kind": "default"
    }
  ],
  "remove_slots": [
    {
      "part": "body",
      "bookmark": "summary_block"
    },
    {
      "part": "body",
      "content_control_alias": "Legacy Form Row"
    },
    {
      "part": "section-header",
      "section": 1,
      "kind": "default",
      "resolved_from_section": 0,
      "linked_to_previous": true,
      "bookmark_name": "legacy_header_note"
    }
  ],
  "rename_slots": [
    {
      "part": "section-header",
      "section": 1,
      "kind": "default",
      "resolved_from_section": 0,
      "linked_to_previous": true,
      "bookmark": "header_title",
      "new_bookmark": "document_title"
    },
    {
      "part": "body",
      "content_control_tag": "order_no",
      "new_content_control_tag": "order_id"
    }
  ],
  "upsert_targets": [
    {
      "part": "body",
      "slots": [
        {
          "bookmark": "customer",
          "kind": "text",
          "count": 2
        },
        {
          "bookmark": "invoice_no",
          "kind": "text"
        }
      ]
    }
  ]
}
```

## 当前能力范围

当前 FeatherDoc 已覆盖以下高价值能力：

- 读取与保存既有 `.docx`
- 段落与 Run 的增删改
- 表格创建、插行、插列、删行、删列、合并、拆分、列宽、fixed-grid 编辑与第一版浮动定位
- 书签填充、模板表格扩展、条件块显隐
- content control 的枚举、tag / alias 过滤、C++ API / CLI 层的纯文本、段落、表格行、整表和图片替换
- 内联图片与浮动图片，已支持 PNG/JPEG/GIF/BMP/SVG/WebP/TIFF 的插入、尺寸识别和内容类型写入
- 页眉、页脚、分节复制 / 插入 / 移动 / 删除
- 列表、自定义编号定义、编号 catalog 内存级与 CLI JSON 导入 / 导出、check、patch、lint、diff，patch 已覆盖 definition level upsert 与 override upsert/remove，单文件 / manifest 脚本级 baseline gate，`build_document_skeleton_governance_report.ps1` 可从 exemplar 文档导出 numbering catalog、汇总样式 usage 和样式编号审计报告，`build_document_skeleton_governance_rollup_report.ps1` 可把多份骨架治理 summary 汇总成跨模板 exemplar catalog / issue / blocker / action item 视图，`build_table_layout_delivery_rollup_report.ps1` 可把多份版式交付 summary 汇总成跨模板 table style issue / `tblLook` 修复 / floating table preset / blocker / action item 视图，`build_release_blocker_rollup_report.ps1` 可把骨架、模板和版式报告里的 blocker / action item 统一汇总，带 `command_template` 修复建议的样式编号审计 gate（缺失 level 会指向 `upsert_levels` patch 工作流），以及 `repair-style-numbering` plan/apply 安全清理、based-on 对齐、唯一同名 definition relink 与 catalog 导入预修复入口，基础样式目录检查与最小样式定义编辑

## 当前限制

- 不支持加密或受密码保护的 `.docx`
- OMML 公式已有轻量 typed API：`list_omml()`、`append_omml()`、`replace_omml()`、`remove_omml()`，以及 `make_omml_text()`、`make_omml_fraction()`、`make_omml_superscript()`、`make_omml_subscript()`、`make_omml_radical()`、`make_omml_delimiter()`、`make_omml_nary()` 等常见片段构造器；CLI 侧也已有 `inspect-omml` / `append-omml` / `replace-omml` / `remove-omml`；更完整公式布局 builder 仍是后续方向
- 自定义表格样式定义已经有第一版 typed API：`ensure_table_style(...)`
  可写入 whole-table、first-row、第一 / 第二条带等区域的边框、底色、文字颜色、加粗、斜体、字号、字体族、单元格垂直对齐、文本方向、段落对齐、段落间距、行距和 cell margin，也可通过
  `find_table_style_definition(...)` / `featherdoc_cli inspect-table-style --json`
  反查，并可用 `audit-table-style-regions` 审计空的已声明区域，
  用 `audit-table-style-inheritance` 审计缺失、跨类型或循环 `basedOn` 继承链，
  用 `audit-table-style-quality` 聚合区域、继承和 `tblLook` 检查作为 CI gate，
  用 `plan-table-style-quality-fixes` 区分可自动修复的 `tblLook` 项和人工样式定义项，
  用 `apply-table-style-quality-fixes --look-only` 只落盘安全的 `tblLook` 修复，
  并用 `scripts/run_table_style_quality_visual_regression.ps1` 固化 before/after Word 渲染、
  contact sheet 和像素摘要证据包，
  用 `check-table-style-look` 检查表格实例 `tblLook` 与条件区域是否一致，
  或用 `repair-table-style-look` 应用安全的标志修复；详见
  `docs/table_style_definition_zh.rst`；表格实例层面还可用
  `Table::set_position(...)` / `position()` / `clear_position()` 读写第一版
  `w:tblpPr` 浮动定位，覆盖水平 / 垂直参照、twips 偏移、环绕距离、
  重叠策略、常用 preset、多表批量目标、plan/apply 回放和 Word 渲染可视化验证。
  当需要把表格样式、`tblLook` 和浮动定位一次性收口成交付审计材料时，
  可以运行 `scripts/build_table_layout_delivery_report.ps1`，它会复用
  `inspect-tables`、`audit-table-style-quality`、`check-table-style-look` 和
  `plan-table-position-presets` 生成 JSON / Markdown 报告、preset 修复建议、
  可回放 position plan，以及 Word 视觉回归入口。多份单文档报告可以继续交给
  `scripts/build_table_layout_delivery_rollup_report.ps1` 汇总，再进入统一发布阻断视图。
- 模板校验已经覆盖 slot、缺失、重复、意外书签、kind 不匹配、occurrence 约束，
  并且已经有 `validate_template_schema(...)` 和
  `featherdoc_cli validate-template-schema` 这套文档级多 part 校验入口，
  还支持 `--schema-file` 读取可复用 JSON schema，并提供
  `template_schema_patch` / `apply_template_schema_patch(...)` /
  `build_template_schema_patch(...)` 这组内存级 schema 变更 API，
  但还没有独立的交互式 schema 管理工具
- 现在已经可以做 content control 的基础枚举与 `--tag` / `--alias` 过滤（`list_content_controls` / `inspect-content-controls`），也可以通过 `replace_content_control_text_by_tag(...)` / `replace_content_control_text_by_alias(...)` 在 `Document` 或 `TemplatePart` 层做纯文本替换，并可用 `replace-content-control-text`、`replace-content-control-paragraphs`、`replace-content-control-table`、`replace-content-control-table-rows` 和 `replace-content-control-image` 做 CLI 一次性替换；C++ 层还支持 `replace_content_control_with_paragraphs_by_*()`、`replace_content_control_with_table_rows_by_*()`、`replace_content_control_with_table_by_*()` 和 `replace_content_control_with_image_by_*()` 这组结构化内容替换入口，并已通过 `scripts/run_content_control_rich_replacement_visual_regression.ps1` 与 `scripts/run_content_control_image_replacement_visual_regression.ps1` 固化 Word 渲染证据；内容控件也已经纳入 template schema 的导出 / 校验；表单状态 inspection 已覆盖 form kind、lock、dataBinding store item / XPath / prefix mappings、checkbox checked、date format / locale 与下拉列表项，mutation 已支持 checkbox checked、下拉 / 组合框选中项、日期文本 / 格式 / locale、lock 设置 / 清除以及 dataBinding 设置 / 清除，CLI 对应 `set-content-control-form-state`；`sync_content_controls_from_custom_xml()` 与 `sync-content-controls-from-custom-xml` 现在还能读取匹配的 `customXml/item*.xml`，按 `w:dataBinding` XPath 单向刷新内容控件显示文本；文档级语义 diff 也已通过 `compare_semantic(...)` / `document_semantic_diff_result` 和 `semantic-diff` CLI 落地，覆盖段落、表格摘要、图片、内容控件、通用字段对象（包括 TOC / REF / SEQ 等字段的 kind、instruction、result_text、dirty / locked、complex 与 depth 元数据）、样式摘要、编号定义、脚注、尾注、批注、修订、section/page setup 摘要以及 typed 的 `body` / `header` / `footer` 模板 part 结果；JSON 现在包含 `fields`、`field_changes`、`styles`、`style_changes`、`numbering`、`numbering_changes`、`footnotes`、`footnote_changes`、`endnotes`、`endnote_changes`、`comments`、`comment_changes`、`revisions`、`revision_changes`、`template_parts` 与 `template_part_results`，changed 项继续提供字段级 `field_changes`，并支持 `--fail-on-diff`；part 明细还包含 resolved `section-header` / `section-footer` 视图，带 `section_index`、`reference_kind` 和左右 `*_resolved_from_section_index`，可以审计继承自上一节的页眉页脚，同时不重复计入物理 header/footer 总变更数；默认启用内容感知序列对齐以避免插入项造成级联误报，也可用 `--index-alignment` 回退到位置对齐、用 `--alignment-cell-limit <count>` 限制对齐成本，用 `--no-fields` 跳过字段对象比较，用 `--no-styles` / `--no-numbering` 跳过样式或编号 catalog 比较，用 `--no-footnotes` / `--no-endnotes` / `--no-comments` / `--no-revisions` 跳过审阅对象比较，用 `--no-template-parts` 只保留正文级 bucket，或用 `--no-resolved-section-template-parts` 只保留物理 part 明细，并用 `scripts/run_semantic_diff_visual_regression.ps1` 与字段专项的 `scripts/run_generic_fields_visual_regression.ps1` 固化 before/after Word 渲染证据；后续只剩更复杂双向同步、重复节保护和表单保护策略
- 现在也已经可以做继承感知的样式属性检查、单样式 / 全量 style usage report、基础 style id 重命名、同类型 style merge、批量 rename / merge 非破坏性计划、带 reason / confidence / evidence / differences 的重复样式保守 merge 建议、顶层 `suggestion_confidence_summary`（含 min/max confidence、exact XML 数量、XML difference 数量与 `recommended_min_confidence`）、可用 `--source-style` / `--target-style` 将建议收敛到具体样式对，再用 `--confidence-profile recommended|strict|review|exploratory` 或 `--min-confidence <0-100>` 过滤、并可用 `--fail-on-suggestion` 作为 CI gate、JSON 输出带 `fail_on_suggestion` / `suggestion_gate_failed` 诊断字段的 CLI 输出 / 持久化 plan 文件、受控 apply 与 rollback 记录、基于 rollback JSON 的 merge restore dry-run（也可用 `--plan-only`）审计 / 正式恢复，并支持重复 `--entry` 或 `--source-style` / `--target-style` 选择 rollback 项，restore issue 也会输出可操作 `suggestion` 以及顶层 `issue_count` / `issue_summary`，以及保守的未使用 custom style 清理计划 / 应用；merge restore 会插回捕获的 source style XML，并只按 `node_ordinal` 恢复原 source usage hits，避免误改既有 target 引用；重复建议的 XML 比较会忽略 `styleId` 和显示名；schema patch 侧已提供 `write_schema_patch_confidence_calibration_report.ps1` 只读校准入口，后续仍需要更多真实语料样本、更完整的样式建议置信度校准和批量恢复选择策略

更细的限制列表请看 [README.md](README.md) 里的 `Current Limitations`。

## 文档入口

仓库里当前的主要文档入口有：

- 英文主 README：[README.md](README.md)
- 中文 README：[README.zh-CN.md](README.zh-CN.md)
- Sphinx 文档首页：`docs/index.rst`
- 项目定位：`docs/project_identity_zh.rst`
- 当前推进方向：`docs/current_direction_zh.rst`
- 版本与发布策略：`docs/release_policy_zh.rst`
- Word 可视化工作流：`docs/automation/word_visual_workflow_zh.rst`
- 许可说明：`docs/licensing_zh.rst`
- 仓库法律说明：`LEGAL.md`

## 项目方向

FeatherDoc 现在应被视为一个持续演进的独立分支，而不是对历史上游项目的被动镜像。

- 产品目标是：围绕正式文档场景，提供 `.docx` 的处理、编辑、修改、生成与验证闭环
- 现代 C++ 与更清晰的 API 语义优先
- MSVC 可构建性是正式支持目标
- 错误诊断、`open()` / `save()` 行为和核心路径性能是一等公民
- 文档、仓库元数据、许可与验证流程都按当前项目方向维护

如果你要判断“这个库下一步该继续推进什么能力”，请先看
`docs/current_direction_zh.rst`。

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
