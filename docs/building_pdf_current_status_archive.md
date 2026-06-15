# BUILDING_PDF 当前可用状态存档

本文件从 `BUILDING_PDF.md` 拆分而来，保留原始当前可用状态的详细历史记录。

原始行号范围：824-1236。

---

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
- 已有 90 个 regression manifest 样本，其中 42 个 visual baseline 样本、
  43 个 CJK text-layer 样本，覆盖纯文本、多页文本、中文路径、中英混排标点、
  Latin ligature 文本、样式文本、字号、颜色、横向页面、标点、边框框体、
  基础线条、固定坐标表格外观、合同样式、页眉页脚、多栏文本、发票网格、
  图片说明文字、metadata 长标题、RTL/Bidi 探针，以及 sectioned/list/long report、
  image report、CJK report、CJK image report、document east-asian style probe、
  document image semantics、document table semantics、document long flow、
  document invoice table、document table header/footer variants、document table wrap flow、
  document table cant split、document table merged cells、document table merged repeat、
  document table merged header footer variants、document table merged cant split 和
  CJK page-flow / table-wrap / image-wrap / complex-layout 系列生成型样本

真实业务样本覆盖以 manifest ID 记录，避免二进制样本和文档说明漂移：

- 合同：`contract-cjk-style`、`document-contract-cjk-style`
- 发票 / 报价单：`invoice-grid-text`、`document-invoice-table-text`
- 图文报告：`image-report-text`、`cjk-image-report-text`、`document-cjk-image-wrap-stress-text`
- 长文档：`long-report-text`、`document-long-flow-text`
- 多 section / 页眉页脚 / 多页流：`sectioned-report-text`、`header-footer-text`、
  `document-cjk-table-wrap-page-flow-text`

这些发布准入样本必须继续被
`test/pdf_real_business_sample_manifest_contract_test.ps1` 同步校验：文档清单里的
manifest ID 要存在于 `test/pdf_regression_manifest.json`，低资源
`regression-business-samples` bounded subset 也必须保持固定的 10 个
`pdf_regression_` 测试顺序，避免真实业务样本只停留在文档描述中。固定标记：
`pdf_real_business_sample_release_entry_trace`。

当前导出支持矩阵（support matrix）：

- 段落：已支持基础段落、分页流和 text-layer 回读；复杂 Word 级版式仍需专项。
- 表格：部分支持固定坐标网格、简单表格外观、部分跨页 / 合并 / cant split
  回归；复杂自动分页和 Word 完整表格布局仍非承诺范围。
- 图片：部分支持图片出现次数、说明文字和受控图文报告；锚点、裁剪、环绕和
  浮动定位仍非发布承诺。
- 页眉页脚：部分支持基础页眉页脚、页码文本和 CJK / RTL 探针；复杂 section
  继承仍需继续收口。
- 字体：部分支持显式字体文件、CJK fallback、synthetic bold / italic 和
  HarfBuzz shaped run；字体许可证 / 捆绑策略仍需 release 层确认。
- CJK：已支持受控中文 / 混排写出、ToUnicode 回读和 43 个 CJK text-layer 样本。
- RTL / Bidi：部分支持探针和回退边界；非 LTR glyph-id stream 仍回退字符串路径。
- 字段：不支持 Word 字段语义到 PDF 的完整转换；当前只承诺受控文本结果。

发布准入清单固定入口（release checklist，详见
`docs/pdf_release_readiness_checklist_zh.rst`）：

1. 文档状态：当前 preflight / full gate evidence 以脚本生成的 summary 和 release
   材料为准；`docs/pdf_visual_validation_status_zh.rst` 只保留为历史状态快照，
   若继续维护，不能停留在旧的 blocked 叙事。
2. 样本计数：`test/pdf_regression_manifest.json` 统计必须与本文 90 / 42 / 43 保持一致。
3. 预检：运行 `scripts/check_pdf_visual_release_gate_preflight.ps1`，确认
   raw summary 为 `blocking_checks = []`、
   `blocking_summary.blocking_check_count = 0`；只有生成 preflight governance report
   时才用 `preflight_ready = true` 表示治理层预检通过。
