模板 Schema 高层变更 API（中文）
=================================

这份文档记录 ``FeatherDoc`` 当前可直接在 C++ 内存对象上执行的 template
schema mutation 能力。它适合模板接入、模板升级和 schema baseline 修复场景，
避免所有变更都必须绕 CLI JSON 文件工作流。


当前能力边界
------------

``template_schema`` 现在支持两类变更入口：

- patch 级入口：``template_schema_patch``、``apply_template_schema_patch(...)``、
  ``preview_template_schema_patch(...)`` 和 ``build_template_schema_patch(...)``。
- 高层 helper：``replace_template_schema_target(...)``、
  ``remove_template_schema_target(...)``、``rename_template_schema_target(...)``、
  ``upsert_template_schema_slot(...)``、``remove_template_schema_slot(...)``、
  ``rename_template_schema_slot(...)`` 和 ``update_template_schema_slot(...)``。

所有 helper 都会复用同一套 normalize / merge 规则，保证输出顺序稳定，并把重复
slot 收敛到同一个 key 上。


Target 重命名 / 迁移示例
------------------------

``rename_template_schema_target(...)`` 会把 source target 下的 slot 整体迁移到目标
target。如果目标 target 已有相同 source + name 的 slot，会按 schema merge 规则覆盖
目标 slot；如果 source target 不存在或 source 与目标相同，则返回空变更 summary。

.. code-block:: cpp

    auto header = featherdoc::template_schema_part_selector{};
    header.part = featherdoc::template_schema_part_kind::header;

    featherdoc::template_schema schema{{
        {{}, {"customer", featherdoc::template_slot_kind::text, true}},
        {{}, {"line_items", featherdoc::template_slot_kind::table_rows, true}},
        {header, {"customer", featherdoc::template_slot_kind::text, false}},
    }};

    const auto summary = featherdoc::rename_template_schema_target(
        schema, featherdoc::template_schema_part_selector{}, header);

    // summary.removed_targets == 1
    // summary.inserted_slots == 1：line_items 被迁移到 header
    // summary.replaced_slots == 1：header/customer 被 body/customer 覆盖


从文档扫描生成 Patch
-------------------

``scan_template_schema(...)`` 会扫描当前文档中的书签和 content control，生成
内存级 ``template_schema_scan_result``。如果已经有一份 committed baseline，
可以直接调用 ``build_template_schema_patch_from_scan(...)`` 得到从 baseline 到
当前文档扫描结果的 patch：

.. code-block:: cpp

    featherdoc::template_schema baseline{{
        {{}, {"customer", featherdoc::template_slot_kind::text, false}},
        {{}, {"obsolete", featherdoc::template_slot_kind::text, true}},
    }};

    auto scan = doc.scan_template_schema();
    if (!scan) {
        throw std::runtime_error(doc.last_error().detail);
    }

    auto patch = doc.build_template_schema_patch_from_scan(baseline);
    if (!patch) {
        throw std::runtime_error(doc.last_error().detail);
    }

    const auto preview = featherdoc::preview_template_schema_patch(baseline, *patch);

扫描默认使用 ``related_parts`` target 模式，也可以切换为 section 目标：

.. code-block:: cpp

    featherdoc::template_schema_scan_options options{};
    options.target_mode =
        featherdoc::template_schema_scan_target_mode::resolved_section_targets;

    auto section_patch = doc.build_template_schema_patch_from_scan(baseline, options);

如果扫描到 ``block_range``、``mixed`` 或 malformed bookmark，它们会进入
``skipped_bookmarks``，不会被强行转换成 slot。


Slot 变更示例
-------------

.. code-block:: cpp

    featherdoc::template_schema_slot_update update{};
    update.kind = featherdoc::template_slot_kind::block;
    update.required = false;
    update.min_occurrences = 1U;
    update.clear_max_occurrences = true;

    const auto update_summary = featherdoc::update_template_schema_slot(
        schema, {header, "customer"}, update);

    const auto rename_summary = featherdoc::rename_template_schema_slot(
        schema, {header, "customer"}, "customer_name");


稳定 JSON Review Summary
------------------------

``make_template_schema_patch_review_summary(...)`` 可以从 baseline + patch，或
baseline + generated schema 生成稳定 review summary。它会记录 baseline / generated
slot 数量、patch 操作数量，以及实际 preview 后会发生的变更数量。

.. code-block:: cpp

    auto patch = doc.build_template_schema_patch_from_scan(baseline);
    if (!patch) {
        throw std::runtime_error(doc.last_error().detail);
    }

    const auto review = featherdoc::make_template_schema_patch_review_summary(
        baseline, *patch);

    const auto json = featherdoc::template_schema_patch_review_json(review);

JSON 顶层 schema 固定为 ``featherdoc.template_schema_patch_review.v1``，便于 CI、
onboarding 报告和后续自动修复流程稳定消费。CLI 侧也可以在 preview / build patch
时同步写出同一份稳定 review JSON：

.. code-block:: powershell

    featherdoc_cli preview-template-schema-patch committed-schema.json generated-schema.json --output-patch schema.patch.json --review-json schema.review.json --json
    featherdoc_cli build-template-schema-patch committed-schema.json generated-schema.json --output schema.patch.json --review-json schema.review.json --json

baseline gate 脚本也可以直接产出同格式 review JSON：

