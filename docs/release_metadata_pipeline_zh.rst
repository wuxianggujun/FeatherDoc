Release 元数据流水线（中文）
================================

这份文档用于说明 FeatherDoc 当前 release metadata 从视觉 gate 到发布包的
传递路径、字段契约和常见维护规则。它面向维护者和自动化脚本作者，重点回答
三个问题：

- 哪个脚本负责生成哪一层 metadata？
- 哪些字段可以被机器消费，哪些只适合人工审阅？
- 遇到不完整的视觉评审计数或 verdict 时应该如何降级？

本文只描述 release metadata 管线本身，不替代正式发布策略。正式发布前检查项
仍以 ``release_policy_zh.rst`` 为准。


一图看懂流水线
--------------

当前 release metadata 的主路径可以简化成下面这条链：

.. code-block:: text

    [run_word_visual_release_gate.ps1]
      ├─ gate_summary.json
      ├─ gate_final_review.md
      └─ review task outputs
              ↓
    [run_release_candidate_checks.ps1]
      ├─ report/summary.json
      ├─ report/final_review.md
      └─ release note bundle paths
              ↓
    [sync_visual_review_verdict.ps1]
      ├─ refresh gate visual_verdict
      ├─ refresh release visual_verdict
      └─ optionally refresh release note bundle
              ↓
    [write_release_note_bundle.ps1]
      ├─ START_HERE.md
      ├─ ARTIFACT_GUIDE.md
      ├─ REVIEWER_CHECKLIST.md
      ├─ release_handoff.md
      ├─ release_body.zh-CN.md
      └─ release_summary.zh-CN.md

如果只想把最新 review task 的结果回灌进去，通常优先使用
``sync_latest_visual_review_verdict.ps1``。它会先发现最新 task pointer，推断
对应 gate summary 和 release candidate summary，再委托
``sync_visual_review_verdict.ps1`` 执行真正的同步。


第一层：视觉 gate
-----------------

``run_word_visual_release_gate.ps1`` 是视觉证据的源头。它负责运行 Word 视觉
smoke、fixed-grid、section/page setup、page-number fields，以及 curated visual
regression bundles。

这一层主要产出：

- ``gate_summary.json``：机器可消费的 gate metadata。
- ``gate_final_review.md``：面向人工 reviewer 的 gate 摘要。
- ``review_tasks``：按标准任务和 curated bundle 分类的 review task 入口。
- ``review_task_summary``：本次需要评审的任务数量。

``review_task_summary`` 必须同时包含下面三个字段才算完整：

- ``total_count``
- ``standard_count``
- ``curated_count``

计数生成时应忽略 ``null``、空字符串和空白字符串等占位任务，避免把未生成的
task 误算进 review scope。


第二层：release candidate preflight
-----------------------------------

``run_release_candidate_checks.ps1`` 把构建、测试、安装 smoke、视觉 gate 和发布
说明材料串成一次本地 release preflight。

这一层主要产出：

- ``report/summary.json``：release candidate 的机器可消费总摘要。
- ``report/final_review.md``：release candidate 的人工审阅摘要。
- ``report`` 下的一组 release note bundle 文件路径。

当视觉 gate 完成后，preflight 会把 ``gate_summary.json`` 中的
``visual_verdict``、各 flow 的 review verdict、review status、review note、
review provenance，以及完整的 ``review_task_summary`` 同步到
``summary.json`` 的 ``steps.visual_gate`` 下。若同时运行 project template smoke，
preflight 还会把 schema approval 的 pending / approved / rejected / compliance /
invalid-result 计数写入 ``steps.project_template_smoke``；当
``schema_patch_approval_gate_status`` 为 ``blocked`` 时，preflight 会把该步骤标记为
failed 并阻断 release candidate。只要启用了 project template smoke，preflight 还会自动调用
``write_project_template_schema_approval_history.ps1``，在 release report 目录生成
``project_template_schema_approval_history.json`` 与 ``project_template_schema_approval_history.md``，
并把路径写回 ``summary.json``、``final_review.md``、``ARTIFACT_GUIDE.md`` 与
``REVIEWER_CHECKLIST.md``。历史报表会突出最近一次 blocking reason，并按模板 entry
汇总历史状态，帮助 reviewer 持续观察 schema approval gate 的 blocked、pending 与
passed 趋势。若 gate 被阻断，顶层 ``release_blockers`` 会同步追加稳定 id 为
``project_template_smoke.schema_approval`` 的 blocker，并写入 ``release_blocker_count``，
CI、发布面板或自动化通知可以直接读取阻断条目、issue keys、审批明细与修复动作，
无需再解析 ``final_review.md``。

