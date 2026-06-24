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
      ├─ report/ARTIFACT_GUIDE.md
      ├─ report/REVIEWER_CHECKLIST.md
      ├─ report/release_handoff.md
      ├─ report/release_body.zh-CN.md
      └─ report/release_summary.zh-CN.md

如果只想把最新 review task 的结果回灌进去，通常优先使用
``sync_latest_visual_review_verdict.ps1``。它会先发现最新 task pointer，推断
对应 gate summary 和 release candidate summary，再委托
``sync_visual_review_verdict.ps1`` 执行真正的同步。


CI artifact publish 边界
------------------------

Release metadata 管线需要区分正式公开发布和 CI artifact publish 两类消费场景。
``Release Publish`` 默认必须消费完整本地 Word visual gate 通过后的 release
candidate summary：``visual_verdict`` 应为 ``pass``，``steps.visual_gate.status``
不能是 ``skipped`` / ``visual_gate_skipped``。只有这类 summary 才能代表
Word 截图级 visual gate pass。

当 release summary 来自 CI artifact 流水线，且 ``execution_status=pass`` 但
``visual_gate`` 被跳过时，发布脚本必须要求操作者显式选择 CI artifact publish
边界：workflow 输入为 ``allow-ci-artifact-publish``，PowerShell 脚本开关为
``-AllowCiArtifactPublish``。该模式只把 CI build/test/install smoke 与 release
metadata 审计视为构建产物依据；它不等同于完整本地 Word visual gate，也不能把
``visual_gate=skipped``、``visual_gate_skipped`` 或 ``pending_manual_review``
解释成截图级 visual pass。下游 release notes、handoff 和 manifest 仍应保留
这个视觉证据边界，避免 reviewer 或发布面板误判。


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

进入完整 Word 视觉 gate 前，可以先运行
``check_word_visual_release_gate_preflight.ps1`` 做只读静态前置检查。该脚本输出
``featherdoc.word_visual_release_gate_preflight.v1``，证据边界固定为
``word_visual_release_gate_preflight_static_contract_only``；其中 ``preflight_ready``
只表示 gate 脚本、helper、CMake 轻量测试注册和文档入口仍然一致，``release_ready``
必须保持为 ``false``。该 preflight 不启动 Word、CMake、CTest、浏览器、
LibreOffice 或任何视觉渲染流程，不能替代截图级 ``gate_summary.json`` 和人工
review verdict。summary 还会输出 ``evidence_scope_note``、``boundary``、
``minimum_risk_next_action_command``、
``strict_preflight_command_template``、``full_gate_command_template`` 和
``output_encoding`` = ``UTF-8 without BOM``，让自动化或 reviewer 在 ready 时
直接进入完整 gate，在 not_ready 时先用 strict preflight 复查静态阻断项，并能
稳定消费无 BOM 的 summary / Markdown 产物。
``evidence_scope_note`` 必须保留
``static scripts, docs, and test registration only``，避免后续维护者把静态
preflight 输出误读为截图级 release evidence。

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
- ``release_note_bundle``：结构化列出六个 release note bundle 产物的
  ``id``、``path``、``path_display``、``location`` 与 ``required`` 标记，
  其中 ``START_HERE.md`` 位于 summary 输出根目录，其余文件位于 ``report``。
- ``project_template_workflow_dashboard_report``：当 ``ReleaseEvidenceScope`` 不是
  ``pdf-only`` 时，preflight 会在 ``report/project-template-workflow-dashboard``
  下生成 ``project_template_workflow_dashboard.json`` 与
  ``project_template_workflow_dashboard.md``，并把 ``status``、``release_ready``、
  ``release_blocker_count``、``warning_count``、``source_report_count``、
  ``next_action``、``next_action_summary`` 与 ``next_action_group_count`` 写回
  ``summary.json``、``steps.project_template_workflow_dashboard`` 和
  ``final_review.md``。发布入口材料也会同步展示
  ``Project template workflow dashboard status``、
  ``Project template workflow dashboard release ready``、
  ``Project template workflow dashboard counts`` 与
  ``Project template workflow dashboard business document types``、
  ``Project template workflow dashboard corpus roles``、
  ``Project template workflow dashboard next action``，并在存在分组时展示
  ``Project template workflow dashboard next action groups`` 和
  ``Project template workflow dashboard action group``；其中
  ``REVIEWER_CHECKLIST.md`` 会在 dashboard 未 release-ready 或 blocker 非零时保留
  明确的 stop condition，``START_HERE.md`` 和 ``ARTIFACT_GUIDE.md`` 会给出
  dashboard JSON / Markdown 证据路径，避免 reviewer 只看到 handoff 计数。

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

项目模板 workflow dashboard 会优先消费 release summary 顶层
``project_template_onboarding_governance`` 与 ``project_template_delivery_readiness``；
当这些字段不存在时，会从 ``release_governance_handoff`` 或
``steps.release_governance_handoff`` 下的
``project_template_onboarding_governance_contract`` 与
``project_template_delivery_readiness_contract`` 读取 ``source_json``、
``source_json_display``、``source_report`` 或 ``source_report_display``。这样
release candidate 可以直接从 handoff contract 串起 onboarding governance 与
delivery readiness 证据，发布面板无需再猜测项目模板工作流的源报告路径。

