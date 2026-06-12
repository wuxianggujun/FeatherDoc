# PDF E7 进展存档：跨页 subtotal 边界

本文件从 `04-pdf-execution-plan.md` 拆分而来，保留 E7 历史进展记录。

原始行号范围：2062-2493。

---

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

