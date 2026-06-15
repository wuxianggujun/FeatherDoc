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
  本轮在当前 `dev` 上进一步把 diagnostics 数组的首个 table candidate 固定为
  `created_new_table` / `no_previous_table`，并确认自定义
  `--min-table-continuation-confidence` 会同步写入首条 diagnostic 的
  `minimum_continuation_confidence`。
- 已补强 CLI / 文档契约可见性：`pdf_cli_import_tests.cpp` 现在确认
  `has_previous_table`、`source_rows_consistent`、`source_row_offset`、
  `previous_has_repeating_header`、`source_has_repeating_header` 和
  `header_matches_previous` 等字段确实出现在 `import-pdf --json` 输出中；
  本轮在当前 `dev` 上进一步固定 `table_continuation_diagnostics` 内
  `created_new_table` 与 `merged_with_previous_table` 两个完整 JSON object 的字段顺序，
  确认这些诊断不是 importer 内部对象，而是用户可直接消费的 CLI JSON 契约；
  `pdf_import_docs_contract_test.ps1` 反向扫描 CLI command / output writer，
  固定 summary key 与每个 diagnostic object key 都由 CLI JSON 写出。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 120`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_cli_import$" --output-on-failure --timeout 120`
  通过；
  `git diff --check` 通过。
- 本轮已提交并推送：
  `6e3255f test: lock PDF import continuation diagnostics` 和
  `9b824e4 test: expose PDF import CLI diagnostics contract`。
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
- 后续轻量 visual gate 前置契约已补充：
  `docs/pdf_release_readiness_checklist_zh.rst` 的 bounded `smoke-import` 说明现在
  明确 `pdf_cli_import` 固定用户可见 `table_continuation_diagnostics` 与
  `failure_kind = no_text_paragraphs` JSON，`pdf_import_failure` 固定 image-only /
  no-text 负样本不写出目标 DOCX；`pdf_visual_validation_status_docs_contract_test.ps1`
  以 `pdf_import_smoke_diagnostics_release_trace` 固定该 release readiness 入口。
- 边界：
  该批只补 release/readiness 文档契约，不运行 full visual gate，不新增或提交
  `output/` 视觉产物。
- 下一阶段入口：
  继续推进新的 PDF import 负样本 / visual gate，以覆盖更复杂的自由表单、
  嵌套合并和视觉审查边界；保持生成产物仅作为本地证据，不纳入提交。

2026-06-15 继续推进（inconsistent_source_rows 文档边界）：

- 已回到实现层确认：当前 PDF parser 的 `build_table_candidate()` 会按检测到的
  column anchors 为每个候选行预填同等数量的 cells，因此正常 `import-pdf`
  路径不应稳定产生行宽不一致的 `PdfParsedTableCandidate`。
- 本轮不新增不稳定 PDF fixture；`inconsistent_source_rows` 继续作为 importer
  内部一致性兜底，而不是常规用户可触发 blocker。
- 已同步 `docs/en/api/pdf_workflow.rst` 与 `docs/zh-CN/api/pdf_workflow.rst`，
  说明正常导入不应依赖该 blocker 作为稳定诊断，并用
  `pdf_import_docs_contract_test.ps1` 固定该用户可见边界。
- 验证命令：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_import_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已知边界：
  本轮不改变 importer 决策、不改变 CLI JSON schema，不新增 visual gate 产物。

2026-06-15 继续推进（PDF import diagnostics 字段顺序与 bounded preflight 契约）：

- 已补强 bounded `smoke-import` 子集的 summary 契约：
  `scripts/run_pdf_ctest_bounded_subset.ps1` 现在在 `smoke-import` summary 中写出
  `import_visual_gate_scope = bounded_smoke_import_preflight`、
  `import_visual_gate_boundary =
  bounded_smoke_import_preflight_does_not_replace_full_visual_gate_verdict`、
  `import_visual_artifact_policy =
  does_not_generate_or_commit_output_visual_artifacts`，并列出
  `import_diagnostics_contract_tests` 与 `import_diagnostics_contract_fields`。