如果 ``review_task_summary`` 不完整，preflight 不应把它写入 release summary，
也不应在 ``final_review.md`` 中渲染空计数行。

``package_release_assets.ps1`` 写出 ``release_assets_manifest.json`` 时，还会把标准
Word 视觉审阅任务的 ``word_visual_standard_review_metadata`` 带入 packaged
manifest。该字段只覆盖 ``smoke``、``fixed_grid``、``section_page_setup`` 和
``page_number_fields`` 四个标准任务，保留 ``review_result_path`` 与
``final_review_path`` 等可追溯证据；``review_note`` 仍属于 operator-only 自由文本，
不得进入 packaged manifest 或公开 release body。
同一份 packaged manifest 中的
``project_template_delivery_readiness_contract`` 与
``project_template_onboarding_governance_contract`` 也必须保留
``business_document_type_summary`` 和 ``corpus_role_summary``，且
``manifest_signoff_entrypoints.required_fields`` 会把这两项列入人工签核必检字段。
``assert_release_material_safety.ps1`` 会直接审计这两条 contract，防止打包层只留下
readiness / onboarding 状态而丢失业务文档类型和语料角色维度。

后续 ``build_release_blocker_rollup_report.ps1`` 会继续把同一组
``word_visual_standard_review_metadata`` 保留在 release candidate source report
contract evidence 中，并额外给出 ``word_visual_standard_review_task_keys``、
``word_visual_standard_review_status_summary`` 与
``word_visual_standard_review_verdict_summary``。``build_release_governance_handoff_report.ps1``
再把这些字段汇总为 ``word_visual_standard_review_metadata_source_reports``，让
治理交接材料能直接展示四个标准 Word 视觉审阅任务的 task key、review task key、
verdict、review status、review method、review result path 和 final review path。
这条下游证据链仍禁止公开 ``review_note``，避免 operator-only 备注从 packaged
manifest 泄漏到 rollup 或 handoff。


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
必要时可以显式传入 ``-GateSummaryJson``，但该路径必须和 latest task pointer
推断出的 ``gate_summary.json`` 一致，避免把不同 gate 的 review task 混在一起。
如果无法发现匹配的 release candidate summary，它可以只更新 gate 层。

当 gate summary 里已有 ``readme_gallery.status=completed`` 时，verdict sync 会把
``readme_gallery`` 和 ``assets_dir`` 原样同步到 release summary，并在 release
``final_review.md`` 里继续渲染 README gallery refresh 状态；如果只想从已有 task
证据刷新仓库预览图，使用 ``refresh_readme_visual_assets.ps1``。


第四层：release note bundle
---------------------------

``write_release_note_bundle.ps1`` 负责把 release candidate summary 变成一组可交付
Markdown 文件。

默认会生成或刷新：

- ``START_HERE.md``
- ``report/ARTIFACT_GUIDE.md``
- ``report/REVIEWER_CHECKLIST.md``
- ``report/release_handoff.md``
- ``report/release_body.zh-CN.md``
- ``report/release_summary.zh-CN.md``

``run_release_candidate_checks.ps1`` 会在 ``summary.json`` 顶层同步写出
``release_note_bundle``，把上述六个文件作为机器可消费的路径契约暴露给 CI、
发布面板和后续同步脚本。``entrypoint_count`` 与 ``required_entrypoint_count``
固定为 6，``entrypoints[]`` 中每项都保留 ``path_display`` 和 ``location``，
避免下游再从散落的平铺字段推断 ``START_HERE.md`` 与 ``report`` 目录边界。
``package_release_assets.ps1`` 会把同一结构带入 ``release_assets_manifest.json``，
``assert_release_material_safety.ps1`` 则在 manifest 审计阶段校验六个入口的
``required``、``location`` 与 ``path_display``，防止发布包丢失 bundle 路径契约。

当 ``summary.json`` 顶层存在 ``release_blockers`` 时，bundle 会在
``START_HERE.md``、``report/ARTIFACT_GUIDE.md``、``report/REVIEWER_CHECKLIST.md``、
``report/release_handoff.md`` 与 ``report/release_body.zh-CN.md`` 中渲染 blocker count、
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
如果 controlled PDF visual smoke 证据存在但未通过，治理报告会发出
``rerun_pdf_controlled_visual_smoke_check`` warning runbook，要求先复核
``controlled_visual_smoke_json_display``，再用
``check_pdf_controlled_visual_smoke.ps1`` 只读重跑已有 PNG/text 证据，随后重新生成
PDF preflight governance report、release blocker rollup 与 release note bundle。
这些 runbook 还会直接展示 ``output gap checks`` 和 ``missing outputs``，与
preflight summary 的顶层 ``output_gap_count`` / ``missing_output_count`` 保持一致，
避免 reviewer 需要手动合计 CLI baseline、visual baseline 和 CJK text-layer 缺口。
PDF release readiness 的机器收口由 ``check_pdf_release_readiness.ps1`` 只读消费
已有 preflight、visual gate、segmented gate 和 CTest 证据；summary schema 固定为
``featherdoc.pdf_release_readiness_check.v1``，并必须保留
``pdf_release_readiness_machine_gate_trace``、``preflight_summary_json_display``、
``visual_gate_summary_json_display``、``visual_full_gate_guarded_summary_json_display``、
``visual_segmented_gate_summary_json_display``、``full_ctest_summary_json_display``、
``full_ctest_remaining_summary_json_display`` 与
``pdf_visual_gate_release_owner_acceptance_trace``，避免 release metadata 只展示
pass / warning 结论而丢失 reviewer 可打开的 PDF readiness 证据路径。
DOCX 功能 smoke 的 ``restore_docx_functional_smoke_evidence`` blocker action 也
映射到固定 runbook：reviewer 先打开 ``source_report_display`` /
``source_json_display`` 指向的 ``docx-functional-smoke-readiness`` 证据，恢复缺失的
DOCX package、功能样例或 Word visual smoke PNG 证据，再只读运行
``check_docx_functional_smoke_readiness.ps1``，最后重建 release governance pipeline /
handoff 与 release note bundle。该 runbook 必须继续声明它不是新鲜 Word COM 渲染。
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
- ``release_blocker_rollup.informational_action_items[]``：展示非阻断的 release checklist
  动作、``open_command``、``source_schema``、``source_report_display`` 与
  ``source_json_display``。

