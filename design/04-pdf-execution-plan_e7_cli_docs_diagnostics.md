# PDF E7 进展存档：CLI、文档与诊断

本文件从 `04-pdf-execution-plan.md` 拆分而来，保留 E7 历史进展记录。

原始行号范围：2974-3158。

---

2026-05-15 继续推进（Document RTL writer fallback E2E）：

- 已把 `pdf_unicode_font_roundtrip` 里的非 LTR writer fallback 回归从单一 RTL 样例扩展为
  `right_to_left` / `top_to_bottom` / `bottom_to_top` / `unknown` 矩阵，确认当前
  glyph-id CID writer 入口只接受 `left_to_right`。
- 已在 `pdf_unicode_font_roundtrip` 增加 Document 级端到端回归：真实 `Run::set_rtl()`
  经 document adapter 生成 `right_to_left` shaped glyph metadata 后，PDFio writer 不生成
  `/FeatherDocGlyph` CID 字体资源，继续走字符串 fallback。
- 回归同时用 PDFium 解析写出的 PDF，断言原文只回读一次，避免后续扩展 RTL writer 时破坏
  当前安全边界和文本提取语义。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_unicode_font_roundtrip_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_unicode_font_roundtrip$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font|unicode_font_roundtrip)$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60`
  通过。
- 矩阵扩展后再次完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_unicode_font_roundtrip_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_unicode_font_roundtrip$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font|unicode_font_roundtrip)$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60`
  通过。
- 本轮在当前 worktree 重新跑 `pdf_regression_` 时发现 Document adapter 的 shaped glyph
  CID 字体路径仍嵌入完整字体，导致 `document-contract-cjk-style`、
  `document-eastasia-style-probe`、`list-report-text` 触发页数级 PDF 大小阈值。
  已让 shaped glyph CID 字体复用 HarfBuzz glyph-id subset，并保留失败时回退完整字体的行为。
  修复后失败样本输出分别收敛到约 39KB、29KB、1.19MB。
- 继续补齐 shaped glyph subset 的 writer option 语义：`subset_unicode_fonts=false`
  现在同样会让 shaped glyph CID 字体回退完整字体嵌入，`true` 时才走 HarfBuzz
  glyph-id subset；`pdf_unicode_font_roundtrip` 已新增 shaped glyph CJK full/subset
  直接对比，避免只靠 regression PDF 大小阈值间接覆盖。
- 下一阶段入口：
  当前已覆盖 Document RTL metadata 到 writer fallback 的闭环；真正支持 RTL glyph-id stream
  前，需要先形成独立验收样本，明确 visual order、logical text extraction、glyph advance
  方向和 text matrix 的组合规则。

2026-05-15 继续推进（CLI import continuation diagnostics JSON）：

- 已把 `import-pdf --json` 的表格跨页 continuation 诊断从只输出
  `table_continuation_diagnostics_count` 扩展为完整
  `table_continuation_diagnostics` 数组，逐条暴露页索引、块索引、源行偏移、
  置信度阈值、表头匹配类型、重复表头跳过状态、阻断原因和最终处置结果。
- 已保留原有 `table_continuation_diagnostics_count` 字段，避免破坏现有 CLI
  JSON 消费方；新增数组用于定位为什么跨页表格被新建、合并或阻断。
- 已补 CLI import 回归：
  `cli import-pdf reports table continuation diagnostics` 使用段落 + 表格 + 分页
  + 表格 + 段落 fixture，断言 opt-in 导入后仍合并为 1 张 3 列 6 行表格，
  同时 JSON 包含 `created_new_table`、`merged_with_previous_table`、
  `no_previous_table`、`none`、`not_required` 等关键诊断字段。
- 已完成验证：
  `cmake -S . -B .bpdf-cli-import-msvc -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -DFEATHERDOC_BUILD_PDF=ON -DFEATHERDOC_BUILD_PDF_IMPORT=ON -DBUILD_CLI=ON -DBUILD_TESTING=ON -DBUILD_SAMPLES=OFF ...`
  通过；
  `cmake --build .bpdf-cli-import-msvc --target pdf_cli_import_tests`
  通过；
  `ctest --test-dir .bpdf-cli-import-msvc -R "^pdf_cli_import$" --output-on-failure --timeout 60`
  通过。
