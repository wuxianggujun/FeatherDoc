# FeatherDoc PDF 构建手册

> PDF 能力仍是实验性方向。默认构建不会启用 PDFio / PDFium，也不会拉取、
> 编译或安装实验性 PDF 头文件。
> 默认核心入口 `include/featherdoc/core.hpp` 与 `include/featherdoc/fwd.hpp`
> 不启用 PDFio / PDFium；实验性 PDF 头仍只在对应构建选项和安装条件下暴露。

## 什么时候需要看这份文档

只有推进 Word ⇄ PDF 转换器时才需要看这里：

- `FEATHERDOC_BUILD_PDF=ON`：启用 PDFio 写出方向
- `FEATHERDOC_BUILD_PDF_IMPORT=ON`：启用 PDFium 读入方向

普通库构建不需要这些步骤。

> 注意：Word COM 只用于 `scripts/run_word_visual_smoke.ps1` 及其上层本地验证脚本，
> 不属于 FeatherDoc 库 API，也不会进入默认关闭的核心构建目标。
> 这些脚本会先做本地预检：Python、Word COM，以及在需要本地生成样例时的
> MSVC 开发环境；如果你传入现成 DOCX，它只检查那个输入文件是否存在。

## 不要提交的生成物

以下都是本地生成或第三方 checkout，不应提交：

- `tmp/depot_tools/`
- `tmp/pdfium-workspace/`
- `tmp/pdfio-src/`
- `.bpdf-*/`
- `.codex-build-*/`
- `.tmp-pdfium-*/`
- `build-pdf*/`
- `out/`
- probe 生成的 `*.pdf`
- 编译产物：`*.exe`、`*.lib`、`*.obj`、`*.pdb`

需要提交的是 FeatherDoc 自己的源码、CMake helper、样例、测试和文档。

## 默认构建确认

默认配置必须保持 PDF 关闭：

```powershell
cmake -S . -B .codex-build-default `
  -G "Unix Makefiles" `
  -DCMAKE_C_COMPILER=clang `
  -DCMAKE_CXX_COMPILER=clang++ `
  -DBUILD_TESTING=OFF `
  -DBUILD_CLI=OFF
```

确认：

```powershell
Select-String -Path .codex-build-default/CMakeCache.txt -Pattern "FEATHERDOC_BUILD_PDF"
cmake --build .codex-build-default --target help | Select-String -Pattern "pdf|Pdf"
```

预期：

```text
FEATHERDOC_BUILD_PDF:BOOL=OFF
FEATHERDOC_BUILD_PDF_IMPORT:BOOL=OFF
```

并且 target help 中没有 PDF 目标。

## 准备 PDFio

PDFio 用于 Word → PDF 写出方向。

如果使用本地源码 checkout：

```powershell
git clone https://github.com/michaelrsweet/pdfio.git tmp/pdfio-src
```

构建 PDFio probe：

```powershell
cmake -S . -B .bpdf-interfaces `
  -G "Unix Makefiles" `
  -DCMAKE_C_COMPILER=clang `
  -DCMAKE_CXX_COMPILER=clang++ `
  -DBUILD_TESTING=ON `
  -DBUILD_CLI=OFF `
  -DBUILD_SAMPLES=ON `
  -DFEATHERDOC_BUILD_PDF=ON `
  -DFEATHERDOC_FETCH_PDFIO=OFF `
  -DFEATHERDOC_PDFIO_SOURCE_DIR="$PWD/tmp/pdfio-src"

cmake --build .bpdf-interfaces --target featherdoc_pdfio_probe
ctest --test-dir .bpdf-interfaces -R pdfio_generator_probe --output-on-failure --timeout 60
```

输出样例：

```text
.bpdf-interfaces/featherdoc-pdfio-probe.pdf
```

构建 docs → PDF 最小适配层 probe：

```powershell
cmake --build .bpdf-interfaces --target featherdoc_pdf_document_probe
ctest --test-dir .bpdf-interfaces -R pdf_document_generator_probe --output-on-failure --timeout 60
```

输出样例：

```text
.bpdf-interfaces/featherdoc-document-probe.pdf
```

## 准备 PDFium

PDFium 用于 PDF → Word 读入方向。

当前支持 4 种 provider：

- `auto`：默认值。优先尝试 `find_package(PDFium)`，其次尝试 prebuilt 输入；
  只有提供 `FEATHERDOC_PDFIUM_SOURCE_DIR` 时才退回 source。
- `source`：从 PDFium 源码和 GN/Ninja 构建。需要 `depot_tools` 和 PDFium checkout。
- `package`：走 `find_package(PDFium)`，要求外部包已经导出 CMake target。
- `prebuilt`：直接导入现成的 PDFium 二进制。至少需要
  `FEATHERDOC_PDFIUM_LIBRARY` 和 `FEATHERDOC_PDFIUM_INCLUDE_DIR`；Windows 下如果
  `pdfium.dll` 不和 `pdfium.lib` 放在同一目录，额外传
  `FEATHERDOC_PDFIUM_RUNTIME_DLL` 或 `FEATHERDOC_PDFIUM_RUNTIME_DIR`。

如果当前环境无法访问 Chromium 的 CIPD 后端，优先保留默认 `auto` 并提供 prebuilt
输入，或者显式切到 `prebuilt`，都可以直接绕开 `depot_tools` bootstrap。

先准备 Chromium `depot_tools`。推荐放在 `tmp/`，不要提交：

```powershell
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git tmp/depot_tools
$env:PATH = "$PWD/tmp/depot_tools;$env:PATH"
```

同步 PDFium 源码：

```powershell
New-Item -ItemType Directory -Force tmp/pdfium-workspace | Out-Null
Push-Location tmp/pdfium-workspace
gclient config --unmanaged https://pdfium.googlesource.com/pdfium.git
gclient sync --no-history --jobs 8
Pop-Location
```

