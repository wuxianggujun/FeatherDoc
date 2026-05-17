文档维护与清理计划
==================

本页记录当前文档体系的维护状态、下一步可推进任务，以及需要删除的
过时文档候选。

状态日期：2026-05-17。


活动文档入口
------------

当前应优先维护这些文档入口：

- ``docs/current_direction_zh.rst``：项目近期产品主线与功能准入标准。
- ``docs/release_metadata_pipeline_zh.rst``：发布治理、handoff、rollup 和
  reviewer-facing bundle 的元数据链路。
- ``docs/release_policy_zh.rst``：发布前检查和人工复核规则。
- ``docs/automation/word_visual_workflow_zh.rst``：Word 视觉验证流程。
- ``BUILDING_PDF.md``：PDFio / PDFium 构建、测试和视觉验证 runbook。
- ``design/02-current-roadmap.md``：PDF 当前路线和阶段边界。
- ``design/04-pdf-execution-plan.md``：PDF 执行记录、验收命令和下一阶段入口。
- ``design/05-pdf-rtl-table-cell-complex-task-plan.md``：RTL table cell complex
  task 的执行源与收口记录。


下一步推进建议
--------------

当前不建议继续无边界扩展 PDF RTL table cell normalizer。更值得推进的任务按优先级是：

1. 发布治理 handoff 明细持续维护

   - handoff details parity 已完成首轮收口；``release_governance_handoff``、
     ``release_blocker_rollup``、``pipeline stages[]``、release summary、
     final review、artifact guide、reviewer checklist 和 handoff markdown
     已覆盖同一批 reviewer-facing 明细。
   - 多项目 schema approval 定位也已完成首轮收口；项目模板治理项会继续透传
     ``project_id`` 和 ``template_name``。
   - 后续新增治理源时继续固定 ``source_schema``、``source_report_display``、
     ``source_json_display`` 和 action item ``open_command`` 的展示规则。
   - release note bundle 入口现在会强制校验这些字段；缺字段的 rollup、
     handoff 或 pipeline stage 明细不会继续生成 reviewer-facing bundle。
   - 目标是让 reviewer 不再只看到计数，而能直接打开证据和复核命令。

2. 模板契约和真实项目模板语料

   - 扩大真实业务模板 smoke manifest。
   - 优先校准真实业务模板 schema patch 的 rename / update / remove 建议置信度，
     并继续验证 approval outcome 如何进入 blocker / warning / action item 分流。
   - content-control data-binding governance 和 onboarding governance 继续作为发布治理
     消费端；schema calibration 已把 invalid approval record 从 warning 升级为
     release blocker。
   - 目标是让模板接入、校验、生成、回归、发布形成稳定闭环。

3. 样式与编号治理

   - 强化 merge restore 冲突处理。
   - 继续做真实语料下的重复样式 merge 建议置信度校准。
   - 衔接 heading / list / theme 的批量治理入口。

4. 表格与版式交付质量

   - 补 custom table style property editing 覆盖面。
   - 深化 floating table 的 wrap distance、overlap control 和 ``w:tblpPr`` 细节。
   - 让 table layout delivery rollup 更稳定地进入发布面板。

5. PDF 读写剩余验收

   - 保留当前 PDFio / PDFium 路线。
   - 对中文 PDF 在常见阅读器中的复制、搜索能力补可复现验收。
   - RTL table cell 仅在出现新的 Unicode class、PDFium raw 形态或用户可见导入风险时再扩展。

本轮已把上述判断同步到 ``docs/current_direction_zh.rst``、
``docs/feature_gap_analysis_zh.rst``、``docs/release_metadata_pipeline_zh.rst``、
``design/02-current-roadmap.md`` 和 ``design/04-pdf-execution-plan.md``。
``release governance handoff details parity`` 和“多项目 schema approval 审计与
发布门禁回放”均已完成首轮实施和回归验证；schema confidence calibration 分流也已把
invalid approval record 升级为 release blocker。下一轮不要继续扩大 PDF RTL
normalizer，也不要先铺散到样式编号；优先推进真实业务模板 schema patch 置信度校准与
复核分流。


已完成任务包
------------

任务名：多项目 schema approval 审计与发布门禁回放。

本轮已经把单模板、单项目的 schema approval evidence 扩展成可在发布面板中按项目和
模板定位的审计链路。reviewer 可以从 release blocker rollup、handoff 和
reviewer-facing bundle 直接看出：哪个项目、哪个模板、哪次 approval history 导致
发布阻断，以及应该打开哪条命令复核。

完成内容：

- ``write_project_template_schema_approval_history.ps1`` 按 ``project_id`` /
  ``template_name`` / approval item name 生成复合 entry history，避免不同项目的
  同名模板互相合并。
- onboarding governance、delivery readiness、release blocker rollup 和 governance
  handoff 继续透传 ``project_id``、``template_name``、``source_schema``、
  ``source_report_display``、``source_json_display`` 与 action item ``open_command``。
- reviewer-facing bundle 会在 handoff details 中渲染
  ``project_template: project_id=... template_name=...``。