- 本轮验证记录：
  Windows 环境只有 `py.exe` 时，PDFium source configure 需要临时提供
  `python3` shim；同时 FeatherDoc import CLI 测试目录必须显式使用
  `CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`，否则会因 PDFium `/MT` 与默认
  `/MD` 或 Debug runtime 不一致在链接阶段失败。
- 下一阶段入口：
  继续 E7 时，优先补 CLI JSON 的失败/阻断样本覆盖，例如 semantic header
  mismatch、column anchor mismatch、confidence below threshold，让用户能从
  CLI 输出直接判断跨页表格未合并的原因。

2026-05-15 继续推进（CLI continuation blocker JSON 回归）：

- 已补 `import-pdf --json` 的阻断类 continuation diagnostics CLI 回归，覆盖：
  semantic repeated header mismatch、column anchor mismatch、confidence below
  threshold 三类常见未合并原因。
- 新增 CLI 测试均复用现有 PDF fixture，断言 opt-in 导入后仍保守拆成两张
  `Document` 表格，并在 JSON 中暴露 `created_new_table` 与对应 blocker：
  `repeated_header_mismatch`、`column_anchors_mismatch`、
  `continuation_confidence_below_threshold`。
- 回归同时确认关键解释字段已经透出：
  `previous_has_repeating_header`、`source_has_repeating_header`、
  `header_matches_previous`、`header_match_kind`、`column_count_matches`、
  `column_anchors_match`、`continuation_confidence`、
  `minimum_continuation_confidence`。
- 已完成验证：
  `cmake --build .bpdf-cli-import-msvc --target pdf_cli_import_tests`
  通过；
  `ctest --test-dir .bpdf-cli-import-msvc -R "^pdf_cli_import$" --output-on-failure --timeout 60`
  通过。
- 下一阶段入口：
  继续 E7 时，可以把同一套 CLI JSON 诊断扩展到 column count mismatch、
  missing/sparse body rows、local anchor drift 等更细边界；或开始整理面向用户的
  `import-pdf --json` 输出 schema 文档。

2026-05-15 继续推进（CLI continuation 细边界 JSON 回归）：

- 已继续扩展 `import-pdf --json` continuation diagnostics 的 CLI 回归，覆盖
  missing Unit cell、sparse body row 两类“应合并”正样本，以及 column count
  mismatch 一类“应保守拆分”负样本。
- missing/sparse 样本确认 opt-in 导入后仍合并为 1 张 4 列 7 行表格，JSON 暴露
  `merged_with_previous_table`、`blocker":"none"`、`header_match_kind":"exact"`、
  `skipped_repeating_header":true` 和 `source_row_offset":1`，证明 CLI 能解释
  重复表头跳过后的续接行为。
- column count mismatch 样本确认 opt-in 导入后保守拆成 4 列表 + 3 列表，JSON 暴露
  `column_count_matches":false`、`column_anchors_match":false`、
  `header_match_kind":"none"` 和 `blocker":"column_count_mismatch"`，方便用户定位
  未合并原因不是阈值或表头文本，而是列结构不兼容。
- 已完成验证：
  `cmake --build .bpdf-cli-import-msvc --target pdf_cli_import_tests`
  通过；
  `ctest --test-dir .bpdf-cli-import-msvc -R "^pdf_cli_import$" --output-on-failure --timeout 60`
  通过。
- 下一阶段入口：
  继续 E7 时，剩余适合 CLI 层补齐的边界主要是 local anchor drift、
  amount-only / isolated amount-only body rows；之后应整理 `import-pdf --json`
  输出 schema 到用户文档，避免测试覆盖已经扩展但外部契约仍只散落在执行计划里。

2026-05-15 继续推进（CLI continuation 最后边界 JSON 回归）：

- 已补齐上一轮留下的 CLI diagnostics 边界：amount-only body row、
  isolated amount-only body row 和 local anchor drift。
- amount-only / isolated amount-only 样本确认 opt-in 导入后仍合并为单表，JSON
  暴露 `merged_with_previous_table`、`blocker":"none"`、
  `header_match_kind":"exact"`、`skipped_repeating_header":true` 和
  `source_row_offset":1`，覆盖极稀疏表体仍可续接的 CLI 解释路径。