Windows 上如果 hook 尝试下载 Google 内部工具链并失败，改用本机 VS：

```powershell
$env:DEPOT_TOOLS_WIN_TOOLCHAIN = "0"
Push-Location tmp/pdfium-workspace
gclient sync --no-history --jobs 8
Pop-Location
```

如果第三方依赖因为长路径被误判为 dirty，只在这个临时 workspace 内重跑：

```powershell
$env:GIT_CONFIG_COUNT = "2"
$env:GIT_CONFIG_KEY_0 = "core.longpaths"
$env:GIT_CONFIG_VALUE_0 = "true"
$env:GIT_CONFIG_KEY_1 = "core.autocrlf"
$env:GIT_CONFIG_VALUE_1 = "false"
Push-Location tmp/pdfium-workspace
gclient sync --no-history --jobs 8 --force --reset
Pop-Location
```

## Windows 构建 PDFium import（source provider）

进入 VS x64 开发环境：

```bat
VsDevCmd.bat -arch=x64 -host_arch=x64
```

如果 VS 安装在非默认路径，配置时传 `FEATHERDOC_PDFIUM_VS_YEAR` 和
`FEATHERDOC_PDFIUM_VS_INSTALL`。

示例：

```powershell
cmake -S . -B .bpdf-pdfium-source-msvc `
  -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
  -DBUILD_TESTING=OFF `
  -DBUILD_CLI=OFF `
  -DBUILD_SAMPLES=ON `
  -DFEATHERDOC_BUILD_PDF_IMPORT=ON `
  -DFEATHERDOC_PDFIUM_PROVIDER=source `
  -DFEATHERDOC_PDFIUM_SOURCE_DIR="$PWD/tmp/pdfium-workspace/pdfium" `
  -DFEATHERDOC_DEPOT_TOOLS_DIR="$PWD/tmp/depot_tools" `
  -DFEATHERDOC_PDFIUM_VS_YEAR=2026 `
  -DFEATHERDOC_PDFIUM_VS_INSTALL="D:/Program Files/Microsoft Visual Studio/18/Professional"

cmake --build .bpdf-pdfium-source-msvc --target featherdoc_pdfium_probe
```

关键点：

- source-built `pdfium.lib` 当前使用 `/MT`
- FeatherDoc 同一构建也要传 `-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`
- Windows 下会自动补 `winmm.lib`
- CMake 会优先使用 PDFium checkout 内的 `buildtools/win/gn.exe`
- CMake 会优先使用 PDFium checkout 内的 `third_party/ninja/ninja.exe`
- 如果 `depot_tools` 目录存在但还没有初始化，helper 会先检查
  `chrome-infra-packages.appspot.com:443` 连通性；不可达时会快速失败并提示先初始化
  `depot_tools` 或恢复网络，避免卡在 CIPD bootstrap 上。

## Windows 构建 PDFium import（prebuilt provider）

如果手里已经有 `pdfium.lib` / `pdfium.dll` 和 `public/fpdfview.h`，可以直接走
`prebuilt`，不需要 `depot_tools`、GN 或 Ninja。

```powershell
cmake -S . -B .bpdf-pdfium-prebuilt-msvc `
  -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
  -DBUILD_TESTING=OFF `
  -DBUILD_CLI=OFF `
  -DBUILD_SAMPLES=ON `
  -DFEATHERDOC_BUILD_PDF_IMPORT=ON `
  -DFEATHERDOC_PDFIUM_PROVIDER=prebuilt `
  -DFEATHERDOC_PDFIUM_LIBRARY="D:/deps/pdfium/lib/pdfium.lib" `
  -DFEATHERDOC_PDFIUM_INCLUDE_DIR="D:/deps/pdfium/public" `
  -DFEATHERDOC_PDFIUM_RUNTIME_DLL="D:/deps/pdfium/bin/pdfium.dll"

cmake --build .bpdf-pdfium-prebuilt-msvc --target featherdoc_pdfium_probe
```

如果 `pdfium.dll` 和 `pdfium.lib` 在同一个目录，可以省略
`FEATHERDOC_PDFIUM_RUNTIME_DLL`。如果运行时目录里有额外依赖，但 DLL 文件名不固定，
改传 `FEATHERDOC_PDFIUM_RUNTIME_DIR`。

预检阶段可以直接传预构建根目录，让脚本自动推导 `include/fpdfview.h`、
`lib/pdfium.dll.lib`（或 `lib/pdfium.lib`）和 `bin/pdfium.dll`：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\check_pdf_dependency_inputs.ps1 `
  -PdfioSourceDir .\tmp\pdfio-src `
  -PdfiumProvider prebuilt `
  -PdfiumPrebuiltRoot "D:/deps/pdfium" `
  -OutputJson .\output\pdf-dependency-inputs\summary.json
```

summary 会写出 `pdfium_prebuilt_root` 和 `pdfium_prebuilt_root_exists`，方便在
preflight 报告里追踪本机使用的是哪一套 PDFium 输入。

## 端到端 smoke

同时开启 PDFio 写出和 PDFium 读入：

这里的 Word COM 仅用于本地验证链路，把生成的 DOCX 导出成 PDF 作为证据，
不是库内部的渲染实现。

```powershell
cmake -S . -B .bpdf-roundtrip-msvc `
  -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
  -DBUILD_TESTING=ON `
  -DBUILD_CLI=OFF `
  -DBUILD_SAMPLES=ON `
  -DFEATHERDOC_BUILD_PDF=ON `
  -DFEATHERDOC_FETCH_PDFIO=OFF `
  -DFEATHERDOC_PDFIO_SOURCE_DIR="$PWD/tmp/pdfio-src" `
  -DFEATHERDOC_BUILD_PDF_IMPORT=ON `
  -DFEATHERDOC_PDFIUM_SOURCE_DIR="$PWD/tmp/pdfium-workspace/pdfium" `
  -DFEATHERDOC_DEPOT_TOOLS_DIR="$PWD/tmp/depot_tools" `
  -DFEATHERDOC_PDFIUM_VS_YEAR=2026 `
  -DFEATHERDOC_PDFIUM_VS_INSTALL="D:/Program Files/Microsoft Visual Studio/18/Professional"

