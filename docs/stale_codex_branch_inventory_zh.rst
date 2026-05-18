旧 Codex 分支库存记录
======================

记录日期：2026-05-18

本文记录当前仍保留在远端的 ``codex/*`` 分支，避免误以为这些分支已经全部合入
``dev``。结论是：这些分支都还有未合并提交，不能在没有归档或明确废弃决定的
情况下直接删除。


当前原则
--------

* ``dev`` 是开发主线。
* ``master`` 只在发布确认后从 ``dev`` 合入。
* 旧 ``codex/*`` 分支只作为参考库存，不再整分支 merge。
* 删除远端分支前，必须先确认该分支功能已经合入、归档，或明确废弃。
* PDF 深水区分支继续冻结；当前只摘入低风险基础设施和字体回退补丁，不整分支扩展
  PDF 主线。


已确认摘入 ``dev`` 的内容
-------------------------

已从旧分支安全摘入的内容只有小范围治理片段、PDFium 基础设施补丁和按当前
``dev`` 重做的 PDF 字体回退小补丁：

.. code-block:: text

   8a3084e Ignore local test work directories
   f9cc0e0 Guard release warning docs contract
   3bd950d Add schema calibration candidate routing
   fcb5f9c Fix release blocker rollup test composite id assertion
   1c5cd2b Add PDFium prebuilt provider option
   df2e4f6 Ignore temporary PDFium probe directories
   6760162 Fail fast when PDFium depot_tools bootstrap is unavailable
   bf945ae Document PDFium source bootstrap guard
   779b351 Add PDFium auto provider selection
   91162e5 Add PDF font glyph fallback guard

其中 ``8a3084e`` 已形成当前 ``dev`` 上的：

.. code-block:: text

   2ccaae5 Ignore local test work directories

``f9cc0e0`` 已形成当前 ``dev`` 上的：

.. code-block:: text

   436902e Guard release warning docs contract

``3bd950d`` 已按当前 ``dev`` 的脚本契约手工重做为：

.. code-block:: text

   88e9187 Carry scoped schema calibration routing

``fcb5f9c`` 已按当前 ``dev`` 的 rollup 测试结构手工重做为：

.. code-block:: text

   36fb375 Stabilize release blocker rollup id assertion

PDFium provider / bootstrap 相关小补丁已在当前 ``dev`` 形成连续提交：

.. code-block:: text

   1c5cd2b Add PDFium prebuilt provider option
   df2e4f6 Ignore temporary PDFium probe directories
   6760162 Fail fast when PDFium depot_tools bootstrap is unavailable
   bf945ae Document PDFium source bootstrap guard
   779b351 Add PDFium auto provider selection

这组提交只覆盖 provider 选择、prebuilt/package/source/auto 配置、临时目录忽略和
source bootstrap 快速失败文档，不代表 ``codex/pdf-cjk-copy-search-gate`` 或
``codex/pdf-cjk-bullet-fallback`` 的 PDF CJK 功能已经完整合入。

PDF 字体缺字形回退已按当前 ``dev`` 的 resolver 结构重做为：

.. code-block:: text

   91162e5 Add PDF font glyph fallback guard

该提交保留当前 ``dev`` 已有 synthetic bold / italic 和 text shaping 结构，只补充
FreeType 字形检查：当 Latin 字体已经命中但缺少当前 Unicode 文本的非忽略字形时，
resolver 再尝试 East Asia 显式映射、``cjk_font_file_path`` 或系统 CJK fallback。
这不代表旧 PDF 分支中的字体矩阵、CJK copy/search、表格流或视觉 gate 已整体合入。

除此之外的文档治理提交，例如 ``0d0b7bc``、``b5a806d``、``c1a9e9a`` 和
``a5bf1b7``，是在当前 ``dev`` 上直接开发或按当前契约重做，不是整分支 merge。


分支清单
--------

``codex/release-governance-warning-entrypoints``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

状态：

* 未完整合并。
* 相对当前 ``dev`` 仍有大量独有治理提交。
* 其中 release warning docs contract 和 rollup composite id 测试修复已经被手工摘入。
* 当前 ``dev`` 已有更完整的 warning metadata、style merge governance、
  release rollup / handoff / pipeline 明细实现，旧分支整体已经不能作为正确版本。

代表性未合并内容包括：

* release governance blocker / warning / action item 明细展示。
* style merge review / restore audit 相关脚本与测试。
* pipeline final-rollup / fail-on-blocker 的 CTest 注册。
* release bundle、reviewer checklist、handoff 细节扩展。

处理建议：

* 暂不删除。
* 不整分支合并。
* 后续如继续推进治理主线，只允许从当前 ``dev`` 出发，手工复核并重做仍有价值的
  小块功能。


``codex/release-governance-rollup-details``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

状态：

* 未完整合并。
* 之前已预检，直接 merge 和逐提交 cherry-pick 都会产生多处冲突。
* 与当前 ``dev`` 的文档治理和 release metadata 契约重叠较深。
* 其中 schema calibration candidate routing 已经按当前 ``dev`` 手工摘入。
* 当前 ``dev`` 已包含 schema calibration 进入 handoff / pipeline / release rollup 的
  更完整实现，不再继续从该旧分支回放 release rollup 提交。

代表性未合并内容包括：

* release governance rollup details。
* 部分早期 schema confidence calibration release rollup 实现。
* onboarding governance / content-control governance / handoff detail 透传。
* 部分 PDF unicode / RTL 相关覆盖。

处理建议：

* 暂不删除。
* 不直接合并。
* 后续若需要其中能力，应以当前 ``dev`` 的脚本和测试为准重新实现，而不是回放旧提交。


``codex/pdf-cjk-copy-search-gate``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

状态：

* 未合并。
* 属于 PDF 深水区分支。
* 相对当前 ``dev`` 包含大量 PDF CJK copy/search、font matrix、table flow 和视觉 gate
  相关提交。
* 已安全摘入的只是 PDFium provider / bootstrap 基础设施和字体缺字形回退小补丁，
  剩余 PDF CJK 功能仍未按当前 ``dev`` 重新实现。

代表性未合并内容包括：

* CJK PDF copy/search release gate。
* 多脚本字体矩阵样例。
* wrapped / merged / repeated table flow PDF regression samples。
* PDF text-layer 检查脚本和 manifest 扩展。

处理建议：

* 暂不删除，作为 PDF 参考库存保留。
* 不整分支合并。
* 只有当 PDF 被重新明确为主线时，再从当前 ``dev`` 出发单独开低风险计划处理。


``codex/pdf-cjk-bullet-fallback``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

状态：

* 未合并。
* 基于 ``codex/pdf-cjk-copy-search-gate`` 继续扩展。
* 属于 PDF 深水区分支。
* 已安全摘入的只是 PDFium provider / bootstrap 基础设施和字体缺字形回退小补丁，
  bullet fallback、CJK table gate 和 CLI PDF export 相关能力仍需另行小步复核。

代表性未合并内容包括：

* CJK bullet / numbered list fallback。
* East Asia font fallback。
* CLI PDF export coverage。
* CJK table header fitting 和 gate coverage。

处理建议：

* 暂不删除，作为 PDF 参考库存保留。
* 当前不整分支合并。
* 后续如果恢复 PDF 主线，应优先从该分支和
  ``codex/pdf-cjk-copy-search-gate`` 的关系开始梳理，避免重复处理相同 PDF 样例。


下一步建议
----------

1. 保留这四个远端分支，直到明确完成归档或废弃决策。
2. 继续在 ``dev`` 上推进文档功能主线，不再从旧分支直接整合。
3. 如果确实要清理 GitHub 分支列表，先创建只读归档记录，例如 tag、bundle 或文本清单，
   再删除远端分支。
4. 每次从旧分支借鉴功能，都应在当前 ``dev`` 上重新实现、重新测试，并在提交说明中
   写明来源和取舍。


直接合并预检记录
----------------

2026-05-18 已在本地安全锚点
``codex/dev-before-branch-merge-20260518-194700`` 之后，对四个远端分支执行
``git merge --no-ff --no-commit`` 预检。每次预检后均已执行 ``git merge --abort``，
当前 ``dev`` 未保留半合并状态。

``codex/release-governance-warning-entrypoints`` 直接合并失败，冲突集中在：

* ``docs/current_direction_zh.rst``
* ``docs/feature_gap_analysis_zh.rst``
* ``docs/index.rst``
* ``scripts/build_content_control_data_binding_governance_report.ps1``
* ``scripts/build_numbering_catalog_governance_report.ps1``
* ``scripts/build_project_template_delivery_readiness_report.ps1``
* ``scripts/build_release_blocker_rollup_report.ps1``
* ``scripts/build_release_governance_handoff_report.ps1``
* ``scripts/build_release_governance_pipeline_report.ps1``
* ``scripts/check_release_metadata_docs.ps1``
* ``scripts/run_release_candidate_checks.ps1``
* release bundle writer 脚本和多组 governance 测试

``codex/release-governance-rollup-details`` 直接合并失败，冲突集中在：

* ``design/04-pdf-execution-plan.md``
* ``docs/current_direction_zh.rst``
* ``docs/feature_gap_analysis_zh.rst``
* ``docs/release_metadata_pipeline_zh.rst``
* release governance rollup / handoff / pipeline 脚本
* project template onboarding / schema calibration 脚本和测试
* ``test/release_note_bundle_version_test.ps1``

``codex/pdf-cjk-copy-search-gate`` 直接合并失败，冲突集中在：

* ``BUILDING_PDF.md``
* ``design/04-pdf-execution-plan.md``
* ``include/featherdoc/pdf/pdf_layout.hpp``
* ``scripts/run_pdf_visual_release_gate.ps1``
* PDF adapter / writer C++ 实现
* PDF regression manifest 和 PDF 字体 / unicode 测试

``codex/pdf-cjk-bullet-fallback`` 直接合并失败，冲突范围与
``codex/pdf-cjk-copy-search-gate`` 相近，并额外覆盖：

* ``src/pdf/pdf_font_resolver.cpp``
* ``test/pdf_cli_export_tests.cpp``
* ``test/pdf_font_resolver_tests.cpp``

因此当前结论是：四个旧分支都不适合整分支直接合入 ``dev``。后续只能按功能主题
拆分，例如继续完善 release governance action item 明细、校准 schema calibration
真实语料置信度，再另开 PDF CJK 专项；每个主题都应在当前 ``dev`` 上小步重做并跑
对应轻量测试。


2026-05-18 后续摘入复核
------------------------

本轮继续按“谁的实现更正确就用谁”的原则复核。结论如下：

* ``codex/release-governance-rollup-details`` 的 ``3bd950d`` 只摘入候选级
  schema calibration routing；整分支仍会回退当前 ``dev`` 的 style merge 工具链、
  release warning 契约和 PDF import 文档维护，因此不合并。
* ``codex/release-governance-warning-entrypoints`` 的 ``fcb5f9c`` 只摘入 rollup
  composite id 测试鲁棒性修复；该分支其余内容已被当前 ``dev`` 的更完整实现覆盖，
  或仅涉及 CTest 注册，当前低资源阶段不继续处理。
* content-control governance source metadata 已在当前 ``dev`` 中更完整地覆盖：
  ``source_json_display``、``source_report_display``、``repair_strategy`` 和
  ``command_template`` 均已通过脚本与测试保留，不再从旧分支重复摘入。
* release governance rollup details、schema calibration handoff / pipeline / release
  bundle 透传已在当前 ``dev`` 中存在更完整实现；旧分支版本不再作为正确版本使用。
* PDF 分支中已低风险摘入 PDFium prebuilt provider、临时 probe 目录忽略、
  depot_tools bootstrap guard 文档和默认 ``auto`` provider 选择。剩余 CJK
  copy/search、bullet fallback、字体矩阵、表格流和视觉 gate 差异仍很大，不再
  直接合并；后续只从当前 ``dev`` 手工重做独立小块。
* ``91162e5`` 已按当前 ``dev`` 重做 East Asia 字体缺字形 fallback：Latin 字体缺少
  Unicode 符号或混排标点字形时，resolver 会尝试切到 East Asia / CJK 字体。该补丁
  来源于 PDF 分支思路，但不是整分支 cherry-pick。
* ``3be71b2`` 中的 CJK table header fitting / gate coverage 不能整体摘入；本轮仅按
  当前 ``dev`` 结构重做其中的表格分页小修复：PDF 表格分页和重复表头适配使用
  ``spanned_row_bottom`` 计算纵向合并或跨行单元格的实际输出高度，避免页底分页判断
  只看当前行高度而低估跨行表格。旧分支里的样例、manifest、视觉 gate 和字体矩阵差异
  继续保留为只读参考。
* ``b52bf60``、``96bac82`` 和 ``11dc255`` 的关键源码能力已被当前 ``dev`` 覆盖：
  ``pdf_font_resolver.cpp`` 已具备 FreeType 字形检查、Unicode prefix 到 East Asia /
  CJK 字体链路的 fallback、日文假名、韩文音节和东亚兼容符号识别；对应契约已由
  ``pdf_font_resolver_tests.cpp``、``pdf_document_adapter_font_tests.cpp`` 和
  ``document-eastasia-style-probe`` 样本固定。旧提交剩余内容主要是大批 regression
  样例、manifest 和视觉 gate 清单，当前低资源阶段不再重复摘入。
