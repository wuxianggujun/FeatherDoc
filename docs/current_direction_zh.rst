当前推进方向（中文）
====================

这份文档用于回答一个非常直接的问题：

``FeatherDoc`` 接下来到底继续推进什么功能，什么功能不该抢着做。

这份文档和 :doc:`project_identity_zh`、:doc:`next_tasks_zh`、
:doc:`long_task_board_zh`、历史路线图 :doc:`v1_7_roadmap_zh` 的关系是：

- ``project_identity_zh`` 负责说明项目是什么、为什么独立演进
- ``next_tasks_zh`` 负责把当前路线拆成可执行 backlog，并记录下一步最小动作
- ``long_task_board_zh`` 负责当前 long-running goal 的逐轮执行台账
- ``v1_7_roadmap_zh`` 只作为前一阶段能力补齐路线的历史归档
- 本文负责约束当前阶段的产品主线和功能准入标准


一句话结论
----------

``FeatherDoc`` 的目标不是覆盖 ``WordprocessingML`` 的全部表面，
而是成为一个面向正式文档场景的 ``.docx`` 处理、编辑、修改、生成引擎。

当前收口决策（2026-08-26）
--------------------------

DOCX/Word 主线已经完成正式文档处理所需的核心闭环。本轮完成
``Table::append_row()`` 的结构事务收尾后，项目进入 **Word-only maintenance mode**：

- 冻结新的大功能和低频 WordprocessingML API 扩张；
- 只接受可复现的 bug、安全、兼容性、构建、测试和文档修复；
- 继续维护 DOCX reopen-save、模板、表格结构和句柄生命周期回归；
- PDF 保持实验性 opt-in，只做必要维护，不再作为近期功能主线；
- 新需求若要重新开启功能开发，必须先给出真实业务场景、兼容性边界和可执行验证证据。

下文的能力线和 backlog 主要用于说明现有能力、维护入口和历史决策，
不再自动构成新的功能承诺。

这里说的“正式文档场景”，主要指：

- 报告、合同、制度文件、发票、标书、通知、函件
- 以模板为基础批量生成的业务文档
- 需要 reopen-save 持续编辑，而不是一次性导出的文档
- 需要用真实 ``Microsoft Word`` 渲染结果做验证的交付型文档


当前主线工作流
--------------

后续功能是否值得继续推进，优先看它是否强化下面四条主线工作流。

1. 打开并诊断既有文档

   - 能稳定 ``open()``
   - 能给出结构化错误诊断
   - 能在 body、header、footer、section part 之间定位目标内容

2. 按结构编辑与修改文档

   - 段落、Run、表格、图片、分节、页眉页脚
   - 样式、编号、页面设置、字段、模板槽位
   - 尽量通过 typed API 操作，而不是让下游自己拼 XML

3. 从模板或空文档生成可交付文档

   - 支持 ``create_empty()``
   - 支持书签模板、模板表格、模板 schema
   - 支持围绕正式文档版式的高频生成动作

4. 对结果做可复现验证

   - reopen-save 回归
   - CLI inspection / mutation
   - schema baseline、project template smoke
   - 基于真实 ``Word`` 的 visual validation


当前能力应如何理解
------------------

结合公开 API、CLI、样例、测试和脚本，当前仓库已经不只是一个“能读写段落”的小库。

它已经具备下面这组可闭环能力：

- ``.docx`` 的打开、保存、另存为、从空文档创建与诊断
- 段落 / Run / 表格 / 图片 / 分节 / 页眉页脚的结构化编辑
- 样式目录检查、样式定义编辑、样式继承检查、usage report 与受控重构
- 托管列表、自定义编号定义、样式挂接编号、numbering catalog JSON 治理
- 书签模板填充、content control 纯文本与富内容替换、模板表格扩展、block 显隐
- 文档级 template schema 校验、扫描、patch、baseline gate 与审批摘要
- 页面设置、通用字段（含 TOC / REF / SEQ 与页码字段）、表格样式定义与第一版浮动表格定位
- CLI 驱动的 inspection / mutation 流程
- 基于 ``Microsoft Word`` 的截图级可视化回归

因此，当前阶段不应该再把 ``FeatherDoc`` 理解成“一个泛用 XML 包装层”，
而应该明确理解成：

``正式文档处理链路里的核心引擎层``。


已完成主线与维护入口
--------------------

