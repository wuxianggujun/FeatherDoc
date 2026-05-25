PDF 发布准入清单
================

本页是 PDF 线每次 release 前的固定检查入口。目标是确认当前 PDF 能在
明确边界内发布，而不是承诺通用 PDF 转 Word、完整 Word 排版引擎、
OCR 或任意视觉精确还原。

准入检查
--------

1. 状态页已同步：

   * ``docs/pdf_visual_validation_status_zh.rst`` 不得停留在旧的
     ``blocked`` / ``not_ready`` 叙事。
   * 当前 preflight、governance 和 full gate 证据状态必须写清楚。

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
   ``finalize_only = true``、``skip_preflight = true``、
   ``visual_baseline_manifest_count = 42``、``baselines_count = 44``、
   ``cjk_manifest_count = 43``、``cjk_copy_search_count = 43`` 和
   ``aggregate_contact_sheet``。其中 ``visual_baseline_manifest_count`` 对应
   manifest 中 ``expect_visual_baseline=true`` 的样本数，``baselines_count``
   对应 full gate 当前渲染并汇总的 baseline 产物数。

6. 发布治理已消费 PDF 结论：

   * ``scripts/run_release_candidate_checks.ps1`` 的 summary / final review
     必须显示 PDF visual gate verdict、baseline count、CJK copy/search count
     和 contact sheet 路径。
   * ``final_review.md`` 中 ``PDF visual gate verdict:`` 行必须直接携带
     ``pass`` 或 ``fail``，``PDF visual gate summary:`` 行必须直接携带
     ``summary.json`` 路径，``PDF visual gate contact sheet:`` 行必须直接携带
     ``aggregate-contact-sheet.png`` 路径；不能让其它段落里的同名路径替代。
     ``PDF visual gate:``、verdict、counts、manifest counts 和 finalizable
     必须保留在同一个 step-status Markdown list run。
     固定标记：``line_scoped_final_review_pdf_visual_trace``、
     ``line_scoped_final_review_pdf_visual_verdict``、
     ``block_scoped_final_review_pdf_visual_step_status``。
   * ``release_handoff.md`` 中 ``PDF visual gate verdict:`` 行必须直接携带
     ``pass`` 或 ``fail``，``PDF visual gate summary:`` 行必须直接携带
     ``summary.json`` 路径，``PDF visual aggregate contact sheet:`` 行必须
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
     ``block_scoped_release_body_pdf_visual_evidence``。
   * ``full_visual_gate_status`` 不能停留在“governance 未消化”的唯一结论；
     release reviewer 应同时看到 full gate summary verdict。
   * ``release_governance_handoff.md`` 中每个 ``source_report:`` block
     必须把 PDF visual gate verdict、finalizable、summary、contact sheet
     和 42/43/44 计数放在同一块里；不能让块外的
     ``aggregate-contact-sheet.png`` 替代当前 source report 的证据。
     固定标记：``block_scoped_pdf_visual_gate_handoff_trace``。
   * preflight governance 中的 ``not_run_by_preflight_governance`` 只能解释
     preflight-governance 报告自身没有重跑 full gate；如果同轮
     ``FinalizeOnly`` summary 已给出 ``verdict = pass``，发布结论应以
     full gate summary 和 contact sheet 证据为准。

7. 可视化证据非空：

   * ``output/pdf-visual-release-gate-current/report/aggregate-contact-sheet.png``
     必须存在且非空。
   * 若本轮没有重跑渲染，必须说明使用了 ``-FinalizeOnly -SkipPreflight``
     复核现有产物。
   * 最近一次复核的 contact sheet 参考尺寸为 ``912x14566``，大小为
     ``1822428`` bytes；后续 release 可接受尺寸变化，但必须记录非空、
     可读和未丢失路径的证据。

8. 轻量验证已跑：

   .. code-block:: powershell

      powershell -NoProfile -ExecutionPolicy Bypass -File .\test\pdf_visual_validation_status_docs_contract_test.ps1 -RepoRoot .
      powershell -NoProfile -ExecutionPolicy Bypass -File .\test\release_candidate_visual_verdict_test.ps1 -RepoRoot . -WorkingDir .\tmp\release_candidate_visual_verdict
      powershell -NoProfile -ExecutionPolicy Bypass -File .\test\pdf_real_business_sample_manifest_contract_test.ps1 -RepoRoot .

   资源窗口允许时再运行：

   .. code-block:: powershell

      ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_" --output-on-failure --timeout 60

发布边界
--------

导出侧当前发布边界是受控 PDF 生成、文本回读、CJK 字体回退、部分表格 /
图片 / 页眉页脚 / RTL 探针和 visual baseline 证据。导入侧当前发布边界是
text-first、表格 opt-in 和保守失败；不支持 OCR、扫描件、任意 PDF 视觉精确
还原或通用 PDF 转 Word。