如果 ``review_task_summary`` 不完整，preflight 不应把它写入 release summary，
也不应在 ``final_review.md`` 中渲染空计数行。


第三层：verdict 同步
--------------------

``sync_visual_review_verdict.ps1`` 用于在 review task 已经产生 verdict 后，把
结果回写到 gate summary 和 release candidate summary。

它负责：

- 根据各 review task 计算整体 ``visual_verdict``。
- 更新 ``gate_summary.json`` 与 ``gate_final_review.md``。
- 在提供 ``-ReleaseCandidateSummaryJson`` 时更新 release ``summary.json`` 与
  ``final_review.md``。
- 在提供 ``-RefreshReleaseBundle`` 时刷新 release note bundle。

同步规则：

- 完整的 ``review_task_summary`` 可以被复制到 release summary。
- 不完整的 ``review_task_summary`` 必须从 release summary 中移除。
- release ``final_review.md`` 只渲染完整计数，不渲染空字段。

``sync_latest_visual_review_verdict.ps1`` 是更短的入口。它负责发现最新 task
pointer、推断 summary 路径，然后调用 ``sync_visual_review_verdict.ps1``。
如果无法发现匹配的 release candidate summary，它可以只更新 gate 层。


第四层：release note bundle
---------------------------

``write_release_note_bundle.ps1`` 负责把 release candidate summary 变成一组可交付
Markdown 文件。

默认会生成或刷新：

- ``START_HERE.md``
- ``ARTIFACT_GUIDE.md``
- ``REVIEWER_CHECKLIST.md``
- ``release_handoff.md``
- ``release_body.zh-CN.md``
- ``release_summary.zh-CN.md``

当 ``summary.json`` 顶层存在 ``release_blockers`` 时，bundle 会在
``START_HERE.md``、``ARTIFACT_GUIDE.md``、``REVIEWER_CHECKLIST.md``、
``release_handoff.md`` 与 ``release_body.zh-CN.md`` 中渲染 blocker count、
稳定 blocker id、issue keys、阻断条目和修复动作。``REVIEWER_CHECKLIST.md``
还会生成必须先清空 ``release_blockers`` 的 stop checklist，确保人工交接不会漏看
机器字段里的发布阻断原因。bundle 生成前会校验 ``release_blocker_count`` 必须等于
``release_blockers`` 的实际条目数，同时要求每个 blocker 的 ``id``、``source``、
``status``、``severity`` 与 ``action`` 非空且 ``id`` 不重复；不一致会直接失败，
避免交接文档展示错误或不可追溯的阻断信息。``REVIEWER_CHECKLIST.md`` 会把已知
``action`` 映射为固定人工 runbook，例如 schema approval blocker 的
``fix_schema_patch_approval_result`` 会提示 reviewer 更新 approval result、补齐审计字段、
重新同步 schema approval metadata 并重新生成 release note bundle。未知 ``action`` 不会
阻断 bundle 生成，但 checklist 会标记未登记 runbook，提醒维护者把新 action 加入
``release_blocker_metadata_helpers.ps1`` 的注册表和固定指引。
PDF visual release gate 预检治理报告产生的
``prepare_pdf_visual_release_gate_build_outputs`` 与
``rerun_pdf_visual_release_gate_preflight`` 也已映射到固定 runbook：reviewer 先查看
``source_report_display`` / ``source_json_display`` 指向的预检证据，再运行轻量
``-PreflightOnly`` 或预检重建命令；只有预检 ready 且资源允许时才进入完整 PDF visual gate，
最后重新生成 release blocker rollup 与 release note bundle。``-PreflightOnly`` 和
预检 summary 只证明前置条件，不是 release-ready evidence；只有完整 PDF visual gate
基于当前 ``dev`` 通过后，才能作为发布就绪证据。
这些 runbook 还会直接展示 ``output gap checks`` 和 ``missing outputs``，与
preflight summary 的顶层 ``output_gap_count`` / ``missing_output_count`` 保持一致，
避免 reviewer 需要手动合计 CLI baseline、visual baseline 和 CJK text-layer 缺口。
无论是预检还是完整 gate，运行完成后都只清理本任务启动的 PDF gate 临时输出和相关进程；
不要结束与 FeatherDoc 无关的 Office、浏览器、node 或其它外部构建进程。

