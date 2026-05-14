# PDF 后续执行路线与验收清单

> 时间窗口：当前主路线，0-6 个月。
>
> 目标：把已经跑通的 PDF 链路推进到“真实文档可用、问题可回归、发布可验收”。
>
> 上游路线：[02-current-roadmap.md](02-current-roadmap.md)。
> 架构前置：[01-architecture.md](01-architecture.md)。

## 当前判断

当前正在推进的不是单独的“生成 PDF writer probe”，而是：

```text
Document / DOCX 语义
    ↓
PdfDocumentLayout
    ↓
PDFio
    ↓
PDF
    ↓
PDFium 回读验证
```

也就是说，当前补充的测试应归类为 **文档生成 PDF** 方向。

单纯 PDF writer probe 已经完成基础验证，后续只保留为底层 smoke，不应继续作为主线目标。

## 当前基线

- PDF 能力默认仍然关闭，不影响默认构建。
- `IPdfGenerator` / `IPdfParser` 已经把 Core 与 PDFio / PDFium 隔离开。
- `Document -> PdfDocumentLayout -> PDFio -> PDF` 已经跑通。
- `PDFio -> PDF -> PDFium` 回读验证已经跑通。
- 当前 `pdf_regression_manifest.json` 包含 39 个样本。
- 当前 `ctest -R "pdf_regression_"` 覆盖 40 个回归测试，包含 manifest 校验。
- 已覆盖基础段落、字体解析、真实字体度量、CJK / ToUnicode 回读、基础样式、页眉页脚、动态页码、图片、表格，以及 `table_position` 水平/纵向基础映射。

## 路线总览

后续推进按依赖顺序拆成 7 个阶段：

```text
E1  回归基线收口与文档同步
 ↓
E2  Word 表格与图片语义补齐
 ↓
E3  真实文档导出样本集
 ↓
E4  CJK 与字体专项收口
 ↓
E5  CLI export-pdf 可用性
 ↓
E6  视觉回归与发布门禁
 ↓
E7  PDFium -> AST 读入方向
```

E1 到 E6 服务于 **Word / Document -> PDF**。

E7 是后续的 PDF 读入方向，不应阻塞当前导出主线。

## E1：回归基线收口与文档同步

### 目标

先把已经完成的能力固定下来，避免后续扩展时基线漂移。

### 任务

- [x] 已将现有设计文档里的旧样本数量同步为当前实际数量。
- [x] 确认 `design/02-current-roadmap.md` 与测试现状一致。
- [x] 确认 `BUILDING_PDF.md` 中的 PDF 构建命令仍能复现当前测试。
- [x] 保持 `FEATHERDOC_BUILD_PDF` 默认 OFF。
- [x] 保持 `FEATHERDOC_BUILD_PDF_IMPORT` 默认 OFF。
- [x] 固定 E1 当时 35 个 manifest 样本的命名、分类和最小断言。

### 验收

- [x] 文档中不存在过期的样本数量描述。
- [x] `pdf_document_adapter_font` 通过。
- [x] `pdf_regression_*` 通过。
- [x] 默认构建不拉取 PDFio / PDFium / FreeType / PDFium import。

### 验收记录

2026-05-07：

- 已确认 `pdf_regression_manifest.json` 在 E1 当时包含 35 个样本。
- 已确认 `ctest -R "pdf_regression_"` 在 E1 当时覆盖 36 个 CTest，包含 manifest 校验。
- 已将 `design/02-current-roadmap.md` 和 `BUILDING_PDF.md` 的旧样本数量同步为 35/36。
- 已修正 `sectioned-report-text` 的 manifest 基线：该样本包含 portrait 正文段和 landscape appendix，当前合理页数为 2 页。
- 已补强 `sectioned-report-text` 的文本断言，覆盖第二 section 的 appendix 正文回读。
- 已确认 `CMakeLists.txt` 中 `FEATHERDOC_BUILD_PDF` 和 `FEATHERDOC_BUILD_PDF_IMPORT` 默认均为 OFF。
- 已确认 `.bdefault-pdf-off/CMakeCache.txt` 中 `FEATHERDOC_BUILD_PDF:BOOL=OFF`、`FEATHERDOC_BUILD_PDF_IMPORT:BOOL=OFF`。
- 已完成可视化验证：将 `sectioned-report-text` PDF 渲染为 2 页 PNG，并生成 contact sheet；目检无空白页、明显裁剪或重叠。

通过命令：

```powershell
cmd /c '"D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build .bpdf-roundtrip-msvc --target pdf_document_adapter_font_tests featherdoc_pdf_regression_sample pdf_regression_manifest_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_document_adapter_font$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-regression-sectioned-report-text.pdf --output-dir .\output\pdf-e1-sectioned-report-visual\pages --summary .\output\pdf-e1-sectioned-report-visual\summary.json --contact-sheet .\output\pdf-e1-sectioned-report-visual\contact-sheet.png --dpi 144
```

可视化验证产物：

- `output/pdf-e1-sectioned-report-visual/summary.json`
- `output/pdf-e1-sectioned-report-visual/contact-sheet.png`

下一步入口：进入 E2，先阅读 `src/pdf/pdf_document_adapter_table_layout.cpp`、`src/pdf/pdf_document_adapter_tables.cpp`、`src/pdf/pdf_document_adapter_images.cpp`、`src/pdf/pdf_writer.cpp` 和 `test/pdf_document_adapter_font_tests.cpp`，确认 `table_position`、图片 crop / behind-text、现有 regression 样本的实际覆盖，再补缺口。

### 推荐验证命令

```powershell
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_document_adapter_font$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60
```

## E2：Word 表格与图片语义补齐

### 目标

把“看起来像表格/图片”的基础输出，推进到能承接真实 Word 语义。

### 任务

- [x] 扩展 `table_position` 水平定位基础覆盖：
  page reference、margin reference、column reference、负 offset。
- [x] 扩展 `table_position` 水平定位剩余边界：
  右对齐边界、定位表格与普通 alignment/indent 的优先级说明。
- [x] 扩展 `table_position` 纵向定位基础覆盖：
  page reference、margin reference、paragraph reference、负 offset、首行和非首个 block 表格。
- [x] 验证 positioned table 后续段落避让：
  positioned table 之后的正文应排在表格底部之后。
- [x] 验证 positioned table 与分页/页面区域的交互：
  表格跨页、页眉页脚区域不被遮挡。
- [x] 补齐表格分页语义：
  repeat header row、cant split row 的第一版处理。
- [x] 扩展图片语义：
  crop、inline size、anchor size、behind text、wrap none / square 的最小覆盖。
- [x] 为表格和图片新增 document-level 回归样本，而不是只测 layout helper。

### 验收

- [x] 表格定位断言同时覆盖 X / Y 坐标。
- [x] 表格跨页时不崩溃、不丢行。
- [x] 图片尺寸、裁剪和锚点位置有稳定断言。
- [x] 新增样本进入 `pdf_regression_manifest.json`。
- [x] PDFium 回读至少能确认页数和关键文本不丢失。

### 进展记录

2026-05-07：

- 已阅读 `src/pdf/pdf_document_adapter_table_layout.cpp`、`src/pdf/pdf_document_adapter_tables.cpp`、`src/pdf/pdf_document_adapter_images.cpp`、`src/pdf/pdf_writer.cpp` 和 `test/pdf_document_adapter_font_tests.cpp`。
- 已确认当前实现已经支持 `table_position` 的 page / margin / column 水平参考、page / margin / paragraph 纵向参考，以及 signed twips offset。
- 已补充 `pdf_document_adapter_font` 回归覆盖：
  margin reference + 负 horizontal offset、margin vertical reference + 负 vertical offset、column reference、paragraph vertical reference、非首个 block 的 positioned table。
- 已补充 right aligned table 边界断言，并确认显式 `table_position` 优先于普通 alignment / indent。
- 已补充 positioned table 后续段落避让断言，确认后续正文排在表格底部之后。
- 已确认新增断言覆盖表格矩形的 X 坐标和 row top Y 坐标。
- 已完成可视化验证：将 `featherdoc-adapter-table-roundtrip.pdf` 渲染为 PNG 并生成 contact sheet；目检无空白页、明显裁剪或重叠。
- 已将 `TableRow::cant_split()` 和 `TableRow::repeats_header()` 接入 `table_inspection_summary`，
  让 PDF adapter 能读取 row 级分页语义。
- 已补充 positioned table 跨页布局断言：表格跨页后继续落在内容区内，不压到页脚区域。
- 已实现 repeat header row 第一版：表格分页到新页时重复起始连续表头行。
- 已补充 cant split row 第一版验收：当前 PDF 表格行仍按原子行处理，页尾剩余空间不足时整行挪到下一页；超出单页可用高度的巨型行仍记录为后续限制。
- 已新增 PDFium roundtrip 样本 `featherdoc-adapter-table-pagination-roundtrip.pdf`，覆盖 positioned table 分页、重复表头和关键文本回读。
- 已完成可视化验证：将分页表格 PDF 渲染为 2 页 PNG/contact sheet；目检第 2 页表头重复，表格线和文本可见，无明显裁剪、重叠或页脚遮挡。
- 已补充 behind-text floating image 的 adapter 单测，确认 `draw_behind_text` 能从 Word 锚点语义传入 PDF layout。
- 已新增 document-level 图片回归样本 `document-image-semantics-text`，覆盖 inline 图片、square wrap floating 图片、crop 元数据和 behind-text floating 图片。
- 已新增 document-level 表格回归样本 `document-table-semantics-text`，覆盖文档流中的表格分页、重复表头和 cant-split 行元数据。
- 已在 E2 完成时将 `pdf_regression_manifest.json` 扩展到 35 个样本；`ctest -R "pdf_regression_"` 当时覆盖 36 个 CTest，包含 manifest 校验。
- 已完成新增图文样本可视化验证：彩色图片可见，inline / square wrap / behind-text 位置清晰，无明显裁剪或文字遮挡。
- 已完成新增表格样本可视化验证：2 页非空，第 2 页重复表头，表格线和文字可见，无明显裁剪或页脚遮挡。

通过命令：

```powershell
cmd /c '"D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build .bpdf-roundtrip-msvc --target pdf_document_adapter_font_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_document_adapter_font$" --output-on-failure --timeout 60
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-adapter-table-roundtrip.pdf --output-dir .\output\pdf-e2-table-roundtrip-visual\pages --summary .\output\pdf-e2-table-roundtrip-visual\summary.json --contact-sheet .\output\pdf-e2-table-roundtrip-visual\contact-sheet.png --dpi 144
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-adapter-table-pagination-roundtrip.pdf --output-dir .\output\pdf-e2-table-pagination-visual\pages --summary .\output\pdf-e2-table-pagination-visual\summary.json --contact-sheet .\output\pdf-e2-table-pagination-visual\contact-sheet.png --dpi 144
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-regression-document-image-semantics-text.pdf --output-dir .\output\pdf-e2-document-image-semantics-visual\pages --summary .\output\pdf-e2-document-image-semantics-visual\summary.json --contact-sheet .\output\pdf-e2-document-image-semantics-visual\contact-sheet.png --dpi 144
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-regression-document-table-semantics-text.pdf --output-dir .\output\pdf-e2-document-table-semantics-visual\pages --summary .\output\pdf-e2-document-table-semantics-visual\summary.json --contact-sheet .\output\pdf-e2-document-table-semantics-visual\contact-sheet.png --dpi 144
```

可视化验证产物：

- `output/pdf-e2-table-roundtrip-visual/summary.json`
- `output/pdf-e2-table-roundtrip-visual/contact-sheet.png`
- `output/pdf-e2-table-pagination-visual/summary.json`
- `output/pdf-e2-table-pagination-visual/contact-sheet.png`
- `output/pdf-e2-document-image-semantics-visual/summary.json`
- `output/pdf-e2-document-image-semantics-visual/contact-sheet.png`
- `output/pdf-e2-document-table-semantics-visual/summary.json`
- `output/pdf-e2-document-table-semantics-visual/contact-sheet.png`

下一步入口：E2 已完成。进入 E3 前，先重新阅读 `samples/pdf_regression_sample.cpp`、`test/pdf_regression_manifest.json`、CLI export-pdf 测试和真实文档样本相关实现，
确认现有真实文档级样本覆盖，再规划合同、发票、图文报告、多 section 和长文档样本的最小增量。

### 推荐验证命令

```powershell
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_document_adapter_font$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_(document|image|table|invoice|report)" --output-on-failure --timeout 60
```

## E3：真实文档导出样本集

### 目标

从“功能样本”升级为“真实文档样本”，让 PDF 导出质量能被稳定讨论。

### 样本范围

- [x] 中文合同：
  标题、编号、条款、签署区、页眉页脚、CJK 字体。
- [x] 发票 / 报价单：
  网格表格、金额、对齐、边框、合计区。
- [x] 图文报告：
  多段正文、图片、说明文字、列表、章节标题。
- [x] 多 section 文档：
  横向页、不同页边距、不同页眉页脚。
- [x] 长文档：
  至少 5 页，覆盖分页、页码和段落连续性。

### 每个样本的最低断言

- [x] PDF 可被 PDFium 打开。
- [x] 页数符合预期。
- [x] 标题、关键正文、页眉页脚文本可回读。
- [x] CJK 文本可回读为 Unicode。
- [x] 输出文件大小在合理范围内，没有异常膨胀。
- [x] 如包含图片，图片数量或关键尺寸可验证。

### 验收

- [x] 至少 5 个真实文档级样本进入 manifest。
- [x] 每个新增样本都能单独通过 CTest 运行。
- [x] 每个样本都有失败时可定位的断言信息。
- [x] 真实样本和功能样本在 manifest 中分类清楚。

### 进展记录

2026-05-07：

- 开始 E3 前已重新阅读 `samples/pdf_regression_sample.cpp`、`test/pdf_regression_manifest.json`、`test/pdf_regression_manifest_test.cpp`、`design/02-current-roadmap.md` 和 `BUILDING_PDF.md`，确认真实进度是 36 个 manifest 样本、37 个 `pdf_regression_` CTest。
- 已新增页眉页脚动态页码占位符展开能力，支持 `{{page}}`、`{{total_pages}}`、`{{section_page}}` 和 `{{section_total_pages}}`，默认关闭，仅由样本显式启用。
- 已增强 `document-contract-cjk-style`，加入合同编号、中文条款、页眉和动态页脚页码，并完成 CJK 可视化验证。
- 已新增 `document-long-flow-text`，生成 5 页文档流样本，覆盖页眉、动态页脚页码、分页和第 1/60/120 段连续性。
- 已新增 `document-invoice-table-text`，生成 1 页发票/报价单样本，覆盖表格列宽、明细金额、边框和合计区。
- 当前 `pdf_regression_manifest.json` 已扩展到 37 个样本；`ctest -R "pdf_regression_"` 覆盖 38 个 CTest，包含 manifest 校验。
- 已完成 E3 关键样本可视化验证：合同 1 页、长文档 5 页、发票 1 页均已渲染 PNG/contact sheet；目检无空白页、明显裁剪、重叠、页眉页脚错位或表格内容换行异常。
- 已将回归样本 runner 补齐为自动化检查：所有样本按页数做输出大小上限校验，含图样本额外通过 `expected_image_count` 回读 image object 数量；这两项已不再依赖手工目检。

通过命令：

```powershell
cmd /c '"D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build .bpdf-roundtrip-msvc --target featherdoc_pdf_regression_sample pdf_regression_manifest_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_document_adapter_font$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_regression_manifest$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_document-contract-cjk-style" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_document-long-flow-text" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_document-invoice-table-text" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-regression-document-contract-cjk-style.pdf --output-dir .\output\pdf-e3-document-contract-cjk-visual\pages --summary .\output\pdf-e3-document-contract-cjk-visual\summary.json --contact-sheet .\output\pdf-e3-document-contract-cjk-visual\contact-sheet.png --dpi 144
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-regression-document-long-flow-text.pdf --output-dir .\output\pdf-e3-document-long-flow-visual\pages --summary .\output\pdf-e3-document-long-flow-visual\summary.json --contact-sheet .\output\pdf-e3-document-long-flow-visual\contact-sheet.png --dpi 144
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-regression-document-invoice-table-text.pdf --output-dir .\output\pdf-e3-document-invoice-table-visual\pages --summary .\output\pdf-e3-document-invoice-table-visual\summary.json --contact-sheet .\output\pdf-e3-document-invoice-table-visual\contact-sheet.png --dpi 144
```

可视化验证产物：

- `output/pdf-e3-document-contract-cjk-visual/summary.json`
- `output/pdf-e3-document-contract-cjk-visual/contact-sheet.png`
- `output/pdf-e3-document-long-flow-visual/summary.json`
- `output/pdf-e3-document-long-flow-visual/contact-sheet.png`
- `output/pdf-e3-document-invoice-table-visual/summary.json`
- `output/pdf-e3-document-invoice-table-visual/contact-sheet.png`

下一步入口：E3 真实文档样本集已收口。进入 E4 前，先阅读 `include/featherdoc/pdf/pdf_font_resolver.hpp`、`src/pdf/pdf_font_resolver.cpp`、`src/pdf/pdf_font_subset.cpp`、`src/pdf/pdf_text_metrics.cpp`、`test/pdf_font_resolver_tests.cpp` 和 `test/pdf_unicode_font_roundtrip_tests.cpp`，确认 CJK fallback、字体子集化、ToUnicode 和缺字体诊断当前做到哪一步。

## E4：CJK 与字体专项收口

### 目标

把中文 PDF 的复制、搜索、字体嵌入和 fallback 行为做成可验收能力。

### 任务

- [x] 明确默认 CJK fallback 字体策略。
- [x] 记录选用字体的许可证义务。
- [x] 确认字体子集化不会把未使用 glyph 写入 PDF。
- [x] 确认 ToUnicode CMap 覆盖中文、英文、数字、标点和混排。
- [x] 补齐缺字体时的错误提示和 fallback 诊断。
- [x] 增加字体 resolver 的边界测试：
  缺 regular、缺 bold、缺 italic、路径不存在、字体文件损坏。

### 验收

- [x] 中文 PDF 在 PDFium 中回读文本一致。
- [ ] 中文 PDF 在常见阅读器中可复制、可搜索。
- [x] 生成 PDF 中嵌入的是字体子集，不是完整大字体。
- [x] 缺字体时有明确错误或明确 fallback，不静默输出乱码。

### 进展记录

2026-05-07：

- 开始 E4 前已阅读 `include/featherdoc/pdf/pdf_font_resolver.hpp`、`src/pdf/pdf_font_resolver.cpp`、`src/pdf/pdf_font_subset.cpp`、`src/pdf/pdf_text_metrics.cpp`、`src/pdf/pdf_writer.cpp`、`test/pdf_font_resolver_tests.cpp` 和 `test/pdf_unicode_font_roundtrip_tests.cpp`，确认已有显式映射、默认字体、CJK fallback、样式变体、FreeType 度量、HarfBuzz 子集化和 PDFium 回读基础覆盖。
- 已补充 resolver 边界测试：缺失显式映射时回退到默认字体；禁用系统 fallback 且无 CJK 字体时返回空路径和 Unicode 标记，供上层诊断。
- 已补充 writer 诊断：Unicode 文本没有解析出嵌入字体文件时直接失败，错误包含 `Unicode PDF text requires an embedded font file` 和字体族名。
- 已补充 writer 缺字体/坏字体测试：不存在的字体路径、损坏字体文件都会失败并在错误信息里带出字体路径。
- 已将 Unicode roundtrip 文本扩展为中文、英文、数字、中文标点混排，确认 PDFium 回读和 ToUnicode 路径覆盖混排标点。
- 已确认 subset PDF 明显小于 full PDF：本地视觉 smoke 中 full 为 10,213,214 bytes，subset 为 19,874 bytes；两者渲染内容一致。
- 已在 `BUILDING_PDF.md` 记录当前 CJK fallback 顺序和字体许可证义务：默认使用系统/用户显式提供字体，不把系统字体重新分发进仓库。
- 已知限制：常见阅读器中的手工复制/搜索还没有形成自动化或人工验收记录；后续需要在 E6 发布门禁中补上可复现检查。

通过命令：

```powershell
cmd /c '"D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build .bpdf-roundtrip-msvc --target pdf_font_resolver_tests pdf_unicode_font_roundtrip_tests pdf_text_metrics_tests'
cmd /c '"D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build .bpdf-roundtrip-msvc --target pdf_unicode_font_roundtrip_tests featherdoc_pdf_regression_sample pdf_document_adapter_font_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_(font_resolver|text_metrics|unicode_font_roundtrip)$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_document_adapter_font$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_document-(contract-cjk-style|long-flow-text|invoice-table-text)" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_unicode_font_roundtrip_visual" --output-on-failure --timeout 60
```

可视化验证产物：

- `.bpdf-roundtrip-msvc/pdf_unicode_font_visual_regression/report/summary.json`
- `.bpdf-roundtrip-msvc/pdf_unicode_font_visual_regression/report/full-contact-sheet.png`
- `.bpdf-roundtrip-msvc/pdf_unicode_font_visual_regression/report/subset-contact-sheet.png`
- `.bpdf-roundtrip-msvc/pdf_unicode_font_visual_regression/report/comparison-contact-sheet.png`

下一步入口：进入 E5 前，先阅读 `cli/featherdoc_cli.cpp`、`cli/featherdoc_cli_usage.cpp`、`test/pdf_cli_export_tests.cpp`、`test/cli_tests.cpp` 和 `test/cli_usage_tests.cpp`，确认 `export-pdf` 参数、未启用 PDF 时的错误、font mapping / CJK fallback 参数和 `--summary-json` 当前做到哪一步。

### 推荐验证命令

