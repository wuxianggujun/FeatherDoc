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
   * ``output/project-template-smoke/summary.json`` 必须保留
     ``featherdoc.project_template_smoke_summary.v1``；delivery readiness
     可以把 ``project_template_smoke_summary`` 作为模板 smoke 证据消费，
     但它仍不能替代 reviewer 对 schema approval history 的追溯。
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
   * 若只有 ``samples/project_template_smoke.manifest.json`` 这类注册清单，
     但还没有 ``output/project-template-smoke/summary.json`` 或 onboarding
     summary，必须先用
     ``scripts/describe_project_template_smoke_manifest.ps1 -Json`` 生成
     ``featherdoc.project_template_smoke_manifest_description.v1`` 作为清单诊断；
     该诊断必须保留 ``business_template_corpus_count``、
     ``registered_business_template_corpus_count``、
     ``planned_business_template_corpus_count`` 和 ``business_document_type_summary``，
     让 reviewer 能区分已注册业务模板与只做计划跟踪的语料类型；
     readiness 只能给出
     ``project_template_smoke_summary_missing`` warning，不能把 manifest
     本身当作模板已通过 smoke 的证据。
   * 一旦加载到 ``featherdoc.project_template_smoke_summary.v1``，
     ``build_project_template_delivery_readiness_report.ps1`` 必须把每个
     smoke entry 映射为 readiness template，并保留
     ``source_json_display`` / ``source_report_display`` 指向原始 smoke
     summary。
   * ``release_ready``、``status``、``template_count``、
     ``ready_template_count``、``blocked_template_count``、
     ``release_blocker_count``、``latest_schema_approval_gate_status`` 和
     ``schema_approval_status_summary`` 必须明确。
   * 若 ``release_ready = false`` 且 ``status = blocked``，``release_blocker_count``
     必须大于 ``0``，且 blocker 必须携带稳定 ``id``、``action``、``message``
     和 source display 字段；若只有 manifest-only / missing-evidence 这类
     warning，``status`` 必须保持 ``needs_review``，并通过 ``warning_count`` 与
     ``warnings[]`` 暴露修复入口。manifest-only warning 也应保留
     ``business_template_corpus_count``、``registered_business_template_corpus_count``
     和 ``planned_business_template_corpus_count``，避免 release 面板只显示注册
     entry 数量却丢失真实业务语料规划边界。

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
   * ``repair_action_class_summary`` 必须把 repair plan 分成
     ``release_blocking``、``auto_repair_candidate`` 和
     ``manual_confirmation_required`` 三类；每个 ``repair_plan_items[]`` 条目也必须保留
     ``repair_action_classes``、``source_report_display``、``source_json_display`` 和
     ``command_template``，让 reviewer 能区分自动可执行候选、需人工确认和必须阻断发布的
     content-control / Custom XML 动作。

