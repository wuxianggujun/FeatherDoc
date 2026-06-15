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
- [x] 中文 PDF 的复制/搜索语义有自动化代理验收：
  PDFium 文本抽取可命中中文整行与常见搜索片段。
- [ ] 中文 PDF 在常见阅读器中完成手工复制、搜索验收。
- [x] 生成 PDF 中嵌入的是字体子集，不是完整大字体。
- [x] 缺字体时有明确错误或明确 fallback，不静默输出乱码。

### 进展记录

2026-05-07：

- 开始 E4 前已阅读 `include/featherdoc/pdf/pdf_font_resolver.hpp`、`src/pdf/pdf_font_resolver.cpp`、`src/pdf/pdf_font_subset.cpp`、`src/pdf/pdf_text_metrics.cpp`、`src/pdf/pdf_writer.cpp`、`test/pdf_font_resolver_tests.cpp` 和 `test/pdf_unicode_font_roundtrip_tests.cpp`，确认已有显式映射、默认字体、CJK fallback、样式变体、FreeType 度量、HarfBuzz 子集化和 PDFium 回读基础覆盖。
- 已补充 resolver 边界测试：缺失显式映射时回退到默认字体；禁用系统 fallback 且无 CJK 字体时返回空路径和 Unicode 标记，供上层诊断。
- 已补充 writer 诊断：Unicode 文本没有解析出嵌入字体文件时直接失败，错误包含 `Unicode PDF text requires an embedded font file` 和字体族名。
- 已补充 writer 缺字体/坏字体测试：不存在的字体路径、损坏字体文件都会失败并在错误信息里带出字体路径。
- 已将 Unicode roundtrip 文本扩展为中文、英文、数字、中文标点混排，确认 PDFium 回读和 ToUnicode 路径覆盖混排标点。
- 已补充 CJK 复制/搜索语义代理回归：生成两行中文混排 PDF，确认 `/ToUnicode`
  与 `/Identity-H` 存在，PDFium 抽取文本可命中整行中文、中文搜索片段和 ASCII 编号。
- 已确认 subset PDF 明显小于 full PDF：本地视觉 smoke 中 full 为 10,213,214 bytes，subset 为 19,874 bytes；两者渲染内容一致。
- 已在 `BUILDING_PDF.md` 记录当前 CJK fallback 顺序和字体许可证义务：默认使用系统/用户显式提供字体，不把系统字体重新分发进仓库。
- 已知限制：常见阅读器中的手工复制/搜索还没有形成自动化或人工验收记录；当前新增的是
  PDFium + ToUnicode 代理验收，后续发布前仍需要补可复现的手工验收记录。

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
  `--render-headers-and-footers`、`--expand-header-footer-page-placeholders`、`--render-inline-images`、`--no-font-subset`、
  `--no-system-font-fallbacks`、`--summary-json` 和 `--json`。
- 已补充 `--font-map` 的解析和转发，`--summary-json` 的机器可读输出，以及 `--no-system-font-fallbacks` 的诊断开关。
- 已补充 `pdf_cli_export` 回归覆盖：成功导出、inline image、font-map、非法 font-map、缺输入、缺字体。
- 已补充 `cli_usage` 断言，固定 `export-pdf` 的帮助文本和新增参数名。
- 已完成可视化验证：CLI 生成的 `font-map-source.pdf` 已渲染为 3 页 PNG/contact sheet；目检无空白页、明显裁剪、重叠或页眉页脚错位。
- 已确认成功路径在 roundtrip 构建里能被 PDFium 回读，正文页数与关键文本一致。
- 已知限制：当前 CLI 还没有单独暴露“PDFium 回读失败”作为独立用户子命令，现阶段通过 PDFium parser 测试和 roundtrip 回归覆盖。

2026-05-15：

- 已补充 `cli_usage` 断言，确保 `export-pdf` 帮助文本继续暴露 `--no-font-subset`。
- 已补充 `pdf_cli_export` 回归，直接对比 `--no-font-subset` 与默认子集化输出，确认
  CJK 导出场景里完整嵌入与子集嵌入确实走的是不同路径。

2026-05-28：

- 已把 `--expand-header-footer-page-placeholders` 暴露到 `export-pdf` CLI，页眉 / 页脚里的
  `{{page}}` 和 `{{total_pages}}` 可以在导出时展开。
- 已让 `--summary-json` 记录 `render_headers_and_footers`、`render_inline_images`、
  `expand_header_footer_page_placeholders`、`subset_unicode_fonts` 和 `use_system_font_fallbacks`，
  方便 CI 与回归报告追踪实际导出配置。

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

2026-05-18：

- 已静态接入 `document-table-merged-cant-split-text` regression sample，
  覆盖 merged cells、`cant_split`、重复表头与分页后的整块下移行为。
- 本轮低资源整合仅完成 sample、manifest、manifest parser 断言和文档说明同步；
  真实导出、PDFium 回读和 PNG 视觉门禁仍需在允许构建/渲染时补跑确认。

2026-06-14：

- 已在 `.bpdf-roundtrip-msvc` 中补跑
  `pdf_regression_document-table-merged-cant-split-text`，确认该样本可真实导出 PDF，
  并通过 PDFium 回读校验 3 页、页眉页脚、重复表头和 merged cant-split 行关键文本。
- 已将生成的
  `.bpdf-roundtrip-msvc/test/featherdoc-pdf-regression-document-table-merged-cant-split-text.pdf`
  渲染为 PNG，并生成样本级 contact sheet；目检确认 3 页非空，Page 2 在重复表头下保留
  merged cant-split 行，未见明显裁剪、重叠或页眉页脚错位。
- 样本级视觉证据产物写入
  `output/pdf-document-table-merged-cant-split-visual/summary.json` 与
  `output/pdf-document-table-merged-cant-split-visual/contact-sheet.png`；这些仍是本地验证产物，
  不作为版本库源文件提交。

通过命令：

```powershell
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_regression_document-table-merged-cant-split-text$" --output-on-failure --timeout 120
.\.venv-pdf-visual-smoke\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-roundtrip-msvc\test\featherdoc-pdf-regression-document-table-merged-cant-split-text.pdf --output-dir .\output\pdf-document-table-merged-cant-split-visual\pages --summary .\output\pdf-document-table-merged-cant-split-visual\summary.json --contact-sheet .\output\pdf-document-table-merged-cant-split-visual\contact-sheet.png --dpi 144
```

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

详细 E7 历史进展已按主题拆分到以下存档，主执行计划只保留当前目标、任务和验收边界：

- [结构与表格导入](04-pdf-execution-plan_e7_structure_and_table_import.md)
- [续页表头与 subtotal](04-pdf-execution-plan_e7_repeated_headers_and_subtotals.md)
- [跨页 subtotal 边界](04-pdf-execution-plan_e7_cross_page_subtotal_boundaries.md)
- [组合合并、塑形与 RTL](04-pdf-execution-plan_e7_merge_shaping_rtl.md)
- [CLI、文档与诊断](04-pdf-execution-plan_e7_cli_docs_diagnostics.md)

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

2026-05-15 继续推进（BUILDING_PDF PDF import 用户内容去重）：

- 已将 `BUILDING_PDF.md` 中面向用户的 PDF import 范围、限制和表格续接说明去重，
  统一指向 `docs/pdf_import.rst` 的 `PDF import JSON diagnostics` 与
  `PDF import supported scope and limits` 小节。
- `BUILDING_PDF.md` 现在只保留构建、开发者测试和
  `PdfDocumentImportResult::table_continuation_diagnostics` 调试入口，避免
  README、Sphinx 文档和构建文档之间的用户契约漂移。
- 本轮是文档-only 收口，没有改动生产代码；验证范围为 `git diff --check`。
- 继续 E7 时，优先把 PDF import 长文档拆成独立 `docs/pdf_import_*.rst` 页面，
  或继续补 CLI diagnostics 的用户示例。

2026-05-15 继续推进（PDF import 独立用户文档页）：

- 已新增 `docs/pdf_import.rst`，把 `import-pdf --json` 示例、
  `table_continuation_diagnostics` 字段契约、支持范围和已知限制从
  `docs/index.rst` 迁出。
