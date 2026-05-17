当前推进方向（中文）
====================

这份文档用于回答一个非常直接的问题：

``FeatherDoc`` 接下来到底继续推进什么功能，什么功能不该抢着做。

这份文档和 :doc:`project_identity_zh`、:doc:`v1_7_roadmap_zh` 的关系是：

- ``project_identity_zh`` 负责说明项目是什么、为什么独立演进
- ``v1_7_roadmap_zh`` 负责记录前一阶段能力补齐的路线
- 本文负责约束当前阶段的产品主线和功能准入标准


一句话结论
----------

``FeatherDoc`` 的目标不是覆盖 ``WordprocessingML`` 的全部表面，
而是成为一个面向正式文档场景的 ``.docx`` 处理、编辑、修改、生成引擎。

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


未来一段时间只推三条能力线
--------------------------

如果一个新功能不属于下面三条能力线之一，它默认不应排到前面。

PDF 方向当前进入保守维护线：PDFio / PDFium 路线继续保留，RTL table
cell normalization 只在出现新的 Unicode class、PDFium raw 形态或用户可见
导入风险时再扩展；近期主力回到发布治理、模板契约、样式编号和版式交付。


一、模板契约与项目模板工作流
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

这是最该继续加强的一条线。

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
  会经 delivery readiness 继续保留
  ``featherdoc.project_template_onboarding_governance_report.v1``、``project_id``、
  ``template_name``、``source_json_display`` 与 reviewer ``open_command``，最终进入
  发布面板和 reviewer-facing bundle；approval history 会按项目、模板和 approval
  item name 生成复合历史，避免同名模板跨项目合并
- schema patch confidence calibration 只读校准报告；pending approval、invalid
  approval record、未打分候选和 recommendation 现在会作为 release blocker /
  warning / action item 进入
  ``schema-patch-confidence-calibration/summary.json``，并被默认发布面板消费；真实业务
  模板 candidate 还会保留 ``project_id``、``template_name`` 和 ``candidate_type``
- release blocker rollup 统一发布阻断汇总
- content-control data-binding governance 只读报告，已把 Custom XML
  同步 issue、绑定占位符和重复绑定复核接入发布治理 pipeline；这些治理项现在会携带
  ``source_schema``、``source_json_display`` 与 action item ``open_command``，
  供发布面板直接展开证据与复核命令
- ``run_project_template_smoke.ps1``

接下来更值得补的是：

1. “真实业务模板 schema patch 置信度校准与复核分流”已完成首轮：发票、合同、制度文档、
   项目报告和交付状态报告 fixture 已进入 calibration 回归，rename / type update /
   remove / required change / add candidate 能随 approval outcome 进入发布治理链路
2. 多项目 schema approval 审计与发布门禁回放已完成首轮回归；后续只在接入真实项目模板
   语料时继续补项目 / 模板维度
3. 继续补齐 release blocker rollup 周边的人工复核分流；onboarding governance、
   confidence calibration 和 content-control data-binding governance 已直接透传
   blocker / warning / action item 明细；release governance pipeline 的 ``stages[]``
   会按 stage 保留 ``source_schema``、``source_report_display``、
   ``source_json_display`` 与 action item 的 ``open_command``，release governance
   handoff 也已把同一组明细同步进 release summary、final review、artifact guide、
   reviewer checklist 和 handoff markdown；release note bundle 入口会拒绝缺少这些
   reviewer-facing 字段的治理明细，方便发布面板先按治理源过滤
4. 下一轮优先做 schema patch 校准阈值建议稳定化：按 ``candidate_type`` 累积更多
   approved / rejected / pending / invalid outcome，给出更可靠的阈值建议和复核分流
5. schema migration 的人工复核入口和更明确的修复建议分流

这条线的目标是：

``一份模板文档可以被接入、校验、生成、回归、发布，而不是只在本地偶然能跑通。``


二、样式与编号治理
^^^^^^^^^^^^^^^^^^