当 release summary 中存在 ``release_blocker_rollup`` 节点时，bundle 面向 reviewer 的
四个交接文件（``START_HERE.md``、``ARTIFACT_GUIDE.md``、``REVIEWER_CHECKLIST.md`` 与
``release_handoff.md``）还会渲染统一的治理 rollup 明细。该明细不会替代顶层
``release_blockers`` 的强校验，而是用于让发布面板和人工复核直接读取 final rollup
中的机器字段：

- ``release_blocker_rollup.release_blockers[]``：展示 ``id``、``action``、``message``、
  ``source_schema``、``source_report_display`` 与 ``source_json_display``。
- ``release_blocker_rollup.warnings[]``：展示 ``id``、``action``、``message``、
  ``source_schema``、``source_report_display`` 与 ``source_json_display``。
- ``release_blocker_rollup.action_items[]``：展示 ``id``、``action``、``open_command``、
  ``source_schema``、``source_report_display`` 与 ``source_json_display``。

对应的人读摘要文件为 ``release_blocker_rollup.md``，用于把同一组机器字段交给
reviewer 复核。
同一轮 release governance 还会保留 ``release_governance_handoff.md`` 与
``release_governance_pipeline.md``，确保 handoff 和 pipeline 两个视角都能追溯到
相同的 blocker / warning / action item 来源。
历史样式合并建议计数字段 ``style_merge_suggestion_count`` 仍保留为机器字段，
并在文档中写作 ``style_merge_suggestion_count``，避免旧 release 面板丢失该计数。

``run_release_candidate_checks.ps1`` 在读取 ``featherdoc.release_blocker_rollup_report.v1``
后会把上述数组同步写入 ``summary.json`` 的 ``release_blocker_rollup`` 与
``steps.release_blocker_rollup``，并在 ``final_review.md`` 里展开同一组字段。这样
``featherdoc.document_skeleton_governance_rollup_report.v1``、onboarding governance、
schema confidence calibration 和 content-control data-binding governance 的治理项可以
被发布面板直接消费，而不只停留在 blocker / warning / action item 计数里。机器摘要里的
``warning_count`` 仍然只作为聚合计数使用，reviewer 必须继续读取下面的明细数组。
其中 content-control data-binding governance 的 blocker、warning 与 action item 会固定携带
``featherdoc.content_control_data_binding_governance_report.v1`` 作为 ``source_schema``，
把 ``inspect-content-controls`` 或治理 summary 同步写入 ``source_report_display`` 与
``source_json_display``，并为 action item 提供重建治理报告的 ``open_command``。
reviewer 可先沿着 ``source_report_display`` 打开源报告，再用 ``source_json_display`` 回到证据 JSON。
占位符阻断项会继续保留 ``content_control_data_binding.bound_placeholder``，修复策略固定为
``sync_bound_content_control``，避免 content-control 修复流程只在聊天记录或人工备注中存在。
这些 release-facing 明细还必须保留 ``input_docx``、``template_name``、
``schema_target`` 与 ``target_mode`` provenance；否则 reviewer 只能看到修复命令，
无法确认该 blocker 属于哪个模板、哪个输入 DOCX 和哪个 schema 目标。

project-template onboarding governance 会先写出
``featherdoc.project_template_onboarding_governance_report.v1``，再由
``featherdoc.project_template_delivery_readiness_report.v1`` 汇总到默认发布阻断输入。
delivery readiness 在透传 onboarding-derived blocker / action item 时保留原始
``source_schema``、``source_report_display`` 与 ``source_json_display``，同时把 reviewer 可执行的
``open_command`` 带入 final rollup。reviewer 在 ``START_HERE.md``、
``ARTIFACT_GUIDE.md``、``REVIEWER_CHECKLIST.md`` 或 ``release_handoff.md`` 中看到
``project_template_onboarding.*`` 阻断项时，应先打开 ``source_report_display`` 查看
governance/report 摘要，再用 ``source_json_display`` 回到 onboarding governance
原始证据，最后按 ``open_command`` 同步或复核 schema approval 记录。