对应的人读摘要文件为 ``release_blocker_rollup.md``，用于把同一组机器字段交给
reviewer 复核。
rollup summary 与 Markdown 顶部还会输出 ``blocker_source_schema_summary``、
``action_item_source_schema_summary``、``informational_action_item_source_schema_summary`` 与
``warning_source_schema_summary``。发布面板和 reviewer 可以先按 ``source_schema``
把 blocker、warning、action item 与 informational action item 分流，再打开对应
``source_report_display`` / ``source_json_display`` 定位证据。
当 source report 是项目模板治理报告时，rollup 的 ``source_reports[]`` 还必须保留
``business_document_type_summary`` 与 ``corpus_role_summary``，避免发布面板只能从
入口 Markdown 中反推业务文档类型和语料角色。
同一轮 release governance 还会保留 ``release_governance_handoff.md`` 与
``release_governance_pipeline.md``，确保 handoff 和 pipeline 两个视角都能追溯到
相同的 blocker / warning / action item 来源。
历史样式合并建议计数字段 ``style_merge_suggestion_count`` 仍保留为机器字段，
并在文档中写作 ``style_merge_suggestion_count``，避免旧 release 面板丢失该计数。
样式合并恢复审计会写出 ``featherdoc.style_merge_restore_audit.v1``。当
``audit_style_merge_restore_plan.ps1`` 发现 restore dry-run issue 时，
``release_blockers`` 会保留 ``review_handoff_steps``、``next_handoff_step``、
``next_copy_command``、``next_step_reason``、``issue_review_commands``、
``issue_review_command_count``、``issue_review_group_summary``、
``first_issue_review_command``、``copy_issue_review_command``、
``handoff_status_summary`` 与 ``rollback_plan_summary``，同时携带 ``source_schema``、``source_report_display``、
``source_json_display``、``rollback_plan_display`` 和 ``repair_strategy``。
当审计 clean 时，同一组字段会进入 ``action_items``，让 reviewer 能先按
``handoff_status_summary.next_step_id`` 判断下一步，再直接复制
``next_copy_command``；有 issue group 时，也可以从顶层
``issue_review_group_summary`` 判断第一组 issue code / source-target 样式对，
再用 ``copy_issue_review_command`` 直接拿到第一条样式对复核命令。``rollback_plan_summary`` 只描述 rollback JSON 本身，
包括 ``merge_rollback_entry_count``、``restorable_merge_rollback_entry_count``、
``non_restorable_merge_rollback_entry_count`` 与
``non_restorable_merge_rollback_entry_indexes``；``restorable_rollback_command_summary``
只描述 reviewer 可复制的单项 / 批量恢复命令是否可用。它们都不等同于 dry-run 实际恢复成功数。

