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

   预期 ``blocking_checks = 0`` 且 ``preflight_ready = true``。

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

   ``report/summary.json`` 必须包含 ``verdict = pass``、
   ``visual_baseline_manifest_count = 42``、``baselines_count = 44``、
   ``cjk_manifest_count = 43``、``cjk_copy_search_count = 43`` 和
   ``aggregate_contact_sheet``。其中 ``visual_baseline_manifest_count`` 对应
   manifest 中 ``expect_visual_baseline=true`` 的样本数，``baselines_count``
   对应 full gate 当前渲染并汇总的 baseline 产物数。

6. 发布治理已消费 PDF 结论：

   * ``scripts/run_release_candidate_checks.ps1`` 的 summary / final review
     必须显示 PDF visual gate verdict、baseline count、CJK copy/search count
     和 contact sheet 路径。
   * ``full_visual_gate_status`` 不能停留在“governance 未消化”的唯一结论；
     release reviewer 应同时看到 full gate summary verdict。

7. 可视化证据非空：

   * ``output/pdf-visual-release-gate-current/report/aggregate-contact-sheet.png``
     必须存在且非空。
   * 若本轮没有重跑渲染，必须说明使用了 ``-FinalizeOnly -SkipPreflight``
     复核现有产物。

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
