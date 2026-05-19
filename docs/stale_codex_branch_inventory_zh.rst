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
* ``e05a969`` 中的 PDF 页眉页脚 run 保留修复可以低风险摘入；本轮按当前 ``dev``
  结构重做为 ``wrap_cursor_paragraph_runs``，让页眉页脚直接包装段落游标中的 run，
  复用现有字体回退、run 样式和 shaping 元数据链路。旧提交中的 bidi、上下标、
  删除线、regression manifest、样例和视觉 gate 大范围变化继续只读保留。
* ``e05a969`` 中页眉页脚段落对齐也可以独立重做：当前只补充内部
  ``HeaderFooterLineLayout``，让每条页眉页脚线携带按段落左/中/右对齐计算后的
  ``start_x_points``。该补丁不引入旧分支的 RTL 大测试、PDF regression 样例、
  manifest 或视觉 gate。


2026-05-19 PDF CJK 分支只读复核
--------------------------------

本轮继续只读复核 ``codex/pdf-cjk-copy-search-gate`` 和
``codex/pdf-cjk-bullet-fallback``。结论是：当前 ``dev`` 已有部分关键 PDF CJK
基础能力，但两个旧分支仍不能整分支合并，也不能被视为已经完全合入。

当前 ``dev`` 已确认包含：

* ``scripts/check_pdf_text_layer.py``。
* ``scripts/run_pdf_visual_release_gate.ps1`` 中的 CJK copy/search text-layer gate。
* ``scripts/run_pdf_visual_release_gate.ps1`` 中的 ``cli-cjk-font-source`` gate 条目。
* ``test/pdf_cli_export_tests.cpp`` 中显式 CJK 字体导出的覆盖。
* ``test/pdf_font_resolver_tests.cpp`` 中 East Asia / CJK 字体 fallback 契约。
* ``document-eastasia-style-probe`` 相关样本契约，用于固定 East Asia 符号探针。

仍保留在旧 PDF 分支、当前低资源阶段不搬入的内容主要是大批 regression 样例、
manifest 和视觉 gate 清单，包括但不限于：

* ``document-cjk-copy-search-matrix-text``。
* ``document-cjk-font-embed-matrix-text``。
* ``document-cjk-image-wrap-stress-text``。
* ``document-cjk-extreme-page-breaks-text``。
* ``document-cjk-table-wrap-page-flow-text``。
* ``document-cjk-multi-anchor-table-flow-text``。
* ``document-cjk-anchor-font-matrix-boundary-text``。
* ``document-cjk-font-search-density-flow-text``。

处理建议保持不变：两个 PDF CJK 分支继续作为只读参考库存保留。后续如果正式恢复
PDF 主线，应从当前 ``dev`` 出发，按单个能力小步重做源码、脚本契约和最小验证；
不要搬入整批样例、manifest 或视觉 baseline，也不要在未完成源码提交推送前运行
重型 PDF 渲染验证。

本轮进一步按提交粒度复核了两个 PDF CJK 分支中的小候选。当前 ``dev`` 已有等价
实现，不再重复摘入：

* ``34fa0e8`` 的 CJK copy/search text-layer gate 已由当前 ``dev`` 的
  ``639e1e2``、``scripts/check_pdf_text_layer.py`` 和
  ``scripts/run_pdf_visual_release_gate.ps1`` 覆盖。
* ``1fb55a1`` 的 CLI CJK PDF export 覆盖已由当前 ``dev`` 的 ``c9f6000``、
  ``62f0e84`` 和 ``test/pdf_cli_export_tests.cpp`` 覆盖。
* ``347f3ff`` 的纵向合并表格分页修复已由当前 ``dev`` 的 ``2c1564a`` 覆盖，
  分页判断会使用 ``spanned_row_bottom`` 计算跨行单元格实际输出高度。
* ``3be71b2`` 中可低风险摘入的 exact-height 表头 fitting 已由当前 ``dev`` 的
  ``bb6bbbd`` 覆盖；其余 visual gate、manifest 和样例继续只读保留。