- malformed / count mismatch 这类 rollup 自身生成的 warning 也会补空项目和模板字段，
  避免 StrictMode 下 Markdown 渲染失败。

已验证：

.. code-block:: powershell

   ctest --test-dir .bpdf-roundtrip-msvc -R "^(write_project_template_schema_approval_history|build_project_template_onboarding_governance_report_aggregate|build_project_template_delivery_readiness_report_aggregate|build_release_governance_handoff_report_aggregate|build_release_governance_handoff_report_include_rollup|release_candidate_blocker_rollup|release_candidate_governance_handoff)$" --output-on-failure --timeout 60
   ctest --test-dir .bpdf-roundtrip-msvc -R "(release_candidate|release_note_bundle|release_metadata|release_governance|release_blocker|governance_handoff|governance_pipeline)" --output-on-failure --timeout 60


下一轮执行任务包
----------------

任务名：真实业务模板 schema patch 置信度校准与复核分流。

目标是把当前受控 fixture 推进到更接近真实业务模板的 schema patch 校准场景。重点是让
rename / update / remove 类 schema patch 建议带着置信度、人工 approval outcome 和
release blocker 证据进入发布治理链路。

建议按下面顺序实施：

1. 选择或构造两到三个真实业务模板语料：发票、合同、制度文档或项目报告优先。
2. 为每个语料生成 schema patch candidate，覆盖 rename、type update、
   required change、slot remove / add 等常见变更类型。
3. 扩展 ``write_schema_patch_confidence_calibration_report.ps1`` 回归，覆盖
   approved、pending、rejected 和 invalid approval record。
4. 确认 confidence calibration 的 blocker / warning / action item 继续保留
   ``project_id``、``template_name``、``source_schema``、``source_json_display`` 和
   action item ``open_command``。
5. 让 release blocker rollup、governance handoff 和 release note bundle 继续消费这些
   真实语料校准结果，避免发布面板只能看到“schema confidence gate failed”。
6. 同步 ``docs/feature_gap_analysis_zh.rst``、``docs/release_metadata_pipeline_zh.rst``、
   ``design/04-pdf-execution-plan.md`` 和本页。

建议验证命令：

.. code-block:: powershell

   ctest --test-dir .bpdf-roundtrip-msvc -R "^(write_schema_patch_confidence_calibration_report|build_project_template_onboarding_governance_report_aggregate|build_project_template_delivery_readiness_report_aggregate|release_candidate_blocker_rollup|release_candidate_governance_handoff)$" --output-on-failure --timeout 60
   ctest --test-dir .bpdf-roundtrip-msvc -R "(release_candidate|release_note_bundle|release_metadata|release_governance|release_blocker|governance_handoff|governance_pipeline)" --output-on-failure --timeout 60

完成标准：

- 真实业务模板语料进入 schema patch calibration 回归。
- calibration blocker / warning / action item 能按项目、模板、candidate 类型定位。
- reviewer-facing 字段在 rollup、handoff、release summary 和 bundle 中保持一致。
- ``git diff --check`` 通过。


已删除的过时文档
----------------

以下文档属于历史研究记录，已经从 Sphinx 文档树中删除：

- ``docs/libreoffice_pdf/index_zh.rst``
- ``docs/libreoffice_pdf/study_plan_zh.rst``
- ``docs/libreoffice_pdf/source_map_zh.rst``
- ``docs/libreoffice_pdf/pdf_pipeline_notes_zh.rst``
- ``docs/libreoffice_pdf/migration_gap_notes_zh.rst``

删除理由：

- 当前 PDF 主线已经稳定在 PDFio / PDFium 路线。
- LibreOffice PDF 文档停留在 2026-05-05 的第一轮源码粗扫。
- 文档内容仍在评估“是否切换到 LibreOffice 方案”，与当前执行路线不一致。
- 关键策略背景已经由 ``design/00-vision.md`` 和 ``design/dependencies/pdfio.md``
  保留，不需要继续把这组研究笔记放在 Sphinx 主文档入口。

本轮已同步修改：

- 从 ``docs/index.rst`` 的 toctree 中移除 ``libreoffice_pdf/index_zh``。
- 删除 ``docs/index.rst`` 中的 LibreOffice PDF Study Notes 段落。
- 删除 ``docs/libreoffice_pdf`` 下 5 份历史研究文档。


暂不删除的文档
--------------

以下文档虽然包含长期评估或历史背景，但仍是当前设计依据，不应删除：

- ``design/00-vision.md``：保留“不重写 LibreOffice / 不盲目迁移”的决策依据。
- ``design/dependencies/pdfio.md``：保留 PDFio 与 LibreOffice 写出器能力对比。
- ``design/dependencies/pdfium.md``：保留 PDFium provider、构建和 CMake 接入信息。
- ``design/dependencies/skia.md``：保留 Skia / SkPDF 长期评估，不进入近期执行线。


后续删除执行规则
----------------

删除文件属于高风险操作。后续如果还有新的删除候选，执行删除前必须明确
列出路径、影响范围和风险，并获得“是 / 确认 / 继续”等明确确认。