- local anchor drift 样本确认两页列数一致但局部列锚点漂移时仍保守拆表，JSON 暴露
  `column_count_matches":true`、`column_anchors_match":false`、
  `disposition":"created_new_table"` 和 `blocker":"column_anchors_mismatch"`，
  补足“同列数但锚点不兼容”的用户可诊断路径。
- 已完成验证：
  `cmake --build .bpdf-cli-import-msvc --target pdf_cli_import_tests`
  通过；
  `ctest --test-dir .bpdf-cli-import-msvc -R "^pdf_cli_import$" --output-on-failure --timeout 60`
  通过。
- 下一阶段入口：
  CLI JSON diagnostics 的主要正负样本已覆盖到测试层；继续 E7 时，优先整理
  `import-pdf --json` 输出 schema 到用户文档，并把这些字段定义为稳定调试契约。

2026-05-15 继续推进（import-pdf JSON schema 用户文档）：

- 已在 `docs/index.rst` 增加 `PDF import JSON diagnostics` 小节，把
  `import-pdf --json` 的成功输出、失败输出、表格 continuation diagnostics 数组、
  诊断字段含义和枚举值从执行计划沉淀到用户可读文档。
- 文档明确 `table_continuation_diagnostics` 按 table candidate discovery 顺序输出，
  `source_row_offset = 1` 通常表示重复表头被识别并跳过；同时说明
  `continuation_confidence` 是规则型诊断分数，不是概率模型。
- 文档列出 `disposition`、`header_match_kind` 和 `blocker` 当前稳定字符串，覆盖
  merge 成功、列数不匹配、锚点不匹配、表头不匹配、阈值不足等 CLI 已回归路径。
- 本轮只改文档，不改变 CLI JSON 行为；下一阶段如果继续 E7，应考虑给
  `docs/index.rst` 里的 `import-pdf` 示例补最小受控样本生成命令，或开始整理
  PDF import 的已知限制用户页。

2026-05-15 继续推进（PDF import JSON diagnostics 入口提示）：

- 已把 `import-pdf --json` continuation diagnostics 的入口提示同步到
  `README.md` 和 `README.zh-CN.md`，说明成功 JSON 会包含
  `table_continuation_diagnostics_count` 与 `table_continuation_diagnostics`，
  并指向 `docs/index.rst` 的字段级 schema。
- 已在 CLI usage 文本里为 PDF import 增加短说明：
  `--json includes table_continuation_diagnostics for PDF table merge decisions`，
  避免用户只能从长文档或测试里发现 diagnostics 能力。
- 已补 `cli_usage` 回归固定 `table_continuation_diagnostics` 出现在帮助文本中。
- 下一阶段入口：
  可以继续整理 PDF import 的已知限制用户页，或把 `BUILDING_PDF.md` 中偏开发者的
  PDF import 限制和 README/docs 中的用户入口进一步去重。

2026-05-15 继续推进（PDF import 用户范围与限制文档）：

- 已在 `docs/index.rst` 的 PDF import 区域新增 `PDF import supported scope and limits`
  小节，把 E7 和 `BUILDING_PDF.md` 中面向用户最关键的范围边界沉淀到 Sphinx 文档。
- 文档明确 PDF import 是实验性 opt-in 路径，面向 text-first 且具备 extractable text
  和 character geometry 的 PDF；不是任意 PDF -> Word 精确复刻。
- 文档列出当前可靠范围：段落导入、保守表格候选、显式表格提升、跨页续接、重复表头匹配、
  受控 subtotal / total 行和 continuation diagnostics。
- 文档同步保守边界：默认拒绝表格候选，列数/锚点/语义表头/中间段落/低置信度会拆表；
  普通双栏 prose、编号列表、短标签 prose 和自由表单应保留为段落。
- 文档明确不支持扫描件、OCR、image-only 表格、任意嵌套表格语义、复杂矢量还原、
  旋转/浮动内容恢复或任意 PDF 的精确视觉重建。
- 下一阶段入口：
  PDF import 用户文档已具备入口、JSON schema、范围和限制；继续 E7 时可以考虑
  抽出独立 `docs/pdf_import_*.rst` 页面，或开始把 `BUILDING_PDF.md` 中重复的用户内容
  收敛为指向 docs 的链接。

2026-06-14 继续推进（PDF import diagnostics 契约闭环）：