* ``b52bf60``、``96bac82`` 和 ``11dc255`` 中的 CJK bullet / East Asia 字体
  fallback 源码能力已由当前 ``dev`` 的 ``5d67c68``、``77680a7``、``91162e5`` 和
  ``pdf_font_resolver_tests.cpp`` / ``pdf_document_adapter_font_tests.cpp`` 覆盖。
* ``document-cjk-numbered-list-page-flow-text``、
  ``document-cjk-bullet-page-flow-text`` 和
  ``document-cjk-bullet-overlay-page-flow-text`` 已按当前 ``dev`` 的轻量样例结构重做为
  1 页 PDF CJK list page-flow 契约入口，并通过静态契约纳入 CJK font gate。
* ``document-table-font-matrix-text`` 已按当前 ``dev`` 的轻量样例结构重做为
  1 页 PDF table font matrix 契约入口，覆盖表格单元格 CJK / RTL 字体映射、bidi 单元格、
  重复表头、cant-split 行和页眉页脚占位符；旧分支的 3 页 visual baseline 继续只读保留。
* ``document-table-cjk-wrap-flow-text`` 已按当前 ``dev`` 的轻量样例结构重做为
  1 页 PDF table CJK wrap flow 契约入口，覆盖 CJK 长文本单元格换行、页眉页脚占位符、
  重复表头、cant-split 行和稳定检索键；旧分支的 2 页 visual baseline 继续只读保留。
* ``document-cjk-copy-search-matrix-text`` 已按当前 ``dev`` 的轻量样例结构重做为
  1 页 PDF CJK copy/search matrix 契约入口，覆盖 CJK 文本层检索锚点、first/even/default
  页眉页脚占位符、重复表头和表格检索键；旧分支的 3 页大矩阵正文和 visual baseline
  继续只读保留，等待最终受控可视化验证。
* ``document-cjk-font-embed-matrix-text`` 已按当前 ``dev`` 的轻量样例结构重做为
  1 页 PDF CJK font embed matrix 契约入口，覆盖 CJK 字体嵌入、styled run 大小字切换、
  first/even/default 页眉页脚占位符、重复表头和表格字体检索键；旧分支的 3 页大矩阵正文
  和 visual baseline 继续只读保留，等待最终受控可视化验证。
* ``document-cjk-anchor-font-matrix-boundary-text`` 已按当前 ``dev`` 的轻量样例结构重做为
  1 页 PDF CJK anchor font matrix boundary 契约入口，覆盖 CJK 锚点字体矩阵、styled run、
  first/even/default 页眉页脚占位符、重复表头、cant-split 尾行和边界表格检索键；
  旧分支的 4 页图片压力语料和 visual baseline 继续只读保留，等待最终受控可视化验证。
* ``document-cjk-font-search-density-flow-text`` 已按当前 ``dev`` 的轻量样例结构重做为
  1 页 PDF CJK font search density flow 契约入口，覆盖 CJK 密排检索、styled run、
  first/even/default 页眉页脚占位符、重复表头、cant-split 尾行和检索密度表；
  旧分支的 4 页图片压力语料和 visual baseline 继续只读保留，等待最终受控可视化验证。
* ``document-cjk-font-embed-wrap-mix-text`` 已按当前 ``dev`` 的轻量样例结构重做为
  1 页 PDF CJK font embed wrap mix 契约入口，覆盖 CJK 字体嵌入、styled run、
  first/even/default 页眉页脚占位符、重复表头、cant-split 尾行和环绕混排表格；
  旧分支的 4 页图片压力语料和 visual baseline 继续只读保留，等待最终受控可视化验证。
* ``document-cjk-repeated-key-boundary-flow-text`` 已按当前 ``dev`` 的轻量样例结构
  重做为 1 页 PDF CJK repeated key boundary flow 契约入口，覆盖 CJK 重复检索键、
  styled run、first/even/default 页眉页脚占位符、重复表头、合并单元格和
  cant-split 行；旧分支的 6 页图片压力语料和 visual baseline 继续只读保留，等待
  最终受控可视化验证。