5. Release governance 已消费结论：

   * ``scripts/build_release_blocker_rollup_report.ps1`` 必须能聚合
     project template delivery readiness 和 content-control governance。
   * ``scripts/build_release_governance_pipeline_report.ps1`` 的 ``stages[]`` 必须保留
     ``project_template_delivery_readiness`` 和
     ``content_control_data_binding_governance``；同时必须保留
     ``docx_functional_smoke_readiness``，让 DOCX 功能 smoke 与复用 Word visual PNG
     非空证据进入同一条 release governance 证据链；其 summary schema 必须保留
     ``featherdoc.docx_functional_smoke_readiness.v1``，同时保留
     ``docx_functional_smoke_readiness_trace``、
     ``persisted_docx_functional_smoke_evidence_only``、
     ``word_visual_smoke.pending_manual_review``、``summary_json_display`` 和
     ``report_markdown_display``，以便 handoff 按固定报告类型消费并展示低资源证据边界。
     该 stage 的治理消费路径固定为
     ``output/docx-functional-smoke-readiness/summary.json`` 和
     ``output/docx-functional-smoke-readiness/docx_functional_smoke_readiness.md``；
     本地低资源复核默认路径固定为
     ``output/docx-functional-smoke-readiness-current/summary.json`` 和
     ``output/docx-functional-smoke-readiness-current/docx_functional_smoke_readiness.md``。
     如果该 stage 产生 ``restore_docx_functional_smoke_evidence`` blocker action，
     reviewer 必须先恢复缺失的持久化 DOCX 功能或 Word visual smoke 证据，再只读重跑
     ``check_docx_functional_smoke_readiness.ps1``，不能把它当作新鲜 Word COM 渲染。
   * ``scripts/build_release_governance_handoff_report.ps1`` 必须把同一批
     blocker / warning / action item 明细继续写入 handoff。
   * content-control ``repair_action_classes`` 必须作为 reviewer-facing 字段从
     release blocker rollup 继续进入 handoff、final review、START_HERE、
     ARTIFACT_GUIDE 和 REVIEWER_CHECKLIST。
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
   * ``START_HERE.md``、``ARTIFACT_GUIDE.md`` 和 ``REVIEWER_CHECKLIST.md`` 中
     任何同时出现 ``project_template_delivery_readiness`` 与
     ``project_template_onboarding.schema_approval`` 的材料，都必须把两条契约放在
     各自独立的行里；不能让 readiness 行的 ``source_report_display`` /
     ``source_json_display`` 顶到账 onboarding 行。固定标记：
     ``block_scoped_entry_trace``。