- 已补强 importer 层 continuation diagnostics 回归：在
  `pdf_import_table_heuristic_import_continuation_tests.cpp` 中集中固定
  `source_row_offset`、`continuation_confidence`、`minimum_continuation_confidence`、
  `header_match_kind`、`blocker`、`disposition` 以及全部关键布尔判定字段，
  覆盖可合并、阈值阻断、列锚点不匹配、页顶距离过低、中间段落重置和非页内首个 block 等路径。
- 已补强 CLI / 文档契约可见性：`pdf_cli_import_tests.cpp` 现在确认
  `has_previous_table`、`source_rows_consistent`、`source_row_offset`、
  `previous_has_repeating_header`、`source_has_repeating_header` 和
  `header_matches_previous` 等字段确实出现在 `import-pdf --json` 输出中；
  `pdf_import_docs_contract_test.ps1` 反向扫描 CLI command / output writer，
  固定 summary key 与每个 diagnostic object key 都由 CLI JSON 写出。
- 已完成验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 120`
  通过；
  `cmake --build .bpdf-roundtrip-msvc --target pdf_cli_import_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_cli_import|pdf_import_docs_contract" --output-on-failure --timeout 120`
  通过；
  `git diff --check` 通过。
- 本轮已提交并推送：
  `3bca24c test: lock PDF import continuation diagnostics` 和
  `30c9905 test: expose PDF import CLI diagnostics contract`。
- 已知边界：
  本轮不改变 importer 决策、不改变 CLI JSON schema，只把已有字段的内部对象、
  CLI 输出和文档契约三者闭环固定；`inconsistent_source_rows` 仍作为内部一致性兜底，
  没有新增稳定用户 fixture；当前未新增视觉 gate 产物，视觉验证仍沿用既有 E7 样本记录。
- 后续已把公开 JSON 示例补齐为真实风格对象：
  `docs/en/api/pdf_workflow.rst` 和 `docs/zh-CN/api/pdf_workflow.rst` 的
  `table_continuation_diagnostics` 不再展示空数组，而是展开 `created_new_table` /
  `no_previous_table` 与 `merged_with_previous_table` / `none` 两个 diagnostic object，
  覆盖 `source_row_offset`、`minimum_continuation_confidence`、header match、布尔判定、
  `disposition` 和 `blocker` 等用户可见字段。
- 对应 docs contract 已同步固定：
  `pdf_import_docs_contract_test.ps1` 要求示例保留 diagnostics 数组、`has_previous_table`
  的 false/true 两种状态、`source_rows_consistent`、合并 disposition、`no_previous_table`
  和 `none` blocker；RST JSON code block 仍由 `ConvertFrom-Json` 校验。
- 本轮追加验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_import_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `272bfff docs: show realistic PDF import diagnostics JSON`。
- 后续负样本契约已补充：
  `test/pdf_import_failure_tests.cpp` 增加 image-only placeholder PDF fixture，
  固定有页面图形但无可提取文字时仍归类为 `no_text_paragraphs`；
  `pdf_import_docs_contract_test.ps1` 将 `scanned PDFs`、`OCR` 和 `image-only`
  scope 声明绑定到该失败回归锚点。
- 对应验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_failure_tests` 通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_import_failure|pdf_import_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 后续 CLI failure JSON 契约已补充：
  `pdf_cli_import_threshold_tests.cpp` 使用同一 image-only placeholder fixture 走
  `featherdoc_cli import-pdf --json`，固定失败输出包含 `ok:false`、`stage:"import"`、
  `failure_kind:"no_text_paragraphs"`、`input` 和 `output`，且不写出目标 DOCX。
- 对应公开文档与 docs contract 已同步：
  `docs/*/api/pdf_workflow.rst` 明确 scanned / image-only PDFs without extractable
  text paragraphs 会报告 `no_text_paragraphs`；`pdf_import_docs_contract_test.ps1`
  反查 `pdf_cli_import*.cpp` 必须覆盖 `table_candidates_detected` 和
  `no_text_paragraphs` 两类 failure JSON。
- 本轮追加验证：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_cli_import_tests` 通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_cli_import|pdf_import_failure|pdf_import_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 下一阶段入口：
  继续推进新的 PDF import 负样本 / visual gate，以覆盖更复杂的自由表单、
  嵌套合并和视觉审查边界；保持生成产物仅作为本地证据，不纳入提交。