```powershell
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_(font_resolver|text_metrics|unicode_font_roundtrip)" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_(cjk|document-eastasia)" --output-on-failure --timeout 60
```

## E5：CLI export-pdf 可用性

### 目标

让 PDF 导出不只停留在测试和 probe，而是有稳定的用户入口。

### 任务

- [x] 固定 `export-pdf` 命令的参数命名和帮助文本。
- [x] 覆盖输入 DOCX / 输出 PDF 的常规路径。
- [x] 支持 font mapping 参数。
- [x] 支持 CJK fallback 参数。
- [x] 支持页眉页脚、图片、表格的开关或诊断输出。
- [x] 支持 `--summary-json`，输出稳定机器可读结果。
- [x] 错误路径要清楚：
  文件不存在、PDF 模块未启用、缺字体、PDFio 写出失败、PDFium 回读失败。

### 验收

- [x] `featherdoc --help` 能看到 PDF 导出入口。
- [x] CLI 导出的 PDF 能进入 PDFium 回读。
- [x] 成功和失败场景都有测试。
- [x] JSON summary 字段稳定，不随日志文案变化。
- [x] 未启用 PDF 构建时，CLI 给出明确提示。

### 进展记录

2026-05-07：

- 已先阅读 `cli/featherdoc_cli.cpp`、`cli/featherdoc_cli_usage.cpp`、`test/pdf_cli_export_tests.cpp`、`test/cli_tests.cpp` 和 `test/cli_usage_tests.cpp`，
  确认当前 E5 实际进度：CLI 已具备 `export-pdf` 用户入口，`--output`、`--font-file`、`--cjk-font-file`、`--font-map`、
  `--render-headers-and-footers`、`--render-inline-images`、`--no-font-subset`、`--no-system-font-fallbacks`、`--summary-json` 和 `--json`。
- 已补充 `--font-map` 的解析和转发，`--summary-json` 的机器可读输出，以及 `--no-system-font-fallbacks` 的诊断开关。
- 已补充 `pdf_cli_export` 回归覆盖：成功导出、inline image、font-map、非法 font-map、缺输入、缺字体。
- 已补充 `cli_usage` 断言，固定 `export-pdf` 的帮助文本和新增参数名。
- 已完成可视化验证：CLI 生成的 `font-map-source.pdf` 已渲染为 3 页 PNG/contact sheet；目检无空白页、明显裁剪、重叠或页眉页脚错位。
- 已确认成功路径在 roundtrip 构建里能被 PDFium 回读，正文页数与关键文本一致。
- 已知限制：当前 CLI 还没有单独暴露“PDFium 回读失败”作为独立用户子命令，现阶段通过 PDFium parser 测试和 roundtrip 回归覆盖。

### 推荐验证命令

```powershell
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_cli_export$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "cli" --output-on-failure --timeout 60
```

## E6：视觉回归与发布门禁

### 目标

PDF 不是只看文本是否回读。最终要建立页面渲染级别的质量门禁。

所有生成的 PDF 都必须进入可视化验证流程。文本回读只能证明内容还在，不能证明版面正确；
PDF 产物至少要渲染为 PNG，并检查是否存在文字裁剪、重叠、错位、缺字、图片异常、页眉页脚
越界等问题。

### 任务

- [x] 固定 PDF -> PNG 渲染脚本。
- [x] 为核心样本生成 baseline PNG。
- [x] 每个新增或变更的 PDF 生成样本，都要接入可视化验证或记录明确的临时豁免原因。
- [x] 定义 baseline 更新规则：
  只有有意图的视觉变化才能更新。
- [x] 对页面尺寸、页数、文本回读、关键像素区域做组合验收。
- [x] 为本地开发保留快速 smoke。
- [x] 为 CI 保留较慢但稳定的完整回归。

### 验收

- [x] 至少合同、发票、图文报告 3 类样本有视觉 baseline。
- [x] 新增 PDF 输出能力必须提供 PNG 渲染检查记录。
- [x] 视觉回归失败时能定位到样本名和页码。
- [x] baseline 更新不依赖手工拷贝临时文件。
- [x] 发布前必须通过文本回读和视觉回归两类门禁。