下面三条能力线描述当前产品边界。进入维护模式后，只维护它们已有的稳定入口，
不再以扩展覆盖面为目标。


一、模板契约与项目模板工作流
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

这是历史上最值得加强、当前进入维护的能力线。

原因很简单：正式文档生成的核心不是“把字写进去”，而是把模板约束、
生成输入、输出结果和回归检查连成闭环。

这条线已经有了不错基础：

- ``validate_template(...)``
- ``validate_template_schema(...)``
- ``export-template-schema``（已覆盖书签与 content control slot）
- ``normalize-template-schema``
- ``diff-template-schema``
- ``template_schema_patch`` / ``apply_template_schema_patch(...)`` /
  ``build_template_schema_patch(...)``
- ``scan_template_schema(...)`` / ``build_template_schema_patch_from_scan(...)``
- ``check-template-schema``
- schema patch review JSON、schema approval gate 与审批历史报表
- project-template onboarding governance 聚合报告；其 blocker 和 action item
  会经 delivery readiness 继续保留，entry 级 ``next_action.reason`` /
  ``next_action.blocker_id``、顶层全局 ``next_action`` 和
  ``next_action_summary`` 分组也会进入聚合视图，
  ``featherdoc.project_template_onboarding_governance_report.v1``、
  ``source_json_display`` 与 reviewer ``open_command``，最终进入发布面板和
  reviewer-facing bundle
- schema patch confidence calibration 只读校准报告；pending approval、未打分候选和
  recommendation 现在会作为 release blocker / warning / action item 进入
  ``schema-patch-confidence-calibration/summary.json``，并被默认发布面板消费；
  报告同时保留 ``business_template_corpus_summary``，让 reviewer 能看到候选项来自
  哪些项目、模板和 source JSON，并对缺少 project/template/source summary、
  ``business_document_type`` 或 ``corpus_role`` 的语料分别输出
  ``add_business_template_source_metadata`` /
  ``add_business_template_document_type_metadata`` /
  ``add_business_template_corpus_role_metadata`` 动作；若候选上的
  ``business_document_type`` / ``corpus_role`` 与来源语料条目不一致，则输出
  ``align_business_template_corpus_metadata`` 动作；
  reviewer 分流约定为先用 ``source_report_display`` 打开 Markdown 报告，再用
  ``source_json_display`` 核对机器证据，最后复制 ``open_command`` 重新生成或复核校准材料
- release blocker rollup 统一发布阻断汇总
- content-control data-binding governance 只读报告，已把 Custom XML
  同步 issue、绑定占位符和重复绑定复核接入发布治理 pipeline；这些治理项现在会携带
  ``source_schema``、``source_json_display`` 与 action item ``open_command``，
  供发布面板直接展开证据与复核命令
- ``check_docx_functional_smoke_readiness.ps1`` 作为低资源 DOCX 功能 smoke
  只读入口，统一确认样本 DOCX 包完整性、段落/表格/图片/section/header/footer/
  content-control/字段/模板渲染证据，以及复用 Word visual smoke PNG 非空证据
- ``run_project_template_smoke.ps1``
- project-template workflow dashboard 会把 onboarding governance 与 delivery readiness
  的 ``release_ready``、blocker、warning、``source_report_count`` 与
  ``next_action`` 固定成 ``featherdoc.project_template_workflow_dashboard.v1``，
  并在 release candidate preflight 中通过
  ``project_template_workflow_dashboard_report`` 进入 ``summary.json``、
  ``final_review.md`` 和发布面板；``START_HERE.md``、``ARTIFACT_GUIDE.md`` 与
  ``REVIEWER_CHECKLIST.md`` 也会展示 dashboard status、release_ready、blocker /
  warning 计数、证据路径和下一步动作，避免 reviewer 只从 handoff 计数反推项目模板状态

历史优先项（维护模式前）：

1. 扩大真实业务模板语料样本，继续校准 rename / update 建议的置信度
2. 多项目 schema approval、release gate 和审批历史的维护体验
3. 继续补齐 release blocker rollup 周边的人工复核分流；onboarding governance、
   confidence calibration 和 content-control data-binding governance 已开始直接透传
   blocker / warning / action item 明细；release governance pipeline 的 ``stages[]``
   也会按 stage 保留 ``source_schema``、``source_report_display``、
   ``source_json_display`` 与 action item 的 ``open_command``，release governance
   handoff 也会把同一组 blocker / warning / action item 明细同步进 release summary、
   final review、bundle 和 reviewer checklist，方便发布面板先按治理源过滤
