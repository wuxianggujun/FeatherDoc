长期任务推进台账（中文）
========================

状态日期：2026-06-18

本页是当前长任务的执行台账，用来回答三个问题：

1. 后续到底按什么顺序推进。
2. 每一项做到什么程度才算完成。
3. 每一轮推进后用什么命令验证、提交和推送。

任务台账维护 marker：``long_task_board_zh``。


当前长任务 goal
---------------

当前 active goal：

``持续推进 FeatherDoc 后续开发任务：维护后续任务文档，按优先级在 dev 分支实现、验证、提交、推送，并保持 CI 绿色。``

Goal 状态：本线程已创建并保持 ``active``；后续不重复创建第二个 goal，
只围绕本台账和 ``docs/next_tasks_zh.rst`` 逐轮推进。

执行边界：

1. 只在 ``dev`` 分支开发，不新建 ``codex/*`` 分支。
2. 小步修改、小步验证、小步提交；CI 失败优先修 CI。
3. 新增能力必须同时考虑源码、脚本、文档、测试和 release governance。
4. 中文文档、PowerShell 输出和测试样例默认保持 UTF-8。
5. 不整分支合并旧参考分支；只摘当前 ``dev`` 缺失且可验证的小能力。


状态说明
--------

``TODO``
    尚未开始，等待排入近期执行。

``DOING``
    当前正在推进，下一轮应优先回到这一项。

``GUARDED``
    已有能力，但需要长期守护、补测试或等待 CI / release gate 验证。

``DONE``
    当前阶段已完成，后续只做回归维护。

``DEFERRED``
    明确后置，除非 release readiness 或用户目标改变，否则不抢先做。


每轮固定闭环
------------

每轮开始先确认分支、工作区和 CI：

.. code-block:: powershell

   git status --short --branch
   git branch --list "codex/*"
   git branch -r --list "origin/codex/*"
   gh run list --branch dev --limit 8 --json databaseId,workflowName,status,conclusion,headSha,createdAt,url

每轮结束至少执行：

.. code-block:: powershell

   git diff --check
   git status --short --branch

如果改到脚本、文档或发布材料，再补对应契约测试。提交后推送 ``dev``：

.. code-block:: powershell

   git add <changed-files>
   git commit -m "<type>: <short summary>"
   git push origin dev


近期执行队列
------------

1. ``P0-CI-01``：dev CI 守护

   * 状态：``GUARDED``。
   * 目标：确认最新 ``dev`` 的 Linux、Windows、macOS 和 Docs Pages 均绿色。
   * 验收：``gh run list --branch dev`` 无失败；若失败，优先抓日志并修复。
   * 当前结果：截至本次台账刷新，本轮基线提交
     ``9bb8fbaf9697c619373ee71f1d04c4bb48703459`` 的 Docs Pages、Linux CMake CI
     和 macOS CMake CI 已通过，Windows MSVC CI 仍在运行中，暂未观察到失败。
     本轮新增提交推送后，以最新 ``dev`` head 的四个 workflow 结果为 P0 跟踪对象。
   * 已修复：上一轮 Windows MSVC 在 ``release_candidate_visual_verdict`` 和
     ``release_candidate_visual_verdict_reports`` 中暴露的 release entry material
     safety 误判已解除。修复点包括让 material-safety helper 在同一 anchor 的
     任一 Markdown list block 中匹配完整 contract，并让 project-template workflow
     dashboard action group 在 ready 场景也保留 ``blocker=`` 与 ``entries=``
     marker。
   * 下一步：继续守护 CI；若后续 ``dev`` 出现失败，优先回到本项抓日志修复。
   * 备注：这项优先级高于继续堆叠功能。

2. ``P1-SCHEMA-01``：schema patch confidence 的真实业务语料来源追踪

   * 状态：``GUARDED``。
   * 目标：让 schema patch confidence calibration report 能看出候选项来自哪些项目、
     模板或业务样本，而不是只有置信度汇总。
   * 最小实现：在报告 JSON / Markdown 中增加业务模板来源摘要、缺失来源信息 warning
     或 action item。
   * 当前产物：``business_template_corpus_summary``、
     ``schema_patch_confidence_calibration.missing_business_template_source_metadata`` 和
     ``add_business_template_source_metadata`` 已进入脚本与测试。
   * 验收：``test/write_schema_patch_confidence_calibration_report_test.ps1`` 通过；
     文档契约测试仍能通过。

3. ``P1-TEMPLATE-01``：真实业务模板语料扩展

   * 状态：``GUARDED``。
   * 目标：逐步补合同、制度、发票、报告、通知、标书等样本入口。
   * 最小实现：先补 manifest / 描述 / smoke contract，不直接塞大体积二进制样本。
   * 当前产物：``samples/project_template_smoke.manifest.json`` 已增加
     ``business_template_corpus``，注册 invoice，规划 contract、policy、report、
     notice、tender；``describe_project_template_smoke_manifest.ps1`` 和 delivery
     readiness warning 已暴露 registered / planned corpus 计数。
     ``check_project_template_smoke_manifest_test.ps1`` 现在还会直接校验仓库真实
     manifest，锁住 invoice、contract、policy、report、notice、tender 6 类
     document type，以及 1 个 registered / 5 个 planned corpus 入口。
   * 验收：project-template smoke manifest 可解释新增样本来源、用途和验证边界。

4. ``P1-DASHBOARD-01``：project-template workflow dashboard 持续治理

   * 状态：``GUARDED``。
   * 目标：保持 dashboard 的 status、release_ready、blocker、warning、
     ``next_action_summary`` 和证据路径进入 release materials。
   * 当前产物：release candidate visual verdict 回归会断言 ready dashboard 的
     action group 在 ``ARTIFACT_GUIDE.md``、``REVIEWER_CHECKLIST.md`` 和
     ``START_HERE.md`` 中同时保留 ``source``、``action``、``blocker=(none)``
     和 ``entries=(none)`` marker，避免 ready 场景因为没有 blocker / entries
     而丢失 reviewer-facing 字段。
     release note bundle 的 blocked dashboard fixture 也已扩展为两组 action group，
     断言入口材料必须同时展示 onboarding governance 与 delivery readiness 的
     ``source``、``action``、``blocker`` 和 ``entries``，避免多项阻断时只显示第一组
     ``next_action_summary``。
   * 验收：``assert_release_material_safety.ps1`` 覆盖 dashboard 入口材料。

5. ``P1-APPROVAL-01``：多项目 schema approval 维护体验

   * 状态：``GUARDED``。
   * 目标：让 pending / rejected / approved 历史更适合多项目、多模板、多轮审批。
   * 当前产物：``write_project_template_schema_approval_history.ps1`` 会输出
     ``project_template_approval_matrix``，并在 ``entry_histories`` 中保留
     ``project_id``、``template_name`` 与 ``template_scope``，方便 reviewer 按项目模板
     复核最新状态、历史阻断和下一步 action。
     approval matrix 现在还会输出 ``reviewer_action_summary``、
     ``reviewer_action_reason`` 和 ``reviewer_actions``，让 pending / rejected
     的模板行直接带可执行复核入口。
     ``build_project_template_delivery_readiness_report.ps1`` 已把这些 reviewer action
     字段透传到模板 ``schema_history`` 和历史 blocker；release blocker rollup 与
     release governance handoff 会继续保留同名字段并在 Markdown 中展示。
     release note bundle 的入口材料也会从统一 source contract 输出
     ``requires_reviewer_action``、``reviewer_action_summary``、
     ``reviewer_action_reason`` 和 ``reviewer_actions``。
   * 验收：approval history 能被 release blocker rollup、checklist 和 reviewer bundle
     同步解释；后续入口材料重构不能丢失 ``reviewer_action_*`` 字段。

6. ``P1-CONTENT-01``：content-control 与 Custom XML 绑定治理

   * 状态：``GUARDED``。
   * 目标：继续强化 content-control slot、数据绑定、修复建议和模板 schema 的闭环。
   * 当前产物：``build_content_control_data_binding_governance_report.ps1`` 会输出
     ``repair_action_class_summary`` 和 ``repair_action_classes``，把 repair plan
     标成 ``release_blocking``、``auto_repair_candidate``、
     ``manual_confirmation_required`` 三类，并保留 source 与 command。
   * 验收：新增规则必须能说明自动修复、人工确认或发布阻断的边界。

7. ``P1-RELEASE-01``：release governance 材料一致性

   * 状态：``DOING``。
   * 目标：blocker / warning / action item 在 dashboard、handoff、bundle、
     checklist 中保持同一来源。
   * 当前产物：``repair_action_classes`` 已从 content-control governance report
     透传到 release blocker rollup、release governance handoff、final review、
     handoff / artifact / start-here bundle 和 reviewer checklist，并由
     release material safety 静态/样例断言守住入口材料里的三类 action marker。
     ``action=sync_or_fill_bound_content_control`` 也已透传到 release asset
     manifest 和入口材料合同，reviewer 可按统一 action id 分流处理。
     release note bundle 的四份入口文档已由统一 helper 断言 content-control
     blocker/action-item 的 action id，避免后续生成逻辑只保留说明文字而丢失
     可机器追踪的处理动作。
     ``repair_action_classes`` 的 reviewer-facing 可见性也已纳入 release note
     bundle fixture 与统一 helper，覆盖 content-control blocker 的三类动作和
     duplicate-binding action item 的人工确认动作。
     ``REVIEWER_CHECKLIST.md`` 进一步用精确断言锁住 content-control blocker
     与 duplicate-binding action item 的 source report、source JSON、
     action、open command 和 command template 指引。
     schema approval history 的 ``reviewer_action_summary``、
     ``reviewer_action_reason`` 和 ``reviewer_actions`` 已从 delivery readiness
     进入 release blocker rollup 与 release governance handoff，便于 reviewer
     直接识别 pending / rejected 模板的下一步处理动作。
     这组字段已继续进入 ``release_handoff.md``、``ARTIFACT_GUIDE.md``、
     ``REVIEWER_CHECKLIST.md`` 和 ``START_HERE.md``；reviewer checklist
     会为 blocker 与 action item 输出 reviewer action、原因和候选动作。
     ``release_assets_manifest.json`` 也开始保留 ``requires_reviewer_action``、
     ``reviewer_action_summary``、``reviewer_action_reason`` 和
     ``reviewer_actions``，并由 manifest signoff 必备字段与 material safety
     负例测试守住。
     ``publish_github_release.ps1`` 的上传后 manifest 刷新逻辑已补 add-or-update
     写入，离线 fake gh 回归会验证 ``upload.remote_assets`` 只包含
     ``release_assets_manifest.json`` 里列出的正式 ZIP，并保留远端 URL、大小和
     下载计数。
     该回归现在还显式覆盖 GitHub Release 远端资产中同时存在
     ``release_assets_manifest.json`` 的场景，要求 ``upload.remote_assets`` 不把
     manifest 自身当作正式 ZIP 资产。
     ``package_release_assets.ps1 -UploadReleaseTag`` 的直接上传路径也已用 fake gh
     fixture 锁住同一规则：远端存在 manifest 和 unrelated asset 时，manifest 的
     ``upload.remote_assets`` 仍只保留三份正式 ZIP 及其 URL、大小和下载计数。
     这条规则已下沉到 ``assert_release_material_safety.ps1`` 的 manifest
     静态 contract：上传成功时 ``upload.remote_assets`` 只能包含三份正式 ZIP，
     且每项必须保留 URL、大小和下载计数；manifest 自身或 unrelated asset
     混入时必须失败。
     ``upload.remote_assets`` 的远端 URL contract 也已继续收紧：每项 URL
     必须通过 URL path 指向 ``/releases/download/<tag>/<asset>.zip``，path
     basename 必须等于同名正式 ZIP 文件名，并包含请求发布 tag 的 release
     download 路径，防止 wrong-file URL、wrong-tag URL、query-only 文件名或
     非 release download 路径被写入 ``release_assets_manifest.json``。
     staged material safety 的路径断言也已兼容 ``<windows-absolute-path>`` 占位和
     repo-relative public display，避免 CMake build dir 位于仓库内时误判安全公开路径。
     GitHub Release refresh / publish workflow 已补维护契约，固定
     ``RELEASE_OUTPUT_ROOT``、发布脚本入口、``release-refresh-output`` /
     ``release-publish-output``，并要求上传路径直接引用
     ``${{ env.RELEASE_OUTPUT_ROOT }}/**``，避免 artifact path 和实际输出根漂移。
     refresh / publish workflow 现在会在调用发布脚本前只清理 workspace 内的
     ``RELEASE_OUTPUT_ROOT``，避免 self-hosted runner 的旧
     ``output/release-assets`` 残留被重新上传到本轮 release artifact。递归清理前
     已改用 ``GetRelativePath`` containment contract，拒绝 workspace 根目录、
     workspace 外路径和相邻目录前缀误判。
     release asset manifest 的 material safety 负例已分别覆盖
     ``reviewer_action_reason`` 和 ``reviewer_actions``，不再只依赖
     ``reviewer_action_summary`` 代表整组 reviewer action 字段。
     package release assets 的 ``START_HERE.md``、``ARTIFACT_GUIDE.md`` 和
     ``REVIEWER_CHECKLIST.md`` 已补 ``reviewer_action_*`` 值级断言，确保
     readiness / onboarding no-action 语义不会只剩字段名。
     release note bundle 现在进一步逐行断言三份入口材料的 manifest signoff
     同时包含 ``status``、``release_ready``、``reviewer_action_*``、
     ``source_report_display`` 和 ``source_json_display`` 等完整必备字段；
     三个入口生成脚本也由文档契约测试锁定同一字段集合。
     ``release_body.zh-CN.md`` 和 ``release_summary.zh-CN.md`` 的
     project-template governance contract 摘要已从 blocker/action item 证据聚合
     ``requires_reviewer_action``、``reviewer_action_summary``、
     ``reviewer_action_reason`` 和 ``reviewer_actions``，避免 release notes
     只展示 readiness 状态而丢失人工复核动作。
     文档契约测试已锁定 ``write_release_body_zh_summary.ps1`` 的 reviewer action
     聚合 helper，以及 release note bundle 对上述字段值的断言，避免后续重构只保留
     release notes 文本而丢失源级聚合路径。
     reviewer action 聚合路径还会被源级契约约束为同时扫描 blocker、action item
     和 warning，并按 ``report_id`` / ``source_schema`` 关联证据，避免只从某个
     偶然存在的 blocker 取值。
     release note bundle 回归现在还包含 warning-only 变体：当 delivery readiness
     的 reviewer action 只存在于 warning 时，``release_body.zh-CN.md`` 和
     ``release_summary.zh-CN.md`` 仍必须输出完整 ``reviewer_action_*`` 字段。
     content-control blocker 与 duplicate-binding action item 也已收紧为同一
     Markdown list block 断言，要求 ``source_report_display``、``source_json_display``、
     ``repair_action_classes``、``repair_strategy``、``repair_hint`` 和
     ``command_template`` 与对应 action 同块出现。
     release candidate visual verdict 回归也会锁住 project-template workflow
     dashboard 的 ready action group 输出，要求入口材料在无 blocker / entries
     时仍保留 ``blocker=(none)`` 和 ``entries=(none)``，避免 material safety
     与 reviewer bundle 对 ready 场景的字段解释发生分歧。
     release note bundle fixture 现在还锁住 blocked dashboard 的两组 action group，
     要求 ``ARTIFACT_GUIDE.md``、``REVIEWER_CHECKLIST.md`` 和 ``START_HERE.md``
     同时保留 onboarding governance 与 delivery readiness 的 source / action /
     blocker / entries，防止 release reviewer bundle 只展示第一条 dashboard action。
   * 验收：新增 release 字段时同步补 release material safety 或 release note bundle 测试。

8. ``P2-STYLE-01``：样式建议置信度校准

   * 状态：``TODO``。
   * 目标：基于真实文档继续校准 style merge / rename suggestion。
   * 验收：低置信度建议输出人工复核原因，不能直接进入自动修复。

9. ``P2-NUMBERING-01``：样式、编号和 document skeleton 治理

   * 状态：``TODO``。
   * 目标：让 heading、list、theme、numbering catalog 和 style numbering 更稳定地联动。
   * 验收：document skeleton governance rollup 保留 per-document source 与 action。

10. ``P2-TABLE-01``：表格与版式交付质量

    * 状态：``TODO``。
    * 目标：继续推进 table layout delivery、floating table 和 PDF-sensitive 布局证据。
    * 验收：表格位置、``tblLook``、固定布局差异和 unresolved item 能进入 release-facing 指标。

11. ``P2-WORD-01``：DOCX smoke 与 Word visual preflight

    * 状态：``GUARDED``。
    * 目标：保持 ``check_docx_functional_smoke_readiness.ps1`` 和
      ``check_word_visual_release_gate_preflight.ps1`` 可运行。
    * 验收：完整 Word visual gate 只在工作区干净、源码已推送、资源稳定时运行。

12. ``P3-PDF-01``：PDF 保守维护

    * 状态：``DEFERRED``。
    * 目标：只维护当前 PDF import/export、CJK、copy/search 和 preflight 能力。
    * 验收：不从旧分支批量搬入 PDF visual baseline 或大样例。

13. ``P3-DOCS-01``：文档、脚本索引和契约测试治理

    * 状态：``GUARDED``。
    * 目标：新增脚本、发布字段或路线任务时，同步维护对应文档和测试。
    * 验收：``current_direction_docs_contract_test.ps1``、
      ``script_task_index_docs_contract_test.ps1`` 和相关 release metadata 测试保持绿色。


下一轮最小动作
--------------

下一轮按这个顺序执行：

1. 复查最新 ``dev`` CI；失败就先修失败。
2. 继续守护本轮 ``upload.remote_assets`` 静态 contract、远端 URL release
   download path / 文件名 / tag 绑定、warning-only reviewer action 回归和
   content-control 同块断言的 CI；
   若失败，优先修复对应 fixture 或生成脚本。
3. 复核 release material safety 与 release asset manifest 对新增发布字段的静态 contract
   是否仍存在盲区；优先补薄弱测试，不做大重构。
4. 运行相关 PowerShell 测试和 ``git diff --check``。
5. 提交并推送 ``dev``。
6. 回到本台账，把 ``DOING`` 项的状态、证据和下一步更新清楚。


不做清单
--------

这些事项不是当前长任务的近期动作：

1. 新建 ``codex/*`` 开发分支。
2. 整分支合并旧参考分支。
3. 大规模加入二进制样本或视觉 baseline。
4. 在没有测试和 release governance 入口的情况下新增孤立 CLI。
5. 在 CI 未确认时继续叠加大改动。


阶段完成定义
------------

当前长任务不以“写完一份文档”为完成条件。只有同时满足下面条件，才算一个阶段可收口：

1. ``dev`` 与 ``origin/dev`` 对齐。
2. 最新 ``dev`` CI 绿色，或已明确记录失败原因和修复任务。
3. 当前 ``DOING`` 项有源码 / 脚本 / 文档 / 测试中的至少一个可验证产物。
4. 相关验证命令通过。
5. 本页和 ``docs/next_tasks_zh.rst`` 的下一步保持一致。