4. 完整门禁：运行 `scripts/run_pdf_visual_release_gate.ps1`，或在资源受限时用
   `-FinalizeOnly -SkipPreflight` 复核现有 full gate 产物。
5. 截断尝试：如果非 `FinalizeOnly` 完整门禁被 60 秒保护截断，运行
   `scripts/write_pdf_visual_gate_attempt_summary.ps1 -ReportDir .\output\pdf-visual-release-gate-current\report`，
   生成 `attempt-summary.json`，确认 `verdict = not_complete`、
   `full_visual_gate_status = not_complete` 和 `evidence_scope = bounded_attempt_auxiliary_only`
   以及 `outer_guard_status = timed_out`、`outer_guard_timed_out = true`、
   `outer_guard_timeout_seconds = 60`
   与已完成的 `pdf_cli_export`、`pdf_regression`、CJK copy/search、fresh baseline 计数同块出现。
6. 受控切片：如果 fresh baseline render 阶段无法一次完成，运行
   `scripts/run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -OutputDir .\output\pdf-visual-release-gate-current -VisualBaselineSliceOnly -VisualBaselineOffset 0 -VisualBaselineLimit 4 -SkipPreflight`，
   生成 `visual-baseline-slice-offset-0-limit-4-summary.json`，确认
   `schema = featherdoc.pdf_visual_baseline_slice.v1`、
   `evidence_scope = visual_baseline_slice_only` 和
   `slice_summary_does_not_replace_full_visual_gate_verdict`。固定标记：
   `pdf_visual_baseline_slice_summary_trace`。
   如果 `attempt-summary.json` 已经写出
   `visual_baseline_resume_slice_offset` 和
   `visual_baseline_resume_slice_limit`，优先运行
   `scripts/run_pdf_visual_segmented_resume.ps1 -BuildDir .\.bpdf-roundtrip-msvc -OutputDir .\output\pdf-visual-release-gate-current -ChunkSize 6 -PerSliceTimeoutSeconds 60`
   自动规划尾段切片并生成 `segmented-resume-summary.json`。该 summary 必须保留
   `schema = featherdoc.pdf_visual_segmented_resume_summary.v1`、
   `evidence_scope = visual_segmented_resume_auxiliary_only`、
   `full_visual_gate_status = not_complete`、`resume_tail_fully_planned`、
   `total_resume_slice_count`、`planned_slice_count`、`executed_slice_count`、
   `passed_slice_count`、`failed_slice_count`、`timeout_slice_count`、
   `aggregate_rebuild_status`、`segmented_summary_status` 和
   `segmented_resume_does_not_replace_full_visual_gate_verdict`。固定标记：
   `pdf_visual_segmented_resume_summary_trace`。
7. 汇总图：如需单独刷新 aggregate contact sheet，运行
   `scripts/run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -OutputDir .\output\pdf-visual-release-gate-current -RebuildAggregateContactSheetOnly -SkipPreflight`，
   生成 `aggregate-contact-sheet-rebuild-summary.json`，确认
   `schema = featherdoc.pdf_visual_aggregate_contact_sheet_rebuild.v1`、
   `evidence_scope = aggregate_contact_sheet_rebuild_only` 和
   `aggregate_rebuild_summary_does_not_replace_full_visual_gate_verdict`。固定标记：
   `pdf_visual_aggregate_contact_sheet_rebuild_trace`。
8. fresh full gate 在 aggregate contact sheet 构建成功后，会先把核心 pass
   summary 写入 `report/summary.json`，再追加较大的 baseline/CJK 详情 payload。
   固定标记：`pdf_visual_gate_core_pass_summary_trace`、
   `summary_detail_payload_included`、`summary_detail_status` 和
   `core_pass_written_before_detail_payload`。如果外层 guard 正好在详情序列化阶段
   触发，`run_pdf_visual_full_gate_guarded.ps1` 只能在同一轮阶段证据全量 pass 时
   输出 `outer_guard_status = timed_out_after_pass_summary`；阶段不全时仍必须保持
   `full_visual_gate_status = not_complete`。