schema confidence calibration 会写出
``featherdoc.schema_patch_confidence_calibration_report.v1``。当 calibration 里仍有
pending approval outcome 时，报告会把
``schema_patch_confidence_calibration.pending_schema_approvals`` 写入
``release_blockers``；未打分 candidate 或无效审批记录会进入 ``warnings``；所有
recommendation 会同步成 ``action_items`` 并携带重建校准报告的 ``open_command``。
默认 auto-discovery、release governance pipeline 和 handoff 都会读取
``schema-patch-confidence-calibration/summary.json``，因此 reviewer 可以在发布面板
中直接看到校准 blocker / warning / action item 的 ``source_schema`` 与证据 JSON。

编号真实语料治理会写出 ``featherdoc.numbering_catalog_governance_report.v1``。
当 catalog 与 baseline 的真实文档键不能对齐时，报告会生成稳定阻断项
``numbering_catalog_governance.real_corpus_alignment_gap``；release metadata 文档检查会
要求这一路径继续出现在发布说明材料中，防止样式/编号治理退回到只看数量的弱检查。

表格与版式交付治理会写出
``featherdoc.table_layout_delivery_governance_report.v1``。它的 ``delivery_quality``
明细会携带表格样式、自动修复、人工复核、浮动表格定位和命令失败计数；release
summary、handoff 与 reviewer bundle 应保留这些字段，让 reviewer 能区分“可发布”、
“需自动修复”和“需人工版式复核”。

此外，``featherdoc.release_governance_pipeline_report.v1`` 的 ``stages[]`` 现在也会
保留每个治理 stage 的 ``release_blockers``、``warnings`` 与 ``action_items`` 明细。
这些 stage-level 明细会补齐 ``stage_id``、``stage_title``、``source_schema``、
``source_report_display`` 与 ``source_json_display``；action item 还会保留
``open_command``。发布面板可以先按 stage 过滤 ``numbering_catalog_governance``、
``content_control_data_binding_governance``、``project_template_delivery_readiness`` 或
``schema_patch_confidence_calibration``，再把同一条治理项交给 final rollup、release
summary、bundle 和 reviewer checklist 继续展示。reviewer 若在 pipeline summary 中看到
阻断项，应优先打开 ``source_json_display`` 对应证据，再按 ``open_command`` 重建或复核
该治理报告，最后重新运行 release blocker rollup / bundle 生成链路。

当 ``summary.json`` 中存在 ``release_governance_handoff`` 节点时，release summary、
``final_review.md`` 和 reviewer-facing bundle 也会展示 handoff 归一化后的三类明细：

- ``release_governance_handoff.release_blockers[]``：展示 ``id``、``action``、
  ``message``、``source_schema``、``source_report_display`` 与
  ``source_json_display``。
- ``release_governance_handoff.warnings[]``：展示 ``id``、``action``、``message``、
  ``source_schema``、``source_report_display`` 与 ``source_json_display``。
- ``release_governance_handoff.action_items[]``：展示 ``id``、``action``、
  ``open_command``、``source_schema``、``source_report_display`` 与
  ``source_json_display``。

其中 content-control handoff 条目还必须展示 ``input_docx``、``template_name``、
``schema_target`` 与 ``target_mode``，与 release blocker rollup、START_HERE、
ARTIFACT_GUIDE 和 REVIEWER_CHECKLIST 中的同一组 provenance 保持一致。

``build_release_governance_handoff_report.ps1`` 会从已加载的治理报告中归一化 blocker、
warning 与 action item；``run_release_candidate_checks.ps1`` 会把这些数组同步到
``summary.json`` 的 ``release_governance_handoff`` 与
``steps.release_governance_handoff``，并在 ``final_review.md`` 中写出
``Release governance handoff details``。因此 reviewer 不需要只依赖 handoff 计数，
而是可以从 ``release_handoff.md``、``ARTIFACT_GUIDE.md``、``REVIEWER_CHECKLIST.md``
或 ``START_HERE.md`` 直接打开 ``source_json_display``，再用 action item 的
``open_command`` 重建相应 governance report。

同时，release candidate summary 的 ``manifest_signoff_entrypoints`` 必须进入 release
blocker rollup 的 source report contract evidence，并由 release governance handoff
汇总为 ``manifest_signoff_entrypoints_source_reports``。``run_release_candidate_checks.ps1``
需要把该 evidence 同步到 ``release_governance_handoff`` 与
``steps.release_governance_handoff``，让 ``final_review.md``、``START_HERE.md``、
``ARTIFACT_GUIDE.md`` 和 ``REVIEWER_CHECKLIST.md`` 都能展示 ``status``、
``release_assets_manifest``、三个必需入口、必需 governance contracts、必需字段和
``reviewer_manifest_scoped_project_template_trace``，避免 packaged manifest signoff
只停留在打包产物层。