4. schema migration 的人工复核入口和更明确的修复建议分流

这条线的目标是：

``一份模板文档可以被接入、校验、生成、回归、发布，而不是只在本地偶然能跑通。``


二、样式与编号治理
^^^^^^^^^^^^^^^^^^

这是历史上的第二优先级能力线，当前进入维护。

正式文档能不能稳定生成，很大程度上不取决于文本写入本身，
而取决于标题层级、目录来源、列表体系、语言字体继承是否可控。

这条线当前已经覆盖：

- 样式目录 inspection
- paragraph / character style definition 与 table style region definition
- style inheritance inspection
- 单样式 / 全量 style usage report（find_style_usage / list_style_usage / inspect-styles --usage）
- style run property materialization
- paragraph / character style rebase
- 自定义编号定义
- paragraph style numbering
- 多样式共享 outline numbering
- style numbering 的 CLI 只读盘点、审计 gate、command_template 修复建议（缺失 level 指向 upsert_levels patch）、plan/apply 安全清理、based-on 对齐、唯一同名 definition relink 和 catalog 导入预修复入口（inspect-style-numbering / audit-style-numbering / repair-style-numbering）
- style id 重命名，并同步 body / header / footer 内 paragraph、run、table 引用（rename_style / rename-style）
- 同类型 style merge，并同步引用后移除源样式（merge_style / merge-style）
- 批量 style rename / merge 的非破坏性计划、审计、持久化 JSON plan、受控 apply 与 rollback 记录，并输出 source usage、issue 与 command_template；merge rollback 会捕获被删除 source style XML 与原 source usage hits，restore dry-run 可无输出文件审计恢复计划，且可重复 `--entry` 或用 `--source-style` / `--target-style` 选择 rollback 项，restore 审计会输出 `selection_summary`、`requested_count`、`review_handoff_step_count`、带 `copy_command`、`source_schema` / `source_report_display` / `source_json_display` / `rollback_plan_display` 的 `review_handoff_steps`、`next_handoff_step`、`next_copy_command`、`next_step_reason`、顶层 `issue_review_commands` / `issue_review_command_count` / `issue_review_group_summary` / `first_issue_review_command` / `copy_issue_review_command`、`handoff_status_summary`、`rollback_plan_summary`、`issue_summary_group_count`、`issue_summary_groups`、带 `first_review_command` / `copy_review_command` 的 `issue_review_groups`、`status_reason`、`minimum_risk_next_action`、`minimum_risk_next_action_command`、`restorable_rollback_command_summary`、`selected_restore_command_template`、`per_entry_dry_run_commands`、`per_entry_restore_command_templates`、`batch_restorable_dry_run_command` 与 `batch_restorable_restore_command_template`，release governance action / blocker 会携带 `source_schema`、`source_report_display`、`source_json_display`、`repair_strategy`、同一份 `review_handoff_steps`、`next_handoff_step`、`next_copy_command`、`next_step_reason`、`issue_review_commands`、`issue_review_command_count`、`issue_review_group_summary`、`first_issue_review_command`、`copy_issue_review_command`、`handoff_status_summary` 与 `rollback_plan_summary`，restore issue 会输出可操作 suggestion 与顶层 issue_count / issue_summary，正式 restore 会按 `node_ordinal` 只恢复原 source hits（plan_style_refactor / plan-style-refactor / apply_style_refactor / apply-style-refactor / plan_style_refactor_restore / restore_style_refactor / restore-style-merge --dry-run / restore-style-merge --plan-only / restore-style-merge）
- 重复 custom paragraph / character style 的保守 merge 建议，并输出可审阅 / 可持久化 JSON plan，包含 reason / confidence / evidence / differences 元数据与顶层 suggestion_confidence_summary；CLI 可用 `--source-style` / `--target-style` 收敛具体样式对，再用 `--confidence-profile recommended|strict|review|exploratory` 或 `--min-confidence <0-100>` 过滤更保守的自动化 plan，并可用 `--fail-on-suggestion` 作为 CI gate；XML 对比会忽略 styleId 与显示名（suggest_style_merges / suggest-style-merges）
- 未使用 custom style 的保守 plan / prune，并保护默认 / 内置样式与 basedOn / next / link 依赖（plan_prune_unused_styles / plan-prune-unused-styles / prune_unused_styles / prune-unused-styles）
- numbering catalog 的 CLI JSON import/export
- numbering catalog JSON definition level upsert 与 override 批量 upsert/remove
- numbering catalog JSON lint 结构校验
- numbering catalog JSON check / diff 准入与单文件 / manifest baseline gate
- exemplar catalog 来源冲突的结构化 patch plan：
  ``featherdoc.numbering_catalog_governance_patch_plan.v1`` 通过顶层
  ``catalog_patch_plan_count`` / ``catalog_patch_plans`` 与冲突项的
  ``catalog_patch_plan_id`` / ``catalog_patch_plan`` 暴露。计划固定为
  ``awaiting_authoritative_catalog``，并明确 ``safe_to_apply=false``、
  ``automatic_patch_available=false``、``patch_apply_supported=false``、
  ``manual_review_required=true`` 与
  ``requires_authoritative_catalog_selection=true``。