cmake --build .bpdf-roundtrip-msvc --target featherdoc_pdfio_probe featherdoc_pdf_document_probe featherdoc_pdfium_probe
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf(io_generator|_document_generator|ium_parser|ium_document_parser)_probe" --output-on-failure --timeout 60
```

当前已验证结果：

```text
pdfio_generator_probe ............ Passed
pdf_document_generator_probe .... Passed
pdfium_parser_probe .............. Passed
pdfium_document_parser_probe ..... Passed
```

## 已有 PDF probe / 测试盘点

- `pdfio_generator_probe`：PDFio 写出最小 smoke
- `pdf_document_generator_probe`：`Document` → `PdfDocumentLayout` → PDF smoke
- `pdfium_parser_probe`：PDFium 读入 PDFio 产物 smoke
- `pdfium_document_parser_probe`：PDFium 读入 `Document` 产物 smoke
- `pdf_import_structure`：PDFium 字符 span 聚合为行 / 段落，并导入纯文本
  `Document` 的读入结构测试
- `pdf_import_failure`：PDF 读入失败样本的分类测试，不混入导出回归；
  当前覆盖禁用 text/geometry、空白 PDF、parse 失败、表格候选已检测
- `pdf_import_table_heuristic`：PDF 简单表格候选识别第一版，覆盖网格正例、
  保守 2 行表头/数据表正例、保守 2 列 key-value 表正例、
  无装饰线条 key-value 表正例、保守不规则列宽表格正例、
  无装饰线条多列表格正例、
  双栏/编号列表/短标签并列文本/票据式三列布局反例，以及显式 opt-in 后把
  简单表格候选导入为 `Document` 表格
- `pdf_font_resolver`：字体解析和回退规则单测
- `pdf_text_metrics`：文本宽度 / 行高估算单测
- `pdf_text_shaper`：HarfBuzz glyph run 桥接层单测
- `pdf_document_adapter_font`：PDF adapter 的字体映射、样式和列表前缀回归
- `pdf_unicode_font_roundtrip`：Unicode / CJK 字体嵌入和 PDFium 回读回环
- `pdf_unicode_font_roundtrip_visual`：Unicode / CJK 字体 PDF 渲染为 PNG 后的视觉 smoke

读入方向可以单独跑，不必混进导出主线：

```powershell
cmake --build .bpdf-roundtrip-msvc --target pdf_import_structure_tests pdf_import_failure_tests pdf_import_table_heuristic_tests
ctest --test-dir .bpdf-roundtrip-msvc -R "pdfium_.*probe|pdf_import_(structure|failure|table_heuristic)" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60
```

`PdfDocumentImporter` 当前要求 `PdfParseOptions::extract_text=true` 且
`PdfParseOptions::extract_geometry=true`。`extract_geometry=false` 会直接返回
`extract_geometry_disabled`，不会降级为纯文本直通导入；因为段落聚合、表格候选识别和
导入顺序都依赖字符 bounds。对应失败路径由 `pdf_import_failure` 覆盖，并确认失败时
不会污染目标 `Document`。

`featherdoc_pdf_regression_sample` 现在还会对每个回归 PDF 做页数级别的输出大小上限
检查；含图样本则从 manifest 读取 `expected_image_count`，再用 PDFium 对页面 image
object 数量做回读断言。这样文件体量和图片出现次数都能留在同一条 CTest 链路里。

资源受限时可复用 bounded CTest summary 作为 release governance 的辅助证据，但它
不替代完整 `pdf_` CTest 套件或 full visual gate。`run_release_candidate_checks.ps1`
会自动发现当前 7 个标准 summary，并把它们聚合为 `pdf_bounded_ctest`：

```text
build\pdf-ctest-bounded-subset-current\summary.json
build\pdf-ctest-bounded-contract-static-current\summary.json
build\pdf-ctest-bounded-cjk-flow-static-current\summary.json
build\pdf-ctest-bounded-regression-basic-text-current\summary.json
build\pdf-ctest-bounded-regression-styled-document-current\summary.json
build\pdf-ctest-bounded-regression-business-samples-current\summary.json
build\pdf-ctest-bounded-regression-table-layout-current\summary.json
```

release governance 会在同一个 `source_report:` block 中保留
`pdf_bounded_ctest_summary_count`、`pdf_bounded_ctest_pass_count`、
`pdf_bounded_ctest_skipped_test_count`、`pdf_bounded_ctest_selected_test_count`、
`pdf_bounded_ctest_subsets`、`pdf_bounded_ctest_summary_json_display`、
`pdf_bounded_ctest_import_diagnostics_contract_tests`、
`pdf_bounded_ctest_import_diagnostics_contract_fields` 和
`pdf_bounded_ctest_import_negative_boundary_contract_cases`。
固定标记：`pdf_bounded_ctest_governance_trace`、
`pdf_bounded_ctest_source_report_block_trace`。
`release_blocker_rollup.md` 的 `Source Report Contracts` 也必须在同一个
`featherdoc.release_candidate_summary` list block 中保留 PDF visual gate status、
`full_visual_gate_status`、verdict、finalizable、summary/contact sheet、CJK/baseline
计数和 bounded CTest 字段，不能由 detached notes 或非 release-candidate schema
补齐。固定标记：`pdf_visual_gate_rollup_material_safety_trace`。

资源受限导致非 `FinalizeOnly` 的完整 visual gate 被外层 60 秒保护截断时，先运行
`scripts/write_pdf_visual_gate_attempt_summary.ps1` 汇总当前 `report` 目录中的
CTest 日志、CJK copy/search 结果、fresh baseline render 计数和 contact-sheet 状态。
该脚本输出 `output/pdf-visual-release-gate-current/report/attempt-summary.json`，
schema 为 `featherdoc.pdf_visual_gate_attempt_summary.v1`，并显式写出
`pdf_visual_gate_attempt_status`、`pdf_visual_gate_attempt_verdict`、
`pdf_visual_gate_attempt_full_visual_gate_status`、`pdf_visual_gate_attempt_evidence_scope`、
`pdf_visual_gate_attempt_pdf_regression_selected_test_count`、
`pdf_visual_gate_attempt_pdf_regression_failed_test_count`、
`pdf_visual_gate_attempt_pdf_regression_skipped_test_count`、
`pdf_visual_gate_attempt_outer_guard_status`、
`pdf_visual_gate_attempt_outer_guard_timed_out`、
`pdf_visual_gate_attempt_outer_guard_timeout_seconds`、
`pdf_visual_gate_attempt_visual_baseline_render_status` 和
`pdf_visual_gate_attempt_aggregate_contact_sheet_status`。固定标记：
`pdf_visual_gate_attempt_summary_trace`、`pdf_visual_gate_attempt_governance_trace`。
如果外层 60 秒保护截断了本次尝试，必须写出
`pdf_visual_gate_attempt_outer_guard_status = timed_out`、
`pdf_visual_gate_attempt_outer_guard_timed_out = true` 和
`pdf_visual_gate_attempt_outer_guard_timeout_seconds = 60`。固定标记：
`pdf_visual_gate_attempt_outer_guard_trace`。
`bounded_attempt_auxiliary_only` 只解释被截断尝试已经完成的子阶段，不能替代
`full_visual_gate_status = pass` 或 full gate summary verdict。`assert_release_material_safety.ps1`
会要求 `pdf_visual_gate_attempt_*` 字段与 `source_report:` 同块，避免 detached notes
单独补齐被截断尝试的子阶段证据。固定标记：
`pdf_visual_gate_attempt_material_safety_trace`。
`release_blocker_rollup.md` 的 `Source Report Contracts` 也必须把 attempt status、
verdict、full status、`bounded_attempt_auxiliary_only` scope、`attempt-summary.json`、
outer guard `timed_out` / `true` / `60`、regression/CJK/render 计数和 contact sheet
保留在同一个 release-candidate list block。固定标记：
`pdf_visual_gate_attempt_rollup_material_safety_trace`。
`final_review.md` 展示 attempt 辅助证据时，attempt status/verdict/full status、
stages、pdf_regression 和 render 行必须保留在 `## Step status` 的同一连续 list run；
attempt summary/contact sheet 必须保留在 `## Key outputs`，且路径必须直接携带
`attempt-summary.json` 和 `aggregate-contact-sheet.png`，不能由 detached notes 补齐。
固定标记：`pdf_visual_gate_attempt_final_review_material_safety_trace`。

