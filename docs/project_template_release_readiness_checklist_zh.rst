项目模板发布准入清单（中文）
============================

这份清单是 project-template governance 的固定发布入口。它不重新运行
``Word``、``CMake``、``CTest`` 或视觉自动化，只规定每次发布前必须把哪些
模板契约证据收束到同一个可追溯结论。

目标是把项目模板从“本地 smoke 可用”推进到“可接入、可审计、可发布”的
状态。发布 reviewer 应优先从这里进入，而不是在 onboarding、schema
approval、content-control 和 release governance 报告之间手工拼结论。


准入边界
--------

包含：

* 项目模板 onboarding summary / onboarding governance。
* Template schema baseline、patch review 和 schema approval history。
* Content-control data-binding governance，特别是 Custom XML 同步、重复绑定、
  bound placeholder 和修复命令。
* Project template delivery readiness 汇总。
* Release blocker rollup、release governance pipeline、handoff、reviewer
  checklist 和公开 release bundle 中的可追溯字段。

不包含：

* 临时承诺任意业务模板都能自动生成最终交付文档。
* 绕过 schema approval，把 generated schema 直接当作已批准 baseline。
* 把 content-control 表单系统扩展成全量表单引擎。
* 只新增 CLI 命令但不接入 release governance 的旁路线。


固定检查顺序
------------

1. 模板接入证据已生成：

   * 新模板必须先跑 ``scripts/onboard_project_template.ps1`` 或等价的
     onboarding 入口。
   * 候选清单应能进入 ``scripts/build_project_template_onboarding_governance_report.ps1``。
   * 输出必须保留 ``source_schema``、``source_json_display``、
     ``source_report_display`` 和 reviewer 可执行的 ``open_command``。

2. Schema approval 状态明确：

   * ``scripts/run_project_template_smoke.ps1`` 生成的
     ``schema_patch_review.json`` 和 ``schema_patch_approval_result.json`` 不能
     只停留在临时输出目录里。
   * ``scripts/sync_project_template_schema_approval.ps1`` 必须把 reviewer
     决策同步回 smoke summary 或 release summary。
   * ``scripts/write_project_template_schema_approval_history.ps1`` 必须生成
     ``featherdoc.project_template_schema_approval_history.v1``。
   * 发布前 ``latest_schema_approval_gate_status`` 不能是 ``blocked``；如果是
     ``pending``，必须在 release blocker / action item 中给出下一步。

3. Delivery readiness 已汇总：

   * 运行或复用 ``scripts/build_project_template_delivery_readiness_report.ps1``。
   * ``summary.json`` 必须包含
     ``featherdoc.project_template_delivery_readiness_report.v1``。
   * ``release_ready``、``status``、``template_count``、
     ``ready_template_count``、``blocked_template_count``、
     ``release_blocker_count`` 和 ``latest_schema_approval_gate_status`` 必须明确。
   * 若 ``release_ready = false``，``release_blocker_count`` 必须大于 ``0``，
     且 blocker 必须携带稳定 ``id``、``action``、``message`` 和 source display
     字段。

4. Content-control data-binding 治理已接入：

   * 运行或复用 ``scripts/build_content_control_data_binding_governance_report.ps1``。
   * 报告 schema 必须是
     ``featherdoc.content_control_data_binding_governance_report.v1``。
   * ``content_control_data_binding.bound_placeholder`` 必须继续作为稳定 blocker id。
   * bound placeholder 的修复策略必须指向 ``sync_bound_content_control`` 和
     ``sync-content-controls-from-custom-xml``，不能只给人工描述。
   * bound placeholder、重复绑定 action、warning 和 repair plan 必须保留
     ``input_docx``、``template_name``、``schema_target`` 和 ``target_mode``，
     让 reviewer 能从 release bundle 追溯到具体模板、输入 DOCX 与 schema 目标。
   * ``repair_plan_schema`` 必须保留
     ``featherdoc.content_control_data_binding_repair_plan.v1``。