``run_release_candidate_checks.ps1`` 在读取 ``featherdoc.release_blocker_rollup_report.v1``
后会把上述数组同步写入 ``summary.json`` 的 ``release_blocker_rollup`` 与
``steps.release_blocker_rollup``，并在 ``final_review.md`` 里展开同一组字段。这样
``featherdoc.document_skeleton_governance_rollup_report.v1``、onboarding governance、
schema confidence calibration 和 content-control data-binding governance 的治理项可以
被发布面板直接消费，而不只停留在 blocker / warning / action item 计数里。机器摘要里的
``warning_count`` 仍然只作为聚合计数使用，reviewer 必须继续读取下面的明细数组。
其中 document skeleton governance rollup 的重复样式建议会以稳定 warning
``document_skeleton.style_merge_suggestions_pending`` 暴露，并保留
``style_merge_suggestion_count``、``source_schema``、``source_report_display``、
``source_json_display`` 与 ``open_command``，让 reviewer 能先打开 rollup summary，
再回到单文档 skeleton source 或更窄的 style merge evidence。
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
其中 ``fix_custom_xml_data_binding_source``、``sync_or_fill_bound_content_control``、
``review_content_control_lock_strategy``、``review_unbound_form_content_control``、
``review_duplicate_content_control_binding``、``collect_content_control_data_binding_evidence``、
``review_content_control_data_binding_evidence`` 与
``run_content_control_custom_xml_sync`` 都映射到固定 reviewer runbook：先打开
``source_report_display`` 与 ``source_json_display``，确认 ``input_docx``、
``template_name``、``schema_target``、``target_mode`` 和具体 content-control target，
再按 ``command_template`` / ``open_command`` 收集 inspection、Custom XML sync 或修复
证据，最后重跑 ``build_content_control_data_binding_governance_report.ps1`` 并刷新
release governance / release note bundle。

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
其中 ``review_schema_update_candidate``、``approve_project_template_schema``、
``promote_schema_update_candidate``、``update_template_or_schema_before_retry``、
``freeze_schema_baseline``、``review_schema_baseline``、
``review_schema_approval_history``、``run_project_template_smoke_for_registered_manifest``、
``run_project_template_smoke_then_review_schema_patch_approval``、
``review_project_template_smoke_failure``、
``collect_project_template_onboarding_governance_evidence`` 与
``review_project_template_delivery_readiness_evidence`` 都映射到固定 reviewer runbook：
先确认 project-template provenance 与 schema approval 状态，再按
``run_project_template_smoke.ps1``、``sync_project_template_schema_approval.ps1``、
``write_project_template_schema_approval_history.ps1``、
``build_project_template_onboarding_governance_report.ps1`` 或
``build_project_template_delivery_readiness_report.ps1`` 重建对应证据，最后刷新
release governance / release note bundle。

schema confidence calibration 会写出
``featherdoc.schema_patch_confidence_calibration_report.v1``。当 calibration 里仍有
pending approval outcome 时，报告会把
``schema_patch_confidence_calibration.pending_schema_approvals`` 写入
``release_blockers``；未打分 candidate 会以
``schema_patch_confidence_calibration.unscored_candidates`` 进入 ``warnings``；
无效审批记录会以 ``schema_patch_confidence_calibration.invalid_approval_records``
进入 ``release_blockers``。所有
recommendation 会同步成 ``action_items`` 并携带重建校准报告的 ``open_command``。
报告还会写出 ``business_template_corpus_summary``，按 project/template/source JSON
汇总真实业务模板语料覆盖，并暴露 ``missing_source_metadata_count``、
``missing_project_id_count``、``missing_template_name_count`` 与
``missing_summary_json_count``，同时暴露
``missing_business_document_type_count``。如果候选缺少项目、模板或来源 summary，报告会生成
``schema_patch_confidence_calibration.missing_business_template_source_metadata`` warning，
并把 ``add_business_template_source_metadata`` 同步为 action item；如果候选缺少
``business_document_type``，报告会生成
``schema_patch_confidence_calibration.missing_business_document_type_metadata`` warning，
并把 ``add_business_template_document_type_metadata`` 同步为 action item；这类问题不直接阻断
发布，但 reviewer 不能把缺来源或缺业务文档类型的语料用于自动阈值收紧。
默认 auto-discovery、release governance pipeline 和 handoff 都会读取
``schema-patch-confidence-calibration/summary.json``，因此 reviewer 可以在发布面板
中直接看到校准 blocker / warning / action item 的 ``source_schema`` 与证据 JSON。
其中 ``resolve_pending_schema_approvals``、``fix_invalid_approval_records``、
``add_explicit_confidence_metadata``、``add_business_template_source_metadata``、
``add_business_template_document_type_metadata`` 和
``review_schema_patch_confidence_calibration_evidence`` 都映射到固定 reviewer
runbook：先打开 ``source_report_display`` 与 ``source_json_display``，修复审批或置信度
证据，再重跑 ``write_schema_patch_confidence_calibration_report.ps1`` 并刷新 release
governance / release note bundle。

编号真实语料治理会写出 ``featherdoc.numbering_catalog_governance_report.v1``，
默认产物为 ``output/numbering-catalog-governance/summary.json``。当 catalog
与 baseline 的真实文档键不能对齐时，报告会生成稳定阻断项
``numbering_catalog_governance.real_corpus_alignment_gap``；同时
per-document ``real_corpus_alignment`` 会把 ``missing_baseline`` 与
``missing_exemplar`` 拆成 ``numbering_catalog_governance.missing_baseline`` /
``numbering_catalog_governance.missing_exemplar`` action item，并保留原始
``source_schema``、``source_report_display``、``source_json_display`` 和
``open_command``，让 reviewer 能直接回到 document skeleton rollup 或 manifest
summary 重建证据。
``numbering_catalog_governance.real_corpus_confidence`` / ``real_corpus_confidence``
会作为治理指标进入 rollup、handoff、bundle 与
``numbering_catalog_real_corpus_confidence`` 打包镜像字段。该指标必须继续携带
score / level、``catalog_coverage_percent``、``baseline_coverage_percent``、
``matched_document_count`` 与 ``penalty_summary``，release metadata 文档检查会
要求这些路径继续出现在发布说明材料中，防止样式/编号治理退回到只看数量的弱检查。
其中 ``review_numbering_catalog_real_corpus_alignment``、
``fix_numbering_catalog_baseline_lint``、
``refresh_numbering_catalog_baseline_or_repair_docx``、
``review_numbering_catalog_check_issues``、
``rebuild_document_skeleton_governance_rollup``、
``rebuild_numbering_catalog_manifest_summary``、
``review_numbering_catalog_governance_sources``、
``review_style_numbering_audit``、``preview_style_numbering_repair``、
``promote_numbering_catalog_exemplar``、``register_numbering_catalog_baseline`` 与
``rerun_document_skeleton_governance_report`` 都映射到固定 reviewer runbook：
先打开 ``source_report_display`` / ``source_json_display``，确认 scope、document key
对齐、catalog/baseline 覆盖率与 style-numbering issue，再按
``build_document_skeleton_governance_report.ps1``、
``build_document_skeleton_governance_rollup_report.ps1``、
``check_numbering_catalog_baseline.ps1``、
``check_numbering_catalog_manifest.ps1`` 或
``build_numbering_catalog_governance_report.ps1`` 重建证据，最后刷新 release
governance / release note bundle。

