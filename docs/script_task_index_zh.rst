脚本任务索引（中文）
====================

本页用于给 ``scripts`` 目录建立维护入口。它不是完整脚本手册，也不替代
各脚本自己的参数说明；它只固定当前 ``dev`` 分支上最常用、最需要持续维护
的脚本任务分组，帮助维护者先找到正确入口，再决定是否进入重型验证。

维护原则
--------

1. 优先使用只读检查、报告生成和 manifest gate，不直接启动 Word、CMake、
   LibreOffice 或完整 PDF visual gate。
2. 单个 PowerShell 验证默认按 60 秒超时管理。
3. 先使用 ``-SkipBuild``、已存在输出和轻量 fixture 形成闭环；资源稳定后再
   执行完整视觉或发布 gate。
4. 脚本入口应服务当前三条主线：模板契约、样式 / 编号治理、表格 / 版式
   交付质量。
5. 新增脚本时，应同时考虑 README / Sphinx 入口、CMake 轻量测试注册、
   输出 schema 版本和发布治理消费路径。


脚本索引自检
------------

- ``scripts/check_script_task_index.ps1``：只读检查本页列出的 ``scripts/*.ps1``
  和 ``scripts/*.py`` 路径是否真实存在，同时确认本文已经进入 Sphinx 首页、
  文档维护计划、项目评分建议和 CMake 轻量测试注册；默认同时写出
  ``summary.json`` 和 ``script_task_index_check.md``，让 CI / release 面板消费
  稳定 JSON，也让人工复核直接阅读 Markdown 摘要；可用 ``-SummaryJson`` /
  ``-ReportMarkdown`` 显式指定输出路径；同一个脚本路径被重复列出时也会失败，
  并在 JSON / Markdown 中写出 ``duplicate_script_reference_count``；报告还会
  写出 ``documentation_entrypoint_count`` 和 ``documentation_entrypoints``，
  固定 README 入口必须同时指向文档维护总览与脚本任务索引；
  通过 ``script_reference_group_count`` 和 ``script_reference_groups`` 按本页
  小节汇总脚本覆盖量，并通过 ``script_reference_extension_count`` 和
  ``script_reference_extensions`` 汇总 ``.ps1`` / ``.py`` 入口分布，方便维护者
  快速发现某个分组或脚本类型的异常变化。


模板契约与项目模板
------------------

这组脚本服务“模板可以被接入、校验、生成、回归、发布”的闭环。

- ``scripts/check_template_schema_baseline.ps1``：检查单个模板 schema
  baseline 是否漂移。
- ``scripts/check_template_schema_manifest.ps1``：按 manifest 批量运行
  schema baseline gate。
- ``scripts/freeze_template_schema_baseline.ps1``：冻结当前 schema 作为
  baseline 候选。
- ``scripts/register_template_schema_manifest_entry.ps1``：把模板 schema
  样本注册进 manifest。
- ``scripts/describe_template_schema_manifest.ps1``：只读描述 schema
  manifest 状态。
- ``scripts/run_project_template_smoke.ps1``：运行项目模板 smoke manifest。
- ``scripts/check_project_template_smoke_manifest.ps1``：校验项目模板 smoke
  manifest 结构。
- ``scripts/describe_project_template_smoke_manifest.ps1``：只读描述项目模板
  smoke manifest 状态与维护入口。
- ``scripts/discover_project_template_smoke_candidates.ps1``：发现尚未纳入
  manifest 的 ``.docx`` / ``.dotx`` 候选。
- ``scripts/new_project_template_smoke_onboarding_plan.ps1``：生成项目模板
  smoke 候选接入计划，不直接修改 manifest。
- ``scripts/register_project_template_smoke_manifest_entry.ps1``：注册真实模板
  smoke 条目。
- ``scripts/sync_project_template_schema_approval.ps1``：同步项目模板 schema
  approval 的 reviewer 决策与发布摘要。
- ``scripts/write_project_template_schema_approval_history.ps1``：汇总多次
  smoke / release summary 的 schema approval 历史。
- ``scripts/build_project_template_onboarding_governance_report.ps1``：生成
  onboarding governance 报告。
- ``scripts/build_project_template_delivery_readiness_report.ps1``：生成项目模板
  delivery readiness 报告。


模板渲染与数据映射
------------------

这组脚本支撑从模板导出 render plan、映射业务数据、再回写文档的轻量链路。

- ``scripts/export_template_render_plan.ps1``：从模板导出 render plan。
- ``scripts/patch_template_render_plan.ps1``：把 patch 应用到 render plan。
- ``scripts/prepare_template_render_data_workspace.ps1``：准备数据映射工作区。
- ``scripts/export_render_data_mapping_draft.ps1``：导出初始 mapping draft。
- ``scripts/lint_render_data_mapping.ps1``：对 mapping draft 做结构 lint。
- ``scripts/validate_render_data_mapping.ps1``：校验 mapping 是否满足渲染输入。
- ``scripts/convert_render_data_to_patch_plan.ps1``：把 render data 转为 patch
  plan。
- ``scripts/render_template_document.ps1``：按 render plan 生成文档。
- ``scripts/render_template_document_from_data.ps1``：从数据直接生成文档。
- ``scripts/render_template_document_from_patch.ps1``：按 patch 链路生成文档。
- ``scripts/render_template_document_from_workspace.ps1``：从准备好的 workspace
  渲染文档。
- ``scripts/edit_document_from_plan.ps1``：按编辑计划修改既有文档。


内容控件与 DOCX 功能 smoke
---------------------------

这组脚本把 content-control、Custom XML 绑定和最小功能 smoke 接入发布治理。

- ``scripts/build_content_control_data_binding_governance_report.ps1``：生成
  data-binding governance 报告。
- ``scripts/check_docx_functional_smoke_readiness.ps1``：只读检查 DOCX 功能
  smoke 证据和可复用 Word visual smoke PNG。
- ``scripts/sync_project_template_smoke_visual_verdict.ps1``：同步项目模板 smoke
  的视觉 verdict 到发布材料。


样式、编号与文档骨架治理
------------------------

这组脚本服务标题、列表、样式和编号目录的可控治理。

- ``scripts/check_numbering_catalog_baseline.ps1``：检查单文档 numbering catalog
  baseline。
- ``scripts/check_numbering_catalog_manifest.ps1``：按 manifest 批量检查
  numbering catalog。
- ``scripts/build_numbering_catalog_governance_report.ps1``：生成 numbering
  catalog governance 报告。
- ``scripts/build_document_skeleton_governance_report.ps1``：生成单文档骨架治理
  报告。
- ``scripts/build_document_skeleton_governance_rollup_report.ps1``：汇总多份骨架
  治理报告。
- ``scripts/write_schema_patch_confidence_calibration_report.ps1``：生成 schema
  patch 置信度校准报告。
- ``scripts/write_style_merge_suggestion_review.ps1``：生成 style merge 建议复核
  材料。
- ``scripts/apply_reviewed_style_merge_suggestions.ps1``：应用已复核的 style merge
  建议。
- ``scripts/audit_style_merge_restore_plan.ps1``：审计 style merge restore plan。


表格与版式交付质量
------------------

这组脚本把表格样式、``tblLook``、浮动表格和版式质量接入治理材料。

- ``scripts/build_table_layout_delivery_report.ps1``：生成单文档 table layout
  delivery 报告。
- ``scripts/build_table_layout_delivery_rollup_report.ps1``：汇总多份 table layout
  delivery 报告。
- ``scripts/build_table_layout_delivery_governance_report.ps1``：把 rollup 晋升为
  release-facing governance summary。
- ``scripts/run_table_layout_delivery_report.ps1``：运行表格交付报告的本地入口。
- ``scripts/run_table_style_quality_visual_regression.ps1``：运行 table style
  quality 视觉回归。
- ``scripts/run_fixed_grid_merge_unmerge_regression.ps1``：运行 fixed-grid
  merge / unmerge 回归 bundle。


Word 视觉验证与人工复核
-----------------------

这组脚本需要真实 ``Microsoft Word``，默认不属于低资源阶段的自动验证。

- ``scripts/run_word_visual_smoke.ps1``：生成基础 Word 渲染截图。
- ``scripts/run_word_visual_release_gate.ps1``：运行完整 Word visual release gate。
- ``scripts/check_word_visual_release_gate_preflight.ps1``：只读检查 release gate
  前置条件，不启动 Word。
- ``scripts/prepare_word_review_task.ps1``：准备人工视觉复核任务。
- ``scripts/open_latest_word_review_task.ps1``：打开最新复核任务。
- ``scripts/record_word_visual_review_result.ps1``：记录人工复核 verdict。
- ``scripts/sync_latest_visual_review_verdict.ps1``：同步最新视觉复核 verdict。
- ``scripts/sync_visual_review_verdict.ps1``：按显式路径同步视觉复核 verdict。
- ``scripts/refresh_readme_visual_assets.ps1``：刷新 README 视觉资产。
- ``scripts/word_visual_review_report.ps1``：生成 Word visual review 报告。


Release governance 与发布材料
-----------------------------

这组脚本把多个治理源统一成 release candidate、handoff 和 reviewer-facing bundle。

- ``scripts/run_release_candidate_checks.ps1``：本地一站式 release candidate
  preflight。
- ``scripts/build_release_blocker_rollup_report.ps1``：统一汇总 release blocker、
  warning 和 action item。
- ``scripts/build_release_governance_pipeline_report.ps1``：生成 release governance
  pipeline summary。
- ``scripts/build_release_governance_handoff_report.ps1``：生成 release governance
  handoff summary。
- ``scripts/check_release_metadata_docs.ps1``：检查发布元数据文档的固定契约与
  reviewer-facing 入口。
- ``scripts/assert_release_material_safety.ps1``：检查发布材料安全边界。
- ``scripts/package_release_assets.ps1``：打包发布资产。
- ``scripts/publish_github_release.ps1``：发布 GitHub release。
- ``scripts/sync_github_release_notes.ps1``：同步 GitHub release notes。
- ``scripts/write_release_metadata_start_here.ps1``：生成发布材料入口
  ``START_HERE.md``。
- ``scripts/write_release_body_zh.ps1``：生成中文 release body 与短摘要。
- ``scripts/write_release_artifact_guide.ps1``：生成 artifact guide。
- ``scripts/write_release_artifact_handoff.ps1``：生成 release handoff 文档。
- ``scripts/write_release_reviewer_checklist.ps1``：生成 reviewer checklist。
- ``scripts/write_release_note_bundle.ps1``：生成 release note bundle。


PDF 保守维护入口
----------------

当前 PDF 不是主线扩展方向。这组脚本只用于保守维护、受控 smoke 和最终验证。

- ``scripts/check_pdf_dependency_inputs.ps1``：只读检查 PDFio / PDFium 本地输入
  是否满足受控 PDF 构建前置条件。
- ``scripts/check_pdf_release_readiness.ps1``：检查 PDF release readiness。
- ``scripts/check_pdf_visual_release_gate_preflight.ps1``：只读检查 PDF visual gate
  前置条件。
- ``scripts/check_pdf_controlled_visual_smoke.ps1``：运行受控 PDF visual smoke
  检查。
- ``scripts/run_pdf_ctest_bounded_subset.ps1``：运行受限 PDF CTest 子集。
- ``scripts/run_pdf_ctest_remaining_guarded.ps1``：运行剩余 PDF CTest 的受控入口。
- ``scripts/run_pdf_full_ctest_guarded.ps1``：完整 PDF CTest 的受控入口。
- ``scripts/run_pdf_visual_release_gate.ps1``：运行 PDF visual release gate。
- ``scripts/run_pdf_visual_full_gate_guarded.ps1``：受控运行完整 PDF visual gate。
- ``scripts/run_pdf_visual_segmented_resume.ps1``：分段恢复 PDF visual gate。


维护检查清单
------------

新增或调整脚本时，至少检查下面事项：

1. 脚本是否属于当前三条主线或明确的保守维护入口。
2. 是否有只读模式、``-SkipBuild`` 或轻量 fixture 路径。
3. 输出 JSON 是否有稳定 ``schema`` / ``schema_version``。
4. 是否有对应 PowerShell 回归或 docs contract。
5. 是否需要接入 ``test/CMakeLists.txt``，并设置 ``TIMEOUT 60``。
6. 是否需要在 ``docs/index.rst``、``README.md`` 或 release metadata 文档中
   暴露入口。