### 推荐验证命令

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_unicode_font_roundtrip_visual" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60
```

### 进展记录

2026-05-07：

- 已落地 `scripts/run_pdf_visual_release_gate.ps1`，把 `pdf_cli_export`、
  `pdf_regression_`、`pdf_unicode_font_roundtrip_visual`、核心样本 PNG baseline
  和 aggregate contact sheet 串成一条可复现门禁。
- 已通过 `powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1`。
- 可视化产物：
  `output/pdf-visual-release-gate/report/aggregate-contact-sheet.png`
  和 `output/pdf-visual-release-gate/unicode-font/report/comparison-contact-sheet.png`。
- 下一步入口：E6 已收口，后续只在需要推进 PDFium -> AST 读入时进入 E7。

## E7：PDFium -> AST 读入方向

### 目标

保留 PDF 读入能力，但不要让它阻塞当前 Word / Document -> PDF 导出。

### 任务

- [x] 定义 PDFium text span 到 Core AST 的中间结构第一版：
  当前是 `PdfParsedTextLine` / `PdfParsedParagraph`。
- [x] 先支持纯文本和段落识别第一版：
  受控文本 PDF 可从字符 span 聚合为行和段落，并可导入为纯文本 `Document`。
- [x] 再支持简单表格启发式识别第一版：
  当前是保守的 `PdfParsedTableCandidate` 候选识别，不承诺直接还原成 Word 表格。
- [x] 明确不支持能力第一版：
  扫描件 OCR、复杂矢量图还原、任意 PDF 精确还原成 Word。
- [x] 将读入方向的失败样本单独分类第一版，不混入导出主线。

### 验收

- [x] PDFium probe 不影响默认构建。
- [x] PDF 读入测试和 PDF 导出测试能分开运行。
- [x] PDF 读入能力有清楚的“不支持”诊断第一版。
- [x] 读入方向不阻塞 E1 到 E6 的发布门禁。

### 进展记录

2026-05-07：

- 开始 E7 前已阅读 `src/pdf/pdfium_parser.cpp`、`include/featherdoc/pdf/pdf_interfaces.hpp`、
  `test/CMakeLists.txt` 和现有 PDFium probe 测试，确认此前 PDFium 读入只提取页尺寸与
  字符级 `text_spans`，尚无行、段落或 `Document` 还原。
- 已新增 `PdfParsedTextLine` 和 `PdfParsedParagraph` 中间结构，并在 `PdfParsedPage`
  中保留 `text_spans`、`text_lines`、`paragraphs` 三层结果。
- 已在 `PdfiumParser` 中增加字符 span 到行、段落的保守聚合逻辑；该逻辑仅在
  `extract_text=true` 且 `extract_geometry=true` 时执行。
- 已新增 `pdf_import_structure` 测试：用 PDFio 生成三行受控文本 PDF，再用 PDFium
  回读并断言 3 行、2 段，确保读入结构测试可与导出回归分开运行。
- 本轮没有新增或变更面向发布的 PDF 输出样本，因此没有新增视觉 baseline；生成 PDF
  的可视化发布门禁仍由 E6 的 `run_pdf_visual_release_gate.ps1` 覆盖。
- 已知限制：当前只完成 `PDFium -> 中间结构`，还没有 `PDFium -> Document` AST 转换；
  段落识别是启发式，不承诺复杂 PDF、扫描件或任意版面精确还原。
- 后续继续 E7 前，已再次阅读 `src/pdf/pdfium_parser.cpp`、`src/paragraph.cpp`、
  `src/document.cpp`、`include/featherdoc.hpp`、`test/basic_tests.cpp`、`test/CMakeLists.txt`
  和 PDF import target 的 CMake 配置，确认 `Document::create_empty()`、`Paragraph::set_text()`、
  `Paragraph::insert_paragraph_after()` 是当前最小 DOCX AST 写入路径。
- 已新增 `PdfDocumentImporter` 和 `import_pdf_text_document()`，作为实验性
  `PDFium -> PdfParsedDocument -> Document` facade；第一版只导入文本段落，不导入样式、
  图片、表格、页眉页脚或复杂版面。
- 已扩展 `pdf_import_structure` 测试：覆盖结构解析、纯文本 PDF 导入 `Document`、
  DOCX 保存后重开文本一致、禁用 geometry 的明确错误，以及无文本 PDF 的“不支持”
  诊断。
- 已新增 `PdfDocumentImportFailureKind` 稳定失败分类枚举，并新增独立
  `pdf_import_failure` 测试目标，单独覆盖 `extract_text` 禁用、`extract_geometry`
  禁用、无文本 PDF 和 parse 失败 4 类读入失败样本；这些失败样本不进入导出 regression
  manifest。
- 已新增 `PdfParsedTableCell` / `PdfParsedTableRow` / `PdfParsedTableCandidate`
  中间结构，并在 `PdfParsedPage` 中保留 `table_candidates`。
- 已在 `PdfiumParser` 中新增简单表格启发式第一版：
  先按字符 span 的 X 间隔把同一行拆成 cell cluster，再按稳定行距和列锚点聚合成
  `PdfParsedTableCandidate`。
- 已新增独立 `pdf_import_table_heuristic` 测试目标：
  用受控 `table-like grid` PDF 验证命中 3x3 稀疏表格候选，用 `two-column` PDF
  验证普通双栏正文不会误判成表格。
- 已将 `PdfParsedTableCandidate` 接入 `PdfDocumentImporter` 的边界诊断：
  纯文本 importer 在检测到表格候选时不再继续扁平导入，而是返回稳定
  `table_candidates_detected` 失败分类，避免误导为“已支持表格导入”。
- 已将 `PdfDocumentImportOptions::import_table_candidates_as_tables` 接入
  `PdfDocumentImporter` 第一版：
  默认仍保持 `table_candidates_detected` 失败分类；显式 opt-in 后，会把简单网格型
  `PdfParsedTableCandidate` 写入 `Document::append_table()`，并统计到
  `PdfDocumentImportResult::tables_imported`。
- 已补齐导入顺序处理：
  当首个导入块是表格时，会主动移除 `create_empty()` 生成的 body 占位段落，避免
  `Document` 头部残留空白段落；同时按页内 `paragraph/table` 的 Y 位置统一排序，
  不再把表格和段落分开两轮追加。
- 已新增 `pdf_import_table_heuristic` 的 opt-in 表格导入测试：
  受控 `table-like grid` PDF 在启用选项后可导入为 3x3 表格，保存重开后可确认
  `Cell A1`、`Cell B2`、`Cell C3` 保留。
- 已新增 body 顺序验收：
  使用 `inspect_body_blocks()` 断言 `paragraph / table / paragraph` 的真实 body 顺序，
  并覆盖“首块为表格时不留下空前导段落”的回归样本。
- 已新增连续表格顺序验收：
  使用 `inspect_body_blocks()` 断言 `paragraph / table / table / paragraph`
  的真实 body 顺序，并覆盖两个连续表格写入后再接尾段落的回归样本。
- 已新增跨页顺序验收：
  使用 `inspect_body_blocks()` 断言 `paragraph / table / table / paragraph`
  在跨页场景下仍保持稳定，并覆盖页面切换后的 body 游标连续性。
- 已补充 `pdf_import_table_heuristic` 负样本：
  编号列表左侧编号、右侧说明文字且行距稳定时，不应被误判为表格候选；当前启发式
  因此保持保守，只接受至少 3 个列锚点的规则网格候选。
- 已补充票据式三列布局负样本：
  `invoice summary` 这类标题 + 非均匀三列明细不应被误判成表格候选。
- 已完成可视化验证：
  将 `featherdoc-pdf-import-table-like-grid.pdf` 和
  `featherdoc-pdf-import-two-column.pdf` 渲染为 PNG/contact sheet；目检确认前者是
  网格型输入、后者是普通双栏输入，和启发式测试预期一致。
- 已完成表格导入结果的可视化验证：
  `featherdoc-pdf-import-table.docx` 先转成
  `output/pdf-e7-table-import-docx-visual/table_visual_smoke.pdf`，
  再渲染为 PNG/contact sheet；视觉报告结论为 `pass`，1 页内容非空，
  网格和文本未见明显裁剪或重叠。
- 已完成新增顺序回归样本的结构验收：
  `featherdoc-pdf-import-table-first.pdf` 和
  `featherdoc-pdf-import-paragraph-table-paragraph.pdf` 在启用表格导入后，分别验证
  “首表格无空前导段落”与“段落-表格-段落顺序保持”。
- 本轮新增的是导入侧中间结构、测试内部受控 PDF 和视觉核对记录，不新增面向发布的
  PDF 输出样本；仍不新增发布 baseline。后续一旦增加新的发布样本或改变导出 PDF，
  必须回到 E6 视觉门禁。
- 已知限制更新：
  纯文本段落导入会保留 PDFium 聚合出的段落换行，但不保证原 PDF 的视觉布局、样式、
  列、表格或图片语义。
- 已知限制更新：
  `PdfDocumentImporter` 当前要求 `extract_text=true` 且 `extract_geometry=true`；
  `extract_geometry=false` 会直接返回 `extract_geometry_disabled`，不会降级为纯文本
  直通导入。该边界由 `pdf_import_failure` 覆盖，并确认失败时不会污染目标
  `Document`。
- 已知限制更新：
  当前表格导入只覆盖“稳定行距 + 规则列锚点”的简单网格型文本，仍不覆盖两行表格、
  两列表格、不规则 invoice、跨列/跨行单元格、复杂分页表格或页内块顺序的精确还原；
  这一版是保守的 opt-in 表格导入，不代表已完成完整 PDF -> Word 表格 AST 还原。

通过命令：

```powershell
cmd /c '"D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake -S . -B .bpdf-roundtrip-msvc && cmake --build .bpdf-roundtrip-msvc --target pdf_import_structure_tests'
cmd /c '"D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build .bpdf-roundtrip-msvc --target pdf_import_structure_tests && ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_structure$" --output-on-failure --timeout 60'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_structure$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_failure$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure)$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_(import_structure|unicode_font_roundtrip|regression_manifest)$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdfium_.*probe|pdf_import_(structure|failure)" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_(import_structure|import_failure|unicode_font_roundtrip|regression_manifest)$" --output-on-failure --timeout 60
cmd /c '"D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build .bpdf-roundtrip-msvc --target pdf_import_structure_tests pdf_import_failure_tests pdf_import_table_heuristic_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdfium_.*probe|pdf_import_(structure|failure|table_heuristic)|pdf_(unicode_font_roundtrip|regression_manifest)$" --output-on-failure --timeout 60
$env:FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS='1'; ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60; Remove-Item Env:FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 -InputDocx .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-table.docx -OutputDir .\output\pdf-e7-table-import-docx-visual -ReviewNote "PDF import table candidate opt-in visual verification"
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-table-like-grid.pdf --output-dir .\output\pdf-e7-table-heuristic-visual\table-like\pages --summary .\output\pdf-e7-table-heuristic-visual\table-like\summary.json --contact-sheet .\output\pdf-e7-table-heuristic-visual\table-like\contact-sheet.png --dpi 144
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-two-column.pdf --output-dir .\output\pdf-e7-table-heuristic-visual\two-column\pages --summary .\output\pdf-e7-table-heuristic-visual\two-column\summary.json --contact-sheet .\output\pdf-e7-table-heuristic-visual\two-column\contact-sheet.png --dpi 144
```

可视化验证产物：

- `output/pdf-e7-table-heuristic-visual/table-like/summary.json`
- `output/pdf-e7-table-heuristic-visual/table-like/contact-sheet.png`
- `output/pdf-e7-table-heuristic-visual/two-column/summary.json`
- `output/pdf-e7-table-heuristic-visual/two-column/contact-sheet.png`
- `output/pdf-e7-table-import-docx-visual/table_visual_smoke.pdf`
- `output/pdf-e7-table-import-docx-visual/evidence/contact_sheet.png`
- `output/pdf-e7-table-import-docx-visual/report/summary.json`
- `output/pdf-e7-table-import-docx-visual/report/final_review.md`

下一步入口：继续 E7 时，先重新阅读 `include/featherdoc/pdf/pdf_interfaces.hpp`、
`src/pdf/pdfium_parser.cpp`、`include/featherdoc/pdf/pdf_document_importer.hpp`、
`src/pdf/pdf_document_importer.cpp` 和 `test/pdf_import_table_heuristic_tests.cpp`，
再决定下一步是：

- 扩展负样本集，继续压低多栏、列表、标题组合、invoice grid 的误判率；或
- 补齐更复杂的表格读入第一版，例如多表格顺序、混合段落与表格的页面顺序、跨页
  限制说明，以及更多导入后视觉验证样本。

进入下一步前仍需保持失败样本分类独立，避免把读入方向的不稳定样本混入导出主线回归。

2026-05-10：

- 本轮开始前已重新阅读 `include/featherdoc/pdf/pdf_interfaces.hpp`、
  `src/pdf/pdfium_parser.cpp`、`src/pdf/pdf_document_importer.cpp`、
  `test/pdf_import_structure_tests.cpp` 和本文 E7 段落，并对齐当前真实进度：
  E7 已有纯文本导入、表格候选、opt-in 表格导入和导入侧独立测试目标。
- 已补强 PDFium 解析结果的阅读顺序恢复：
  `PdfiumParser` 现在会在字符 span 聚合为 `PdfParsedTextLine` 后，按几何
  `top -> left` 顺序稳定排序，再生成 `PdfParsedParagraph` 和
  `PdfParsedTableCandidate`；这避免受控 PDF 中底部文本先写入时把段落顺序反过来。
- 已补强 `PdfDocumentImporter` 的块级排序：
  页内 `paragraph/table` 导入块现在按 `top -> left` 排序，保留原有
  table-overlap 过滤和首表格删除占位段落逻辑，避免同一高度附近的段落/表格靠写入顺序漂移。
- 已新增最小受控 import 样本
  `write_out_of_order_paragraph_table_paragraph_pdf()`：
  视觉布局仍是 `paragraph -> table -> paragraph`，但文本对象故意按
  `tail paragraph -> table rows -> intro paragraph` 写入，用来回归“PDF 内容流顺序
  不等于阅读顺序”的导入场景。
- 已补充 `pdf_import_structure` 测试：
  `PDFium parser recovers paragraph order from out-of-order text runs` 断言新样本
  回读后仍恢复为 6 条视觉顺序 text lines，首段是 intro，末段是 tail，并继续识别
  1 个表格候选。
- 已补充 `pdf_import_table_heuristic` 测试：
  `PDF text importer preserves body order from out-of-order text runs` 在显式启用
  `import_table_candidates_as_tables=true` 后，断言真实 `Document` body 顺序为
  `paragraph / table / paragraph`，保存重开后顺序仍稳定，表格仍是 3x3 可编辑表格。
- 已完成新增 PDF 样本的 PNG/contact sheet 可视化验证：
  两个新样本均渲染为 1 页 PNG，并生成 contact sheet；页面 PNG 非空，
  非空像素包围盒为 `(143, 118, 865, 546)`，视觉内容与预期的段落-表格-段落布局一致。
- 本轮仍保持导入侧测试和导出回归分离：
  新样本只存在于 `pdf_import_structure` / `pdf_import_table_heuristic` 测试内部，
  未加入 `pdf_regression_manifest.json`，也未重复运行完整导出回归。
- 已知限制更新：
  当前阅读顺序恢复仍是页面内 `top -> left` 的保守线性化，不承诺多栏文章、
  旋转文字、浮动/重叠文本、页眉页脚语义或任意 PDF 内容流的精确阅读顺序。
- 已知限制更新：
  表格导入仍是 opt-in，只覆盖稳定行距 + 规则列锚点的简单网格；仍不覆盖跨行/跨列合并、
  两列表格、不规则票据表单、复杂分页表格、图片或样式还原。

本轮通过命令：

```powershell
cmd /c '"D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build .bpdf-roundtrip-msvc --target featherdoc_pdfio_probe featherdoc_pdf_document_probe featherdoc_pdfium_probe pdf_import_structure_tests pdf_import_failure_tests pdf_import_table_heuristic_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^(pdfio_generator_probe|pdf_document_generator_probe|pdfium_parser_probe|pdfium_document_parser_probe)$" --output-on-failure --timeout 60
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-structure-reordered.pdf --output-dir .\output\pdf-e7-order-recovery-visual\structure-reordered\pages --summary .\output\pdf-e7-order-recovery-visual\structure-reordered\summary.json --contact-sheet .\output\pdf-e7-order-recovery-visual\structure-reordered\contact-sheet.png --dpi 144
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-structure-reordered-source.pdf --output-dir .\output\pdf-e7-order-recovery-visual\structure-reordered-source\pages --summary .\output\pdf-e7-order-recovery-visual\structure-reordered-source\summary.json --contact-sheet .\output\pdf-e7-order-recovery-visual\structure-reordered-source\contact-sheet.png --dpi 144
```

本轮可视化验证产物：

- `output/pdf-e7-order-recovery-visual/structure-reordered/summary.json`
- `output/pdf-e7-order-recovery-visual/structure-reordered/contact-sheet.png`
- `output/pdf-e7-order-recovery-visual/structure-reordered-source/summary.json`
- `output/pdf-e7-order-recovery-visual/structure-reordered-source/contact-sheet.png`

下一步入口：

- 继续 E7 时，优先补更难的导入负样本和边界说明：表格旁注/标题、跨页表格候选、
  以及更复杂的多栏/混排页；当前两栏样本只覆盖 row-wise 保序，不代表列语义已恢复。
- 若继续推进真实表格 AST，还应先设计 `PdfParsedTableCandidate` 到合并单元格、
  行/列宽和表头语义的中间表示，而不是直接扩展当前简单网格启发式。

2026-05-10 继续推进：

- 已将跨页连续 `PdfParsedTableCandidate` 从“两个独立表格”提升为“同一个可编辑
  `Document` 表格”的受控导入能力：
  当上一页最后一个导入块是表格、下一页第一个导入块也是表格，且列数与列宽间距兼容时，
  importer 会把下一页候选行追加到上一页已经创建的 `Table`。
- 已保持同页连续表格不合并：
  现有 `paragraph / table / table / paragraph` 回归仍保留两个独立表格，避免把同页两个
  视觉上分离的表格误合并。
- 已补强导入器状态：
  `ImportCursor` 现在记录上一个表格的行数、列数和列锚点；段落写入会清空表格连续状态，
  防止跨页表格被页内段落、标题或尾注误连接。
- 已更新跨页表格测试：
  `PDF table import merges compatible table candidates across page boundary` 现在断言
  `paragraph / table / paragraph` 的 body 顺序、`tables_imported == 1`、合并表格为
  6 行 x 3 列，并确认保存重开后不存在第二个表格。
- 已完成跨页输入 PDF 可视化验证：
  `featherdoc-pdf-import-paragraph-table-pagebreak-table-paragraph.pdf` 渲染为 2 页
  PNG/contact sheet；两页均非空，page-01 非空包围盒为 `(143, 118, 865, 449)`，
  page-02 非空包围盒为 `(143, 183, 865, 494)`。
- 已完成导入后 DOCX 的视觉 smoke：
  保留测试生成的 `featherdoc-pdf-import-paragraph-table-pagebreak-table-paragraph.docx`，
  通过 Word 导出为 PDF 并渲染 contact sheet；视觉报告 verdict 为 `pass`，
  1 页非空，合并后的 6 行表格可见。
- 本轮仍没有新增发布导出样本，也没有修改 `pdf_regression_manifest.json`；
  验证范围保持在 PDF import targets、PDFium probe 和导入视觉产物。
- 已知限制更新：
  跨页合并只处理“下一页第一个块是兼容表格候选”的直接延续场景；如果页面顶部有标题、
  页眉页脚文本被解析为段落、列宽不兼容或表格结构变化，当前实现会保守地创建新表格。
- 已知限制更新：
  当前合并只追加普通行，不恢复跨页表头、跨页行拆分、合并单元格、边框连续性或原始分页语义。

本轮继续推进通过命令：

```powershell
cmd /c '"D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests pdf_import_structure_tests pdf_import_failure_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60
$env:FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS='1'; ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60; Remove-Item Env:FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-paragraph-table-pagebreak-table-paragraph.pdf --output-dir .\output\pdf-e7-cross-page-table-import-visual\source-pdf\pages --summary .\output\pdf-e7-cross-page-table-import-visual\source-pdf\summary.json --contact-sheet .\output\pdf-e7-cross-page-table-import-visual\source-pdf\contact-sheet.png --dpi 144
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 -InputDocx .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-paragraph-table-pagebreak-table-paragraph.docx -OutputDir .\output\pdf-e7-cross-page-table-import-visual\merged-docx -ReviewVerdict pass -ReviewNote "PDF import cross-page table candidate merge visual verification"
```

本轮继续推进可视化验证产物：

- `output/pdf-e7-cross-page-table-import-visual/source-pdf/summary.json`
- `output/pdf-e7-cross-page-table-import-visual/source-pdf/contact-sheet.png`
- `output/pdf-e7-cross-page-table-import-visual/merged-docx/table_visual_smoke.pdf`
- `output/pdf-e7-cross-page-table-import-visual/merged-docx/evidence/contact_sheet.png`
- `output/pdf-e7-cross-page-table-import-visual/merged-docx/report/summary.json`
- `output/pdf-e7-cross-page-table-import-visual/merged-docx/report/final_review.md`

2026-05-10 继续推进（跨页表格负样本）：

- 已补齐跨页表格不应合并的受控负样本：
  `write_paragraph_table_pagebreak_wide_table_paragraph_pdf()` 覆盖下一页表格列宽变化；
  `write_paragraph_table_pagebreak_caption_table_paragraph_pdf()` 覆盖下一页表格前出现普通段落；
  `write_paragraph_table_pagebreak_low_table_paragraph_pdf()` 覆盖下一页首块表格明显偏离页顶。
- 已补强 `pdf_import_table_heuristic` 的导入侧断言：
  列宽不兼容时保持 `paragraph / table / table / paragraph`，并保留两个 3x3 `Document` 表格；
  段落介入时保持 `paragraph / table / paragraph / table / paragraph`，不穿过段落合并表格；
  低位表格场景保持 `paragraph / table / table / paragraph`，不把页面中部的新表格续接到上一页。
- 已继续保持导入与导出回归分离：
  新样本只作为 import 测试内部受控 PDF 使用，未加入 `pdf_regression_manifest.json`，
  也未扩展 E6 发布 baseline。
- 已完成新增 PDF 样本可视化验证：
  三个样本均渲染为 2 页 PNG/contact sheet；列宽变化样本 page-01 非空包围盒为
  `(143, 118, 865, 449)`，page-02 为 `(143, 183, 1045, 494)`；段落介入样本
  page-01 为 `(143, 118, 875, 449)`，page-02 为 `(143, 118, 914, 550)`；
  低位表格样本 page-01 为 `(143, 118, 865, 449)`，page-02 为 `(143, 463, 877, 750)`。
- 已完成导入后 DOCX 的视觉 smoke：
  三个保留的 DOCX 均通过 Word 导出 PDF 并渲染 contact sheet，视觉报告 verdict 均为
  `pass`；列宽变化样本和低位表格样本导入后显示两个独立表格，段落介入样本导入后显示
  两个表格之间的普通段落。
- 已知限制更新：
  跨页表格合并仍只在“下一页第一个块是兼容表格候选，且开始位置足够靠近页顶”时触发；
  列宽变化、页面中部开始的新表格、表格前说明段落、标题、页眉页脚文本或结构变化都会
  保守地创建新表格。
- 已知限制更新：
  当前仍不恢复跨页表头、跨页行拆分、合并单元格、边框连续性、原始分页语义或表格标题语义；
  负样本只是锁定保守边界，不代表完整 PDF 表格理解。

本轮继续推进通过命令：

```powershell
cmd /c "call ""D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat"" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_structure_tests pdf_import_failure_tests pdf_import_table_heuristic_tests"
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60
cmake -E env FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS=1 ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-pagebreak-low-table.pdf --output-dir .\output\pdf-e7-cross-page-table-negative-visual\low-table\pages --summary .\output\pdf-e7-cross-page-table-negative-visual\low-table\summary.json --contact-sheet .\output\pdf-e7-cross-page-table-negative-visual\low-table\contact-sheet.png --dpi 144
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 -InputDocx .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-pagebreak-low-table.docx -OutputDir .\output\pdf-e7-cross-page-table-negative-visual\low-table-docx -ReviewVerdict pass -ReviewNote "PDF import low-position cross-page table visual verification"
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-pagebreak-wide-table.pdf --output-dir .\output\pdf-e7-cross-page-table-negative-visual\wide-table\pages --summary .\output\pdf-e7-cross-page-table-negative-visual\wide-table\summary.json --contact-sheet .\output\pdf-e7-cross-page-table-negative-visual\wide-table\contact-sheet.png --dpi 144
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-pagebreak-caption-table.pdf --output-dir .\output\pdf-e7-cross-page-table-negative-visual\caption-table\pages --summary .\output\pdf-e7-cross-page-table-negative-visual\caption-table\summary.json --contact-sheet .\output\pdf-e7-cross-page-table-negative-visual\caption-table\contact-sheet.png --dpi 144
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 -InputDocx .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-pagebreak-wide-table.docx -OutputDir .\output\pdf-e7-cross-page-table-negative-visual\wide-table-docx -ReviewVerdict pass -ReviewNote "PDF import negative cross-page width mismatch visual verification"
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 -InputDocx .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-pagebreak-caption-table.docx -OutputDir .\output\pdf-e7-cross-page-table-negative-visual\caption-table-docx -ReviewVerdict pass -ReviewNote "PDF import negative cross-page intervening paragraph visual verification"
```

本轮继续推进可视化验证产物：

- `output/pdf-e7-cross-page-table-negative-visual/wide-table/summary.json`
- `output/pdf-e7-cross-page-table-negative-visual/wide-table/contact-sheet.png`
- `output/pdf-e7-cross-page-table-negative-visual/caption-table/summary.json`
- `output/pdf-e7-cross-page-table-negative-visual/caption-table/contact-sheet.png`
- `output/pdf-e7-cross-page-table-negative-visual/low-table/summary.json`
- `output/pdf-e7-cross-page-table-negative-visual/low-table/contact-sheet.png`
- `output/pdf-e7-cross-page-table-negative-visual/low-table-docx/table_visual_smoke.pdf`
- `output/pdf-e7-cross-page-table-negative-visual/low-table-docx/evidence/contact_sheet.png`
- `output/pdf-e7-cross-page-table-negative-visual/low-table-docx/report/summary.json`
- `output/pdf-e7-cross-page-table-negative-visual/low-table-docx/report/final_review.md`
- `output/pdf-e7-cross-page-table-negative-visual/wide-table-docx/table_visual_smoke.pdf`
- `output/pdf-e7-cross-page-table-negative-visual/wide-table-docx/evidence/contact_sheet.png`
- `output/pdf-e7-cross-page-table-negative-visual/wide-table-docx/report/summary.json`
- `output/pdf-e7-cross-page-table-negative-visual/wide-table-docx/report/final_review.md`
- `output/pdf-e7-cross-page-table-negative-visual/caption-table-docx/table_visual_smoke.pdf`
- `output/pdf-e7-cross-page-table-negative-visual/caption-table-docx/evidence/contact_sheet.png`
- `output/pdf-e7-cross-page-table-negative-visual/caption-table-docx/report/summary.json`
- `output/pdf-e7-cross-page-table-negative-visual/caption-table-docx/report/final_review.md`

下一步入口：

- 继续 E7 时应先把跨页连续性判断从 importer 内部状态提炼成更明确的候选置信度和
  continuation 诊断，避免更多复杂启发式继续堆叠在写入逻辑里。
- 若继续扩展真实表格 AST，应优先设计 `PdfParsedTableCandidate` 到行列宽、表头、
  合并单元格和分页语义的中间表示，再考虑导入更多复杂样本。

2026-05-10 继续推进（同页表格分隔）：

- 已补齐同页表格之间的段落分隔样本：
  `write_paragraph_table_note_table_paragraph_pdf()` 覆盖“同一页面上的两个表格中间插入普通段落”
  场景，用来验证 importer 会清空表格连续状态，不跨段落黏连表格。
- 已补强 `pdf_import_table_heuristic` 的同页边界断言：
  `PDF table import preserves same-page paragraph separation between table candidates` 现在断言
  `paragraph / table / paragraph / table / paragraph` 的 body 顺序，且保存重开后仍保持两个独立表格。
- 已继续保持导入与导出回归分离：
  新样本只作为 import 测试内部受控 PDF 使用，未加入 `pdf_regression_manifest.json`，
  也未扩展 E6 发布 baseline。
- 已完成新增 PDF 样本可视化验证：
  `featherdoc-pdf-import-paragraph-table-note-table-paragraph.pdf` 渲染为 1 页 PNG/contact sheet；
  page-01 非空包围盒为 `(143, 118, 865, 910)`。
- 已完成导入后 DOCX 的视觉 smoke：
  保留的 `featherdoc-pdf-import-paragraph-table-note-table-paragraph.docx` 通过 Word 导出 PDF 并渲染
  contact sheet，视觉报告 verdict 为 `pass`；导出页显示两个独立表格，中间保留普通段落。
- 已知限制更新：
  当前仍只覆盖规则网格型表格导入；同页或跨页的更复杂表格连续性、标题语义、合并单元格、
  同一 y 坐标左右块的多栏阅读顺序，仍未进入稳定支持范围。

本轮继续推进通过命令：

```powershell
cmd /c "call ""D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat"" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_structure_tests pdf_import_failure_tests pdf_import_table_heuristic_tests"
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60
cmake -E env FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS=1 ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-paragraph-table-note-table-paragraph.pdf --output-dir .\output\pdf-e7-same-page-table-separation-visual\source-pdf\pages --summary .\output\pdf-e7-same-page-table-separation-visual\source-pdf\summary.json --contact-sheet .\output\pdf-e7-same-page-table-separation-visual\source-pdf\contact-sheet.png --dpi 144
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 -InputDocx .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-paragraph-table-note-table-paragraph.docx -OutputDir .\output\pdf-e7-same-page-table-separation-visual\merged-docx -ReviewVerdict pass -ReviewNote "PDF import same-page table separation visual verification"
```

本轮继续推进可视化验证产物：

- `output/pdf-e7-same-page-table-separation-visual/source-pdf/summary.json`
- `output/pdf-e7-same-page-table-separation-visual/source-pdf/contact-sheet.png`
- `output/pdf-e7-same-page-table-separation-visual/merged-docx/table_visual_smoke.pdf`
- `output/pdf-e7-same-page-table-separation-visual/merged-docx/evidence/contact_sheet.png`
- `output/pdf-e7-same-page-table-separation-visual/merged-docx/report/summary.json`
- `output/pdf-e7-same-page-table-separation-visual/merged-docx/report/final_review.md`

2026-05-10 继续推进（两栏 row-wise 顺序）：

- 已新增受控两栏样本 `write_out_of_order_two_column_pdf()`，用右栏先写、左栏后写的文本
  流验证同一页面左右块的块级顺序恢复和表格误判边界。
- 已补充 `pdf_import_structure` 的 parser/importer 回归：
  `PDFium parser preserves two-column row-wise reading order from out-of-order text runs` 与
  `PDF text importer preserves two-column row-wise reading order from out-of-order text runs`，
  断言该页会稳定导入为 3 个段落，文本顺序保持为 `title -> left/right row 1 -> left/right row 2`。
- 已将 PDFium 行聚合补成更保守的顺序恢复：同一行内若出现明显的 X 回退，会开始新行，
  以便后续更复杂的外部 PDF 仍有机会恢复几何顺序。
- 已继续保持导入与导出回归分离：
  该样本只作为 import 测试内部受控 PDF 使用，未加入 `pdf_regression_manifest.json`，
  也未扩展 E6 发布 baseline。
- 已完成新增 PDF 样本可视化验证：
  `featherdoc-pdf-import-two-column-reordered.pdf` 渲染为 1 页 PNG/contact sheet；目检确认
  页面是两栏正文，底部无空白页或裁切，文本与分栏线可见。
- 已完成导入后 DOCX 的视觉 smoke：
  保留的 `featherdoc-pdf-import-two-column-reordered-source.docx` 通过 Word 导出 PDF 并渲染
  contact sheet；视觉报告 verdict 为 `pass`，页面内容完整，左/右栏文本按行保序落入同一页。
- 已知限制更新：
  当前两栏页仍以 row-wise 的合并段落导入，没有真正的 column AST；同一 y 坐标的左右块
  仍是保守的块级文本恢复，而不是列级拆分。
- 已知限制更新：
  当前顺序恢复仍是启发式，遇到更复杂的多栏排版、标题穿插、表格旁注或混排块时，
  仍会优先保守合并而不是强行拆成多个语义块。

本轮继续推进通过命令：

```powershell
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_structure_tests pdf_import_table_heuristic_tests pdf_import_failure_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60
cmake -E env FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS=1 ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_structure$" --output-on-failure --timeout 60
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-two-column-reordered.pdf --output-dir .\output\pdf-e7-two-column-order-visual\source-pdf\pages --summary .\output\pdf-e7-two-column-order-visual\source-pdf\summary.json --contact-sheet .\output\pdf-e7-two-column-order-visual\source-pdf\contact-sheet.png --dpi 144
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 -InputDocx .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-two-column-reordered-source.docx -OutputDir .\output\pdf-e7-two-column-order-visual\merged-docx -ReviewVerdict pass -ReviewNote "PDF import two-column row-wise reading order visual verification"
ctest --test-dir .bpdf-roundtrip-msvc -R "^(pdfium_parser_probe|pdfium_document_parser_probe|pdf_import_structure|pdf_import_failure|pdf_import_table_heuristic)$" --output-on-failure --timeout 60
```

本轮继续推进可视化验证产物：

- `output/pdf-e7-two-column-order-visual/source-pdf/summary.json`
- `output/pdf-e7-two-column-order-visual/source-pdf/contact-sheet.png`
- `output/pdf-e7-two-column-order-visual/merged-docx/table_visual_smoke.pdf`
- `output/pdf-e7-two-column-order-visual/merged-docx/evidence/contact_sheet.png`
- `output/pdf-e7-two-column-order-visual/merged-docx/report/summary.json`
- `output/pdf-e7-two-column-order-visual/merged-docx/report/final_review.md`

2026-05-10 继续推进（表格标题段落顺序）：

- 已新增受控 PDF 样本 `write_table_title_paragraph_pdf()`，用表格标题段落、真实 3x3
  表格和尾段落验证导入侧块级顺序恢复。
- 已新增导入回归 `PDF text importer preserves table title paragraph before imported table`，
  断言该页稳定导入为 `paragraph / table / paragraph`，并确认真实 `Document` 表格
  仍然按 3 列 3 行写回、重开后可重新识别。
- 已把本轮临时的侧注片段切分实验收回，保留此前已验证的表格导入、跨页合并和
  两栏 row-wise 顺序修正，不回退已收口的导出侧工作。
- 已继续保持导入与导出回归分离：
  该样本仅作为 import 测试内部受控 PDF 使用，未加入 `pdf_regression_manifest.json`。
- 已完成新增 PDF 样本可视化验证：
  `featherdoc-pdf-import-table-title.pdf` 渲染为 1 页 PNG/contact sheet；目检确认标题、
  表格和尾段落未重叠，页内布局正常。
- 已完成导入后 DOCX 的视觉 smoke：
  保留的 `featherdoc-pdf-import-table-title.docx` 通过 Word 导出 PDF 并渲染 contact sheet；
  视觉报告 verdict 为 `pass`，表格仍然以真实表格形式落在导出页中。
- 已知限制更新：
  当前导入仍以块级顺序恢复为主，尚未做表格旁注/同段内混排的精细片段切分；
  遇到更复杂的旁注或混排时，仍会优先保守地把整段文本当作普通段落或直接跳过，
  不会强行拆成语义更细的 AST 片段。

本轮继续推进通过命令：

```powershell
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_structure_tests pdf_import_table_heuristic_tests pdf_import_failure_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^(pdfium_parser_probe|pdfium_document_parser_probe|pdf_import_structure|pdf_import_failure|pdf_import_table_heuristic)$" --output-on-failure --timeout 60
cmake -E env FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS=1 ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-table-title.pdf --output-dir .\output\pdf-e7-table-title-order-visual\source-pdf\pages --summary .\output\pdf-e7-table-title-order-visual\source-pdf\summary.json --contact-sheet .\output\pdf-e7-table-title-order-visual\source-pdf\contact-sheet.png --dpi 144
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 -InputDocx .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-table-title.docx -OutputDir .\output\pdf-e7-table-title-order-visual\merged-docx -ReviewVerdict pass -ReviewNote "PDF import table title paragraph visual verification"
```

本轮继续推进可视化验证产物：

- `output/pdf-e7-table-title-order-visual/source-pdf/summary.json`
- `output/pdf-e7-table-title-order-visual/source-pdf/contact-sheet.png`
- `output/pdf-e7-table-title-order-visual/merged-docx/table_visual_smoke.pdf`
- `output/pdf-e7-table-title-order-visual/merged-docx/evidence/contact_sheet.png`
- `output/pdf-e7-table-title-order-visual/merged-docx/report/summary.json`
- `output/pdf-e7-table-title-order-visual/merged-docx/report/final_review.md`

2026-05-11 继续推进（长表重复表头导入）：

- 已把 `PdfParsedTableCandidate` 到真实 `Document` 表格的行高映射从“文本框自身高度”
  调整为“候选表格平均行距与行框高度取最大值”，避免长表导入后行高过小、Word 重新排版后
  把 56 行网格塞进 1 页。
- 已保持表头重复语义的导入：首行仍会写入 `w:tblHeader`，并继续写入行高 `w:trHeight`
  与 `hRule=exact`，使导入后的 DOCX 能在 Word 中按真实分页展示重复表头。
- 已补强导入侧测试：
  `PDF text importer marks repeated header rows on long invoice table` 现在同时断言
  行高 / 行高规则 / 重复表头标记在保存重开后仍稳定存在，并要求首行行高明显高于文本框高度。
- 已完成新增样本的可视化验证：
  源 PDF `featherdoc-pdf-import-invoice-grid-repeat-header.pdf` 渲染为 4 页 PNG/contact sheet；
  导入后 DOCX 通过 Word 导出为 3 页 PDF，contact sheet 显示第 2、3 页都重复了表头，
  表格和尾注可见，无空白页或裁切。
- 已知限制更新：
  当前行高仍是启发式估算，来自候选表格的整体平均行距，不是 PDFium 的逐行原始排版语义；
  对于更不规则的票据表、混排表格、跨页表头变化或多层嵌套表格，仍需要后续专门样本。
- 已知限制更新：
  这条导入语义仍只覆盖规则网格型表格；同页/跨页的复杂合并单元格、标题旁注和列语义恢复，
  仍不在当前稳定支持范围内。

本轮继续推进通过命令：

```powershell
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_structure_tests pdf_import_failure_tests pdf_import_table_heuristic_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60
cmake -E env FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS=1 ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60
python .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-invoice-grid-repeat-header.pdf --output-dir .\output\pdf-e7-repeat-header-visual\source-pdf\pages --summary .\output\pdf-e7-repeat-header-visual\source-pdf\summary.json --contact-sheet .\output\pdf-e7-repeat-header-visual\source-pdf\contact-sheet.png --dpi 144
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 -InputDocx .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-invoice-grid-repeat-header.docx -OutputDir .\output\pdf-e7-repeat-header-visual\merged-docx-heightfix -ReviewVerdict pass -ReviewNote "PDF import repeated header row visual verification after row-height pitch import"
```

本轮继续推进可视化验证产物：

- `output/pdf-e7-repeat-header-visual/source-pdf/summary.json`
- `output/pdf-e7-repeat-header-visual/source-pdf/contact-sheet.png`
- `output/pdf-e7-repeat-header-visual/merged-docx-heightfix/table_visual_smoke.pdf`
- `output/pdf-e7-repeat-header-visual/merged-docx-heightfix/evidence/contact_sheet.png`
- `output/pdf-e7-repeat-header-visual/merged-docx-heightfix/report/summary.json`
- `output/pdf-e7-repeat-header-visual/merged-docx-heightfix/report/final_review.md`

2026-05-11 继续推进（跨页重复表头去重合并）：

- 已新增最小受控跨页样本
  `write_paragraph_table_pagebreak_repeated_header_table_paragraph_pdf()`：
  第 1 页和第 2 页都包含同一个 3 列表头行，用来回归“源 PDF 自带重复 header，
  importer 合并跨页表格时不应把续页 header 当成普通数据行再插入中间”。
- 已补强 `PdfDocumentImporter` 的跨页合并状态：
  `ImportCursor` 现在会在创建真实 `Document` 表格时记住首行 header 的 cell 文本；
  当下一页首个兼容 `PdfParsedTableCandidate` 继续合并时，如果其首行仍满足表头启发式，
  且 cell 文本与前一页 header 精确一致，就跳过该首行，仅追加真实 body 行。
- 已补充导入侧回归
  `PDF table import skips repeated source header rows while merging cross-page repeated-header table`：
  断言导入结果为 `paragraph / table / paragraph`、`tables_imported == 1`、合并后的
  `Document` 表格为 5 行 x 3 列，且第 4 行已经是续页 body，而不是再次出现 `Item` header；
  保存重开后行数、表头标记和边界 cell 文本仍稳定。
- 已继续保持导入与导出回归分离：
  新样本只存在于 `pdf_import_table_heuristic` 内部，未加入 `pdf_regression_manifest.json`。
- 已完成新增 PDF 样本的 PNG/contact sheet 可视化验证：
  `featherdoc-pdf-import-pagebreak-repeated-header-table.pdf` 渲染为 2 页 PNG/contact sheet；
  目检确认两页都显示同一个 header 行，第 2 页保留尾段落，表格与文本未重叠。
- 已知限制更新：
  当前续页 header 去重仍依赖“跨页续表边界已成立 + 首行继续满足表头启发式 +
  cell 文本逐列精确一致”三项条件；如果外部 PDF 把续页表头改写、截断、OCR 扰动、
  合并单元格或轻微列语义漂移，当前实现仍会保守地把该行作为普通数据行导入。
- 已知限制更新：
  本轮只完成了源 PDF 的 PNG/contact sheet 视觉留档；导入后的真实 `Document` 结果
  仍主要依赖 `ctest` 中的保存/重开断言验证，尚未为该新样本保留可复用的 Word smoke
  产物。

本轮继续推进通过命令：

```powershell
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^(pdfio_generator_probe|pdf_document_generator_probe|pdfium_parser_probe|pdfium_document_parser_probe)$" --output-on-failure --timeout 60
python .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-pagebreak-repeated-header-table.pdf --output-dir .\output\pdf-e7-repeated-source-header-merge-visual\source-pdf\pages --summary .\output\pdf-e7-repeated-source-header-merge-visual\source-pdf\summary.json --contact-sheet .\output\pdf-e7-repeated-source-header-merge-visual\source-pdf\contact-sheet.png --dpi 144
```

本轮继续推进可视化验证产物：

- `output/pdf-e7-repeated-source-header-merge-visual/source-pdf/summary.json`
- `output/pdf-e7-repeated-source-header-merge-visual/source-pdf/contact-sheet.png`

2026-05-11 继续推进（长 repeated-header 续表导入回归）：

- 已新增 4 页最小受控样本
  `write_long_repeated_header_table_pdf()`：
  每页都重复同一个 3 列 header 行，总计 52 条 body 数据；用来回归
  “parser 每页都要稳定给出 `PdfParsedTableCandidate`，importer 合并跨页续表时要把
  续页 header 去重，并导入成单个可编辑 `Document` 表格”。
- 已补充 parser 侧回归
  `PDFium parser detects long repeated-header table candidate on every page`：
  断言 4 页都各自产生 1 个表格候选，且每页候选都是 14 行 x 3 列，
  首行都保留 `Item / Owner / Status` header 文本。
- 已补充 importer 侧回归
  `PDF table import merges long repeated-header source tables into one editable table`：
  断言导入结果为 `paragraph / table / paragraph`，文本顺序保持为标题段落、
  单张真实表格、尾段落；合并后表格为 53 行 x 3 列，只保留第 0 行
  `repeats_header == true`，续页边界行 `14 / 27 / 40` 不再插入重复 header，
  且 `Feature14 / Feature27 / Feature40` 这些首条 body 行在保存重开后仍稳定存在。
- 已补强 importer 的重复表头判定启发式：
  `should_mark_repeating_header_row()` 不再只依赖“body 平均文本长度显著更长”，
  还要求首行更像标签型 header（有字母、无数字、长度较短），并结合 body 中
  更长文本单元格的分布做判定；这样同一逻辑同时覆盖了 `repeats_header`
  标记和跨页续表的源 header 去重。
- 已继续保持导入与导出回归分离：
  新长样本和回归仍只存在于 `pdf_import_table_heuristic` 相关测试内，
  未加入 `pdf_regression_manifest.json`。
- 已完成新增 PDF 样本的 PNG/contact sheet 可视化验证：
  `featherdoc-pdf-import-long-repeated-header.pdf` 渲染为 4 页 PNG/contact sheet；
  目检确认第 1 页有标题段落、4 页表格都显示同一个 header 行、第 4 页保留尾段落，
  页面无裁切、无重叠，续页布局稳定。
- 已知限制更新：
  当前 header 判定仍是规则启发式，偏向 `Item / Owner / Status` 这类短标签首行；
  如果外部 PDF 的 header 含数字编号、极短缩写、多语混排、OCR 噪声或明显断裂，
  仍可能无法触发 `repeats_header` 标记。
- 已知限制更新：
  当前跨页 header 去重仍要求“跨页续表边界成立 + 列锚点兼容 + 首行 cell 文本逐列精确一致”；
  如果续页 header 被改写、截断、换行重排、合并单元格或列语义轻微漂移，仍会保守地保留该行。

本轮继续推进通过命令：

```powershell
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_structure_tests pdf_import_failure_tests pdf_import_table_heuristic_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target featherdoc_pdfio_probe featherdoc_pdf_document_probe featherdoc_pdfium_probe'
ctest --test-dir .bpdf-roundtrip-msvc -R "^(pdfio_generator_probe|pdf_document_generator_probe|pdfium_parser_probe|pdfium_document_parser_probe)$" --output-on-failure --timeout 60
python .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-long-repeated-header.pdf --output-dir .\output\pdf-e7-long-repeated-header-visual\source-pdf\pages --summary .\output\pdf-e7-long-repeated-header-visual\source-pdf\summary.json --contact-sheet .\output\pdf-e7-long-repeated-header-visual\source-pdf\contact-sheet.png --dpi 144
```

本轮继续推进可视化验证产物：

- `output/pdf-e7-long-repeated-header-visual/source-pdf/summary.json`
- `output/pdf-e7-long-repeated-header-visual/source-pdf/contact-sheet.png`

2026-05-11 继续推进（近表尾段落误吞边界）：

- 已新增最小受控样本
  `write_paragraph_table_touching_note_paragraph_pdf()`：
  标题段落后接一个 3x3 规则表格，再接一个紧贴表格下沿的 note 段落和尾段落；
  用来回归“paragraph 只是轻微贴近 table candidate 边界时，不应被 importer 当成表内文本吞掉”。
- 已补充 parser 侧回归
  `PDFium parser preserves note line that sits close to table border`：
  断言该页仍保持 6 条 text lines、1 个 table candidate，并保留
  `Note paragraph touching table border` 与尾段落的阅读顺序。
- 已补充 importer 侧回归
  `PDF text importer preserves note paragraph that only lightly overlaps table bounds`：
  断言导入结果为 `paragraph / table / paragraph / paragraph`，
  `collect_document_text(document)` 保持为标题段落、近表 note 段落、尾段落，
  同时真实 `Document` 表格仍是 3 行 x 3 列，保存重开后块级顺序不变。
- 已收紧 importer 的 table-overlap 过滤粒度：
  `overlaps_table_candidate()` 不再使用“paragraph bounds 与整张 table bounds 任意相交”
  作为唯一条件，而是要求 paragraph 的 line/bounds 与候选表格的某一 `row.bounds`
  发生“中心落入行框或重叠面积占比较高”的有效重叠，避免把仅仅贴近表格下沿的普通段落误删。
- 已继续保持导入与导出回归分离：
  新样本和回归仍只存在于 `pdf_import_structure` / `pdf_import_table_heuristic`
  相关测试内，未加入 `pdf_regression_manifest.json`。
- 已完成新增 PDF 样本的 PNG/contact sheet 可视化验证：
  `featherdoc-pdf-import-touching-note.pdf` 渲染为 1 页 PNG/contact sheet；
  目检确认 note 段落紧贴表格下沿但仍位于表格外部，标题、表格、note 和尾段落顺序正确，
  页面无裁切、无重叠。
- 已知限制更新：
  当前 overlap 过滤仍是规则启发式，只能区分“明显属于某一表格行的文本”和“表格外近邻段落”；
  如果旁注本身跨多行、嵌入表格行带、位于单元格内部边缘，或与表格正文共享更复杂的几何关系，
  仍可能需要后续更细的 fragment/region 级语义。
- 已知限制更新：
  本轮只完成了源 PDF 的 PNG/contact sheet 留档；导入后的真实 `Document` 结果
  仍主要依赖 `ctest` 的保存/重开断言验证，尚未为该新样本保留可复用的 Word smoke 产物。

本轮继续推进通过命令：

```powershell
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_structure_tests pdf_import_table_heuristic_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^(pdf_import_structure|pdf_import_table_heuristic)$" --output-on-failure --timeout 60
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_failure_tests featherdoc_pdfio_probe featherdoc_pdf_document_probe featherdoc_pdfium_probe'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^(pdfio_generator_probe|pdf_document_generator_probe|pdfium_parser_probe|pdfium_document_parser_probe)$" --output-on-failure --timeout 60
cmake -E env FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS=1 ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60
python .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-touching-note.pdf --output-dir .\output\pdf-e7-touching-note-visual\source-pdf\pages --summary .\output\pdf-e7-touching-note-visual\source-pdf\summary.json --contact-sheet .\output\pdf-e7-touching-note-visual\source-pdf\contact-sheet.png --dpi 144
```

本轮继续推进可视化验证产物：

- `output/pdf-e7-touching-note-visual/source-pdf/summary.json`
- `output/pdf-e7-touching-note-visual/source-pdf/contact-sheet.png`

2026-05-11 继续推进（混合段落按 line 切分导入）：

- 已新增更苛刻的 1 页受控样本
  `write_paragraph_table_touching_multiline_note_paragraph_pdf()`：
  表格最后一行 `Touching multi-line C3` 之后，紧跟一个两行说明段落；
  第一行故意贴近表格下沿，第二行和尾段落明确落在表格外，用来回归
  “parser 可能把 `C3 + note` 聚成一个 paragraph 时，importer 仍应把表格正文和表外说明拆开导入”。
- 已补充 parser 侧回归
  `PDFium parser groups touching multi-line note outside table as one paragraph`：
  断言新样本仍有 7 条 `text_lines`、1 个 `table_candidate`，并确认两行 note 文本继续落在
  同一个 parser paragraph 内，锁定“问题发生在导入分段而不是解析丢字”。
- 已补充 importer 侧回归
  `PDF text importer preserves touching multi-line note paragraph outside table`：
  断言导入结果为 `paragraph / table / paragraph / paragraph`，
  `collect_document_text(document)` 只包含标题段落、两行 note 段落和尾段落；
  真实 `Document` 表格仍保留 3 行 x 3 列，且 `Touching multi-line C3`
  继续留在表格内，保存重开后块顺序和文本都稳定。
- 已把 importer 的表格过滤从“整段保留/整段丢弃”推进到“按 line 切 paragraph fragment”：
  `build_import_blocks()` 现在会先按 `PdfParsedTextLine` 判断哪些行应由表格候选接管，
  再把剩余的非表格行重新拼成 paragraph fragments；这样 parser 即便把
  `C3 + note` 聚成一个 paragraph，importer 仍能把表格行剥回真实表格，
  并把表外说明保留为可编辑段落。
- 已收紧 paragraph/table overlap 判定的段落语义：
  对多行 paragraph，不再因为“任意一行落进表格行框”就把整段视为表内文本；
  现在要求超过半数的 lines 真正与某个 `row.bounds` 发生有效重叠，才会把该 paragraph
  交给表格候选消费。
- 已继续保持导入与导出回归分离：
  新样本和回归仍只落在 `pdf_import_structure` / `pdf_import_table_heuristic`，
  未加入 `pdf_regression_manifest.json`。
- 已完成新增 PDF 样本的 PNG/contact sheet 可视化验证：
  `featherdoc-pdf-import-touching-multiline-note.pdf` 渲染为 1 页 PNG/contact sheet；
  目检确认 `C3` 与 note 第一行在视觉上接近，但 note 两行和尾段落仍位于表格外，
  页面无裁切、无重叠。
- 已完成导入后 DOCX 的视觉 smoke：
  保留的 `featherdoc-pdf-import-touching-multiline-note.docx` 通过 Word 导出 PDF 并渲染
  contact sheet；视觉报告 verdict 为 `pass`，导出页中 `C3` 仍留在表格内，
  两行 note 与尾段落在表格下方顺序正确。
- 已知限制更新：
  当前 line-fragment 切分仍只在 paragraph/table 二分边界上工作，不会恢复更细的
  cell 内多段文本、侧注锚点、列表层级或真正的 region AST；如果旁注和表格正文在同一行
  里更复杂地交错，仍需要后续 fragment/region 级中间表示。
- 已知限制更新：
  当前 overlap 规则仍以几何启发式为主；当 OCR 噪声、旋转文本、异常行高或跨行批注
  让 line bounds 明显漂移时，仍可能保守地把文本留在段落侧，而不是强行并回表格。

本轮继续推进通过命令：

```powershell
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_structure_tests pdf_import_table_heuristic_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^(pdf_import_structure|pdf_import_table_heuristic)$" --output-on-failure --timeout 60
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_failure_tests featherdoc_pdfio_probe featherdoc_pdf_document_probe featherdoc_pdfium_probe'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^(pdfio_generator_probe|pdf_document_generator_probe|pdfium_parser_probe|pdfium_document_parser_probe)$" --output-on-failure --timeout 60
cmake -E env FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS=1 ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60
python .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-touching-multiline-note.pdf --output-dir .\output\pdf-e7-touching-multiline-note-visual\source-pdf\pages --summary .\output\pdf-e7-touching-multiline-note-visual\source-pdf\summary.json --contact-sheet .\output\pdf-e7-touching-multiline-note-visual\source-pdf\contact-sheet.png --dpi 144
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 -InputDocx .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-touching-multiline-note.docx -OutputDir .\output\pdf-e7-touching-multiline-note-visual\merged-docx -ReviewVerdict pass -ReviewNote "PDF import touching multi-line note paragraph visual verification"
```

本轮继续推进可视化验证产物：

- `output/pdf-e7-touching-multiline-note-visual/source-pdf/summary.json`
- `output/pdf-e7-touching-multiline-note-visual/source-pdf/contact-sheet.png`
- `output/pdf-e7-touching-multiline-note-visual/merged-docx/table_visual_smoke.pdf`
- `output/pdf-e7-touching-multiline-note-visual/merged-docx/evidence/contact_sheet.png`
- `output/pdf-e7-touching-multiline-note-visual/merged-docx/report/summary.json`
- `output/pdf-e7-touching-multiline-note-visual/merged-docx/report/final_review.md`

2026-05-11 继续推进（续页表头空白变形去重）：

- 已新增 2 页受控样本
  `write_paragraph_table_pagebreak_repeated_header_whitespace_variant_pdf()`：
  第 1 页 header 为 `Owner Name / Project Status`，第 2 页续表 header 改成
  `Owner  Name / Project   Status`，其余列语义不变；用来回归
  “续页表头只出现空白数量差异时，cross-page merge 仍应识别为同一 header 并跳过”。
- 已补充 importer 侧回归
  `PDF table import skips whitespace-varied repeated source header rows while merging cross-page repeated-header table`：
  断言导入结果仍为 `paragraph / table / paragraph`，只导入 1 张 5 行 x 3 列表格，
  续页首行已经是 `Invoice merge pass` 而不是再次插入 header；保存重开后行数和表头标记稳定。
- 已补强续页 header 匹配逻辑：
  `row_cell_texts_match()` 现在会先对每个 cell 文本做空白归一化，再逐列比较；
  连续空格、换行、制表等 whitespace 差异不再阻断 repeated-header 去重。
- 已继续保持导入与导出回归分离：
  新样本只存在于 `pdf_import_table_heuristic` 内部，未加入 `pdf_regression_manifest.json`。
- 已完成新增 PDF 样本的 PNG/contact sheet 可视化验证：
  `featherdoc-pdf-import-pagebreak-repeated-header-whitespace-variant.pdf` 渲染为 2 页 PNG/contact sheet；
  目检确认两页都显示同一套 3 列 header，第 2 页保留尾段落，页面无裁切和重叠。
- 已完成导入后 DOCX 的视觉 smoke：
  保留的 `featherdoc-pdf-import-pagebreak-repeated-header-whitespace-variant.docx`
  通过 Word 导出 PDF 并渲染 contact sheet；视觉报告 verdict 为 `pass`，
  导出页中只保留一行 header，后续 4 条 body 数据顺序正确。
- 已知限制更新：
  当前 repeated-header 匹配只做 whitespace 归一化，尚未处理大小写漂移、OCR 近似字符、
  标点细小差异、同义缩写、列标题改写或轻微词序变化；这些情况仍会保守地保留续页 header 行。
- 已知限制更新：
  当前 header 语义判断仍依赖规则启发式；若首行标签过长、带编号或页间表头结构变化明显，
  仍可能无法触发 `repeats_header` 或 continuation dedupe。

本轮继续推进通过命令：

```powershell
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^(pdfio_generator_probe|pdf_document_generator_probe|pdfium_parser_probe|pdfium_document_parser_probe)$" --output-on-failure --timeout 60
cmake -E env FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS=1 ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60
python .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-pagebreak-repeated-header-whitespace-variant.pdf --output-dir .\output\pdf-e7-repeated-header-whitespace-variant-visual\source-pdf\pages --summary .\output\pdf-e7-repeated-header-whitespace-variant-visual\source-pdf\summary.json --contact-sheet .\output\pdf-e7-repeated-header-whitespace-variant-visual\source-pdf\contact-sheet.png --dpi 144
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 -InputDocx .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-pagebreak-repeated-header-whitespace-variant.docx -OutputDir .\output\pdf-e7-repeated-header-whitespace-variant-visual\merged-docx -ReviewVerdict pass -ReviewNote "PDF import repeated header whitespace variant visual verification"
```

本轮继续推进可视化验证产物：

- `output/pdf-e7-repeated-header-whitespace-variant-visual/source-pdf/summary.json`
- `output/pdf-e7-repeated-header-whitespace-variant-visual/source-pdf/contact-sheet.png`
- `output/pdf-e7-repeated-header-whitespace-variant-visual/merged-docx/table_visual_smoke.pdf`
- `output/pdf-e7-repeated-header-whitespace-variant-visual/merged-docx/evidence/contact_sheet.png`
- `output/pdf-e7-repeated-header-whitespace-variant-visual/merged-docx/report/summary.json`
- `output/pdf-e7-repeated-header-whitespace-variant-visual/merged-docx/report/final_review.md`

2026-05-11 继续推进（续页表头轻微文本变形去重）：

- 已新增 2 页受控样本
  `write_paragraph_table_pagebreak_repeated_header_text_variant_pdf()`：
  第 1 页 header 为 `Item / Owner Name / Project Status`，第 2 页续表 header 改成
  `item / owner-name / project/status`，也就是只改变大小写和常见分隔符，不改词本身；
  用来回归 “续页表头轻微文本变形但语义不变时，cross-page merge 仍应去重”。
- 已补充 importer 侧回归
  `PDF table import skips case and separator-varied repeated source header rows while merging cross-page repeated-header table`：
  断言导入结果仍为 `paragraph / table / paragraph`，只导入 1 张 5 行 x 3 列表格，
  第 4 行和第 5 行已经是续页 body，而不是再次出现 header；保存重开后行数和表头标记稳定。
- 已扩展 header 文本归一化：
  `normalize_header_match_text()` 现在除了折叠 whitespace，还会统一大小写，并把
  `- _ / \\ : .` 这些常见分隔符折叠为单个空格；这样 `owner-name`、`Owner Name`
  和 `project/status`、`Project Status` 会落到同一保守 token 形式再比较。
- 已继续保持导入与导出回归分离：
  新样本只存在于 `pdf_import_table_heuristic` 内部，未加入 `pdf_regression_manifest.json`。
- 已完成新增 PDF 样本的 PNG/contact sheet 可视化验证：
  `featherdoc-pdf-import-pagebreak-repeated-header-text-variant.pdf` 渲染为 2 页 PNG/contact sheet；
  目检确认第 2 页 header 仍是同一列语义，只是大小写和分隔符变形，尾段落保留正常。
- 已完成导入后 DOCX 的视觉 smoke：
  保留的 `featherdoc-pdf-import-pagebreak-repeated-header-text-variant.docx`
  通过 Word 导出 PDF 并渲染 contact sheet；视觉报告 verdict 为 `pass`，
  导出页中只保留一行 header，后续 4 条 body 数据顺序正确。
- 已知限制更新：
  当前 repeated-header 匹配已覆盖 whitespace、大小写和常见分隔符差异，但仍不处理
  OCR 近似字符、同义缩写、列标题改写、词序变化或真正的模糊文本相似度；这些情况仍会保守地保留续页 header 行。
- 已知限制更新：
  当前 token 归一化仍假设 header 的词边界大体稳定；如果外部 PDF 把一个 header
  压成异常连写、缺字、错字或合并单元格后的重排文本，当前实现仍不会强行判定为同一个续页 header。

本轮继续推进通过命令：

```powershell
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^(pdfio_generator_probe|pdf_document_generator_probe|pdfium_parser_probe|pdfium_document_parser_probe)$" --output-on-failure --timeout 60
cmake -E env FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS=1 ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60
python .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-pagebreak-repeated-header-text-variant.pdf --output-dir .\output\pdf-e7-repeated-header-text-variant-visual\source-pdf\pages --summary .\output\pdf-e7-repeated-header-text-variant-visual\source-pdf\summary.json --contact-sheet .\output\pdf-e7-repeated-header-text-variant-visual\source-pdf\contact-sheet.png --dpi 144
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 -InputDocx .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-pagebreak-repeated-header-text-variant.docx -OutputDir .\output\pdf-e7-repeated-header-text-variant-visual\merged-docx -ReviewVerdict pass -ReviewNote "PDF import repeated header text variant visual verification"
```

本轮继续推进可视化验证产物：

- `output/pdf-e7-repeated-header-text-variant-visual/source-pdf/summary.json`
- `output/pdf-e7-repeated-header-text-variant-visual/source-pdf/contact-sheet.png`
- `output/pdf-e7-repeated-header-text-variant-visual/merged-docx/table_visual_smoke.pdf`
- `output/pdf-e7-repeated-header-text-variant-visual/merged-docx/evidence/contact_sheet.png`
- `output/pdf-e7-repeated-header-text-variant-visual/merged-docx/report/summary.json`
- `output/pdf-e7-repeated-header-text-variant-visual/merged-docx/report/final_review.md`

2026-05-11 继续推进（续页表头 plural 变体与语义分裂）：

- 已新增 2 页受控样本
  `write_paragraph_table_pagebreak_repeated_header_plural_variant_pdf()`：
  第 1 页 header 为 `Item / Owner Name / Project Status`，第 2 页续表 header 改成
  `Items / Owner Names / Project Statuses`，用于回归仅在词尾 `s` / `es`
  变化时仍应识别为同一续表并跳过重复 header。
- 已新增 2 页受控负样本
  `write_paragraph_table_pagebreak_repeated_header_semantic_variant_pdf()`：
  第 2 页 header 改成 `Item / Owner Team / Project State`，用于确认语义变化时
  importer 不再把第二页当作同一 repeated-header 续表直接吞并。
- 已补充 importer 侧回归：
  `PDF table import skips plural-varied repeated source header rows while merging cross-page repeated-header table`
  断言 plural 变体仍能导入为 1 张 `5x3` 表，保存重开后行数和表头标记稳定。
  `PDF table import keeps semantic header variants as separate cross-page tables`
  断言语义变体会落成 2 张独立表格，且第二张表保留自己的 header row。
- 已补强 `row_cell_texts_match()`：
  现在除了 whitespace / 大小写 / 常见分隔符归一化，还接受 cell 级别的
  末尾 `s` / `es` 复数变体，再配合跨页 header mismatch 诊断避免把语义不同的
  repeated-header 续表误合并。
- 已继续保持导入与导出回归分离：
  新样本只存在于 `pdf_import_table_heuristic` 内部，未加入 `pdf_regression_manifest.json`。
- 已完成新增 PDF 样本的 PNG/contact sheet 可视化验证：
  plural / semantic 两个 PDF 都渲染为 2 页 PNG/contact sheet；目检确认 plural 变体
  第 2 页 header 仅有词尾复数差异且导入后应保持单表，semantic 变体第 2 页 header
  改写后导入应拆成第二张表，页面无裁切和重叠。
- 已完成导入后 DOCX 的视觉 smoke：
  保留的 `featherdoc-pdf-import-pagebreak-repeated-header-plural-variant.docx`
  与 `featherdoc-pdf-import-pagebreak-repeated-header-semantic-variant.docx`
  均通过 Word 导出 PDF 并渲染 contact sheet；两个 visual report verdict 都为 `pass`。
- 已知限制更新：
  目前 repeated-header 匹配只覆盖 whitespace、大小写、常见分隔符，以及 cell 末尾
  `s` / `es` 的保守复数变体；仍不处理 OCR 近似字符、同义改写、词序变化、
  irregular plural、标题语义重写或真正的模糊文本相似度。
- 已知限制更新：
  如果外部 PDF 把 header 压成异常连写、缺字、错字或合并单元格后的重排文本，
  当前实现仍不会强行判定为同一个续页 header；这类场景仍会保守分裂或保留 header。

本轮继续推进通过命令：

```powershell
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests pdf_import_structure_tests pdf_import_failure_tests'
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^(pdfio_generator_probe|pdf_document_generator_probe|pdfium_parser_probe|pdfium_document_parser_probe)$" --output-on-failure --timeout 60
cmake -E env FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS=1 ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60
& .\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-pagebreak-repeated-header-plural-variant.pdf --output-dir .\output\pdf-e7-repeated-header-plural-variant-visual\source-pdf\pages --summary .\output\pdf-e7-repeated-header-plural-variant-visual\source-pdf\summary.json --contact-sheet .\output\pdf-e7-repeated-header-plural-variant-visual\source-pdf\contact-sheet.png --dpi 144
& .\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-pagebreak-repeated-header-semantic-variant.pdf --output-dir .\output\pdf-e7-repeated-header-semantic-variant-visual\source-pdf\pages --summary .\output\pdf-e7-repeated-header-semantic-variant-visual\source-pdf\summary.json --contact-sheet .\output\pdf-e7-repeated-header-semantic-variant-visual\source-pdf\contact-sheet.png --dpi 144
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 -InputDocx .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-pagebreak-repeated-header-plural-variant.docx -OutputDir .\output\pdf-e7-repeated-header-plural-variant-visual\merged-docx -ReviewVerdict pass -ReviewNote "PDF import repeated header plural variant visual verification"
powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 -InputDocx .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-import-pagebreak-repeated-header-semantic-variant.docx -OutputDir .\output\pdf-e7-repeated-header-semantic-variant-visual\merged-docx -ReviewVerdict pass -ReviewNote "PDF import repeated header semantic variant visual verification"
```

本轮继续推进可视化验证产物：

- `output/pdf-e7-repeated-header-plural-variant-visual/source-pdf/summary.json`
- `output/pdf-e7-repeated-header-plural-variant-visual/source-pdf/contact-sheet.png`
- `output/pdf-e7-repeated-header-plural-variant-visual/merged-docx/table_visual_smoke.pdf`
- `output/pdf-e7-repeated-header-plural-variant-visual/merged-docx/evidence/contact_sheet.png`
- `output/pdf-e7-repeated-header-plural-variant-visual/merged-docx/report/summary.json`
- `output/pdf-e7-repeated-header-plural-variant-visual/merged-docx/report/review_result.json`
- `output/pdf-e7-repeated-header-plural-variant-visual/merged-docx/report/final_review.md`
- `output/pdf-e7-repeated-header-semantic-variant-visual/source-pdf/summary.json`
- `output/pdf-e7-repeated-header-semantic-variant-visual/source-pdf/contact-sheet.png`
- `output/pdf-e7-repeated-header-semantic-variant-visual/merged-docx/table_visual_smoke.pdf`
- `output/pdf-e7-repeated-header-semantic-variant-visual/merged-docx/evidence/contact_sheet.png`
- `output/pdf-e7-repeated-header-semantic-variant-visual/merged-docx/report/summary.json`
- `output/pdf-e7-repeated-header-semantic-variant-visual/merged-docx/report/review_result.json`
- `output/pdf-e7-repeated-header-semantic-variant-visual/merged-docx/report/final_review.md`

2026-05-11 继续推进（跨页续接诊断外提）：

- 已将跨页表格续接判断收拢成显式诊断对象
  `PdfTableContinuationDiagnostic`，并把每个表格候选的 page/block 索引、续接分支、
  阻断原因、`source_row_offset`、`skipped_repeating_header` 和
  `continuation_confidence` 写回 `PdfDocumentImportResult`，避免继续依赖 importer
  内部隐式状态。
- 已把 importer 里的跨页决策整理为单一 decision helper：
  现在先评估是否存在可续接的前置表格，再按 page 顶部位置、行宽一致性、
  列锚点兼容性、重复表头匹配等条件给出明确 blocker，而不是散落在写入路径里。
- 已补充导入侧诊断断言：
  单表样本会记录 `no_previous_table`；跨页可合并样本会记录
  `merged_with_previous_table`；列宽/列锚点不兼容、页面过低、同页第二张表，
  以及重复表头语义不匹配都会留下对应 blocker。
- 已继续保持导入与导出回归分离：
  这轮没有新增 `pdf_regression_manifest.json` 样本，也没有改动导出侧收口逻辑。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests pdf_import_structure_tests pdf_import_failure_tests`
  与 `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  均通过；PDF 相关 probe 组也在 `--timeout 60` 下通过。
- 已知限制更新：
  `continuation_confidence` 仍是规则型评分，只用于解释候选续接强弱，不代表真正的概率模型；
  它主要服务于诊断和回归，不处理合并单元格、列语义恢复或更复杂的版面推断。
- 已知限制更新：
  当前诊断仍只覆盖表格候选的续接路径；如果后续要把更多 `PdfParsedTableCandidate`
  提升为可编辑 AST，还需要单独设计 cell 合并、表头语义和分页语义的中间表示。

2026-05-12 继续推进（跨页 repeated-header 缩写归一化）：

- 已把 repeated-header 页间匹配从“大小写/分隔符/复数”继续推进到少量明确缩写：
  `Qty`/`Quantity`、`Amt`/`Amount`、`Desc`/`Description`、`No`/`Num`/`Number`
  会在表头匹配时映射到同一个规范词。
- 已保持保守边界：
  归一化只发生在已经满足跨页续表、列锚点兼容、两侧都像 repeated header 的路径里；
  不引入模糊文本相似度，也不把 `Owner Team` / `Owner Name` 这类语义变化合并。
- 已补导入侧回归：
  新样本第 1 页表头为 `Item / Quantity / Amount`，第 2 页重复表头为
  `Item / Qty / Amt`；导入后仍应合并成单个可编辑表格，并跳过第 2 页重复表头行。
- 已知限制更新：
  缩写映射是固定白名单，只覆盖低歧义英文表头；OCR 错字、词序变化、同义改写、
  多语表头和复杂多层表头仍保持保守，不会强行合并。

2026-05-12 继续推进（跨页 repeated-header 词序归一化）：

- 已把 repeated-header 页间匹配继续推进到“同一 cell 内 token 集合完全一致”的
  词序变化：
  例如 `Project Status` 与 `Status Project` 会被视为同一个表头单元格。
- 已保持保守边界：
  该规则只在 canonical token 数量为 2 到 4 个且两侧 token 多集合完全一致时触发；
  它不会处理缺字、错字、同义改写、跨 cell 重排，也不会放宽列锚点或跨页续表条件。
- 已补导入侧回归：
  新样本第 1 页表头为 `Item / Owner Name / Project Status`，第 2 页重复表头为
  `Item / Name Owner / Status Project`；导入后仍应合并成单个可编辑表格，并跳过
  第 2 页重复表头行。
- 已知限制更新：
  词序归一化不是语义相似度模型，只覆盖 token 完全一致的受控场景；多语混排、
  OCR 扰动、近义词替换和复杂多层表头仍保持保守。

2026-05-12 继续推进（跨页 repeated-header 单位标点归一化）：

- 已把 repeated-header 表头文本归一化继续扩展到逗号、分号、括号和方括号：
  `Owner, Name` 会折叠为 `Owner Name`，`Amount (USD)` 会折叠为 `Amount USD`。
- 已保持保守边界：
  这只是 separator 级别的文本折叠，不引入 OCR 近似字符、不处理单位换算，也不放宽
  repeated-header 的列锚点、页顶位置和跨页续表判断。
- 已补导入侧回归：
  新样本第 1 页表头为 `Item / Owner Name / Amount USD`，第 2 页重复表头为
  `Item / Owner, Name / Amount (USD)`；导入后仍应合并成单个可编辑表格，并跳过
  第 2 页重复表头行。
- 已知限制更新：
  该规则只覆盖分隔符引起的同 token 变体；`USD Amount`、币种缺失、单位缩写误识别、
  OCR 错字和跨 cell 单位拆分仍保持保守。

2026-05-12 继续推进（跨页 repeated-header 匹配诊断外提）：

- 已把 repeated-header 匹配命中路径写入公开诊断：
  `PdfTableContinuationDiagnostic::header_match_kind` 现在会区分
  `exact`、`normalized_text`、`plural_variant`、`canonical_text` 和 `token_set`，
  便于回归和调用方解释为什么第二页重复表头被跳过。
- 已保持行为边界不变：
  诊断只记录既有合并判断的命中原因，不新增模糊匹配，也不放宽列锚点、页顶位置、
  repeated-header 判定或语义不匹配时的拆表策略。
- 已补导入侧回归断言：
  exact、separator/case 归一化、复数、缩写 canonical、词序 token-set 和语义不匹配
  都会检查对应 `header_match_kind`，避免后续规则调整时变成不可追踪的布尔值。
- 已知限制更新：
  `header_match_kind` 只解释 repeated-header 文本匹配路径；它不是版面置信度，
  也不覆盖复杂表头结构、跨 cell 语义重排或 OCR 近似字符。

2026-05-11 继续推进（宽文本合并落表）：

- 已给 `PdfParsedTableCell` 补入最小 `column_span` 语义，并让 `PdfiumParser`
  对明显宽于单列的文本簇推断横向合并范围，继续保持保守，只覆盖明显宽文本的
  受控样本，不尝试从线条版式反推任意复杂表格。
- 已让 `PdfDocumentImporter` 在写入真实 `Document` 表格时，基于 `column_span`
  调用 `Table::merge_right()`，把解析结果从“普通网格文本”推进成可编辑表格。
- 已补导入侧回归：
  新样本覆盖宽标题跨两列、普通数据行、正文前后段落，以及保存重开后的
  `inspect_table_cell_by_grid_column()` / `column_span` 断言；导入侧测试与导出回归
  仍然分离，未改 `pdf_regression_manifest.json`。
- 已完成视觉验证：
  新样本 PDF 渲染出 `output/pdf-e7-merged-table-visual/source-pdf/contact-sheet.png`，
  导入后的 DOCX 通过 Word smoke 生成
  `output/pdf-e7-merged-table-docx-visual/evidence/contact_sheet.png` 和
  `output/pdf-e7-merged-table-docx-visual/table_visual_smoke.pdf`；目检通过，版面未见
  裁剪、重叠或顺序漂移。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_structure_tests pdf_import_failure_tests pdf_import_table_heuristic_tests`
  与 `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  均通过；PDFium probe 组也通过。
- 已知限制更新：
  这版横向合并仍是启发式，只覆盖明显宽于单列的文本簇；对真实多行表头、
  纵向合并、复杂单元格布局和不规则票据表单，当前仍不会自动恢复。

2026-05-11 继续推进（合并表头跨页续接）：

- 已把跨页重复表头这一条链路继续向前推了一步：
  现在 `PdfDocumentImporter` 会对带横向合并的表头行放宽 repeated-header 判定，
  让首行 `column_span > 1` 的候选也能稳定进入“可续接表头”的判断路径。
- 已补最小受控回归样本：
  新增的页间表头样本覆盖“首行横向合并 + 第二页重复同一合并表头 + 正文继续”
  这三个关键点，用来验证解析结果能稳定提升为真实 `Document` 表格。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --parallel 8 --target pdf_import_structure_tests pdf_import_failure_tests pdf_import_table_heuristic_tests featherdoc_pdfio_probe featherdoc_pdf_document_probe featherdoc_pdfium_probe`
  与
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^(pdfio_generator_probe|pdf_document_generator_probe|pdfium_parser_probe|pdfium_document_parser_probe|pdf_import_structure|pdf_import_failure|pdf_import_table_heuristic)$" --output-on-failure --timeout 60`
  均通过。
