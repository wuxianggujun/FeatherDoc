# PDF E7 进展存档：续页表头与 subtotal

本文件从 `04-pdf-execution-plan.md` 拆分而来，保留 E7 历史进展记录。

原始行号范围：1207-2061。

---

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