表格与版式交付治理会写出
``featherdoc.table_layout_delivery_governance_report.v1``。它的 ``delivery_quality``
明细会携带表格样式、自动修复、人工复核、浮动表格定位和命令失败计数；release
summary、handoff 与 reviewer bundle 应保留这些字段，让 reviewer 能区分“可发布”、
“需自动修复”和“需人工版式复核”。
PDF 浮动表格能力还会固定输出 ``pdf_floating_table_supported_geometry_percent`` 和
``pdf_floating_table_tracked_geometry_count``，把已支持的稳定几何子集和仍处于 metadata-only
的 ``tblpPr`` 字段变成可比较的百分比，而不只展示 supported / metadata-only 数量。
release governance handoff、artifact guide 和 reviewer checklist 会继续展示这些字段，
让发布复核能直接看到 PDF 浮动表格稳定几何覆盖率，而不用回到底层 layout JSON 手工换算。
当覆盖率低于 100% 时，这些 release-facing 材料还应给出
``pdf_floating_table_reviewer_focus``，明确提示 reviewer 复核 metadata-only
``tblpPr`` 字段，避免 reviewer 只看到百分比却不知道下一步该检查什么。
同一证据还应保留 ``pdf_floating_table_metadata_only_fields`` 与
``pdf_floating_table_review_required_fields``，让 release summary、handoff、
artifact guide 和 reviewer checklist 能直接列出具体 metadata-only 字段与完整
Word-compatible wrapping 复核边界。

表格与版式 action 也应映射到固定 reviewer runbook：
``apply_safe_tblLook_fixes_then_visual_regression`` 和
``apply_safe_tblLook_fixes`` 先应用 ``tblLook``-only 安全修复，再重跑
``run_table_style_quality_visual_regression``；``review_table_style_quality_plan`` 与
``review_manual_table_style_definition_work`` 用于人工复核表格样式定义；
``review_table_position_plan``、``review_floating_table_position_plans`` 与
``dry_run_apply_table_position_plans`` 用于浮动表格定位计划复核和指纹 dry-run；
``review_table_layout_delivery_governance_sources`` 和
``rebuild_table_layout_delivery_rollup`` 则固定回到 source JSON、rollup 与 governance
报告重建路径，避免 reviewer 只看到 action token 而不知道该打开哪份证据。

此外，``featherdoc.release_governance_pipeline_report.v1`` 的 ``stages[]`` 现在也会
保留每个治理 stage 的 ``release_blockers``、``warnings`` 与 ``action_items`` 明细。
这些 stage-level 明细会补齐 ``stage_id``、``stage_title``、``source_schema``、
``source_report_display`` 与 ``source_json_display``；action item 还会保留
``open_command``。发布面板可以先按 stage 过滤 ``numbering_catalog_governance``、
``content_control_data_binding_governance``、``project_template_delivery_readiness`` 或
``schema_patch_confidence_calibration``。现在 pipeline 也会生成
``docx_functional_smoke_readiness``（schema:
``featherdoc.docx_functional_smoke_readiness.v1``），复用既有 Word visual smoke PNG 非空证据；
该 stage 必须保留 ``docx_functional_smoke_readiness_trace``、
``persisted_docx_functional_smoke_evidence_only``、``summary_json_display`` 和
``report_markdown_display``，让 reviewer 能确认它是低资源复用证据而不是新鲜 Word
渲染。该 stage 的治理消费路径固定为
``output/docx-functional-smoke-readiness/summary.json`` 和
``output/docx-functional-smoke-readiness/docx_functional_smoke_readiness.md``；
本地低资源复核默认路径固定为
``output/docx-functional-smoke-readiness-current/summary.json`` 和
``output/docx-functional-smoke-readiness-current/docx_functional_smoke_readiness.md``。
当 ``review_result.json`` 仍是 ``pending_manual_review`` 时保留
``word_visual_smoke.pending_manual_review`` warning 边界，并继续传递
``warning_count`` 与 ``release_blocker_count``，
当截图级 review verdict 已记录为 ``pass`` 时则作为闭合证据传递给 final rollup、release
summary、bundle 和 reviewer checklist 继续展示。reviewer 若在 pipeline summary 中看到
阻断项，应优先打开 ``source_json_display`` 对应证据，再按 ``open_command`` 重建或复核
该治理报告，最后重新运行 release blocker rollup / bundle 生成链路。