这是第二优先级主线。

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
- 批量 style rename / merge 的非破坏性计划、审计、持久化 JSON plan、受控 apply 与 rollback 记录，并输出 source usage、issue 与 command_template；merge rollback 会捕获被删除 source style XML 与原 source usage hits，restore dry-run 可无输出文件审计恢复计划，且可重复 `--entry` 或用 `--source-style` / `--target-style` 选择 rollback 项，restore issue 会输出可操作 suggestion 与顶层 issue_count / issue_summary，正式 restore 会按 `node_ordinal` 只恢复原 source hits（plan_style_refactor / plan-style-refactor / apply_style_refactor / apply-style-refactor / plan_style_refactor_restore / restore_style_refactor / restore-style-merge --dry-run / restore-style-merge --plan-only / restore-style-merge）
- 重复 custom paragraph / character style 的保守 merge 建议，并输出可审阅 / 可持久化 JSON plan，包含 reason / confidence / evidence / differences 元数据与顶层 suggestion_confidence_summary；CLI 可用 `--source-style` / `--target-style` 收敛具体样式对，再用 `--confidence-profile recommended|strict|review|exploratory` 或 `--min-confidence <0-100>` 过滤更保守的自动化 plan，并可用 `--fail-on-suggestion` 作为 CI gate；XML 对比会忽略 styleId 与显示名（suggest_style_merges / suggest-style-merges）
- 未使用 custom style 的保守 plan / prune，并保护默认 / 内置样式与 basedOn / next / link 依赖（plan_prune_unused_styles / plan-prune-unused-styles / prune_unused_styles / prune-unused-styles）
- numbering catalog 的 CLI JSON import/export
- numbering catalog JSON definition level upsert 与 override 批量 upsert/remove
- numbering catalog JSON lint 结构校验
- numbering catalog JSON check / diff 准入与单文件 / manifest baseline gate
- 多份 document skeleton governance summary 的 rollup 汇总入口，可把
  exemplar catalog、样式编号 issue、release blocker 和 action item 先聚合成
  ``featherdoc.document_skeleton_governance_rollup_report.v1``，再进入统一发布阻断视图

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
4. 在已有骨架治理报告和多文档 rollup 基础上继续强化 exemplar 冲突审计和 catalog patch 衔接

这条线的目标是：

``让下游可以稳定控制文档骨架，而不是生成完后再进 Word 手工修标题和列表。``


三、表格与版式交付能力
^^^^^^^^^^^^^^^^^^^^^^

这是第三优先级主线。

表格、页边距、图片布局、section 结构，决定了这个库能不能真正支撑
报告、发票、制度文件和业务单据这类交付物。

当前这条线已经很强，但还没有完全收口。单文档交付报告已经可以把
table style quality、安全 ``tblLook`` 修复、floating table preset plan 和视觉回归入口
收成 JSON / Markdown；多文档 ``build_table_layout_delivery_rollup_report.ps1``
也可以继续汇总成 ``featherdoc.table_layout_delivery_rollup_report.v1``，
再交给 release blocker rollup。

接下来更值得补的是：

1. 更完整的 custom table style property editing 覆盖面
2. 浮动表格环绕距离、重叠控制和更多 ``w:tblpPr`` 细节
3. 更高层的页面与区段版式组合 helper
4. 围绕“生成后无需人工微调”的交付质量打磨，并让 layout rollup 更稳定地进入发布面板

这条线的目标是：

``生成出来的文档不仅结构正确，而且版式足够接近最终交付状态。``


当前明确不优先的事项
--------------------

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

当前更合理的节奏不是“想到什么补什么”，而是：

1. release governance handoff 明细一致性和多项目 schema approval 定位已完成首轮收口
2. 接下来把模板契约链路继续做实，优先校准真实业务模板的 schema patch 置信度与复核分流
3. 再把样式 / 编号治理做深
4. 最后把表格 / 版式交付能力收口

只有当这些主线已经明显稳定之后，再考虑更重、更宽的能力面。


建议的对外表述
--------------

如果要用一句话向别人介绍当前项目，可以直接说：

``FeatherDoc`` 是一个面向正式文档场景的现代 C++ ``.docx`` 引擎，
重点覆盖文档处理、结构化编辑、模板生成与结果验证。