* ``document-cjk-style-overlay-page-flow-text`` 已按当前 ``dev`` 的轻量样例结构重做为
  1 页 PDF CJK style overlay page-flow 契约入口，覆盖 CJK style overlay、
  superscript、subscript、strikethrough、first/even/default 页眉页脚占位符、
  重复表头、合并单元格、cant-split 行和 styled table cell 段落；旧分支的 6 页图片
  压力语料和 visual baseline 继续只读保留，等待最终受控可视化验证。
* ``document-cjk-complex-layout-text`` 已按当前 ``dev`` 的轻量样例结构重做为
  1 页 PDF CJK complex layout 契约入口，覆盖 CJK 复杂版式概览、styled run、
  first/even/default 页眉页脚占位符、重复表头、合并单元格、cant-split 行和表格
  检索键；旧分支的 3 页浮动图片、裁剪环绕压力语料和 visual baseline 继续只读保留，
  等待最终受控可视化验证。
* ``document-table-cjk-vertical-merged-cant-split-text`` 已按当前 ``dev`` 的轻量样例
  结构重做为 1 页 PDF table CJK vertical merged cant-split 契约入口，覆盖 CJK 表格
  纵向合并、横向合并、重复表头、cant-split 行、垂直居中和页眉页脚占位符；旧分支的
  多页 visual baseline 继续只读保留，等待最终受控可视化验证。
* ``document-table-cjk-merged-repeat-text`` 已按当前 ``dev`` 的轻量样例结构重做为
  1 页 PDF table CJK merged repeat 契约入口，覆盖 CJK 表格横向合并看板表头、纵向
  合并负责人块、重复表头、cant-split 尾行、垂直居中和页眉页脚占位符；旧分支的
  4 页 visual baseline 继续只读保留，等待最终受控可视化验证。
* ``document-cjk-table-wrap-page-flow-text`` 已按当前 ``dev`` 的轻量样例结构重做为
  1 页 PDF CJK table wrap page-flow 契约入口，覆盖 CJK 表格换行、合并看板表头、
  纵向合并阶段块、重复表头、cant-split 尾行和页眉页脚占位符；旧分支的 5 页图片
  压力语料和 visual baseline 继续只读保留，等待最终受控可视化验证。
* ``document-cjk-multi-anchor-table-flow-text`` 已按当前 ``dev`` 的轻量样例结构重做为
  1 页 PDF CJK multi-anchor table-flow 契约入口，覆盖 CJK 多锚点文本流、first/even/
  default 页眉页脚占位符、合并看板表头、纵向合并锚点块、重复表头、cant-split 行和
  styled CJK run；旧分支的 5 页图片压力语料和 visual baseline 继续只读保留，等待
  最终受控可视化验证。

因此，当前 PDF CJK 分支剩余价值主要是大规模 regression 样例库、manifest 条目、
视觉 baseline 和后续可视化验收脚本参考；低资源阶段没有新的独立源码小补丁需要从这
两个分支搬入。


2026-05-19 release governance warning-entrypoints 只读复核
----------------------------------------------------------

本轮只读复核 ``codex/release-governance-warning-entrypoints`` 中的 reviewer checklist
入口相关提交。该分支相对当前 ``dev`` 仍有大量差异，并且会删除当前 ``dev`` 的恢复、
整合和 PDF CJK 复核文档，因此仍不能整分支合并。

当前 ``dev`` 已确认覆盖旧分支中的关键入口能力：

* ``Get-ReleaseGovernanceBlockerChecklistItems``。
* ``Get-ReleaseGovernanceActionItemChecklistItems``。
* ``Get-ReleaseGovernanceWarningChecklistItems``。
* reviewer checklist 会分别渲染 blocker、action item 和 warning 的 checkbox。
* checklist guidance 会透传 ``command``、``open_command``、``audit_command`` 和
  ``review_command``。
