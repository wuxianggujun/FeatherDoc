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

``run_release_candidate_checks.ps1`` 在读取 ``featherdoc.release_blocker_rollup_report.v1``
后会把上述数组同步写入 ``summary.json`` 的 ``release_blocker_rollup`` 与
``steps.release_blocker_rollup``，并在 ``final_review.md`` 里展开同一组字段。这样
``featherdoc.document_skeleton_governance_rollup_report.v1``、onboarding governance、
schema confidence calibration 和 content-control data-binding governance 的治理项可以
被发布面板直接消费，而不只停留在 blocker / warning / action item 计数里。
其中 content-control data-binding governance 的 blocker、warning 与 action item 会固定携带
``featherdoc.content_control_data_binding_governance_report.v1`` 作为 ``source_schema``，
把 ``inspect-content-controls`` 或治理 summary 写入 ``source_json_display``，并为 action item
提供重建治理报告的 ``open_command``，reviewer 可直接沿着 bundle 中的 rollup 明细回到证据 JSON。

project-template onboarding governance 会先写出
``featherdoc.project_template_onboarding_governance_report.v1``，再由
``featherdoc.project_template_delivery_readiness_report.v1`` 汇总到默认发布阻断输入。
delivery readiness 在透传 onboarding-derived blocker / action item 时保留原始
``source_schema`` 与 ``source_json_display``，同时把 reviewer 可执行的
``open_command`` 带入 final rollup。reviewer 在 ``START_HERE.md``、
``ARTIFACT_GUIDE.md``、``REVIEWER_CHECKLIST.md`` 或 ``release_handoff.md`` 中看到
``project_template_onboarding.*`` 阻断项时，应先打开 ``source_json_display`` 对应的
onboarding governance 证据，再按 ``open_command`` 同步或复核 schema approval 记录。

schema confidence calibration 会写出
``featherdoc.schema_patch_confidence_calibration_report.v1``。当 calibration 里仍有
pending approval outcome 时，报告会把
``schema_patch_confidence_calibration.pending_schema_approvals`` 写入
``release_blockers``；未打分 candidate 或无效审批记录会进入 ``warnings``；所有
recommendation 会同步成 ``action_items`` 并携带重建校准报告的 ``open_command``。
默认 auto-discovery、release governance pipeline 和 handoff 都会读取
``schema-patch-confidence-calibration/summary.json``，因此 reviewer 可以在发布面板
中直接看到校准 blocker / warning / action item 的 ``source_schema`` 与证据 JSON。

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