如果 44 个 visual baseline 无法在单个 60 秒外层保护内一次性重渲染，可以先用
`scripts/run_pdf_visual_release_gate.ps1 -VisualBaselineSliceOnly -VisualBaselineOffset <n> -VisualBaselineLimit <m>`
补跑受控切片。该模式会跳过 CTest、Unicode font 和 CJK copy/search，只重渲染选中的
baseline，生成 `visual-baseline-slice-offset-<n>-limit-<m>-summary.json` 和对应
contact sheet。slice summary 使用 `featherdoc.pdf_visual_baseline_slice.v1`，
`evidence_scope = visual_baseline_slice_only`，并固定写出
`full_visual_gate_status = not_complete` 与
`slice_summary_does_not_replace_full_visual_gate_verdict`。固定标记：
`pdf_visual_baseline_slice_summary_trace`。这些切片只能作为推进 fresh render 计数的辅助证据，
不能替代 full gate summary verdict。

当所有 baseline 已经由切片或完整运行生成，但 aggregate contact sheet 需要在 60 秒保护内单独刷新时，
运行 `scripts/run_pdf_visual_release_gate.ps1 -RebuildAggregateContactSheetOnly`。该模式只读取现有
baseline summary / page-01 PNG，重建 `report/aggregate-contact-sheet.png`，并写出
`aggregate-contact-sheet-rebuild-summary.json`。该 summary 使用
`featherdoc.pdf_visual_aggregate_contact_sheet_rebuild.v1`，
`evidence_scope = aggregate_contact_sheet_rebuild_only`，并固定写出
`full_visual_gate_status = not_complete` 与
`aggregate_rebuild_summary_does_not_replace_full_visual_gate_verdict`。固定标记：
`pdf_visual_aggregate_contact_sheet_rebuild_trace`。它只能证明 contact-sheet 重建阶段完成，
不能替代 full gate summary verdict。

当一次 full visual gate 被拆成 `attempt-summary.json`、多个
`visual-baseline-slice-*-summary.json` 和
`aggregate-contact-sheet-rebuild-summary.json` 后，运行
`scripts/write_pdf_visual_segmented_gate_summary.ps1 -ReportDir .\output\pdf-visual-release-gate-current\report`
生成 `segmented-summary.json`，把分段证据聚合成单一 reviewer 入口。该 summary 使用
`featherdoc.pdf_visual_segmented_gate_summary.v1`，并显式写出 `status` /
`verdict`、`full_visual_gate_status = not_complete`、
`evidence_scope = segmented_visual_gate_auxiliary_only` 和
`segmented_summary_does_not_replace_full_visual_gate_verdict`。固定标记：
`pdf_visual_segmented_gate_summary_trace`、`pdf_visual_segmented_gate_governance_trace`。
即使 `status = pass` 且 `verdict = pass`，也只表示分段辅助证据完整，不能替代
非 `FinalizeOnly` 的 full visual gate verdict。

