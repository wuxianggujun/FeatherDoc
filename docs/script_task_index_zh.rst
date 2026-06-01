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

当前三条主线在本文中也作为 ``template_contract_project_template_workflow``、
``style_numbering_governance_workflow`` 和 ``table_layout_delivery_workflow``
的脚本索引锚点，供 ``current_direction_guardrails`` 契约测试交叉检查。


脚本索引自检
------------

- ``scripts/check_script_task_index.ps1``：只读检查本页列出的 ``scripts/*.ps1``
  和 ``scripts/*.py`` 路径是否真实存在，同时确认本文已经进入 Sphinx 首页、
  文档维护计划、项目评分建议和 CMake 轻量测试注册；默认同时写出
  ``summary.json`` 和 ``script_task_index_check.md``，让 CI / release 面板消费
  稳定 JSON，也让人工复核直接阅读 Markdown 摘要；可用 ``-SummaryJson`` /
  ``-ReportMarkdown`` 显式指定输出路径；同一个脚本路径被重复列出时也会失败，
  并在 JSON / Markdown 中写出 ``duplicate_script_reference_count``、
  ``occurrence_lines`` 和 ``occurrence_groups``，让维护者能直接定位重复行号；
  Markdown 报告同时写出 ``checked_at_utc``、``powershell_version`` 和
  ``output_encoding`` = ``UTF-8 without BOM``，固定运行环境与 JSON /
  Markdown 产物的无 BOM 输出边界，避免自动化重新猜测编码；还会写出
  ``documentation_entrypoint_count`` 和 ``documentation_entrypoints``，
  固定 README 入口必须同时指向文档维护总览与脚本任务索引；
  通过 ``script_reference_group_count`` 和 ``script_reference_groups`` 按本页
  小节汇总脚本覆盖量，并通过 ``script_reference_extension_count`` 和
  ``script_reference_extensions`` 汇总 ``.ps1`` / ``.py`` 入口分布，方便维护者
  快速发现某个分组或脚本类型的异常变化；同时写出
  ``repository_script_count``、``unindexed_script_count`` 和
  ``unindexed_scripts``，只读盘点 ``scripts`` 目录下尚未纳入本维护索引的入口，
  默认不把未索引脚本作为失败条件；需要把脚本索引升级为 CI gate 时，可显式传入
  ``-FailOnUnindexed``，并在 JSON / Markdown 中通过 ``fail_on_unindexed`` 固定本次运行
  是否启用该门禁。


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
- ``scripts/template_schema_cli_common.ps1``：提供模板 schema CLI 共享的路径、
  相对路径和命令行参数转义工具。
- ``scripts/run_template_validation_regression.ps1``：运行模板校验视觉回归。
- ``scripts/run_template_bookmark_multiline_visual_regression.ps1``：运行模板书签
  多行文本视觉回归。
- ``scripts/run_template_bookmark_paragraphs_visual_regression.ps1``：运行模板书签
  段落替换视觉回归。
- ``scripts/run_template_bookmark_paragraphs_pagination_visual_regression.ps1``：
  运行模板书签段落分页视觉回归。
- ``scripts/run_bookmark_block_visibility_visual_regression.ps1``：运行书签块显示 /
  隐藏视觉回归。
- ``scripts/run_bookmark_image_visual_regression.ps1``：运行书签图片替换视觉回归。
- ``scripts/run_bookmark_floating_image_visual_regression.ps1``：运行书签浮动图片
  视觉回归。
- ``scripts/run_bookmark_table_replacement_visual_regression.ps1``：运行书签表格
  替换视觉回归。
- ``scripts/run_template_table_cli_bookmark_visual_regression.ps1``：运行 template
  table CLI bookmark 路径视觉回归。
- ``scripts/run_template_table_cli_visual_regression.ps1``：运行 template table
  CLI 基础视觉回归。
- ``scripts/run_template_table_cli_selector_visual_regression.ps1``：运行
  template table CLI selector 视觉回归。
- ``scripts/run_template_table_cli_column_visual_regression.ps1``：运行 template
  table CLI 列操作视觉回归。
- ``scripts/run_template_table_cli_merge_unmerge_visual_regression.ps1``：运行
  template table CLI 合并 / 拆分视觉回归。
- ``scripts/run_template_table_cli_direct_visual_regression.ps1``：运行 template
  table CLI direct 路径基础视觉回归。
- ``scripts/run_template_table_cli_direct_column_visual_regression.ps1``：运行
  template table CLI direct 列操作视觉回归。
- ``scripts/run_template_table_cli_direct_merge_unmerge_visual_regression.ps1``：
  运行 template table CLI direct 合并 / 拆分视觉回归。
- ``scripts/run_template_table_cli_section_kind_visual_regression.ps1``：运行
  template table CLI section-kind 基础视觉回归。
- ``scripts/run_template_table_cli_section_kind_row_visual_regression.ps1``：运行
  template table CLI section-kind 行操作视觉回归。
- ``scripts/run_template_table_cli_section_kind_column_visual_regression.ps1``：
  运行 template table CLI section-kind 列操作视觉回归。
- ``scripts/run_template_table_cli_section_kind_merge_unmerge_visual_regression.ps1``：
  运行 template table CLI section-kind 合并 / 拆分视觉回归。
- ``scripts/run_project_template_smoke.ps1``：运行项目模板 smoke manifest。
- ``scripts/check_project_template_smoke_manifest.ps1``：校验项目模板 smoke
  manifest 结构、入口唯一性、启用检查项和 ``-CheckPaths`` 路径存在性。
- ``scripts/project_template_smoke_manifest_common.ps1``：提供项目模板 smoke
  manifest 共享的属性读取、路径解析和校验问题工具。
- ``scripts/describe_project_template_smoke_manifest.ps1``：只读描述项目模板
  smoke manifest 状态与维护入口；JSON 输出使用
  ``featherdoc.project_template_smoke_manifest_description.v1`` schema。
- ``scripts/discover_project_template_smoke_candidates.ps1``：发现尚未纳入
  manifest 的 ``.docx`` / ``.dotx`` 候选；支持 ``-FailOnUnregistered``
  作为 CI gating，并在 JSON 报告中写入 ``unregistered_candidate_count``。
- ``scripts/new_project_template_smoke_onboarding_plan.ps1``：生成项目模板
  smoke 候选接入计划，不直接修改 manifest。
- ``scripts/register_project_template_smoke_manifest_entry.ps1``：注册或用
  ``-ReplaceExisting`` 更新真实模板 smoke 条目。
- ``scripts/sync_project_template_schema_approval.ps1``：同步项目模板 schema
  approval 的 reviewer 决策与发布摘要。
- ``scripts/write_project_template_schema_approval_history.ps1``：汇总多次
  smoke / release summary 的 schema approval 历史。
- ``scripts/build_project_template_onboarding_governance_report.ps1``：生成
  onboarding governance 报告。
- ``scripts/build_project_template_delivery_readiness_report.ps1``：生成项目模板
  delivery readiness 报告。
- ``scripts/onboard_project_template.ps1``：为真实项目模板生成可复核的接入
  bundle，并在显式要求时注册 smoke manifest。


模板渲染与数据映射
------------------

这组脚本支撑从模板导出 render plan、映射业务数据、再回写文档的轻量链路。

- ``scripts/export_template_render_plan.ps1``：从模板导出 render plan。
- ``scripts/patch_template_render_plan.ps1``：把 patch 应用到 render plan。
- ``scripts/prepare_template_render_data_workspace.ps1``：准备数据映射工作区。
- ``scripts/export_render_data_mapping_draft.ps1``：导出初始 mapping draft。
- ``scripts/export_render_data_skeleton.ps1``：从 mapping draft 导出可编辑的
  业务数据 JSON skeleton。
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
  的视觉 verdict、``review_verdict=fail``、
  ``visual_review_undetermined_count`` 和发布材料。
- ``scripts/run_content_control_rich_replacement_visual_regression.ps1``：运行
  content-control 富文本替换视觉回归。
- ``scripts/run_content_control_image_replacement_visual_regression.ps1``：运行
  content-control 图片替换视觉回归。


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
  patch 置信度校准报告，并把 reviewer 分流固定为 ``source_report_display``、
  ``source_json_display`` 和 ``open_command`` 三段入口。
- ``scripts/write_style_merge_suggestion_review.ps1``：生成 style merge 建议复核
  材料。
- ``scripts/apply_reviewed_style_merge_suggestions.ps1``：应用已复核的 style merge
  建议。
- ``scripts/audit_style_merge_restore_plan.ps1``：只读审计 style merge restore plan，
  输出 ``review_handoff_steps``、``next_copy_command``、``issue_review_commands``、
  ``copy_issue_review_command``、``rollback_plan_summary``、
  ``restorable_rollback_command_summary``
  和 release governance action / blocker evidence，帮助 reviewer 从 dry-run
  结果进入 issue group 复核、Word 视觉审阅或选择性 restore。
- ``scripts/run_paragraph_list_visual_regression.ps1``：运行段落列表视觉回归。
- ``scripts/run_paragraph_numbering_visual_regression.ps1``：运行段落编号视觉回归。
- ``scripts/run_paragraph_style_numbering_visual_regression.ps1``：运行段落样式编号
  视觉回归。
- ``scripts/run_paragraph_run_style_visual_regression.ps1``：运行段落 run 样式视觉
  回归。
- ``scripts/run_ensure_style_visual_regression.ps1``：运行 ensure-style 写入和样式
  应用视觉回归。


表格与版式交付质量
------------------

这组脚本把表格样式、``tblLook``、浮动表格和版式质量接入治理材料。

- ``scripts/build_table_layout_delivery_report.ps1``：生成单文档 table layout
  delivery 报告。
- ``scripts/build_table_layout_delivery_rollup_report.ps1``：汇总多份 table layout
  delivery 报告。
- ``scripts/build_table_layout_delivery_governance_report.ps1``：把 rollup 晋升为
  release-facing governance summary。
  该 summary 必须保留 ``pdf_floating_table_support_coverage``、
  ``pdf_floating_table_reviewer_focus``、
  ``pdf_floating_table_metadata_only_fields``、
  ``pdf_floating_table_review_required_fields``、``metadata_only_fields``、
  ``review_required_fields`` 和 ``metadata-only tblpPr``，让 reviewer 能从脚本索引
  追到 PDF 浮动表的 ``stable_pdf_geometry_subset_not_full_word_wrapping`` 支持边界与
  元数据级复核要求。
- ``scripts/run_table_layout_delivery_report.ps1``：运行表格交付报告的本地入口。
- ``scripts/run_table_style_quality_visual_regression.ps1``：运行 table style
  quality 视觉回归。
- ``scripts/summarize_table_style_quality_pixels.py``：汇总 table style quality
  像素证据。
- ``scripts/run_table_row_visual_regression.ps1``：运行表格行新增、插入和删除
  视觉回归。
- ``scripts/run_table_row_height_visual_regression.ps1``：运行表格行高度视觉回归。
- ``scripts/run_table_row_repeat_header_visual_regression.ps1``：运行表格重复表头
  视觉回归。
- ``scripts/run_table_row_cant_split_visual_regression.ps1``：运行表格行禁止跨页
  拆分视觉回归。
- ``scripts/run_table_cell_width_visual_regression.ps1``：运行表格单元格宽度视觉
  回归。
- ``scripts/run_table_cell_margin_visual_regression.ps1``：运行表格单元格边距视觉
  回归。
- ``scripts/run_table_cell_fill_visual_regression.ps1``：运行表格单元格填充视觉
  回归。
- ``scripts/run_table_cell_border_visual_regression.ps1``：运行表格单元格边框视觉
  回归。
- ``scripts/run_table_cell_merge_visual_regression.ps1``：运行表格单元格合并视觉
  回归。
- ``scripts/run_table_cell_vertical_alignment_visual_regression.ps1``：运行表格
  单元格垂直对齐视觉回归。
- ``scripts/run_table_cell_text_direction_visual_regression.ps1``：运行表格单元格
  文字方向视觉回归。
- ``scripts/run_fixed_grid_merge_unmerge_regression.ps1``：运行 fixed-grid
  merge / unmerge 回归 bundle。
- ``scripts/run_section_page_setup_regression.ps1``：运行分节页设置回归 bundle。
- ``scripts/run_section_page_setup_visual_regression.ps1``：运行分节页设置视觉
  回归。
- ``scripts/run_section_order_visual_regression.ps1``：运行分节顺序视觉回归。
- ``scripts/run_section_part_refs_visual_regression.ps1``：运行分节 part
  references 视觉回归。
- ``scripts/run_section_text_multiline_visual_regression.ps1``：运行分节多行文本
  视觉回归。


Word 视觉验证与人工复核
-----------------------

这组脚本需要真实 ``Microsoft Word``，默认不属于低资源阶段的自动验证。

- ``scripts/run_word_visual_smoke.ps1``：生成基础 Word 渲染截图。
- ``scripts/run_word_visual_release_gate.ps1``：运行完整 Word visual release gate。
- ``scripts/check_word_visual_release_gate_preflight.ps1``：只读检查 release gate
  前置条件，不启动 Word。
- ``scripts/run_page_number_fields_regression.ps1``：运行页码字段回归 bundle。
- ``scripts/run_page_number_fields_visual_regression.ps1``：运行页码字段视觉回归。
- ``scripts/run_review_inspection_visual_regression.ps1``：运行审阅对象 inspection
  视觉回归。
- ``scripts/run_review_mutation_visual_regression.ps1``：运行审阅对象 mutation
  视觉回归。
- ``scripts/run_append_image_visual_regression.ps1``：运行追加图片视觉回归。
- ``scripts/run_replace_remove_image_visual_regression.ps1``：运行图片替换和移除
  视觉回归。
- ``scripts/run_extended_image_formats_visual_regression.ps1``：运行 SVG / WebP /
  TIFF 等扩展图片格式视觉回归。
- ``scripts/run_floating_image_z_order_visual_regression.ps1``：运行浮动图片层级顺序
  视觉回归。
- ``scripts/run_fill_bookmarks_visual_regression.ps1``：运行批量填充书签视觉回归。
- ``scripts/run_remove_bookmark_block_visual_regression.ps1``：运行移除书签块视觉
  回归。
- ``scripts/run_generic_fields_visual_regression.ps1``：运行通用字段占位和更新设置
  视觉回归。
- ``scripts/run_hyperlinks_visual_regression.ps1``：运行超链接显示文本视觉回归。
- ``scripts/run_omml_visual_regression.ps1``：运行 OMML 公式插入和替换视觉回归。
- ``scripts/run_run_font_language_visual_regression.ps1``：运行 run 字体语言视觉
  回归。
- ``scripts/run_semantic_diff_visual_regression.ps1``：运行语义 diff 文档对视觉
  回归。
- ``scripts/run_edit_document_from_plan_visual_regression.ps1``：运行编辑计划回写文档
  视觉回归。
- ``scripts/build_image_contact_sheet.py``：生成视觉回归 contact sheet。
- ``scripts/prepare_word_review_task.ps1``：准备人工视觉复核任务。
- ``scripts/open_latest_word_review_task.ps1``：打开最新复核任务。
- ``scripts/open_latest_fixed_grid_review_task.ps1``：打开最新 fixed-grid 复核任务。
- ``scripts/open_latest_section_page_setup_review_task.ps1``：打开最新 section/page setup 复核任务。
- ``scripts/open_latest_page_number_fields_review_task.ps1``：打开最新 page-number fields 复核任务。
- ``scripts/print_latest_fixed_grid_review_prompt.ps1``：打印最新 fixed-grid 复核提示。
- ``scripts/record_word_visual_review_result.ps1``：记录人工复核 verdict。
- ``scripts/sync_latest_visual_review_verdict.ps1``：同步最新视觉复核 verdict。
- ``scripts/sync_visual_review_verdict.ps1``：按显式路径同步视觉复核 verdict。
- ``scripts/find_superseded_review_tasks.ps1``：只读扫描复核任务目录，报告已被
  更新任务取代的旧任务。
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
- ``scripts/release_blocker_metadata_helpers.ps1``：提供 release blocker 元数据、
  action 指引和质量检查 helper。
- ``scripts/release_visual_metadata_helpers.ps1``：提供 release 视觉复核 metadata
  汇总、路径解析和任务 verdict helper。
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


构建与安装 smoke
----------------

这组脚本用于本地复用构建、安装包和外部 consumer 的轻量验证。

- ``scripts/run_reused_build_check.ps1``：复用 canonical 本地构建并带进程、
  内存和 per-test timeout guard 运行构建 / 测试。
- ``scripts/run_install_find_package_smoke.ps1``：安装 FeatherDoc 后验证外部
  ``find_package`` consumer 能正常配置和构建。


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
- ``scripts/check_pdf_controlled_visual_smoke.py``：执行受控 PDF visual smoke
  产物检查。
- ``scripts/check_pdf_text_layer.py``：检查 PDF text layer 产物。
- ``scripts/compare_pdf_reference_branch_manifest.ps1``：比较 PDF reference branch
  manifest。
- ``scripts/render_pdf_pages.py``：把 PDF 页面渲染成视觉证据图片。
- ``scripts/run_pdf_ctest_bounded_subset.ps1``：运行受限 PDF CTest 子集。
- ``scripts/run_pdf_ctest_remaining_guarded.ps1``：运行剩余 PDF CTest 的受控入口。
- ``scripts/run_pdf_full_ctest_guarded.ps1``：完整 PDF CTest 的受控入口。
- ``scripts/run_pdf_visual_release_gate.ps1``：运行 PDF visual release gate。
- ``scripts/run_pdf_visual_full_gate_guarded.ps1``：受控运行完整 PDF visual gate。
- ``scripts/run_pdf_visual_segmented_resume.ps1``：分段恢复 PDF visual gate。
- ``scripts/run_pdf_unicode_font_roundtrip_visual_regression.ps1``：运行 PDF Unicode
  字体 roundtrip 视觉回归。
- ``scripts/write_pdf_visual_gate_attempt_summary.ps1``：写出 PDF visual gate
  attempt summary。
- ``scripts/write_pdf_visual_release_gate_preflight_governance_report.ps1``：写出
  PDF visual release gate preflight governance 报告。
- ``scripts/write_pdf_visual_segmented_gate_summary.ps1``：写出 PDF visual
  segmented gate summary。


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
