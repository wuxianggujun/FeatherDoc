后续任务清单（中文）
====================

状态日期：2026-06-18

本页是当前长任务的可执行 backlog。它承接
:doc:`current_direction_zh` 的三条主线，但比路线说明更具体：每个任务都要能落到
脚本、源码、文档、测试或发布治理材料上。
长期逐轮执行台账见 :doc:`long_task_board_zh`，本页负责保留完整 backlog。

任务清单维护 marker：``next_tasks_zh``。


执行原则
--------

1. 只在 ``dev`` 分支推进开发；不创建 ``codex/*`` 开发分支。
2. 每个任务先做最小可验证闭环，再扩展覆盖面。
3. 代码、脚本、文档和测试要一起推进；不能只补说明不补契约。
4. 发布治理相关改动必须能进入 reviewer-facing bundle，至少覆盖
   ``START_HERE.md``、``ARTIFACT_GUIDE.md`` 或 ``REVIEWER_CHECKLIST.md``。
5. Windows / PowerShell / 中文输出默认按 UTF-8 处理，避免重新引入乱码。
6. 任何重型 Word、PDF、CMake 或完整 CTest 验证，都必须在工作区干净且改动已推送后执行。


P0：当前发布与 CI 守护
----------------------

这些任务优先级最高，目标是保证 ``dev`` 可持续推进。

1. 跟踪当前 ``dev`` 最新提交的 GitHub Actions：

   * Docs Pages 必须保持绿色。
   * Linux CMake CI、macOS CMake CI、Windows MSVC CI 若失败，先抓日志定位。
   * Windows MSVC CI 仍是最高风险入口，因为它同时覆盖 MSVC、PowerShell、UTF-8 和发布资产预览。

2. 保持分支策略：

   * 本地只用 ``dev`` 做开发。
   * 定期确认本地和远端不存在 ``codex/*`` 分支。
   * 固定用 ``git branch --list "codex/*"`` 和
     ``git branch -r --list "origin/codex/*"`` 复查临时开发分支。
   * 不做 ``git reset --hard``、强推或整分支回滚。

3. 保持轻量验证基线：

   * ``git status --short --branch``
   * ``git diff --check``
   * 与当前改动直接相关的 PowerShell 契约测试
   * 必要时检查 ``gh run list --branch dev`` 的最新 CI 状态


P1：模板契约与项目模板工作流
----------------------------

目标：让真实业务模板可以被接入、校验、生成、回归和发布。

1. 扩大真实业务模板语料样本：

   * 增加合同、制度文件、发票、报告、通知、标书等典型 ``.docx`` / ``.dotx`` 样本。
   * 保持样本进入 project-template smoke manifest，而不是散落在本地输出目录。
   * 继续校准 schema patch、style rename / merge、content-control 修复建议的置信度。

2. 强化 project-template workflow dashboard：

   * 继续把 onboarding governance 与 delivery readiness 的
     ``release_ready``、blocker、warning、``next_action`` 串到 dashboard。
   * 在 reviewer-facing bundle 中保持 dashboard status、release_ready、计数、
     证据路径和下一步命令可见。
   * 后续可补 ``next_action_summary`` 的分组展示，避免多项 schema approval
     或多模板阻断只显示第一条 action。

3. 多项目 schema approval 维护体验：

   * 让 schema approval history 更适合多项目、多模板、多轮审批。
   * 增强 pending / rejected / approved 状态的人工复核入口。
   * 保持 ``project_template_approval_matrix`` 可按 ``project_id`` /
     ``template_name`` 展示最新状态、历史阻断和 reviewer action。
   * ``project_template_approval_matrix`` 已增加 ``reviewer_action_summary``、
     ``reviewer_action_reason`` 和 ``reviewer_actions``，并用 pending /
     rejected fixture 覆盖人工复核入口。
   * ``reviewer_action_*`` 字段已从 approval history 接入 delivery readiness、
     release blocker rollup 和 release governance handoff。
   * 同一组 reviewer action 字段已纳入 release note bundle 入口材料，
     ``REVIEWER_CHECKLIST.md`` 会输出 reviewer action、原因和候选动作，
     release note bundle 回归会同时验证 material safety 审计。

4. schema migration 人工复核入口：

   * 为 schema patch / migration 输出更明确的修复建议。
   * 区分自动可修复、需人工确认、必须阻断发布三类动作。
   * 对 content-control / Custom XML repair plan，保留
     ``repair_action_class_summary`` 和 ``repair_action_classes``，并让每类动作都能追溯到
     ``source_report_display``、``source_json_display`` 和 ``command_template``。
   * 保持 ``source_schema``、``source_report_display``、``source_json_display`` 和
     ``open_command`` 一路透传到发布材料。


P1：Release governance 与发布材料一致性
----------------------------------------

目标：让发布面板从机器证据到人工 checklist 都能直接定位阻断来源。

1. 继续补齐 release blocker rollup 周边的人工作业分流：

   * blocker / warning / action item 必须带 source 和 command。
   * content-control ``repair_action_classes`` 必须保持从 governance report 到
     release blocker rollup、handoff、bundle、reviewer checklist 的同名透传。
   * final review、handoff、bundle 和 reviewer checklist 应显示同一组治理明细。
   * 避免只保留汇总计数，丢失具体阻断项。
   * schema approval history 的 ``reviewer_action_summary``、
     ``reviewer_action_reason`` 和 ``reviewer_actions`` 已进入 rollup 与 handoff；
     后续 bundle/checklist 改动必须继续保留。

2. 加强发布资产 contract：

   * ``release_assets_manifest.json`` 继续保留 project-template governance contracts。
   * ``requires_reviewer_action``、``reviewer_action_summary``、
     ``reviewer_action_reason`` 和 ``reviewer_actions`` 已纳入
     ``release_assets_manifest.json`` 的 delivery readiness / onboarding governance
     contract，并进入 manifest signoff 必备字段。
   * GitHub Release refresh / publish 前必须能从 manifest 复核 release readiness。
   * 资产上传、release notes 同步和正式发布保持分步可控。

3. 维护 release material safety：

   * 新增发布材料字段时同步补 ``assert_release_material_safety`` 覆盖。
   * 如果字段只出现在 detached notes 而没有进入入口材料，应视为不完整。
   * 重点保护 ``START_HERE.md``、``ARTIFACT_GUIDE.md``、``REVIEWER_CHECKLIST.md``。
   * ``repair_action_classes`` 已纳入 release material safety 静态/样例断言，
     后续入口材料重构时必须继续保留同名字段和三类 action marker。
   * content-control ``action=sync_or_fill_bound_content_control`` 已纳入
     JSON、entrypoint 和 release asset manifest 合同。
   * release note bundle 四份入口文档已通过统一 helper 断言
     ``action=sync_or_fill_bound_content_control`` 和
     ``action=review_duplicate_content_control_binding``，防止 reviewer-facing
     文档丢失可分流处理的 action id。
   * release note bundle fixture 已补 ``repair_action_classes``，统一 helper
     现在要求四份入口文档保留 content-control blocker 的
     ``release_blocking`` / ``auto_repair_candidate`` /
     ``manual_confirmation_required`` 以及 duplicate-binding action item 的
     ``manual_confirmation_required``。
   * ``REVIEWER_CHECKLIST.md`` 已补 content-control blocker 与
     duplicate-binding action item 的精确 source / command guidance 断言，
     包括 source report、source JSON、action、open command 和
     command template。
   * schema approval ``reviewer_action_*`` 已进入 ``release_handoff.md``、
     ``ARTIFACT_GUIDE.md``、``REVIEWER_CHECKLIST.md`` 和 ``START_HERE.md``，
     并由 release note bundle fixture 断言入口材料与 material safety 审计。
   * release asset manifest 已开始承接 schema approval reviewer action 字段，
     ``publish_github_release.ps1`` 也已补上传后 manifest 刷新回归，确保
     ``upload.remote_assets`` 只记录 manifest assets 中的正式 ZIP，并保留远端
     URL、大小和下载计数。
   * release asset manifest 的 material safety 负例已补到
     ``reviewer_action_reason`` 和 ``reviewer_actions``，避免只用
     ``reviewer_action_summary`` 代表整组 reviewer action 字段。
   * package release assets 的三份入口材料已补 ``reviewer_action_*`` 值级断言，
     要求 readiness / onboarding 的 no-action 语义与对应 contract 出现在同一条
     reviewer-facing 材料里。
   * release note bundle 现在会逐行断言 ``START_HERE.md``、
     ``ARTIFACT_GUIDE.md`` 和 ``REVIEWER_CHECKLIST.md`` 的 manifest signoff
     同时包含 ``status``、``release_ready``、``reviewer_action_*``、
     ``source_report_display`` 和 ``source_json_display`` 等完整必备字段；
     三个入口生成脚本也由文档契约测试锁定同一字段集合。
   * ``release_body.zh-CN.md`` 和 ``release_summary.zh-CN.md`` 的
     project-template governance contract 摘要已从 blocker/action item 证据聚合
     ``requires_reviewer_action``、``reviewer_action_summary``、
     ``reviewer_action_reason`` 和 ``reviewer_actions``，避免 release notes
     只展示 readiness 状态而丢失人工复核动作。
   * 文档契约测试已锁定 ``write_release_body_zh_summary.ps1`` 的 reviewer action
     聚合 helper，以及 release note bundle 对上述字段值的断言，避免后续重构只保留
     release notes 文本而丢失源级聚合路径。
   * reviewer action 聚合路径还会被源级契约约束为同时扫描 blocker、action item
     和 warning，并按 ``report_id`` / ``source_schema`` 关联证据，避免只从某个
     偶然存在的 blocker 取值。
   * GitHub Release refresh / publish workflow artifact 输出已补契约检查，
     锁定 ``RELEASE_OUTPUT_ROOT``、``publish_github_release.ps1`` 调用、
     ``release-refresh-output`` / ``release-publish-output`` 以及
     ``output/release-assets/**`` 上传路径。
   * GitHub Release refresh / publish workflow 现在会在调用发布脚本前只清理
     workspace 内的 ``RELEASE_OUTPUT_ROOT``，避免 self-hosted runner 旧
     ``output/release-assets`` 残留混入本轮 release artifact。
   * 下一步等待当前 ``dev`` 最新 CI 全绿；若失败则先修 CI，否则继续推进
     release blocker rollup 与 reviewer bundle 的治理证据一致性。


P2：样式与编号治理
------------------

目标：让下游可以稳定控制文档骨架，而不是生成后手工修标题和列表。

1. 完善 merge restore 冲突处理：

   * 补齐同名样式、缺失 source style、节点序号漂移、部分恢复失败的审计路径。
   * 保持 dry-run、plan-only、正式 restore 三种模式输出一致的 review handoff。
   * 让 release governance 能直接展示 restore issue 的最小风险动作。

2. 基于真实语料校准样式建议置信度：

   * 对 style merge / rename suggestion 增加更多真实文档验证。
   * 继续收敛 ``recommended``、``strict``、``review``、``exploratory`` profile。
   * 对低置信度建议输出人工复核原因，而不是直接进入自动修复。

3. 面向 heading / list / theme 的稳定重构入口：

   * 把标题层级、列表体系、主题字体和语言继承纳入更明确的 mutation API。
   * 继续保护 builtin/default style、basedOn、next、link 依赖。
   * 保持 numbering catalog 与 style numbering 的双向衔接。

4. 强化 document skeleton governance：

   * exemplar catalog 冲突审计。
   * numbering catalog patch 衔接。
   * 多文档 rollup 中保留 per-document source 与 action。


P2：表格与版式交付能力
----------------------

目标：让正式文档中的表格和页面布局可被稳定交付。

1. table layout delivery governance：

   * 继续保留 ``ready_document_percent``、``unresolved_item_count``、
     ``table_position_review_count`` 等 release-facing 指标。
   * 对 ``tblLook``、表格样式和固定布局差异输出更直接的修复建议。

2. floating table 与 PDF-sensitive 布局：

   * 继续区分 metadata-only 支持与真实 Word-compatible 布局支持。
   * 对 leftFromText、rightFromText、topFromText、tblOverlap 等字段保留 reviewer focus。
   * 不把 PDF 视觉 baseline 大量搬回主线；先做受控小样本验证。

3. section / page setup / page number fields：

   * 保持 Word visual review task 可打开、可复核、可同步 verdict。
   * 涉及页面方向、页边距、页码字段和 header/footer 的变更必须有轻量 gate。


P2：Word 视觉验证与 DOCX smoke
------------------------------

目标：在不强制每次跑重型 Word gate 的前提下，保留可复现的视觉质量证据。

1. 继续维护 ``check_docx_functional_smoke_readiness.ps1``：

   * 只读确认样本 DOCX 包完整性。
   * 覆盖段落、表格、图片、section、header/footer、content-control、字段和模板渲染证据。
   * 复用 Word visual smoke PNG 非空证据。

2. Word release gate 前置检查：

   * 继续用 ``check_word_visual_release_gate_preflight.ps1`` 保护静态脚本、
     helper、CMake 注册和文档入口。
   * 只有在资源稳定、工作区干净、源码已推送时再跑完整 Word visual gate。

3. review task 维护：

   * stale task audit 保持可运行。
   * latest task pointer 与 generated release materials 保持一致。
   * curated visual regression bundle 必须能从发布材料直接打开。


P3：PDF 保守维护
----------------

目标：保持 PDF import/export 当前能力可验证，不把 PDF 扩成当前主线。

1. PDF CJK 与 copy/search：

   * 保留 CJK manifest、copy/search 和 missing text 计数。
   * 不引入大批旧分支 PDF 样例或 baseline。

2. PDF visual gate：

   * 优先维护 preflight、bounded CTest 和小样本 visual evidence。
   * 完整 PDF visual gate 继续后置，只在 release readiness 需要时运行。

3. PDF build options：

   * 继续明确 ``FEATHERDOC_BUILD_PDF`` 与 ``FEATHERDOC_BUILD_PDF_IMPORT`` 状态。
   * 缺依赖时给出可操作修复命令，而不是让 reviewer 猜 CMake 选项。


P3：文档、测试与索引治理
------------------------

目标：让新增能力不会脱离文档和脚本索引。

1. 文档入口：

   * ``docs/current_direction_zh.rst`` 负责产品主线。
   * ``docs/next_tasks_zh.rst`` 负责可执行 backlog。
   * ``docs/long_task_board_zh.rst`` 负责长任务逐轮推进台账。
   * ``docs/documentation_maintenance_zh.rst`` 负责维护边界。
   * ``docs/script_task_index_zh.rst`` 负责脚本入口。

2. 契约测试：

   * 新增脚本时同步更新 script task index 和对应契约测试。
   * 新增发布材料字段时同步更新 release note bundle 测试。
   * 新增路线任务时同步更新当前任务文档或维护文档契约。

3. 变更节奏：

   * 小步提交，小步推送。
   * CI 失败先修失败，不继续堆叠大改动。
   * 每次阶段收口后更新本页任务状态。


暂不优先事项
------------

以下事项不是当前长任务优先方向：

1. 整分支合并旧 ``codex/*`` 参考分支。
2. 大规模引入 PDF visual baseline 或历史样例。
3. 为了覆盖 WordprocessingML 表面而新增低频 API。
4. 未接入 governance / smoke / checklist 的孤立 CLI。
5. 未经证据支撑的性能优化。


当前下一步
----------

最小下一步按下面顺序执行：

1. 等待当前 ``dev`` 最新 CI 全部完成；失败则先修 CI。
2. project-template workflow dashboard 的 ``next_action_summary`` 分组展示已进入
   release materials 和版本契约测试；后续保持该展示不被删减。
3. release material safety 已覆盖 dashboard 入口、分组摘要、reviewer stop
   condition 和 content-control ``repair_action_classes``；后续保持
   ``assert_release_material_safety.ps1`` 默认参与正式发布路径。
4. ``P1-SCHEMA-01`` 已先补 schema patch confidence calibration 的业务模板来源摘要和
   缺失来源 warning/action；``P1-TEMPLATE-01`` 已补 manifest / 描述 / smoke contract
   级别的 ``business_template_corpus``，当前注册 invoice 并规划 contract、policy、
   report、notice、tender。``P1-APPROVAL-01`` 的 approval matrix reviewer action
   字段已经进入 release blocker rollup、handoff 和 reviewer bundle；下一步按
   ``docs/long_task_board_zh.rst`` 的 ``P1-RELEASE-01`` 继续复核 GitHub Release
   refresh / publish workflow artifact 输出是否只来自本轮重新生成的
   ``RELEASE_OUTPUT_ROOT``，并继续与 ``release_assets_manifest.json`` 保持一致。
