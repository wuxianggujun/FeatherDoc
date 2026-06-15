PDF 发布准入清单
================

本页是 PDF 线每次 release 前的固定检查入口。目标是确认当前 PDF 能在
明确边界内发布，而不是承诺通用 PDF 转 Word、完整 Word 排版引擎、
OCR 或任意视觉精确还原。

准入检查
--------

1. 状态证据已同步：

   * 当前 preflight、governance 和 full gate 证据状态必须在脚本 summary
     和 release/reviewer 材料里写清楚。
   * 若继续维护 ``docs/pdf_visual_validation_status_zh.rst`` 这份历史状态快照，
     不得停留在旧的 ``blocked`` / ``not_ready`` 叙事。

2. 样本统计已同步：

   * ``test/pdf_regression_manifest.json`` 当前包含 90 个样本。
   * 其中 42 个样本标记 ``expect_visual_baseline=true``。
   * 其中 43 个样本标记 ``expect_cjk=true``。

3. 真实业务样本覆盖已保持：

   * 合同：``contract-cjk-style``、``document-contract-cjk-style``。
   * 发票 / 报价单：``invoice-grid-text``、``document-invoice-table-text``。
   * 图文报告：``image-report-text``、``cjk-image-report-text``、
     ``document-cjk-image-wrap-stress-text``。
   * 长文档：``long-report-text``、``document-long-flow-text``。
   * 多 section / 页眉页脚 / 多页流：``sectioned-report-text``、
     ``header-footer-text``、``document-cjk-table-wrap-page-flow-text``。
   * 上述 manifest ID 必须继续由
     ``test/pdf_real_business_sample_manifest_contract_test.ps1`` 校验存在性，并且
     ``regression-business-samples`` bounded subset 必须保持固定的 10 个
     ``pdf_regression_`` 测试顺序。固定标记：
     ``pdf_real_business_sample_release_entry_trace``。

4. 预检通过：

   .. code-block:: powershell

      powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\check_pdf_visual_release_gate_preflight.ps1 `
        -BuildDir .\.bpdf-roundtrip-msvc `
        -OutputJson .\output\pdf-visual-release-gate-preflight\summary.json

   预期 raw summary 表达为 ``blocking_checks = []``，且
   ``blocking_summary.blocking_check_count = 0``；如果同时生成 preflight
   governance report，才应在 governance summary 中看到
   ``preflight_ready = true``。
   当前 ``dev`` 最近一次轻量复核还必须能解释为真实构建证据：
   ``status = ready``、``evidence_kind = real_build``、``output_gap_count = 0``、
   ``missing_output_count = 0``、``pdf_dependency_inputs_status = ready``、
   ``pdf_build_options_enabled = true``、``pdfio_dependency_ready = true``、
   ``pdfium_dependency_ready = true``、``selected_pdfium_provider = prebuilt``，并且
   ``ctest_list_contains_pdf_gate_tests = pass``。

5. 完整 visual gate 有可复核证据：

   .. code-block:: powershell

      powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 `
        -BuildDir .\.bpdf-roundtrip-msvc `
        -OutputDir .\output\pdf-visual-release-gate-current

   资源受限时可用现有产物走复核路径：

   .. code-block:: powershell

      powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 `
        -BuildDir .\.bpdf-roundtrip-msvc `
        -OutputDir .\output\pdf-visual-release-gate-current `
        -FinalizeOnly `
        -SkipPreflight

   ``report/summary.json`` 必须包含 ``status = pass``、``verdict = pass``、
   ``visual_baseline_manifest_count = 42``、``baselines_count = 44``、
   ``cjk_manifest_count = 43``、``cjk_copy_search_count = 43`` 和
   ``aggregate_contact_sheet``。其中 ``visual_baseline_manifest_count`` 对应
   manifest 中 ``expect_visual_baseline=true`` 的样本数，``baselines_count``
   对应 full gate 当前渲染并汇总的 baseline 产物数。

   如果当前是干净检出或新 worktree，且本地 ``output/`` 没有持久化 visual gate
   产物，只能把本轮状态记录为 ``local_persisted_visual_artifacts_missing``。
   这种状态不是源码失败，也不能替代完整 visual gate；低资源轮次只能执行只读
   preflight/readiness 契约和治理报告测试，等待 PDFio/PDFium、PDF build options、
   baseline/render 输入以及 contact-sheet 产物齐备后，再进入 ``-FinalizeOnly``
   复核或 full visual gate。

   发布证据允许三种可解释形态：资源受限复核路径必须显示
   ``finalize_only = true`` 和 ``skip_preflight = true``；fresh 非
   ``FinalizeOnly`` full visual gate 完整通过时，必须由
   ``run_pdf_visual_full_gate_guarded.ps1`` 写出
   ``full-visual-gate-guarded-summary.json``，并同时满足 ``status = pass``、
   ``verdict = pass``、``full_visual_gate_status = pass``、
   且满足 ``outer_guard_status = completed`` / ``outer_guard_timed_out = false``，
   或 ``outer_guard_status = timed_out_after_pass_summary`` /
   ``pass_summary_before_outer_timeout = true``。后者只表示 pass summary 已先于
   外层退出保护落盘，不能和普通 ``outer_guard_status = timed_out`` 混淆。
   如果单次 fresh full gate 被 60 秒外层保护截断，但分段 visual gate 已经给出
   44/44 baseline 覆盖、0 failed slice、contact sheet pass 和 aggregate rebuild pass，
   ``check_pdf_release_readiness.ps1`` 必须把它消化为
   ``visual_gate_segmented_full_coverage_evidence = true``。该路径只能证明分段
   visual evidence 已完整覆盖，不能把 ``full_visual_gate_status = not_complete``
   改写为 full gate pass。``check_pdf_release_readiness.ps1`` 还必须把 fresh full gate
   完整通过消化为
   ``visual_gate_fresh_full_guarded_evidence = true`` 和
   ``visual_gate_release_evidence_accepted = true``。