6. 公开材料安全审计已覆盖：

   * ``scripts/assert_release_material_safety.ps1`` 必须继续检查
     ``project_template_delivery_readiness_contract``，并要求入口材料中
     ``project_template_delivery_readiness`` 所在行携带
     ``schema_approval_status_summary``，不能被 onboarding 行的同名字段误满足。
     入口材料只要出现 ``project_template_onboarding.schema_approval``，也必须同时
     保留 delivery readiness 与 onboarding governance 两条契约，不能只暴露单边阻断项。
   * 公开材料里出现 ``content_control_data_binding.bound_placeholder`` 时，必须同时
     出现 ``source_schema``、``source_json_display``、``repair_strategy``、
     ``repair_hint``、``command_template``、``input_docx``、``template_name``、
     ``schema_target`` 和 ``target_mode``。这些字段必须和 blocker id 保持在
     同一个 Markdown list block，或保持在相邻的 content-control 入口条目组内；
     不能让 detached notes 补齐当前入口。固定标记：
     ``block_scoped_entry_content_control_trace``。
   * 入口材料里出现 ``numbering_catalog_governance.real_corpus_confidence`` 或
     ``table_layout_delivery_governance.delivery_quality`` 时，覆盖率、匹配文档、
     penalty summary、表格样式/定位计数和 ready/unresolved 详情必须和对应指标保持
     在同一行或同一个 Markdown list block 内；PDF 浮动表还必须保留
     ``pdf_floating_table_support_coverage``、``pdf_floating_table_reviewer_focus``、
     ``pdf_floating_table_metadata_only_fields``、
     ``pdf_floating_table_review_required_fields`` 和 metadata-only ``tblpPr`` 复核提示，
     确保 reviewer 知道哪些几何能力已经覆盖、哪些字段仍只是元数据级交付；
     不能让 detached notes 补齐当前指标。
     固定标记：``block_scoped_entry_governance_metric_trace``。
   * 入口材料里出现 ``project_template_delivery_readiness`` 或
     ``project_template_onboarding.schema_approval`` 时，``source_report_display``、
     ``source_json_display``、``schema_approval_status_summary`` 与契约详情必须和对应
     anchor 保持在同一个 Markdown list block 内；不能让 detached notes 把 readiness /
     onboarding 字段拆开拼接。固定标记：
     ``block_scoped_entry_project_template_trace``、
     ``block_scoped_release_bundle_project_template_trace``。
     其中 readiness block 还必须保留 ``status`` 与 ``release_ready``，不能只留下
     schema approval 状态；``status`` 只能使用可解释 readiness 枚举，
     ``release_ready`` 只能使用布尔值。固定标记：
     ``block_scoped_entry_project_template_status_trace``、
     ``entry_readiness_value_set_trace``。
   * ``START_HERE.md``、``ARTIFACT_GUIDE.md``、``REVIEWER_CHECKLIST.md``、
     ``release_handoff.md``、``release_body.zh-CN.md``、
     ``release_summary.zh-CN.md``、``release_governance_handoff.md`` 和
     ``final_review.md`` 不能丢失这些治理入口。
   * ``release_governance_handoff.md`` 中的
     ``project_template_delivery_readiness`` 与
     ``project_template_onboarding.schema_approval`` 必须分别在自己的
     Markdown list block 内保留 ``schema_approval_status_summary``、
     ``source_report_display`` 和 ``source_json_display``；其中 readiness
     report-status block 还必须保留 ``status`` 与 ``ready``，不能只留下
     source display 或 schema approval 状态。不能让 readiness block、
     detached notes 或 source path 里的同名片段补齐 onboarding governance block。
     字段值也必须保持证据源身份：readiness 的 ``source_report_display`` /
     ``source_json_display`` 必须回到 delivery-readiness 证据，onboarding 的
     ``source_json_display`` 必须回到 onboarding governance 原始证据；``status``
     只能使用可解释 readiness 枚举，``ready`` 只能使用布尔值。嵌套
     ``project_template_onboarding_governance_contract`` 也必须携带并校验
     ``status`` / ``release_ready``，不能只靠 schema summary 推断 release 状态。
     固定标记：``block_scoped_governance_handoff_trace``、
     ``single_block_governance_handoff_project_template_trace``、
     ``block_scoped_governance_handoff_project_template_status_trace``、
     ``block_scoped_governance_handoff_source_identity_trace``、
     ``governance_handoff_readiness_value_set_trace``、
     ``onboarding_contract_status_release_ready_value_set_trace``。
   * ``release_handoff.md`` 中的
     ``project_template_delivery_readiness`` 与
     ``project_template_onboarding.schema_approval`` 也必须分别在自己的
     Markdown list block 内保留 ``source_report_display``、
     ``source_json_display`` 和对应 contract；不能让另一条契约的 source
     display、detached notes 或重复 anchor 补齐当前块。readiness contract 的
     ``status`` 只能使用可解释 readiness 枚举，``release_ready`` 只能使用布尔值。
     onboarding governance contract 也必须保留并校验 ``status`` /
     ``release_ready``。字段值也必须保持同样的 delivery-readiness /
     onboarding governance 源身份。
     固定标记：
     ``block_scoped_release_handoff_trace``、
     ``single_block_release_handoff_project_template_trace``、
     ``block_scoped_release_handoff_source_identity_trace``、
     ``release_handoff_readiness_value_set_trace``。
   * ``final_review.md`` 中的
     ``project_template_delivery_readiness / project_template_onboarding.schema_approval``
     handoff blocker block 必须同时保留 ``source_report_display``、
     ``source_json_display``、``readiness_status``、
     ``readiness_release_ready``、``schema_approval_status_summary`` 和
     onboarding governance contract；不能让 detached notes、重复 anchor、source path
     或其它段落补齐当前块；``source_json_display`` 字段值必须回到 onboarding
     governance 原始证据；``readiness_status`` 只能使用可解释枚举状态，
     ``readiness_release_ready`` 只能使用布尔值；嵌套 onboarding governance
     contract 的 ``status`` / ``release_ready`` 也必须使用相同值域。固定标记：
     ``final_review_readiness_value_set_trace``、
     ``block_scoped_final_review_project_template_status_trace``、
     ``block_scoped_final_review_project_template_trace``、
     ``single_block_final_review_project_template_trace``、
     ``block_scoped_final_review_source_identity_trace``。
   * ``release_summary.zh-CN.md`` 与 ``release_body.zh-CN.md`` 的
     project-template 短摘要必须把 readiness / onboarding anchor 与
     ``schema_approval_status_summary``、``source_report_display``、
     ``source_json_display`` 保持在同一行；不能让 detached notes 或另一条短摘要
     补齐当前 release note。固定标记：
     ``line_scoped_release_note_project_template_trace``。其中
     ``release_summary.zh-CN.md`` / ``release_body.zh-CN.md`` 里的字段值还必须能识别
     证据源：delivery readiness 的 ``source_report_display`` /
     ``source_json_display`` 必须指向 ``project_template_delivery_readiness`` /
     ``project-template-delivery-readiness``；onboarding governance 的
     ``source_report_display`` 可以指向 delivery-readiness 汇总或 onboarding 原始报告，
     但 ``source_json_display`` 必须回到 ``project_template_onboarding_governance`` /
     ``project-template-onboarding-governance`` 原始证据，不能由同一行其它 token 冒充。
     delivery readiness 与 onboarding governance 的 ``status`` 都只能使用可解释
     readiness 枚举，``release_ready`` 都只能使用布尔值，且 onboarding 行必须使用
     报告级结论，不能从 blocker / action item 推断整体发布状态。固定标记：
     ``line_scoped_release_note_source_identity_trace``、
     ``line_scoped_release_note_readiness_value_set_trace``、
     ``line_scoped_release_note_onboarding_value_set_trace``。
   * ``release_assets_manifest.json`` 必须保留
     ``project_template_delivery_readiness_contract`` 与
     ``project_template_onboarding_governance_contract``，并继续带出
     ``schema_approval_status_summary``、``onboarding_governance_next_action``、
     ``onboarding_governance_next_action_summary``、
     ``onboarding_governance_next_action_group_count``、``next_action``、
     ``next_action_summary``、``next_action_group_count``、``source_report_display``
     和 ``source_json_display``。两条契约的 source display 必须能识别自身证据源：
     delivery readiness 只能指向 ``project_template_delivery_readiness`` /
     ``project-template-delivery-readiness``，onboarding governance 只能指向
     ``project_template_onboarding_governance`` /
     ``project-template-onboarding-governance``，不能互相冒充。两条契约的
     ``status`` 与 ``release_ready`` 必须双向一致：``status = ready`` 等价于
     ``release_ready = true``，且 ``status`` 只能使用可解释 readiness 枚举、
     ``release_ready`` 只能使用布尔值，不能留下互相矛盾的发布结论。固定标记：
     ``manifest_scoped_project_template_trace``、
     ``manifest_source_display_identity_trace``、
     ``manifest_status_release_ready_consistency_trace``、
     ``manifest_readiness_value_set_trace``。
   * ``START_HERE.md``、``ARTIFACT_GUIDE.md`` 与 ``REVIEWER_CHECKLIST.md`` 必须
     直接保留 ``Project template release readiness checklist`` 与
     ``docs/project_template_release_readiness_checklist_zh.rst``，让 reviewer 从三个
     release entry 都能回到本固定准入入口。固定标记：
     ``release_entry_project_template_readiness_checklist_trace``。
   * ``scripts/run_release_candidate_checks.ps1`` 生成的 ``summary.json`` 必须声明
     ``project_template_readiness_checklist_entrypoints``，把上述三个 release entry、
     ``Project template release readiness checklist``、固定 checklist 路径与
     ``release_entry_project_template_readiness_checklist_trace`` 作为机器可消费证据。
     ``scripts/package_release_assets.ps1`` 写出的 ``release_assets_manifest.json`` 必须
     保留该字段，并由 ``scripts/assert_release_material_safety.ps1`` 直接审计，不能只依赖
     人工阅读 entry 文案。固定标记：
     ``project_template_readiness_checklist_entrypoints_manifest_trace``。
   * ``scripts/build_release_blocker_rollup_report.ps1`` 必须把
     ``project_template_readiness_checklist_entrypoints`` 展平成 source report contract
     字段；``scripts/build_release_governance_handoff_report.ps1`` 必须继续汇总为
     ``project_template_readiness_checklist_entrypoints_source_reports``，并由
     ``release_governance_handoff.md``、``final_review.md``、``START_HERE.md``、
     ``ARTIFACT_GUIDE.md`` 和 ``REVIEWER_CHECKLIST.md`` 展示 ``status``、
     ``checklist_label``、``checklist_path``、``required_entrypoint_count=3``、
     三个必需入口的 ``required=true`` / ``path_display`` 和
     ``release_entry_project_template_readiness_checklist_trace``。固定标记：
     ``project_template_readiness_checklist_entrypoints_governance_trace``。
     ``scripts/assert_release_material_safety.ps1`` 必须继续审计
     ``release_governance_handoff.md``、``final_review.md``、``release_handoff.md`` 与
     三个 release entry 文档中同一个 ``source_report`` 列表块是否同时携带
     ``checklist_path``、``required_entrypoint_count=3``、三个必需入口的
     ``required=true`` / ``path_display`` 和固定 checklist marker，不能把 marker
     放到 detached notes 中补齐；同块还必须保留
     ``featherdoc.release_candidate_summary`` schema 身份，避免 release blocker
     rollup 或其它报告冒充原始 release-candidate 证据。固定标记：
     ``project_template_readiness_checklist_entrypoints_material_safety_trace``、
     ``project_template_readiness_checklist_entrypoints_handoff_source_schema_identity_trace``、
     ``project_template_readiness_checklist_entrypoints_final_review_source_schema_identity_trace``、
     ``project_template_readiness_checklist_entrypoints_release_metadata_details_source_schema_identity_trace``。
     ``release_blocker_rollup.md`` 的 ``Source Report Contracts`` 也必须把
     ``project_template_readiness_checklist_entrypoints`` 的 status、checklist label/path、
     required entrypoint count、entrypoint ids 和 checklist marker 保留在同一个
     ``featherdoc.release_candidate_summary`` 列表块内，不能由 detached notes 或 rollup
     自身 schema 冒充。固定标记：
     ``project_template_readiness_checklist_entrypoints_rollup_material_safety_trace``。
     ``scripts/write_release_metadata_start_here.ps1``、
     ``scripts/write_release_artifact_guide.ps1`` 与
     ``scripts/write_release_reviewer_checklist.ps1`` 还必须把
     ``project_template_readiness_checklist_entrypoints_source_reports`` 的 compact evidence
     行展示到发布入口材料，让 reviewer 在 ``START_HERE.md``、``ARTIFACT_GUIDE.md`` 和
     ``REVIEWER_CHECKLIST.md`` 中直接看到 ``checklist_path``、
     ``required_entrypoint_count=3``、``entrypoint_paths``、三个入口的
     ``path_display``、固定 marker 和
     ``source_schema=featherdoc.release_candidate_summary``。
     固定标记：``project_template_readiness_checklist_entrypoints_release_entry_trace``。
     ``scripts/assert_release_material_safety.ps1`` 必须继续审计这条 compact evidence 行，
     要求 count、status、``checklist_path``、``required_entrypoint_count=3``、
     ``entrypoint_paths``、三个入口的 ``path_display``、marker、
     ``source_schema=featherdoc.release_candidate_summary`` 和 ``source_report`` 保持在
     同一行，且 ``source_schema`` 必须显式保留 release-candidate summary schema 身份，
     ``source_report`` 必须能识别 release-candidate summary 证据源，
     不能用 detached notes 或错误 source report 补齐。固定标记：
     ``project_template_readiness_checklist_entrypoints_release_entry_path_display_trace``、
     ``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``、
     ``project_template_readiness_checklist_entrypoints_release_entry_source_report_identity_trace``。
     ``scripts/package_release_assets.ps1`` 还必须在 staged release materials 阶段强制检查
     三个入口文件都保留该 compact evidence 行，并把通过后的审计结果写入
     ``release_assets_manifest.json`` 的
     ``release_entry_project_template_readiness_checklist_material_safety_audit``，再由
     material-safety 审计该 manifest 字段。固定标记：
     ``project_template_readiness_checklist_entrypoints_packaged_material_safety_trace``。
     ``scripts/build_release_blocker_rollup_report.ps1`` 还必须把该 packaged audit
     展平成 source report contract 字段，至少保留 ``status``、``audit_script``、
     ``audited_entrypoint_count``、``audited_entrypoints``、compact evidence
     label/field/source schema、固定 checklist
     路径和 material-safety marker；其中 ``compact_evidence_source_schema`` 必须固定为
     ``featherdoc.release_candidate_summary``，让打包阶段的入口审计结论可以继续进入
     release blocker rollup，而不是只停留在 ``release_assets_manifest.json``。固定标记：
     ``project_template_readiness_checklist_entrypoints_packaged_audit_rollup_trace``。
     ``release_blocker_rollup.md`` 的 ``Source Report Contracts`` 还必须把该 packaged
     audit 的 status、audit script、audited entrypoints、compact evidence label/field/source
     schema、checklist path/marker 和 material-safety marker 保留在同一个
     ``featherdoc.release_candidate_summary`` 列表块内，不能由 detached notes 或 rollup
     自身 schema 冒充。固定标记：
     ``project_template_readiness_checklist_entrypoints_packaged_audit_rollup_material_safety_trace``。
     ``scripts/build_release_governance_handoff_report.ps1`` 与
     ``scripts/run_release_candidate_checks.ps1`` 还必须把同一 packaged audit 汇总到
     ``release_governance_handoff`` 和 ``steps.release_governance_handoff``，并由
     release metadata helper 展示到 handoff details，避免最终 release summary 看不到
     staged entry materials 的 material-safety 结论。固定标记：
     ``project_template_readiness_checklist_entrypoints_packaged_audit_handoff_trace``。
     ``scripts/assert_release_material_safety.ps1`` 必须继续审计
     ``release_governance_handoff.md``、``final_review.md``、``release_handoff.md`` 与
     三个 release entry 文档中同一个 packaged audit ``source_report`` 列表块，
     要求 audit status、audit script、三个 audited entrypoints、compact evidence
     field、checklist path、checklist marker 和 material-safety marker 保持同块，
     不能用 detached notes 补齐；同块还必须保留
     ``featherdoc.release_candidate_summary`` schema 身份，避免用 rollup 自身 summary
     冒充 packaged audit 的原始来源。固定标记：
     ``project_template_readiness_checklist_entrypoints_packaged_audit_handoff_material_safety_trace``、
     ``project_template_readiness_checklist_entrypoints_packaged_audit_handoff_source_schema_identity_trace``、
     ``project_template_readiness_checklist_entrypoints_packaged_audit_final_review_source_schema_identity_trace``、
     ``project_template_readiness_checklist_entrypoints_packaged_audit_release_metadata_details_source_schema_identity_trace``。
     ``scripts/write_release_metadata_start_here.ps1``、
     ``scripts/write_release_artifact_guide.ps1`` 与
     ``scripts/write_release_reviewer_checklist.ps1`` 还必须把该 packaged audit 压缩成
     ``Project-template readiness checklist packaged audit evidence`` 行，展示到三个最终
     release entry summary 中，让 reviewer 不需要进入内部 handoff 才能看到 staged entry
     materials 已经通过 material-safety 审计。固定标记：
     ``project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_trace``。
     ``scripts/assert_release_material_safety.ps1`` 必须继续审计这条 packaged audit
     compact evidence 行，要求 count、status、audit script、三个 audited entrypoints、
     compact evidence identity、``compact_evidence_source_schema=featherdoc.release_candidate_summary``、
     checklist path、checklist marker、material-safety marker 和
     ``source_schema=featherdoc.release_candidate_summary``、``source_report`` 保持同一行。
     ``compact_evidence_source_schema`` 标识被审计 compact evidence 的原始 schema；
     ``source_schema`` 则必须显式保留当前 source report block 的 release-candidate
     summary schema 身份，``source_report`` 必须能识别 release-blocker rollup 证据源，
     不能用 detached notes 或 release-candidate summary 冒充打包审计来源。
     固定标记：
     ``project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_material_safety_trace``、
     ``project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_source_report_identity_trace``。
   * ``scripts/write_release_body_zh.ps1`` 生成的 ``release_body.zh-CN.md`` 和
     ``release_summary.zh-CN.md`` 必须复用同一批 compact evidence helper，展示
     ``Project-template readiness checklist handoff evidence`` 和
     ``Project-template readiness checklist packaged audit evidence``。release notes
     不能只展示 delivery readiness / onboarding governance 合同摘要，而遗漏固定准入清单
     handoff 和 packaged material-safety audit 结论。固定标记：
     ``project_template_readiness_checklist_entrypoints_release_notes_trace``。
   * ``START_HERE.md``、``ARTIFACT_GUIDE.md`` 与 ``REVIEWER_CHECKLIST.md`` 必须
     在发布 ZIP 刷新步骤前后显式提示 reviewer 打开打包后的
     ``release_assets_manifest.json``，并核对
     ``project_template_delivery_readiness_contract`` 与
     ``project_template_onboarding_governance_contract`` 是否同时携带
     ``status``、``release_ready``、``release_blocker_count``、``warning_count``、
     ``schema_approval_status_summary``、``onboarding_governance_next_action``、
     ``onboarding_governance_next_action_summary``、
     ``onboarding_governance_next_action_group_count``、``next_action``、
     ``next_action_summary``、``next_action_group_count``、``source_report_display``
     和 ``source_json_display``。这条人工签核入口不能
     只依赖 manifest 安全审计或缺失数为 0 的间接推断，首屏入口也不能只把核对项
     留在 reviewer checklist 深处。固定标记：
     ``reviewer_manifest_scoped_project_template_trace``。
   * ``scripts/run_release_candidate_checks.ps1`` 生成的 ``summary.json`` 还必须提供
     ``manifest_signoff_entrypoints`` 机器可消费字段，明确声明
     ``START_HERE.md``、``ARTIFACT_GUIDE.md`` 与 ``REVIEWER_CHECKLIST.md`` 三个入口
     都是必签项，并写出打包后的 ``release_assets_manifest.json``、
     ``required_contracts``、``required_fields`` 和
     ``reviewer_manifest_scoped_project_template_trace``。固定标记：
     ``manifest_signoff_entrypoints_release_trace``。
     ``release_governance_handoff.md`` 中的
     ``manifest_signoff_entrypoints_source_reports`` 必须继续把
     ``status=declared``、打包后的 ``release_assets_manifest.json``、三个必签入口、
     两条 required contracts、13 required fields 和
     ``reviewer_manifest_scoped_project_template_trace`` 保留在同一个
     ``schema=featherdoc.release_candidate_summary`` 的 ``source_report:`` block 中，
     不能由 detached notes 补齐。固定标记：
     ``manifest_signoff_entrypoints_handoff_material_safety_trace``。
     ``release_blocker_rollup.md`` 的 ``Source Report Contracts`` 也必须把同一组
     manifest signoff 字段保留在 ``featherdoc.release_candidate_summary`` 列表块内，
     不能让 release blocker rollup 自身 schema 或 detached notes 冒充原始
     release-candidate signoff 证据。固定标记：
     ``manifest_signoff_entrypoints_rollup_material_safety_trace``。
   * ``scripts/package_release_assets.ps1`` 写出的 ``release_assets_manifest.json`` 必须继续
     保留 ``manifest_signoff_entrypoints``，并由
     ``scripts/assert_release_material_safety.ps1`` 直接审计 ``status=declared``、
     ``required_entrypoint_count=3``、三个人工签核入口的 ``required=true`` /
     ``path_display``、两条 project-template contract、13 required fields 与
     ``reviewer_manifest_scoped_project_template_trace``。这样 packaged asset 层也能
     作为发布准入证据，不能只依赖 release summary。固定标记：
     ``manifest_signoff_entrypoints_manifest_trace``。