release governance 会在同一个 `source_report:` block 中保留
`pdf_visual_segmented_gate_status`、`pdf_visual_segmented_gate_verdict`、
`pdf_visual_segmented_gate_full_visual_gate_status`、
`pdf_visual_segmented_gate_evidence_scope`、`pdf_visual_segmented_gate_boundary`、
`pdf_visual_segmented_gate_summary_json_display`、
`pdf_visual_segmented_gate_slice_summary_count`、
`pdf_visual_segmented_gate_slice_pass_count`、
`pdf_visual_segmented_gate_slice_failed_count`、
`pdf_visual_segmented_gate_covered_baseline_count`、
`pdf_visual_segmented_gate_expected_visual_render_count`、
`pdf_visual_segmented_gate_attempt_stage_count`、
`pdf_visual_segmented_gate_attempt_passed_stage_count`、
`pdf_visual_segmented_gate_visual_baseline_render_status`、
`pdf_visual_segmented_gate_aggregate_contact_sheet_status`、
`pdf_visual_segmented_gate_aggregate_contact_sheet_display`、
`pdf_visual_segmented_gate_aggregate_contact_sheet_bytes`、
`pdf_visual_segmented_gate_aggregate_rebuild_status` 和
`pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count`。
`release_governance_handoff.md` 中这些 segmented gate 字段必须继续与
`schema=featherdoc.release_candidate_summary` 的 `source_report:` 保持同块，不能由
detached notes 补齐。固定标记：
`pdf_visual_segmented_gate_material_safety_trace`。
`release_blocker_rollup.md` 的 `Source Report Contracts` 也必须把 segmented status、
verdict、full status、`segmented_visual_gate_auxiliary_only` scope/boundary、
`segmented-summary.json`、slice coverage、contact sheet bytes/status 和 aggregate
rebuild 计数保留在同一个 release-candidate list block。固定标记：
`pdf_visual_segmented_gate_rollup_material_safety_trace`。
`final_review.md` 展示 segmented gate 辅助证据时，segmented status/verdict/full status、
scope、slices 和 coverage 行必须保留在 `## Step status` 的同一连续 list run，
scope 行必须直接携带 `segmented_visual_gate_auxiliary_only`；segmented summary/contact sheet
必须保留在 `## Key outputs`，且路径必须直接携带 `segmented-summary.json` 和
`aggregate-contact-sheet.png`，不能由 detached notes 补齐。固定标记：
`pdf_visual_segmented_gate_final_review_material_safety_trace`。

默认 `PdfDocumentImporter` 遇到 `PdfParsedTableCandidate` 仍返回
`table_candidates_detected`，避免把表格误扁平化成正文。只有显式设置
`PdfDocumentImportOptions::import_table_candidates_as_tables=true` 时，才会尝试把简单
网格候选写入 `Document` 表格。当前实现还会在首个导入块是表格时，移除
`create_empty()` 留下的 body 占位段落，并通过 `inspect_body_blocks()` 做顺序验收。
面向用户的支持范围、保守边界和 `import-pdf --json` 输出契约已经集中到
`docs/zh-CN/api/pdf_workflow.rst`；英文镜像见
`docs/en/api/pdf_workflow.rst`。本文档只保留构建、开发者测试和调试入口，
避免用户契约在多处漂移。

导入侧的发布边界必须保持清晰：当前是 text-first importer，只承诺可抽取文本、
受控段落聚合和显式 opt-in 的简单表格候选。它不是通用 PDF 转 Word 引擎，
不承诺 OCR / 扫描件支持，也不承诺任意 PDF 的视觉级精确还原。

开发者调试跨页表格续接时，重点查看
`PdfDocumentImportResult::table_continuation_diagnostics`：

- `disposition` 区分 `created_new_table` 和 `merged_with_previous_table`。
- `blocker` 记录保守拆表原因，例如 `column_count_mismatch`、
  `column_anchors_mismatch`、`repeated_header_mismatch` 或
  `continuation_confidence_below_threshold`。
- `header_match_kind` 记录 repeated-header 命中路径，例如 `exact`、
  `normalized_text`、`plural_variant`、`canonical_text` 或 `token_set`。
- `source_row_offset` 记录续接时跳过的源行数，常见值 `1` 表示重复表头被跳过。

需要保留测试 DOCX 并做 Word -> PDF 视觉 smoke 时，可运行：

```powershell
$env:FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS='1'; ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60; Remove-Item Env:FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 -InputDocx .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-table.docx -OutputDir .\output\pdf-e7-table-import-docx-visual -ReviewNote "PDF import table candidate opt-in visual verification"
```

## CLI 导出入口

`featherdoc_cli export-pdf` 已经可以作为用户入口使用。当前常用参数是：
完整选项、JSON summary 结构和导出边界见 `docs/zh-CN/api/pdf_workflow.rst`；
英文镜像见 `docs/en/api/pdf_workflow.rst`。

- `--output <pdf>`
- `--font-file <path>`
- `--cjk-font-file <path>`
- `--font-map <family>=<path>`
- `--render-headers-and-footers`
- `--expand-header-footer-page-placeholders`
- `--render-inline-images`
- `--no-font-subset`
- `--no-system-font-fallbacks`
- `--summary-json <path>`
- `--json`

建议至少在 roundtrip 构建里验证一次：

```powershell
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_cli_export$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^cli_usage$" --output-on-failure --timeout 60
```

## PDF regression samples

同时开启 PDFio 写出和 PDFium 读入后，可以直接跑当前 90 个 regression manifest 样本。
`pdf_regression_*` 当前覆盖 91 个 CTest，其中包含 manifest 校验测试。
其中 42 个样本标记 `expect_visual_baseline=true`，作为 PNG visual baseline manifest
输入；43 个样本标记 `expect_cjk=true`，作为 CJK text-layer copy/search 输入。

