文档维护与清理计划
==================

状态日期：2026-05-19

本页用于固定当前 ``dev`` 分支的文档维护边界，避免后续继续把旧
``codex/*`` 分支里的文档、PDF 样例或历史研究材料整批搬入。它只记录维护
策略，不代表已经完成完整构建、完整 CTest、Word release gate 或 PDF visual gate。


当前活动入口
------------

当前应优先维护这些文档入口：

1. ``docs/current_direction_zh.rst``：当前产品主线和功能准入标准。
2. ``docs/document_governance_acceptance_zh.rst``：文档治理阶段验收边界。
3. ``docs/release_metadata_pipeline_zh.rst``：发布治理、handoff、rollup 和
   reviewer-facing bundle 的元数据链路。
4. ``docs/release_metadata_maintenance_checklist_zh.rst``：release metadata 修改流程。
5. ``docs/automation/word_visual_workflow_zh.rst``：Word 视觉验证流程。
6. ``BUILDING_PDF.md`` 与 ``design/04-pdf-execution-plan.md``：PDF 保守维护和
   后续验证入口。

历史参考页可以继续保留，但不再作为首页主入口：

1. ``docs/document_api_mainline_status_zh.rst``：文档 API 主线和旧分支复核快照。
2. ``docs/branch_recovery_and_governance_sync_zh.rst``：工作区恢复和分支同步记录。
3. ``docs/stale_codex_branch_inventory_zh.rst``：远端 ``codex/*`` 参考分支库存。
4. ``docs/pdf_visual_validation_status_zh.rst``：PDF 可视化验证状态快照。
5. ``docs/v1_7_roadmap_zh.rst``：历史路线图归档。


当前推进顺序
------------

当前阶段继续按下面顺序推进：

1. 模板契约与项目模板工作流。
2. Content-control 修复和 Custom XML 绑定治理。
3. 样式、编号和真实语料置信度校准。
4. 表格与版式交付质量。
5. Release governance 材料一致性。
6. Word 最小可视化 smoke、``check_docx_functional_smoke_readiness.ps1`` 和后续受控 release gate。
7. PDF CJK 仅做保守维护、文档衔接和最终验证，不扩展为当前主线。


已完成的维护收口
----------------

当前 ``dev`` 已经完成这些轻量收口：

1. 文档接口主线已记录，确认公共 API 覆盖正式文档处理的核心链路。
2. 四个远端 ``codex/*`` 参考分支已完成多轮只读复核。
3. ``origin/codex/release-governance-warning-entrypoints`` 中的 checklist helper、
   warning/action/blocker 入口已由当前 ``dev`` 覆盖。
4. ``origin/codex/release-governance-rollup-details`` 中的 rollup、handoff、pipeline
   和 schema calibration routing 已由当前 ``dev`` 按新结构覆盖。
5. 两个 PDF CJK 分支中的 copy/search gate、CLI CJK export、字体 fallback、
   表头 fitting 等可低风险摘入能力已经在当前 ``dev`` 中有等价实现。
6. 最小 Word smoke 已生成证据，并完成只读证据质量复核；这不等同于完整
   ``run_word_visual_release_gate.ps1`` 通过。


旧参考分支处理规则
------------------

旧 ``codex/*`` 分支继续作为只读参考库存保留。后续不做整分支合并，原因是：

1. 旧分支会回退当前 ``dev`` 的恢复记录、文档主线记录和轻量验证状态。
2. PDF CJK 分支混合大量源码、manifest、视觉 baseline 和构建入口，风险高。
3. Release governance 分支的大部分目标已经被当前 ``dev`` 用更完整结构重做。
4. 当前阶段机器资源紧张，不适合把大批样例和重型验证重新引入主线。

如果后续确实要继续从旧分支取内容，应满足这些条件：

1. 只摘单个能力或单份文档，不整分支 merge。
2. 先确认当前 ``dev`` 没有等价实现。
3. 不删除现有恢复记录、验收记录、分支库存和视觉证据记录。
4. 源码改动必须配套轻量验证；PowerShell 测试单个 60 秒超时。
5. 有改动后先提交并推送 ``origin/dev``，再进入任何重型可视化验证。


LibreOffice PDF 研究文档
------------------------

当前 ``dev`` 仍保留 ``docs/libreoffice_pdf`` 研究文档，并在 ``docs/index.rst`` 中
保留入口。这些文档属于历史研究和路线决策背景，不是当前 PDF 主线。

后续不应在没有明确确认的情况下删除这些文件。若要清理，应先列出路径、影响范围和
风险，并获得明确确认。当前更稳妥的处理方式是：

1. 保留研究文档作为历史背景。
2. 在当前方向文档中明确 PDF 主线仍以现有 PDFio / PDFium 路线和保守维护为准。
3. 不把旧 ``codex/*`` 分支里的大批 PDF regression 样例、manifest 或视觉 baseline
   直接搬入 ``dev``。


验证边界
--------

低资源阶段默认只做这些检查：

1. ``git status --short --branch``。
2. ``git rev-list --left-right --count dev...origin/dev``。
3. ``git diff --check``。
4. 单个 PowerShell 脚本或测试，使用 60 秒超时。
5. 必要时只读检查 JSON、Markdown、RST、PNG 元数据和输出证据。

低资源阶段默认不运行：

1. CMake、Ninja、MSBuild 或完整 CTest。
2. Word、LibreOffice、浏览器。
3. PDF visual gate 或大规模 PDF 渲染。
4. 整分支 merge、强推、改写历史或删除分支。


下一步最小风险动作
------------------

下一步优先做三件事：

1. 继续保持 ``dev`` 与 ``origin/dev`` 对齐，任何小改动后及时提交推送。
2. 若资源允许，先补 Word release gate 的更受控前置检查；完整 gate 仍需等工作区干净、
   源码已推送且本机资源稳定。
3. PDF visual gate 继续后置，只验证当前 ``dev`` 已有能力，不从旧分支搬入大批样例。


删除规则
--------

删除文件或目录属于高风险操作。后续如果还有删除候选，必须先明确说明：

1. 删除路径。
2. 影响范围。
3. 可恢复方式。
4. 为什么保留会造成实际维护风险。

只有用户明确回复“是”、“确认”或“继续”后，才能执行删除。