9. full gate 的 baseline 渲染默认传递 `--skip-contact-sheet` 给
   `scripts/render_pdf_pages.py`，只生成页面 PNG 和 summary；reviewer-facing
   视觉证据仍由 `report/aggregate-contact-sheet.png` 统一承载。summary 会保留
   `contact_sheet_skipped`，用于解释为什么单样本 `contact-sheet.png` 不是 full gate
   的硬前提。
10. 分段汇总：如 visual gate 由 attempt、slice 和 aggregate rebuild 分段完成，
   运行 `scripts/write_pdf_visual_segmented_gate_summary.ps1 -ReportDir .\output\pdf-visual-release-gate-current\report`，
   生成 `segmented-summary.json`，确认
   `schema = featherdoc.pdf_visual_segmented_gate_summary.v1`、
   `status` / `verdict` 明确写出、
   `full_visual_gate_status = not_complete`、
   `evidence_scope = segmented_visual_gate_auxiliary_only` 和
   `segmented_summary_does_not_replace_full_visual_gate_verdict`。固定标记：
   `pdf_visual_segmented_gate_summary_trace`、
   `pdf_visual_segmented_gate_governance_trace`。
11. 机器结论：`output/pdf-visual-release-gate-current/report/summary.json` 必须包含
   `verdict = pass`、`baselines_count > 0`、`cjk_copy_search_count > 0` 和
   `aggregate_contact_sheet`。