pipeline summary 还会写出 ``local_governance_closure``（schema:
``featherdoc.release_governance_local_closure.v1``），把本地发布治理闭环从隐式
stage 列表提升为可审计字段。该对象必须保留
``local_governance_closure.status``、``local_governance_closure.closed``、
``governance_detail_source``、``pipeline_summary_json`` /
``pipeline_summary_json_display``、``pipeline_report_markdown`` /
``pipeline_report_markdown_display``、``final_governance_report_count``、
``final_governance_reports``、``required_stage_count``、
``completed_required_stage_count`` 与 ``required_stages``；其中 ``required_stages``
固定覆盖 ``docx_functional_smoke_readiness``、``release_governance_handoff`` 与
``release_blocker_rollup``。reviewer 可以先看 ``local_governance_closure.closed``，
再打开 pipeline summary / Markdown 确认 DOCX readiness、handoff 和 final rollup 是否
都已在本地完成并进入最终治理证据。

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
``release_assets_manifest``、三个必需入口、必需 governance contracts、必需字段
（至少包含 ``status``、``release_ready``、``release_blocker_count``、
``warning_count``、``schema_approval_status_summary``、
``onboarding_governance_next_action``、``onboarding_governance_next_action_summary``、
``onboarding_governance_next_action_group_count``、``next_action``、
``next_action_summary``、``next_action_group_count``、``source_report_display`` 和
``source_json_display``）和
``reviewer_manifest_scoped_project_template_trace``，避免 packaged manifest signoff
只停留在打包产物层。固定标记：
``manifest_signoff_entrypoints_release_trace``。
``release_assets_manifest.json`` 必须继续保留同一组 signoff 字段和
``reviewer_manifest_scoped_project_template_trace``，固定标记：
``manifest_signoff_entrypoints_manifest_trace``。
``release_blocker_rollup.md`` 的
``Source Report Contracts`` 必须先把同一组字段保留在
``featherdoc.release_candidate_summary`` 列表块内，不能由 detached notes 或
``featherdoc.release_blocker_rollup_report.v1`` 冒充原始 signoff 证据。固定标记：
``manifest_signoff_entrypoints_rollup_material_safety_trace``。
``release_governance_handoff.md`` 中的
``manifest_signoff_entrypoints_source_reports`` 必须继续把上述字段保留在同一个
``schema=featherdoc.release_candidate_summary`` 的 ``source_report:`` block 中，
不能由 detached notes 补齐。固定标记：
``manifest_signoff_entrypoints_handoff_material_safety_trace``。

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
``checklist_label``、``checklist_path``、``required_entrypoint_count=3``、三个入口的
``required=true`` / ``path_display`` 和固定 marker，而不需要 reviewer 从 packaged
manifest 反向推断。固定标记：
``project_template_readiness_checklist_entrypoints_governance_trace``。
``assert_release_material_safety.ps1`` 还必须审计
``release_governance_handoff.md``、``final_review.md``、``release_handoff.md`` 与
三个 release entry 文档中同一个 ``source_report`` 列表块是否同时携带
``checklist_path``、``required_entrypoint_count=3``、三个必需入口的
``required=true`` / ``path_display`` 和
``release_entry_project_template_readiness_checklist_trace``，避免 detached notes
补齐发布证据；同块还必须保留 ``featherdoc.release_candidate_summary`` schema 身份，
避免 release blocker rollup 或其它报告冒充 checklist entrypoints 的原始来源。
固定标记：
``project_template_readiness_checklist_entrypoints_material_safety_trace``、
``project_template_readiness_checklist_entrypoints_handoff_source_schema_identity_trace``、
``project_template_readiness_checklist_entrypoints_final_review_source_schema_identity_trace``、
``project_template_readiness_checklist_entrypoints_release_metadata_details_source_schema_identity_trace``。
``release_blocker_rollup.md`` 的 ``Source Report Contracts`` 也必须把 checklist
entrypoints 的 status、checklist label/path、required entrypoint count、entrypoint ids
和 checklist marker 保留在同一个 ``featherdoc.release_candidate_summary`` 列表块内，
不能由 detached notes 或 rollup 自身 schema 冒充。固定标记：
``project_template_readiness_checklist_entrypoints_rollup_material_safety_trace``。
``write_release_metadata_start_here.ps1``、``write_release_artifact_guide.ps1`` 和
``write_release_reviewer_checklist.ps1`` 还会把同一证据压缩成
``Project-template readiness checklist handoff evidence`` 行，并写入
``START_HERE.md``、``ARTIFACT_GUIDE.md`` 与 ``REVIEWER_CHECKLIST.md``，避免 reviewer
必须进入内部 handoff 章节才能看到 checklist source-report 证据。固定标记：
``project_template_readiness_checklist_entrypoints_release_entry_trace``。
``assert_release_material_safety.ps1`` 会继续审计该 compact evidence 行，要求
``project_template_readiness_checklist_entrypoints_source_reports``、``status``、
``checklist_path``、``required_entrypoint_count=3``、``entrypoint_paths``、
三个入口的 ``path_display``、固定 marker、
``source_schema=featherdoc.release_candidate_summary`` 与 ``source_report`` 保持在同一行，
避免 detached notes 补齐入口材料；``source_schema`` 必须显式保留 release-candidate
summary schema 身份，``source_report`` 还必须能识别 release-candidate summary
证据源，避免把同名 compact evidence 挂到错误报告上。固定标记：
``project_template_readiness_checklist_entrypoints_release_entry_path_display_trace``、
``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``、
``project_template_readiness_checklist_entrypoints_release_entry_source_report_identity_trace``。
同一 compact evidence 的 ``source_report`` 选择必须优先指向最终
``release-candidate-checks\report\summary.json``；即使
``release-candidate-checks-source\summary.json`` 在 source report 列表中排在前面，
也不能用 source-only 摘要替代最终 release-candidate summary。