6. 机器准入结论已生成：

   .. code-block:: powershell

      powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\check_pdf_release_readiness.ps1 `
        -OutputJson .\output\pdf-release-readiness-current\summary.json

   该入口只读取现有 preflight、visual gate、manifest、contact-sheet 和本清单，
   不运行 CMake、CTest、Ninja、MSBuild、Office、LibreOffice、浏览器、PDF 渲染或
   PDF 生成。summary 必须包含 ``schema = featherdoc.pdf_release_readiness_check.v1``、
   ``status = pass``、``release_ready = true``、
   ``evidence_scope = persisted_pdf_release_evidence_only``、
   ``visual_gate_release_evidence_accepted`` 和
   ``visual_gate_fresh_full_guarded_evidence``、
   ``visual_gate_pass_summary_before_outer_timeout``、
   ``visual_gate_segmented_full_coverage_evidence``。
   summary 还必须保留可打开的证据路径字段：
   ``preflight_summary_json_display``、``visual_gate_summary_json_display``、
   ``visual_full_gate_guarded_summary_json_display``、
   ``visual_segmented_gate_summary_json_display``、``full_ctest_summary_json_display`` 和
   ``full_ctest_remaining_summary_json_display``，避免 reviewer 只看到结论字段却无法
   直接打开对应的 preflight、visual gate、segmented gate 或 CTest 证据。
   当 fresh full visual gate 和
   full PDF CTest 组合证据都已闭合时，``verdict`` 应为 ``pass`` 且
   ``warning_count = 0``；如果任一重型证据只达到可追溯但未完整闭合，
   ``verdict`` 才应为 ``pass_with_warnings``，并携带
   ``pdf_full_fresh_visual_gate.not_completed_in_current_window`` 或
   ``pdf_full_ctest.not_completed_in_current_window``。固定标记：
   ``pdf_release_readiness_machine_gate_trace``。

   如果已经运行 fresh 非 ``FinalizeOnly`` full visual gate 的受控尝试，必须通过
   ``scripts/run_pdf_visual_full_gate_guarded.ps1`` 写出
   ``output/pdf-visual-release-gate-current/report/full-visual-gate-guarded-summary.json``，
   并让 readiness summary 保留
   ``schema = featherdoc.pdf_visual_full_gate_guarded_summary.v1``、
   ``visual_full_gate_status``、``visual_full_gate_verdict``、
   ``visual_full_gate_full_visual_gate_status``、
   ``visual_full_gate_outer_guard_status``、
   ``visual_full_gate_outer_guard_timed_out``、
   ``visual_full_gate_pass_summary_before_outer_timeout`` 和
   ``visual_full_gate_attempt_passed_stage_count``、
   ``visual_full_gate_attempt_visual_baseline_fresh_rendered_count``、
   ``visual_full_gate_attempt_aggregate_contact_sheet_status``。当
   ``outer_guard_status = timed_out`` 或 ``full_visual_gate_status != pass`` 时，
   ``pdf_full_fresh_visual_gate.not_completed_in_current_window`` warning 必须携带这些计数，
   不能只留下人工日志路径。固定标记：
   ``pdf_visual_full_gate_guarded_summary_trace``。
   如果超时后又运行 ``write_pdf_visual_gate_attempt_summary.ps1`` 和 aggregate
   contact-sheet rebuild 补齐辅助证据，readiness summary 还必须保留
   ``visual_full_gate_attempt_summary_status``、
   ``visual_full_gate_attempt_summary_visual_baseline_fresh_rendered_count``、
   ``visual_full_gate_attempt_summary_aggregate_contact_sheet_status``，并把
   ``attempt_summary_visual_baseline_fresh_rendered_count`` 与
   ``attempt_summary_aggregate_contact_sheet_status`` 附到同一个 warning 上。该
   warning 还必须携带
   ``attempt_summary_visual_baseline_fresh_missing_sample_count``、
   ``attempt_summary_visual_baseline_resume_slice_offset``、
   ``attempt_summary_visual_baseline_resume_slice_limit`` 和
   ``attempt_summary_visual_baseline_resume_slice_command_template``，让 reviewer
   能直接看到下一段 ``-VisualBaselineSliceOnly`` 恢复命令，而不是从
   ``attempt-summary.json`` 反推。
   post-timeout attempt-summary evidence 只能解释辅助证据补齐情况，不能替代
   fresh full visual gate pass。

   如果已经生成分段 visual gate 汇总，必须通过
   ``scripts/write_pdf_visual_segmented_gate_summary.ps1`` 写出
   ``output/pdf-visual-release-gate-current/report/segmented-summary.json``，并让
   readiness summary 保留
   ``schema = featherdoc.pdf_visual_segmented_gate_summary.v1``、
   ``visual_segmented_gate_status``、``visual_segmented_gate_verdict``、
   ``visual_segmented_gate_full_visual_gate_status``、
   ``visual_segmented_gate_evidence_scope``、
   ``visual_segmented_gate_covered_baseline_count``、
   ``visual_segmented_gate_expected_visual_render_count``、
   ``visual_segmented_gate_aggregate_contact_sheet_status`` 和
   ``visual_segmented_gate_aggregate_contact_sheet_bytes``。当 fresh full visual gate
   尚未完整通过时，``pdf_full_fresh_visual_gate.not_completed_in_current_window``
   warning 必须同步携带 ``segmented_gate_covered_baseline_count`` 和
   ``segmented_gate_aggregate_contact_sheet_bytes``，让 reviewer 不必打开 ignored JSON
   才能看到分段覆盖情况。固定标记：
   ``pdf_visual_segmented_gate_summary_trace``。当该分段证据满足 44/44 baseline、
   0 failed slice、``aggregate_contact_sheet_status = pass`` 和
   ``aggregate_rebuild_status = pass`` 时，readiness 可以写出
   ``visual_gate_segmented_full_coverage_evidence = true`` 并接受为发布证据；
   warning 仍必须保留 fresh full gate 未完整闭合的事实。这些字段只能解释
   ``segmented_visual_gate_auxiliary_only`` 辅助证据，不能替代 fresh 非
   ``FinalizeOnly`` full visual gate pass。

   如果已经运行完整 PDF CTest 的受控尝试，必须通过
   ``scripts/run_pdf_full_ctest_guarded.ps1`` 写出
   ``output/pdf-ctest-current/summary.json``，并让 readiness summary 保留
   ``schema = featherdoc.pdf_full_ctest_guarded_summary.v1``、
   ``full_ctest_status``、``full_ctest_verdict``、
   ``full_ctest_outer_guard_status``、``full_ctest_outer_guard_timed_out``、
   ``full_ctest_completed_test_count``、``full_ctest_not_run_test_count``、
   ``full_ctest_completion_percent``、``full_ctest_remaining_test_count`` 和
   ``full_ctest_zero_failed_tests_observed``。
   当 ``outer_guard_status = timed_out`` 或 ``status != pass`` 时，
   ``pdf_full_ctest.not_completed_in_current_window`` warning 必须携带这些计数和派生
   结论，不能只留下人工日志路径。固定标记：
   ``pdf_full_ctest_guarded_summary_trace``。

   如果 60 秒外层保护已经留下一个接近完成但超时的完整 PDF CTest 尝试，可以用
   ``scripts/run_pdf_ctest_remaining_guarded.ps1`` 只补跑该尝试未完成的尾段测试，并写出
   ``output/pdf-ctest-current/remaining-summary.json``。该 remaining summary 必须使用
   ``schema = featherdoc.pdf_ctest_remaining_guarded_summary.v1``，保留
   ``selected_test_count``、``completed_test_count``、``combined_completed_test_count``、
   ``combined_not_run_test_count``、``combined_tail_covers_previous_remaining`` 和
   ``remaining_ctest_tail_evidence_does_not_replace_single_full_ctest`` 边界。
   ``scripts/check_pdf_release_readiness.ps1`` 必须消费该 tail summary，并在
   readiness summary 中显式写出 ``full_ctest_combined_evidence_completed`` 和
   ``full_ctest_remaining_tail_covers_previous_remaining``；该组合证据只能证明
   “受控 full attempt + tail 补跑”共同覆盖，不能伪装成单次完整 ``ctest -R pdf_``。
   固定标记：``pdf_ctest_remaining_guarded_summary_trace``。

7. 发布治理已消费 PDF 结论：

   * ``scripts/run_release_candidate_checks.ps1`` 的 summary / final review
     必须显示 PDF visual gate verdict、baseline count、CJK copy/search count
     和 contact sheet 路径。
   * ``final_review.md`` 中 ``PDF visual gate verdict:`` 行必须直接携带
     ``pass`` 或 ``fail``，``PDF visual gate summary:`` 行必须直接携带
     ``summary.json`` 路径，``PDF visual gate contact sheet:`` 行必须直接携带
     ``aggregate-contact-sheet.png`` 路径；不能让其它段落里的同名路径替代。
     summary/contact-sheet 两条 evidence 还必须保留在 ``## Key outputs`` 区段内，
     不能让 detached notes 补齐发布入口输出证据。
     ``PDF visual gate:``、verdict、counts、manifest counts 和 finalizable
     必须保留在同一个 step-status Markdown list run。
     固定标记：``line_scoped_final_review_pdf_visual_trace``、
     ``line_scoped_final_review_pdf_visual_verdict``、
     ``block_scoped_final_review_pdf_visual_step_status``、
     ``section_scoped_final_review_pdf_visual_key_outputs``。
   * ``final_review.md`` 同步展示 attempt 辅助证据时，
     ``PDF visual gate attempt:``、verdict、full status、stages、pdf_regression
     和 render 必须保留在 ``## Step status`` 的同一连续 list run；
     ``PDF visual gate attempt summary:`` 与 contact sheet 必须保留在
     ``## Key outputs``，且路径必须直接携带 ``attempt-summary.json`` 和
     ``aggregate-contact-sheet.png``。detached notes 不能补齐这些入口证据。
     固定标记：``pdf_visual_gate_attempt_final_review_material_safety_trace``。
   * ``final_review.md`` 同步展示 segmented gate 辅助证据时，
     ``PDF visual segmented gate:``、verdict、full status、scope、slices 和 coverage
     必须保留在 ``## Step status`` 的同一连续 list run，scope 行必须直接携带
     ``segmented_visual_gate_auxiliary_only``；summary/contact sheet 必须保留在
     ``## Key outputs``，且路径必须直接携带 ``segmented-summary.json`` 和
     ``aggregate-contact-sheet.png``。detached notes 不能补齐这些入口证据。
     固定标记：``pdf_visual_segmented_gate_final_review_material_safety_trace``。
   * ``release_handoff.md`` 中 ``PDF visual gate verdict:`` 行必须直接携带
     ``pass`` 或 ``fail``，``PDF visual gate summary:`` 行必须直接携带
     ``summary.json`` 路径，``PDF visual gate aggregate contact sheet:`` 行必须
     直接携带 ``aggregate-contact-sheet.png`` 路径；不能让 detached notes
     或其它段落替代当前 handoff 证据；status、verdict、counts 和 contact sheet
     必须保留在同一个 PDF visual gate Markdown list run。
     固定标记：``line_scoped_release_handoff_pdf_visual_trace``、
     ``line_scoped_release_handoff_pdf_visual_verdict``、
     ``block_scoped_release_handoff_pdf_visual_evidence``。
   * ``release_summary.zh-CN.md`` 的 PDF visual gate 短摘要行必须直接携带
     ``summary.json``、``aggregate-contact-sheet.png``、``verdict=pass`` 或
     ``verdict=fail``、CJK count 和 visual baseline count；``release_body.zh-CN.md`` 的
     PDF visual gate summary / contact-sheet 行也必须直接携带对应路径；
     ``PDF visual gate verdict`` 行必须直接携带 ``pass`` 或 ``fail``；
     status、verdict、counts 和 contact sheet 必须保留在同一个 release body
     PDF visual gate Markdown list run，不能让 detached notes 或其它段落补齐
     发布说明证据。
     固定标记：``line_scoped_release_note_pdf_visual_trace``、
     ``line_scoped_release_note_pdf_visual_verdict``、
     ``line_scoped_release_bundle_pdf_visual_summary``、
     ``block_scoped_release_body_pdf_visual_evidence``。
   * ``START_HERE.md`` 和 ``ARTIFACT_GUIDE.md`` 的 PDF visual gate summary、
     CJK count、visual baseline count 和 ``PDF visual gate aggregate contact sheet:``
     必须保留在同一个 Markdown list run；``REVIEWER_CHECKLIST.md`` 的 summary 确认行
     必须直接携带 ``summary.json`` 与 count，contact-sheet 确认行必须直接携带
     ``aggregate-contact-sheet.png``。固定标记：
     ``block_scoped_entry_pdf_visual_evidence``、
     ``line_scoped_reviewer_checklist_pdf_visual_evidence``。
   * ``full_visual_gate_status`` 不能停留在“governance 未消化”的唯一结论；
     release reviewer 应同时看到 full gate summary verdict。
   * ``release_governance_handoff.md`` 中每个 ``source_report:`` block
     必须把 ``full_visual_gate_status = pass`` 或 ``fail``、PDF visual gate
     verdict、finalizable、summary、contact sheet
     和 42/43/44 计数放在同一块里；不能让块外的
     ``aggregate-contact-sheet.png`` 替代当前 source report 的证据。
     固定标记：``block_scoped_pdf_visual_gate_handoff_trace``。
   * bounded CTest 只作为资源受限时的辅助证据进入同一个
     ``source_report:`` block，必须包含
     ``pdf_bounded_ctest_summary_count``、``pdf_bounded_ctest_pass_count``、
     ``pdf_bounded_ctest_skipped_test_count``、
     ``pdf_bounded_ctest_selected_test_count``、``pdf_bounded_ctest_subsets`` 和
     ``pdf_bounded_ctest_summary_json_display``，并保留
     ``pdf_bounded_ctest_import_diagnostics_contract_tests``、
     ``pdf_bounded_ctest_import_diagnostics_contract_fields`` 和
     ``pdf_bounded_ctest_import_negative_boundary_contract_cases``；不能替代 full visual gate
     verdict。固定标记：``pdf_bounded_ctest_governance_trace``、
     ``pdf_bounded_ctest_source_report_block_trace``。
     ``release_blocker_rollup.md`` 的 ``Source Report Contracts`` 也必须在同一个
     ``featherdoc.release_candidate_summary`` list block 中保留 PDF visual gate status、
     ``full_visual_gate_status``、verdict、finalizable、summary/contact sheet、
     CJK/baseline 计数和 bounded CTest 字段，不能由 detached notes 或非
     release-candidate schema 补齐。固定标记：
     ``pdf_visual_gate_rollup_material_safety_trace``。
   * 非 ``FinalizeOnly`` full visual gate 被 60 秒保护截断时，必须运行
     ``scripts/write_pdf_visual_gate_attempt_summary.ps1`` 生成
     ``attempt-summary.json``。该辅助证据必须在 release governance 中保留
     ``pdf_visual_gate_attempt_status``、
     ``pdf_visual_gate_attempt_verdict``、
     ``pdf_visual_gate_attempt_full_visual_gate_status``、
     ``pdf_visual_gate_attempt_evidence_scope``、
     ``pdf_visual_gate_attempt_outer_guard_status``、
     ``pdf_visual_gate_attempt_outer_guard_timed_out``、
     ``pdf_visual_gate_attempt_outer_guard_timeout_seconds``、
     ``pdf_visual_gate_attempt_pdf_regression_skipped_test_count`` 和
     ``pdf_visual_gate_attempt_visual_baseline_render_status``；当
     ``evidence_scope = bounded_attempt_auxiliary_only`` 或
     ``verdict = not_complete`` 时，不能替代 full visual gate verdict。固定标记：
     ``pdf_visual_gate_attempt_summary_trace``、
     ``pdf_visual_gate_attempt_governance_trace``、
     ``pdf_visual_gate_attempt_material_safety_trace``。
     ``release_blocker_rollup.md`` 的 ``Source Report Contracts`` 也必须把
     attempt status、verdict、full status、``bounded_attempt_auxiliary_only`` scope、
     ``attempt-summary.json``、outer guard ``timed_out`` / ``true`` / ``60``、
     regression/CJK/render 计数和 contact sheet 保留在同一个 release-candidate
     list block。固定标记：
     ``pdf_visual_gate_attempt_rollup_material_safety_trace``。
     如果外层 60 秒保护截断本次尝试，
     必须保留 ``outer_guard_status = timed_out``、
     ``outer_guard_timed_out = true`` 和 ``outer_guard_timeout_seconds = 60``；
     这些字段只解释外层保护状态，不能替代 full visual gate verdict。固定标记：
     ``pdf_visual_gate_attempt_outer_guard_trace``。
     从 2026-05-26 起，``scripts/run_release_candidate_checks.ps1`` 还必须把这类
     fresh attempt 未完成且辅助证据仍不完整的事实上提为顶层 ``warnings[]``，稳定 id 为
     ``pdf_visual_gate_attempt.incomplete_fresh_render``。warning 必须直接携带
     ``attempt-summary.json`` 路径、outer guard ``timed_out`` / ``true`` / ``60``、
     ``visual_gate_release_evidence_accepted``、
     ``visual_gate_fresh_full_guarded_evidence``、
     ``visual_gate_pass_summary_before_outer_timeout``、
     ``visual_gate_segmented_full_coverage_evidence`` 和 ``visual_gate_finalize_only``，
     并明确说明当前发布结论是依赖 segmented full-coverage、fresh guarded full gate
     还是 explicit ``FinalizeOnly`` 复核证据；它是 reviewer-facing warning，
     不是新的 release blocker。
     若 readiness 已接受 segmented full-coverage，且 ``attempt-summary.json`` 已补齐
     阶段全 pass、0 failed、44/44 fresh baseline 和 contact sheet pass，
     release candidate / governance 不再把这条边界计入 ``warning_count``。
     ``final_review.md`` 中的 attempt summary/contact sheet reviewer 入口也必须经过
     line/section scoped 审计。固定标记：
     ``pdf_visual_gate_attempt_final_review_material_safety_trace``。
     当 ``attempt-summary.json`` 已经写出
     ``visual_baseline_resume_slice_offset`` 和
     ``visual_baseline_resume_slice_limit`` 时，优先运行
     ``scripts/run_pdf_visual_segmented_resume.ps1`` 自动规划和执行 tail 切片。
     该 helper 必须写出 ``segmented-resume-summary.json``，schema 为
     ``featherdoc.pdf_visual_segmented_resume_summary.v1``，并保留
     ``evidence_scope = visual_segmented_resume_auxiliary_only``、
     ``full_visual_gate_status = not_complete``、
     ``resume_tail_fully_planned``、``total_resume_slice_count``、
     ``planned_slice_count``、``executed_slice_count``、``passed_slice_count``、
     ``failed_slice_count``、``timeout_slice_count``、
     ``aggregate_rebuild_status``、``segmented_summary_status`` 和
     ``segmented_resume_does_not_replace_full_visual_gate_verdict``。固定标记：
     ``pdf_visual_segmented_resume_summary_trace``。该证据只能说明 resume tail
     辅助闭合，不能替代 fresh full visual gate verdict。
   * visual baseline render 阶段如果无法在单个 60 秒外层保护内完成，可以补跑
     ``scripts/run_pdf_visual_release_gate.ps1 -VisualBaselineSliceOnly``。
     该模式必须携带 ``VisualBaselineOffset`` 和 ``VisualBaselineLimit``，生成
     ``visual-baseline-slice-offset-<n>-limit-<m>-summary.json`` 和对应 contact sheet；
     summary 必须包含 ``schema = featherdoc.pdf_visual_baseline_slice.v1``、
     ``evidence_scope = visual_baseline_slice_only``、
     ``full_visual_gate_status = not_complete`` 和
     ``slice_summary_does_not_replace_full_visual_gate_verdict``。固定标记：
     ``pdf_visual_baseline_slice_summary_trace``。切片证据只能补强 fresh baseline render
     计数，不能替代 full visual gate verdict。
   * fresh full gate 在 aggregate contact sheet 构建成功后，必须先把核心 pass
     summary 写入 ``report/summary.json``，再追加较大的 baseline / CJK 详情 payload。
     该路径必须保留 ``pdf_visual_gate_core_pass_summary_trace``、
     ``summary_detail_payload_included``、``summary_detail_status`` 和
     ``core_pass_written_before_detail_payload``。如果 60 秒外层保护正好在详情
     payload 序列化阶段触发，``run_pdf_visual_full_gate_guarded.ps1`` 只有在同轮
     stage 已全 pass 时才能输出 ``outer_guard_status = timed_out_after_pass_summary``。
   * full gate 的 baseline 渲染默认传递 ``--skip-contact-sheet`` 给
     ``scripts/render_pdf_pages.py``，只生成页面 PNG 和 sample summary；release
     reviewer-facing 可视化入口仍统一使用 ``report/aggregate-contact-sheet.png``。
     sample summary 必须保留 ``contact_sheet_skipped``，用于说明单样本
     ``contact-sheet.png`` 不是 full gate 硬前提。
   * baseline 已经存在但 aggregate contact sheet 需要在 60 秒外层保护内单独刷新时，
     可以运行 ``scripts/run_pdf_visual_release_gate.ps1 -RebuildAggregateContactSheetOnly``。
     该模式只读取现有 baseline summary / ``page-01.png``，重建
     ``report/aggregate-contact-sheet.png``，并写出
     ``aggregate-contact-sheet-rebuild-summary.json``；summary 必须包含
     ``schema = featherdoc.pdf_visual_aggregate_contact_sheet_rebuild.v1``、
     ``evidence_scope = aggregate_contact_sheet_rebuild_only``、
     ``full_visual_gate_status = not_complete`` 和
     ``aggregate_rebuild_summary_does_not_replace_full_visual_gate_verdict``。固定标记：
     ``pdf_visual_aggregate_contact_sheet_rebuild_trace``。该证据不能替代 full visual gate
     verdict。
   * attempt、visual baseline slice 与 aggregate contact-sheet rebuild 已经形成完整分段链路时，
     必须运行 ``scripts/write_pdf_visual_segmented_gate_summary.ps1`` 生成
     ``segmented-summary.json``。summary 必须包含
     ``schema = featherdoc.pdf_visual_segmented_gate_summary.v1``、
     明确的 ``status`` / ``verdict``、
     ``full_visual_gate_status = not_complete``、
     ``evidence_scope = segmented_visual_gate_auxiliary_only`` 和
     ``segmented_summary_does_not_replace_full_visual_gate_verdict``；即使
     ``status = pass`` 且 ``verdict = pass``，也只能证明分段辅助证据完整。
     release governance 必须同块保留
     ``pdf_visual_segmented_gate_status``、
     ``pdf_visual_segmented_gate_verdict``、
     ``pdf_visual_segmented_gate_full_visual_gate_status``、
     ``pdf_visual_segmented_gate_evidence_scope``、
     ``pdf_visual_segmented_gate_boundary``、
     ``pdf_visual_segmented_gate_summary_json_display``、
     ``pdf_visual_segmented_gate_slice_summary_count``、
     ``pdf_visual_segmented_gate_slice_pass_count``、
     ``pdf_visual_segmented_gate_slice_failed_count``、
     ``pdf_visual_segmented_gate_covered_baseline_count``、
     ``pdf_visual_segmented_gate_expected_visual_render_count``、
     ``pdf_visual_segmented_gate_attempt_stage_count``、
     ``pdf_visual_segmented_gate_attempt_passed_stage_count``、
     ``pdf_visual_segmented_gate_visual_baseline_render_status``、
     ``pdf_visual_segmented_gate_aggregate_contact_sheet_status``、
     ``pdf_visual_segmented_gate_aggregate_contact_sheet_display`` 和
     ``pdf_visual_segmented_gate_aggregate_contact_sheet_bytes``、
     ``pdf_visual_segmented_gate_aggregate_rebuild_status`` 和
     ``pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count``。固定标记：
     ``pdf_visual_segmented_gate_summary_trace``、
     ``pdf_visual_segmented_gate_governance_trace``。该证据不能替代 full visual gate
     verdict。
     ``release_governance_handoff.md`` 中这些 segmented gate 字段必须继续与
     ``schema=featherdoc.release_candidate_summary`` 的 ``source_report:`` 保持同块，
     不能由 detached notes 补齐。固定标记：
     ``pdf_visual_segmented_gate_material_safety_trace``。
     ``release_blocker_rollup.md`` 的 ``Source Report Contracts`` 也必须把
     segmented status、verdict、full status、``segmented_visual_gate_auxiliary_only``
     scope/boundary、``segmented-summary.json``、slice coverage、contact sheet
     bytes/status 和 aggregate rebuild 计数保留在同一个 release-candidate
     list block。固定标记：
     ``pdf_visual_segmented_gate_rollup_material_safety_trace``。
     ``final_review.md`` 中的 segmented summary/contact sheet reviewer 入口也必须经过
     line/section scoped 审计。固定标记：
     ``pdf_visual_segmented_gate_final_review_material_safety_trace``。
   * ``release_assets_manifest.json`` 中的 ``pdf_visual_gate_evidence`` 必须保留
     ``cjk_manifest_count >= 43``、``visual_baseline_manifest_count >= 42``，
     并且 ``verdict = pass`` 时 ``cjk_missing_text_count = 0``；不能只依赖
     ``summary.json`` 路径或 ``full_visual_gate_status`` 推断样本覆盖完整。
     固定标记：``manifest_scoped_pdf_visual_gate_count_trace``。
   * release 入口物料必须直接指向本页作为固定 PDF 发布准入入口：
     ``START_HERE.md``、``ARTIFACT_GUIDE.md`` 和 ``REVIEWER_CHECKLIST.md``
     都必须保留 ``docs/pdf_release_readiness_checklist_zh.rst``。本页要与
     ``pdf_visual_gate_evidence``、``pdf_bounded_ctest_evidence`` 和
     ``manifest_signoff_entrypoints`` 一起被 reviewer 看到，避免 PDF 发布结论
     只散落在脚本或生成产物里。固定标记：
     ``release_entry_pdf_readiness_checklist_trace``。
   * preflight governance 中的 ``not_run_by_preflight_governance`` 只能解释
     preflight-governance 报告自身没有重跑 full gate；如果同轮
     ``FinalizeOnly`` summary 已给出 ``verdict = pass``，发布结论应以
     full gate summary 和 contact sheet 证据为准。

8. 可视化证据非空：

   * ``output/pdf-visual-release-gate-current/report/aggregate-contact-sheet.png``
     必须存在且非空。
   * 若本轮没有重跑渲染，必须说明使用了 ``-FinalizeOnly -SkipPreflight``
     复核现有产物。
   * 最近一次复核的 contact sheet 参考尺寸为 ``912x14566``，大小为
     ``1822428`` bytes；后续 release 可接受尺寸变化，但必须记录非空、
     可读和未丢失路径的证据。

9. 轻量验证已跑：

   .. code-block:: powershell

      powershell -NoProfile -ExecutionPolicy Bypass -File .\test\pdf_visual_validation_status_docs_contract_test.ps1 -RepoRoot .
      powershell -NoProfile -ExecutionPolicy Bypass -File .\test\release_candidate_visual_verdict_test.ps1 -RepoRoot . -WorkingDir .\tmp\release_candidate_visual_verdict
      powershell -NoProfile -ExecutionPolicy Bypass -File .\test\release_note_bundle_visual_verdict_metadata_test.ps1 -RepoRoot . -WorkingDir .\tmp\release_note_bundle_visual_verdict_metadata
      powershell -NoProfile -ExecutionPolicy Bypass -File .\test\pdf_real_business_sample_manifest_contract_test.ps1 -RepoRoot .

   资源受限时先运行 bounded PDF CTest 子集，而不是把完整 ``pdf_`` 套件塞进
   单个 60 秒外层保护里：

   .. code-block:: powershell

      powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 `
        -Subset smoke-import `
        -BuildDir .\.bpdf-roundtrip-msvc `
        -OutputJson .\build\pdf-ctest-bounded-subset-current\summary.json

   ``smoke-import`` 固定覆盖 10 个 smoke/import 测试：
   ``pdf_document_generator_probe``、``pdf_font_resolver``、
   ``pdf_text_metrics``、``pdf_text_shaper``、``pdf_document_adapter_font``、
   ``pdf_cli_export``、``pdf_cli_import``、``pdf_import_structure``、
   ``pdf_import_failure`` 和 ``pdf_import_table_heuristic``。summary 必须写出
   ``status = pass``、``verdict = pass``、``subset = smoke-import``、
   ``selected_test_count = 10`` 和 ``ctest_timeout_seconds = 120``。
   该子集同时作为 PDF import diagnostics 的轻量视觉门禁前置证据：
   ``pdf_cli_import`` 固定用户可见 ``table_continuation_diagnostics`` 与
   ``failure_kind = no_text_paragraphs`` JSON，``pdf_import_failure`` 固定
   image-only / no-text 负样本不会生成目标 DOCX；``pdf_cli_import`` 还固定
   short-label prose 与 invoice summary form 在开启
   ``--import-table-candidates-as-tables --json`` 后仍保持 ``tables_imported = 0``、
   ``table_continuation_diagnostics = []`` 和
   ``import_table_candidates_as_tables = true``。``smoke-import`` summary
   还必须保留 ``import_visual_gate_scope = bounded_smoke_import_preflight``、
   ``import_visual_gate_boundary =
   bounded_smoke_import_preflight_does_not_replace_full_visual_gate_verdict``、
   ``import_visual_artifact_policy =
   does_not_generate_or_commit_output_visual_artifacts``、
   ``import_diagnostics_contract_tests`` 中的 ``pdf_cli_import``、
   ``pdf_import_failure`` 和 ``pdf_import_table_heuristic``，以及
   ``import_diagnostics_contract_fields`` 中的
   ``table_continuation_diagnostics``、``table_continuation_diagnostics=[]``、
   ``tables_imported=0``、``import_table_candidates_as_tables=true`` 和
   ``failure_kind=no_text_paragraphs``；该字段清单还必须保留完整 blocker diagnostic
   object 的用户可见关键值：``source_row_offset=0``、
   ``skipped_repeating_header=false``、``source_rows_consistent=false``、
   ``disposition=created_new_table``、``blocker=inconsistent_source_rows``、
   ``blocker=repeated_header_mismatch``、``blocker=column_count_mismatch``、
   ``blocker=column_anchors_mismatch``、
   ``blocker=continuation_confidence_below_threshold``、
   ``continuation_confidence=70``、``continuation_confidence=55``、
   ``continuation_confidence=85``、``continuation_confidence=25``、
   ``continuation_confidence=30``、
   ``minimum_continuation_confidence=90``、``column_count_matches=false`` 和
   ``column_anchors_match=false``；summary 还必须保留
   ``import_negative_boundary_contract_cases`` 中的
   ``short_label_prose_remains_paragraphs`` 和
   ``invoice_summary_form_remains_paragraphs``。
   这些字段只证明资源受限窗口里的 import diagnostics preflight，不替代 full
   visual gate verdict，也不要求生成或提交 ``output/`` 视觉产物。固定标记：
   ``pdf_import_smoke_diagnostics_release_trace``。

   资源受限时还可以补跑静态契约子集：

   .. code-block:: powershell

      powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 `
        -Subset contract-static `
        -BuildDir .\.bpdf-roundtrip-msvc `
        -OutputJson .\build\pdf-ctest-bounded-contract-static-current\summary.json

   ``contract-static`` 固定覆盖 10 个 docs/layout/ctest 契约测试：
   ``pdf_import_docs_contract``、``pdf_ctest_timeout_contract``、
   ``pdf_ctest_label_contract``、``pdf_bidi_line_layout_static_contract``、
   ``pdf_document_style_gallery_contract``、``pdf_document_font_matrix_contract``、
   ``pdf_document_table_font_matrix_contract``、
   ``pdf_cjk_copy_search_matrix_contract``、
   ``pdf_cjk_font_embed_matrix_contract`` 和
   ``pdf_cjk_anchor_font_matrix_boundary_contract``。summary 必须写出
   ``status = pass``、``verdict = pass``、``subset = contract-static``、
   ``selected_test_count = 10`` 和 ``ctest_timeout_seconds = 60``。
   ``smoke-import`` 单独使用 120 秒是因为它包含完整 ``pdf_cli_import`` 聚合入口；
   其它 bounded static / regression 子集仍保持 60 秒调度窗口。
   固定标记：``pdf_ctest_bounded_subset_release_trace``、
   ``pdf_ctest_bounded_contract_static_release_trace``。

   资源受限时还可以补跑 CJK / RTL flow 静态契约子集：

   .. code-block:: powershell

      powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 `
        -Subset cjk-flow-static `
        -BuildDir .\.bpdf-roundtrip-msvc `
        -OutputJson .\build\pdf-ctest-bounded-cjk-flow-static-current\summary.json

   ``cjk-flow-static`` 固定覆盖 10 个 CJK / RTL flow 静态契约测试：
   ``pdf_cjk_font_search_density_flow_contract``、
   ``pdf_cjk_font_embed_wrap_mix_contract``、
   ``pdf_cjk_repeated_key_boundary_flow_contract``、
   ``pdf_cjk_style_overlay_page_flow_contract``、
   ``pdf_cjk_complex_layout_contract``、
   ``pdf_cjk_image_wrap_stress_contract``、
   ``pdf_cjk_extreme_page_breaks_contract``、
   ``pdf_cjk_vertical_merge_wrap_cant_split_contract``、
   ``pdf_rtl_bidi_light_contract`` 和 ``pdf_cjk_list_page_flow_contract``。
   summary 必须写出 ``status = pass``、``verdict = pass``、
   ``subset = cjk-flow-static``、``selected_test_count = 10`` 和
   ``ctest_timeout_seconds = 60``。固定标记：
   ``pdf_ctest_bounded_cjk_flow_static_release_trace``。

   资源受限时还可以补跑基础文本 regression 子集：

   .. code-block:: powershell

      powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 `
        -Subset regression-basic-text `
        -BuildDir .\.bpdf-roundtrip-msvc `
        -OutputJson .\build\pdf-ctest-bounded-regression-basic-text-current\summary.json

   ``regression-basic-text`` 固定覆盖 10 个真实 ``pdf_regression_`` 基础文本样本测试：
   ``pdf_regression_manifest``、``pdf_regression_single-text``、
   ``pdf_regression_multi-page-text``、``pdf_regression_font-size-text``、
   ``pdf_regression_color-text``、``pdf_regression_three-page-text``、
   ``pdf_regression_landscape-text``、``pdf_regression_title-body-text``、
   ``pdf_regression_dense-text`` 和 ``pdf_regression_four-page-text``。
   summary 必须写出 ``status = pass``、``verdict = pass``、
   ``subset = regression-basic-text``、``selected_test_count = 10`` 和
   ``ctest_timeout_seconds = 60``。固定标记：
   ``pdf_ctest_bounded_regression_basic_text_release_trace``。

   资源受限时还可以补跑样式/文档 regression 子集：

   .. code-block:: powershell

      powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 `
        -Subset regression-styled-document `
        -BuildDir .\.bpdf-roundtrip-msvc `
        -OutputJson .\build\pdf-ctest-bounded-regression-styled-document-current\summary.json

   ``regression-styled-document`` 固定覆盖 10 个真实 ``pdf_regression_``
   样式/文档样本测试：``pdf_regression_styled-text``、
   ``pdf_regression_mixed-style-text``、``pdf_regression_contract-cjk-style``、
   ``pdf_regression_document-contract-cjk-style``、
   ``pdf_regression_underline-text``、``pdf_regression_strikethrough-text``、
   ``pdf_regression_superscript-subscript-text``、
   ``pdf_regression_style-superscript-subscript-text``、
   ``pdf_regression_document-style-gallery-text`` 和
   ``pdf_regression_document-font-matrix-text``。summary 必须写出
   ``status = pass``、``verdict = pass``、``subset = regression-styled-document``、
   ``selected_test_count = 10`` 和 ``ctest_timeout_seconds = 60``。固定标记：
   ``pdf_ctest_bounded_regression_styled_document_release_trace``。

   资源受限时还可以补跑真实业务样本 regression 子集：

   .. code-block:: powershell

      powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 `
        -Subset regression-business-samples `
        -BuildDir .\.bpdf-roundtrip-msvc `
        -OutputJson .\build\pdf-ctest-bounded-regression-business-samples-current\summary.json

   ``regression-business-samples`` 固定覆盖 10 个真实 ``pdf_regression_``
   业务样本测试，覆盖合同、发票 / 报价单、图文报告、长文档和多 section
   文档：``pdf_regression_contract-cjk-style``、
   ``pdf_regression_document-contract-cjk-style``、
   ``pdf_regression_invoice-grid-text``、``pdf_regression_document-invoice-table-text``、
   ``pdf_regression_image-report-text``、``pdf_regression_cjk-image-report-text``、
   ``pdf_regression_document-cjk-image-wrap-stress-text``、
   ``pdf_regression_long-report-text``、``pdf_regression_document-long-flow-text`` 和
   ``pdf_regression_sectioned-report-text``。summary 必须写出
   ``status = pass``、``verdict = pass``、``subset = regression-business-samples``、
   ``selected_test_count = 10`` 和 ``ctest_timeout_seconds = 60``。固定标记：
   ``pdf_ctest_bounded_regression_business_samples_release_trace``。

   资源受限时还可以补跑表格布局 regression 子集：

   .. code-block:: powershell

      powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_ctest_bounded_subset.ps1 `
        -Subset regression-table-layout `
        -BuildDir .\.bpdf-roundtrip-msvc `
        -OutputJson .\build\pdf-ctest-bounded-regression-table-layout-current\summary.json

   ``regression-table-layout`` 固定覆盖 10 个真实 ``pdf_regression_``
   表格布局样本测试：``pdf_regression_table-like-grid-text``、
   ``pdf_regression_invoice-grid-text``、
   ``pdf_regression_document-table-semantics-text``、
   ``pdf_regression_document-invoice-table-text``、
   ``pdf_regression_document-table-header-footer-variants-text``、
   ``pdf_regression_document-table-wrap-flow-text``、
   ``pdf_regression_document-table-cant-split-text``、
   ``pdf_regression_document-table-merged-cells-text``、
   ``pdf_regression_document-table-merged-header-repeat-text`` 和
   ``pdf_regression_document-table-merged-header-footer-variants-text``。summary 必须写出
   ``status = pass``、``verdict = pass``、``subset = regression-table-layout``、
   ``selected_test_count = 10`` 和 ``ctest_timeout_seconds = 60``。固定标记：
   ``pdf_ctest_bounded_regression_table_layout_release_trace``。该子集只声明已实际运行的
   非 skipped 表格样本，不覆盖当前环境会 skip 的 CJK 表格变体。

   所有 bounded subset summary 都必须同时写出 ``skipped_test_count = 0``；
   ``scripts/run_pdf_ctest_bounded_subset.ps1`` 遇到任何 ``***Skipped`` CTest 项时会把
   ``status`` / ``verdict`` 标记为 ``fail`` 并返回失败，避免 skipped 测试被误当作
   发布证据。

   资源窗口允许时再运行：

   .. code-block:: powershell

      powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_full_gate_guarded.ps1 `
        -BuildDir .\.bpdf-roundtrip-msvc `
        -OutputDir .\output\pdf-visual-release-gate-current

   该入口内部执行 fresh 非 ``FinalizeOnly`` full visual gate，并额外记录 60 秒
   外层保护状态。summary 必须包含
   ``guarded_full_visual_gate_attempt_does_not_replace_completed_full_visual_gate``；
   只有 ``status = pass``、``full_visual_gate_status = pass``，且
   ``outer_guard_status = completed`` 或
   ``outer_guard_status = timed_out_after_pass_summary`` /
   ``pass_summary_before_outer_timeout = true``，才能视为 fresh full visual gate
   已形成可发布 pass 证据；若只达到
   ``visual_gate_segmented_full_coverage_evidence = true``，只能说明分段可视化证据已被
   release readiness 接受，不能声称单次 full gate 已完成。

   如果发布负责人决定接受该分段证据继续发版，readiness warning 必须保留
   ``release_owner_acceptance_required = true``、
   ``release_owner_acceptance_policy``、
   ``release_owner_acceptance_boundary`` 和
   ``release_owner_acceptance_command_template``。该 signoff 只接受
   ``segmented_full_coverage`` 与 contact-sheet 非空等辅助证据，不能改写
   ``full_visual_gate_status``，也不能删除
   ``pdf_full_fresh_visual_gate.not_completed_in_current_window``。固定标记：
   ``pdf_visual_gate_release_owner_acceptance_trace``。

   .. code-block:: powershell

      powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_full_ctest_guarded.ps1 `
        -BuildDir .\.bpdf-roundtrip-msvc `
        -OutputJson .\output\pdf-ctest-current\summary.json

   该入口内部执行
   ``ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_" --output-on-failure --timeout 60``，
   并额外记录 60 秒外层保护状态。summary 必须包含
   ``guarded_full_ctest_attempt_does_not_replace_completed_full_ctest``；只有
   ``status = pass`` 且 ``outer_guard_status = completed`` 才能视为完整 PDF CTest
   已完成。readiness summary 还必须从该 attempt 派生
   ``full_ctest_completion_percent``、``full_ctest_remaining_test_count`` 和
   ``full_ctest_zero_failed_tests_observed``，让治理报告可以直接解释“接近完成但仍非
   pass”的边界。

发布边界
--------

导出侧当前发布边界是受控 PDF 生成、文本回读、CJK 字体回退、部分表格 /
图片 / 页眉页脚 / RTL 探针和 visual baseline 证据。导入侧当前发布边界是
text-first、表格 opt-in 和保守失败；不支持 OCR、扫描件、任意 PDF 视觉精确
还原或通用 PDF 转 Word。
核心头入口调整只影响 C++ include 路径与安装布局，不扩大 PDF 导出 / 导入
发布边界。