12. 机器准入入口：运行
   `scripts/check_pdf_release_readiness.ps1 -OutputJson .\output\pdf-release-readiness-current\summary.json`，
   确认 `schema = featherdoc.pdf_release_readiness_check.v1`、
   `status = pass`、`release_ready = true`、当前 fresh full gate pass 时
   `verdict = pass` / `warning_count = 0`，以及
   `evidence_scope = persisted_pdf_release_evidence_only`。固定标记：
   `pdf_release_readiness_machine_gate_trace`。该入口只读取现有证据，不运行 CMake、
   CTest、渲染、Office、LibreOffice、浏览器或 PDF 生成；其中
   `pdf_full_fresh_visual_gate.not_completed_in_current_window` 和
   `pdf_full_ctest.not_completed_in_current_window` 只在对应 heavy gate 未完整闭合时
   作为 `verdict = pass_with_warnings` 的 warning，不能被解释为 fresh full gate
   已完成。若已执行
   `scripts/run_pdf_visual_full_gate_guarded.ps1`，该 readiness summary 还必须读取
   `output/pdf-visual-release-gate-current/report/full-visual-gate-guarded-summary.json`，
   保留 `schema = featherdoc.pdf_visual_full_gate_guarded_summary.v1`、
   `visual_full_gate_status`、`visual_full_gate_verdict`、
   `visual_full_gate_full_visual_gate_status`、
   `visual_full_gate_outer_guard_status`、
   `visual_full_gate_outer_guard_timed_out`、
   `visual_full_gate_pass_summary_before_outer_timeout` 和
   `visual_full_gate_attempt_passed_stage_count`、
   `visual_full_gate_attempt_visual_baseline_fresh_rendered_count`、
   `visual_full_gate_attempt_aggregate_contact_sheet_status`，并把这些字段附到
   `pdf_full_fresh_visual_gate.not_completed_in_current_window` warning 上。固定标记：
   `pdf_visual_full_gate_guarded_summary_trace`。如果超时后又重写
   `attempt-summary.json` 并重建 aggregate contact sheet，readiness summary 还必须保留
   `visual_gate_pass_summary_before_outer_timeout`、
   `visual_full_gate_attempt_summary_visual_baseline_fresh_rendered_count` 和
   `visual_full_gate_attempt_summary_aggregate_contact_sheet_status`，并把
   `attempt_summary_visual_baseline_fresh_rendered_count` 与
   `attempt_summary_aggregate_contact_sheet_status` 附到同一个 warning 上。这些
   post-timeout attempt-summary 字段只能说明辅助证据补齐情况，不能替代 fresh full
   visual gate pass。若已生成
   `output/pdf-visual-release-gate-current/report/segmented-summary.json`，该 readiness
   summary 还必须读取 `schema = featherdoc.pdf_visual_segmented_gate_summary.v1`，
   保留 `visual_segmented_gate_status`、`visual_segmented_gate_verdict`、
   `visual_segmented_gate_full_visual_gate_status`、
   `visual_segmented_gate_evidence_scope`、
   `visual_segmented_gate_covered_baseline_count`、
   `visual_segmented_gate_expected_visual_render_count`、
   `visual_segmented_gate_aggregate_contact_sheet_status` 和
   `visual_segmented_gate_aggregate_contact_sheet_bytes`，并把对应的
   `segmented_gate_covered_baseline_count` 与
   `segmented_gate_aggregate_contact_sheet_bytes` 附到
   `pdf_full_fresh_visual_gate.not_completed_in_current_window` warning 上。固定标记：
   `pdf_visual_segmented_gate_summary_trace`。当分段证据满足 44/44 baseline、
   0 failed slice、`aggregate_contact_sheet_status = pass` 和
   `aggregate_rebuild_status = pass` 时，readiness summary 必须写出
   `visual_gate_segmented_full_coverage_evidence = true`，并允许
   `visual_gate_release_evidence_accepted = true`；该状态只解释分段辅助证据覆盖情况，
   不能替代 fresh 非 `FinalizeOnly` full visual gate pass。若 `attempt-summary.json`
   也已补齐阶段全 pass、0 failed、44/44 fresh baseline 和 contact sheet pass，
   release candidate / governance 不再把该边界计入 `warning_count`。若已执行
   release-owner acceptance 继续发版，summary / handoff 必须保留
   `release_owner_acceptance_required = true`、`release_owner_acceptance_policy`、
   `release_owner_acceptance_boundary` 和 `release_owner_acceptance_command_template`。
   当 fresh attempt 仍是 partial/not_complete 时，warning 还必须保留
   `attempt_summary_visual_baseline_fresh_missing_sample_count`、
   `attempt_summary_visual_baseline_resume_slice_offset`、
   `attempt_summary_visual_baseline_resume_slice_limit` 和
   `attempt_summary_visual_baseline_resume_slice_command_template`，让 reviewer
   可以直接使用 `-VisualBaselineSliceOnly` 命令继续补齐，而不是只看到
   “缺失数不为 0”的结论。
   该 acceptance 只接受 segmented full coverage、aggregate contact sheet 非空和
   bounded CTest 等辅助证据；不能改写 `full_visual_gate_status`，也不能删除
   `pdf_full_fresh_visual_gate.not_completed_in_current_window` 的单次 full gate debt。
   固定标记：`pdf_visual_gate_release_owner_acceptance_trace`。若已执行
   `scripts/run_pdf_full_ctest_guarded.ps1`，该 readiness summary 还必须读取
   `output/pdf-ctest-current/summary.json`，保留
   `schema = featherdoc.pdf_full_ctest_guarded_summary.v1`、
   `full_ctest_status`、`full_ctest_verdict`、`full_ctest_outer_guard_status`、
   `full_ctest_completed_test_count`、`full_ctest_not_run_test_count`、
   `full_ctest_completion_percent`、`full_ctest_remaining_test_count` 和
   `full_ctest_zero_failed_tests_observed`，并把这些字段附到
   `pdf_full_ctest.not_completed_in_current_window` warning 上。固定标记：
   `pdf_full_ctest_guarded_summary_trace`。