.. code-block:: powershell

    pwsh -ExecutionPolicy Bypass -File .\scripts\check_template_schema_baseline.ps1 -InputDocx .\template.docx -SchemaFile .\template.schema.json -GeneratedSchemaOutput .\generated-template.schema.json -ReviewJsonOutput .\template-schema.review.json -SkipBuild
    pwsh -ExecutionPolicy Bypass -File .\scripts\check_template_schema_manifest.ps1 -ManifestPath .\baselines\template-schema\manifest.json -ReviewJsonOutputDir .\output\template-schema-manifest-reviews -SkipBuild

project template smoke 会为每个 ``schema_baseline`` entry 默认写出
``schema_patch_review.json``，并在 ``summary.json`` / ``summary.md`` 中聚合
``schema_patch_review_count``、``schema_patch_review_changed_count`` 和
``schema_patch_reviews``。当 review 显示 schema drift 时，smoke 还会生成
``schema_patch_approval_result.json`` 审批记录模板，并汇总
``schema_patch_approval_pending_count``、``schema_patch_approval_approved_count``、
``schema_patch_approval_rejected_count``、``schema_patch_approval_compliance_issue_count``
与 ``schema_patch_approval_items``，把 ``generated_output`` 标记为待审批的 schema
更新候选。

人工复核时，可以编辑 ``schema_patch_approval_result.json`` 的 ``decision`` 字段：
``pending`` 表示仍待审，``approved`` 表示同意把 generated schema 作为更新候选，
``rejected`` / ``needs_changes`` 表示需要继续调整模板或 schema。除 ``pending`` 外，
``approved`` / ``rejected`` / ``needs_changes`` 都必须提供 ``reviewer`` 与
``reviewed_at``；缺失时条目会标记为 ``invalid_result``，``action`` 会变成
``fix_schema_patch_approval_result``，并在 ``compliance_issues`` 中列出
``missing_reviewer`` 或 ``missing_reviewed_at``。也可以在 manifest 的
``schema_baseline.approval_result`` 中指定稳定路径，避免审批记录只留在临时输出目录。

复核完成后，运行同步脚本把审批结论回灌到 ``summary.json`` / ``summary.md``：

.. code-block:: powershell

    pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1 -SummaryJson .\output\project-template-smoke\summary.json
    pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1 -SummaryJson .\output\project-template-smoke\summary.json -EntryName invoice-template -Decision approved -Reviewer alice -Note "accept generated schema"
    pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1 -SummaryJson .\output\project-template-smoke\summary.json -ReleaseCandidateSummaryJson .\output\release-candidate\summary.json
    pwsh -ExecutionPolicy Bypass -File .\scripts\write_project_template_schema_approval_history.ps1 -SummaryJson .\output\project-template-smoke\summary.json,.\output\release-candidate\summary.json -OutputJson .\output\project-template-schema-approval-history\history.json
    pwsh -ExecutionPolicy Bypass -File .\scripts\write_project_template_schema_approval_history.ps1 -SummaryJsonDir .\output -Recurse -OutputJson .\output\project-template-schema-approval-history\history.json

``onboard_project_template.ps1`` 会把这些 review / approval 聚合信息带入 onboarding
summary 和人工复核文档；如果传入 ``-ReleaseCandidateSummaryJson``，同步脚本还会刷新
release-candidate summary，并在 ``final_review.md`` 中维护 schema approval 专属区块。
同步后的 release summary 会携带 ``schema_patch_approval_gate_status``、
``schema_patch_approval_invalid_result_count`` 与 ``schema_patch_approval_gate_blocked``；
当审批结果缺少审计字段或仍存在 ``invalid_result`` 时，状态会变为 ``blocked``，
release candidate preflight / reviewer checklist 会把它视作发布阻断项。
release summary 顶层会同步维护 ``release_blockers`` / ``release_blocker_count``，
schema approval 阻断使用 ``project_template_smoke.schema_approval`` 作为稳定 id，
便于 CI 和发布面板直接消费阻断原因、``issue_keys`` 与待修复审批条目，
无需解析 ``final_review.md``。
``write_project_template_schema_approval_history.ps1`` 可以进一步把多次 smoke 或
release summary 汇总成 ``featherdoc.project_template_schema_approval_history.v1``，
输出长期趋势 JSON / Markdown。目录模式只递归收集 ``summary.json``，避免把
``schema_patch_approval_result.json`` 等审批明细误算成一次运行。报表同时写出
``latest_blocking_summary`` 和 ``entry_histories``：前者直接指出最近一次阻断的
条目、审计缺口和修复动作，后者按模板 entry 聚合 blocked / pending / approved /
needs_changes 的历史状态。release-candidate preflight 在启用 project template smoke
时会自动生成 ``project_template_schema_approval_history.json`` / ``.md`` 并写入 release
交付文档。这样接入新模板或发布前复核时，都能直接看到 schema drift 的可审计摘要、
审批记录与下一步动作。


使用建议
--------

1. 如果要整体替换某个 target，优先使用 ``replace_template_schema_target(...)``。
2. 如果是把 body/header/footer 的 schema 契约迁移到另一个 target，使用
   ``rename_template_schema_target(...)``，这样能保留 slot 形状并自动处理冲突。
3. 如果只是单个 slot 的名称或约束变化，使用 slot 级 helper，避免手写 patch。
4. 对 CI 或自动修复流程，先用 ``preview_template_schema_patch(...)`` 计算 summary，
   再决定是否正式 apply。


后续扩展方向
------------

- 基于更多业务模板样本校准 rename / update 建议的置信度。
- 继续沉淀真实项目里的 schema approval 审计字段和 release gate 复核策略。
- 基于历史报表继续补充趋势阈值、异常波动提示和版本治理看板。