- patch plan 的 ``supported_patch_operations`` 只支持 ``upsert_levels`` / ``upsert_overrides`` /
  ``remove_overrides``；``definition_topology_changes`` /
  ``instance_topology_changes`` 保留在 ``unsupported_automatic_changes``，不做自动合并。候选
  catalog 通过 ``candidate_catalog_count`` / ``candidate_catalog_paths`` /
  ``candidate_catalog_displays`` 暴露，
  ``reviewer_inputs``、``patch_counts``、``diff_commands`` / ``review_command``、
  ``patch_command_template``、``lint_command_template``、
  ``verification_command_template`` 与有序 ``required_steps`` 会随
  ``catalog_patch_plan`` 进入下游 rollup 和 release governance handoff。
- 多份 document skeleton governance summary 的 rollup 汇总入口，可把
  exemplar catalog、样式编号 issue、release blocker 和 action item 先聚合成
  ``featherdoc.document_skeleton_governance_rollup_report.v1``，再进入统一发布阻断视图。
  单文档入口为 ``build_document_skeleton_governance_report.ps1``，
  汇总入口为 ``build_document_skeleton_governance_rollup_report.ps1``，默认写入
  ``output/document-skeleton-governance`` 与
  ``output/document-skeleton-governance-rollup/summary.json``。

  这一层现在不应只看聚合计数。发布面板会从 final release blocker rollup 中继续展示
  ``release_blocker_rollup.release_blockers``、``warnings`` 与 ``action_items`` 的明细，
  包括 ``id``、``action``、``message``、``open_command``、``source_schema``、
  ``source_report_display`` 和 ``source_json_display``。reviewer 可以直接从
  ``START_HERE.md``、``ARTIFACT_GUIDE.md``、``REVIEWER_CHECKLIST.md`` 与
  ``release_handoff.md`` 定位骨架治理证据和下一步命令。pipeline summary 的
  ``numbering_catalog_governance`` stage 也会保留同一组明细字段，避免骨架治理项只停留在
  stage 计数里；release governance handoff 会把这些明细继续同步到 handoff summary、
  final review 和 reviewer checklist，避免发布面板只能看到 handoff 计数。

接下来最值得补的是：

1. merge restore 的更完整冲突处理与基于真实语料的样式建议置信度校准
2. 面向 heading / list / theme 的稳定重构入口
3. 样式与编号之间更明确的批量治理 mutation API
4. exemplar catalog 来源冲突审计及结构化 catalog patch plan 已接入 numbering
   governance；下一步仍由 reviewer 选择唯一 authoritative catalog，人工编写只包含
   受支持 operation 的 reviewed patch，再按 ``required_steps`` 完成 apply、lint 与
   ``--fail-on-diff`` 验证

这条线的目标是：

``让下游可以稳定控制文档骨架，而不是生成完后再进 Word 手工修标题和列表。``


三、表格与版式交付能力
^^^^^^^^^^^^^^^^^^^^^^

这是历史上的第三优先级能力线，当前进入维护。

表格、页边距、图片布局、section 结构，决定了这个库能不能真正支撑
报告、发票、制度文件和业务单据这类交付物。