13. 发布治理：`scripts/run_release_candidate_checks.ps1` 的 summary / final review
   必须消费 PDF visual gate verdict、计数和 contact sheet 路径。
14. 视觉证据：复用或生成的 `aggregate-contact-sheet.png` 必须非空，且抽检不是白图。
15. 轻量验证：至少运行相关 PowerShell 契约测试；资源窗口允许时再运行
   `ctest -R "pdf_" --output-on-failure --timeout 60`。

资源受限时，先使用 bounded PDF CTest helper 记录 smoke/import 证据，避免完整
`pdf_` 套件在单个 60 秒外层保护里被截断后被误记为通过：

如需直接尝试完整 `pdf_` 套件，优先使用受控入口，避免临时命令的日志脱离
release readiness：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_full_gate_guarded.ps1 `
  -BuildDir .\.bpdf-roundtrip-msvc `
  -OutputDir .\output\pdf-visual-release-gate-current
```

summary 必须写出 `schema = featherdoc.pdf_visual_full_gate_guarded_summary.v1`、
`status`、`verdict`、`full_visual_gate_status`、`outer_guard_status`、
`outer_guard_timed_out`、`outer_guard_timeout_seconds`、`pass_summary_before_outer_timeout`、
`attempt_summary_json`、
`attempt_passed_stage_count`、`attempt_visual_baseline_fresh_rendered_count`、
`attempt_aggregate_contact_sheet_status` 和
`guarded_full_visual_gate_attempt_does_not_replace_completed_full_visual_gate`。固定标记：
`pdf_visual_full_gate_guarded_summary_trace`。只有 `status = pass` 且
`full_visual_gate_status = pass`，并且 `outer_guard_status = completed`，或
`outer_guard_status = timed_out_after_pass_summary` 且
`pass_summary_before_outer_timeout = true` 时，才能视为 fresh 非 `FinalizeOnly`
full visual gate 已写出可发布 pass 结论。后者表示渲染 summary 已经 pass，只是外层在进程退出阶段
观察到超时；普通 `outer_guard_status = timed_out` 仍只能作为可追溯 attempt evidence。

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_full_ctest_guarded.ps1 `
  -BuildDir .\.bpdf-roundtrip-msvc `
  -OutputJson .\output\pdf-ctest-current\summary.json
```

summary 必须写出 `schema = featherdoc.pdf_full_ctest_guarded_summary.v1`、
`status`、`verdict`、`full_ctest_status`、`outer_guard_status`、
`outer_guard_timed_out`、`outer_guard_timeout_seconds`、`selected_test_count`、
`completed_test_count`、`not_run_test_count` 和
`guarded_full_ctest_attempt_does_not_replace_completed_full_ctest`。固定标记：
`pdf_full_ctest_guarded_summary_trace`。只有 `status = pass` 且
`outer_guard_status = completed` 才能声明完整 PDF CTest 已完成；`timeout` 只能作为
可追溯 attempt evidence。readiness summary 会额外派生
`full_ctest_completion_percent`、`full_ctest_remaining_test_count` 和
`full_ctest_zero_failed_tests_observed`，用于解释最近一次受控尝试的完成度、剩余
测试数和是否观察到失败。

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 `
  -Subset smoke-import `
  -BuildDir .\.bpdf-roundtrip-msvc `
  -OutputJson .\build\pdf-ctest-bounded-subset-current\summary.json
```

该 helper 固定覆盖 10 个测试：`pdf_document_generator_probe`、`pdf_font_resolver`、
`pdf_text_metrics`、`pdf_text_shaper`、`pdf_document_adapter_font`、`pdf_cli_export`、
`pdf_cli_import`、`pdf_import_structure`、`pdf_import_failure` 和
`pdf_import_table_heuristic`。summary 中的 `status = pass`、`verdict = pass`、
`subset = smoke-import`、`selected_test_count = 10` 和 `ctest_timeout_seconds = 120`
只能证明这条 bounded 子集通过，不替代完整 visual gate 或 `pdf_regression_`
全量样本链。