5. Release governance 已消费结论：

   * ``scripts/build_release_blocker_rollup_report.ps1`` 必须能聚合
     project template delivery readiness 和 content-control governance。
   * ``scripts/build_release_governance_pipeline_report.ps1`` 的 ``stages[]`` 必须保留
     ``project_template_delivery_readiness`` 和
     ``content_control_data_binding_governance``。
   * ``scripts/build_release_governance_handoff_report.ps1`` 必须把同一批
     blocker / warning / action item 明细继续写入 handoff。
   * ``scripts/run_release_candidate_checks.ps1`` 生成的 summary / final review
     不能只显示计数，必须保留可打开的 ``source_report_display`` /
     ``source_json_display``。其中 ``steps.release_governance_handoff`` 和
     ``final_review.md`` 必须能从 ``project_template_delivery_readiness`` 追溯到
     ``project-template-delivery-readiness`` 汇总，并能从
     ``project_template_onboarding_governance`` 回到
     ``project-template-onboarding-governance`` 原始 JSON。
   * release blocker rollup、handoff、START_HERE、ARTIFACT_GUIDE 和
     REVIEWER_CHECKLIST 必须继续显示 content-control provenance，不得只保留
     ``source_json_display`` 与修复命令。

6. 公开材料安全审计已覆盖：

   * ``scripts/assert_release_material_safety.ps1`` 必须继续检查
     ``project_template_delivery_readiness_contract``。
   * 公开材料里出现 ``content_control_data_binding.bound_placeholder`` 时，必须同时
     出现 ``source_schema``、``source_json_display``、``repair_strategy``、
     ``repair_hint``、``command_template``、``input_docx``、``template_name``、
     ``schema_target`` 和 ``target_mode``。
   * ``START_HERE.md``、``ARTIFACT_GUIDE.md``、``REVIEWER_CHECKLIST.md``、
     ``release_handoff.md`` 和 ``release_body.zh-CN.md`` 不能丢失这些治理入口。


推荐轻量验证
------------

资源紧张时，优先运行这些不启动 Word、浏览器、LibreOffice 或完整构建的测试：

.. code-block:: powershell

   powershell -NoProfile -ExecutionPolicy Bypass -File .\test\project_template_release_readiness_checklist_docs_contract_test.ps1 -RepoRoot .
   powershell -NoProfile -ExecutionPolicy Bypass -File .\test\build_project_template_delivery_readiness_report_test.ps1 -RepoRoot . -WorkingDir .\build\project-template-readiness-check -Scenario aggregate
   powershell -NoProfile -ExecutionPolicy Bypass -File .\test\build_content_control_data_binding_governance_report_test.ps1 -RepoRoot . -WorkingDir .\build\content-control-governance-check
   powershell -NoProfile -ExecutionPolicy Bypass -File .\test\release_governance_metrics_contract_test.ps1 -RepoRoot .
   powershell -NoProfile -ExecutionPolicy Bypass -File .\test\assert_release_material_safety_test.ps1 -RepoRoot . -WorkingDir .\build\release-material-safety-check

如果资源允许，再让 ``scripts/run_release_candidate_checks.ps1`` 生成完整 release
candidate summary，并检查 final review、handoff 和 reviewer checklist 中是否能从
``project_template_delivery_readiness`` 与
``content_control_data_binding_governance`` 追溯到原始 JSON。


发布判定
--------

可以发布：

* ``project_template_delivery_readiness`` 的 ``release_ready`` 为 ``true``，或者
  所有 blocker 都有明确 owner、action 和 source display。
* Schema approval history 没有 ``blocked`` gate。
* Content-control data-binding blocker 要么已清零，要么公开材料中有稳定修复路径，
  并保留 ``input_docx``、``template_name``、``schema_target`` 与 ``target_mode``
  provenance。
* Release blocker rollup、governance pipeline、handoff 和 reviewer checklist
  都消费了同一批治理证据。

不能发布：

* schema approval 结果缺少 reviewer / reviewed_at 等审计字段。
* generated schema 未经 approval 被当作 baseline。
* ``content_control_data_binding.bound_placeholder`` 出现在公开材料里但缺少修复命令
  或缺少 ``input_docx``、``template_name``、``schema_target``、``target_mode``
  provenance。
* release summary 只有计数，没有 ``source_json_display`` /
  ``source_report_display``。
* reviewer 无法从 release bundle 回到原始治理报告。
