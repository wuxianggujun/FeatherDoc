文档治理阶段验收记录（中文）
================================

记录日期：2026-05-18

契约标记：``document_governance_acceptance.v1``、
``document_governance_primary_track``、``pdf_conservative_maintenance``、
``no_pdf_rendering_in_low_resource_stage``、
``long_task_document_governance_closure``。

本记录用于固化当前阶段的文档功能推进结论，避免后续线程只依赖聊天上下文。
当前阶段主线应继续优先完善文档能力；PDF 能力保持保守维护，等文档治理链路
稳定后再扩展。


执行边界
--------

本阶段按低资源约束执行：

- 不运行 CMake、Ninja、MSBuild 或 CTest。
- 不启动浏览器、Word、LibreOffice。
- 不执行 PDF 渲染。
- PowerShell 脚本验证使用后台 Job，并设置 60 秒超时。
- 不删除输出目录。
- 不关闭无法确认属于本任务遗留的外部进程。


验收范围
--------

模板契约与交付就绪：

- 模板交付就绪报告已覆盖项目模板的契约检查。
- 报告 schema 固定为
  ``featherdoc.project_template_delivery_readiness_report.v1``。
- 验证入口包括 ``build_project_template_delivery_readiness_report_test.ps1``。

Content-control 修复工作流：

- 治理报告已输出 ``repair_strategy``、``repair_hint``、
  ``command_template``、``source_json_display``。
- 治理报告 schema 固定为
  ``featherdoc.content_control_data_binding_governance_report.v1``。
- ``repair_plan_items`` 会保留 action-derived ``open_command``，
  让 reviewer 可以从修复计划直接回到治理报告重建入口。
- 占位符阻塞项锁定为
  ``content_control_data_binding.bound_placeholder``。
- 同步修复路径锁定为 ``sync_bound_content_control`` 和
  ``sync-content-controls-from-custom-xml``。

样式与编号真实语料置信度：

- 编号治理报告 schema 固定为
  ``featherdoc.numbering_catalog_governance_report.v1``。
- 真实语料置信度已按文档键对齐，不再只依赖数量对比。
- 报告字段覆盖 ``matched_document_count``、
  ``unmatched_catalog_document_count``、
  ``unmatched_baseline_document_count``、``alignment_gap_count``、
  ``catalog_document_keys``、``baseline_document_keys``、
  ``matched_document_keys``。
- 对齐缺口会产生阻塞项
  ``numbering_catalog_governance.real_corpus_alignment_gap``。

表格与版式交付质量：

- 表格版式治理报告 schema 固定为
  ``featherdoc.table_layout_delivery_governance_report.v1``。
- ``delivery_quality`` 已暴露表格样式、自动修复、人工复核、
  浮动表格定位和命令失败等明细计数。
- PDF 浮动表交付边界固定为 ``pdf_floating_table_support_coverage``、
  ``pdf_floating_table_reviewer_focus``、
  ``pdf_floating_table_metadata_only_fields``、
  ``pdf_floating_table_review_required_fields`` 和 ``metadata-only tblpPr``，
  reviewer 必须先复核元数据级 ``tblpPr`` 字段，再批准 PDF 版式敏感发布。
- ``floating_table_plans_pending`` 保留兼容。
- 发布汇总、handoff 报告和公开材料审计路径已纳入这些明细。

PDF 能力：

- 当前阶段不扩展 PDF 主线。
- PDF 相关工作维持研究记录、现有说明和保守维护，不占用文档治理收口资源。


已完成的轻量验证
----------------

以下脚本已按 60 秒超时策略完成验证：

- ``build_table_layout_delivery_governance_report_test.ps1``
- ``build_release_blocker_rollup_report_test.ps1``
- ``build_release_governance_handoff_report_test.ps1``
- ``release_note_bundle_version_test.ps1``
- ``assert_release_material_safety_test.ps1``
- ``package_release_assets_safety_test.ps1``
- ``package_release_assets_allow_incomplete_test.ps1``
- ``build_content_control_data_binding_governance_report_test.ps1``
- ``build_project_template_delivery_readiness_report_test.ps1``
- ``build_numbering_catalog_governance_report_test.ps1``
- ``release_governance_metrics_contract_test.ps1``
- ``build_release_governance_pipeline_report_test.ps1``


剩余限制
--------

由于机器卡顿，本阶段没有运行完整构建、CTest、Word 视觉回归、LibreOffice
渲染或 PDF 渲染。后续如果机器资源允许，应优先补一轮最小 release
governance 验证，再考虑打开视觉或 PDF 相关重型链路。


下一步建议
----------

1. 继续推进文档功能主线，优先把模板契约、content-control 修复、编号语料
   对齐和表格版式质量作为一个 release-governance 闭环。
2. PDF 功能只做缺陷修复和文档维护，不新开重型渲染任务。
3. 如需开长任务，应开“文档治理收口”任务，而不是 PDF 扩展任务。
