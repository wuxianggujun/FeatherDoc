文档接口主线现状记录（中文）
============================

记录日期：2026-05-18

契约标记：``document_api_mainline_status.v1``、
``document_api_status_current_dev``、``codex_branch_reference_only``、
``pdf_cjk_branch_deferred``。

本文用于回答两个容易混淆的问题：

1. 文档接口是不是还没有做完。
2. 旧 ``codex/*`` 分支里的功能是不是已经全部进入 ``dev``。

结论是：文档接口主线已经在 ``dev`` 上具备完整的核心交付面；旧分支没有全部合入，
也不应该再整分支合并。后续应继续从当前 ``dev`` 出发，按功能主题小步复核、重做和
验证，而不是回放旧分支提交。


文档接口不是空白状态
--------------------

当前 ``dev`` 上的公共接口已经覆盖正式文档处理的核心链路。代表性入口包括：

- ``Document`` 文档对象。
- ``fill_bookmarks`` 书签填充。
- ``list_content_controls`` content control 盘点。
- ``sync_content_controls_from_custom_xml`` Custom XML 绑定同步。
- ``replace_content_control_text_by_tag`` 和
  ``replace_content_control_text_by_alias``。
- ``replace_content_control_with_paragraphs_*``、
  ``replace_content_control_with_table_rows_*``、
  ``replace_content_control_with_table_*`` 和
  ``replace_content_control_with_image_*``。
- ``validate_template_schema`` 模板契约校验。
- ``onboard_template`` 项目模板接入。
- ``export_numbering_catalog`` 和 ``import_numbering_catalog``。
- ``ensure_table_style`` 表格样式定义入口。

因此，当前阶段继续推进“文档功能”，不是因为接口还没有开始做，而是因为发布前还需要
把契约一致性、治理报告、真实语料校准和交付材料继续收口。


脚本与测试证据
--------------

当前主线仍保留了围绕文档接口的轻量脚本和回归入口。代表性入口包括：

- ``scripts/edit_document_from_plan.ps1``。
- ``scripts/build_content_control_data_binding_governance_report.ps1``。
- ``scripts/check_template_schema_manifest.ps1``。
- ``scripts/check_template_schema_baseline.ps1``。
- ``scripts/check_numbering_catalog_manifest.ps1``。
- ``scripts/check_numbering_catalog_baseline.ps1``。
- ``scripts/audit_style_merge_restore_plan.ps1``。
- ``scripts/apply_reviewed_style_merge_suggestions.ps1``。
- ``scripts/write_style_merge_suggestion_review.ps1``。

代表性测试包括：

- ``test/edit_document_from_plan_test.ps1``。
- ``test/edit_document_from_plan_content_control_sync_test.ps1``。
- ``test/edit_document_from_plan_content_control_text_aliases_test.ps1``。
- ``test/edit_document_from_plan_numbering_catalog_test.ps1``。
- ``test/build_content_control_data_binding_governance_report_test.ps1``。
- ``test/apply_reviewed_style_merge_suggestions_test.ps1``。
- ``test/audit_style_merge_restore_plan_test.ps1``。

这些入口说明文档接口不是只停留在头文件声明，而是已经进入脚本、治理报告和轻量回归
链路。


旧分支没有全部合入
------------------

旧 ``codex/*`` 分支目前只作为参考库存处理。已确认安全进入 ``dev`` 的内容是小范围
治理片段，例如 schema calibration candidate routing 和 release blocker rollup
复合 id 断言修复。

剩余旧分支仍有独有提交，但不能直接整分支合并：

- ``codex/release-governance-warning-entrypoints``：旧治理入口与当前 ``dev`` 的
  warning metadata、style merge governance、release rollup / handoff / pipeline
  明细实现重叠较深。
- ``codex/release-governance-rollup-details``：部分 schema calibration 和 release
  rollup 能力已经按当前契约重做，旧提交继续回放会带来冲突和回退风险。
- ``codex/pdf-cjk-copy-search-gate``：属于 PDF CJK copy/search、font matrix 和
  text-layer gate 深水区。
- ``codex/pdf-cjk-bullet-fallback``：继续扩展 PDF CJK bullet、numbered list 和字体
  fallback，仍属于 PDF 深水区。

这些分支不能被误认为“已经全部合入”，也不能在没有明确归档或废弃决定前删除。


后续推进原则
------------

后续工作按下面顺序推进：

1. 继续以 ``dev`` 为唯一开发主线。
2. 文档接口主线优先做模板契约、content-control 修复、样式与编号治理、表格版式交付
   质量和 release governance 材料一致性。
3. 如旧分支里还有价值功能，只从当前 ``dev`` 出发手工复核并重做小块，不做整分支
   merge。
4. PDF CJK 分支暂时冻结为参考库存，只做保守维护和文档衔接，不扩展为当前主线。
5. 每次改动后优先运行低资源脚本测试，并及时提交、推送 ``origin/dev``。


低资源验证边界
--------------

当前机器资源紧张时，验证默认遵守以下边界：

- 不运行 CMake、Ninja、MSBuild 或完整 CTest。
- 不启动 Word、LibreOffice、浏览器或 PDF 渲染器。
- PowerShell 单个测试使用 60 秒超时。
- 只关闭明确由当前任务启动且已经不用的进程。
- 外部进程只报告，不擅自结束。


最终重新测试与可视化验证入口
----------------------------

四个远端 ``codex/*`` 分支已经完成低资源只读复核。当前结论是：仍保留这些分支作为
参考库存，但不再整分支合并，也没有新的低风险源码小补丁需要从旧分支直接搬入。

进入最终重新测试前，必须先满足以下前置条件：

1. 当前工作区位于 ``dev``，且 ``dev`` 与 ``origin/dev`` 对齐。
2. ``git status --short --branch`` 显示工作区干净。
3. 最近一轮低资源文档、脚本或源码改动已经提交并推送。
4. 先确认本机资源和残留进程；只关闭由当前任务启动且已经不用的进程。

最终重新测试应分阶段执行，先轻后重：

1. 先运行 PowerShell 契约测试，每个测试单独设置 60 秒超时。
2. 再运行必要的源码级或脚本级静态检查，例如 ``git diff --check`` 和 PowerShell
   parser 检查。
3. 只有在源码已提交推送、工作区干净、资源情况允许时，才进入截图级 Word
   visual validation 或 PDF 可视化验证。
4. PDF CJK 可视化验证只验证当前 ``dev`` 已有能力，不直接搬入旧分支的大批
   regression 样例、manifest 或视觉 baseline。

如果机器仍然卡顿，下一轮只做轻量验证清单准备，不启动 Word、LibreOffice、浏览器、
PDF 渲染或完整构建。


最终轻量测试清单
----------------

最终重新测试前，先执行一轮不依赖 Office、PDF 渲染器或完整构建的轻量清单。该清单用于
确认文档接口主线和 release governance 材料没有明显脚本回退，也用于判断是否可以进入
后续可视化验证。

第一阶段固定入口：

1. ``git diff --check``。
2. PowerShell parser 检查关键脚本，确认语法层面没有破坏。
3. ``test/check_release_metadata_docs_test.ps1``。
4. ``test/release_governance_warning_helper_contract_test.ps1``。
5. ``test/build_release_blocker_rollup_report_test.ps1 -Scenario passing``。
6. ``test/build_release_governance_handoff_report_test.ps1 -Scenario aggregate``。
7. ``test/build_release_governance_pipeline_report_test.ps1 -Scenario aggregate``。
8. ``test/write_schema_patch_confidence_calibration_report_test.ps1 -Scenario aggregate``。

第二阶段可选补充入口：

1. ``test/build_content_control_data_binding_governance_report_test.ps1``。
2. ``test/build_project_template_delivery_readiness_report_test.ps1``。
3. ``test/build_table_layout_delivery_governance_report_test.ps1``。

执行规则：

1. 每个 PowerShell 测试必须单独设置 60 秒超时。
2. 不并发启动大批测试；资源紧张时按上述顺序串行执行。
3. 如果机器仍然卡顿，只维护清单和记录，不启动 Word、LibreOffice、浏览器、PDF 渲染、
   CMake、CTest、Ninja 或 MSBuild。
4. 只有轻量清单通过、源码已提交推送、工作区干净且资源允许时，才进入截图级 Word
   visual validation 或 PDF 可视化验证。


2026-05-19 低资源验证结果
-------------------------

本轮已按轻量清单完成脚本级验证，未启动 CMake、CTest、Ninja、MSBuild、Word、
LibreOffice、浏览器或 PDF 渲染。

已通过的固定入口：

1. ``git diff --check``。
2. 10 个 release governance 相关 PowerShell 脚本和测试的 parser 检查。
3. ``test/check_release_metadata_docs_test.ps1``。
4. ``test/release_governance_warning_helper_contract_test.ps1``。
5. ``test/build_release_blocker_rollup_report_test.ps1 -Scenario passing``。
6. ``test/build_release_governance_handoff_report_test.ps1 -Scenario aggregate``。
7. ``test/build_release_governance_pipeline_report_test.ps1 -Scenario aggregate``。
8. ``test/write_schema_patch_confidence_calibration_report_test.ps1 -Scenario aggregate``。

已通过的第二阶段补充入口：

1. 6 个文档治理相关 PowerShell 脚本和测试的 parser 检查。
2. ``test/build_content_control_data_binding_governance_report_test.ps1 -Scenario aggregate``。
3. ``test/build_project_template_delivery_readiness_report_test.ps1 -Scenario aggregate``。
4. ``test/build_table_layout_delivery_governance_report_test.ps1 -Scenario aggregate``。

剩余边界：

1. Word visual validation 和 PDF 可视化验证仍未执行。
2. PDF CJK 两个旧分支仍保留为参考库存，不在本轮低资源验证中扩展。
3. 进入可视化验证前仍需重新确认 ``dev`` 与 ``origin/dev`` 对齐、工作区干净和本机资源
   状态。


2026-05-19 可视化验证前置检查
------------------------------

本轮只读复核了当前仓库已有的可视化验证入口，未启动 Word、LibreOffice、浏览器或 PDF
渲染。

当前主要入口：

1. Word release gate：``scripts/run_word_visual_release_gate.ps1``。
2. Word review verdict 同步：``scripts/sync_latest_visual_review_verdict.ps1`` 和
   ``scripts/sync_visual_review_verdict.ps1``。
3. PDF visual gate：``scripts/run_pdf_visual_release_gate.ps1``。
4. 任务流程说明：``docs/automation/word_visual_workflow_zh.rst``。

执行前置条件：

1. ``dev`` 与 ``origin/dev`` 对齐，且工作区干净。
2. 本机没有明显高负载的外部 Office、浏览器、PDF 或构建进程；外部进程不由当前任务
   擅自关闭。
3. Word 可视化验证优先使用 ``-SkipBuild`` 复用已有 build 目录，不在验证阶段触发
   CMake、Ninja、MSBuild 或完整 CTest。
4. PDF 可视化验证只在资源充足时执行；该入口可能创建本地 Python 渲染环境或安装
   Pillow / PyMuPDF，因此不适合在机器卡顿时运行。

建议执行顺序：

1. 先只做 Word gate 的最小范围 smoke / release gate 预检，输出到新的 ``output``
   子目录，避免覆盖已有证据。
2. Word 证据生成并人工或自动记录 verdict 后，再运行 latest verdict sync。
3. PDF CJK 只验证当前 ``dev`` 已有能力；不搬入旧分支的大批 regression 样例、manifest
   或视觉 baseline。
4. 任一可视化阶段失败时，先提交失败记录和复现命令，不继续叠加重型验证。


2026-05-19 Word 可视化资源预检
-------------------------------

本轮在用户关闭部分后台程序后做了最小资源预检，但仍未运行完整 Word visual smoke、
release gate 或 PDF visual gate。

已确认：

1. ``dev`` 与 ``origin/dev`` 对齐，工作区干净。
2. ``Microsoft Word`` COM 可创建并退出。
3. 本轮启动的 ``WINWORD`` 进程已在预检结束后关闭。

阻塞完整 Word smoke 的前置条件：

1. 当前工作区没有可直接复用的跟踪内 ``.docx`` 输入。
2. 现有 ``build`` 目录中未找到 ``featherdoc_visual_smoke_tables`` 可执行文件。
3. ``PATH`` 中没有 ``python``；Codex bundled Python 存在，但缺少 ``fitz`` / PyMuPDF。

因此，当前不能在低资源约束下直接运行 ``scripts/run_word_visual_smoke.ps1`` 或
``scripts/run_word_visual_release_gate.ps1``。否则会触发构建、创建 Python 环境或安装
渲染依赖。后续若要继续可视化验证，最小风险路径是先提供或复用一个现成 DOCX，并准备
已带 ``PIL`` 与 ``fitz`` 的 Python，再只跑 ``run_word_visual_smoke.ps1 -InputDocx ...``
这一条最小链路。


2026-05-19 最小 Word smoke 结果
-------------------------------

本轮按最小风险路径完成了一次 Word 可视化 smoke 预检。

执行内容：

1. 创建并复用本地忽略目录 ``.venv-word-visual-smoke``，安装 ``Pillow`` 和 ``PyMuPDF``。
2. 用 OpenXML ZIP 方式生成临时输入 ``output/word-visual-input/minimal-word-visual-input.docx``，
   避免再次用 Word COM 生成 DOCX。
3. 运行 ``scripts/run_word_visual_smoke.ps1 -InputDocx ...``，输出到
   ``output/word-visual-smoke-minimal-20260519``。

结果：

1. Word 成功将 DOCX 导出为 PDF。
2. PyMuPDF 成功渲染 PDF 为 PNG。
3. ``summary.json`` 报告 ``page_count = 1``。
4. 生成了 ``evidence/contact_sheet.png`` 和 ``evidence/pages/page-01.png``。
5. 生成了 ``report/summary.json``、``review_checklist.md``、``review_result.json`` 和
   ``final_review.md``。

资源清理：

1. 前一次用 Word COM 直接保存 DOCX 的尝试卡住，已关闭本轮启动的 ``WINWORD`` 和对应
   PowerShell 进程。
2. 最小 smoke 完成后，未发现本轮遗留的 ``WINWORD`` 或 ``python`` 进程。
3. 可视化证据和渲染虚拟环境位于 ``.gitignore`` 覆盖目录，不纳入提交。

剩余边界：

1. 这只是最小 Word smoke，不是完整 ``run_word_visual_release_gate.ps1``。
2. 完整 release gate 仍应等资源稳定后再按 ``-SkipBuild`` 和分阶段策略执行。
3. PDF visual gate 仍未执行。
