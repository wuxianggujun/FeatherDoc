PDF 可视化验证状态
====================

本页记录当前 ``dev`` 上 PDF 线的受控可视化验证边界。

当前结论
--------

1. ``dev`` 与 ``origin/dev`` 已对齐。
2. 2026-05-20 复核时仓库工作区干净；后续轮次需以
   ``git status --short`` 为准。
3. 最新的 PDF preflight governance 报告仍是 ``blocked``，因为
   ``check_pdf_visual_release_gate_preflight.ps1`` 还报告：

   * ``required_check_count = 11``
   * ``blocking_check_count = 7``
   * ``missing_cli_pdf_count = 2``
   * ``visual_baseline_sample_count = 42``
   * ``missing_visual_baseline_pdf_count = 42``
   * ``cjk_text_layer_sample_count = 43``
   * ``missing_cjk_text_layer_pdf_count = 43``
   * ``memory_guard_blocked = false``（当前阻断不是内存不足，而是缺少可复用
     build / CTest / PDF baseline 输出）
   * ``output_gap_count = 3``
   * ``missing_output_count = 87``
   * ``preflight_ready = false``
   * ``full_visual_gate_required = true``
   * ``full_visual_gate_status = not_run_by_preflight_governance``

4. ``preflight_ready`` 只表示预检是否清零；即使它为 ``true``，
   也仍需完整 ``scripts/run_pdf_visual_release_gate.ps1`` 产出新的
   full visual gate 证据后，才可把 PDF 线视为完整可视化验收通过。

治理链路
--------

当前 PDF preflight 的阻断数字已经接入发布治理链路：

* ``scripts/check_pdf_visual_release_gate_preflight.ps1`` 会在 summary JSON 中输出
  ``blocking_summary``。
* 同一份 preflight summary 现在也直接输出顶层 ``output_gap_count`` 和
  ``missing_output_count``，让状态页、governance report 和 release blocker
  使用同一组缺失输出总数。
* ``check_pdf_visual_release_gate_preflight.ps1`` 默认要求工作站空闲物理内存不少于
  ``2048 MB``；如果低于阈值，会把
  ``workstation_free_memory_available`` 记录为 blocking check，并在
  ``blocking_summary`` 中写入 ``free_memory_mb``、``min_free_memory_mb``、
  ``memory_guard_blocked`` 和 ``memory_guard_skipped``。
* ``scripts/write_pdf_visual_release_gate_preflight_governance_report.ps1`` 会把同一份
  ``blocking_summary`` 透传到 governance summary、release blocker 和 action item。
  如果它自动生成 preflight summary，也会把 ``-MinFreeMemoryMB`` 或
  ``-SkipMemoryGuard`` 继续传给 preflight 脚本，避免绕过同一套资源门槛。
  同一份 summary 还会显式写入 ``preflight_ready``、
  ``full_visual_gate_required`` 和 ``full_visual_gate_status``，避免把
  ``-PreflightOnly`` 结果误读为完整 visual gate 结果。
* ``scripts/release_blocker_metadata_helpers.ps1`` 会在
  ``prepare_pdf_visual_release_gate_build_outputs`` 的 runbook / checklist guidance 中展示
  缺口摘要，并继续显示 ``memory guard blocked=false``、
  ``memory guard skipped=false``、``free memory MB`` 和
  ``minimum free memory MB``，让 reviewer 先判断是否是资源不足阻断，再准备
  build 输出或尝试完整 gate。
* ``test/release_note_bundle_version_test.ps1`` 已覆盖 ``REVIEWER_CHECKLIST.md`` 中的
  ``missing CLI PDFs=2``、``missing visual baseline PDFs=42`` 和
  ``missing CJK text-layer PDFs=43``，并覆盖 memory guard 状态和阈值字段。

这些改动只让缺口更可追踪，不会生成 PDF baseline，也不会把完整 visual gate 标记为通过。

2026-05-20 轻量复核
-------------------

本轮在 ``dev`` 与 ``origin/dev`` 对齐、工作区干净、空闲内存高于 ``2048 MB``
门槛时，只运行轻量 preflight 和 governance report 生成检查；未运行 CMake、CTest、
Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。

复核结果：

* ``check_pdf_visual_release_gate_preflight.ps1`` 返回 ``not_ready``。
* ``write_pdf_visual_release_gate_preflight_governance_report.ps1`` 返回 ``blocked``。
* 默认 ``.bpdf-roundtrip-msvc`` 不存在且 ``build`` / ``out\build`` 不像可复用
  CMake build 时，``build_dir_source = requested``；自动候选只有在包含
  ``CMakeCache.txt`` 或 ``CTestTestfile.cmake`` 时才会被视为可复用 build。
  summary JSON 会同步记录 ``build_dir_auto_candidates``，用于说明每个自动候选的
  ``cmake_cache_exists``、``ctest_manifest_exists`` 和 ``looks_reusable`` 状态。
* 已新增并通过普通 ``build\tmp`` 回归场景：仓库里只有普通 ``build`` 子目录时，
  preflight 仍保持 ``build_dir_source = requested``，并把缺失的
  ``.bpdf-roundtrip-msvc`` 记录为 ``build_dir_exists`` 阻断项，避免旧
  ``auto:build`` 证据误导后续分支清理判断。
* ``release_blocker_count = 1``。
* ``action_item_count = 1``。
* ``output_gap_count = 3``。
* ``missing_output_count = 87``。
* ``memory_guard_blocked = false``，本轮阻断不是内存不足。
* ``free_memory_mb`` 高于 ``min_free_memory_mb = 2048``。
* 仍缺 ``.bpdf-roundtrip-msvc``、``CMakeCache.txt``、``CTestTestfile.cmake``、
  2 个 CLI baseline PDF、
  42 个 visual baseline PDF 和 43 个 CJK text-layer PDF。
* ``test/pdf_visual_release_gate_preflight_test.ps1`` 里的 ``fake-pdf-build``、fake ctest
  和 fake python 只是脚本契约测试使用的 test fixture；它们不是不可复用 release gate
  build 的示例，也不是 reusable release build substitute，不能作为完整
  ``run_pdf_visual_release_gate.ps1`` 的可复用输入。

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
2. 在资源允许、源码已提交推送且工作区干净时，先准备可复用的 PDF build /
   CTest / baseline 输出；不要直接启动完整 visual gate。
3. 重新运行 ``check_pdf_visual_release_gate_preflight.ps1``。只有当
   ``workstation_free_memory_available``、build 目录、CTest manifest、CLI PDF
   baseline、visual baseline 和 CJK text-layer 输出全部通过后，才把预检视为
   ``ready``。
4. 只有完整 visual gate 通过后，才考虑归档或回收旧 PDF 参考分支。