第二条低资源证据可运行静态契约子集：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 `
  -Subset contract-static `
  -BuildDir .\.bpdf-roundtrip-msvc `
  -OutputJson .\build\pdf-ctest-bounded-contract-static-current\summary.json
```

`contract-static` 固定覆盖 10 个 docs/layout/ctest 契约测试，包括
`pdf_import_docs_contract`、`pdf_ctest_timeout_contract`、`pdf_ctest_label_contract`、
`pdf_bidi_line_layout_static_contract`、`pdf_document_style_gallery_contract`、
`pdf_document_font_matrix_contract`、`pdf_document_table_font_matrix_contract`、
`pdf_cjk_copy_search_matrix_contract`、`pdf_cjk_font_embed_matrix_contract` 和
`pdf_cjk_anchor_font_matrix_boundary_contract`。summary 仍必须写出
`status = pass`、`verdict = pass`、`subset = contract-static`、
`selected_test_count = 10` 和 `ctest_timeout_seconds = 60`。

第三条低资源证据可运行 CJK / RTL flow 静态契约子集：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 `
  -Subset cjk-flow-static `
  -BuildDir .\.bpdf-roundtrip-msvc `
  -OutputJson .\build\pdf-ctest-bounded-cjk-flow-static-current\summary.json
```

`cjk-flow-static` 固定覆盖 10 个 CJK / RTL flow 静态契约测试，包括
`pdf_cjk_font_search_density_flow_contract`、
`pdf_cjk_font_embed_wrap_mix_contract`、
`pdf_cjk_repeated_key_boundary_flow_contract`、
`pdf_cjk_style_overlay_page_flow_contract`、`pdf_cjk_complex_layout_contract`、
`pdf_cjk_image_wrap_stress_contract`、`pdf_cjk_extreme_page_breaks_contract`、
`pdf_cjk_vertical_merge_wrap_cant_split_contract`、`pdf_rtl_bidi_light_contract`
和 `pdf_cjk_list_page_flow_contract`。summary 仍必须写出 `status = pass`、
`verdict = pass`、`subset = cjk-flow-static`、`selected_test_count = 10` 和
`ctest_timeout_seconds = 60`。固定标记：`pdf_ctest_bounded_cjk_flow_static_release_trace`。

第四条低资源证据可运行基础文本 regression 子集：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 `
  -Subset regression-basic-text `
  -BuildDir .\.bpdf-roundtrip-msvc `
  -OutputJson .\build\pdf-ctest-bounded-regression-basic-text-current\summary.json
```

`regression-basic-text` 固定覆盖 10 个真实 `pdf_regression_` 样本测试，包括
`pdf_regression_manifest`、`pdf_regression_single-text`、
`pdf_regression_multi-page-text`、`pdf_regression_font-size-text`、
`pdf_regression_color-text`、`pdf_regression_three-page-text`、
`pdf_regression_landscape-text`、`pdf_regression_title-body-text`、
`pdf_regression_dense-text` 和 `pdf_regression_four-page-text`。summary 仍必须写出
`status = pass`、`verdict = pass`、`subset = regression-basic-text`、
`selected_test_count = 10` 和 `ctest_timeout_seconds = 60`。固定标记：
`pdf_ctest_bounded_regression_basic_text_release_trace`。

第五条低资源证据可运行样式/文档 regression 子集：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 `
  -Subset regression-styled-document `
  -BuildDir .\.bpdf-roundtrip-msvc `
  -OutputJson .\build\pdf-ctest-bounded-regression-styled-document-current\summary.json
```

