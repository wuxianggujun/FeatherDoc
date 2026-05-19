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


2026-05-19 最小 Word smoke 证据复核
------------------------------------

本轮只做上一轮最小 Word smoke 输出的只读证据复核，未重新启动 Word、LibreOffice、
浏览器、CMake、CTest、Ninja、MSBuild 或 PDF visual gate。

已确认：

1. ``report/summary.json`` 可读取，且 ``page_count = 1``。
2. ``evidence/contact_sheet.png`` 存在，尺寸为 ``408 x 549``，文件非空，灰度范围为
   ``6..255``。
3. ``evidence/pages/page-01.png`` 存在，尺寸为 ``1224 x 1584``，文件非空，灰度范围为
   ``0..255``。
4. ``report/review_result.json`` 与 ``report/final_review.md`` 可读取，当前 verdict 仍为
   ``pending_manual_review``。
5. 复核后未发现本轮遗留的 ``WINWORD`` 或 ``python`` 进程；只看到外部 ``node`` /
   ``powershell`` 进程，未擅自关闭。

剩余边界：

1. 这次复核确认的是最小 smoke 证据质量，不等同于完整 Word release gate 通过。
2. ``scripts/run_word_visual_release_gate.ps1`` 仍未执行。
3. ``scripts/run_pdf_visual_release_gate.ps1`` 仍未执行；PDF CJK 分支继续作为参考库存冻结。


2026-05-19 Word smoke 复跑与目检
---------------------------

本轮在 ``dev`` 与 ``origin/dev`` 继续对齐、工作区干净、重型进程空闲后，复跑了一次
最小 Word 可视化 smoke。输入复用现成的
``output/word-visual-input/minimal-word-visual-input.docx``，执行时使用本地
``.venv-word-visual-smoke``，没有触发构建。

已确认：

1. ``summary.json`` 报告 ``page_count = 1``。
2. ``contact_sheet.png`` 和 ``page-01.png`` 可读，文本与表格都正常显示。
3. ``review_result.json`` 仍为 ``pending_manual_review``，但肉眼目检未见空白页、乱码或
   明显布局异常。
4. 本轮结束后未发现 ``WINWORD`` 或 ``python`` 残留进程。


2026-05-19 PDF 新功能整合结论
-----------------------------

本轮继续按低资源方式复核 PDF 主线，没有运行 CMake、CTest、Ninja、MSBuild、
Word、LibreOffice、浏览器或 PDF 渲染。结论是：PDF 分支里的核心小能力已经按当前
``dev`` 结构陆续重做进主线，但两个旧 ``codex/pdf-*`` 分支并没有、也不应该被整分支
合并。

当前 ``dev`` 已确认包含的 PDF 能力包括：

1. CJK copy/search 的 text-layer gate 入口：``scripts/check_pdf_text_layer.py`` 与
   ``scripts/run_pdf_visual_release_gate.ps1`` 中的 ``cjk-copy-search`` 链路。
2. CLI CJK PDF export 覆盖：``test/pdf_cli_export_tests.cpp`` 中已覆盖
   ``--cjk-font-file``、字体子集开关和显式 CJK 字体导出路径。
3. East Asia / CJK 字体 fallback：``src/pdf/pdf_font_resolver.cpp`` 已包含 FreeType
   字形检查、Unicode prefix 到 East Asia / CJK 字体链路的 fallback，以及日文假名、
   韩文音节和东亚兼容符号识别。
4. CJK bullet / East Asia 字体回退契约：``test/pdf_font_resolver_tests.cpp`` 与
   ``test/pdf_document_adapter_font_tests.cpp`` 已保留对应测试入口。
5. PDF 表格分页与表头 fitting 小修复：当前实现使用 ``spanned_row_bottom`` 计算纵向
   合并或跨行单元格的实际输出高度，避免页底分页判断低估跨行表格。
6. PDF 页眉页脚段落 run 保留与对齐补丁：当前实现已保留页眉页脚段落中的 run 元数据，
   并携带对齐后的内部布局信息。
7. PDF visual release gate 的样式与 text-shaping 静态契约：manifest 中保留
   style/text-shaping baseline 标记，release gate 会携带 ``visual_style_focus``、
   ``matched_text`` 与 ``missing_text`` 等证据字段。

