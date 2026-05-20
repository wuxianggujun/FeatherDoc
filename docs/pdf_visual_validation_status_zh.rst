PDF 可视化验证状态
====================

本页记录当前 ``dev`` 上 PDF 线的受控可视化验证边界。

当前结论
--------

1. ``dev`` 与 ``origin/dev`` 已对齐。
2. 当前仓库工作区干净，没有未提交修改。
3. 最新的 PDF preflight governance 报告仍是 ``blocked``，因为
   ``check_pdf_visual_release_gate_preflight.ps1`` 还报告：

   * ``required_check_count = 10``
   * ``blocking_check_count = 6``
   * ``missing_cli_pdf_count = 2``
   * ``visual_baseline_sample_count = 42``
   * ``missing_visual_baseline_pdf_count = 42``
   * ``cjk_text_layer_sample_count = 43``
   * ``missing_cjk_text_layer_pdf_count = 43``
   * ``output_gap_count = 3``
   * ``missing_output_count = 87``

4. 这表示完整的 ``scripts/run_pdf_visual_release_gate.ps1`` 仍然不能在
   低资源边界内直接进入 ready 状态。

治理链路
--------

当前 PDF preflight 的阻断数字已经接入发布治理链路：

* ``scripts/check_pdf_visual_release_gate_preflight.ps1`` 会在 summary JSON 中输出
  ``blocking_summary``。
* ``scripts/write_pdf_visual_release_gate_preflight_governance_report.ps1`` 会把同一份
  ``blocking_summary`` 透传到 governance summary、release blocker 和 action item。
* ``scripts/release_blocker_metadata_helpers.ps1`` 会在
  ``prepare_pdf_visual_release_gate_build_outputs`` 的 runbook / checklist guidance 中展示
  缺口摘要。
* ``test/release_note_bundle_version_test.ps1`` 已覆盖 ``REVIEWER_CHECKLIST.md`` 中的
  ``missing CLI PDFs=2``、``missing visual baseline PDFs=42`` 和
  ``missing CJK text-layer PDFs=43``。

这些改动只让缺口更可追踪，不会生成 PDF baseline，也不会把完整 visual gate 标记为通过。

受控视觉烟测
--------------

当前已经复用现有小型产物完成一次受控视觉烟测检查：

* ``output/pdf-controlled-visual-smoke-20260520/minimal/pages/page-01.png``
* ``output/pdf-controlled-visual-smoke-20260520/minimal/contact-sheet.png``
* ``output/pdf-controlled-visual-smoke-20260520/rerun/pages/page-01.png``
* ``output/pdf-controlled-visual-smoke-20260520/rerun/contact-sheet.png``

检查结果：

* 页面不是空白。
* 标题、正文和小表格都可见。
* 未见明显裁剪、重叠或整页空白问题。
* 像素抽检显示 PNG 不是白图，且有稳定的非白像素占比。

这只说明当前 smoke 产物可读，不等于完整 PDF visual gate
或者 PDF CJK 样例库已经完成最终验收。

下一步
------

1. 继续保留远端 ``origin/codex/*`` 为只读参考库存。
2. 等可复用的 PDF build / CTest / baseline 输出补齐后，再把
   ``check_pdf_visual_release_gate_preflight.ps1`` 推进到 ``ready``。
3. 只有完整 visual gate 通过后，才考虑回收旧 PDF 参考分支。