`regression-styled-document` 固定覆盖 10 个真实 `pdf_regression_` 样式/文档样本测试，包括
`pdf_regression_styled-text`、`pdf_regression_mixed-style-text`、
`pdf_regression_contract-cjk-style`、`pdf_regression_document-contract-cjk-style`、
`pdf_regression_underline-text`、`pdf_regression_strikethrough-text`、
`pdf_regression_superscript-subscript-text`、
`pdf_regression_style-superscript-subscript-text`、
`pdf_regression_document-style-gallery-text` 和
`pdf_regression_document-font-matrix-text`。summary 仍必须写出 `status = pass`、
`verdict = pass`、`subset = regression-styled-document`、`selected_test_count = 10`
和 `ctest_timeout_seconds = 60`。固定标记：
`pdf_ctest_bounded_regression_styled_document_release_trace`。

第六条低资源证据可运行真实业务样本 regression 子集：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 `
  -Subset regression-business-samples `
  -BuildDir .\.bpdf-roundtrip-msvc `
  -OutputJson .\build\pdf-ctest-bounded-regression-business-samples-current\summary.json
```

`regression-business-samples` 固定覆盖 10 个真实 `pdf_regression_`
业务样本测试，覆盖合同、发票 / 报价单、图文报告、长文档和多 section 文档：
`pdf_regression_contract-cjk-style`、`pdf_regression_document-contract-cjk-style`、
`pdf_regression_invoice-grid-text`、`pdf_regression_document-invoice-table-text`、
`pdf_regression_image-report-text`、`pdf_regression_cjk-image-report-text`、
`pdf_regression_document-cjk-image-wrap-stress-text`、`pdf_regression_long-report-text`、
`pdf_regression_document-long-flow-text` 和 `pdf_regression_sectioned-report-text`。
summary 仍必须写出 `status = pass`、`verdict = pass`、
`subset = regression-business-samples`、`selected_test_count = 10` 和
`ctest_timeout_seconds = 60`。固定标记：
`pdf_ctest_bounded_regression_business_samples_release_trace`、
`pdf_real_business_sample_release_entry_trace`。

第七条低资源证据可运行表格布局 regression 子集：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 `
  -Subset regression-table-layout `
  -BuildDir .\.bpdf-roundtrip-msvc `
  -OutputJson .\build\pdf-ctest-bounded-regression-table-layout-current\summary.json
```

`regression-table-layout` 固定覆盖 10 个真实 `pdf_regression_` 表格布局样本测试：
`pdf_regression_table-like-grid-text`、`pdf_regression_invoice-grid-text`、
`pdf_regression_document-table-semantics-text`、
`pdf_regression_document-invoice-table-text`、
`pdf_regression_document-table-header-footer-variants-text`、
`pdf_regression_document-table-wrap-flow-text`、
`pdf_regression_document-table-cant-split-text`、
`pdf_regression_document-table-merged-cells-text`、
`pdf_regression_document-table-merged-header-repeat-text` 和
`pdf_regression_document-table-merged-header-footer-variants-text`。summary 仍必须写出
`status = pass`、`verdict = pass`、`subset = regression-table-layout`、
`selected_test_count = 10` 和 `ctest_timeout_seconds = 60`。固定标记：
`pdf_ctest_bounded_regression_table_layout_release_trace`。该子集只声明已实际运行的
非 skipped 表格样本，不覆盖当前环境会 skip 的 CJK 表格变体。

所有 bounded subset summary 都必须同时写出 `skipped_test_count = 0`；
`scripts/run_pdf_ctest_bounded_subset.ps1` 遇到任何 `***Skipped` CTest 项时会把
`status` / `verdict` 标记为 `fail` 并返回失败，避免 skipped 测试被误当作发布证据。

边界外不承诺：

- 不承诺 general PDF-to-Word，也不承诺 OCR / 扫描件或任意视觉精确还原。
- 还没有 `AST → PDFio` 完整翻译层；复杂分页、图片锚点/裁剪/环绕和发布级合同样式视觉 baseline 还没收口
- RTL / 竖排文字的方向模型还需要继续专项收口
- 还没有 `PDFium → AST` 文档结构重建
- 还需要继续扩充更多外部真实 PDF 样本回归集
- 发布级 CJK 字体许可证 / 捆绑策略、复杂表格分页和 PNG baseline 门禁还在专项推进中