```powershell
cmake --build .bpdf-roundtrip-msvc --target featherdoc_pdf_regression_sample pdf_regression_manifest_tests
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60
```

当前首批样本包括：

- `single-text`
- `multi-page-text`
- `cjk-text`
- `mixed-cjk-punctuation-text`
- `styled-text`
- `font-size-text`
- `color-text`
- `mixed-style-text`
- `contract-cjk-style`
- `document-contract-cjk-style`
- `three-page-text`
- `landscape-text`
- `title-body-text`
- `dense-text`
- `four-page-text`
- `underline-text`
- `punctuation-text`
- `latin-ligature-text`
- `two-page-text`
- `repeat-phrase-text`
- `bordered-box-text`
- `line-primitive-text`
- `table-like-grid-text`
- `metadata-long-title-text`
- `header-footer-text`
- `two-column-text`
- `invoice-grid-text`
- `image-caption-text`
- `sectioned-report-text`
- `list-report-text`
- `long-report-text`
- `image-report-text`
- `cjk-report-text`
- `cjk-image-report-text`
- `document-eastasia-style-probe`
- `document-image-semantics-text`
- `document-table-semantics-text`
- `document-long-flow-text`
- `document-invoice-table-text`
- `document-table-header-footer-variants-text`
- `document-table-wrap-flow-text`
- `document-table-cant-split-text`
- `document-table-merged-cells-text`
- `document-table-merged-header-repeat-text`
- `document-table-merged-header-footer-variants-text`
- `document-table-merged-cant-split-text`

其中 `cjk-text` 在找不到可用 CJK 字体时会跳过，不会把整个套件判失败。

PDFium 本地探测或依赖排查阶段可能会留下 `.tmp-pdfium-*` 目录。这些目录只用于
本机临时检查，不应提交；需要清理时直接手工删除即可。

## 视觉发布门禁

完整门禁会复用一个已经构建好的 PDF 测试目录。资源紧张或不确定 build
目录是否可用时，先运行只读预检：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\check_pdf_dependency_inputs.ps1 `
  -PdfioSourceDir .\tmp\pdfio-src `
  -PdfiumProvider auto `
  -PdfiumPrebuiltRoot "D:/deps/pdfium" `
  -PdfiumSourceDir .\tmp\pdfium-workspace\pdfium `
  -OutputJson .\output\pdf-dependency-inputs\summary.json
```

这一步只检查本机 PDFio / PDFium 输入是否存在，不会运行 CMake、CTest、Ninja、
MSBuild、下载依赖或渲染 PDF。它会输出 `pdfio_ready`、`pdfium_ready`、
`selected_pdfium_provider` 和 `missing_inputs`，适合在重新配置
`.bpdf-roundtrip-msvc` 之前先确认本机是否具备真实输入。

只有当依赖输入检查通过后，才值得继续跑下面的 build-output 预检；否则完整
visual gate 仍会被 `FEATHERDOC_BUILD_PDF=OFF` / `FEATHERDOC_BUILD_PDF_IMPORT=OFF`
后的缺口阻断。

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\check_pdf_visual_release_gate_preflight.ps1 `
  -BuildDir .\.bpdf-roundtrip-msvc `
  -PdfioSourceDir .\tmp\pdfio-src `
  -PdfiumProvider prebuilt `
  -PdfiumPrebuiltRoot "D:/deps/pdfium" `
  -OutputJson .\output\pdf-visual-release-gate-preflight\summary.json
```

预检会先检查 PDFio / PDFium 依赖输入，再检查 build 目录、`CMakeCache.txt`、`CTestTestfile.cmake`、`CMakeCache.txt`
里的 `FEATHERDOC_BUILD_PDF` / `FEATHERDOC_BUILD_PDF_IMPORT` 是否同时为 `ON`、
`ctest -N` 中的 PDF 测试注册、既有 PDF 输出、manifest 驱动样本和可复用的
`PIL` / `fitz` 渲染 Python。
`-PdfioSourceDir`、`-PdfiumProvider`、`-PdfiumSourceDir`、`-PdfiumPrebuiltRoot`、
`-PdfiumLibrary`、`-PdfiumIncludeDir`、`-PdfiumRuntimeDll` 和 `-PdfiumRuntimeDir`
只覆盖 dependency input check 的输入来源；summary 会写出 `dependency_overrides`，
方便 reviewer 区分 CMake cache 推导值和本次命令显式覆盖值。
这些覆盖参数不会绕过 `pdf_build_options_enabled`、PDF CTest 注册、CLI baseline PDF、
visual baseline PDF 或 CJK text-layer PDF 等 release gate blockers。比如 PDFium prebuilt
已经齐备但 `tmp\pdfio-src\pdfio.h` 仍缺失时，预检仍会保持 `not_ready`，并把缺失的
PDFio header 记录到 `missing_inputs`。
其中 `ctest -N` 只在 build 目录、`CMakeCache.txt` 和 `CTestTestfile.cmake`
都存在时执行；缺失时只记录 `skipped` / `missing` 状态，不触发构建或渲染。
如果 CMake cache 存在但 `FEATHERDOC_BUILD_PDF=OFF` 或
`FEATHERDOC_BUILD_PDF_IMPORT=OFF`，预检会用 `pdf_build_options_enabled`
提前阻断，提示先重新配置 PDFio / PDFium 输入。
它不会创建虚拟环境、安装依赖、运行 PDF 渲染或触发构建。需要让缺失前置条件
直接失败时，额外传 `-Strict`。