- 已在 `docs/index.rst` 的 hidden toctree 注册 `pdf_import`，首页只保留短入口，
  避免主文档继续膨胀。
- 已同步 `BUILDING_PDF.md` 的用户文档指向，继续保持构建文档只承载开发者入口。
- 本轮仍是文档-only 结构整理；验证范围为 `git diff --check`、暂存区空白检查和
  文档引用检索。当前本机 Python 3.13 缺少 `sphinx` 模块，HTML 文档构建未执行。

2026-05-15 继续推进（PDF import 失败 JSON 文档补强）：

- 已核对 `PdfDocumentImportFailureKind` 和 CLI `print_pdf_import_failure()` 输出，
  确认失败 JSON 字段为 `command`、`ok`、`stage`、`failure_kind`、`message`、
  `input` 和 `output`。
- 已在 `docs/pdf_import.rst` 补充完整失败 JSON 示例、`failure_kind` 枚举，以及
  `table_candidates_detected` 不写出目标 DOCX 的用户可见语义。
- 本轮未改动生产代码；验证范围保持为 `git diff --check`、暂存区空白检查和
  文档引用检索。Sphinx 构建仍受本机缺少 `sphinx` 模块限制，未执行。

2026-05-15 继续推进（PDF import 文档契约回归）：

- 已新增 `test/pdf_import_docs_contract_test.ps1`，固定 `docs/pdf_import.rst`
  必须包含成功 JSON、失败 JSON、continuation diagnostics 字段、`failure_kind`
  枚举和支持范围入口。
- 已把 `pdf_import_docs_contract` 挂入 `test/CMakeLists.txt` 的 Windows
  PowerShell 测试集合，并设置 60 秒超时和 `pdf;docs;smoke` 标签。
- 回归同时检查 `docs/index.rst` hidden toctree 仍注册 `pdf_import`，首页只保留
  `:doc:`pdf_import`` 短入口，且中英文 README 的字段级 schema 指向
  `docs/pdf_import.rst`。
- 本轮验证通过：
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File .\test\pdf_import_docs_contract_test.ps1 -RepoRoot .`
  和 `git diff --check`。

2026-05-15 继续推进（PDF import 安装文档入口闭环）：

- 已确认中英文 README 的 PDF import schema 入口指向 `docs/pdf_import.rst`，
  但安装规则此前没有携带该文件，安装包内会形成指向缺失文件的入口。
- 已在 `CMakeLists.txt` 中安装 `docs/pdf_import.rst` 到
  `${FEATHERDOC_INSTALL_DATADIR}/docs`，对应安装树为
  `share/FeatherDoc/docs/pdf_import.rst`。
- 已同步 `README.md` 和 `README.zh-CN.md` 的安装产物清单，并扩展
  `pdf_import_docs_contract_test.ps1`，固定 CMake 安装规则必须包含该文档。
- 本轮验证通过：
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File .\test\pdf_import_docs_contract_test.ps1 -RepoRoot .`
  和 `git diff --check`。

2026-05-15 继续推进（PDF import 发布打包安全回归）：

- 已扩展 `test/package_release_assets_safety_test.ps1`，在模拟安装前缀中加入
  `share/FeatherDoc/docs/pdf_import.rst`，并固定 staging 目录必须保留该文件。
- 已新增安装 ZIP 条目断言，确保
  `FeatherDoc-v1.6.4-msvc-install.zip` 内继续携带
  `build-msvc-install/share/FeatherDoc/docs/pdf_import.rst`。