- 已完成视觉验证：
  源 PDF contact sheet 为
  `output/pdf-e7-merged-repeated-header-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-merged-repeated-header-docx-visual/evidence/contact_sheet.png`
  与 `output/pdf-e7-merged-repeated-header-docx-visual/table_visual_smoke.pdf`；
  目检未见裁剪、重叠或页序漂移。
- 已知限制更新：
  这条 repeated-header 续接仍然是规则型启发式，当前只覆盖横向合并的表头；
  纵向合并、复杂多层表头、任意不规则表单网格，以及基于矢量线条的精确网格恢复，
  仍未进入本阶段的实现范围。

2026-05-11 继续推进（近表边界混合段落收紧整段保留）：

- 本轮开始前已重新阅读 `include/featherdoc/pdf/pdf_interfaces.hpp`、
  `src/pdf/pdfium_parser.cpp`、`src/pdf/pdf_document_importer.cpp`、
  `test/pdf_import_structure_tests.cpp` 和本文 E7 段落，继续把重点放在
  `PDFium -> 可编辑 Document` 导入，而不回到导出侧已收口工作。
- 已修正受控样本
  `write_paragraph_table_partial_overlap_multiline_note_paragraph_pdf()`：
  把第一行 note 下移到仍然轻微压表格下沿、但不再和 `Partial overlap C3`
  混成同一 `text_line` 的位置；parser 侧 `PDFium parser keeps partial-overlap multi-line note as one paragraph`
  现在稳定回到 `7` 条 `text_lines` 和 `1` 个 `table_candidate`。