`test/pdf_visual_release_gate_preflight_test.ps1` 里生成的 `fake-pdf-build`、fake ctest
和 fake python 只用于脚本契约测试，是 test fixture。它不是不可复用 release gate build
的示例，也不是 reusable release build substitute；不能作为 `run_pdf_visual_release_gate.ps1`
的可复用输入。真正可复用的 release build 必须来自当前 `dev` 的真实 CMake build，并同时包含 `CMakeCache.txt`、
`CTestTestfile.cmake`、PDF CTest 注册、CLI baseline PDF、visual baseline PDF、
CJK text-layer PDF、可复用渲染 Python，并在 CMake cache 中同时启用
`FEATHERDOC_BUILD_PDF=ON` 和 `FEATHERDOC_BUILD_PDF_IMPORT=ON`。

`run_pdf_visual_release_gate.ps1` 默认也会先执行同一套严格预检。只想确认完整门禁
前置条件时，可以运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 `
  -BuildDir .\.bpdf-roundtrip-msvc `
  -PdfioSourceDir .\tmp\pdfio-src `
  -PdfiumProvider prebuilt `
  -PdfiumPrebuiltRoot "D:/deps/pdfium" `
  -PreflightOnly
```

如果要把预检结果纳入发布治理汇总，而不是只停留在命令行输出，可以生成
release blocker 兼容的治理报告：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\write_pdf_visual_release_gate_preflight_governance_report.ps1 `
  -BuildDir .\.bpdf-roundtrip-msvc `
  -OutputDir .\output\pdf-visual-release-gate-preflight-governance
```

该报告会读取或生成预检 summary，并在预检为 `not_ready` 时输出
`pdf_visual_release_gate_preflight.build_outputs_missing` 阻塞项。它只做 JSON/Markdown
治理材料，不构建、不渲染 PDF、不安装依赖，也不会创建虚拟环境。生成的
`summary.json` 可以交给 `scripts/build_release_blocker_rollup_report.ps1` 聚合。

只有在你已经用其他方式确认 build 输出完整、且明确要绕过保护时，才传
`-SkipPreflight`。

运行预检或完整门禁时，建议先记录本轮命令启动的进程和输出目录。收尾时只清理这些
由本任务启动且已经不用的 PDF gate 资源，例如临时 `output/pdf-visual-release-gate*`
输出；不要为了释放内存而结束无关的 Office、浏览器、node、PowerShell 或外部构建进程。

如果你要一次性验证“文本回读 + 页面视觉”门禁，优先跑这条：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1
```

它会顺序执行：

- `pdf_cli_export`
- `pdf_regression_`
- `pdf_unicode_font_roundtrip_visual`
- CJK 样本的文本层 copy/search 检查，基于 regression manifest 中
  `expect_cjk=true` 的样本提取 PDF 文本层并核对 `expected_text`
- 核心样本的 PNG baseline 渲染，样本由 regression manifest 中
  `expect_visual_baseline=true` 的条目驱动，其中包含 `styled-text`、`mixed-style-text`、
  `underline-text`、`document-contract-cjk-style`、`document-eastasia-style-probe`、
  `mixed-cjk-punctuation-text` 和 `latin-ligature-text` 这组样式 / 文字塑形视觉样本
- 聚合 contact sheet 生成

主要产物会落到：

- `output/pdf-visual-release-gate/report/summary.json`
- `output/pdf-visual-release-gate/report/aggregate-contact-sheet.png`
- `output/pdf-visual-release-gate/report/cjk-copy-search/*-summary.json`
- `output/pdf-visual-release-gate/report/cjk-copy-search/*-text.txt`
- `output/pdf-visual-release-gate/unicode-font/report/summary.json`
- `output/pdf-visual-release-gate/unicode-font/report/comparison-contact-sheet.png`

## 字体选择

FeatherDoc 的 PDF 字体选择已经拆成两层：

- `PdfFontResolver`：把 `font_family`、`east_asia_font_family`、`bold`、
  `italic` 和文本内容映射到最终 `font_file_path`
- `PdfDocumentAdapterOptions`：把文档级默认字体、CJK 回退字体和映射表传给
  resolver

实用配置顺序是：

1. 先给 `font_mappings` 配显式映射
2. 再给 `font_file_path` 和 `cjk_font_file_path` 配默认文件
3. 最后才依赖系统字体回退

粗体 / 斜体策略是：

1. 标准 PDF base fonts 使用 Helvetica / Times / Courier 自带的 bold / italic 变体。
2. 显式 `font_mappings` 可以按 `bold` / `italic` 注册真实字体文件，resolver 会优先选中
   style-specific 映射。
3. 显式映射、默认字体或 fallback 只能找到 regular 字体时，resolver 会把该 run 标记为
   synthetic bold / italic；PDFio writer 会用 fill+stroke 做最小加粗，用 12 度斜切矩阵做
   最小斜体兜底。
4. synthetic 兜底只是防止样式完全丢失，不等价于真实字重或真实 italic 字体。发布级样本
   仍应优先配置真实 bold / italic 字体文件。

CJK 字体选择的默认顺序是：

1. run 或文档默认属性里的 `east_asia_font_family` 显式映射
2. `PdfDocumentAdapterOptions::cjk_font_file_path`
3. `FEATHERDOC_PDF_CJK_FONT` / `FEATHERDOC_TEST_CJK_FONT`
4. Windows 系统字体候选：Deng、Microsoft YaHei、SimHei、SimSun 等
5. Linux 系统字体候选：NotoSansCJK、AR PL UMing 等

CJK 检测会把中文、日文假名、韩文、注音、CJK 兼容符号和全角形式都视为
East Asia 字体路径候选，避免这些脚本误走 Latin fallback。

如果 run 已经先命中 Latin 字体，但 FreeType 检查发现该字体缺少当前 Unicode 文本中的
非忽略字形，resolver 会再尝试 East Asia 显式映射、`cjk_font_file_path` 或系统 CJK
fallback。这个兜底用于 CJK 符号、列表前缀和混排标点，避免文本被绑定到缺字形字体。

