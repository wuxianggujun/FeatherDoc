PDF 可视化验证状态
====================

本页记录当前 ``dev`` 上 PDF 线的受控可视化验证边界。

当前结论
--------

1. ``dev`` 与 ``origin/dev`` 已对齐。
2. 2026-05-20 复核时仓库工作区干净；后续轮次需以
   ``git status --short`` 为准。
3. 2026-05-25 复核时，最新 PDF preflight / visual gate 证据已经对齐到
   ``ready`` / ``pass``：

   * ``evidence_kind = real_build``
   * ``synthetic_markers = 0``
   * raw preflight summary 中 ``blocking_checks = []``
   * ``blocking_summary.blocking_check_count = 0``
   * preflight governance summary 中 ``preflight_ready = true``
   * ``release_ready = true``
   * ``full_visual_gate_required = true``
   * preflight governance summary 中的
     ``full_visual_gate_status = not_run_by_preflight_governance`` 只表示
     该 preflight-governance 报告自身没有重跑 full gate；发布结论已由
     full gate summary 的 ``verdict = pass`` 和 contact sheet 证据消化
   * ``controlled_visual_smoke_status = pass``
   * ``controlled_visual_smoke_passed = true``
   * ``blocking_check_count = 0``
   * ``output_gap_count = 0``
   * ``missing_output_count = 0``
   * ``release_blocker_count = 0``
   * ``action_item_count = 0``
   * ``pdf_dependency_inputs_status = ready``
   * ``pdf_build_options_enabled = true``
   * ``pdfio_dependency_ready = true``
   * ``pdfium_dependency_ready = true``
   * ``ctest_list_contains_pdf_gate_tests = pass``
   * ``memory_guard_blocked = false``
   * ``memory_guard_skipped = false``
   * ``free_memory_mb`` 高于 ``min_free_memory_mb = 2048``
   * preflight summary ``generated_at = 2026-05-25T07:13:30``，并记录
     ``selected_pdfium_provider = prebuilt``
   * 现有 ``output/pdf-visual-release-gate-current/report/summary.json`` 已包含
     ``status = pass``、``verdict = pass``、``finalize_only = true``、
     ``skip_preflight = true``、``visual_baseline_manifest_count = 42``、
     ``baselines_count = 44``、``cjk_manifest_count = 43`` 与
     ``cjk_copy_search_count = 43``；其中 ``visual_baseline_manifest_count`` 是
     regression manifest 中标记 ``expect_visual_baseline=true`` 的样本数，
     ``baselines_count`` 是 full gate 当前渲染并汇总的 baseline 产物数
   * full gate summary ``generated_at = 2026-05-25T07:15:13``
   * 现有 full gate 产物目录已包含 ``pdf-cli-export-test.log``、
     ``pdf-regression-test.log``、``aggregate-contact-sheet.png``、
     ``report/cjk-copy-search/*``、``report/unicode-font.log`` 以及各 baseline
     页图 / summary / contact-sheet，可用于脚本原生复核
   * 已成功执行
     ``run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -OutputDir .\output\pdf-visual-release-gate-current -FinalizeOnly -SkipPreflight``
   * 当前结论应理解为“已有 full gate 产物链且可复核”，而不是“仍然缺少 full gate 证据”

4. 2026-05-26 fresh preflight 复核再次确认完整 visual gate 输入已齐备：

   * ``output/pdf-visual-release-gate-preflight-current/summary.json``
     写回 ``generated_at = 2026-05-26T23:19:01``
   * ``status = ready``、``evidence_kind = real_build``
   * ``blocking_check_count = 0``、``output_gap_count = 0``、
     ``missing_output_count = 0``
   * ``pdf_dependency_inputs_status = ready``、
     ``pdf_dependency_missing_input_count = 0``、
     ``pdf_build_options_enabled = true``
   * ``visual_baseline_sample_count = 42``、
     ``missing_visual_baseline_pdf_count = 0``
   * ``cjk_text_layer_sample_count = 43``、
     ``missing_cjk_text_layer_pdf_count = 0``
   * ``free_memory_mb = 3357.2`` 高于 ``min_free_memory_mb = 2048``，
     且 ``memory_guard_blocked = false``
   * 该结果只证明 full visual gate 的输入和前置产物 ready；在外部浏览器 /
     Node / PowerShell 进程压力仍高时，仍不应直接启动重型 full visual gate。

5. 2026-05-26 同轮 fresh 非 ``FinalizeOnly`` full visual gate 在外层 60 秒守护下
   未完成，但已经沉淀为机器可读 attempt 辅助证据：

   * ``output/pdf-visual-release-gate-current/report/attempt-summary.json``
     写回 ``generated_at = 2026-05-26T23:20:51``
   * ``status = partial``、``verdict = not_complete``、
     ``full_visual_gate_status = not_complete``
   * ``evidence_scope = bounded_attempt_auxiliary_only``
   * ``outer_guard_status = timed_out``、
     ``outer_guard_timed_out = true``、
     ``outer_guard_timeout_seconds = 60``
   * 已完成子阶段包括 ``pdf_cli_export_status = pass``、
     ``pdf_regression_status = pass``、``unicode_font_status = pass``、
     ``cjk_copy_search_status = pass``
   * ``pdf_regression_selected_test_count = 91``、
     ``pdf_regression_failed_test_count = 0``、
     ``pdf_regression_skipped_test_count = 7``
   * ``cjk_copy_search_count = 43``、
     ``cjk_copy_search_missing_text_count = 0``
   * 当前未完成部分集中在
     ``visual_baseline_render_status = partial``、
     ``visual_baseline_fresh_rendered_count = 37`` /
     ``expected_visual_render_count = 44``，
     以及 ``aggregate_contact_sheet_status = stale``

6. 2026-05-26 同轮 ``ctest -R "pdf_" --output-on-failure --timeout 60`` 也在外层
   60 秒守护下未完成。因此当前最准确的 release 解释不是“full gate 失败”，而是：

   * 发布主结论仍依赖现有 ``FinalizeOnly`` full gate summary 与 contact sheet
   * fresh 重跑未完成必须被 reviewer 看见，但它应作为 governance warning，
     而不是直接推翻已存在的 full gate pass 结论

7. 2026-05-27 再次运行轻量 preflight，脚本在 60 秒守护内完成，并验证
   默认 current raw summary 会被本次运行刷新：

   * ``check_pdf_visual_release_gate_preflight.ps1`` 默认写回
     ``output/pdf-visual-release-gate-preflight-current/summary.json``；
     不传 ``-OutputJson`` 时也会覆盖该 current raw summary，避免命令行输出已经
     是新结论而文件仍停留在旧 ``ready`` 证据。
   * 本次默认输出 raw preflight 写回
     ``generated_at = 2026-05-27T01:38:37``。
   * 本次 raw preflight 结论为 ``status = ready``，
     ``blocking_check_count = 0``、``blocking_checks = []``。
   * ``free_memory_mb = 2691.4`` 高于 ``min_free_memory_mb = 2048``，
     且 ``memory_guard_blocked = false``。
   * 同一份 summary 仍确认 ``evidence_kind = real_build``、
     ``pdf_dependency_inputs_status = ready``、
     ``pdf_dependency_missing_input_count = 0``、
     ``pdf_build_options_enabled = true``、
     ``visual_baseline_sample_count = 42``、
     ``missing_visual_baseline_pdf_count = 0``、
     ``cjk_text_layer_sample_count = 43``、
     ``missing_cjk_text_layer_pdf_count = 0``。
   * 若未来再次遇到 ``workstation_free_memory_available`` 阻断，该阻断也会写入
     同一个 current raw summary，而不会被旧 ``ready`` 文件掩盖。
   * 固定标记：``pdf_preflight_default_current_summary_trace``。

8. ``preflight_ready`` 是 preflight governance report 写出的治理字段，不是
   ``check_pdf_visual_release_gate_preflight.ps1`` raw summary 的顶层字段。它只表示
   预检是否清零；即使它为 ``true``，也仍需结合 full gate 证据链判断是否可以
   视为完整可视化验收通过。当前 ``dev`` 的状态不是“尚无 full gate 证据”，而是
   “已有 full gate 产物链，且已被 ``-FinalizeOnly`` 路径原生复核；只是本轮没有
   重跑重型 full gate”。

治理链路
--------

当前 PDF preflight 的阻断数字已经接入发布治理链路：

* ``scripts/check_pdf_dependency_inputs.ps1`` 会在重新配置 PDF build 前检查
  本机 PDFio / PDFium 输入是否齐备，并输出
  ``featherdoc.pdf_dependency_inputs_check.v1``。它只读取路径，不运行 CMake、
  CTest、Ninja、MSBuild、下载依赖或 PDF 渲染。
* ``scripts/check_pdf_visual_release_gate_preflight.ps1`` 会在 summary JSON 中输出
  ``blocking_summary``。
* ``scripts/check_pdf_visual_release_gate_preflight.ps1`` 默认把 raw summary 写入
  ``output/pdf-visual-release-gate-preflight-current/summary.json``；即使显式传入空的
  ``-OutputJson``，也会回退到该 current 路径。该文件表示最近一次 raw preflight
  结果；如果资源不足导致 ``not_ready``，也必须覆盖旧 ``ready`` 文件并暴露新的
  ``workstation_free_memory_available`` 阻断。固定标记：
  ``pdf_preflight_default_current_summary_trace``。
* 同一份 preflight summary 现在还会附带 ``pdf_dependency_inputs``，并在
  ``blocking_summary`` 中同步写入 ``pdf_dependency_inputs_status``、
  ``pdf_dependency_missing_input_count``、``selected_pdfium_provider``、
  ``pdfio_dependency_ready`` 和 ``pdfium_dependency_ready``，让 reviewer 不必手工对照两份
  JSON 才知道当前是否被 PDFio / PDFium 输入卡住。
* 同一份 preflight summary 现在也直接输出顶层 ``output_gap_count`` 和
  ``missing_output_count``，让状态页、governance report 和 release blocker
  使用同一组缺失输出总数。
* ``check_pdf_visual_release_gate_preflight.ps1`` 会读取 ``CMakeCache.txt``，
  并用 ``pdf_build_options_enabled`` 显式检查 ``FEATHERDOC_BUILD_PDF`` 和
  ``FEATHERDOC_BUILD_PDF_IMPORT``。如果它们是 ``OFF`` 或缺失，preflight 会在
  运行完整 visual gate 前直接阻断，避免把 87 个缺失 PDF 输出误读成单纯的
  baseline 漏生成。
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
* 同一份治理报告还会检查 ``evidence_kind``。如果 preflight summary 标记为
  ``synthetic_fixture``，或阻断项包含 ``synthetic_preflight_evidence``，即使脚本
  fixture 看起来满足前置条件，治理报告也必须保持 ``release_ready = false`` 和
  ``preflight_ready = false``。这些 fake / synthetic 证据只能用于脚本契约测试，
  不能作为真实 ``run_pdf_visual_release_gate.ps1`` 通过或发布就绪证据。
* ``scripts/release_blocker_metadata_helpers.ps1`` 会在
  ``prepare_pdf_visual_release_gate_build_outputs`` 的 runbook / checklist guidance 中展示
  缺口摘要，并继续显示 ``memory guard blocked=false``、
  ``memory guard skipped=false``、``free memory MB`` 和
  ``minimum free memory MB``，让 reviewer 先判断是否是资源不足阻断，再准备
  build 输出或尝试完整 gate。
* ``test/release_note_bundle_version_test.ps1`` 已覆盖 ``REVIEWER_CHECKLIST.md`` 中的
  ``missing CLI PDFs=2``、``missing visual baseline PDFs=42`` 和
  ``missing CJK text-layer PDFs=43``，并覆盖 memory guard 状态和阈值字段。

这些改动最初是为了让缺口更可追踪；而当前 ``dev`` 上现有 full gate 产物链已经可以被
原生脚本复核，状态不应继续停留在旧的 ``blocked`` / ``not_ready`` 叙事。

2026-05-20 轻量复核
-------------------

本轮在 ``dev`` 与 ``origin/dev`` 对齐、工作区干净、空闲内存高于 ``2048 MB``
门槛时，只运行轻量 preflight 和 governance report 生成检查；未运行 CMake、CTest、
Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。

当时的复核结果：

* ``check_pdf_visual_release_gate_preflight.ps1`` 返回 ``not_ready``。
* ``write_pdf_visual_release_gate_preflight_governance_report.ps1`` 返回 ``blocked``。
* 默认 ``.bpdf-roundtrip-msvc`` 不存在且 ``build`` / ``out\build`` 不像可复用
  CMake build 时，``build_dir_source = requested``；自动候选只有在包含
  ``CMakeCache.txt`` 或 ``CTestTestfile.cmake`` 时才会被视为可复用 build。
  summary JSON 会同步记录 ``build_dir_auto_candidates``，用于说明每个自动候选的
  ``cmake_cache_exists``、``ctest_manifest_exists``、``pdf_build_options_enabled``、
  ``pdf_build_options`` 和 ``looks_reusable`` 状态。governance Markdown 也会列出
  ``FEATHERDOC_BUILD_PDF`` / ``FEATHERDOC_BUILD_PDF_IMPORT`` 的
  ``present``、``value`` 和 ``enabled``，避免候选目录有 CMake / CTest 文件但 PDF
  writer/import 选项为 ``OFF`` 时被误判为可复用。
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
* 2026-05-21 复核时，``.bpdf-roundtrip-msvc``、``CMakeCache.txt`` 和
  ``CTestTestfile.cmake`` 已存在；但 ``FEATHERDOC_BUILD_PDF=OFF``、
  ``FEATHERDOC_BUILD_PDF_IMPORT=OFF``，所以仍缺 2 个 CLI baseline PDF、
  42 个 visual baseline PDF 和 43 个 CJK text-layer PDF。
* 同日只读依赖输入检查还确认 ``selected_pdfium_provider = prebuilt``、
  ``pdfium_ready = true``、``pdfium_prebuilt_root_exists = true``，并从
  ``TinaToolBox\dependencies\pdfium-win-x64`` 探测到
  ``lib\pdfium.dll.lib``、``include\fpdfview.h`` 和 ``bin\pdfium.dll``；
  当前真实缺口收敛为 ``tmp\pdfio-src\pdfio.h``，所以
  ``missing_input_count = 1``，在补齐 PDFio 输入前，重新配置
  ``.bpdf-roundtrip-msvc`` 仍会继续停留在 ``not_ready``。
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

这只说明当时 smoke 产物可读，不等于完整 PDF visual gate
或者 PDF CJK 样例库已经完成最终验收。

2026-05-24 full gate 复核结论
-----------------------------

本轮没有重跑重型 ``ctest`` / PDF 渲染，而是沿着脚本原生路径完成了 full gate 现有产物复核：

* ``test/pdf_visual_release_gate_style_baselines_test.ps1`` 通过。
* ``test/pdf_visual_release_gate_text_shaping_baselines_test.ps1`` 通过。
* ``test/pdf_visual_release_gate_preflight_governance_report_test.ps1`` 通过。
* ``test/pdf_visual_validation_status_docs_contract_test.ps1`` 通过。
* 实际执行
  ``scripts/run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -OutputDir .\output\pdf-visual-release-gate-current -FinalizeOnly -SkipPreflight``
  成功。
* 刷新后的 ``report/summary.json`` 写回时间为 ``2026-05-24T18:24:52``。
* 现有 evidence 仍保持：
  ``verdict = pass``、``visual_baseline_manifest_count = 42``、
  ``baselines_count = 44``、``cjk_manifest_count = 43``、
  ``cjk_copy_search_count = 43``，且 ``aggregate-contact-sheet.png`` 非空。

因此当前更准确的结论是：

* PDF preflight / governance 已处于 ``ready``。
* 现有 full gate 产物链完整且可复核。
* 当前剩余动作不是“补齐证据”，而是“是否值得在资源窗口里重跑一次新的重型 full gate”。

2026-05-25 preflight / finalize-only 复核结论
---------------------------------------------

本轮先执行轻量 preflight，再用 ``FinalizeOnly`` 复用现有 full gate 产物刷新 summary：

* ``scripts/check_pdf_visual_release_gate_preflight.ps1 -OutputJson .\build\pdf_visual_release_gate_preflight_current\summary.json``
  在 60 秒 PowerShell Job 内通过。
* preflight summary 写回时间为 ``2026-05-25T07:13:30``，结果为
  ``status = ready``、``evidence_kind = real_build``、``blocking_check_count = 0``、
  ``output_gap_count = 0``、``missing_output_count = 0``。
* 同一份 preflight 证据确认 ``pdf_dependency_inputs_status = ready``、
  ``pdf_build_options_enabled = true``、``pdfio_dependency_ready = true``、
  ``pdfium_dependency_ready = true``、``selected_pdfium_provider = prebuilt``，并且
  ``ctest_list_contains_pdf_gate_tests = pass``。
* ``scripts/run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -OutputDir .\output\pdf-visual-release-gate-current -FinalizeOnly -SkipPreflight``
  在 60 秒 PowerShell Job 内通过。
* 刷新后的 ``report/summary.json`` 写回时间为 ``2026-05-25T07:15:13``，结果为
  ``status = pass``、``verdict = pass``、``finalize_only = true``、
  ``skip_preflight = true``。
* 可视化非空复核继续通过：
  ``output/pdf-visual-release-gate-current/report/aggregate-contact-sheet.png`` 为
  ``912x14566``，大小 ``1822428`` bytes。

2026-05-26 fresh preflight 复核结论
-----------------------------------

本轮在提交 release notes evidence 闭环后，再次运行轻量 preflight，只写 ignored
summary，不触发 CMake、CTest、Ninja、MSBuild、Office、浏览器或 full visual gate：

* ``scripts/check_pdf_visual_release_gate_preflight.ps1 -OutputJson .\output\pdf-visual-release-gate-preflight-current\summary.json``
  在 60 秒 PowerShell Job 内通过。
* preflight summary 写回时间为 ``2026-05-26T22:50:00``，结果为
  ``status = ready``、``evidence_kind = real_build``、``blocking_check_count = 0``、
  ``output_gap_count = 0``、``missing_output_count = 0``。
* 同一份 preflight 证据确认 ``pdf_dependency_inputs_status = ready``、
  ``pdf_dependency_missing_input_count = 0``、``pdf_build_options_enabled = true``、
  ``pdfio_dependency_ready = true``、``pdfium_dependency_ready = true`` 和
  ``selected_pdfium_provider = prebuilt``。
* 样本输入缺口仍为 0：``visual_baseline_sample_count = 42``、
  ``missing_visual_baseline_pdf_count = 0``、``cjk_text_layer_sample_count = 43``、
  ``missing_cjk_text_layer_pdf_count = 0``。
* 资源门槛通过：``free_memory_mb = 2439.4``、
  ``min_free_memory_mb = 2048``、``memory_guard_blocked = false``。
* 该证据将“是否可以启动 full visual gate”的前置条件推进为 ready；剩余阻断
  是外部 Edge / Node / PowerShell 进程压力下是否值得启动重型 full gate，而不是
  PDFio/PDFium 输入、PDF build options、baseline/render 输入缺失。

2026-05-26 bounded CTest 复核结论
---------------------------------

完整 ``ctest -R "pdf_" --output-on-failure --timeout 60`` 在单个 60 秒外层
PowerShell 保护内曾推进到 128/139 个测试但被保护截断，因此不能作为完整通过证据。
当前低资源窗口下改用 ``scripts/run_pdf_ctest_bounded_subset.ps1`` 固化一条可复核的
bounded PDF CTest 路径：

2026-05-27 起，完整 ``pdf_`` 套件也有固定受控入口
``scripts/run_pdf_full_ctest_guarded.ps1``。该入口把
``ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_" --output-on-failure --timeout 60``
包在 60 秒外层保护中，并写出 ``output/pdf-ctest-current/summary.json``。
summary schema 为 ``featherdoc.pdf_full_ctest_guarded_summary.v1``，必须保留
``full_ctest_status``、``outer_guard_status``、``outer_guard_timed_out``、
``outer_guard_timeout_seconds``、``selected_test_count``、
``completed_test_count`` 和 ``not_run_test_count``。readiness summary 会额外派生
``full_ctest_completion_percent``、``full_ctest_remaining_test_count`` 和
``full_ctest_zero_failed_tests_observed``，用于解释最近一次受控尝试的完成度、剩余
测试数和是否观察到失败。固定标记：``pdf_full_ctest_guarded_summary_trace``。
如果外层保护超时，该 summary 只能作为 attempt evidence，不能替代完整 PDF
CTest pass。

2026-05-27 起，fresh 非 ``FinalizeOnly`` full visual gate 也有固定受控入口
``scripts/run_pdf_visual_full_gate_guarded.ps1``。该入口把
``scripts/run_pdf_visual_release_gate.ps1`` 包在 60 秒外层保护中，并写出
``output/pdf-visual-release-gate-current/report/full-visual-gate-guarded-summary.json``。
summary schema 为 ``featherdoc.pdf_visual_full_gate_guarded_summary.v1``，必须保留
``visual_full_gate_status``、``visual_full_gate_verdict``、
``visual_full_gate_outer_guard_status``、``visual_full_gate_outer_guard_timed_out``、
``visual_full_gate_pass_summary_before_outer_timeout``、
``visual_full_gate_attempt_passed_stage_count``、
``visual_full_gate_attempt_visual_baseline_fresh_rendered_count``、
``visual_full_gate_attempt_aggregate_contact_sheet_status`` 和 ``attempt_summary_json``。固定标记：
``pdf_visual_full_gate_guarded_summary_trace``。如果 summary 同时满足 ``status = pass``、
``full_visual_gate_status = pass``、``outer_guard_status = timed_out_after_pass_summary`` 和
``pass_summary_before_outer_timeout = true``，表示 pass summary 已先于外层退出保护落盘，
可作为 fresh full visual gate pass 证据；普通 ``outer_guard_status = timed_out`` 仍只能作为
attempt evidence，不能替代 fresh full visual gate pass。
如果超时后通过 ``write_pdf_visual_gate_attempt_summary.ps1`` 与 aggregate contact-sheet
rebuild 补齐辅助证据，readiness summary 必须额外暴露
``visual_full_gate_attempt_summary_visual_baseline_fresh_rendered_count``、
``visual_full_gate_attempt_summary_aggregate_contact_sheet_status``、
``attempt_summary_visual_baseline_fresh_rendered_count`` 和
``attempt_summary_aggregate_contact_sheet_status``，并继续保留 full gate
``not_complete`` 边界。

* ``scripts/run_pdf_full_ctest_guarded.ps1`` 仍通过
  ``ctest --test-dir .bpdf-roundtrip-msvc`` 执行真实测试，不使用
  fake ctest 或 synthetic fixture。
* ``smoke-import`` 固定覆盖 10 个 smoke/import 测试：
  ``pdf_document_generator_probe``、
  ``pdf_font_resolver``、``pdf_text_metrics``、``pdf_text_shaper``、
  ``pdf_document_adapter_font``、``pdf_cli_export``、``pdf_cli_import``、
  ``pdf_import_structure``、``pdf_import_failure`` 和
  ``pdf_import_table_heuristic``。
* ``contract-static`` 固定覆盖 10 个 docs/layout/ctest 契约测试：
  ``pdf_import_docs_contract``、``pdf_ctest_timeout_contract``、
  ``pdf_ctest_label_contract``、``pdf_bidi_line_layout_static_contract``、
  ``pdf_document_style_gallery_contract``、``pdf_document_font_matrix_contract``、
  ``pdf_document_table_font_matrix_contract``、
  ``pdf_cjk_copy_search_matrix_contract``、
  ``pdf_cjk_font_embed_matrix_contract`` 和
  ``pdf_cjk_anchor_font_matrix_boundary_contract``。
* ``cjk-flow-static`` 固定覆盖 10 个 CJK / RTL flow 静态契约测试：
  ``pdf_cjk_font_search_density_flow_contract``、
  ``pdf_cjk_font_embed_wrap_mix_contract``、
  ``pdf_cjk_repeated_key_boundary_flow_contract``、
  ``pdf_cjk_style_overlay_page_flow_contract``、
  ``pdf_cjk_complex_layout_contract``、
  ``pdf_cjk_image_wrap_stress_contract``、
  ``pdf_cjk_extreme_page_breaks_contract``、
  ``pdf_cjk_vertical_merge_wrap_cant_split_contract``、
  ``pdf_rtl_bidi_light_contract`` 和 ``pdf_cjk_list_page_flow_contract``。
* ``regression-basic-text`` 固定覆盖 10 个真实 ``pdf_regression_`` 基础文本样本测试：
  ``pdf_regression_manifest``、``pdf_regression_single-text``、
  ``pdf_regression_multi-page-text``、``pdf_regression_font-size-text``、
  ``pdf_regression_color-text``、``pdf_regression_three-page-text``、
  ``pdf_regression_landscape-text``、``pdf_regression_title-body-text``、
  ``pdf_regression_dense-text`` 和 ``pdf_regression_four-page-text``。
* ``regression-styled-document`` 固定覆盖 10 个真实 ``pdf_regression_``
  样式/文档样本测试：``pdf_regression_styled-text``、
  ``pdf_regression_mixed-style-text``、``pdf_regression_contract-cjk-style``、
  ``pdf_regression_document-contract-cjk-style``、
  ``pdf_regression_underline-text``、``pdf_regression_strikethrough-text``、
  ``pdf_regression_superscript-subscript-text``、
  ``pdf_regression_style-superscript-subscript-text``、
  ``pdf_regression_document-style-gallery-text`` 和
  ``pdf_regression_document-font-matrix-text``。
* ``regression-business-samples`` 固定覆盖 10 个真实 ``pdf_regression_``
  业务样本测试，覆盖合同、发票 / 报价单、图文报告、长文档和多 section
  文档：``pdf_regression_contract-cjk-style``、
  ``pdf_regression_document-contract-cjk-style``、
  ``pdf_regression_invoice-grid-text``、``pdf_regression_document-invoice-table-text``、
  ``pdf_regression_image-report-text``、``pdf_regression_cjk-image-report-text``、
  ``pdf_regression_document-cjk-image-wrap-stress-text``、
  ``pdf_regression_long-report-text``、``pdf_regression_document-long-flow-text`` 和
  ``pdf_regression_sectioned-report-text``。
* ``regression-table-layout`` 固定覆盖 10 个真实 ``pdf_regression_``
  表格布局样本测试：``pdf_regression_table-like-grid-text``、
  ``pdf_regression_invoice-grid-text``、
  ``pdf_regression_document-table-semantics-text``、
  ``pdf_regression_document-invoice-table-text``、
  ``pdf_regression_document-table-header-footer-variants-text``、
  ``pdf_regression_document-table-wrap-flow-text``、
  ``pdf_regression_document-table-cant-split-text``、
  ``pdf_regression_document-table-merged-cells-text``、
  ``pdf_regression_document-table-merged-header-repeat-text`` 和
  ``pdf_regression_document-table-merged-header-footer-variants-text``。该子集只声明已实际运行的
  非 skipped 表格样本，不覆盖当前环境会 skip 的 CJK 表格变体。
* summary 必须写出 ``status = pass``、``verdict = pass``、
  ``subset = smoke-import``、``subset = contract-static`` 或
  ``subset = cjk-flow-static``、``subset = regression-basic-text``、
  ``subset = regression-styled-document``、``subset = regression-business-samples``、
  ``subset = regression-table-layout``、
  ``selected_test_count = 10``、``skipped_test_count = 0`` 和
  ``ctest_timeout_seconds = 60``。``scripts/run_pdf_ctest_bounded_subset.ps1`` 遇到任何
  ``***Skipped`` CTest 项时会把 ``status`` / ``verdict`` 标记为 ``fail`` 并返回失败。
* 固定标记：``pdf_ctest_bounded_subset_release_trace``、
  ``pdf_ctest_bounded_contract_static_release_trace``、
  ``pdf_ctest_bounded_cjk_flow_static_release_trace``、
  ``pdf_ctest_bounded_regression_basic_text_release_trace``、
  ``pdf_ctest_bounded_regression_styled_document_release_trace``、
  ``pdf_ctest_bounded_regression_business_samples_release_trace``、
  ``pdf_ctest_bounded_regression_table_layout_release_trace``。

这些路径只补足资源受限时的 CTest smoke/import、静态契约、CJK / RTL flow
静态契约、基础文本 regression、样式/文档 regression 与真实业务样本 regression
证据，以及非 skipped 表格布局 regression 证据；它们不替代完整 PDF visual gate、
不替代 ``pdf_regression_`` 全量样本链，也不把被 60 秒外层保护截断的完整
``pdf_`` 套件标记为通过。

release governance 现在会把 bounded CTest 作为独立辅助证据同块写入
``source_report:``，字段包括 ``pdf_bounded_ctest_summary_count``、
``pdf_bounded_ctest_pass_count``、``pdf_bounded_ctest_skipped_test_count``、
``pdf_bounded_ctest_selected_test_count``、``pdf_bounded_ctest_subsets`` 和
``pdf_bounded_ctest_summary_json_display``。固定标记：
``pdf_bounded_ctest_governance_trace``、
``pdf_bounded_ctest_source_report_block_trace``。这些字段不能替代
``full_visual_gate_status`` 或 full gate summary verdict。
``release_blocker_rollup.md`` 的 ``Source Report Contracts`` 也必须在同一个
``featherdoc.release_candidate_summary`` list block 中保留 PDF visual gate status、
``full_visual_gate_status``、verdict、finalizable、summary/contact sheet、CJK/baseline
计数和 bounded CTest 字段，不能由 detached notes 或非 release-candidate schema 补齐。
固定标记：``pdf_visual_gate_rollup_material_safety_trace``。
自 2026-05-26 起，``scripts/run_release_candidate_checks.ps1`` 还必须把
fresh 非 ``FinalizeOnly`` attempt 未完成的事实写成顶层 ``warnings[]``，
稳定 id 为 ``pdf_visual_gate_attempt.incomplete_fresh_render``。该 warning 必须直接携带
``attempt-summary.json`` 路径、outer guard ``timed_out`` / ``true`` / ``60``、
``visual_gate_release_evidence_accepted``、
``visual_gate_fresh_full_guarded_evidence``、
``visual_gate_pass_summary_before_outer_timeout``、
``visual_gate_segmented_full_coverage_evidence`` 和 ``visual_gate_finalize_only``，
并按当前 evidence path 说明发布结论依赖 segmented full-coverage、fresh guarded
full gate 或 explicit ``FinalizeOnly`` 证据；
它会继续流入 release blocker rollup 与 governance handoff，作为 reviewer-facing
提示，而不是新的 release blocker。

2026-05-26 之后，非 ``FinalizeOnly`` full visual gate 如果被外层 60 秒保护截断，
必须先运行 ``scripts/write_pdf_visual_gate_attempt_summary.ps1``，把本次尝试写成
``output/pdf-visual-release-gate-current/report/attempt-summary.json``。该 summary 的
schema 为 ``featherdoc.pdf_visual_gate_attempt_summary.v1``，固定标记：
``pdf_visual_gate_attempt_summary_trace``、
``pdf_visual_gate_attempt_governance_trace``。release governance 会把
``pdf_visual_gate_attempt_status``、``pdf_visual_gate_attempt_verdict``、
``pdf_visual_gate_attempt_full_visual_gate_status``、
``pdf_visual_gate_attempt_evidence_scope``、
``pdf_visual_gate_attempt_pdf_regression_selected_test_count``、
``pdf_visual_gate_attempt_pdf_regression_failed_test_count``、
``pdf_visual_gate_attempt_pdf_regression_skipped_test_count``、
``pdf_visual_gate_attempt_outer_guard_status``、
``pdf_visual_gate_attempt_outer_guard_timed_out``、
``pdf_visual_gate_attempt_outer_guard_timeout_seconds``、
``pdf_visual_gate_attempt_visual_baseline_render_status`` 和
``pdf_visual_gate_attempt_aggregate_contact_sheet_status`` 作为同一
``source_report:`` block 的辅助证据。``bounded_attempt_auxiliary_only`` 只能解释
已完成子阶段；当 ``verdict = not_complete`` 时，仍不能替代
``full_visual_gate_status = pass``。``assert_release_material_safety.ps1`` 会审计
这些 ``pdf_visual_gate_attempt_*`` 字段必须与 ``source_report:`` 同块，避免
detached notes 单独补齐被截断尝试的子阶段证据。固定标记：
``pdf_visual_gate_attempt_material_safety_trace``。
``release_blocker_rollup.md`` 的 ``Source Report Contracts`` 也必须把 attempt status、
verdict、full status、``bounded_attempt_auxiliary_only`` scope、``attempt-summary.json``、
outer guard ``timed_out`` / ``true`` / ``60``、regression/CJK/render 计数和 contact sheet
保留在同一个 release-candidate list block。固定标记：
``pdf_visual_gate_attempt_rollup_material_safety_trace``。
如果本次尝试由外层 60 秒保护截断，
attempt summary 必须写出 ``outer_guard_status = timed_out``、
``outer_guard_timed_out = true`` 和 ``outer_guard_timeout_seconds = 60``；这些字段
只解释外层保护状态，不能替代 fresh full visual gate pass。固定标记：
``pdf_visual_gate_attempt_outer_guard_trace``。
``final_review.md`` 同步展示 attempt 证据时，``PDF visual gate attempt:``、
``PDF visual gate attempt verdict:``、``PDF visual gate attempt full status:``、
``PDF visual gate attempt stages:``、``PDF visual gate attempt pdf_regression:`` 和
``PDF visual gate attempt render:`` 必须保留在 ``## Step status`` 的同一连续
Markdown list run；``PDF visual gate attempt summary:`` 与
``PDF visual gate attempt contact sheet:`` 必须保留在 ``## Key outputs``，且路径
必须分别直接携带 ``attempt-summary.json`` 和 ``aggregate-contact-sheet.png``。
detached notes 不能补齐这些 reviewer-facing 入口证据。固定标记：
``pdf_visual_gate_attempt_final_review_material_safety_trace``。

如果 44 个 visual baseline 无法在单个 60 秒外层保护内一次性重渲染，可以使用
``scripts/run_pdf_visual_release_gate.ps1 -VisualBaselineSliceOnly`` 做受控切片。
该模式必须携带 ``VisualBaselineOffset`` 和 ``VisualBaselineLimit``，只重渲染选中的
baseline，并生成
``visual-baseline-slice-offset-<n>-limit-<m>-summary.json`` 与对应 contact sheet。
slice summary 的 schema 为 ``featherdoc.pdf_visual_baseline_slice.v1``，并固定写出
``evidence_scope = visual_baseline_slice_only``、
``full_visual_gate_status = not_complete`` 和
``slice_summary_does_not_replace_full_visual_gate_verdict``。固定标记：
``pdf_visual_baseline_slice_summary_trace``。这些切片只能推进 fresh baseline render
证据，不能替代 ``full_visual_gate_status = pass`` 或 full gate summary verdict。

当 baseline 已经存在但需要把 aggregate contact sheet 单独刷新进 60 秒外层保护时，
可以运行 ``scripts/run_pdf_visual_release_gate.ps1 -RebuildAggregateContactSheetOnly``。
该模式只读取现有 baseline summary / ``page-01.png``，重建
``report/aggregate-contact-sheet.png``，并写出
``aggregate-contact-sheet-rebuild-summary.json``。该 summary 的 schema 为
``featherdoc.pdf_visual_aggregate_contact_sheet_rebuild.v1``，并固定写出
``evidence_scope = aggregate_contact_sheet_rebuild_only``、
``full_visual_gate_status = not_complete`` 和
``aggregate_rebuild_summary_does_not_replace_full_visual_gate_verdict``。固定标记：
``pdf_visual_aggregate_contact_sheet_rebuild_trace``。该证据只能证明 contact-sheet
重建阶段完成，不能替代 full gate summary verdict。

当一次 full visual gate 被拆成 attempt summary、多个 baseline slice summary 和
aggregate contact-sheet rebuild summary 后，必须再运行
``scripts/write_pdf_visual_segmented_gate_summary.ps1 -ReportDir .\output\pdf-visual-release-gate-current\report``
生成 ``segmented-summary.json``，把这些受控分段证据聚合成单一 reviewer 入口。
该 summary 的 schema 为 ``featherdoc.pdf_visual_segmented_gate_summary.v1``，
固定写出 ``status`` / ``verdict``，但即使 ``status = pass`` 且
``verdict = pass``，也只能表示分段辅助证据完整，仍必须保留
``full_visual_gate_status = not_complete``、
``evidence_scope = segmented_visual_gate_auxiliary_only`` 和
``segmented_summary_does_not_replace_full_visual_gate_verdict``。固定标记：
``pdf_visual_segmented_gate_summary_trace``、
``pdf_visual_segmented_gate_governance_trace``。release governance 会把
``pdf_visual_segmented_gate_status``、``pdf_visual_segmented_gate_verdict``、
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
``pdf_visual_segmented_gate_aggregate_contact_sheet_display``、
``pdf_visual_segmented_gate_aggregate_contact_sheet_bytes``、
``pdf_visual_segmented_gate_aggregate_rebuild_status`` 和
``pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count`` 写入同一
``source_report:`` block。它只能解释分段 visual gate 辅助证据已经闭合，
不能替代一份非 ``FinalizeOnly`` 的 full gate summary verdict。
``release_governance_handoff.md`` 中这些 segmented gate 字段必须继续与
``schema=featherdoc.release_candidate_summary`` 的 ``source_report:`` 保持同块，不能由
detached notes 补齐。固定标记：
``pdf_visual_segmented_gate_material_safety_trace``。
``release_blocker_rollup.md`` 的 ``Source Report Contracts`` 也必须把 segmented status、
verdict、full status、``segmented_visual_gate_auxiliary_only`` scope/boundary、
``segmented-summary.json``、slice coverage、contact sheet bytes/status 和 aggregate
rebuild 计数保留在同一个 release-candidate list block。固定标记：
``pdf_visual_segmented_gate_rollup_material_safety_trace``。
``final_review.md`` 同步展示 segmented gate 证据时，
``PDF visual segmented gate:``、``PDF visual segmented gate verdict:``、
``PDF visual segmented gate full status:``、``PDF visual segmented gate scope:``、
``PDF visual segmented gate slices:`` 和 ``PDF visual segmented gate coverage:`` 必须保留在
``## Step status`` 的同一连续 Markdown list run，且 scope 行必须直接携带
``segmented_visual_gate_auxiliary_only``；``PDF visual segmented gate summary:`` 与
``PDF visual segmented gate contact sheet:`` 必须保留在 ``## Key outputs``，且路径
必须分别直接携带 ``segmented-summary.json`` 和 ``aggregate-contact-sheet.png``。
detached notes 不能补齐这些 reviewer-facing 入口证据。固定标记：
``pdf_visual_segmented_gate_final_review_material_safety_trace``。

2026-05-27 当前 ``segmented-summary.json`` 已刷新为可审计的分段辅助证据：

* ``generated_at = 2026-05-27T04:36:21``。
* ``status = pass``、``verdict = pass``、``full_visual_gate_status = not_complete``。
* ``evidence_scope = segmented_visual_gate_auxiliary_only``。
* ``slice_summary_count = 6``、``slice_pass_count = 6``、``slice_failed_count = 0``。
* ``covered_baseline_count = 44``、``expected_visual_render_count = 44``。
* ``attempt_stage_count = 6``、``attempt_passed_stage_count = 6``。
* ``visual_baseline_render_status = pass``。
* ``aggregate_contact_sheet_status = pass``、``aggregate_contact_sheet_bytes = 1822428``。
* ``aggregate_rebuild_status = pass``、``aggregate_rebuild_selected_baseline_count = 44``。

``scripts/check_pdf_release_readiness.ps1`` 现在会读取
``output/pdf-visual-release-gate-current/report/segmented-summary.json``，校验
``featherdoc.pdf_visual_segmented_gate_summary.v1``，并把
``visual_segmented_gate_status``、``visual_segmented_gate_verdict``、
``visual_segmented_gate_full_visual_gate_status``、
``visual_segmented_gate_evidence_scope``、``visual_segmented_gate_covered_baseline_count``、
``visual_segmented_gate_expected_visual_render_count``、
``visual_segmented_gate_aggregate_contact_sheet_status`` 和
``visual_segmented_gate_aggregate_contact_sheet_bytes`` 暴露到 release readiness summary。
当 fresh full visual gate 仍未完整通过时，同一份 warning
``pdf_full_fresh_visual_gate.not_completed_in_current_window`` 还会携带
``segmented_gate_covered_baseline_count`` 和
``segmented_gate_aggregate_contact_sheet_bytes``。这些字段只说明分段辅助证据覆盖情况，
不能替代 fresh 非 ``FinalizeOnly`` full visual gate pass。若分段证据达到 44/44 baseline、
0 failed slice、contact sheet pass 和 aggregate rebuild pass，readiness 会写出
``visual_gate_segmented_full_coverage_evidence = true`` 并接受为发布证据；warning 仍保留
单次 fresh full gate 未闭合边界。
固定标记：``pdf_visual_segmented_gate_summary_trace``。

下一步
------

1. 继续保留远端 ``origin/codex/*`` 为只读参考库存。
2. 优先把当前 ``ready + finalize-only 可复核`` 的结论沉淀到 release/governance handoff
   和 reviewer-facing 材料，而不是继续重复前置审查。
3. 只有在外部进程明显回落、并且确实需要一份“本轮新鲜生成”的 full gate 证据时，
   才重跑重型 ``run_pdf_visual_release_gate.ps1``。
4. 如果后续要重新准备 PDF 构建输入，仍需保留
   ``tmp\pdfio-src\pdfio.h``、``FEATHERDOC_BUILD_PDF=ON``、
   ``FEATHERDOC_BUILD_PDF_IMPORT=ON`` 这些旧 runbook 提示；但它们现在属于“重跑 full gate
   的准备动作”，而不是“当前证据链仍然缺失”的判断。
5. 在 release/reviewer 材料完成同步后，再决定是否归档或回收旧 PDF 参考分支。