- ZIP 条目比较已统一归一化路径分隔符，避免 Windows 反斜杠条目导致跨实现误报。
- 本轮验证通过：
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File .\test\package_release_assets_safety_test.ps1 -RepoRoot . -WorkingDir .\output\package-release-assets-safety-test`
  和 `git diff --check`。

2026-05-15 继续推进（发布包 PowerShell CTest 门禁治理）：

- 已给 `package_release_assets_safety` 和 `package_release_assets_allow_incomplete`
  两个 PowerShell CTest 补上 `TIMEOUT 60`，对齐 PDF 执行计划的测试超时规范。
- 已为两个测试增加 `release;package;smoke` 标签，方便 CI 或本地按发布包门禁集合筛选。
- 本轮不改变打包行为，只补 CTest 调度边界；继续 E7 时，发布包安全测试可作为
  PDF import 安装文档入口的轻量发布门禁。

2026-05-15 继续推进（PDF import diagnostics 枚举文档契约）：

- 已扩展 `pdf_import_docs_contract_test.ps1`，把
  `table_continuation_diagnostics` 的布尔诊断字段纳入文档契约。
- 已把 `header_match_kind`、`disposition` 和 `blocker` 的稳定字符串枚举纳入回归，
  覆盖重复表头匹配、合并/拆表结论和跨页续接阻断原因。
- 本轮不改变 CLI JSON 输出，只把已经面向用户文档化的诊断 schema 固定为
  可回归契约，降低后续 E7 调整时的文档漂移风险。

2026-05-15 继续推进（PDF import enum 驱动文档契约）：

- 已增强 `pdf_import_docs_contract_test.ps1`，从
  `include/featherdoc/pdf/pdf_document_importer.hpp` 解析 PDF import failure、
  table continuation disposition、blocker 和 header match kind 枚举成员。
- 文档契约现在会要求 `docs/pdf_import.rst` 覆盖这些源定义中的公开字符串，
  其中 `PdfDocumentImportFailureKind::none` 仍作为内部状态不要求写入失败文档枚举。
- 同一回归还检查 `cli/featherdoc_cli.cpp` 为每个 enum 成员保留对应
  `return "<member>"` 映射，避免源码 enum、CLI JSON 和用户文档三方漂移。

2026-05-15 继续推进（PDF import confidence 阈值文案统一）：

- 已把 `--min-table-continuation-confidence` 的用户可见占位符从 `<count>`
  统一为 `<score>`，覆盖 CLI usage、英文 README 和中文 README。
- 已扩展 `cli_usage_tests.cpp`，固定 usage 必须出现 `<score>` 且不能回退到
  `<count>`；该参数语义是规则型 continuation confidence 阈值，不是数量。
- 已扩展 `pdf_import_docs_contract_test.ps1`，同步约束中英文 README 中的同一占位符。

2026-05-15 继续推进（PDF import confidence 解析错误回归）：

- 已补充 `pdf_cli_import_tests.cpp`，覆盖
  `--min-table-continuation-confidence` 缺失值、非法值和重复传参三类 parse 错误。
- 回归固定这些错误在 `--json` 下输出 `command:"import-pdf"`、`ok:false`、
  `stage:"parse"` 和对应错误消息，避免被误归类为 import 阶段失败。
- 本轮不改变 CLI 行为，只补齐 confidence 阈值参数的负路径 JSON 契约。

2026-05-15 继续推进（PDF CLI CTest 超时治理）：

- 已给 `pdf_cli_export` 和 `pdf_cli_import` 两个 CTest 入口补上 `TIMEOUT 60`，
  对齐 PDF 执行计划中后台测试统一 60 秒超时的约束。
- 本轮不改变 CLI 导入/导出行为，只收紧 CTest 调度边界，避免本地或 CI 后台任务
  因 fixture、字体环境或进程异常而无界等待。
- 继续 E7 时，可优先把 PDF CLI import/export 作为轻量烟测集合，配合
  `cli;smoke;pdf` 标签筛选执行。

2026-05-15 继续推进（PDF CTest 超时 helper 收口）：

- 已把 `featherdoc_set_test_labels()` 扩展为：凡标签列表包含 `pdf` 的 C++ CTest，
  默认继承 `TIMEOUT 60`，避免后续新增 PDF smoke 测试时只补标签、漏补超时。
- 已移除 `pdf_cli_export`、`pdf_cli_import` 和 `pdf_regression_manifest` 的重复手写
  超时设置，让 PDF C++ 测试的调度边界统一由标签 helper 承担。
- 仍保留 PowerShell、动态 PDF regression 和视觉 gate 的显式 `TIMEOUT 60`，
  因为这些测试不是通过 `featherdoc_set_test_labels()` 注册。

2026-05-15 继续推进（PDF CTest 超时契约回归）：

- 已新增 `pdf_ctest_timeout_contract_test.ps1`，直接扫描生成后的
  `CTestTestfile.cmake`，要求所有 `LABELS` 包含 `pdf` 的测试都带 bounded
  `TIMEOUT`；默认仍为 `60`，只有已量测的聚合入口可以显式登记例外。
- 已把该契约挂入 CTest，标签为 `pdf;ctest;smoke`，自身也显式设置 60 秒超时。
- 该回归覆盖 C++ helper、PowerShell 文档契约、视觉 gate 和动态 regression 测试，
  避免后续新增 PDF 测试时绕过统一调度边界。

2026-05-15 继续推进（CJK 常见阅读器手工验收入口）：

- 已将 CJK/PDF 面向发布的手工复制、搜索验收纳入 `REVIEWER_CHECKLIST.md` 生成流程：
  发布 reviewer 需要在至少一个常见阅读器中确认生成的中文 PDF 可以复制和搜索，并在 release notes
  或 final review 中记录阅读器和版本。
- 已补 `release_note_bundle_version_test.ps1` 契约，固定该 checklist 文案，避免后续 release bundle
  重构时遗漏人工验收入口。
- 验证过程中同步加固 `release_note_bundle_version_test.ps1` 的 UTF-8 断言读取，并将中文期望值改为码点构造，
  让该回归在 Windows PowerShell 5.1 读取无 BOM 脚本时也能稳定执行。
- 自动化代理验收与人工验收边界保持分离：PDFium 回读、`/ToUnicode`、`/Identity-H` 和中文搜索片段回归继续覆盖
  可自动验证的语义代理；常见阅读器复制/搜索仍作为发布前人工签核项，不标记为自动完成。

2026-05-15 继续推进（PDF import 文档拆页与安装闭环）：

- 已将 `docs/pdf_import.rst` 收敛为 PDF import 用户总览页，把字段级 JSON 契约迁移到
  `docs/pdf_import_json_diagnostics.rst`，把支持范围和限制迁移到 `docs/pdf_import_scope.rst`。
- 已同步 `docs/index.rst` hidden toctree、README 中英入口、`BUILDING_PDF.md` 开发者说明和
  CMake 安装清单，确保源码文档入口与安装包文档入口一致。
- 已扩展 `pdf_import_docs_contract_test.ps1` 和 `package_release_assets_safety_test.ps1`：
  前者固定拆页、README 指向和 CMake 安装规则，后者固定发布 ZIP 内必须携带三个 PDF import 文档页。
- 本轮仍是文档和发布打包契约收口，不改变 PDF import 行为；继续 E7 时可以优先补 CLI diagnostics
  的用户示例或更复杂导入负样本。

2026-05-15 继续推进（PDF import CLI diagnostics 用户示例）：

- 已在 `docs/pdf_import_json_diagnostics.rst` 增加常见 continuation blocker 示例，覆盖
  `repeated_header_mismatch`、`column_anchors_mismatch` 和
  `continuation_confidence_below_threshold` 三类用户最容易遇到的拆表原因。
- 已扩展 `pdf_import_docs_contract_test.ps1`，固定这些 JSON 片段和
  `minimum_continuation_confidence` 示例，避免后续文档拆分或重写时丢失用户可诊断入口。
- 本轮只把现有 CLI 测试已经覆盖的诊断语义写入用户文档，不改变 importer 或 CLI JSON 输出。

2026-05-15 继续推进（PDF import blocker CLI 覆盖补齐）：

- 已补 `pdf_cli_import_tests.cpp` 的 CLI JSON 回归，覆盖
  `not_near_page_top` 和 `not_first_block_on_page` 两个此前只有 importer 层断言的 continuation blocker。
- 已同步 `docs/pdf_import_json_diagnostics.rst`，增加页顶距离过低和非页内首个 block 的用户可读示例。
- 已扩展 `pdf_import_docs_contract_test.ps1`，固定这两类 blocker 的 JSON 片段，避免 CLI 层诊断文档漂移。
- 剩余 `inconsistent_source_rows` 目前仍缺少稳定 PDF fixture；继续 E7 时应优先判断是否需要新增专门样本，
  或把该 blocker 保持为内部保守兜底并在文档中明确触发条件。

2026-05-15 继续推进（PDF import blocker 覆盖契约）：

- 已将 `inconsistent_source_rows` 在 `docs/pdf_import_json_diagnostics.rst` 中明确标记为内部一致性兜底：
  当前 PDF parser 按 column anchors 预填 rows，正常用户样本不应稳定触发该 blocker。
- 已扩展 `pdf_import_docs_contract_test.ps1`，从 `PdfTableContinuationBlocker` enum 和
  `pdf_cli_import_tests.cpp` 的 JSON 断言反向校验：除 `none` 与内部兜底
  `inconsistent_source_rows` 外，每个 blocker 都必须有 CLI JSON 覆盖。
- 这样后续如果新增 continuation blocker，测试会要求同步 CLI 层可见覆盖，避免只改 importer 内部映射。

2026-05-15 继续推进（PDF import parse-error JSON 文档）：

- 已在 `docs/pdf_import_json_diagnostics.rst` 增加 `Command-line parse errors` 小节，
  明确 `--json` 下参数校验失败使用 `stage:"parse"` 和 `message` 字段。
- 已记录 `--min-table-continuation-confidence` 缺值、非法值、重复值三类当前 CLI 已覆盖的错误消息。
- 已扩展 `pdf_import_docs_contract_test.ps1` 固定这些 parse-error 文档片段，避免 CLI 负路径 JSON 契约只留在测试里。

2026-05-15 继续推进（PDF import 文档 JSON 示例解析契约）：

- 已扩展 `pdf_import_docs_contract_test.ps1`，解析 `docs/pdf_import_json_diagnostics.rst` 中全部
  `.. code-block:: json` 示例并逐个执行 `ConvertFrom-Json`。
- 该契约确保后续补充 success、failure、continuation blocker 或 parse-error 示例时，文档里的 JSON 片段本身保持可解析。

2026-05-15 继续推进（PDF CTest 标签契约）：

- 已新增 `pdf_ctest_label_contract_test.ps1`，扫描生成后的 `CTestTestfile.cmake`，要求所有 `pdf`
  标签测试同时带 `smoke` 标签。
- 同一契约还固定 `pdf_cli_*` 测试必须带 `cli;smoke;pdf`，把 PDF CLI import/export 作为轻量烟测入口的约定落到回归。
- 该测试自身注册为 `pdf;ctest;smoke`，并显式设置 `TIMEOUT 60`，继续遵守 PDF 后台测试调度边界。

2026-05-15 继续推进（PDF import parse-error 输出安全）：

- 已增强 `pdf_cli_import_tests.cpp` 的 parse-error 回归：`--min-table-continuation-confidence`
  缺值、非法值、重复值三类错误在返回 `stage:"parse"` JSON 时，都必须不写目标 DOCX。
- 已同步 `docs/pdf_import_json_diagnostics.rst`，明确 parse errors do not write the target DOCX，
  并用 `pdf_import_docs_contract_test.ps1` 固定该用户可见契约。

2026-05-15 继续推进（PDF import README 与安装入口契约）：

- 已同步 `README.md` 和 `README.zh-CN.md` 的 PDF import 小节，明确总览、
  JSON diagnostics、supported scope 三份拆页文档的入口，避免只在安装清单里隐式出现。
- 已扩展 `pdf_import_docs_contract_test.ps1`，从 README 的 PDF import 小节范围内断言
  `docs/pdf_import.rst`、`docs/pdf_import_json_diagnostics.rst` 和
  `docs/pdf_import_scope.rst` 都可见。
- 同一契约现在还会解析 CMake `install(FILES ... DESTINATION ...)` 块，要求三份 PDF import
  用户文档安装到同一个 `${FEATHERDOC_INSTALL_DATADIR}/docs` 目录，而不是只做全文件弱匹配。

2026-05-15 继续推进（PDF import CLI usage 契约）：

- 已让 `cli_usage_tests` 在 PDF import 构建中同步定义 `FEATHERDOC_CLI_ENABLE_PDF_IMPORT`，
  避免测试目标单独编译 `featherdoc_cli_usage.cpp` 时漏测真实 CLI 的 `import-pdf` usage。
- 已扩展 `cli_usage_tests.cpp`，固定 `import-pdf` 命令行、`--import-table-candidates-as-tables`、
  `--min-table-continuation-confidence <score>`、`table_continuation_diagnostics` 和
  `min_table_continuation_confidence` 的帮助文本入口。
- 已同步 `featherdoc_cli_usage.cpp`，说明设置 continuation confidence 后 JSON 会记录
  `min_table_continuation_confidence`，让 CLI help 与 JSON diagnostics 文档保持一致。

2026-05-15 继续推进（PDF import toctree 契约）：

- 已扩展 `pdf_import_docs_contract_test.ps1`，解析 RST `.. toctree::` 条目并断言
  `pdf_import`、`pdf_import_json_diagnostics` 和 `pdf_import_scope` 都在文档目录中。
- 该检查比单纯文本包含更严格，避免后续正文仍引用页面但 Sphinx 导航目录漏挂拆页文档。

2026-05-15 继续推进（PDF import scope 覆盖锚点契约）：

- 已扩展 `pdf_import_docs_contract_test.ps1`，把 `docs/pdf_import_scope.rst` 的支持范围、
  保守拆表边界和段落保守分类声明映射到代表性测试锚点。
- 当前契约会检查纯文本导入、默认拒绝表格候选、表格 opt-in、key-value / borderless 表格、
  跨页合并、repeated-header 各类匹配、subtotal 行、continuation diagnostics、列数 /
  anchor / semantic header / confidence / intervening paragraph / 低页位拆表，以及两栏正文、
  编号列表、短标签正文和 free-form column drift 保持段落的测试覆盖。
- 这样后续若扩展或重写 scope 文档，必须同时保留可追溯的测试覆盖入口，避免范围说明脱离回归样本。

2026-05-15 继续推进（PDF import 发布包内容入口契约）：

- 已增强 `package_release_assets_safety_test.ps1` 的安装包桩文档，让 `pdf_import.rst`
  同时引用 `pdf_import_json_diagnostics` 与 `pdf_import_scope`，更贴近真实拆页入口关系。
- 发布包 safety 回归现在会在 staging 后确认三份 PDF import 文档不仅存在，还保留总览页到
  JSON diagnostics / scope 页的入口、JSON parse-error 小节，以及 scope 页的支持范围标题。

2026-06-14 继续推进（PDF import diagnostics 契约闭环）：

- 已补强 importer 层 continuation diagnostics 回归，固定
  `source_row_offset`、`continuation_confidence`、`minimum_continuation_confidence`、
  `header_match_kind`、`blocker`、`disposition` 以及关键布尔判定字段，覆盖合并、
  阈值阻断、列锚点不匹配、页顶距离过低、中间段落重置和非页内首个 block 等路径。
  当前 `dev` 还固定了 diagnostics 数组首个 table candidate 的
  `created_new_table` / `no_previous_table` 契约，以及阈值参数在首条 diagnostic
  上的 `minimum_continuation_confidence` 记录。
- 已补强 CLI / 文档契约：CLI JSON 回归确认关键 diagnostic 字段可被用户看到；
  当前 `dev` 进一步固定 `table_continuation_diagnostics` 中首个新建表诊断与续页合并
  诊断的完整 JSON object 顺序；
  `pdf_import_docs_contract_test.ps1` 反向固定 CLI command / output writer 必须写出
  `table_continuation_diagnostics_count`、`table_continuation_diagnostics` 和每个
  diagnostic object key。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 120`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_cli_import$" --output-on-failure --timeout 120`
  通过；
  `git diff --check` 通过。