固定 project-template 发布准入清单也要有同级机器证据：
``run_release_candidate_checks.ps1`` 必须写出
``project_template_readiness_checklist_entrypoints``，声明 ``START_HERE.md``、
``ARTIFACT_GUIDE.md`` 与 ``REVIEWER_CHECKLIST.md`` 三个入口都指向
``docs/project_template_release_readiness_checklist_zh.rst``，并携带
``release_entry_project_template_readiness_checklist_trace``。
``package_release_assets.ps1`` 必须把该字段保留到 ``release_assets_manifest.json``，
再由 ``assert_release_material_safety.ps1`` 审计。这样固定清单入口不是只靠人工文案
或 grep 推断，而是成为 packaged asset 层可检查的发布证据。
同一字段还必须继续进入 ``build_release_blocker_rollup_report.ps1`` 的 source report
contract evidence，再由 ``build_release_governance_handoff_report.ps1`` 汇总为
``project_template_readiness_checklist_entrypoints_source_reports``，并同步到
``steps.release_governance_handoff``。这样 ``final_review.md``、``START_HERE.md``、
``ARTIFACT_GUIDE.md`` 和 ``REVIEWER_CHECKLIST.md`` 可以直接展示 ``status``、
``checklist_label``、``checklist_path``、三个入口和固定 marker，而不需要 reviewer
从 packaged manifest 反向推断。固定标记：
``project_template_readiness_checklist_entrypoints_governance_trace``。
``assert_release_material_safety.ps1`` 还必须审计
``release_governance_handoff.md`` 中同一个 ``source_report`` 列表块是否同时携带
``checklist_path``、三个必需入口和
``release_entry_project_template_readiness_checklist_trace``，避免 detached notes
补齐发布证据。固定标记：
``project_template_readiness_checklist_entrypoints_material_safety_trace``。
``write_release_metadata_start_here.ps1``、``write_release_artifact_guide.ps1`` 和
``write_release_reviewer_checklist.ps1`` 还会把同一证据压缩成
``Project-template readiness checklist handoff evidence`` 行，并写入
``START_HERE.md``、``ARTIFACT_GUIDE.md`` 与 ``REVIEWER_CHECKLIST.md``，避免 reviewer
必须进入内部 handoff 章节才能看到 checklist source-report 证据。固定标记：
``project_template_readiness_checklist_entrypoints_release_entry_trace``。
``assert_release_material_safety.ps1`` 会继续审计该 compact evidence 行，要求
``project_template_readiness_checklist_entrypoints_source_reports``、``status``、
``checklist_path``、三个入口、固定 marker 与 ``source_report`` 保持在同一行，避免
detached notes 补齐入口材料。固定标记：
``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``。
``package_release_assets.ps1`` 在 staged release materials 审计前还会直接检查
``START_HERE.md``、``ARTIFACT_GUIDE.md`` 与 ``REVIEWER_CHECKLIST.md`` 中的同一条
compact evidence 行，并把通过后的
``release_entry_project_template_readiness_checklist_material_safety_audit`` 写入
``release_assets_manifest.json``；``assert_release_material_safety.ps1`` 会继续审计该
manifest 字段，避免打包阶段只靠日志推断。固定标记：
``project_template_readiness_checklist_entrypoints_packaged_material_safety_trace``。

对 project-template governance，``final_review.md`` 和
``steps.release_governance_handoff`` 还必须同时保留
``source_report_display`` 与 ``source_json_display``：前者指向
``project-template-delivery-readiness`` 汇总，后者可回到
``project-template-onboarding-governance`` 原始证据。这样发布 reviewer 可以从
最终 release candidate 摘要直接追溯到模板准入与 schema approval 证据，而不是在
handoff 计数和 release blocker rollup 之间手工拼路径。
``release_governance_handoff.md`` 还必须遵守
``block_scoped_governance_handoff_trace``：``project_template_delivery_readiness`` 与
``project_template_onboarding.schema_approval`` 各自的 Markdown list block 必须分别保留
``schema_approval_status_summary``、``source_report_display`` 和
``source_json_display``，其中 readiness report-status block 还必须保留 ``status`` 与
``ready``。固定标记：``block_scoped_governance_handoff_project_template_status_trace``。
不能让 readiness block 的 source display 或 schema approval 状态代替 onboarding
governance block。

