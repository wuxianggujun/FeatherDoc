# PDF E7 进展存档：结构与表格导入

本文件从 `04-pdf-execution-plan.md` 拆分而来，保留 E7 历史进展记录。

原始行号范围：500-1206。

---

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