- 已提交并推送：
  `6e3255f test: lock PDF import continuation diagnostics` 和
  `9b824e4 test: expose PDF import CLI diagnostics contract`。
- 已知边界：
  本轮不改变 importer 行为或 CLI JSON schema，不新增视觉 gate 产物；
  `inconsistent_source_rows` 继续作为内部一致性兜底，暂不新增用户稳定 fixture。
- 详情见：
  `design/04-pdf-execution-plan_e7_cli_docs_diagnostics.md`。

2026-06-14 继续推进（PDF import diagnostics JSON 示例契约）：

- 已将 `docs/en/api/pdf_workflow.rst` 与 `docs/zh-CN/api/pdf_workflow.rst`
  的成功 JSON 示例从空 `table_continuation_diagnostics` 数组改为两个真实风格
  diagnostic object：首个候选 `created_new_table` / `no_previous_table`，续页候选
  `merged_with_previous_table` / `none`。
- 示例现在显式展示用户可见字段：
  `page_index`、`block_index`、`source_row_offset`、`continuation_confidence`、
  `minimum_continuation_confidence`、所有 continuation 布尔判定、
  `header_match_kind`、`skipped_repeating_header`、`disposition` 和 `blocker`。
- 已同步 `pdf_import_docs_contract_test.ps1`，固定 diagnostics 数组形态、false/true
  状态、合并 disposition、`no_previous_table` 与 `none` blocker，并继续要求 RST JSON
  code block 可被 `ConvertFrom-Json` 解析。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_import_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已知边界：
  本轮只同步公开文档示例与 docs contract，不改变 importer 决策、不改变 CLI JSON schema，
  不新增或提交 `output/` 视觉 gate 产物。

2026-06-14 继续推进（PDF import image-only 负样本契约）：

- 已新增 image-only placeholder PDF fixture：页面包含矩形和线条图形，但不包含 text run，
  用于模拟 scanned / image-only 页面对 PDFium 可见但没有可提取文字的边界。
- 已扩展 `pdf_import_failure_tests.cpp`，固定该类非空页面导入失败时返回
  `PdfDocumentImportFailureKind::no_text_paragraphs`，错误信息继续提示
  `text paragraphs only`。
- 已同步 `pdf_import_docs_contract_test.ps1` 的 scope 覆盖锚点，将公开文档中的
  `scanned PDFs`、`OCR` 和 `image-only` 声明绑定到该负样本回归，避免 unsupported
  scope 只停留在文档文字。
- 已完成验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_failure_tests` 通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_import_failure|pdf_import_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已知边界：
  该 fixture 只固定 no-text/image-only 失败分类，不引入 OCR，不改变 importer 行为，
  不新增或提交 `output/` 视觉 gate 产物。

2026-06-14 继续推进（PDF import image-only CLI failure JSON 契约）：

- 已扩展 `pdf_cli_import_threshold_tests.cpp`，用 image-only placeholder fixture 覆盖
  `featherdoc_cli import-pdf --json` 失败路径，固定输出包含 `ok:false`、
  `stage:"import"`、`failure_kind:"no_text_paragraphs"`、`input` 和 `output`。
- 已同步 `docs/en/api/pdf_workflow.rst` 与 `docs/zh-CN/api/pdf_workflow.rst`，
  说明没有可提取文本段落的 scanned / image-only PDF 会报告 `no_text_paragraphs`，
  并且不会写出目标 DOCX。
- 已增强 `pdf_import_docs_contract_test.ps1`，反查 `pdf_cli_import*.cpp` 必须覆盖
  `table_candidates_detected` 与 `no_text_paragraphs` 两类 failure JSON，避免
  `failure_kind` 只在 writer 映射或文档枚举中存在。
- 已完成验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_cli_import_tests` 通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_cli_import|pdf_import_failure|pdf_import_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已知边界：
  本轮不改变 CLI JSON schema，只固定现有 failure kind 的用户可见输出；不新增或提交
  `output/` 视觉 gate 产物。

2026-06-14 继续推进（PDF import diagnostics 轻量 visual gate 前置契约）：

- 已同步 `docs/pdf_release_readiness_checklist_zh.rst`，把 PDF import diagnostics /
  no-text 边界纳入 bounded `smoke-import` 子集说明：`pdf_cli_import` 固定用户可见
  `table_continuation_diagnostics` 与 `failure_kind = no_text_paragraphs` JSON，
  `pdf_import_failure` 固定 image-only / no-text 负样本不写出目标 DOCX。
- 已扩展 `pdf_visual_validation_status_docs_contract_test.ps1`，固定
  `pdf_import_smoke_diagnostics_release_trace`、`table_continuation_diagnostics`、
  `failure_kind = no_text_paragraphs` 和 `image-only / no-text` 必须保留在 release
  readiness checklist 中。
