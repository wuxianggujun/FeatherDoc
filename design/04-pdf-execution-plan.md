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
- 当前 `pdf_regression_manifest.json` 包含 54 个样本。
- 当前 `ctest -R "pdf_regression_"` 覆盖 55 个回归测试，包含 manifest 校验。
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
- 已重构 `sectioned-report-text` 的 manifest 基线：该样本现在覆盖 portrait first-page header/footer、
  landscape appendix even/odd header/footer，以及 `{{page}} / {{total_pages}} /
  {{section_page}} / {{section_total_pages}}` 占位符展开，当前基线按 4 页收口。
- 已补强 `sectioned-report-text` 的文本断言，覆盖 first-page、odd/even header/footer 和 appendix 正文回读。
- 已补充 `pdf_document_adapter_font` 的 layout 层组合断言，覆盖多 section、
  first-page、odd/even header/footer 与页码占位符同时展开的 4 页场景。
- 已同步 `pdf_regression_manifest` 的硬编码断言，固定 `sectioned-report-text`
  当前为 4 页且包含扩展后的页眉页脚文本断言。
- 已确认 `CMakeLists.txt` 中 `FEATHERDOC_BUILD_PDF` 和 `FEATHERDOC_BUILD_PDF_IMPORT` 默认均为 OFF。
- 已确认 `.bdefault-pdf-off/CMakeCache.txt` 中 `FEATHERDOC_BUILD_PDF:BOOL=OFF`、`FEATHERDOC_BUILD_PDF_IMPORT:BOOL=OFF`。
- 已复跑临时 layout-only 验证，确认 `sectioned-report-text` 分页为 4 页：
  第 1 页 portrait first-page header/footer，第 2 页 portrait even header/footer，
  第 3 页 appendix odd header/footer，第 4 页 appendix even header/footer。
- 当前 worktree 已有 `tmp/pdfium-workspace/pdfium`，但 depot_tools 的 CIPD 依赖下载在
  `chrome-infra-packages.appspot.com` 连接超时，导致 PDFium source build 仍停在
  `python3_bin_reldir.txt` / bootstrap 阶段。
- 已新增 `.bpdf-export-msvc` 作为 `FEATHERDOC_BUILD_PDF=ON`、
  `FEATHERDOC_BUILD_PDF_IMPORT=OFF` 的本地验证构建，复用已下载的 PDFio 和 PDFium
  third_party 源，避免 PDFium import 网络阻塞影响导出侧 layout 回归。
- 已在 `.bpdf-export-msvc` 中构建并通过 `pdf_document_adapter_font`，确认新 4 页
  section/header/footer layout 断言可在本地稳定执行。
- 已在 `.bpdf-export-msvc` 中构建并通过 `pdf_unicode_font_diagnostics`，把
  缺字体路径、未解析字体和损坏字体文件的诊断从 PDFium import 阻塞里拆了出来。
- 已将 `pdf_document_adapter_font` 与 PDFium roundtrip 覆盖拆分：基础导出/layout
  测试不再自动链接 `FeatherDocPdfImport`，启用 PDFium import 时另行注册
  `pdf_document_adapter_font_import`。
- 已将 `featherdoc_pdf_regression_sample` 拆成 export-only 可构建的样本生成器；
  启用 PDFium import 时才额外启用回读文字 / 图片校验。
- 已确认 `.bpdf-roundtrip-full-msvc` 中构建并运行 `pdf_document_adapter_font` 不再触发
  PDFium source build，CIPD 阻塞只影响 import/roundtrip 专项目标。
- 已新增 `document-style-gallery-text` regression sample，覆盖居中 / 右对齐、
  首行缩进 / 悬挂缩进、以及多 run 样式组合；并已在
  `output/pdf-export-msvc-document-style-gallery/contact-sheet.png` 完成
  export-only 渲染 smoke。
- 已新增 `superscript-subscript-text` regression sample，并把 run-level
  `superscript/subscript` 透传到 PDF layout；当前导出链会缩小字号并调整
  baseline，便于覆盖科学公式 / 化学式这类常见样式。
- 已新增 `style-superscript-subscript-text` regression sample，并补齐
  `style` 继承链上的 `superscript/subscript` 解析、物化和 baseline reset。
- 已新增 `strikethrough-text` regression sample，并补齐 run-level 与
  style-level 的 `strikethrough` 到 PDF layout / writer 的整条导出链。
- 已新增 `document-rtl-bidi-text` regression sample，覆盖 paragraph bidi、
  run rtl、Arabic 字体映射与 mixed-direction 文本回归。
- `cmake/FeatherDocRunPdfiumBuild.cmake` 已补 `depot_tools` bootstrap 的
  Windows CIPD 连通性预检；当
  `chrome-infra-packages.appspot.com:443` 不可达时，会快速失败并明确指出
  是外部依赖网络阻塞，而不是继续长时间卡死在 bootstrap。
- 已为 `FEATHERDOC_PDFIUM_PROVIDER` 增加 `prebuilt` 模式，可直接导入现成
  `pdfium.lib` / `libpdfium.a` 和 `fpdfview.h`，把 roundtrip / import 构建链从
  `depot_tools` / CIPD 强依赖里拆开。
- 已把 `FEATHERDOC_PDFIUM_PROVIDER` 默认值切到 `auto`：优先尝试 package，其次
  prebuilt，最后才退回 source；这样启用 import 时不再默认走最脆弱的
  `depot_tools` bootstrap 路径。
- 已显式记录 `FEATHERDOC_RESOLVED_PDFIUM_PROVIDER`，避免 `auto` 模式下 install/export
  逻辑仍按请求值判断，导致 package 分支行为丢失。