- 已新增 `pdf_ctest_bounded_subset_summary_test.ps1`，用 fake CTest 固定
  `smoke-import` summary 的字段值、诊断测试列表、诊断字段列表和 10 个 smoke/import
  测试顺序，避免该轻量门禁只停留在文档描述里。
- 已修复 `pdf_cli_export_tests.cpp` 中与 `cli_test_support.hpp` 重复的 binary file
  helper 定义，保证 bounded `smoke-import` 子集能在当前 `.bpdf-roundtrip-msvc`
  构建目录完整跑通。
- 已同步 `docs/en/api/pdf_workflow.rst` 与 `docs/zh-CN/api/pdf_workflow.rst`，
  明确 `table_continuation_diagnostics` 内每个 diagnostic object 的用户可见字段
  按 CLI JSON emission order 展示；`pdf_import_docs_contract_test.ps1` 同时校验
  英文文档、中文文档和 `cli/featherdoc_cli_pdf_import_output.cpp` 中的字段顺序。
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
  本轮不改变 importer 决策、不改变 CLI JSON schema；bounded preflight 只作为资源受限窗口的
  import diagnostics 辅助证据，不替代 full visual gate verdict；不新增或提交 `output/`
  视觉 gate 产物。

2026-06-15 继续推进（PDF import prose / form 负样本契约）：

- 已把用户可见 scope 边界继续向 importer 层收口：新增
  `PDF text importer keeps short-label prose as paragraphs` 与
  `PDF text importer keeps invoice summary form as paragraphs`，固定开启
  `import_table_candidates_as_tables` 后短标签两列 prose 与 invoice summary 表单仍不会被
  提升成 DOCX table。
- 已让 `pdf_import_docs_contract_test.ps1` 的 scope coverage anchor 指向新增 importer
  测试，确保 `short-label prose` 与 `free-form forms` 文档边界能回归到真实导入行为。
- 验证命令：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_import_table_heuristic|pdf_import_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `0d7efc2 test: lock PDF import prose negative boundaries`。
- 已知边界：
  本轮只锁定 text-first PDF import 的保守负样本行为；不改变 table candidate
  heuristic、不新增 CLI JSON schema、不替代 full visual gate，也不新增或提交 `output/`
  视觉 gate 产物。

2026-06-15 继续推进（PDF import prose / form CLI 用户可见契约）：

- 已在 `pdf_cli_import_tests.cpp` 增加
  `cli import-pdf keeps prose and forms as paragraphs with table promotion`，
  将 short-label prose 与 invoice summary form 的保守边界从 importer 层推进到
  CLI JSON 用户可见层。
- 回归固定 `--import-table-candidates-as-tables --json` 的输出契约：
  `ok = true`、`paragraphs_imported` 存在、`tables_imported = 0`、
  `table_continuation_diagnostics_count = 0`、
  `table_continuation_diagnostics = []` 和
  `import_table_candidates_as_tables = true`；导出的 DOCX 保存重开后不包含 table。
- 验证命令：
  `cmake --build .bpdf-roundtrip-msvc --target pdf_cli_import_tests`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_cli_import|pdf_import_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `f027db1 test: expose PDF import prose CLI contract`。
- 已知边界：
  本轮不改变 CLI JSON schema、不改变 importer/table candidate heuristic；该测试只固定
  用户可见的 prose/form 负样本结果，不替代 full visual gate，也不新增或提交 `output/`
  视觉 gate 产物。

2026-06-15 继续推进（PDF bounded smoke-import prose / form preflight 契约）：

- 已扩展 bounded `smoke-import` summary，把 prose/form 负样本的用户可见字段纳入
  `import_diagnostics_contract_fields`：`table_continuation_diagnostics=[]`、
  `tables_imported=0` 和 `import_table_candidates_as_tables=true`。
- 已新增 summary 字段 `import_negative_boundary_contract_cases`，固定
  `short_label_prose_remains_paragraphs` 与
  `invoice_summary_form_remains_paragraphs`，明确这些 case 由 `pdf_cli_import` 与
  `pdf_import_table_heuristic` 在同一 bounded preflight 里覆盖。