``START_HERE.md``、``ARTIFACT_GUIDE.md``、``REVIEWER_CHECKLIST.md``、
``final_review.md`` 与 ``release_handoff.md`` 也会渲染
``Word visual standard review metadata evidence`` compact line，用同一条 release
governance handoff evidence 暴露四个标准 Word 视觉审阅任务的
``word_visual_standard_review_metadata_source_reports``、``task_reviews=``、
task/review key、verdict、review status、review method、review result path 与
``final_review_path=``。``release_governance_handoff.md`` 继续保留 detailed
``word_visual_standard_review_metadata_source_reports`` source reports，不降级为
发布入口使用的 compact line。``assert_release_material_safety.ps1`` 会审计该行仍把
``source_schema=featherdoc.release_candidate_summary`` 与 ``source_report`` 保持在同一行，
且 ``source_report`` 指向 ``release-candidate-checks`` 的 release-candidate summary
证据源，并继续禁止 ``review_note`` 进入任何发布入口。
该 compact line 也沿用相同的最终报告优先规则：``release-candidate-checks-source``
只能作为源材料来源，不能覆盖 ``release-candidate-checks\report\summary.json``。

``package_release_assets.ps1`` 在 staged release materials 审计前还会直接检查
``START_HERE.md``、``ARTIFACT_GUIDE.md`` 与 ``REVIEWER_CHECKLIST.md`` 中的同一条
compact evidence 行，并把通过后的
``release_entry_project_template_readiness_checklist_material_safety_audit`` 写入
``release_assets_manifest.json``；``assert_release_material_safety.ps1`` 会继续审计该
manifest 字段，避免打包阶段只靠日志推断。固定标记：
``project_template_readiness_checklist_entrypoints_packaged_material_safety_trace``。
``build_release_blocker_rollup_report.ps1`` 还必须继续消费这个 packaged audit，
并在 source report contract evidence 中保留 ``status``、``audit_script``、
``audited_entrypoints``、compact evidence label/field/source schema、固定 checklist 路径和
``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``；
``compact_evidence_source_schema`` 必须固定为
``featherdoc.release_candidate_summary``。

同一轮 staged release material safety audit 也会审计 project-template workflow dashboard
入口材料：只要 ``START_HERE.md``、``ARTIFACT_GUIDE.md`` 或
``REVIEWER_CHECKLIST.md`` 出现 ``Project template workflow dashboard``，就必须保留
status、release-ready、counts、summary/report 路径、``next action groups`` 和非零
``action group`` 摘要；``REVIEWER_CHECKLIST.md`` 在 dashboard 未 release-ready 或
blocked/failed/missing 时必须保留 stop condition。
这样 release blocker rollup 能直接展示打包入口材料已经过 material-safety 审计，
不需要 reviewer 从 ``release_assets_manifest.json`` 手工反推。固定标记：
``project_template_readiness_checklist_entrypoints_packaged_audit_rollup_trace``。
``release_blocker_rollup.md`` 的 ``Source Report Contracts`` 还必须把该 packaged
audit 的 status、audit script、audited entrypoints、compact evidence label/field/source
schema、checklist path/marker 和 material-safety marker 保留在同一个
``featherdoc.release_candidate_summary`` 列表块内，不能由 detached notes 或 rollup
自身 schema 冒充。固定标记：
``project_template_readiness_checklist_entrypoints_packaged_audit_rollup_material_safety_trace``。
``build_release_governance_handoff_report.ps1`` 会继续把该 packaged audit 汇总成
``release_entry_project_template_readiness_checklist_material_safety_audit_source_reports``；
``run_release_candidate_checks.ps1`` 必须把该数组和计数同步到
``release_governance_handoff`` 与 ``steps.release_governance_handoff``。共享 release
metadata helper 也必须在 handoff details 中展示 status、audit script、audited
entrypoints、compact evidence field/source schema 和 material-safety marker，避免最终
summary/entry 材料只能看到 checklist entrypoints，却看不到 staged entry materials
已经通过审计。
固定标记：
``project_template_readiness_checklist_entrypoints_packaged_audit_handoff_trace``。
``assert_release_material_safety.ps1`` 还会审计
``release_governance_handoff.md``、``final_review.md``、``release_handoff.md`` 与
三个 release entry 文档中同一个 packaged audit ``source_report`` 列表块，要求 audit
status、audit script、三个 audited entrypoints、compact evidence field、checklist path、
checklist marker 与 material-safety marker 保持同块，避免 detached notes 补齐最终
handoff 证据；同块还必须保留 ``featherdoc.release_candidate_summary`` schema 身份，
避免 rollup summary 冒充 packaged audit 原始来源。固定标记：
``project_template_readiness_checklist_entrypoints_packaged_audit_handoff_material_safety_trace``、
``project_template_readiness_checklist_entrypoints_packaged_audit_handoff_source_schema_identity_trace``、
``project_template_readiness_checklist_entrypoints_packaged_audit_final_review_source_schema_identity_trace``、
``project_template_readiness_checklist_entrypoints_packaged_audit_release_metadata_details_source_schema_identity_trace``。
``write_release_metadata_start_here.ps1``、``write_release_artifact_guide.ps1`` 和
``write_release_reviewer_checklist.ps1`` 还会把该 packaged audit 压缩成
``Project-template readiness checklist packaged audit evidence`` 行，展示到三个最终
release entry summary 中，让 reviewer 可以直接看到 staged entry materials 已经通过
material-safety 审计。固定标记：
``project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_trace``。
``assert_release_material_safety.ps1`` 会继续审计这条 packaged audit compact evidence 行，
要求 count、status、audit script、三个 audited entrypoints、compact evidence identity、
``compact_evidence_source_schema=featherdoc.release_candidate_summary``、checklist path、
checklist marker、material-safety marker、
``source_schema=featherdoc.release_candidate_summary`` 和 ``source_report`` 保持同一行，
避免 detached notes 补齐入口材料；``compact_evidence_source_schema`` 标识被审计的
compact evidence 原始 schema，``source_schema`` 必须显式保留当前 source report block
的 release-candidate summary schema 身份，``source_report`` 还必须能识别 release-blocker rollup 证据源，
避免 release-candidate summary 冒充打包入口审计来源。固定标记：
``project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_material_safety_trace``、
``project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_source_report_identity_trace``。