- 已收紧 `PdfDocumentImporter::build_paragraph_fragments()` 的“整段保留”条件：
  只有当一个 parser paragraph 内 **没有任何** `line_overlaps_table_candidate()`
  命中的行时，才整段保留；一旦段内存在明确落在表格里的行，就回退到既有的
  line-level 过滤，把表格行和表外说明重新拆开导入。
- 已继续下沉 parser 级修复：
  `PdfiumParser` 现在会先检测 `table_candidates`，再按“是否落在同一张表里”
  切分 `PdfParsedParagraph`；同时给 `PdfParsedTextLine` / `PdfParsedParagraph`
  增加了显式 `table_candidate_index` 标注，让表格尾行和紧邻说明段在中间结构层就先分开；
  importer 仍保留 line-level 过滤作为兜底，但不再是唯一修复点。
- 已补强并跑通导入侧回归：
  `PDF text importer preserves touching multi-line note paragraph outside table`
  和
  `PDF text importer preserves partial-overlap multi-line note paragraph as one block`
  现在都能稳定保持 `paragraph / table / paragraph / paragraph`，
  且 `Touching multi-line C3` / `Partial overlap C3` 继续留在真实 `Document`
  表格里，不再泄漏到正文段落。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_structure_tests pdf_import_table_heuristic_tests pdf_import_failure_tests`
  与
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^(pdf_import_structure|pdf_import_failure|pdf_import_table_heuristic)$" --output-on-failure --timeout 60`
  均通过；同时用
  `cmake -E env FEATHERDOC_KEEP_PDF_IMPORT_TEST_OUTPUTS=1 ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  保留了这轮 import 回归产物。
- 已完成样本变更后的 PNG/contact sheet 可视化验证：
  `output/pdf-e7-partial-overlap-multiline-note-visual/source-pdf/contact-sheet.png`
  与
  `output/pdf-e7-partial-overlap-multiline-note-visual/merged-docx/evidence/contact_sheet.png`
  已目检通过；源 PDF 中第一行 note 仍然按设计轻压表格边界，导入后的 DOCX
  中表格和两行 note 已正确分离，未见裁剪、重叠或块顺序漂移。
  为避免这次 importer 收紧把相邻边界样本打坏，也补留了
  `output/pdf-e7-touching-multiline-note-visual/source-pdf/contact-sheet.png`
  和
  `output/pdf-e7-touching-multiline-note-visual/merged-docx/evidence/contact_sheet.png`，
  目检同样通过。
- 本轮 parser-aware 修复后，又补跑并保留了新的 Word smoke 产物：
  `output/pdf-e7-touching-multiline-note-parserfix-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-partial-overlap-multiline-note-parserfix-visual/merged-docx/evidence/contact_sheet.png`；
  目检结果与预期一致，note 段和表格块都保持分离，未引入新裁剪或顺序漂移。
- 在显式 `table_candidate_index` 标注接入后，又按当前构建重新保留了
  `output/pdf-e7-touching-multiline-note-parseraware-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-partial-overlap-multiline-note-parseraware-visual/merged-docx/evidence/contact_sheet.png`；
  目检结果与前一轮一致，说明 parser 语义显式化没有引入新的导入版面回退。
- 本轮还把 parser 语义断言补上了：
  结构测试直接检查 note 段与表格行的 `table_candidate_index` 是否一致，避免以后只靠
  文本内容相似度判断“看起来没丢字”。
- 本轮继续保持导入与导出回归分离：
  只改了 importer 和测试内部受控样本，未改 `pdf_regression_manifest.json`，
  也没有回退既有导出修正。
- 已知限制更新：
  当前 parser 仍是基于几何和 table candidate 的启发式切分，不是完整语义 AST；
  `table_candidate_index` 只是把现有启发式显式化了，还没有补齐纵向合并、多层表头、
  扫描版或更复杂版面的真正 block 中间表示。

2026-05-11：

- 已把 `PdfParsedPage` 的导入中间层再推进一层：新增 `PdfParsedContentBlock`，
  由 parser 显式产出 paragraph / table_candidate 的统一块序列，避免 importer 再
  自己拼接页内块顺序。
- 已让 `PdfDocumentImporter` 优先消费 `content_blocks`，把块级顺序恢复从“导入侧排序
  规则”前移到“解析结果的一部分”，并保留旧路径作为空块序列时的兼容回退。
- 已补充结构测试，锁定 out-of-order 段落 / 表格 / 段落样本的块序列为
  `paragraph / table_candidate / paragraph`。
- 已重新构建并通过相关导入侧测试：
  `pdf_import_structure`、`pdf_import_failure`、`pdf_import_table_heuristic`。
- 已把 `content_blocks` 的断言补到双表同页、跨页宽度不兼容和表格标题样本里，
  让新中间层在单表、双表、跨页和标题/尾注场景都被显式回归覆盖。
- 本轮没有新增或修改 PDF 样本，因此没有补做新的 PNG/contact sheet 视觉产物；
  现有导入样本与导出回归仍保持分离。
- 已知限制更新：
  `content_blocks` 仍然只是对现有几何启发式的统一线性化，不代表完整的 PDF 内容流
  还原；多栏、旋转文本、扫描件 OCR、浮动对象和更复杂的表格语义仍未覆盖。

2026-05-11 继续推进（最小纵向合并单元格落表）：

- 已在 `PdfiumParser` 里补上最小 `row_span` 推断，只覆盖首列、首行、两行
  的受控纵向合并样本，避免把普通空白误判成通用合并单元格。
- 已让 `PdfDocumentImporter` 改为先写入文本和横向合并，再统一处理纵向
  `merge_down()`，避免续写单元格和已合并单元格互相冲突。
- 已补导入侧回归，覆盖 `featherdoc-pdf-import-vertical-merged-table.pdf` /
  `featherdoc-pdf-import-vertical-merged-table.docx` 的解析、保存和重开断言。
- 已完成视觉验证：
  `output/pdf-e7-vertical-merged-table-visual/source-pdf/contact-sheet.png`、
  `output/pdf-e7-vertical-merged-table-docx-visual/evidence/contact_sheet.png`
  和 `output/pdf-e7-vertical-merged-table-docx-visual/table_visual_smoke.pdf`
  均已生成并目检通过。
- 已完成构建与测试验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  通过；前置构建和相关导入测试也已保持通过。
- 已知限制更新：
  这版只覆盖“最小纵向合并”启发式，不尝试恢复任意多行、多列、多层表头的
  通用 row-span 语义。

2026-05-11 继续推进（纵向合并扩展到任意列与多行）：

- 已把 `PdfiumParser::detect_row_span()` 从首行首列特例扩展为任意列、
  连续多行的受控推断：只要锚点单元格有文本、所在行与后续行都保持足够
  文本密度、且同列在后续行保持空白，就会累计 `row_span`。
- 已补新的中间列三行合并样本：
  `featherdoc-pdf-import-middle-column-merged-table-source.pdf` 与
  `featherdoc-pdf-import-middle-column-merged-table.pdf`，
  用来验证 parser / importer 不再局限于首列。
- 已补导入侧回归，覆盖中间列 `row_span = 3` 的解析、保存和重开断言，
  以防以后把纵向合并又收缩回首列特例。
- 已完成视觉验证：
  `output/pdf-e7-middle-column-merged-table-visual/source-pdf/contact-sheet.png`、
  `output/pdf-e7-middle-column-merged-table-docx-visual/evidence/contact_sheet.png`
  和 `output/pdf-e7-middle-column-merged-table-docx-visual/table_visual_smoke.pdf`
  已生成并目检通过。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --parallel 8 --target pdf_import_structure_tests pdf_import_failure_tests pdf_import_table_heuristic_tests`
  与
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  均通过。
- 已知限制更新：
  这仍是规则型启发式；它只覆盖规则网格里连续空白列的纵向合并，
  还不处理更复杂的多层表头、非规则空白、扫描版表格或矢量线条缺失的
  任意版面恢复。