- 已知边界：
  本轮只补轻量 visual gate 前置说明与文档契约，不运行 full visual gate，不新增或提交
  `output/` 视觉产物。

2026-06-15 继续推进（PDF import inconsistent source rows 边界）：

- 已确认当前 `build_table_candidate()` 会按检测到的 column anchors 为候选表每一行预填
  cells，正常 public import 路径不应稳定触发 `inconsistent_source_rows`。
- 因此本轮不新增不稳定 PDF fixture，而是把该 blocker 固定为 importer 内部一致性兜底：
  双语 `pdf_workflow.rst` 已说明正常导入不应依赖它作为稳定用户可触发诊断，
  `pdf_import_docs_contract_test.ps1` 同步固定该边界。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_import_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已知边界：
  本轮不改变 importer 决策、不改变 CLI JSON schema，不新增或提交 `output/` 视觉产物。

2026-06-15 继续推进（PDF import diagnostics 字段顺序与 bounded preflight 契约）：

- 已把 bounded `smoke-import` 子集从“文档说明”推进为可回归 summary 契约：
  `run_pdf_ctest_bounded_subset.ps1` 在 `smoke-import` summary 中写出
  `bounded_smoke_import_preflight` scope、`does_not_replace_full_visual_gate_verdict`
  boundary、`does_not_generate_or_commit_output_visual_artifacts` artifact policy、
  诊断契约测试列表和诊断字段列表。
- 已新增 `pdf_ctest_bounded_subset_summary_test.ps1` 并注册到 CTest，固定 fake
  `smoke-import` summary 的字段值和 10 个 smoke/import 测试顺序；同时修复
  `pdf_cli_export_tests.cpp` 的重复 binary helper 定义，确保 bounded 子集能完整跑通。
- 已把 `table_continuation_diagnostics` 的 per-diagnostic JSON 字段顺序写入
  `docs/en/api/pdf_workflow.rst` 与 `docs/zh-CN/api/pdf_workflow.rst`，并让
  `pdf_import_docs_contract_test.ps1` 同时校验英文示例、中文示例和 CLI output writer
  的字段顺序，避免用户文档示例和实际 CLI JSON drift。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_import_docs_contract|pdf_visual_validation_status_docs_contract" --output-on-failure --timeout 120`
  通过；
  `powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 -Subset smoke-import -BuildDir .\.bpdf-roundtrip-msvc -OutputJson .\build\pdf-ctest-bounded-subset-current\summary.json`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_ctest_bounded_subset_summary|pdf_visual_validation_status_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `bd4ab81 test: expose PDF import bounded visual gate contract`、
  `0c13f79 test: lock PDF bounded import gate summary` 和
  `d4bdda9 docs: lock PDF import diagnostic field order`。
- 已知边界：
  本轮不改变 importer 决策、不改变 CLI JSON schema；bounded preflight 不替代 full
  visual gate verdict；不新增或提交 `output/` 视觉 gate 产物。

2026-06-15 继续推进（PDF import prose / form 负样本契约）：

- 已补强 importer 层负样本回归：`PDF text importer keeps short-label prose as paragraphs`
  和 `PDF text importer keeps invoice summary form as paragraphs` 断言即使开启
  `--import-table-candidates-as-tables`，短标签两列 prose 与 invoice summary 表单仍保持
  `tables_imported = 0`，保存重开后也不会产生 DOCX table。
- 已同步 `pdf_import_docs_contract_test.ps1`，让公开 scope 文档中的 `short-label prose`
  与 `free-form forms` 边界追到 importer 级测试，而不只停留在 parser candidate 层。
- 已完成验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_import_table_heuristic|pdf_import_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `0d7efc2 test: lock PDF import prose negative boundaries`。
- 已知边界：
  本轮不改变 importer 启发式、不新增 CLI JSON 字段、不运行 full visual gate；负样本只固定
  text-first importer 的保守结构行为，不新增或提交 `output/` 视觉产物。

2026-06-15 继续推进（PDF import prose / form CLI 用户可见契约）：

- 已把上一批 importer 负样本继续推进到 CLI 用户可见 JSON：新增
  `cli import-pdf keeps prose and forms as paragraphs with table promotion`，
  覆盖 short-label prose 与 invoice summary form 两个样本。
- 该 CLI 回归固定用户开启 `--import-table-candidates-as-tables --json` 后仍能看到
  成功导入结果：`ok = true`、`tables_imported = 0`、
  `table_continuation_diagnostics_count = 0`、`table_continuation_diagnostics = []`
  和 `import_table_candidates_as_tables = true`，同时保存后的 DOCX 不包含 table。
- 已完成验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_cli_import_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_cli_import|pdf_import_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `f027db1 test: expose PDF import prose CLI contract`。
- 已知边界：
  本轮不改变 importer 启发式和 CLI JSON schema，只固定用户可见的保守导入结果；
  未运行 full visual gate，不新增或提交 `output/` 视觉产物。

2026-06-15 继续推进（PDF bounded smoke-import prose / form preflight 契约）：

- 已把 prose/form 负样本继续纳入 bounded `smoke-import` summary：
  `scripts/run_pdf_ctest_bounded_subset.ps1` 现在在 `smoke-import` summary 中写出
  `table_continuation_diagnostics=[]`、`tables_imported=0`、
  `import_table_candidates_as_tables=true`，并新增
  `import_negative_boundary_contract_cases`。
- 已固定 `import_negative_boundary_contract_cases` 的两个 case：
  `short_label_prose_remains_paragraphs` 和
  `invoice_summary_form_remains_paragraphs`，把 importer -> CLI -> bounded preflight
  串成可回归闭环。
- 已同步 `docs/pdf_release_readiness_checklist_zh.rst` 与
  `pdf_visual_validation_status_docs_contract_test.ps1`，让发布清单和 docs contract
  同时固定这些 bounded preflight 字段。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_ctest_bounded_subset_summary|pdf_visual_validation_status_docs_contract" --output-on-failure --timeout 120`
  通过；
  `powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 -Subset smoke-import -BuildDir .\.bpdf-roundtrip-msvc -OutputJson .\build\pdf-ctest-bounded-subset-current\summary.json`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `26304e4 test: extend PDF bounded import prose contract`。
- 已知边界：
  bounded `smoke-import` 只证明资源受限窗口里的 import diagnostics / prose-form
  preflight，不替代 full visual gate verdict；本轮不新增或提交 `output/` 视觉产物。

2026-06-15 继续推进（PDF import diagnostics governance source-report 契约）：

- 已把 bounded CTest summary 中的 import diagnostics 契约继续传递到 release
  governance 可见层：`release_candidate_checks` 聚合
  `import_diagnostics_contract_tests`、`import_diagnostics_contract_fields` 和
  `import_negative_boundary_contract_cases`，`release_blocker_rollup` 再展开为
  `pdf_bounded_ctest_import_diagnostics_contract_tests`、
  `pdf_bounded_ctest_import_diagnostics_contract_fields` 和
  `pdf_bounded_ctest_import_negative_boundary_contract_cases`。
- 已固定 `release_governance_handoff.md` 的同一个 `source_report:` block 继续展示
  bounded CTest 计数、summary 路径、diagnostics contract fields 和 prose/form 负样本
  case，避免这些证据只停留在内部对象或 detached notes。