* 当前实现额外保留 ``project_id``、``template_name``、``candidate_type``、
  ``source_json_display``、``repair_strategy`` 和 ``command_template`` 等后续治理字段。

已用 60 秒限时的 ``release_governance_warning_helper_contract_test.ps1`` 确认当前
``dev`` 的 helper 契约仍通过。结论是：旧分支中的 checklist 入口提交不再需要重复
摘入；后续如果继续处理该分支，只应寻找当前 ``dev`` 尚未覆盖、且不会回退现有治理契约
的小补丁。CTest 注册、旧 metrics 测试和大范围 release bundle 文案调整继续保留为只读
参考，不在低资源阶段处理。


2026-05-19 release governance rollup-details 只读复核
-----------------------------------------------------

本轮只读复核 ``codex/release-governance-rollup-details`` 剩余提交。该分支相对当前
``dev`` 仍有 8 个未直接合入提交，但其中治理详情链路已经被当前 ``dev`` 用新的脚本
结构覆盖；整分支仍会回退当前文档恢复记录、PDF CJK 复核记录和 release warning
契约，因此不直接合并。

当前 ``dev`` 已确认覆盖以下旧分支方向：

* release blocker rollup 会汇总 content-control、project onboarding、schema
  confidence calibration 等治理来源。
* release governance handoff 会保留 release blocker rollup 嵌套摘要、明细来源和
  ``source_json_display``。
* release governance pipeline 会渲染阶段详情、最终 rollup、handoff 和 blocker
  来源信息。
* schema patch confidence calibration 会保留 ``project_id``、``template_name``、
  ``candidate_type``、候选类型汇总和 action item / blocker / warning 路由字段。
* rollup、handoff、pipeline 与 reviewer material helper 均会继续透传
  ``project_id``、``template_name``、``candidate_type``、``source_report_display`` 和
  ``source_json_display``。

已用 60 秒限时的轻量 PowerShell 契约测试确认当前 ``dev`` 仍通过：

* ``build_release_blocker_rollup_report_test.ps1 -Scenario passing``。
* ``build_release_governance_handoff_report_test.ps1 -Scenario aggregate``。
* ``build_release_governance_pipeline_report_test.ps1 -Scenario aggregate``。
* ``write_schema_patch_confidence_calibration_report_test.ps1 -Scenario aggregate``。

结论是：``505baa0``、``bc6ef28``、``68ce74f``、``b1a9948``、``460ff95`` 和
``5c41714`` 的治理详情目标已由当前 ``dev`` 覆盖，不再重复摘入；``3bd950d`` 的
candidate routing 也已在前序提交中按当前结构重做。``5e8b859`` 混合了 PDF/RTL
文档、PDFium parser、unicode font roundtrip 大测试和 release governance 调整，
属于重型综合提交，当前低资源阶段继续只读保留，不拆入本轮。


2026-05-19 远端参考分支再复核
------------------------------

本轮在 ``dev`` 已完成第一阶段和第二阶段轻量验证后，重新抓取 ``origin`` 并复核四个
远端参考分支：

* ``origin/codex/release-governance-warning-entrypoints``。
* ``origin/codex/release-governance-rollup-details``。
* ``origin/codex/pdf-cjk-copy-search-gate``。
* ``origin/codex/pdf-cjk-bullet-fallback``。

复核结论没有变化：四个分支仍作为只读参考库存保留。当前 ``dev`` 新增了轻量验证清单
和验证结果记录，因此差异计数会继续变化；这不代表旧分支出现了新的低风险源码补丁。

本轮未发现需要从旧分支直接手工重做的新小补丁。后续最小风险动作是继续保持
``dev`` 与 ``origin/dev`` 对齐，等资源允许后再进入受控的 Word/PDF 可视化验证前置
检查；PDF CJK 大批 regression 样例、manifest 和视觉 baseline 仍不在低资源阶段搬入。