2026-05-11 继续推进（组合合并）：

- 已补齐左上角 2x2 组合合并样本：
  `featherdoc-pdf-import-merged-corner-table-source.pdf` 与
  `featherdoc-pdf-import-merged-corner-table.pdf`，用于回归同一锚点同时
  `merge_right()` + `merge_down()` 的导入场景。
- 已在 `PdfDocumentImporter` 里补强 row-span 的横向补齐：
  真正执行 `merge_down()` 前，会先把后续行对应的空单元格按锚点列跨度
  执行 `merge_right()`，避免目标行列跨度不一致导致纵向合并失败。
- 已补 parser 回归：
  `PDFium parser detects top-left two-by-two merged table candidate spans` 断言
  锚点单元格 `column_span = 2`、`row_span = 2`，并确认下方 continuation 行
  仍保持保守的单元格结构。
- 已补 importer 回归：
  `PDF text importer preserves top-left two-by-two merged table cells` 断言导入后
  为 1 个 4 列 x 3 行表格，锚点、右侧 continuation、下方 continuation 与
  保存重开后一致。
- 已完成视觉验证：
  `output/pdf-e7-merged-corner-visual/source-pdf/contact-sheet.png`、
  `output/pdf-e7-merged-corner-visual/merged-docx/evidence/contact_sheet.png`
  和 `output/pdf-e7-merged-corner-visual/merged-docx/table_visual_smoke.pdf`
  均已生成并目检通过。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --parallel 8 --target pdf_import_structure_tests pdf_import_failure_tests pdf_import_table_heuristic_tests`
  与
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  均通过；单独保留样本的
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  也通过。
- [x] 代码变更。
- [x] 测试覆盖。
- [x] 视觉验证。
- [x] 设计文档同步。
- [x] 已知限制记录。
- [x] 下一阶段入口保留：更复杂的嵌套合并、不规则跨列宽、扫描件和装饰线条缺失表格。
- 已知限制更新：
  组合合并目前只覆盖规则网格里可稳定复原的 2x2 锚点；不处理更复杂的嵌套合并、
  不规则跨列宽、扫描件或带装饰线条缺失的表格。

2026-05-14 继续推进（两行表格导入）：

- 已把 PDF 表格候选识别从“至少 3 行”保守扩展到受控的 2 行表：
  仅当候选有 3 列以上、首行全部像短标签表头、第二行至少包含一个明显数据型单元格时，
  才允许 `PdfiumParser` 把它提升为 `PdfParsedTableCandidate`。
- 已保持误判边界：
  普通两行三列 prose 样本不会被识别为表格；既有 two-column prose、aligned list、
  invoice summary 等负样本继续保持不命中。
- 已补导入侧回归：
  新样本 `featherdoc-pdf-import-two-row-table.pdf` 覆盖
  `paragraph / table / paragraph` 块顺序、2 行 x 3 列真实 `Document` 表格、
  保存重开后的表格行列数和关键单元格文本。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_structure_tests pdf_import_failure_tests pdf_import_table_heuristic_tests`
  与
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  均通过；单独保留样本的 `pdf_import_table_heuristic` 也通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-two-row-table-import-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-two-row-table-import-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-two-row-table-import-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 本轮继续保持导入与导出回归分离：
  新样本只存在于 `pdf_import_table_heuristic` 相关测试内，未加入
  `pdf_regression_manifest.json`，也未扩展 E6 发布 baseline。
- 已知限制更新：
  2 行表格识别仍是规则型启发式，偏向“短标签表头 + 明显数据行”的业务表；
  纯文本数据行、无明显数据特征的 2 行表、不规则列宽或扫描/OCR
  场景仍保持保守，不会强行导入为表格。
- 下一阶段入口保留：
  更复杂的嵌套合并、不规则跨列宽、扫描件和缺失装饰线条的表格。

2026-05-14 继续推进（两列 key-value 表格导入）：

- 已把 PDF 表格候选识别继续扩展到保守的 2 列 key-value 表：
  仅当候选至少 3 行、每行恰好 2 个文本簇、左列全部像短标签、
  右列至少半数具备明显数据特征时，才允许 `PdfiumParser` 把 2 列候选提升为
  `PdfParsedTableCandidate`。
- 已保持误判边界：
  普通 two-column prose、aligned numbered list、invoice summary、两列短标签 prose
  均继续保持不命中，避免把普通并列文本误导入为表格。
- 已补导入侧回归：
  新样本 `featherdoc-pdf-import-key-value-table.pdf` 覆盖
  `paragraph / table / paragraph` 块顺序、4 行 x 2 列真实 `Document` 表格、
  保存重开后的表格行列数和关键单元格文本。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  与
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  均通过；保留样本的同一专项回归也通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-key-value-table-import-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-key-value-table-import-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-key-value-table-import-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 本轮继续保持导入与导出回归分离：
  新样本只存在于 `pdf_import_table_heuristic` 相关测试内，未加入
  `pdf_regression_manifest.json`，也未扩展 E6 发布 baseline。
- 已知限制更新：
  2 列 key-value 表识别仍偏向“左短标签 + 右明显数据值”的业务表；
  右列全是短文本、标签和值跨行、缺失网格线、扫描/OCR 或更自由的表单布局仍保持保守。
- 下一阶段入口保留：
  更复杂的嵌套合并、不规则跨列宽、缺失装饰线条的表格、扫描件和 OCR 场景。

2026-05-14 继续推进（无装饰线条 key-value 表格）：

- 已补充无网格线的 2 列 key-value PDF 样本：
  `featherdoc-pdf-import-key-value-borderless-table.pdf` 只包含两列对齐文本，
  不绘制表格线，用于确认当前候选识别确实基于文本几何，而不是依赖装饰线条。
- 已补 parser 和 importer 回归：
  parser 断言该样本仍产生 1 个 `PdfParsedTableCandidate`，并保持
  `paragraph / table / paragraph` 块顺序；importer 断言显式 opt-in 后会写入
  4 行 x 2 列 `Document` 表格，保存重开后行列数和关键文本仍保留。
- 已完成测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  与
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  均通过；导入相关 5 项组合回归也通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-key-value-borderless-table-import-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-key-value-borderless-table-import-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-key-value-borderless-table-import-visual/merged-docx/table_visual_smoke.pdf`。
- 已知限制更新：
  本轮只覆盖规则两列、稳定行距的无装饰线条 key-value 表；缺失线条的多列表格、
  不规则跨列宽、扫描件和 OCR 场景仍未覆盖。
- 下一阶段入口保留：
  更复杂的嵌套合并、不规则跨列宽、扫描件和 OCR 场景。

2026-05-14 继续推进（不规则列宽表格导入）：

- 已把 3 列以上表格的列距规则从“必须近似等宽”保守扩展到一类不规则列宽表：
  仅当首行全部是短标签表头、后续每行列数完整、且每个数据行至少半数单元格具备明显
  数据特征时，才允许不规则列距候选进入 `PdfParsedTableCandidate`。
- 已保留误判边界：
  既有 invoice summary 三列表单仍保持不命中；普通双栏、编号列表、短标签并列文本等
  负样本继续由 `pdf_import_table_heuristic` 覆盖。
- 已补 parser 和 importer 回归：
  新样本 `featherdoc-pdf-import-irregular-width-table.pdf` 使用窄 item 列、
  宽 description 列和窄 amount 列，列距差超过常规 spacing tolerance；
  parser 断言仍能产生 1 个 4 行 x 3 列候选，importer 断言显式 opt-in 后写入真实
  `Document` 表格，保存重开后行列数和关键文本仍保留。
- 已完成测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  与
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  均通过；导入相关 5 项组合回归也通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-irregular-width-table-import-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-irregular-width-table-import-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-irregular-width-table-import-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  本轮不处理缺列、跨列标题、自由表单、不规则行距、扫描/OCR 或需要视觉线条解析的表格。
- 下一阶段入口保留：
  更复杂的嵌套合并、缺失线条的多列表格、扫描件和 OCR 场景。

2026-05-14 继续推进（缺失线条的多列表格）：

- 已补充无网格线的 3 列不规则列宽表格样本：
  `featherdoc-pdf-import-borderless-irregular-width-table.pdf` 只包含表头和数据文本，
  不绘制任何表格线，用于覆盖“缺失装饰线条的多列表格”入口。
- 已补 parser 和 importer 回归：
  parser 断言该样本仍产生 1 个 4 行 x 3 列 `PdfParsedTableCandidate`，并保持
  `paragraph / table / paragraph` 块顺序；importer 断言显式 opt-in 后写入真实
  `Document` 表格，保存重开后行列数和关键文本仍保留。
- 已完成测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  与
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  均通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-borderless-irregular-width-table-import-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-borderless-irregular-width-table-import-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-borderless-irregular-width-table-import-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  本轮只覆盖规则行距、列数完整、表头明确且数据特征明显的无线条多列表格；
  缺列、跨列表头、自由表单、扫描/OCR 和需要图像理解的表格仍未覆盖。
- 下一阶段入口保留：
  更复杂的嵌套合并、跨列表头、扫描件和 OCR 场景。

2026-05-14 继续推进（跨列表头表格导入）：

- 已补充分组表头横跨整表的 4 列 PDF 样本：
  `featherdoc-pdf-import-cross-column-header-table.pdf`，第一行是居中的
  `Project delivery overview`，第二行才是完整列标题，用于覆盖“跨列表头 + 明细表头”
  的正式文档常见结构。
- 已调整 `PdfiumParser` 的列锚点构建：
  当候选中存在至少两行完整列数据时，单簇分组表头不再污染列锚点；同时只在存在完整列行、
  且单簇文本居中时，才把该行提升为横跨全部列的 header cell，避免回退影响稀疏表格样本。
- 已补 parser 和 importer 回归：
  parser 断言该样本产生 1 个 4 行 x 4 列候选，首行 `column_span = 4`；
  importer 断言显式 opt-in 后写入真实 `Document` 表格，保存重开后跨列表头、
  明细表头和数据单元格仍保留。
- 已完成测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  与
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  均通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-cross-column-header-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-cross-column-header-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-cross-column-header-table-visual/merged-docx/table_visual_smoke.pdf`，
  目检未见裁剪、重叠或块顺序漂移。
- 已知限制更新：
  本轮只覆盖“单个居中分组表头 + 完整明细列标题 + 完整数据行”的规则结构；
  不处理多层分组表头、多个分组标题并列、缺列、自由表单、扫描/OCR 或需要图像理解的表格。
- 下一阶段入口保留：
  多层 / 并列分组表头、更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-14 继续推进（并列分组表头导入）：

- 已补充同一表头行内多个分组标题的 4 列 PDF 样本：
  `featherdoc-pdf-import-parallel-group-header-table.pdf`，第一行包含
  `Delivery scope` 与 `Review status` 两个并列分组标题，各自横跨 2 列；
  第二行仍保留完整明细列标题。
- 已将 `PdfiumParser` 的跨列表头推断从“单个居中标题”推广为“首行少簇分组标题”：
  仅当候选中存在完整列行时启用，并按相邻分组标题中心点切分列锚点，推断每个分组标题覆盖的
  连续列范围。规则限制在候选第一行，避免把中间缺项行、纵向合并续行或汇总行误判为分组表头。
- 已补 parser 和 importer 回归：
  parser 断言首行两个 header cell 均为 `column_span = 2`；
  importer 断言显式 opt-in 后写入真实 `Document` 表格，保存重开后两个并列分组标题、
  明细表头和数据单元格仍保留。
- 已完成测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  与
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  均通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-parallel-group-header-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-parallel-group-header-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-parallel-group-header-table-visual/merged-docx/table_visual_smoke.pdf`，
  目检未见裁剪、重叠或块顺序漂移。
- 已知限制更新：
  本轮只覆盖第一行并列分组标题；多层分组标题、交错 group、缺列、自由表单、
  扫描/OCR 和需要图像理解的表格仍不覆盖。
- 下一阶段入口保留：
  多层分组表头、更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-14 继续推进（多层分组表头导入）：

- 已补充两级分组表头的 4 列 PDF 样本：
  `featherdoc-pdf-import-multilevel-group-header-table.pdf`，第一行
  `Program delivery dashboard` 横跨整表，第二行 `Delivery scope` 与
  `Review status` 各自横跨 2 列，第三行才是完整明细列标题。
- 已把 `PdfiumParser` 的分组表头推断从“仅候选第一行”收敛扩展到
  “第一个完整列行之前的连续分组行”：
  每个分组行必须能按相邻标题中心点切分出连续列范围，并且该行所有分组标题共同覆盖整张表；
  一旦遇到无法完整覆盖的少簇行就停止，避免把缺项数据行、纵向合并续行或汇总行误判为表头。
- 已补 parser 和 importer 回归：
  parser 断言第一行 `column_span = 4`，第二行两个 `column_span = 2`；
  importer 断言显式 opt-in 后写入真实 `Document` 表格，保存重开后两级分组标题、
  明细表头和数据单元格仍保留。
- 已完成测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  与
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  均通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-multilevel-group-header-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-multilevel-group-header-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-multilevel-group-header-table-visual/merged-docx/table_visual_smoke.pdf`，
  目检未见裁剪、重叠或块顺序漂移。
- 已知限制更新：
  本轮只覆盖“完整覆盖整表的连续分组表头 + 完整明细列标题 + 完整数据行”；
  交错 group、缺列、自由表单、扫描/OCR 和需要图像理解的表格仍不覆盖。
- 下一阶段入口保留：
  更复杂的嵌套合并、缺列/汇总行识别、扫描件和 OCR 场景。

2026-05-14 继续推进（汇总行跨列导入）：

- 已补充尾部汇总行跨列的 4 列 invoice PDF 样本：
  `featherdoc-pdf-import-merged-summary-row-table.pdf`，末行只有
  `Grand total` 与 `USD 135` 两个文本簇，前者视觉上横跨前三列，金额保留在最后一列。
- 已在 `PdfiumParser` 中增加受控汇总行 span 推断：
  仅当候选已经出现完整列行之后，且当前行恰好是“首列短标签 + 末列数据值”时，
  才把首列标签推断为跨到末列之前；同时 row-span 检测会识别下方列已被横向 span
  覆盖，避免把上一行空缺列误判为纵向合并续行。
- 已补 parser 和 importer 回归：
  parser 断言汇总行 `Grand total` 为 `column_span = 3`、金额仍为单列；
  importer 断言显式 opt-in 后写入真实 `Document` 表格，保存重开后汇总标签跨列和金额单元格仍保留。
- 已完成测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  与
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  均通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-merged-summary-row-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-merged-summary-row-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-merged-summary-row-table-visual/merged-docx/table_visual_smoke.pdf`，
  目检未见裁剪、重叠或块顺序漂移。
- 已知限制更新：
  本轮只覆盖尾部“首列标签跨到末列前 + 末列金额/数据值”的规则汇总行；
  中间缺列、任意 subtotal 位置、右对齐标签、自由表单、扫描/OCR 和图像理解仍不覆盖。
- 下一阶段入口保留：
  中间缺列 / subtotal 行、更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-14 继续推进（中间 subtotal 跨列导入）：

- 已补充中间 subtotal 行的 4 列 invoice PDF 样本：
  `featherdoc-pdf-import-inline-subtotal-row-table.pdf`，明细行之后插入
  `Design subtotal` / `USD 100` 两簇 subtotal 行，后面继续保留普通明细行和
  `Grand total` 汇总行，用于确认稀疏跨列行不会截断后续数据。
- 已补 parser 和 importer 回归：
  parser 断言中间 subtotal 行的标签 `column_span = 3`，后续 `Visual validation`
  明细行仍在同一候选中；importer 断言显式 opt-in 后写入真实 `Document` 表格，
  保存重开后 subtotal、后续明细和 grand total 的跨列状态仍保留。
- 已完成测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  与
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  均通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-inline-subtotal-row-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-inline-subtotal-row-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-inline-subtotal-row-table-visual/merged-docx/table_visual_smoke.pdf`，
  目检未见裁剪、重叠或块顺序漂移。
- 已知限制更新：
  当前 subtotal 规则仍要求“首列标签 + 末列数据值”，不处理右对齐 subtotal 标签、
  中间金额列、跨页 subtotal 语义聚合、自由表单、扫描/OCR 或图像理解。
- 下一阶段入口保留：
  更复杂的嵌套合并、跨页 subtotal 诊断、扫描件和 OCR 场景。

2026-05-14 继续推进（右侧 subtotal 跨列导入）：

- 已补充右侧 subtotal 标签的 4 列 invoice PDF 样本：
  `featherdoc-pdf-import-right-subtotal-row-table.pdf`，subtotal / grand total 行保留首列为空，
  标签从第 2 列开始横跨到金额列之前，金额仍在末列。
- 已把 `PdfiumParser` 的 subtotal span 推断从“必须首列开始”放宽为
  “标签列在金额列之前即可”：
  仍要求候选中已经出现完整列行、当前行只有标签和值两簇、值位于末列，避免把普通缺项明细行
  扩成汇总行。
- 已补 parser 和 importer 回归：
  parser 断言 `Design subtotal` 和 `Grand total` 都从第 2 列开始 `column_span = 2`；
  importer 断言首列空单元格、右侧 subtotal 标签跨列、后续明细行和保存重开后的状态都保留。