仍未合入、继续只读保留在旧 PDF 分支中的内容主要是：

1. 大批 CJK PDF regression 样例。
2. 批量 manifest 条目。
3. 视觉 baseline 图片和完整视觉 gate 扩展。
4. 需要构建、渲染、PDF 可视化复核或更大资源预算才能验证的深水区改动。

本轮轻量验证通过：

1. ``git diff --check``。
2. ``test/pdf_visual_release_gate_text_shaping_baselines_test.ps1``，单独 60 秒超时。
3. ``test/pdf_visual_release_gate_style_baselines_test.ps1``，单独 60 秒超时。

因此，对“PDF 新功能是否都添加成功”的准确回答是：核心源码能力、CLI 覆盖、字体回退、
表格分页、页眉页脚 run 保留和静态 gate 契约已经进入 ``dev``；旧分支里重型的样例库、
manifest 扩展和视觉 baseline 还没有全部添加，这部分需要后续作为 PDF 专项，在资源允许时
分批重做和可视化验证，不能直接整分支搬入。


2026-05-19 继续整合筛选结果
---------------------------

本轮在 ``64425c1`` 之后继续更新远端引用并复核四个 ``origin/codex/*`` 参考分支，仍然
没有整分支合并、强推、改写历史或删除分支。

筛选结果：

1. ``origin/codex/release-governance-warning-entrypoints`` 中最小的
   ``d10f8d8`` 方向已经被当前 ``dev`` 覆盖，并且当前实现比旧分支更完整：
   ``release_blocker_metadata_helpers.ps1`` 与
   ``release_governance_warning_helper_contract_test.ps1`` 不只保留
   ``source_report_display`` / ``source_json_display``，还继续保留
   ``project_id``、``template_name``、``candidate_type``、``repair_strategy``、
   ``command_template`` 以及 ``command`` / ``open_command`` / ``audit_command`` /
   ``review_command`` 等后续治理字段。因此旧分支版本不能直接摘入，否则会削弱当前
   ``dev`` 的元数据契约。
2. ``origin/codex/release-governance-rollup-details`` 剩余提交的目标也已由当前
   ``dev`` 的 release blocker rollup、handoff、pipeline 和 schema confidence
   calibration 链路覆盖；该分支仍会回退当前恢复记录、PDF CJK 复核记录和 release
   warning 契约，因此继续只读保留。
3. 两个 PDF CJK 分支的剩余差异仍以大批 regression 样例、manifest、visual baseline
   和重型 gate 为主；核心小能力已在当前 ``dev`` 重做，剩余部分不进入低资源整合。

本轮轻量验证通过：

1. ``test/release_governance_warning_helper_contract_test.ps1``，单独 60 秒超时。

当前建议是：后续继续整合时，不再从这些旧分支里寻找整分支级合并机会；只在发现新的、
明确小于当前实现且不会回退文档恢复记录或治理元数据契约的补丁时，才按当前 ``dev``
结构手工重做。


2026-05-19 治理契约补充验证
---------------------------

本轮继续收尾 ``release-governance-*`` 参考分支的低风险验证，没有修改源码，也没有运行
CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。

已补充验证：

1. ``test/release_governance_warning_contract_test.ps1``，单独 60 秒超时。该测试确认
   当前 ``scripts`` 下所有字面 ``warnings.Add([ordered]@{ ... })`` 块仍保留
   ``id``、``action``、``message`` 和 ``source_schema`` 等必要字段。
2. ``test/release_governance_metrics_contract_test.ps1``，单独 60 秒超时。该测试确认
   release handoff、release blocker rollup、package manifest、安全审计和治理验收文档
   仍保留 real-corpus confidence、delivery quality、content-control repair workflow、
   numbering alignment、table layout delivery quality 和低资源 PDF 保守边界等契约标记。

因此，``release-governance-warning-entrypoints`` 和
``release-governance-rollup-details`` 的剩余差异目前没有新的低风险源码补丁需要搬入；
当前 ``dev`` 的治理契约已通过 warning helper、warning contract 和 metrics contract
三条轻量验证链路。后续最小风险动作是继续维持这些分支为只读参考，除非发现新的独立小补丁。


2026-05-19 PDF 分支二次复核
---------------------------

本轮继续只读复核 ``origin/codex/pdf-cjk-copy-search-gate`` 和
``origin/codex/pdf-cjk-bullet-fallback``，没有整分支合并、没有删除分支、没有运行
CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。

复核结论：

1. 两个 PDF 分支相对当前 ``dev`` 仍有一万行级别差异，集中在
   ``samples/pdf_regression_sample.cpp``、``test/pdf_regression_manifest.json``、
   ``test/CMakeLists.txt``、PDFium 构建脚本、manifest schema 和大批 visual gate
   样例扩展。
2. 当前 ``dev`` 已经保留核心小能力入口：``scripts/check_pdf_text_layer.py``、
   ``scripts/run_pdf_visual_release_gate.ps1`` 中的 ``cjk-copy-search`` 与
   ``cli-cjk-font-source``、PDF 表格 ``spanned_row_bottom`` 分页高度计算、
   页眉页脚 ``wrap_cursor_paragraph_runs`` / ``HeaderFooterLineLayout``，以及
   CJK 字体 fallback 和 CLI ``--cjk-font-file`` 覆盖。
3. 剩余差异不适合低资源阶段直接搬入，因为需要构建、PDF 渲染、视觉 baseline
   复核或更大样例治理预算；整分支合并还会回退当前 ``dev`` 已经恢复的文档主线记录。

因此，当前 PDF 功能整合状态是：核心源码能力和静态 gate 契约已经在 ``dev``；
旧分支中未合入的部分继续作为 PDF 专项参考库保留，后续应按样例主题分批重做并做可视化验证，
而不是整分支合并。

2026-05-19 PDF CJK 列表样例小批量搬入
-------------------------------------

本轮从两个旧 PDF CJK 分支的剩余样例库中选择低风险主题，按当前 ``dev`` 结构重做了
轻量级 CJK 列表 regression 契约，没有整分支合并、没有删除分支，也没有运行 CMake、
CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。

已搬入内容：

1. 新增 ``document-cjk-bullet-list-text``，覆盖 Document API 到 PDF adapter 的 CJK
   bullet list 路径，保留 East Asia 字体映射和稳定检索键 ``BL-101``。
2. 新增 ``document-cjk-numbered-list-text``，覆盖 CJK decimal list restart 路径，
   保留 East Asia 字体映射和稳定检索键 ``NL-101``。
3. ``test/CMakeLists.txt`` 将这两个样例纳入 CJK PDF regression 分类；后续完整构建时
   会携带 ``--require-cjk-font``，缺少字体时按既有规则返回 77 跳过。
4. 新增 ``test/pdf_cjk_list_regression_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake CJK 分类保持一致。

本轮轻量验证已通过：

1. ``git diff --check``。
2. ``test/pdf_cjk_list_regression_contract_test.ps1 -RepoRoot <repo>``，单独 60 秒超时。
3. ``test/pdf_visual_release_gate_text_shaping_baselines_test.ps1``，单独 60 秒超时。
4. ``test/pdf_visual_release_gate_style_baselines_test.ps1``，单独 60 秒超时。

本轮仍未执行 PDF 渲染或可视化验证。下一步最小风险动作是继续从旧 PDF 分支中筛选
copy/search matrix 或 CJK table wrap 的轻量契约，但仍应先做静态小样例，再等待资源
允许后统一进入受控可视化验证。

2026-05-19 PDF CJK copy/search 轻量契约搬入
------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的重型 matrix 样例中提取低风险
主题，但没有搬入旧分支的 3 页 visual baseline、图片资产或大矩阵。当前 ``dev`` 只新增
一个 1 页轻量样例，用来锁住 CJK 文本层 copy/search 锚点和 East Asia 字体映射。

已搬入内容：

1. 新增 ``document-cjk-copy-search-lite-text``，覆盖 Document API 到 PDF adapter 的
   CJK styled run、East Asia font family 映射和稳定检索键 ``CS-101``、``CS-202``、
   ``CS-999``。
2. ``test/CMakeLists.txt`` 将该样例纳入 CJK PDF regression 分类；后续完整构建时携带
   ``--require-cjk-font``，缺少字体时按既有规则返回 77 跳过。
3. 新增 ``test/pdf_cjk_copy_search_lite_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake CJK 分类保持一致。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该样例只作为低资源阶段的契约入口；真实 copy/search 可视化验证仍需等资源允许后通过
``scripts/run_pdf_visual_release_gate.ps1`` 受控执行。

2026-05-19 PDF CJK font embed 轻量契约搬入
------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的 CJK font embed matrix
方向提取低风险主题，但没有搬入旧分支的 3 页 matrix、visual baseline 或重型 gate。
当前 ``dev`` 只新增一个 1 页轻量样例，用来锁住 CJK 字体嵌入、字号切换、
styled run 和文本层检索锚点。

已搬入内容：

1. 新增 ``document-cjk-font-embed-lite-text``，覆盖 Document API 到 PDF adapter 的
   East Asia font family 映射、CJK 字体文件绑定和稳定检索键 ``FE-101``、
   ``FE-202``、``FE-999``。
2. ``test/CMakeLists.txt`` 将该样例纳入 CJK PDF regression 分类；后续完整构建时携带
   ``--require-cjk-font``，缺少字体时按既有规则返回 77 跳过。
3. 新增 ``test/pdf_cjk_font_embed_lite_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake CJK 分类保持一致。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该样例只作为低资源阶段的契约入口；真实字体嵌入和复制搜索效果仍需等资源允许后通过
受控 PDF 可视化验证确认。

2026-05-19 PDF CJK style overlay 轻量契约搬入
---------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的 CJK style overlay page flow
方向提取低风险主题，但没有搬入旧分支的 6 页 page flow、图片资产、visual baseline
或重型 gate。当前 ``dev`` 只新增一个 1 页轻量样例，用来锁住 CJK 上标、下标、
删除线、styled run 和文本层检索锚点。

已搬入内容：

1. 新增 ``document-cjk-style-overlay-lite-text``，覆盖 Document API 到 PDF adapter 的
   CJK superscript、subscript、strikethrough 和 East Asia font family 映射，
   保留稳定检索键 ``SO-101``、``SO-202``、``SO-303``、``SO-999``。
2. ``test/CMakeLists.txt`` 将该样例纳入 CJK PDF regression 分类；后续完整构建时携带
   ``--require-cjk-font``，缺少字体时按既有规则返回 77 跳过。
3. 新增 ``test/pdf_cjk_style_overlay_lite_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake CJK 分类保持一致。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该样例只作为低资源阶段的契约入口；真实 style overlay 版式和复制搜索效果仍需等资源
允许后通过受控 PDF 可视化验证确认。

2026-05-19 PDF CJK anchor matrix 轻量契约搬入
---------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的 CJK anchor font matrix boundary
方向提取低风险主题，但没有搬入旧分支的 4 页 boundary matrix、图片资产、表格流、
页眉页脚分页或 visual baseline。当前 ``dev`` 只新增一个 1 页轻量样例，用来锁住
CJK 多锚点检索、字体密度文本和 East Asia 字体映射。

已搬入内容：

1. 新增 ``document-cjk-anchor-matrix-lite-text``，覆盖 Document API 到 PDF adapter 的
   多锚点文本、CJK styled run 和 East Asia font family 映射，保留稳定检索键
   ``AM-101``、``AM-202``、``AM-303``、``AM-999``。
2. ``test/CMakeLists.txt`` 将该样例纳入 CJK PDF regression 分类；后续完整构建时携带
   ``--require-cjk-font``，缺少字体时按既有规则返回 77 跳过。
3. 新增 ``test/pdf_cjk_anchor_matrix_lite_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake CJK 分类保持一致。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该样例只作为低资源阶段的契约入口；真实 anchor matrix 分页、图片和表格版式仍需等资源
允许后通过受控 PDF 可视化验证确认。

2026-05-19 PDF CJK search density 轻量契约搬入
----------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的 CJK font search density flow
方向提取低风险主题，但没有搬入旧分支的 4 页 flow、图片资产、表格衔接、页眉页脚分页
或 visual baseline。当前 ``dev`` 只新增一个 1 页轻量样例，用来锁住 CJK 密排检索、
字宽回读和 East Asia 字体映射。

已搬入内容：

1. 新增 ``document-cjk-search-density-lite-text``，覆盖 Document API 到 PDF adapter 的
   CJK 密排文本、styled run 和 East Asia font family 映射，保留稳定检索键
   ``SD-101``、``SD-202``、``SD-303``、``SD-999``。
2. ``test/CMakeLists.txt`` 将该样例纳入 CJK PDF regression 分类；后续完整构建时携带
   ``--require-cjk-font``，缺少字体时按既有规则返回 77 跳过。
3. 新增 ``test/pdf_cjk_search_density_lite_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake CJK 分类保持一致。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该样例只作为低资源阶段的契约入口；真实密排检索、分页、图片和表格版式仍需等资源允许后
通过受控 PDF 可视化验证确认。

2026-05-19 PDF CJK repeated key 轻量契约搬入
--------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的 CJK repeated key boundary flow
方向提取低风险主题，但没有搬入旧分支的 6 页 boundary flow、图片资产、表格衔接、
页眉页脚分页或 visual baseline。当前 ``dev`` 只新增一个 1 页轻量样例，用来锁住
CJK 重复检索键、共享锚点和 East Asia 字体映射。

已搬入内容：

1. 新增 ``document-cjk-repeated-key-lite-text``，覆盖 Document API 到 PDF adapter 的
   CJK repeated key、styled run 和 East Asia font family 映射，保留稳定检索键
   ``RK-101``、重复共享键 ``RK-777``、以及收口键 ``RK-999``。
2. ``test/CMakeLists.txt`` 将该样例纳入 CJK PDF regression 分类；后续完整构建时携带
   ``--require-cjk-font``，缺少字体时按既有规则返回 77 跳过。
3. 新增 ``test/pdf_cjk_repeated_key_lite_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake CJK 分类保持一致。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该样例只作为低资源阶段的契约入口；真实 repeated-key 分页边界、图片和表格版式仍需等资源
允许后通过受控 PDF 可视化验证确认。

2026-05-19 PDF CJK vertical merge 轻量契约搬入
----------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的 CJK vertical merge wrap
方向提取低风险主题，但没有搬入旧分支的 4 页分页、图片资产、页眉页脚或 visual baseline。
当前 ``dev`` 只新增一个 1 页轻量样例，用来锁住 CJK 表格纵向合并、cant-split 行、
垂直居中和 East Asia 字体映射。

已搬入内容：

1. 新增 ``document-cjk-vertical-merge-lite-text``，覆盖 Document API 到 PDF adapter 的
   CJK 表格文本、``merge_down`` 纵向合并、``set_cant_split`` 禁拆行和
   ``cell_vertical_alignment::center`` 垂直对齐，保留稳定检索键 ``VM-101``、
   ``VM-202``、``VM-303``、``VM-777``、``VM-999``。
2. ``test/CMakeLists.txt`` 将该样例纳入 CJK PDF regression 分类；后续完整构建时携带
   ``--require-cjk-font``，缺少字体时按既有规则返回 77 跳过。
3. 新增 ``test/pdf_cjk_vertical_merge_lite_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake CJK 分类保持一致。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该样例只作为低资源阶段的契约入口；真实纵向合并分页、图片和页眉页脚版式仍需等资源允许后
通过受控 PDF 可视化验证确认。

2026-05-19 PDF CJK table wrap 轻量契约搬入
------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的 CJK table wrap page flow
方向提取低风险主题，但没有搬入旧分支的 5 页分页、图片资产、页眉页脚或 visual baseline。
当前 ``dev`` 只新增一个 1 页轻量样例，用来锁住 CJK 长文本单元格换行、显式列宽、
重复表头、cant-split 行和 East Asia 字体映射。

已搬入内容：

1. 新增 ``document-cjk-table-wrap-lite-text``，覆盖 Document API 到 PDF adapter 的
   CJK 表格长文本、``set_column_width_twips``、``set_repeats_header`` 和
   ``set_cant_split``，保留稳定检索键 ``TW-101``、``TW-202``、``TW-303``、
   ``TW-404``、``TW-999``。
2. ``test/CMakeLists.txt`` 将该样例纳入 CJK PDF regression 分类；后续完整构建时携带
   ``--require-cjk-font``，缺少字体时按既有规则返回 77 跳过。
3. 新增 ``test/pdf_cjk_table_wrap_lite_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake CJK 分类保持一致。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该样例只作为低资源阶段的契约入口；真实表格换行分页、图片和页眉页脚版式仍需等资源允许后
通过受控 PDF 可视化验证确认。

2026-05-19 PDF CJK font embed wrap mix 轻量契约搬入
---------------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的 CJK font embed wrap mix
方向提取低风险主题，但没有搬入旧分支的 4 页分页样例、页眉页脚、图片资产或 visual baseline。
当前 ``dev`` 只新增一个 1 页轻量样例，用来锁住 CJK 字体嵌入、长句换行、styled run、
copy/search 锚点和 East Asia 字体映射。

已搬入内容：

1. 新增 ``document-cjk-font-embed-wrap-mix-lite-text``，覆盖 Document API 到 PDF adapter 的
   CJK 字体嵌入、混排长句换行和 styled run，保留稳定检索键 ``WM-101``、
   ``FE-WM-202``、``WM-303``、``FE-WM-999``。
2. ``test/CMakeLists.txt`` 将该样例纳入 CJK PDF regression 分类；后续完整构建时携带
   ``--require-cjk-font``，缺少字体时按既有规则返回 77 跳过。
3. 新增 ``test/pdf_cjk_font_embed_wrap_mix_lite_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake CJK 分类保持一致。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该样例只作为低资源阶段的契约入口；真实字体嵌入、换行分页、页眉页脚和可视化版式仍需等资源允许后
通过受控 PDF 可视化验证确认。

2026-05-19 PDF CJK multi anchor table flow 轻量契约搬入
-------------------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的 CJK multi anchor table flow
方向提取低风险主题，但没有搬入旧分支的 5 页分页样例、图片资产、页眉页脚或 visual baseline。
当前 ``dev`` 只新增一个 1 页轻量样例，用来锁住多锚点文本、表格连续流、重复表头、
cant-split 行、styled run 和 East Asia 字体映射。

已搬入内容：

1. 新增 ``document-cjk-multi-anchor-table-flow-lite-text``，覆盖 Document API 到 PDF adapter 的
   CJK 多锚点表格流、``set_column_width_twips``、``set_repeats_header``、
   ``set_cant_split`` 和 styled run，保留稳定检索键 ``MA-101``、``FE-MA-202``、
   ``MA-A-04``、``MA-B-04``、``FE-MA-999``。
2. ``test/CMakeLists.txt`` 将该样例纳入 CJK PDF regression 分类；后续完整构建时携带
   ``--require-cjk-font``，缺少字体时按既有规则返回 77 跳过。
3. 新增 ``test/pdf_cjk_multi_anchor_table_flow_lite_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake CJK 分类保持一致。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该样例只作为低资源阶段的契约入口；真实多页锚点切换、图片环绕、页眉页脚和可视化版式仍需等资源允许后
通过受控 PDF 可视化验证确认。

2026-05-19 PDF CJK bullet overlay 轻量契约搬入
----------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-bullet-fallback`` 的 CJK bullet overlay page flow
方向提取低风险主题，但没有搬入旧分支的多页分页、图片资产、页眉页脚或 visual baseline。
当前 ``dev`` 只新增一个 1 页轻量样例，用来锁住 CJK bullet 列表、overlay styled run、
East Asia 字体映射和稳定复制检索键。

已搬入内容：

1. 新增 ``document-cjk-bullet-overlay-lite-text``，覆盖 Document API 到 PDF adapter 的
   CJK bullet 列表、superscript/subscript/strikethrough overlay 样式和
   East Asia 字体映射，保留稳定检索键 ``BO-101``、``BO-202``、``BO-303``、
   ``BO-777``、``FE-BO-999``。
2. ``test/CMakeLists.txt`` 将该样例纳入 CJK PDF regression 分类；后续完整构建时携带
   ``--require-cjk-font``，缺少字体时按既有规则返回 77 跳过。
3. 新增 ``test/pdf_cjk_bullet_overlay_lite_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake CJK 分类保持一致。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该样例只作为低资源阶段的契约入口；真实多页 bullet overlay、页眉页脚和可视化版式仍需等资源允许后
通过受控 PDF 可视化验证确认。

2026-05-19 PDF CJK numbered list page flow 轻量契约搬入
------------------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-bullet-fallback`` 的 CJK numbered list page flow
方向提取低风险主题，但没有搬入旧分支的 5 页分页样例、图片资产、页眉页脚或 visual baseline。
当前 ``dev`` 只新增一个 1 页轻量样例，用来锁住 CJK decimal 编号列表、编号重启、
bullet 切换、styled run、East Asia 字体映射和稳定复制检索键。

已搬入内容：

1. 新增 ``document-cjk-numbered-list-page-flow-lite-text``，覆盖 Document API 到 PDF adapter 的
   CJK decimal list、编号重启、bullet 切换和 styled run，保留稳定检索键 ``NL-101``、
   ``FE-NL-202``、``NL-888``、``FE-NL-921``、``FE-NL-999``。
2. ``test/CMakeLists.txt`` 将该样例纳入 CJK PDF regression 分类；后续完整构建时携带
   ``--require-cjk-font``，缺少字体时按既有规则返回 77 跳过。
3. 新增 ``test/pdf_cjk_numbered_list_page_flow_lite_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake CJK 分类保持一致。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该样例只作为低资源阶段的契约入口；真实多页 numbered list flow、图片环绕、页眉页脚和可视化版式仍需等资源允许后
通过受控 PDF 可视化验证确认。

2026-05-19 CLI PDF CJK 导出静态契约补齐
---------------------------------------

本轮继续只读复核 ``origin/codex/pdf-cjk-bullet-fallback`` 中的 CLI PDF export 覆盖。
当前 ``dev`` 已经包含 ``--cjk-font-file``、``--no-font-subset``、
``--no-system-font-fallbacks``、CJK 字体发现、PDFium readback 和 PDF 未启用诊断路径。
因此本轮没有搬入旧分支的大块 C++ 改动，只新增一个低资源静态契约，防止这些入口在后续
整理中退化。

已补齐内容：

1. 新增 ``test/pdf_cli_cjk_export_static_contract_test.ps1``，用纯文本检查
   ``test/pdf_cli_export_tests.cpp`` 中的 CJK 字体导出、字体子集开关和显式无系统
   fallback 覆盖。
2. 同一脚本检查 ``test/cli_tests.cpp`` 保留 PDF 未启用时的 CLI 诊断。
3. 同一脚本检查 ``test/CMakeLists.txt`` 保留 ``pdf_cli_export_tests`` 注册、对
   ``featherdoc_cli`` 的依赖、``cli/smoke/pdf`` 标签和 PDF-enabled CLI 编译覆盖。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
旧分支中剩余的源码差异继续作为 PDF 专项参考；如果要继续搬入，应优先挑选可以用静态契约
描述的小主题，等资源允许后再做受控 PDF 可视化验证。

2026-05-19 PDF CJK fallback 与表格 fitting 静态契约补齐
-------------------------------------------------------

本轮继续只读复核 ``origin/codex/pdf-cjk-bullet-fallback`` 中的 East Asia 字体 fallback
和 CJK 表格 header fitting 补丁。当前 ``dev`` 已包含对应源码与 C++ 测试入口，因此没有
整分支合并，也没有改动 PDF 核心实现，只新增一个低资源静态契约来防止后续整理时退化。

已补齐内容：

1. 新增 ``test/pdf_cjk_fallback_table_static_contract_test.ps1``，检查
   ``src/pdf/pdf_font_resolver.cpp`` 保留 Unicode fallback helper、字形支持检查、
   East Asia resolved family 切换和默认 CJK 字体 fallback 链路。
2. 同一脚本检查 ``test/pdf_font_resolver_tests.cpp`` 保留 Latin 字体缺字时切到
   East Asia 映射、Unicode prefix 切到 East Asia / 默认 CJK 字体的测试入口。
3. 同一脚本检查 ``test/pdf_document_adapter_font_tests.cpp`` 保留 bullet prefix
   East Asia fallback 和 exact-height repeated table header 可见性测试。
4. 同一脚本检查 ``src/pdf/pdf_document_adapter_table_layout.*`` 与
   ``src/pdf/pdf_document_adapter_tables.cpp`` 保留 ``spanned_row_bottom``、
   ``row_emission_height`` 和 ``repeated_headers_fit_with_row`` 的表格 fitting 链路。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该契约只确认当前 ``dev`` 已有实现入口仍存在；真实 PDF 表格分页、重复表头可见性和字体回退
版式仍需等资源允许后通过受控 PDF 可视化验证确认。