许可证义务按字体来源处理：FeatherDoc 当前只引用用户显式提供的字体或本机系统字体，
不把这些字体重新分发进仓库。若未来要随发行包捆绑 CJK 字体，必须先记录字体许可证、
分发条件和 NOTICE 要求，再把该字体加入默认 fallback。

对中文或混排内容，优先给 run / 默认属性设置 `east_asia_font_family`。
这样 `PdfFontResolver` 会先选 East Asia 字体，再回退到普通 `font_family`。

## 文字塑形

当 HarfBuzz 目标可用时，`FeatherDoc::Pdf` 会同时启用字体子集化和文字塑形：

- `FEATHERDOC_ENABLE_PDF_FONT_SUBSET=1`
- `FEATHERDOC_ENABLE_PDF_TEXT_SHAPER=1`

当前 `pdf_text_shaper` 提供独立桥接层：

- 输入 UTF-8 文本、字体文件路径和字号
- 输出 `PdfGlyphRun`，包含 glyph id、cluster、direction、script tag、language tag、x/y advance、x/y offset
- 通过 `pdf_text_shaper_has_harfbuzz()` 暴露当前构建是否启用 HarfBuzz
- `FEATHERDOC_TEST_RTL_FONT` 可指定 RTL 字体，用于验证 HarfBuzz direction / script 元数据
- `PdfTextShaperOptions` 现在也支持显式 direction / script / language override，未指定时继续走 HarfBuzz guess

当前 document adapter 会在 file-backed `PdfTextRun` 上保留成功塑形得到的
`PdfGlyphRun`，并优先使用 glyph advance 计算 layout 宽度和后续 run 坐标。Run 级
`rtl` 格式会映射为 shaper direction override；`language` / `bidi_language`
会映射为 shaper language override，RTL run 优先使用 `bidi_language`。writer
尚未支持非 LTR glyph-id stream 时仍回退字符串路径。
PDFio writer 会在满足安全条件的 shaped run 上创建独立 Type0 / CIDFontType2 字体资源：

- 为每个 shaped glyph occurrence 分配私有 CID
- 只接受 `PdfGlyphDirection::left_to_right` 的 shaped glyph CID 写出；RTL / 竖排先保留字符串路径
- 用 `CIDToGIDMap` 把 CID 映射到 HarfBuzz glyph id
- 用 `W` 数组写入 HarfBuzz x advance，避免回退到未塑形字宽
- 遇到非零 glyph x/y offset 或 y advance 时，逐 glyph 写 `Tm` + `Tj`，用 HarfBuzz
  pen position 定位；普通无 offset run 仍走整段 `<cid...> Tj` 快路径
- 同一 cluster 覆盖多个 glyph 时，只把第一个 CID 映射到完整 cluster Unicode 文本，后续同
  cluster CID 不写 ToUnicode entry，避免复制文本重复
- 用 ToUnicode / ActualText 保留原始文本提取语义

如果 run 没有 file-backed font、HarfBuzz 不可用、字体大小不匹配、direction 不是
`left_to_right`、cluster 越界或倒序、cluster 不在 UTF-8 codepoint 边界上，writer 会保留
原字符串路径。这样 RTL / 竖排等还没有完整方向语义的 shaped run 不会误走 LTR
glyph-id content stream。

单独验证文字塑形桥接层：

```powershell
cmake --build .bpdf-roundtrip-msvc --target pdf_text_shaper_tests pdf_document_adapter_font_tests
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font)$" --output-on-failure --timeout 60
```

如果你只想把一个已有 `.ttf` / `.ttc` 文件接进来，最直接的入口是：

```cpp
featherdoc::pdf::PdfDocumentAdapterOptions options;
options.font_mappings = {
    {"Unit Latin", "/path/to/latin.ttf"},
    {"Unit CJK", "/path/to/cjk.ttc"},
};
options.font_file_path = "/path/to/default-latin.ttf";
options.cjk_font_file_path = "/path/to/default-cjk.ttf";
```

手动运行：

```powershell
.bpdf-roundtrip-msvc/featherdoc_pdfium_probe.exe .bpdf-roundtrip-msvc/featherdoc-pdfio-probe.pdf
```

预期类似：

```text
parsed .bpdf-roundtrip-msvc\featherdoc-pdfio-probe.pdf (1 pages, 87 text spans)
```

## 当前可用状态

现在仍是实验性 smoke 可用，但已具备以下可复用结论：

- 可以生成带正文段落、表格和基础样式的 PDF 样例。
- 可以按字体映射和 CJK 回退写出中文 / 混排内容。
- 可以把 `Run` 的字体族/字号/颜色/粗体/斜体/下划线传到 PDFio writer。
- 可以做 Unicode / ToUnicode roundtrip 验证，并用 PDFium 解析页数和文字 span。
- 可以对 Unicode / CJK 字体 roundtrip 产物做 PNG 视觉 smoke。
- 可以独立调用 HarfBuzz shaper bridge，并让 layout 优先使用 GlyphRun 进位。
- 现有验证入口：`pdfio_generator_probe`、`pdf_document_generator_probe`、`pdfium_parser_probe`、`pdfium_document_parser_probe`、`pdf_import_structure`、`pdf_import_failure`、`pdf_import_table_heuristic`。
- 业务样本与支持矩阵关键标记：`manifest ID`、`support matrix`、`release checklist`、`text-first importer`、`general PDF-to-Word`、`ToUnicode`、`43 个 CJK text-layer 样本`、`RTL / Bidi`、`CJK`、`Word 段落语义`。
- 业务样本 ID：`contract-cjk-style`、`document-contract-cjk-style`、`invoice-grid-text`、`document-invoice-table-text`、`image-report-text`、`cjk-image-report-text`、`document-cjk-image-wrap-stress-text`、`long-report-text`、`document-long-flow-text`、`sectioned-report-text`、`header-footer-text`、`document-cjk-table-wrap-page-flow-text`。
- 详细历史记录见 `docs/building_pdf_current_status_archive.md`。