- `prebuilt` 模式会单独记录 PDFium runtime 目录，测试与 sample 的运行环境现在会优先
  注入该目录，再回退到旧的 `FEATHERDOC_RESOLVED_PDFIUM_OUT_DIR` 逻辑。
- 完整 PDFium 回读和 PNG/contact sheet 视觉验证仍需等 CIPD/PDFium 依赖可用后，
  在 `.bpdf-roundtrip-full-msvc` 上重新跑一轮。

通过命令：

```powershell
git diff --check
Get-Content test/pdf_regression_manifest.json -Raw | ConvertFrom-Json | Out-Null
ctest --test-dir .bpdf-roundtrip-full-msvc -N -R "^pdf_regression_sectioned-report-text$|^pdf_regression_manifest$|^pdf_document_adapter_font$"
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cl /nologo /std:c++20 /EHsc /utf-8 /Iinclude /Ithirdparty\pugixml /Ithirdparty\zip /Itest /c test\pdf_document_adapter_font_tests.cpp /Fo"%TEMP%\fd_pdf_document_adapter_font_tests.obj"'
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake -S . -B .bpdf-export-msvc -G "NMake Makefiles" -DFEATHERDOC_BUILD_PDF=ON -DFEATHERDOC_BUILD_PDF_IMPORT=OFF -DFEATHERDOC_FETCH_PDFIO=OFF -DFEATHERDOC_PDFIO_SOURCE_DIR=%CD%\.bpdf-roundtrip-full-msvc\_deps\featherdoc_pdfio-src'
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build .bpdf-export-msvc --target pdf_document_adapter_font_tests'
ctest --test-dir .bpdf-export-msvc -R "^pdf_document_adapter_font$" --output-on-failure --timeout 60
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake -S . -B .bpdf-roundtrip-full-msvc && cmake --build .bpdf-roundtrip-full-msvc --target pdf_document_adapter_font_tests'
cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build .bpdf-roundtrip-full-msvc --target pdf_regression_manifest_tests'
ctest --test-dir .bpdf-roundtrip-full-msvc -R "^pdf_regression_manifest$|^pdf_document_adapter_font$" --output-on-failure --timeout 60
.\tmp\render-venv\Scripts\python.exe .\scripts\render_pdf_pages.py --input .\.bpdf-export-msvc\test\featherdoc-adapter-linked-section-header-footer.pdf --output-dir .\output\pdf-export-msvc-linked-section-visual\pages --summary .\output\pdf-export-msvc-linked-section-visual\summary.json --contact-sheet .\output\pdf-export-msvc-linked-section-visual\contact-sheet.png --dpi 144
```

可视化验证产物：

- `output/pdf-export-msvc-linked-section-visual/summary.json`
- `output/pdf-export-msvc-linked-section-visual/contact-sheet.png`

`sectioned-report-text` 的 4 页 PDFium roundtrip 与 contact sheet 仍待 `.bpdf-roundtrip-full-msvc`
补齐 PDFium 依赖后执行。

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
- [x] 中文 PDF 在常见阅读器中可复制、可搜索。
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
- 已在 E6 发布门禁中补上 manifest 中所有 `expect_cjk=true`
  样本的 text layer 可复制/可搜索代理检查；使用 PDF 文本层提取结果
  作为可复现验收，不再依赖手工 UI 操作。
- 2026-05-09：已新增 `document-table-cjk-merged-repeat-text` regression sample，
  覆盖 CJK merged cells、重复表头、first/even/default 页眉页脚分页占位符，
  并完成真实导出、PNG 可视化验证和 manifest 接入。
- 2026-05-09：已新增 `document-table-merged-cant-split-text` regression sample，
  覆盖 merged cells、cant_split、重复表头与分页后的整块下移行为，
  并完成真实导出、PNG 可视化验证和 manifest 接入。
- 2026-05-09：已新增 `document-table-vertical-merged-cant-split-text` regression sample，
  覆盖纵向 merged block、cant_split、重复表头与分页时的整块保持行为，
  并修正 table 分页在 merge_down 跨页时的裁切问题。
- 2026-05-09：已新增 `document-table-cjk-vertical-merged-cant-split-text`
  regression sample，覆盖 CJK 字体链路、纵向 merged block、cant_split 与
  重复表头分页后的整块保持行为，并完成真实导出与 PNG 可视化验证。
- 2026-05-10：已新增 `document-cjk-bullet-page-flow-text` regression sample，
  覆盖 CJK bullet 前缀、三级项目符号、图片裁剪/环绕、重复表头和小页高分页，
  并补强 Unicode bullet prefix 在缺失 Latin 显式映射时回退到 east Asia /
  CJK 字体链路。
- 2026-05-10：已新增 `document-cjk-bullet-overlay-page-flow-text`
  regression sample，把 bullet 前缀回退、上标/下标/删除线样式叠加、
  图片锚点与表格分页压进同一条文档流，继续扩充 CJK 复杂版式回归集。

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
cmd /c '"D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build .bpdf-roundtrip-msvc --target pdf_document_adapter_font_tests'
cmd /c '"D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build .bpdf-roundtrip-msvc --target featherdoc_pdf_regression_sample pdf_regression_manifest_tests pdf_document_adapter_font_import_tests'
```

第二条命令需要 PDFium import 依赖已可用；只验证导出侧 layout / 字体诊断 /
样本出图时，优先跑 `pdf_document_adapter_font_tests`、
`pdf_unicode_font_diagnostics_tests` 和 `featherdoc_pdf_regression_sample`。

常用回归命令：

```powershell
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_document_adapter_font$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_(font_resolver|text_metrics|document_adapter_font|cli_export|unicode_font_roundtrip|regression_manifest)$" --output-on-failure --timeout 60
ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_document_adapter_font_import$" --output-on-failure --timeout 60
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