推荐轻量验证
------------

资源紧张时，优先运行这些不启动 Word、浏览器、LibreOffice 或完整构建的测试：

.. code-block:: powershell

   powershell -NoProfile -ExecutionPolicy Bypass -File .\test\project_template_release_readiness_checklist_docs_contract_test.ps1 -RepoRoot .
   powershell -NoProfile -ExecutionPolicy Bypass -File .\test\build_project_template_delivery_readiness_report_test.ps1 -RepoRoot . -WorkingDir .\build\project-template-readiness-check -Scenario aggregate
   powershell -NoProfile -ExecutionPolicy Bypass -File .\test\build_content_control_data_binding_governance_report_test.ps1 -RepoRoot . -WorkingDir .\build\content-control-governance-check
   powershell -NoProfile -ExecutionPolicy Bypass -File .\test\release_governance_metrics_contract_test.ps1 -RepoRoot .
   powershell -NoProfile -ExecutionPolicy Bypass -File .\test\release_note_bundle_version_test.ps1 -RepoRoot . -WorkingDir .\build\release-note-bundle-version-check
   powershell -NoProfile -ExecutionPolicy Bypass -File .\test\assert_release_material_safety_test.ps1 -RepoRoot . -WorkingDir .\build\release-material-safety-check

material safety 全量回归在 Windows 上可能较慢；定位 final review 的 PDF visual
证据时，可以先运行聚焦切片，但正式发布路径仍必须跑完整审计：