- 已同步 `BUILDING_PDF.md`、`docs/pdf_release_readiness_checklist_zh.rst` 和
  `pdf_visual_validation_status_docs_contract_test.ps1`，让治理报告字段、发布清单和
  文档契约保持一致。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "release_candidate_visual_verdict|build_release_governance_handoff_report_include_rollup|pdf_visual_validation_status_docs_contract" --output-on-failure --timeout 120`
  中 `release_candidate_visual_verdict`、
  `release_candidate_visual_verdict_reports`、
  `release_candidate_visual_verdict_contract` 和
  `build_release_governance_handoff_report_include_rollup` 通过；补齐
  `BUILDING_PDF.md` 后，
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_visual_validation_status_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `edf08c0 test: expose PDF import diagnostics governance contract`。
- 已知边界：
  该闭环仍是 bounded preflight / governance handoff 证据，不替代 full visual gate
  verdict；本轮不改变 importer 启发式、不新增 CLI JSON schema 字段、不提交
  `output/` 或 build 产物。

2026-06-15 继续推进（PDF repeated-header subtotal CLI diagnostic object 契约）：

- 已补 `pdf_cli_import_tests.cpp` 本地 helper，固定 repeated-header 续页合并时的完整
  `table_continuation_diagnostics` JSON object，覆盖字段顺序和值：
  `source_row_offset = 1`、`continuation_confidence = 95`、
  `minimum_continuation_confidence = 0`、`has_previous_table = true`、
  `is_first_block_on_page = true`、`is_near_page_top = true`、
  `source_rows_consistent = true`、`column_count_matches = true`、
  `column_anchors_match = true`、`previous_has_repeating_header = true`、
  `source_has_repeating_header = true`、`header_matches_previous = true`、
  `header_match_kind = exact`、`skipped_repeating_header = true`、
  `disposition = merged_with_previous_table` 和 `blocker = none`。
- 已把该完整 object 契约应用到三个用户可见 CLI subtotal 场景：
  missing-unit、sparse-body 与 amount-only body，防止后续只保留散点字段但破坏
  JSON object 顺序或关键诊断值。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_cli_import" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `631239e test: lock PDF repeated header CLI diagnostics`。
- 已知边界：
  本轮只收紧 CLI JSON 诊断契约，不改变 importer merge heuristic、不新增 CLI JSON
  schema 字段，不运行 full visual gate，也不新增或提交 `output/` 视觉产物。

2026-06-15 继续推进（PDF repeated-header diagnostics 公开 workflow 契约）：

- 已同步 `docs/en/api/pdf_workflow.rst` 与 `docs/zh-CN/api/pdf_workflow.rst`，
  将 repeated-header 续页合并诊断从 CLI 回归证据沉淀到用户可见 API workflow 文档：
  `source_row_offset = 1` 表示跳过续页首行重复表头，
  `skipped_repeating_header = true`、`continuation_confidence = 95`、
  `header_match_kind = exact`、`disposition = merged_with_previous_table` 和
  `blocker = none` 固定为该受控合并路径的公开说明。
- 已用 `pdf_import_docs_contract_test.ps1` 固定中英文 workflow 文档必须保留上述
  字段值和 JSON 片段；PowerShell contract marker 保持 ASCII-only，避免
  Windows PowerShell 5.1 对无 BOM UTF-8 脚本文字量的解析漂移。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_import_docs_contract|docs_bilingual_entrypoints_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `a22352c docs: document PDF repeated header diagnostics contract`。
- 已知边界：
  本轮只补用户可见 workflow 文档契约，不改变 importer continuation heuristic、
  CLI JSON schema 或 release bundle 内容，也不新增或提交 `output/` 视觉产物。

2026-06-15 继续推进（PDF import blocker diagnostic object 契约）：

- 已把 `pdf_cli_import_tests.cpp` 中的拆表负样本从散点字段断言升级为完整
  `table_continuation_diagnostics` object 契约，覆盖
  `repeated_header_mismatch`、`column_anchors_mismatch`、
  `continuation_confidence_below_threshold` 和 `column_count_mismatch` 四类用户可见
  blocker。
- 每个 object 均固定字段顺序和值，包括 `source_row_offset = 0`、
  `has_previous_table = true`、页顶判定、列数/列锚点判定、repeated-header 判定、
  `skipped_repeating_header = false`、`disposition = created_new_table` 以及对应
  blocker；同时固定规则型 `continuation_confidence` 分数：70、55、85/90 和 30。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_cli_import" --output-on-failure --timeout 120`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_import_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `f400970 test: lock PDF import blocker diagnostics`。
- 已知边界：
  本轮只锁定现有 CLI blocker JSON 展示契约，不改变 importer continuation heuristic、
  blocker 枚举或用户文档 schema，也不新增视觉产物。

2026-06-15 继续推进（PDF import blocker diagnostics 公开 workflow 契约）：

- 已把 `docs/en/api/pdf_workflow.rst` 与 `docs/zh-CN/api/pdf_workflow.rst`
  的代表性 continuation blocker 片段从最小字段扩展为完整 diagnostic object，
  让用户文档与 `pdf_cli_import_tests.cpp` 中已锁定的 CLI JSON object 契约对齐。
- 公开文档现在覆盖 `repeated_header_mismatch`、`column_count_mismatch`、
  `column_anchors_mismatch` 和 `continuation_confidence_below_threshold` 四类拆表场景，
  并展示 `page_index`、`block_index`、`source_row_offset = 0`、页顶判定、
  列数/列锚点判定、repeated-header 判定、`skipped_repeating_header = false`、
  `disposition = created_new_table`、对应 blocker 与规则型 confidence 分数。
- `pdf_import_docs_contract_test.ps1` 已固定这些公开片段必须保留
  `continuation_confidence` 的 70、55、85、30 四个值，以及
  `column_count_mismatch`、`column_count_matches = false`、
  `column_anchors_match = false` 等关键用户诊断入口。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_import_docs_contract|docs_bilingual_entrypoints_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `8cc196d docs: document PDF import blocker diagnostics contract`。
- 已知边界：
  本轮只补公开 workflow 文档与 docs contract，不改变 CLI JSON schema、importer
  blocker 判断或 release bundle 内容，也不新增视觉产物。

2026-06-15 继续推进（PDF import blocker diagnostics rollup 字段链路）：

- 已把 `scripts/run_pdf_ctest_bounded_subset.ps1` 的
  `smoke-import.import_diagnostics_contract_fields` 从基础 import 字段扩展到完整
  blocker diagnostic object 的关键用户可见值，确保后续
  release candidate summaries、release blocker rollup 和 governance handoff 继续通过
  既有转发链路暴露这些字段。
- 新增字段包括 `source_row_offset=0`、`skipped_repeating_header=false`、
  `disposition=created_new_table`、四类 blocker、`continuation_confidence=70/55/85/30`、
  `minimum_continuation_confidence=90`、`column_count_matches=false` 和
  `column_anchors_match=false`。
- 已同步 `docs/pdf_release_readiness_checklist_zh.rst` 与
  `pdf_visual_validation_status_docs_contract_test.ps1`，固定 release readiness 文档必须
  保留这些 import diagnostics preflight 字段；`pdf_ctest_bounded_subset_summary_test.ps1`
  同步固定 summary JSON 的字段顺序。
- 已完成验证：
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File test\pdf_ctest_bounded_subset_summary_test.ps1 -RepoRoot . -WorkingDir .bpdf-roundtrip-msvc\test\pdf_ctest_bounded_subset_summary_direct`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_visual_validation_status_docs_contract|release_candidate_visual_verdict_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 验证备注：
  较宽的
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_ctest_bounded_subset_summary|pdf_visual_validation_status_docs_contract|release_candidate_visual_verdict" --output-on-failure --timeout 120`
  因 `pdf_ctest_bounded_subset_summary` 60s CTest property 和
  `release_candidate_visual_verdict*` 长链路触发 timeout；未出现字段断言失败，已改用上述
  聚焦 contract 验证。
- 已提交并推送：
  `e560756 test: expose PDF import blocker diagnostics rollup fields`。
- 已知边界：
  本轮只扩展 bounded summary / release readiness 的字段链路，不改变 importer、CLI JSON
  schema、release bundle 渲染模板或 full visual gate verdict。

2026-06-15 继续推进（PDF import blocker diagnostics Markdown rollup 可见性）：

- 已补 `build_release_governance_handoff_report_include_rollup` fixture 与断言，确认
  `pdf_bounded_ctest_import_diagnostics_contract_fields` 中的完整 blocker diagnostic
  字段不只进入 nested rollup summary JSON，也会出现在 release governance handoff
  Markdown 的 `source_report:` block 中。
- Markdown 契约现在固定 `source_row_offset=0`、`skipped_repeating_header=false`、
  `disposition=created_new_table`、四类 blocker、
  `continuation_confidence=70/55/85/30`、`minimum_continuation_confidence=90`、
  `column_count_matches=false` 和 `column_anchors_match=false`，避免后续
  release-facing Markdown 只保留字段名而丢失用户可诊断的 blocker 依据。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "build_release_governance_handoff_report_include_rollup" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `28231cf test: lock PDF import blocker diagnostics markdown rollup`。