对 project-template governance，``final_review.md`` 和
``steps.release_governance_handoff`` 还必须同时保留
``source_report_display`` 与 ``source_json_display``：前者指向
``project-template-delivery-readiness`` 汇总，后者可回到
``project-template-onboarding-governance`` 原始证据。这样发布 reviewer 可以从
最终 release candidate 摘要直接追溯到模板准入与 schema approval 证据，而不是在
handoff 计数和 release blocker rollup 之间手工拼路径。
release blocker rollup 的 ``source_reports`` 也必须进入 release candidate summary
和 ``steps.release_blocker_rollup``，并在 rollup Markdown、governance handoff
Markdown 与 ``final_review.md`` 中展示 ``business_document_type_summary`` 与
``corpus_role_summary``，避免 final review 只能看到 readiness/source path 而看不到
业务文档类型和语料角色覆盖范围。
packaged ``release_assets_manifest.json`` 也必须在
``project_template_delivery_readiness_contract`` 与
``project_template_onboarding_governance_contract`` 中保留这两组 summary。完整 package、
allow-incomplete package 与 warning-only readiness package 都要值级验证，不能只依赖
``manifest_signoff_entrypoints.required_fields`` 的字段名清单。
release material safety 负例也必须覆盖缺失或空数组情况：缺失
``project_template_delivery_readiness_contract.business_document_type_summary`` 或
``project_template_onboarding_governance_contract.corpus_role_summary`` 为空时，审计必须
失败。
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
面向发布说明有意义的 verdict / status 摘要，以及 project-template governance
contract 中可公开复核的 reviewer action 与业务维度摘要，例如
``business_document_type_summary`` 和 ``corpus_role_summary``。

``write_release_note_bundle.ps1`` 默认会运行 ``assert_release_material_safety.ps1``，
用于防止内部路径、操作员备注或不适合公开发布的内容进入 release material。
``run_release_candidate_checks.ps1`` 也会在最终材料写完并消毒后再次审计。
只有聚焦内容渲染或部分治理 fixture 的回归测试才应使用
``-SkipMaterialSafetyAudit``，正式发布和打包流程不应跳过该审计。


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

正式发版前推荐启用严格 release gate，让 release blocker rollup 与 governance
handoff 中的 blocker、missing report 和 warning 都能直接阻断候选发布：

.. code-block:: powershell

    pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1 `
        -ReleaseBlockerRollupAutoDiscover `
        -ReleaseBlockerRollupFailOnBlocker `
        -ReleaseBlockerRollupFailOnWarning `
        -ReleaseGovernanceHandoff `
        -ReleaseGovernanceHandoffIncludeRollup `
        -ReleaseGovernanceHandoffFailOnMissing `
        -ReleaseGovernanceHandoffFailOnBlocker `
        -ReleaseGovernanceHandoffFailOnWarning

只同步最新视觉 verdict：

.. code-block:: powershell

    pwsh -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1

只从已有 review task 证据刷新 README gallery：

.. code-block:: powershell

    powershell -ExecutionPolicy Bypass -File .\scripts\refresh_readme_visual_assets.ps1

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