.. code-block:: powershell

   powershell -NoProfile -ExecutionPolicy Bypass -File .\test\assert_release_material_safety_test.ps1 -RepoRoot . -WorkingDir .\build\release-material-safety-final-review-pdf-check -CasePattern "final-review-pdf-visual"

如果资源允许，再让 ``scripts/run_release_candidate_checks.ps1`` 生成完整 release
candidate summary，并检查 final review、handoff 和 reviewer checklist 中是否能从
``project_template_delivery_readiness`` 与
``content_control_data_binding_governance`` 追溯到原始 JSON。


发布判定
--------

可以发布：

* ``project_template_delivery_readiness`` 的 ``release_ready`` 为 ``true``，或者
  所有 blocker 都有明确 owner、action 和 source display。
* Schema approval history 没有 ``blocked`` gate，且
  ``project_template_approval_matrix`` 中每个 ``project_id`` / ``template_name``
  的最新状态不是 ``blocked``、``pending`` 或 ``rejected``。
* Content-control data-binding blocker 要么已清零，要么公开材料中有稳定修复路径，
  并保留 ``input_docx``、``template_name``、``schema_target`` 与 ``target_mode``
  provenance。
* Release blocker rollup、governance pipeline、handoff 和 reviewer checklist
  都消费了同一批治理证据。
* ``release_summary.zh-CN.md``、``release_governance_handoff.md``、
  ``final_review.md`` 与 ``release_assets_manifest.json`` 都能追溯到
  project-template readiness / onboarding governance 的 schema approval status
  summary、next-action 证据与 source display。

不能发布：

* schema approval 结果缺少 reviewer / reviewed_at 等审计字段。
* generated schema 未经 approval 被当作 baseline。
* ``content_control_data_binding.bound_placeholder`` 出现在公开材料里但缺少修复命令
  或缺少 ``input_docx``、``template_name``、``schema_target``、``target_mode``
  provenance。
* release summary 只有计数，没有 ``source_json_display`` /
  ``source_report_display``。
* reviewer 无法从 release bundle 回到原始治理报告。