内部 handoff 文件可以展示更完整的 reviewer metadata，例如：

- ``review_status``
- ``reviewed_at``
- ``review_method``
- ``review_note``
- ``review_task_summary``

公开 release 正文应保持简洁，不应泄露 operator-only 的自由文本 note 或内部
provenance。当前 ``release_body.zh-CN.md`` 和 ``release_summary.zh-CN.md`` 只保留
面向发布说明有意义的 verdict / status 摘要。

``write_release_note_bundle.ps1`` 默认会运行 ``assert_release_material_safety.ps1``，
用于防止内部路径、操作员备注或不适合公开发布的内容进入 release material。
只有聚焦内容渲染的回归测试才应使用 ``-SkipMaterialSafetyAudit``，正式发布和
打包流程不应跳过该审计。


字段优先级与降级规则
--------------------

release note bundle 在读取视觉评审 metadata 时遵循下面的原则：

1. 优先读取 release summary 中 ``steps.visual_gate`` 的同轮字段。
2. release summary 缺字段或字段不完整时，回退读取 gate summary。
3. ``review_task_summary`` 必须完整才渲染；不完整时视为缺失。
4. curated visual regression 读取 ``review_verdict`` 作为唯一 verdict 字段。
5. ``reviewed_at`` 需要保持文化无关的 ``yyyy-MM-ddTHH:mm:ss`` 输出格式。

这些规则集中在 ``release_visual_metadata_helpers.ps1``，避免 handoff、artifact
guide、reviewer checklist、start-here 和 release note body 各自实现一套不同逻辑。


推荐命令
--------

完整 release preflight：

.. code-block:: powershell

    pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1

只同步最新视觉 verdict：

.. code-block:: powershell

    pwsh -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1

显式同步某次 gate 和 release candidate：

.. code-block:: powershell

    pwsh -ExecutionPolicy Bypass -File .\scripts\sync_visual_review_verdict.ps1 `
        -GateSummaryJson .\output\word-visual-release-gate\report\gate_summary.json `
        -ReleaseCandidateSummaryJson .\output\release-candidate-checks\report\summary.json `
        -RefreshReleaseBundle

只重写 release note bundle：

.. code-block:: powershell

    pwsh -ExecutionPolicy Bypass -File .\scripts\write_release_note_bundle.ps1 `
        -SummaryJson .\output\release-candidate-checks\report\summary.json


维护检查清单
------------

修改 release metadata 管线时，建议按下面顺序验证：

1. 先跑最靠近改动的 PowerShell 回归测试。
2. 再跑对应的 CTest 名称，确认 CMake 注册入口也能通过。
3. 如果改动影响 bundle 输出，至少覆盖：

   - ``release_note_bundle_version``
   - ``release_note_bundle_visual_verdict_metadata``
   - ``release_visual_verdict_metadata_consistency``

4. 如果改动影响 verdict 同步，至少覆盖：

   - ``sync_visual_review_verdict_section_page_setup``
   - ``sync_visual_review_verdict_page_number_fields``
   - ``sync_visual_review_verdict_curated_visual_bundle``

5. 如果改动影响 gate task 计数，至少覆盖：

   - ``word_visual_release_gate_smoke_verdict``
   - ``release_candidate_visual_verdict``

6. 最后进行固定 contact sheet 可视化复核，确认视觉证据入口仍可打开。


常见反模式
----------

- 不要在 release summary 中保留不完整的 ``review_task_summary``。
- 不要让公开 release body 输出 ``review_note``、``reviewed_at`` 或
  ``review_method``。
- 不要为了测试速度在正式发布路径跳过 material safety audit。
- 不要让多个脚本各自拼接 review task count 文本，优先复用共享 helper。
- 不要把空字符串、空数组占位任务计入 review scope。


当前收敛状态
------------

截至当前管线，release metadata 已经具备下面几项保护：

- gate 生成端忽略空 review task 占位。
- preflight 只复制完整 review task count。
- verdict sync 会移除不完整 review task count。
- bundle helper 会在 release summary 不完整时回退到 gate summary。
- 聚焦 bundle 内容的测试可以跳过 material safety audit，降低 Windows CTest
  偶发超时风险；正式发布默认仍执行审计。

这意味着后续再扩展新的视觉 flow 或 curated bundle 时，优先接入现有字段契约，
而不是新增一套平行 metadata 格式。