- 已知边界：
  本轮只锁 release/governance Markdown 输出可见性，不改变 importer、CLI JSON schema、
  bounded summary 生成逻辑或 full visual gate verdict；未生成或提交 `output/` 视觉产物。

2026-06-15 继续推进（PDF import diagnostics release candidate 字段契约）：

- 已补 `release_candidate_visual_verdict_contract` 的 bounded CTest fixture 与
  candidate summary 断言，确认完整 import diagnostics contract fields 能从
  bounded CTest summary 聚合到 release candidate summary 的
  `steps.pdf_bounded_ctest.import_diagnostics_contract_fields`。
- 新增断言固定 `source_row_offset=0`、`skipped_repeating_header=false`、
  `disposition=created_new_table`、四类 blocker、
  `continuation_confidence=70/55/85/30`、`minimum_continuation_confidence=90`、
  `column_count_matches=false` 和 `column_anchors_match=false`，补上 bounded summary
  与 release/governance rollup 之间的 release candidate 中间层契约。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "release_candidate_visual_verdict_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `a92cea4 test: lock PDF import diagnostics release candidate fields`。
- 已知边界：
  本轮只补 release candidate summary 的字段保留契约，不改变 visual verdict 判定、
  importer heuristic、CLI JSON schema 或 release governance Markdown writer。

2026-06-15 继续推进（PDF import diagnostics 契约字段 helper 收敛）：

- 已新增 `test/pdf_import_diagnostics_contract_field_helpers.ps1`，将完整
  `import_diagnostics_contract_fields` 清单和 blocker-only Markdown 断言清单集中到
  一个测试 helper，避免 release candidate 与 governance handoff 测试继续复制
  `source_row_offset=0`、四类 blocker、confidence 分数和 column/header 判定字段。
- 已让 `release_candidate_visual_verdict_contract_checks.ps1`、
  `release_candidate_visual_verdict_candidate_core_assertions.ps1`、
  `build_release_governance_handoff_report_include_rollup_setup.ps1`、
  `build_release_governance_handoff_report_include_rollup_summary_assertions.ps1` 和
  `build_release_governance_handoff_report_include_rollup_markdown_assertions.ps1`
  复用该 helper，同时保留原有 summary JSON / Markdown source_report block 的可见性断言。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "build_release_governance_handoff_report_include_rollup|release_candidate_visual_verdict_contract" --output-on-failure --timeout 120`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^release_candidate_visual_verdict$" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `c5f03f6 test: share PDF import diagnostics contract fields`。
- 已知边界：
  本轮只收敛测试 helper，不改生产脚本、bounded summary 字段来源、CLI JSON schema、
  importer heuristic 或 release visual verdict。

2026-06-15 继续推进（PDF import diagnostics helper 覆盖 bounded/docs contract）：

- 已让 `pdf_ctest_bounded_subset_summary_test.ps1` 复用
  `Get-PdfImportDiagnosticsContractFields`，继续以顺序断言固定
  `smoke-import.import_diagnostics_contract_fields`，但不再在该测试内复制完整字段清单。
- 已让 `pdf_visual_validation_status_docs_contract_test.ps1` 复用同一 helper 拼接
  release readiness checklist、bounded CTest script marker 和 paragraph block 的
  import diagnostics 字段断言，确保文档/脚本可见性与 bounded summary / release candidate /
  governance handoff 使用同一份测试字段清单。
- 已完成验证：
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File test\pdf_ctest_bounded_subset_summary_test.ps1 -RepoRoot . -WorkingDir .bpdf-roundtrip-msvc\test\pdf_ctest_bounded_subset_summary_direct`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_visual_validation_status_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `97645b5 test: reuse PDF import diagnostics contract fields`。
- 已知边界：
  本轮只扩展测试 helper 的使用面，不改变 `scripts/run_pdf_ctest_bounded_subset.ps1`
  的字段输出、不改 docs 内容、不改 CLI JSON schema 或 importer heuristic。

2026-06-15 继续推进（PDF import diagnostics 生产脚本字段 helper）：

- 已新增 `scripts/pdf_import_diagnostics_contract_fields.ps1`，将 bounded
  `smoke-import.import_diagnostics_contract_fields` 的生产字段清单从
  `run_pdf_ctest_bounded_subset.ps1` 抽出，避免生产脚本继续内嵌完整 blocker
  diagnostics 字段列表。
- `scripts/run_pdf_ctest_bounded_subset.ps1` 现在 dot-source 该 helper 并通过
  `Get-PdfImportDiagnosticsContractFields` 写入 summary；`pdf_ctest_bounded_subset_summary_test.ps1`
  仍用测试侧 helper 固定精确输出顺序和值，因此脚本 helper 变更仍会被 bounded
  summary contract 捕获。
- `pdf_visual_validation_status_docs_contract_test.ps1` 同步把
  `scripts/pdf_import_diagnostics_contract_fields.ps1` 纳入 bounded CTest script marker
  聚合，保持 release readiness/docs contract 对生产字段来源的可见性。
- 已完成验证：
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File test\pdf_ctest_bounded_subset_summary_test.ps1 -RepoRoot . -WorkingDir .bpdf-roundtrip-msvc\test\pdf_ctest_bounded_subset_summary_direct`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_visual_validation_status_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过（仅保留既有 CRLF/LF warning）。
- 已提交并推送：
  `09ffea5 test: share bounded PDF import diagnostics fields`。
- 已知边界：
  本轮只抽出生产字段清单，不改变 summary JSON 字段值、字段顺序、CLI JSON schema、
  importer heuristic 或 release visual verdict。

2026-06-15 继续推进（PDF import diagnostics release metadata 透传契约）：

- 已补 `release_visual_verdict_metadata_consistency_test.ps1` 的动态契约，直接
  dot-source `release_visual_metadata_helpers.ps1` 并构造 `pdf_bounded_ctest`
  summary，确认 `Get-PdfBoundedCtestEvidence` 对
  `import_diagnostics_contract_fields` 只做原样数组透传，不过滤、不重排。
- 同一测试还固定 `Get-PdfBoundedCtestImportDiagnosticsDisplay` 的 `fields` 展示顺序
  必须与 `Get-PdfImportDiagnosticsContractFields` 一致，避免 release note / artifact
  writer 使用 display helper 时丢失 blocker 诊断字段顺序。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "release_visual_verdict_metadata_consistency" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `f3c6dcc test: lock PDF import diagnostics metadata passthrough`。
- 已知边界：
  本轮只锁 release metadata helper 的透传行为，不改变 release note 文案、
  package assets、governance handoff Markdown、bounded summary JSON 或 importer 逻辑。

2026-06-15 继续推进（PDF import diagnostics release report 字段契约）：

- 已补 `release_candidate_visual_verdict_candidate_report_assertions.ps1`，把 release
  candidate generated reports 中的 import diagnostics 字段断言从前三个样例字段升级为
  复用 `Get-PdfImportDiagnosticsContractFields` 的完整字段清单。
- 覆盖 `final_review.md` 的 Key outputs / Step status、`release_handoff.md`、
  `ARTIFACT_GUIDE.md`、`REVIEWER_CHECKLIST.md`、`START_HERE.md`、
  `release_body.zh-CN.md` 和 `release_summary.zh-CN.md`，确认
  `PDF bounded CTest import diagnostics contract fields` / `import_diagnostics_fields=`
  不只存在于 summary metadata，而是能进入 reviewer-facing release report 文本。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "release_candidate_visual_verdict_reports" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `b05a2d9 test: lock PDF import diagnostics release report fields`。
- 已知边界：
  本轮只补 release report 测试契约，不改变 release writer 输出形状、
  metadata helper、bounded summary JSON、CLI JSON schema 或 importer 逻辑。

2026-06-15 继续推进（PDF import diagnostics release bundle 字段契约）：

- 已补 `release_note_bundle_visual_verdict_metadata_test.ps1`，在 release note bundle
  fixture 的 `steps.pdf_bounded_ctest` 中加入
  `import_diagnostics_contract_tests`、`import_diagnostics_contract_fields` 和
  `import_negative_boundary_contract_cases`。
- 回归现在固定 `write_release_note_bundle.ps1` 生成的 release handoff、artifact guide、
  reviewer checklist、START_HERE、中文 release body 和中文 release summary 都能展示完整
  import diagnostics 字段清单，而不是只展示 bounded CTest auxiliary 计数。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "release_note_bundle_visual_verdict_metadata" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过（仅保留既有 CRLF/LF warning）。
- 已提交并推送：
  `a9ce7f2 test: lock PDF import diagnostics release bundle fields`。
- 已知边界：
  本轮只补 release note bundle 测试 fixture 与断言，不改变 release writer 模板、
  release metadata helper、bounded summary JSON、CLI JSON schema 或 importer 逻辑。

2026-06-15 继续推进（PDF import diagnostics release assets manifest 契约）：

- 已补 `package_release_assets_safety_test.ps1`，把 `steps.pdf_bounded_ctest` 里的
  `import_diagnostics_contract_tests`、`import_diagnostics_contract_fields` 和
  `import_negative_boundary_contract_cases` 一并透传到 `release_assets_manifest.json`，
  让 release assets 层也能直接看到完整 bounded CTest import diagnostics 证据。
- 已同步 `test/cmake/WindowsScriptReleaseTests.cmake`，将
  `package_release_assets_safety` 的 CTest 超时从 `TIMEOUT 60` 调整为
  `TIMEOUT 120`，避免 release assets 打包和校验阶段在真实 Windows 环境下因压缩封装
  耗时被误判为超时失败。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "package_release_assets_safety" --output-on-failure --timeout 120`
  通过；`cmake --build .bpdf-roundtrip-msvc --target rebuild_cache` 通过；
  `git diff --check` 通过。