- 已完成测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  与
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  均通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-right-subtotal-row-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-right-subtotal-row-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-right-subtotal-row-table-visual/merged-docx/table_visual_smoke.pdf`，
  目检未见裁剪、重叠或块顺序漂移。
- 已知限制更新：
  当前仍要求 subtotal 值在末列；中间金额列、跨页 subtotal 语义聚合、自由表单、
  扫描/OCR 或图像理解仍不覆盖。
- 下一阶段入口保留：
  中间金额列、更复杂的嵌套合并、跨页 subtotal 诊断、扫描件和 OCR 场景。

2026-05-14 继续推进（中间金额列 subtotal 导入）：

- 已补充金额不在末列的 4 列 invoice PDF 样本：
  `featherdoc-pdf-import-middle-amount-subtotal-row-table.pdf`，subtotal / grand total 行的
  标签从首列开始横跨到金额列之前，金额位于第 3 列，最后一列保留为空。
- 已收紧 subtotal span 推断：
  只有标签文本包含 `total` / `subtotal` 语义时，才允许“标签 + 值”稀疏行被提升为跨列汇总行；
  同时 row-span 检测遇到横向 span 行会停止，避免上一行末列被错误纵向合并到尾列空白。
- 已补 parser 和 importer 回归：
  parser 断言 `Design subtotal` / `Grand total` 均为 `column_span = 2`，金额在第 3 列，
  末列保持空白；importer 断言显式 opt-in 后写入真实 `Document` 表格，保存重开后
  subtotal、后续明细、金额列和尾列空白状态仍保留。
- 已完成测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  与
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  均通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-middle-amount-subtotal-row-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-middle-amount-subtotal-row-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-middle-amount-subtotal-row-table-visual/merged-docx/table_visual_smoke.pdf`，
  目检未见裁剪、重叠或块顺序漂移。
- 已知限制更新：
  当前只覆盖受控的 total/subtotal 稀疏行，不做跨页 subtotal 语义聚合、自由表单、
  扫描/OCR 或图像理解。
- 下一阶段入口保留：
  跨页 subtotal 诊断、更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-14 继续推进（跨页 subtotal 行续接导入）：

- 已补充跨页 invoice subtotal PDF 样本：
  `featherdoc-pdf-import-pagebreak-subtotal-row-table.pdf`。第一页包含
  `Design subtotal` 稀疏跨列行，第二页重复 `Item / Qty / Unit / Total`
  表头后继续明细并以 `Grand total` 收尾，用于覆盖 subtotal 跨列与跨页续接的组合场景。
- 已补导入侧回归：
  `PDF table import merges cross-page subtotal rows with repeated headers`
  断言 opt-in 导入后只生成 1 张 `Document` 表格，第二页 continuation diagnostic 为
  `merged_with_previous_table`，`skipped_repeating_header = true`，
  `source_row_offset = 1`，并且保存重开后 subtotal / grand total 的
  `column_span = 3` 状态仍保留。
- 本轮没有放宽 parser 的 subtotal 语义规则：
  仍复用已经收紧的 `total` / `subtotal` 标签判断；本增量主要验证 importer
  在 `append_rows_to_imported_table` 跳过重复表头后，仍能把追加行里的跨列单元格
  应用到正确的目标行索引。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-pagebreak-subtotal-row-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-pagebreak-subtotal-row-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-pagebreak-subtotal-row-table-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  当前覆盖“跨页续接 + 重复表头跳过 + 受控 total/subtotal 稀疏行”的组合，
  不做跨页 subtotal 语义聚合、跨页金额合计推导、自由表单、扫描/OCR 或图像理解。
- 下一阶段入口保留：
  更复杂的嵌套合并、缺列 / 变体表头下的跨页续接、扫描件和 OCR 场景。

2026-05-14 继续推进（跨页 subtotal 变体表头续接）：

- 已补充跨页 invoice subtotal 的缩写表头变体样本：
  `featherdoc-pdf-import-pagebreak-subtotal-header-variant-table.pdf`。第一页表头使用
  `Item / Quantity / Unit / Amount`，第二页重复表头使用
  `Item / Qty / Unit / Amt`，同时保留第一页 `Design subtotal` 和第二页
  `Grand total` 稀疏跨列行。
- 已修正 importer 的 repeated-header 判定入口：
  当表体存在受控的横向 span 摘要行时，允许较长业务表头使用稍宽松的正文长度判定；
  这样 `Quantity` / `Amount` 这类较长表头不会阻止后续 canonical abbreviation 匹配，
  但仍要求首行全是 header-like label、正文至少两行明显长于表头，并且存在正文跨列摘要行。
- 已补导入侧回归：
  `PDF table import merges cross-page subtotal rows with abbreviated repeated headers`
  断言第二页 continuation diagnostic 命中 `canonical_text`，最终 `source_row_offset = 1`、
  `skipped_repeating_header = true`，并确认第二页 `Qty` / `Amt` 表头没有写入最终表格；
  保存重开后 subtotal / grand total 的 `column_span = 3` 仍保留。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-pagebreak-subtotal-header-variant-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-pagebreak-subtotal-header-variant-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-pagebreak-subtotal-header-variant-table-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  本轮只放宽“正文含横向 span 摘要行”的 repeated-header 入口；不处理缺列表体、
  跨页金额合计推导、自由表单、扫描/OCR 或需要图像理解的表格。
- 下一阶段入口保留：
  缺列 / 变体表头下的跨页续接负样本、更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-14 继续推进（跨页 subtotal 语义表头负样本）：

- 已补充跨页 invoice subtotal 的语义变体表头负样本：
  `featherdoc-pdf-import-pagebreak-subtotal-semantic-header-variant-table.pdf`。
  第一页表头为 `Item / Quantity / Unit / Amount`，第二页表头改为
  `Item / Owner / Phase / Amount`，两页都保留 total/subtotal 稀疏跨列行，
  用于验证上一轮 repeated-header 入口放宽后仍不会误合并语义不同的表格。
- 已补导入侧回归：
  `PDF table import keeps cross-page subtotal tables separate for semantic header variants`
  断言第二页 continuation diagnostic 同时记录
  `previous_has_repeating_header = true`、`source_has_repeating_header = true`、
  `header_match_kind = none`、`blocker = repeated_header_mismatch`，
  最终导入结果保留为两张 `Document` 表格。
- 已确认保存重开后的结构：
  第一张表的 `Design subtotal` 和第二张表的 `Grand total` 仍保留
  `column_span = 3`，且第二张表的语义变体表头 `Owner / Phase` 没有被当作重复表头跳过。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-pagebreak-subtotal-semantic-header-variant-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-pagebreak-subtotal-semantic-header-variant-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-pagebreak-subtotal-semantic-header-variant-table-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  当前只确认语义不同的 repeated header 会保守拆表；缺列、错位列锚点、跨页金额合计推导、
  自由表单、扫描/OCR 或需要图像理解的表格仍未覆盖。
- 下一阶段入口保留：
  缺列 / 错位列锚点下的跨页续接边界、更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-14 继续推进（跨页 subtotal 列锚点错位负样本）：

- 已补充跨页 invoice subtotal 的列锚点错位负样本：
  `featherdoc-pdf-import-pagebreak-subtotal-anchor-mismatch-table.pdf`。
  两页表头文本都为 `Item / Qty / Unit / Total`，也都保留 total/subtotal 稀疏跨列行；
  但第二页网格列宽从 130pt 改为 150pt，用于验证列锚点不兼容时不会因为表头相同而误合并。
- 已补导入侧回归：
  `PDF table import keeps cross-page subtotal tables separate for anchor mismatches`
  断言第二页 continuation diagnostic 同时记录
  `previous_has_repeating_header = true`、`source_has_repeating_header = true`、
  `header_match_kind = exact`、`column_count_matches = true`、
  `column_anchors_match = false`、`blocker = column_anchors_mismatch`，
  最终导入结果保留为两张 `Document` 表格。
- 已确认保存重开后的结构：
  第一张表的 `Design subtotal` 和第二张表的 `Grand total` 仍保留
  `column_span = 3`；第二张表的重复表头没有被跳过，因为列锚点不兼容优先阻止续接。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-pagebreak-subtotal-anchor-mismatch-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-pagebreak-subtotal-anchor-mismatch-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-pagebreak-subtotal-anchor-mismatch-table-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  当前只确认列宽变化导致的列锚点错位会保守拆表；缺列表体、局部错位、跨页金额合计推导、
  自由表单、扫描/OCR 或需要图像理解的表格仍未覆盖。
- 下一阶段入口保留：
  缺列表体 / 局部错位下的跨页续接边界、更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-14 继续推进（跨页 subtotal 列数不匹配负样本）：

- 已补充跨页 invoice subtotal 的列数不匹配负样本：
  `featherdoc-pdf-import-pagebreak-subtotal-column-count-mismatch-table.pdf`。
  第一页为 `Item / Qty / Unit / Total` 四列表格并包含 `Design subtotal`
  稀疏跨列行；第二页改为 `Item / Qty / Total` 三列表格并包含
  `Grand total`，用于验证列数不同不会因为 repeated header 和 subtotal/total
  语义相近而误合并。
- 已补导入侧回归：
  `PDF table import keeps cross-page subtotal tables separate for column count mismatches`
  断言第二页 continuation diagnostic 同时记录
  `previous_has_repeating_header = true`、`source_has_repeating_header = true`、
  `column_count_matches = false`、`column_anchors_match = false`、
  `header_match_kind = none`、`header_matches_previous = false`、
  `skipped_repeating_header = false`、`source_row_offset = 0`、
  `blocker = column_count_mismatch`，最终导入结果保留为两张 `Document` 表格。
- 已确认保存重开后的结构：
  body blocks 保持 `paragraph / table / table / paragraph`；第一张表保留 4 列，
  第二张表保留 3 列；`Design subtotal` 的 `column_span = 3`，
  `Grand total` 的 `column_span = 2`，第二页表头不会被跳过。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-pagebreak-subtotal-column-count-mismatch-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-pagebreak-subtotal-column-count-mismatch-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-pagebreak-subtotal-column-count-mismatch-table-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  当前只确认整表列数不匹配会保守拆表；缺列表体、局部错位、跨页金额合计推导、
  自由表单、扫描/OCR 或需要图像理解的表格仍未覆盖。
- 下一阶段入口保留：
  缺列表体 / 局部错位下的跨页续接边界、更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-14 继续推进（跨页 subtotal 局部缺格续接）：

- 已补充跨页 invoice subtotal 的受控局部缺格样本：
  `featherdoc-pdf-import-pagebreak-subtotal-missing-unit-table.pdf`。
  第二页重复 `Item / Qty / Unit / Total` 表头后，第一条明细故意缺失 `Unit`
  文本但保留 4 列网格和末列金额，用于确认同列锚点、同表头、局部空单元格的
  subtotal 表格仍可续接。
- 已补 parser 侧回归：
  `PDFium parser keeps cross-page subtotal rows with a missing body cell aligned`
  断言两页各生成一张 4 列 table candidate；第二页缺 `Unit` 的明细行仍补齐
  4 个单元格，其中第 3 列 `has_text = false`，末列 `USD 10` 保持在第 4 列，
  `Grand total` 仍推断为 `column_span = 3`。
- 已补导入侧回归：
  `PDF table import merges cross-page subtotal rows with missing body cells`
  断言第二页 continuation diagnostic 为
  `column_count_matches = true`、`column_anchors_match = true`、
  `header_match_kind = exact`、`source_row_offset = 1`、
  `skipped_repeating_header = true`、`blocker = none`，最终导入为一张 7 行 4 列表格。
- 已确认保存重开后的结构：
  body blocks 保持 `paragraph / table / paragraph`；缺失 `Unit` 的单元格为空文本，
  同行 `Qty`、`Total`、后续正常明细以及 `Design subtotal` / `Grand total`
  跨列状态都保留。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-pagebreak-subtotal-missing-unit-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-pagebreak-subtotal-missing-unit-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-pagebreak-subtotal-missing-unit-table-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  当前只确认受控的单个明细空单元格会被补齐并允许跨页续接；局部列锚点错位、
  多个缺格导致的稀疏行、跨页金额合计推导、自由表单、扫描/OCR 或需要图像理解的表格仍未覆盖。
- 下一阶段入口保留：
  局部列锚点错位 / 多缺格下的跨页续接边界、更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-14 继续推进（跨页 subtotal 多缺格稀疏行续接）：

- 已补充跨页 invoice subtotal 的受控多缺格稀疏行样本：
  `featherdoc-pdf-import-pagebreak-subtotal-sparse-body-table.pdf`。
  第二页重复表头后，第一条明细只保留 `Item` 和 `Total`，故意缺失
  `Qty` / `Unit` 两个中间单元格文本，用于确认列锚点和列数兼容时，
  稀疏表体行不会破坏跨页 subtotal 续接。
- 已补 parser 侧回归：
  `PDFium parser keeps cross-page subtotal sparse body rows aligned`
  断言第二页仍生成 4 列 table candidate；稀疏明细行第 2、3 列
  `has_text = false`，末列 `USD 10` 保持在第 4 列，下一条正常明细和
  `Grand total` 跨列推断仍保留。
- 已补导入侧回归：
  `PDF table import merges cross-page subtotal rows with sparse body rows`
  断言第二页 continuation diagnostic 为
  `column_count_matches = true`、`column_anchors_match = true`、
  `header_match_kind = exact`、`source_row_offset = 1`、
  `skipped_repeating_header = true`、`blocker = none`，最终仍导入为一张
  7 行 4 列表格。
- 已确认保存重开后的结构：
  body blocks 保持 `paragraph / table / paragraph`；稀疏行的 `Qty` / `Unit`
  单元格为空文本，末列金额、后续正常明细和 `Grand total` 的 `column_span = 3`
  都保留。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-pagebreak-subtotal-sparse-body-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-pagebreak-subtotal-sparse-body-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-pagebreak-subtotal-sparse-body-table-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  当前只确认同一明细行两个中间空单元格的受控稀疏行；局部列锚点错位、
  只有单个数据簇的极稀疏行、跨页金额合计推导、自由表单、扫描/OCR 或需要图像理解的表格仍未覆盖。
- 下一阶段入口保留：
  局部列锚点错位 / 极稀疏行下的跨页续接边界、更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-15 继续推进（跨页 subtotal 金额-only 极稀疏行续接）：

- 已补充跨页 invoice subtotal 的金额-only 极稀疏行样本：
  `featherdoc-pdf-import-pagebreak-subtotal-amount-only-body-table.pdf`。
  第二页重复表头后，第一条明细只保留末列 `Total` 金额，故意缺失
  `Item` / `Qty` / `Unit` 三个单元格文本，用于确认有完整表头和后续完整行提供
  列锚点时，单个金额数据簇仍可稳定落在末列。
- 已补 parser 侧回归：
  `PDFium parser keeps cross-page subtotal amount-only body rows aligned`
  断言第二页仍生成 4 列 table candidate；极稀疏行前三列 `has_text = false`，
  `USD 10` 保持在第 4 列，下一条正常明细和 `Grand total` 跨列推断仍保留。
- 已补导入侧回归：
  `PDF table import merges cross-page subtotal rows with amount-only body rows`
  断言第二页 continuation diagnostic 为
  `column_count_matches = true`、`column_anchors_match = true`、
  `header_match_kind = exact`、`source_row_offset = 1`、
  `skipped_repeating_header = true`、`blocker = none`，最终仍导入为一张
  7 行 4 列表格。
- 已确认保存重开后的结构：
  body blocks 保持 `paragraph / table / paragraph`；极稀疏行前三列为空文本，
  末列金额、后续正常明细和 `Grand total` 的 `column_span = 3` 都保留。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-pagebreak-subtotal-amount-only-body-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-pagebreak-subtotal-amount-only-body-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-pagebreak-subtotal-amount-only-body-table-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  当前只确认有完整表头和相邻完整数据行支撑列锚点时的金额-only 极稀疏行；
  局部列锚点错位、孤立极稀疏表、跨页金额合计推导、自由表单、扫描/OCR 或需要图像理解的表格仍未覆盖。
- 下一阶段入口保留：
  局部列锚点错位 / 孤立极稀疏表下的跨页续接边界、更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-15 继续推进（跨页 subtotal 局部列锚点漂移负样本）：

- 已补充跨页 invoice subtotal 的局部列锚点漂移负样本：
  `featherdoc-pdf-import-pagebreak-subtotal-local-anchor-drift-table.pdf`。
  第二页仍保留 4 列网格和相同 `Item / Qty / Unit / Total` 表头，但明细行与
  `Grand total` 的数值列在同一列簇内局部右移，用于确认列数相同、表头相同且存在
  subtotal / total 跨列行时，列锚间距不兼容仍会保守拆表。
- 已补导入侧回归：
  `PDF table import keeps cross-page subtotal tables separate for local anchor drift`
  断言第二页 continuation diagnostic 为
  `previous_has_repeating_header = true`、`source_has_repeating_header = true`、
  `column_anchors_match = false`、`source_row_offset = 0`、
  `skipped_repeating_header = false`、`blocker = column_anchors_mismatch`，
  最终导入为两张独立的 4 行表格。
- 已确认保存重开后的结构：
  body blocks 保持 `paragraph / table / table / paragraph`；
  第一张表的 `Design subtotal` 和第二张表的 `Grand total` 均保留
  `column_span = 3`，第二张表没有因为 repeated header 文本相同而续接到第一页表格。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-pagebreak-subtotal-local-anchor-drift-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-pagebreak-subtotal-local-anchor-drift-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-pagebreak-subtotal-local-anchor-drift-table-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  当前只确认同列簇内的受控局部漂移会在平均列锚间距不兼容时拆表；
  不处理自由表单式列漂移、跨页金额合计推导、扫描/OCR 或需要图像理解的表格。
- 下一阶段入口保留：
  孤立极稀疏表、自由表单列漂移、更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-15 继续推进（跨页 subtotal 孤立金额-only 极稀疏行续接）：

- 已补充跨页 invoice subtotal 的孤立金额-only 极稀疏行样本：
  `featherdoc-pdf-import-pagebreak-subtotal-isolated-amount-only-body-table.pdf`。
  第二页只有重复表头、一个只保留末列金额的明细行，以及 `Grand total` 跨列行；
  不再放置后续完整明细行，用于确认列锚点可由重复表头和汇总行共同支撑。
- 已补 parser 侧回归：
  `PDFium parser keeps cross-page subtotal isolated amount-only body rows aligned`
  断言第二页仍生成 4 列 table candidate；孤立金额行前三列 `has_text = false`，
  `USD 10` 保持在第 4 列，`Grand total` 仍推断为 `column_span = 3`。
- 已补导入侧回归：
  `PDF table import merges cross-page subtotal rows with isolated amount-only body rows`
  断言第二页 continuation diagnostic 为
  `column_count_matches = true`、`column_anchors_match = true`、
  `header_match_kind = exact`、`source_row_offset = 1`、
  `skipped_repeating_header = true`、`blocker = none`，最终仍导入为一张
  6 行 4 列表格。
- 已确认保存重开后的结构：
  body blocks 保持 `paragraph / table / paragraph`；孤立金额行前三列为空文本，
  末列金额和 `Grand total` 的 `column_span = 3` 都保留。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-pagebreak-subtotal-isolated-amount-only-body-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-pagebreak-subtotal-isolated-amount-only-body-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-pagebreak-subtotal-isolated-amount-only-body-table-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  当前只确认重复表头和 subtotal/total 汇总行可支撑孤立金额-only 行的列锚点；
  孤立自由表单、局部列漂移、跨页金额合计推导、扫描/OCR 或需要图像理解的表格仍未覆盖。
- 下一阶段入口保留：
  自由表单列漂移、更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-15 继续推进（自由表单列漂移负样本）：

- 已补充自由表单列漂移样本：
  `featherdoc-pdf-import-free-form-column-drift-prose-import.pdf`。
  样本保留稳定行距和每行 3 个文本簇，但每行列位置都不同，模拟自由表单式并列文本；
  该场景不应因为行距稳定而被误识别为表格。
- 已补 parser 侧回归：
  `PDFium parser does not classify free-form column drift prose as table candidate`
  断言 `table_candidates.empty()`，并确认 `Topic` 到 `Closed` 的文本仍保留在段落结构中。
- 已补导入侧回归：
  `PDF text importer keeps free-form column drift prose as paragraphs`
  断言即使启用 `import_table_candidates_as_tables`，也保持
  `tables_imported = 0`，保存重开后仍没有表格，文本内容完整保留。
- 已完成构建与测试验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-free-form-column-drift-prose-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-free-form-column-drift-prose-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-free-form-column-drift-prose-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  当前支持整张表一致的不规则列宽；每行列位置各自漂移的自由表单仍按段落保留，
  不做表格结构推断、跨行字段对齐、扫描/OCR 或图像理解。
- 下一阶段入口保留：
  更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-15 继续推进（内部 2x2 组合合并）：

- 已补充内部 2x2 组合合并样本：
  `featherdoc-pdf-import-center-merged-table-source.pdf` 与
  `featherdoc-pdf-import-center-merged-table.pdf`。
  样本使用 4x4 规则网格，中间 `(row=1, col=1)` 单元格同时横跨 2 列和 2 行，
  用于覆盖非左上角锚点的组合合并路径。
- 已补 parser 侧回归：
  `PDFium parser detects center two-by-two merged table candidate spans`
  断言表格候选仍为 4 行 x 4 列，中心锚点 `column_span = 2`、
  `row_span = 2`，右侧和下方覆盖单元格保持为空文本占位。
- 已补导入侧回归：
  `PDF text importer preserves center two-by-two merged table cells`
  断言 opt-in 导入后仍保持 `paragraph / table / paragraph` 顺序，
  写入 4 行 x 4 列真实 `Document` 表格，保存重开后中心锚点为
  `vertical_merge = restart`，下方覆盖单元格为 `continue_merge`。
  Word 实际 cell 节点数为 14，避免把 2x2 合并错误展开成 16 个独立单元格。