- 已同步 `pdf_ctest_bounded_subset_summary_test.ps1`、
  `docs/pdf_release_readiness_checklist_zh.rst` 和
  `pdf_visual_validation_status_docs_contract_test.ps1`，把脚本 summary、发布清单和
  docs contract 锁在同一契约上。
- 验证命令：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_ctest_bounded_subset_summary|pdf_visual_validation_status_docs_contract" --output-on-failure --timeout 120`
  通过；
  `powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 -Subset smoke-import -BuildDir .\.bpdf-roundtrip-msvc -OutputJson .\build\pdf-ctest-bounded-subset-current\summary.json`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `26304e4 test: extend PDF bounded import prose contract`。
- 已知边界：
  bounded `smoke-import` 只作为资源受限窗口的 import diagnostics / prose-form
  preflight 辅助证据，不替代 full visual gate verdict；本轮不新增或提交 `output/`
  视觉 gate 产物。

2026-06-15 继续推进（PDF import diagnostics governance source-report 契约）：

- 已把 bounded `smoke-import` 中的 diagnostics / prose-form 契约继续向治理层传递：
  `Get-PdfBoundedCtestSummaryInfo` 聚合
  `import_diagnostics_contract_tests`、`import_diagnostics_contract_fields` 和
  `import_negative_boundary_contract_cases`，release blocker rollup source report 再暴露
  `pdf_bounded_ctest_import_diagnostics_contract_tests`、
  `pdf_bounded_ctest_import_diagnostics_contract_fields` 和
  `pdf_bounded_ctest_import_negative_boundary_contract_cases`。
- 已补 `build_release_governance_handoff_report_include_rollup` 回归，固定
  JSON summary 与 Markdown `source_report:` block 同时展示
  `pdf_import_table_heuristic`、`table_continuation_diagnostics=[]` 和
  `short_label_prose_remains_paragraphs`。
- 已同步 `BUILDING_PDF.md` 与 PDF release readiness checklist，明确这些字段是
  bounded preflight 的 release governance handoff 证据，不能替代 full visual gate
  verdict。
