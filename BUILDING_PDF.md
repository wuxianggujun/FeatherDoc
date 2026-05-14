# FeatherDoc PDF 构建手册

> PDF 能力仍是实验性方向。默认构建不会启用 PDFio / PDFium，也不会拉取、
> 编译或安装实验性 PDF 头文件。

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

## Windows 构建 PDFium import

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

默认 `PdfDocumentImporter` 遇到 `PdfParsedTableCandidate` 仍返回
`table_candidates_detected`，避免把表格误扁平化成正文。只有显式设置
`PdfDocumentImportOptions::import_table_candidates_as_tables=true` 时，才会尝试把简单
网格候选写入 `Document` 表格。当前实现还会在首个导入块是表格时，移除
`create_empty()` 留下的 body 占位段落，并通过 `inspect_body_blocks()` 做顺序验收。
当前回归还覆盖 2 行表头/数据表的保守识别和导入，以及
2 列 key-value 表的保守识别和导入；key-value 识别要求至少 3 行、每行恰好
2 个文本簇、左列全部像短标签、右列至少半数具备明显数据特征。
该识别基于文本几何，不要求源 PDF 存在可见网格线；无装饰线条 key-value 样本也有
独立回归覆盖。
不规则列宽表格只在“首行全是短标签表头、后续每行列数完整且至少半数单元格像数据值”
时放行，避免把普通三列表单误判成表格。
同一规则也覆盖缺失可见网格线的多列表格，回归样本会确认源 PDF 无线条时仍可导入为
真实 `Document` 表格。
普通双栏、编号列表和两列短标签 prose 仍不会被提升为表格。
此外还覆盖 `paragraph / table / table / paragraph` 的连续混排顺序，避免
连续表格写入时 body 游标错位；同一顺序也覆盖跨页场景，避免页面切换后游标断裂。
跨页 repeated-header 去重会先做 whitespace、大小写、常见分隔符（包含逗号和
括号）、单元格末尾复数、少量明确缩写（如 `Qty`/`Quantity`、`Amt`/`Amount`），
以及同一单元格内 token 完全一致时的词序归一化；语义不同的表头仍会保守拆成
独立表格。
当表体包含受控的横向跨列摘要行（例如 subtotal / total）时，repeated-header
入口会允许较长的业务表头参与同一套缩写匹配，避免跨页 subtotal 表格把第二页
缩写表头误写成正文行。
即使表体包含 subtotal / total 跨列行，语义不同的 repeated header 仍会通过
`repeated_header_mismatch` 保守拆成独立表格。
列锚点不兼容时也会优先通过 `column_anchors_mismatch` 拆表；表头文本相同或
subtotal / total 跨列行存在都不会绕过该版面边界。即使两页仍解析为相同列数，
局部内容漂移导致平均列锚间距不兼容时，也会保守拆成独立表格，并保留
`column_anchors_mismatch` 诊断。
列数不匹配时会通过 `column_count_mismatch` 保守拆成独立表格；表头文本、
缩写匹配或 subtotal / total 跨列行都不会绕过列数边界。
在列锚点和列数都兼容的前提下，受控的表体空单元格会补齐为空文本单元格。
单个空格、中间双空格、有完整相邻行支撑的金额-only 表体行，以及只依赖重复表头和
汇总行支撑列锚的孤立金额-only 表体行，仍可跨页续接；这不等同于支持局部列错位、
孤立自由表单或任意稀疏表。
整张表一致的不规则列宽可以被识别；每行列位置各自漂移的自由表单文本会保守保留为
段落，即使启用 `import_table_candidates_as_tables` 也不会写入表格。
规则网格里的组合合并已覆盖左上角 2x2、内部 2x2 和内部 2x3 矩形锚点；
这仍不等同于支持任意嵌套合并、扫描/OCR 或需要图像理解的表格。
跨页表格合并默认维持原有启发式；如果调用方更担心误合并，可以设置
`PdfDocumentImportOptions::min_table_continuation_confidence`，把低于阈值的候选保留为
独立表格，同时从 `table_continuation_diagnostics` 读取实际 confidence 和 blocker。
当候选两侧都存在 repeated header 时，诊断还会写入
`header_match_kind`，区分 exact、normalized text、plural variant、
canonical abbreviation 和 token-set 词序归一化等命中路径。
需要保留测试 DOCX 并做 Word -> PDF 视觉 smoke 时，可运行：

```powershell
$env:FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS='1'; ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60; Remove-Item Env:FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 -InputDocx .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-table.docx -OutputDir .\output\pdf-e7-table-import-docx-visual -ReviewNote "PDF import table candidate opt-in visual verification"
```

## CLI 导出入口

`featherdoc_cli export-pdf` 已经可以作为用户入口使用。当前常用参数是：