- 已完成构建与测试验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-center-merged-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-center-merged-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-center-merged-table-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  组合合并现在覆盖规则网格里的左上角和内部 2x2 锚点；仍不处理任意嵌套合并、
  扫描/OCR、缺失线条后的视觉推断或需要图像理解的表格。
- 下一阶段入口保留：
  更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-15 继续推进（矩形 2x3 组合合并）：

- 已补充更大的内部矩形组合合并样本：
  `featherdoc-pdf-import-rectangular-merged-table-source.pdf` 与
  `featherdoc-pdf-import-rectangular-merged-table.pdf`。
  样本使用 5x5 规则网格，中间 `(row=1, col=1)` 单元格横跨 2 列和 3 行，
  用于确认组合合并不是 2x2 特例。
- 已补 parser 侧回归：
  `PDFium parser detects center two-by-three merged table candidate spans`
  断言表格候选为 5 行 x 5 列，中心锚点 `column_span = 2`、
  `row_span = 3`，两行下方覆盖区域都保持为空文本占位。
- 已补导入侧回归：
  `PDF text importer preserves center two-by-three merged table cells`
  断言 opt-in 导入后仍保持 `paragraph / table / paragraph` 顺序，
  写入 5 行 x 5 列真实 `Document` 表格，保存重开后中心锚点为
  `vertical_merge = restart`，两行下方覆盖单元格均为 `continue_merge`。
  Word 实际 cell 节点数为 22，避免把 2x3 合并错误展开成 25 个独立单元格。
- 已完成构建与测试验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-rectangular-merged-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-rectangular-merged-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-rectangular-merged-table-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  组合合并现在覆盖规则网格里的左上角 2x2、内部 2x2 和内部 2x3 矩形锚点；
  仍不处理任意嵌套合并、扫描/OCR、缺失线条后的视觉推断或需要图像理解的表格。
- 下一阶段入口保留：
  更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-15 继续推进（样式映射状态同步）：

- 已核对样式映射的当前真实实现状态，确认基础链路已经落地：
  `PdfTextRun` 已表达字体族/字体文件、字号、填充色、粗体、斜体、下划线、
  Unicode 标记和旋转角度；`layout_document_paragraphs()` 经 paragraph/table
  adapter 把直接 run 属性和继承样式翻译到 layout fragment；PDFio writer 已在
  content stream 中写出标准字体粗斜体变体、字号、颜色和下划线。
- 已同步 `design/02-current-roadmap.md`，把优先级 3 从“字段尚未串通”的过期描述
  调整为“基础样式映射已完成，剩余为合同级视觉 baseline、非标准字体粗斜体质量和
  发布门禁”。
- 已同步 `BUILDING_PDF.md`，在当前可用状态中补充 document adapter → PDFio writer
  的样式映射能力，并把正式可用前的限制收窄到复杂分页、图片锚点/裁剪/环绕、
  发布级合同样式视觉 baseline、字体捆绑和 PNG baseline 门禁。
- 本轮不新增生产代码，目标是避免后续继续推进时重复实现已完成的样式映射能力。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_document_adapter_font$" --output-on-failure --timeout 60`
  通过。
- 下一阶段入口保留：
  优先推进发布级视觉门禁或 HarfBuzz 文字塑形；如果继续补样式相关能力，应先补
  合同 sample 的视觉 baseline，而不是重复扩展已存在的 `PdfTextRun` 字段。

2026-05-15 继续推进（样式视觉 baseline 门禁）：

- 已把 `run_pdf_visual_release_gate.ps1` 的核心 PNG baseline 渲染从“合同、发票、
  长文档、图片语义、CLI font-map”扩展到样式专用样本：
  `styled-text`、`mixed-style-text`、`underline-text`、
  `document-contract-cjk-style` 和 `document-eastasia-style-probe`。
- 已给每个样式视觉样本补 `style_focus` 摘要，发布门禁的 `summary.json` 可以直接标出
  当前 contact sheet 需要人工关注的字号、颜色、粗斜体、下划线、CJK 字体映射和
  文档样式继承点。
- 已新增 `pdf_visual_release_gate_style_baselines` 轻量回归，静态解析
  `run_pdf_visual_release_gate.ps1` 并断言样式样本和 `style_focus` 标记不会被误删。
- 已同步 `BUILDING_PDF.md` 和 `design/02-current-roadmap.md`，把合同级样式视觉 baseline
  从“待补”推进到“已纳入 PDF 视觉发布门禁”。
- 已完成验证：
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File test/pdf_visual_release_gate_style_baselines_test.ps1 -RepoRoot . -WorkingDir .bpdf-roundtrip-msvc/pdf_visual_release_gate_style_baselines`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_visual_release_gate_style_baselines|pdf_regression_(styled|mixed|underline|document-contract|document-eastasia)|pdf_document_adapter_font|pdf_regression_manifest" --output-on-failure --timeout 60`
  通过 8/8；
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File scripts/run_pdf_visual_release_gate.ps1 -BuildDir .bpdf-roundtrip-msvc -OutputDir output/pdf-style-visual-release-gate -SkipUnicodeBaseline`
  通过，并生成 `output/pdf-style-visual-release-gate/report/aggregate-contact-sheet.png`。
- 下一阶段入口保留：
  非标准字体缺少 bold / italic 变体时的质量策略，或继续进入 HarfBuzz 文字塑形。

2026-05-15 继续推进（非标准字体 synthetic 粗斜体兜底）：

- 已把非标准字体缺少 bold / italic 变体时的策略从文档 TODO 落到代码：
  `PdfFontResolver` 仍优先选择 style-specific 映射；当显式映射、默认字体或 fallback
  只能提供 regular 字体时，会在 `PdfResolvedFont` 上标记 `synthetic_bold` /
  `synthetic_italic`。
- 已把 synthetic 标记经 document adapter 传入 `PdfTextRun`，PDFio writer 对 synthetic
  bold 使用 `PDFIO_TEXTRENDERING_FILL_AND_STROKE` 和小 stroke width，对 synthetic italic
  使用 12 度斜切 text matrix。这样不会重复 `TextShow`，避免 PDFium 回读出现重复文本。
- 已补回归：
  `pdf_font_resolver` 断言 style-specific 映射不会误标 synthetic，regular fallback 会标记
  synthetic；
  `pdf_document_adapter_font` 断言 synthetic 标记进入 layout，并覆盖 writer 输出后 PDFium
  回读不重复文本。
- 已同步 `BUILDING_PDF.md` 的字体选择说明，明确真实 bold / italic 字体优先，
  synthetic 只是防止样式完全丢失的视觉兜底。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_font_resolver_tests pdf_document_adapter_font_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(font_resolver|document_adapter_font)$" --output-on-failure --timeout 60`
  通过 2/2；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_(styled-text|mixed-style-text|underline-text|document-contract-cjk-style|document-eastasia-style-probe|contract-cjk-style)$" --output-on-failure --timeout 60`
  通过 6/6；
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File scripts/run_pdf_visual_release_gate.ps1 -BuildDir .bpdf-roundtrip-msvc -OutputDir output/pdf-style-visual-release-gate -SkipUnicodeBaseline`
  通过。
- 下一阶段入口：
  优先级 3 基础样式映射收口完成，下一步进入优先级 4 HarfBuzz 文字塑形。

2026-05-15 继续推进（HarfBuzz shaper bridge 最小接口）：

- 已新增独立 `pdf_text_shaper` 桥接层，先不改现有 writer/layout 行为：
  `shape_pdf_text()` 输入 UTF-8 文本、字体文件路径和字号，输出 `PdfGlyphRun`，
  包含 glyph id、cluster、x/y advance 和 x/y offset。
- 已在 `FeatherDoc::Pdf` 的 HarfBuzz 可用构建中定义
  `FEATHERDOC_ENABLE_PDF_TEXT_SHAPER=1`，并显式链接 `harfbuzz::harfbuzz`。
  `pdf_text_shaper_has_harfbuzz()` 可用于测试和诊断当前构建是否启用 HarfBuzz。
- 已补 `pdf_text_shaper` 单测，覆盖空文本、非法字号、缺字体路径，以及 HarfBuzz 可用时
  `office` 能产出非空 glyph run、正向 advance 和合法 cluster。
- 已同步 `design/02-current-roadmap.md` 和 `BUILDING_PDF.md`：
  优先级 4 的“接入 HarfBuzz 构建”和“最小 shaper_bridge”已完成；
  已知限制是 `PdfDocumentLayout` 仍消费字符串 `PdfTextRun`，PDFio writer 还没有用 glyph id
  写 content stream。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_text_shaper_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_text_shaper$" --output-on-failure --timeout 60`
  通过。
- 下一阶段入口：
  把 GlyphRun 接入 `PdfDocumentLayout` 的内部文本片段，先保留字符串回退，再让 PDFio writer
  增加 glyph id 写出路径。

2026-05-15 继续推进（GlyphRun 接入 PdfTextRun）：

- 已把 glyph run 数据结构拆成独立 `pdf_glyph_run.hpp`，供 shaper bridge 和 layout
  共同使用，避免让纯数据 layout 头直接承担 HarfBuzz 调用接口。
- `PdfTextRun` 现在可以携带 `PdfGlyphRun`。document adapter 在 file-backed text fragment
  上调用 `shape_pdf_text()`，仅在 HarfBuzz 成功产出非空 glyph run 时保留结果；
  标准 PDF base font、缺字体和坏字体仍保持原字符串路径。
- 已补 `pdf_document_adapter_font` 回归，断言 file-backed `office` 文本在 HarfBuzz 可用时
  会进入 `PdfTextRun::glyph_run`，并检查 glyph advance、cluster、字体路径和字号。
- 已同步 `BUILDING_PDF.md` 和 `design/02-current-roadmap.md`：
  当前状态是 layout 已能携带 GlyphRun，但 line wrapping 和 PDFio writer 仍然消费字符串。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_document_adapter_font_tests pdf_text_shaper_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font|unicode_font_roundtrip)$" --output-on-failure --timeout 60`
  通过 3/3。
- 下一阶段入口：
  研究 PDFio Unicode CID 字体当前的 `CIDToGIDMap` / `ToUnicode` 写法，再决定 writer
  是直接写 CID/glyph hex string，还是先扩展 PDFio-side helper。

2026-05-15 继续推进（GlyphRun advance 接入 layout 宽度）：

- 已确认 PDFio 当前 Unicode 字体写法是：content stream 写 UTF-16 codepoint，
  font resource 的 `CIDToGIDMap` 再把 Unicode CID 映射到 glyph id；因此不能直接把
  HarfBuzz glyph id 当 CID 写入，否则会和现有 `CIDToGIDMap` / `ToUnicode` 语义错配。
- 已先推进低风险前置：`measure_text()` 在有字体文件时会优先调用 `shape_pdf_text()`，
  并用 `glyph_run_x_advance_points()` 作为 layout 宽度。HarfBuzz 不可用、缺字体、
  坏字体或塑形失败时仍回退到 FreeType / 估算宽度。
- 已扩展 `pdf_document_adapter_font` 回归：同一段落两个 file-backed run 使用不同字体族
  保持拆分，第二个 run 的 baseline x 坐标必须等于第一个 run 的 baseline x 加上
  `PdfGlyphRun` 的 x advance。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_document_adapter_font_tests pdf_text_shaper_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(font_resolver|text_metrics|text_shaper|document_adapter_font|unicode_font_roundtrip)$" --output-on-failure --timeout 60`
  通过 5/5。
- 下一阶段入口：
  若继续推进 writer，需要先设计新的 glyph-id font resource：要么让 CIDToGIDMap 变为
  identity 并为 clusters 生成 ToUnicode，要么继续借助 ActualText 做复制语义兜底。

2026-05-15 继续推进（HarfBuzz 相关 regression 样本扩展）：

- 已新增两个导出回归样本：
  `mixed-cjk-punctuation-text` 覆盖中英混排、中文逗号/分号/句号、中文括号和弯引号；
  `latin-ligature-text` 覆盖 `office`、`affinity`、`flow`、`file`、`fixture` 等
  fi/fl 相关文本。
- 已把 `pdf_regression_manifest.json` 从 37 个样本扩展到 39 个样本；
  `pdf_regression_*` 当前覆盖 40 个 CTest，包含 manifest 校验。
- 已更新 `test/CMakeLists.txt` 的 CJK skip 规则，让 `mixed_cjk_punctuation_text`
  和其他 CJK 样本一样，在缺少 CJK 字体时以 77 跳过。
- 已同步 `BUILDING_PDF.md` 和 `design/02-current-roadmap.md` 的样本数量与覆盖范围。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^(pdf_regression_manifest|pdf_regression_(mixed-cjk-punctuation-text|latin-ligature-text))$" --output-on-failure --timeout 60`
  通过 3/3；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60`
  通过 40/40。
- 下一阶段入口：
  继续考虑是否把这两个 HarfBuzz 相关样本纳入视觉发布门禁 baseline。

2026-05-15 继续推进（文字塑形样本纳入视觉发布门禁）：

- 已把 `mixed-cjk-punctuation-text` 和 `latin-ligature-text` 加入
  `run_pdf_visual_release_gate.ps1` 的核心 PNG baseline 渲染列表。
- 已新增 `pdf_visual_release_gate_text_shaping_baselines` 静态回归，解析视觉门禁脚本并断言
  两个文字塑形样本的 `name`、PDF 路径和 baseline 输出目录不会被误删。
- 已同步 `BUILDING_PDF.md` 和 `design/02-current-roadmap.md`，把优先级 4 的
  “视觉回归 sample 集扩展到中英混排、CJK 标点用例”推进为已完成。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_visual_release_gate_(style|text_shaping)_baselines$" --output-on-failure --timeout 60`
  通过 2/2；
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File scripts/run_pdf_visual_release_gate.ps1 -BuildDir .bpdf-roundtrip-msvc -OutputDir output/pdf-text-shaping-visual-release-gate -SkipUnicodeBaseline`
  通过，并生成文字塑形样本 baseline 与 aggregate contact sheet。
- 下一阶段入口：
  优先级 4 剩余核心风险集中在 PDFio writer 的 glyph-id content stream 与自定义
  CIDToGIDMap / ToUnicode 设计。

2026-05-15 继续推进（PDFio writer glyph-id content stream）：

- 已在 `pdf_writer.cpp` 增加 shaped glyph font resource 路径：对满足安全条件的
  file-backed `PdfTextRun::glyph_run` 分配私有 CID，创建独立 Type0 / CIDFontType2 字体资源，
  用 `CIDToGIDMap` 映射到 HarfBuzz glyph id，用 `W` 数组写入 HarfBuzz x advance，并保留
  ToUnicode / ActualText 文本提取语义。
- 已保留字符串 fallback：HarfBuzz 不可用、缺少字体文件、glyph offset / y advance 当前无法安全表达、
  或 glyph id 超出 16-bit CID 范围时仍走原 `pdfioContentTextShow()` 路径。
- 已补 `pdf_unicode_font_roundtrip` writer 回归：构造 shaped Latin 文本，断言 PDF 中出现
  `FeatherDocGlyph` / `CIDToGIDMap` / `ToUnicode`，并用 PDFium 回读确认原文只出现一次。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font|unicode_font_roundtrip)$" --output-on-failure --timeout 60`
  通过 3/3；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60`
  通过 40/40；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_visual_release_gate_(style|text_shaping)_baselines$" --output-on-failure --timeout 60`
  通过 2/2；
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File scripts/run_pdf_visual_release_gate.ps1 -BuildDir .bpdf-roundtrip-msvc -OutputDir output/pdf-glyph-writer-visual-release-gate -SkipUnicodeBaseline`
  通过，并生成 aggregate contact sheet。
- 下一阶段入口：
  后续若扩大 writer 塑形能力，优先处理 x/y offset、RTL / 竖排和
  多 glyph 同 cluster 的 ToUnicode 映射策略。

2026-05-15 继续推进（shaped glyph writer 安全边界与 CMap 回归）：

- 已收紧 `pdf_writer.cpp` 的 shaped glyph writer 入口：要求 `PdfGlyphRun` 字号与
  `PdfTextRun` 一致，cluster 必须落在原始 UTF-8 文本范围内，并且 glyph 顺序中的 cluster
  不能倒序。倒序 cluster 代表 RTL 或更复杂重排语义，当前仍回退到字符串写出路径。
- 已扩展 `pdf_unicode_font_roundtrip`：
  解压 PDF Flate stream，定位 `/FeatherDocGlyphToUnicode` CMap，并逐项断言 shaped glyph
  私有 CID 到 cluster Unicode 文本的 `beginbfchar` 映射；同时新增倒序 cluster 的 fallback
  回归，确认不会生成 `/FeatherDocGlyph` 字体资源，PDFium 仍能回读原文。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_unicode_font_roundtrip_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font|unicode_font_roundtrip)$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60`
  通过；
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File scripts/run_pdf_visual_release_gate.ps1 -BuildDir .bpdf-roundtrip-msvc -OutputDir output/pdf-glyph-positioned-visual-release-gate -SkipUnicodeBaseline`
  通过。
- 下一阶段入口：
  继续扩展 shaped writer 前，先补齐 glyph offset / y advance 的定位模型；RTL 和竖排需要
  独立设计基线、方向和 text matrix 语义，不能直接复用当前 LTR `Tj` 路径。

2026-05-15 继续推进（shaped glyph offset 定位写出）：

- 已放开 shaped glyph writer 对 x/y offset 和 y advance 的硬 fallback：入口仍要求 file-backed
  font、HarfBuzz 成功、字号一致、cluster 有界且非倒序、glyph id 可放入 16-bit CID，并新增
  advance / offset 的 finite 检查。
- 已新增定位写出分支：普通无 offset run 保持原来的整段 `<cid...> Tj`；一旦 shaped run
  带有 x/y offset 或 y advance，就按 HarfBuzz pen position 逐 glyph 写 `Tm` + `<cid> Tj`。
  该分支复用当前 text run 的旋转矩阵和 synthetic italic 斜切矩阵，避免定位语义和既有
  `PdfTextRun` 样式分叉。
- 已扩展 `pdf_unicode_font_roundtrip`：构造带非零 offset / y advance 的 shaped Latin run，
  解压 PDF content stream 断言逐 glyph `Tm` 与 `Tj` 数量，并用 PDFium 回读确认原文只出现一次。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_unicode_font_roundtrip_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_unicode_font_roundtrip$" --output-on-failure --timeout 60`
  通过。
- 下一阶段入口：
  shaped writer 剩余风险主要是 RTL / 竖排的方向模型，以及多 glyph 同 cluster 在定位和
  文本选择中的更细粒度验收。

## 阶段推进规则

每一阶段开始前必须满足：

- 先阅读本地源码和现有测试，确认当前真实进度，不只依赖记忆或旧文档判断。
- 前一阶段核心测试通过。
- 新增任务能沉淀成测试或文档。
- 不需要默认开启 PDF 模块。
- 不引入必须联网下载的默认构建依赖。

开始实现下一个任务前，至少要检查：

- `src/pdf/` 中对应实现是否已经存在。
- `include/featherdoc/pdf/` 中接口和数据结构是否已经覆盖该能力。
- `test/` 中是否已有对应 PDF 单测、CLI 测试或 regression manifest 样本。
- `design/02-current-roadmap.md`、本文档和 `BUILDING_PDF.md` 是否已经记录过该能力。

每一阶段结束时必须留下：

- [ ] 代码变更。
- [ ] 测试覆盖。
- [ ] 本文档 checkbox 和进度说明同步。
- [ ] 设计文档或构建文档同步。
- [ ] 已知限制记录。
- [ ] 下一阶段入口条件。

## 阻塞与放弃条件

遇到以下情况时，必须暂停当前实现并回到设计评估：

- PDFio 无法稳定完成字体嵌入或 ToUnicode。
- PDFium 回读结果无法支撑基础验收。
- 默认构建被 PDF 依赖污染。
- 真实样本只能靠大量 hard-code 通过。
- 输出质量无法通过文本回读和视觉回归共同验证。

不允许把以下方案作为短期绕路：

- 默认开启 PDF 模块。
- 自研完整 PDF parser。
- 自研完整 PDF renderer。
- 从 LibreOffice 挖局部代码直接拼进 Core。
- 当前阶段改接 Skia 作为主线。

## 本地构建与测试命令

MSVC 环境下推荐先进入 VS 开发环境再构建 PDF 测试目标：

```powershell
cmd /c '"D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build .bpdf-roundtrip-msvc --target pdf_document_adapter_font_tests featherdoc_pdf_regression_sample pdf_regression_manifest_tests'
```

常用回归命令：

```powershell
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_document_adapter_font$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_(font_resolver|text_metrics|document_adapter_font|cli_export|unicode_font_roundtrip|regression_manifest)$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdfium_.*probe|pdf_import_structure" --output-on-failure --timeout 60
```

测试超时统一使用 60 秒，避免后台任务卡死。

## 维护规则

- 本文档记录后续执行路线，完成状态以 checkbox 更新。
- 每次开始实现下一个 PDF 任务前，必须先阅读本地源码和测试，确认实际已经做到哪一步。
- 每次完成一个 PDF 任务后，必须更新本文档，至少同步 checkbox、验收结果、遗留问题和下一步入口。
- 已稳定落地并面向用户的内容，应迁移或同步到 `docs/`。
- 临时 PDF、PNG、字体和 baseline 生成物不应无说明地进入版本库。
- 每次新增 PDF 能力，必须同时回答四个问题：
  能否生成、能否回读、能否可视化验证、能否回归。

## Owner

本方向负责人：wuxianggujun。