- 已提交并推送：
  `0b942ee test: lock PDF import diagnostics release assets fields`。
- 已知边界：
  本轮只扩展 package release assets safety 的诊断字段闭环并放宽该测试超时，
  不改变 release writer、CLI JSON schema、importer 续表逻辑或 full visual gate 产物。

2026-06-15 继续推进（PDF import diagnostics release assets allow-incomplete 超时同步）：

- 已同步 `package_release_assets_allow_incomplete` 的 CTest timeout 到 `TIMEOUT 120`，
  与 `package_release_assets_safety` 保持一致的 release 包装窗口，避免 AllowIncomplete
  入口在 Windows 上因 staging / ZIP creation 耗时波动被 60 秒默认门槛误伤。
- 该入口仍然保留 `visual_gate=skipped` 语义，不新增 manifest 字段、不裁剪 release
  note bundle 内容，也不改变 package script 的行为。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "package_release_assets_allow_incomplete" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已知边界：
  本轮只调整 AllowIncomplete 包装测试超时，不改变 package script、manifest schema、
  release note bundle 或文档正文。

2026-06-15 继续推进（pdf_cli_import 聚合入口超时契约）：

- 已将 `pdf_cli_import` 这个聚合 CTest 入口的 `TIMEOUT` 从默认 `60` 提升到
  `120`，匹配当前 CLI import diagnostics 回归样本数量，避免完整入口在 Windows
  上因 60 秒 CTest property 被误判为超时。
- 已同步 `pdf_ctest_timeout_contract_test.ps1`：PDF CTest 仍默认要求 bounded
  `TIMEOUT 60`，但允许命名、可审计的量测例外；当前唯一例外为
  `pdf_cli_import=120`。
- 本轮不改变 importer、CLI JSON schema、docs diagnostics 字段或 visual gate
  产物，只收紧测试调度契约。

2026-06-15 继续推进（bounded smoke-import timeout 同步）：

- 已同步 `scripts/run_pdf_ctest_bounded_subset.ps1`：`smoke-import` 子集因为包含完整
  `pdf_cli_import` 聚合入口，`ctest_timeout_seconds` 与实际 CTest property 一起提升到
  `120`；其它 bounded static / regression 子集继续默认 `60` 秒。
- 已补 `pdf_ctest_bounded_subset_summary_test.ps1`，fake CTest 现在会校验
  `--timeout 120`，并断言 summary 中写出 `ctest_timeout_seconds = 120`。
- 已同步发布准入清单和视觉状态文档，避免 release / visual gate 前置证据仍把
  `smoke-import` 误描述为 60 秒窗口。
- 已知边界：
  本轮只调整 bounded subset 调度与文档契约，不改变测试集合、importer 行为、
  CLI JSON schema 或 full visual gate verdict。

2026-06-15 继续推进（bounded subset 默认 timeout 回归）：

- 已扩展 `pdf_ctest_bounded_subset_summary_test.ps1`，新增 `contract-static` fake
  CTest 路径，固定普通 bounded static 子集继续使用默认 `ctest_timeout_seconds=60`。
- 该回归与 `smoke-import=120` 断言配对，避免后续把 `pdf_cli_import` 聚合入口的
  120 秒例外误扩散到所有 bounded 子集。
- 已知边界：
  本轮只补测试契约，不改变 `run_pdf_ctest_bounded_subset.ps1` 的子集列表、
  release/readiness 输出字段、importer 或 CLI JSON。

2026-06-15 继续推进（pdf_cli_import threshold 轻量入口）：

- 已新增 `pdf_cli_import_threshold` CTest 入口，复用 `pdf_cli_import_tests.exe`，
  通过 doctest `--source-file=*pdf_cli_import_threshold_tests.cpp` 只跑 threshold /
  no-text / parse-error 相关 CLI import case。
- 原 `pdf_cli_import` 聚合入口保持不变，继续作为完整 CLI import diagnostics 回归；
  两个入口共享 `RESOURCE_LOCK pdf_cli_import_fixtures`，避免并发写同一个
  `pdf_cli_import` 工作目录时互相清理输出。
- 已知边界：
  本轮只新增测试调度入口，不拆分源码文件、不改变 CLI/importer 行为、不改变
  bounded summary 或 release/readiness schema。

2026-06-15 继续推进（importer continuation diagnostic 位置字段契约）：

- 已收紧 `pdf_import_table_heuristic_import_continuation_tests.cpp` 的 importer 层
  continuation diagnostic helper：除既有续表判断字段外，统一固定
  `page_index`、`block_index` 和 `skipped_repeating_header`。
- 该回归覆盖跨页续接、阈值阻断、列锚点不匹配、非页顶、跨页中间段落阻断、
  同页非首块阻断等样本，避免 importer 内部 diagnostic 与 CLI JSON 位置字段
  或 repeated-header 跳过语义发生漂移。
- 已知边界：
  本轮只补 importer 测试契约，不改变 continuation heuristic、CLI JSON schema、
  release/readiness 字段或 full visual gate 产物。

2026-06-15 继续推进（PDF import 成功 JSON 公共字段文档契约）：

- 已同步英文/中文 PDF workflow 文档：`import-pdf --json` 成功示例现在展示
  `write_json_mutation_result` 提供的公共 mutation 字段 `in_place`、`sections`、
  `headers` 和 `footers`，再接 PDF import 专属字段。
- 已补 `pdf_import_docs_contract_test.ps1`，同时固定用户文档示例和 CLI 公共
  mutation writer 的字段链路，避免用户文档只展示 import 专属字段而漏掉实际
  JSON 根字段。
- 已知边界：
  本轮只补用户可见文档和静态契约，不改变 CLI JSON 输出、importer 续表逻辑、
  release assets 或 full visual gate 产物。

2026-06-15 继续推进（importer continuation 重复表头与列数诊断契约）：

- 已扩展 `pdf_import_table_heuristic_import_continuation_tests.cpp` 专项覆盖：
  repeated-header 续页合并跳过源表头、repeated-header mismatch、column count
  mismatch 三类 importer diagnostic。
- 新断言固定 `source_row_offset = 1`、`continuation_confidence = 95`、
  `header_match_kind = exact`、`skipped_repeating_header = true` 的成功合并路径，
  以及 `repeated_header_mismatch` 的 70 分和 `column_count_mismatch` 的 30 分
  阻断路径。
- 已知边界：
  本轮继续只补 importer 层回归，不改变 continuation heuristic、CLI JSON schema、
  用户文档示例、release assets 或 full visual gate verdict。

## Owner

本方向负责人：wuxianggujun。