- 验证命令：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "release_candidate_visual_verdict|build_release_governance_handoff_report_include_rollup|pdf_visual_validation_status_docs_contract" --output-on-failure --timeout 120`
  中除首次文档缺口外的 release candidate / handoff 测试均通过；补齐
  `BUILDING_PDF.md` 后，
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_visual_validation_status_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `edf08c0 test: expose PDF import diagnostics governance contract`。
- 已知边界：
  本轮只打通 bounded summary -> release blocker source report -> governance handoff
  Markdown 的用户可见诊断链路，不改变 importer merge heuristic、不新增 CLI JSON
  schema 字段，也不新增或提交 `output/` 视觉产物。

2026-06-15 继续推进（PDF repeated-header subtotal CLI diagnostic object 契约）：

- 已在 `pdf_cli_import_tests.cpp` 中新增完整 JSON object 断言，固定 repeated-header
  subtotal 续页合并时的用户可见诊断对象，而不再只依赖散点字段查找。
- 该断言覆盖 `source_row_offset = 1`、`continuation_confidence = 95`、
  `minimum_continuation_confidence = 0`、所有 continuation 布尔判定、
  `header_match_kind = exact`、`skipped_repeating_header = true`、
  `disposition = merged_with_previous_table` 和 `blocker = none`，并固定 object
  字段顺序。
- 已应用到 missing-unit、sparse-body、amount-only body 三个 CLI subtotal 合并场景，
  与已有 `table_continuation_diagnostics_count = 2`、`tables_imported = 1` 和
  DOCX 单表结构断言形成闭环。
- 验证命令：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_cli_import" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `631239e test: lock PDF repeated header CLI diagnostics`。
- 已知边界：
  只固定 CLI JSON 诊断展示契约，不改变 importer continuation heuristic、不新增 CLI
  JSON schema 字段，也不新增或提交 `output/` 视觉产物。

2026-06-15 继续推进（PDF repeated-header diagnostics 公开 workflow 契约）：

- 已把 repeated-header 续页合并的关键诊断值同步到
  `docs/en/api/pdf_workflow.rst` 与 `docs/zh-CN/api/pdf_workflow.rst`，让
  CLI JSON 中已固定的 `source_row_offset = 1`、`skipped_repeating_header = true`、
  `continuation_confidence = 95`、`header_match_kind = exact` 和 `blocker = none`
  在用户可见 workflow 文档中也有稳定解释。
- 文档同时说明 `continuation_confidence = 95` 是确定性的启发式诊断分数，
  不是概率；并给出最小 JSON 片段，避免只在测试断言里保留该契约。
- 已补 `pdf_import_docs_contract_test.ps1` marker，固定中英文 workflow 文档必须
  保留上述字段值；脚本侧 marker 继续使用 ASCII-only 字面量，兼容
  Windows PowerShell 5.1。
- 验证命令：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_import_docs_contract|docs_bilingual_entrypoints_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `a22352c docs: document PDF repeated header diagnostics contract`。
- 已知边界：
  本轮只补公开文档与 docs contract，不新增 CLI JSON 字段、不改变 importer 合并
  启发式，也不运行 full visual gate。

2026-06-15 继续推进（PDF import blocker diagnostic object 契约）：

- 已继续收紧 `pdf_cli_import_tests.cpp` 的 CLI JSON diagnostics 契约：拆表负样本现在
  不只查找 blocker 字符串，而是要求完整 diagnostic object 出现在
  `table_continuation_diagnostics` 中。
- 新增覆盖四类用户最常见 blocker：
  `repeated_header_mismatch` 固定 `continuation_confidence = 70`、header 不匹配；
  `column_anchors_mismatch` 固定 `continuation_confidence = 55`、列数匹配但列锚点不匹配；
  `continuation_confidence_below_threshold` 固定 `continuation_confidence = 85` 与
  `minimum_continuation_confidence = 90`；
  `column_count_mismatch` 固定 `continuation_confidence = 30`、列数和列锚点均不匹配。
- 四个 object 均固定 `source_row_offset = 0`、`skipped_repeating_header = false`、
  `disposition = created_new_table` 和字段输出顺序，避免后续 CLI 仍输出 blocker
  字符串但丢失上下文判定。
- 验证命令：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_cli_import" --output-on-failure --timeout 120`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_import_docs_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `f400970 test: lock PDF import blocker diagnostics`。
- 已知边界：
  本轮不新增 CLI JSON 字段、不改变 importer 判断逻辑；只把既有 blocker 场景的
  用户可见 object 形状锁成回归契约。

2026-06-15 继续推进（PDF import blocker diagnostics 公开 workflow 契约）：

- 已将 CLI 已锁定的 blocker diagnostic object 继续同步到双语
  `pdf_workflow.rst`，避免公开文档只列 blocker 名称而缺少上下文字段。
- `repeated_header_mismatch`、`column_count_mismatch`、`column_anchors_mismatch` 和
  `continuation_confidence_below_threshold` 现在都以完整 JSON object 形式展示，
  字段顺序与 CLI JSON emission order 对齐。
- 文档片段明确四类 blocker 的关键判定组合：header mismatch、column count
  mismatch、column anchor mismatch，以及 `continuation_confidence = 85` /
  `minimum_continuation_confidence = 90` 的阈值阻断。
- 已补 `pdf_import_docs_contract_test.ps1` marker 固定公开 workflow 必须保留
  `continuation_confidence = 70/55/85/30`、`column_count_matches = false`、
  `column_anchors_match = false` 和 `blocker = column_count_mismatch`。
- 验证命令：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_import_docs_contract|docs_bilingual_entrypoints_contract" --output-on-failure --timeout 120`
  通过；`git diff --check` 通过。
- 已提交并推送：
  `8cc196d docs: document PDF import blocker diagnostics contract`。
- 已知边界：
  本轮只补公开文档契约，不新增或更改 CLI JSON 字段、不改变 importer 续表启发式，
  也不运行 full visual gate。