当前这条线已经很强，但还没有完全收口。单文档交付报告已经可以把
table style quality、安全 ``tblLook`` 修复、floating table preset plan 和视觉回归入口
收成 JSON / Markdown；多文档 ``build_table_layout_delivery_rollup_report.ps1``
也可以继续汇总成 ``featherdoc.table_layout_delivery_rollup_report.v1``，
再交给 release blocker rollup。PDF 浮动表格仍按
``stable_pdf_geometry_subset_not_full_word_wrapping`` 管理：稳定几何子集可以进入
``pdf_floating_table_support_coverage``，但 ``metadata-only tblpPr``、完整 Word 环绕、
重叠避让和 inside/outside page-side 语义必须继续进入 reviewer 视觉复核。

历史优先项（维护模式前）：

1. 更完整的 custom table style property editing 覆盖面
2. 浮动表格环绕距离、重叠控制和更多 ``w:tblpPr`` 细节
3. 更高层的页面与区段版式组合 helper
4. 围绕“生成后无需人工微调”的交付质量打磨，并让 layout rollup 更稳定地进入发布面板

这条线的目标是：

``生成出来的文档不仅结构正确，而且版式足够接近最终交付状态。``


当前明确不优先的事项
--------------------

本节作为 ``current_non_priority_guardrails`` 契约锚点，防止后续维护把低 ROI
能力重新排到当前三条主线之前。

至少在当前阶段，下面这些方向不该抢在前面：

1. 加密或密码保护 ``.docx`` 的支持
2. 完整的 ``OMML`` 公式构造器
3. 批注、修订、审阅痕迹的完整 authoring API
4. content control 的复杂表单系统和全量表单状态 API
5. 纯粹为了命令数量好看而继续堆 CLI 子命令
6. 为历史兼容性长期保留低价值旧接口

这些能力不是永远不做，而是当前 ROI 明显低于前三条主线。


功能准入标准
------------

本节作为 ``current_feature_admission_criteria`` 契约锚点，约束后续新增能力先说明
适用场景、边界和验证入口。

后续判断一个功能值不值得继续做，建议至少过下面六条检查。

1. 它是否明显强化了“处理、编辑、修改、生成、验证”闭环中的某一环
2. 它是否属于正式文档场景里的高频需求，而不是一次性边角需求
3. 它是否优先提供 typed API，而不是先做一个大而全 CLI 包装
4. 它是否有清晰作用边界，能说明自己覆盖 body、header、footer、section 的哪一层
5. 它如果影响版式，是否能补 reopen-save 回归和 ``Word`` visual validation
6. 它是否会把项目重新拖回“做一个什么都想支持的 Word 杂货库”

如果答案大多是否定的，这个功能就不该排进近期主线。


维护节奏建议
------------

本节作为 ``current_maintenance_cadence`` 契约锚点，约束后续以问题修复为主，
不再自动恢复功能扩张。

当前更合理的节奏不是“想到什么补什么”，而是：

1. 优先修复影响 DOCX 正确性、安全性和兼容性的可复现问题
2. 维护模板契约、样式 / 编号治理和表格 / 版式交付的现有验证入口
3. 只在明确的发布或用户场景需要时运行较重的 Word visual / release gate

不再以继续扩大功能面作为项目完成条件；任何扩展提案都必须先经过新的范围评审。


路线维护守护点
--------------

本节也作为 ``current_direction_guardrails`` 契约锚点，约束后续维护时不要把
``template_contract_project_template_workflow``、
``style_numbering_governance_workflow`` 和
``table_layout_delivery_workflow`` 三条主线打散。

后续新增能力、脚本或发布治理入口时，应先把它归入本文三条能力线之一：
模板契约与项目模板工作流、样式与编号治理、表格与版式交付能力。
如果暂时无法归类，需要在变更说明中写清功能准入依据、验证证据，以及
为什么它不属于“当前明确不优先的事项”。

每次调整本文时，至少同步检查下面三件事：

1. 三条能力线标题仍保留，并能对应 ``docs/script_task_index_zh.rst`` 的脚本分类
2. 功能准入标准、当前明确不优先事项和维护节奏建议仍被契约测试覆盖
3. 影响版式或发布治理的变更必须说明 ``reopen-save``、``Word`` visual validation
   或轻量只读 gate 的验证入口


建议的对外表述
--------------

如果要用一句话向别人介绍当前项目，可以直接说：

``FeatherDoc`` 是一个面向正式文档场景的现代 C++ ``.docx`` 引擎，
重点覆盖文档处理、结构化编辑、模板生成与结果验证。