- `--output <pdf>`
- `--font-file <path>`
- `--cjk-font-file <path>`
- `--font-map <family>=<path>`
- `--render-headers-and-footers`
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

同时开启 PDFio 写出和 PDFium 读入后，可以直接跑首批 39 个 regression manifest 样本。
`pdf_regression_*` 当前覆盖 40 个 CTest，其中包含 manifest 校验测试。

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

其中 `cjk-text` 在找不到可用 CJK 字体时会跳过，不会把整个套件判失败。

## 视觉发布门禁

如果你要一次性验证“文本回读 + 页面视觉”门禁，优先跑这条：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1
```

它会顺序执行：

- `pdf_cli_export`
- `pdf_regression_`
- `pdf_unicode_font_roundtrip_visual`
- 核心样本的 PNG baseline 渲染，其中包含 `styled-text`、`mixed-style-text`、
  `underline-text`、`document-contract-cjk-style`、`document-eastasia-style-probe`、
  `mixed-cjk-punctuation-text` 和 `latin-ligature-text` 这组样式 / 文字塑形视觉样本
- 聚合 contact sheet 生成

主要产物会落到：

- `output/pdf-visual-release-gate/report/summary.json`
- `output/pdf-visual-release-gate/report/aggregate-contact-sheet.png`
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
- 输出 `PdfGlyphRun`，包含 glyph id、cluster、direction、script tag、x/y advance、x/y offset
- 通过 `pdf_text_shaper_has_harfbuzz()` 暴露当前构建是否启用 HarfBuzz
- `FEATHERDOC_TEST_RTL_FONT` 可指定 RTL 字体，用于验证 HarfBuzz direction / script 元数据
- `PdfTextShaperOptions` 现在也支持显式 direction / script tag override，未指定时继续走 HarfBuzz guess

当前 document adapter 会在 file-backed `PdfTextRun` 上保留成功塑形得到的
`PdfGlyphRun`，并优先使用 glyph advance 计算 layout 宽度和后续 run 坐标。Run 级
`rtl` 格式会映射为 shaper direction override；writer 尚未支持非 LTR glyph-id stream 时仍回退字符串路径。
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

现在只能算 **实验性 smoke 可用**，但已经不只是“最小段落文本”原型了：

- 可以生成带正文段落、表格和基础样式的 PDF 样例
- 可以按字体映射和 CJK 回退写出中文 / 混排内容
- 可以把 `Run` 的字体族/字体文件、字号、颜色、粗体、斜体和下划线从
  document adapter 传到 PDFio writer；段落继承样式和表格单元格样式已有
  `pdf_document_adapter_font` 回归
- 可以做 Unicode / ToUnicode roundtrip 验证，并用 PDFium 解析页数和文字 span
- 可以对 Unicode / CJK 字体 roundtrip 产物做 PNG 渲染级视觉 smoke
- 可以独立调用 HarfBuzz shaper bridge，把 UTF-8 文本和字体塑形成 GlyphRun
- 可以让 document adapter 在 `PdfTextRun` 上携带成功塑形的 `PdfGlyphRun`
- 可以让 layout 宽度和后续 run 坐标优先使用 GlyphRun 的 x advance
- 可以让 PDFio writer 对受控 shaped run 写出 glyph-id CID content stream，并用
  PDFium 回读验证文本不重复、不丢失；`pdf_unicode_font_roundtrip` 还会解压 PDF stream
  检查 shaped glyph ToUnicode CMap 的 cluster 映射，验证带 offset / y advance 的 glyph
  会逐 glyph 写定位矩阵，验证重复 cluster 只映射一次，并验证倒序 cluster / 非 UTF-8
  边界 cluster / 非 LTR direction 会回退到字符串路径
- 可以跑 PDFio → PDFium 的端到端 smoke
- 已有首批 39 个 regression manifest 样本，覆盖纯文本、多页文本、中文路径、中英混排标点、Latin ligature 文本、样式文本、字号、颜色、横向页面、标点、边框框体、基础线条、固定坐标表格外观、合同样式、页眉页脚、多栏文本、发票网格、图片说明文字、metadata 长标题，以及 sectioned/list/long report、image report、CJK report、CJK image report、document east-asian style probe、document image semantics、document table semantics、document long flow 和 document invoice table 这几个更接近真实文档流的生成型样本

还不能算正式可用：

- 还没有 `AST → PDFio` 完整翻译层；复杂分页、图片锚点/裁剪/环绕和发布级合同样式视觉 baseline 还没收口
- RTL / 竖排文字的方向模型还需要继续专项收口
- 还没有 `PDFium → AST` 文档结构重建
- 还需要继续扩充真实 PDF 样本回归集
- 发布级 CJK 字体许可证 / 捆绑策略、复杂表格分页和 PNG baseline 门禁还在专项推进中
