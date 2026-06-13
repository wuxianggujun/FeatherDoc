文档接口主线现状记录：治理与视觉存档
==================

本文件从 ``document_api_mainline_status_zh.rst`` 拆分而来，保留历史推进记录。

原始行号范围：1047-1601。

----

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

2026-05-19 release governance 明细汇总静态契约补齐
---------------------------------------------------

本轮转向 ``origin/codex/release-governance-rollup-details`` 的低风险部分。该分支整体
相对当前 ``dev`` 已经严重陈旧，直接合并会删除当前 PDF 契约和多份恢复记录，因此没有整分支
合并，也没有改动发布脚本运行逻辑。当前 ``dev`` 已经具备 release blocker rollup、
release governance pipeline、handoff、reviewer checklist 和 release note bundle 的
明细透传链路，本轮只新增一个静态契约锁住这些入口。

已补齐内容：

1. 新增 ``test/release_governance_detail_rollup_static_contract_test.ps1``，检查
   rollup / pipeline / handoff 脚本保留 ``source_schema``、``source_report_display``、
   ``source_json_display``、``candidate_type`` 和 action command 字段。
2. 同一脚本检查发布治理测试继续覆盖 content-control、project onboarding、schema
   confidence calibration 的 source JSON、source schema、open command 和 candidate
   routing。
3. 同一脚本检查 ``release_blocker_metadata_helpers.ps1``、release candidate summary 和
   reviewer checklist 仍消费 release blocker rollup、governance handoff 以及 nested
   rollup 的明细数组。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该契约只作为发布治理明细透传的防退化入口；如后续继续整合该旧分支，应逐项挑选仍未覆盖的
小型文档或契约补丁，避免回退当前 ``dev`` 的 PDF 契约和恢复记录。

2026-05-19 release governance reviewer 明细质量门禁补齐
--------------------------------------------------------

本轮继续只读复核 ``origin/codex/release-governance-rollup-details`` 的剩余 8 个
branch-only 提交。复核结果是：发布包入口脚本已经在当前 ``dev`` 中输出
release blocker rollup 与 release governance handoff 明细段，旧分支剩余的大块差异仍
混有 PDF/RTL 源码、视觉样例和旧文档删除，不适合整分支合并。本轮只搬入一个仍有价值的小
门禁：发布包生成前校验 reviewer-facing governance 明细字段完整性。

已补齐内容：

1. ``release_blocker_metadata_helpers.ps1`` 新增 release governance reviewer metadata
   quality 校验，覆盖 ``release_blocker_rollup``、``release_governance_handoff`` 和
   ``stages`` 内的 blocker、warning、action item 明细。
2. ``write_release_note_bundle.ps1`` 在读取 summary 后同时执行 release blocker 元数据
   校验与 governance reviewer 明细校验；缺少 ``source_json_display`` 或 action item
   ``open_command`` 时会在发布包生成前失败。
3. ``release_note_bundle_version_test.ps1`` 增加 rollup 缺少 source JSON display 与
   handoff 缺少 open command 的负例，``release_governance_detail_rollup_static_contract_test.ps1``
   锁住这些校验入口。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该门禁只强化发布材料一致性；真实 PDF/RTL 分支功能仍保留为后续受控可视化验证前的参考项。

2026-05-19 release governance 轻量契约 CMake 入口补齐
-----------------------------------------------------

本轮继续复核 ``origin/codex/release-governance-warning-entrypoints``。该分支整体仍是旧基线，
直接合并会回退当前 ``dev`` 的 PDF 契约、文档恢复记录和更完整的治理 helper；但其中
“把 release governance warning 相关轻量契约接入常规测试入口”的方向仍有价值。当前脚本
文件已经存在且前几轮已手工验证，本轮只补 CMake 注册，不运行 CMake 或 CTest。

已补齐内容：

1. ``test/CMakeLists.txt`` 在 Windows PowerShell 测试区注册
   ``release_governance_warning_contract``、
   ``release_governance_warning_helper_contract``、
   ``release_governance_metrics_contract`` 和
   ``release_governance_detail_rollup_static_contract``。
2. 这些入口都设置 ``TIMEOUT 60`` 和 ``release;governance;smoke`` 标签，避免后续
   CTest 选择器遗漏治理契约或出现无界后台测试。
3. ``release_governance_detail_rollup_static_contract_test.ps1`` 增加对这些 CMake
   注册入口、脚本路径、超时和标签的静态检查。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该补丁只提升已有低资源契约的可发现性；``release-governance-warning-entrypoints`` 中
剩余的大块 style merge / restore audit / pipeline 历史差异继续只读保留，不整分支合并。

2026-05-19 CLI CJK PDF 可视化 gate 静态保护补齐
------------------------------------------------

本轮继续只读复核 ``origin/codex/pdf-cjk-bullet-fallback`` 的 CLI CJK PDF export
和 visual gate 连接点。当前 ``dev`` 已经保留 ``cjk-font-source.pdf`` 的 CLI 导出
测试产物和 ``run_pdf_visual_release_gate.ps1`` 中的 ``cli-cjk-font-source`` baseline
条目；旧分支剩余的大块 PDF regression 样例和 C++ 布局改动仍不适合低资源整批搬入。

已补齐内容：

1. ``test/pdf_cli_cjk_export_static_contract_test.ps1`` 继续锁住 ``--cjk-font-file``、
   ``--no-font-subset``、``--no-system-font-fallbacks`` 和 PDF 未启用诊断。
2. 同一静态契约新增检查 ``scripts/run_pdf_visual_release_gate.ps1`` 中的
   ``cli-cjk-font-source``、``test\\pdf_cli_export\\cjk-font-source.pdf``、
   stable baseline 输出目录和 3 页预期。
3. 本轮只做文本级静态保护，不执行 CMake、CTest、Ninja、MSBuild、Word、
   LibreOffice、浏览器或 PDF 渲染。

该补丁的目的不是宣称 PDF CJK 分支已完整合入，而是防止已经搬入 ``dev`` 的
CLI CJK 导出到受控可视化 gate 的连接点在后续整理时退化。

2026-05-19 PDF Bidi/RTL 行布局适配层小批量搬入
------------------------------------------------

本轮继续从旧 PDF 分支的 RTL / CJK 深水区中提取低风险源码能力，没有整分支合并，
也没有搬入大批 regression 样例、visual baseline 或 PDF 渲染 gate。当前只在
PDF adapter 的文本换行与发射层搬入 Bidi/RTL 元数据链路，作为后续可视化验证前的
小步能力补齐。

已补齐内容：

1. ``ResolvedRunStyle``、``TextToken``、``TextFragment`` 和 ``LineState`` 保留
   RTL / Bidi 标记，换行时通过 ``line_contains_rtl_fragments`` 识别 RTL 片段。
2. 段落适配层新增 ``resolve_paragraph_bidi``，优先使用段落显式 Bidi，其次使用段落
   样式，最后回退 ``document.default_paragraph_bidi()``。
3. 表格单元格换行会汇总单元格内段落 Bidi 标记，避免表格中的 RTL 文本丢失布局元数据。
4. PDF 发射层新增 ``fragment_visual_advances``，在不改动 PDF writer 和字体嵌入逻辑的
   前提下，对含 RTL 片段的混排行计算视觉 advance。
5. 新增 ``test/pdf_bidi_line_layout_static_contract_test.ps1`` 并注册到
   ``pdf_bidi_line_layout_static_contract``，设置 ``TIMEOUT 60`` 和
   ``pdf;layout;smoke`` 标签。

本轮仍不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
该补丁不表示旧 PDF CJK / RTL 分支已经完整合入；真实混排、表格 RTL 和 copy/search
视觉质量仍需要在源码提交推送、工作区干净且资源允许后，通过受控 PDF 可视化验证确认。

2026-05-19 PDF CJK page boundary 轻量契约搬入
------------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的 extreme page break
视觉样例中提取低风险主题，按当前 ``dev`` 结构压缩为 1 页轻量样例。没有整分支合并，
没有搬入旧分支的大批 5 页 visual baseline，也没有运行 CMake、CTest、Ninja、MSBuild、
Word、LibreOffice、浏览器或 PDF 渲染。

已补齐内容：

1. 新增 ``document-cjk-page-boundary-lite-text``，覆盖 Document API 到 PDF adapter 的
   CJK page boundary 文本、页眉页脚、内联图片、floating image square wrap 和稳定检索键
   ``PB-101`` / ``FE-PB-202`` / ``FE-PB-999``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并保持
   ``expected_pages`` 为 1、``expected_image_count`` 为 2，避免低资源阶段引入重型
   多页视觉假设。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK PDF regression 分类；后续完整构建时携带
   ``--require-cjk-font``，缺少字体时按既有规则返回 77 跳过。
4. 新增 ``test/pdf_cjk_page_boundary_lite_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake CJK 分类保持一致。

该补丁只补轻量入口，不代表旧分支的 full extreme page-break visual 样例已经完整合入。
真实分页边界、图片环绕和页眉页脚视觉质量仍需等资源允许后通过受控 PDF 可视化验证确认。

2026-05-19 PDF CJK list page flow 正式 ID 轻量契约搬入
--------------------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-bullet-fallback`` 的 CJK list page-flow
样例中提取低风险主题，按当前 ``dev`` 结构重做为 1 页轻量契约入口。没有整分支合并，
没有搬入旧分支的 5 页 visual baseline，也没有运行 CMake、CTest、Ninja、MSBuild、
Word、LibreOffice、浏览器或 PDF 渲染。

已补齐内容：

1. 新增 ``document-cjk-numbered-list-page-flow-text``、
   ``document-cjk-bullet-page-flow-text`` 和
   ``document-cjk-bullet-overlay-page-flow-text``，覆盖 CJK decimal/bullet list、
   list level、编号重启、页眉页脚、表格插入和 overlay 样式回读。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，样例总数更新为 72；
   ``test/pdf_regression_manifest_test.cpp`` 同步断言新样例和既有 lite 样例入口。
3. ``test/CMakeLists.txt`` 将三个正式 ID 纳入 CJK PDF regression 分类，并注册
   ``pdf_cjk_list_page_flow_contract``，保持 ``TIMEOUT 60`` 和 ``pdf;layout;smoke``。
4. 新增 ``test/pdf_cjk_list_page_flow_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake CJK 分类保持一致。

该补丁只补低资源契约入口，不表示旧分支中完整多页列表分页视觉样例已经全部合入。
真实列表分页、页眉页脚、表格断页和 overlay 视觉质量仍需等资源允许后通过受控 PDF
可视化验证确认。

2026-05-19 PDF table font matrix 轻量契约搬入
------------------------------------------------

本轮继续从旧 PDF 分支的表格字体矩阵样例中提取低风险主题，按当前 ``dev`` 结构重做为
1 页轻量契约入口。没有整分支合并，也没有搬入旧分支的 3 页 visual baseline 或执行 PDF
渲染。

已补齐内容：

1. 新增 ``document-table-font-matrix-text``，覆盖 Document API 表格单元格里的 CJK、RTL
   Arabic、bidi 段落、重复表头、cant-split 行、页眉页脚占位符和稳定检索键
   ``FE-420`` / ``FE-421`` / ``FE-422`` / ``FE-423``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并保持
   ``expected_pages`` 为 1，避免低资源阶段引入旧分支多页视觉假设。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK 与 RTL font gate，并注册
   ``pdf_document_table_font_matrix_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_document_table_font_matrix_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake 分类保持一致。

该补丁只表示表格字体矩阵的低资源契约入口已进入 ``dev``。真实表格字体映射、RTL 混排、
重复表头与页眉页脚视觉质量，仍需等源码提交推送且工作区干净后再通过受控 PDF 可视化验证
确认。

2026-05-19 PDF table CJK wrap flow 轻量契约搬入
-------------------------------------------------

本轮继续从旧 PDF 分支的 CJK 表格换行流程样例中提取低风险主题，按当前 ``dev`` 结构重做为
1 页轻量契约入口。没有整分支合并，也没有搬入旧分支的 2 页 visual baseline 或执行 PDF
渲染。

已补齐内容：

1. 新增 ``document-table-cjk-wrap-flow-text``，覆盖 Document API 表格单元格里的 CJK
   长文本换行、页眉页脚占位符、重复表头、cant-split 行和稳定检索键
   ``CW-101`` / ``CW-201`` / ``CW-202`` / ``CW-303`` / ``FE-CW-303`` /
   ``CW-999``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并保持
   ``expected_pages`` 为 1，避免低资源阶段引入旧分支多页视觉假设。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK font gate，并注册
   ``pdf_document_table_cjk_wrap_flow_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_document_table_cjk_wrap_flow_contract_test.ps1``，用纯文本静态契约确认
   样例生成器、manifest、manifest parser 测试和 CMake 分类保持一致。

该补丁只表示 CJK 表格换行流程的低资源契约入口已进入 ``dev``。真实 CJK 字体度量、表格
换行、重复表头与页眉页脚视觉质量，仍需等源码提交推送且工作区干净后再通过受控 PDF
可视化验证确认。

PDF 表格 CJK 纵向合并 cant-split 契约入口
------------------------------------------------

本轮继续从旧 PDF 分支的表格合并语料中提取低风险主题，按当前 ``dev`` 的 1 页轻量
样例结构重做，不整分支合并，也不搬入旧分支的多页 visual baseline。

已补齐内容：

1. 新增 ``document-table-cjk-vertical-merged-cant-split-text``，覆盖 CJK 表格里的
   纵向合并、横向合并、重复表头、cant-split 行、居中垂直对齐、页眉页脚占位符和稳定
   检索键 ``CVC-101`` / ``CVC-202`` / ``CVC-303`` / ``FE-CVC-404`` /
   ``CVC-999`` / ``ABC 123``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并将总样例数推进到
   75；该样例仍保持 ``expected_pages`` 为 1，避免低资源阶段扩大渲染成本。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK font gate，并注册
   ``pdf_document_table_cjk_vertical_merged_cant_split_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_document_table_cjk_vertical_merged_cant_split_contract_test.ps1``，
   用纯文本静态契约确认样例生成器、manifest、manifest parser 测试和 CMake 分类一致。

该补丁只表示 CJK 纵向合并表格的低资源契约入口已进入 ``dev``。真实 PDF 中合并单元格
边框、CJK 字体换行、cant-split 分页和页眉页脚视觉质量，仍需等源码提交推送且工作区
干净后再通过受控 PDF 可视化验证确认。

PDF 表格 CJK 合并重复表头契约入口
------------------------------------------------

本轮继续从旧 PDF 分支的 ``document-table-cjk-merged-repeat-text`` 多页样例中提取
低风险主题，按当前 ``dev`` 的 1 页轻量样例结构重做，不整分支合并，也不搬入旧分支的
4 页 visual baseline。

已补齐内容：

1. 新增 ``document-table-cjk-merged-repeat-text``，覆盖 CJK 表格里的横向合并看板表头、
   纵向合并负责人块、重复表头、cant-split 尾行、垂直居中和页眉页脚占位符。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并将总样例数推进到
   76；该样例保持 ``expected_pages`` 为 1，避免低资源阶段扩大 PDF 渲染成本。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK font gate，并注册
   ``pdf_document_table_cjk_merged_repeat_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_document_table_cjk_merged_repeat_contract_test.ps1``，用纯文本静态契约
   确认样例生成器、manifest、manifest parser 测试和 CMake 分类一致。

该补丁只表示 CJK 合并重复表头表格的低资源契约入口已进入 ``dev``。真实 PDF 中重复
表头、合并单元格边框、CJK 字体换行和页眉页脚视觉质量，仍需等源码提交推送且工作区
干净后再通过受控 PDF 可视化验证确认。

PDF CJK table wrap page-flow 契约入口
------------------------------------------------

本轮继续从旧 PDF 分支的 ``document-cjk-table-wrap-page-flow-text`` 5 页样例中提取
低风险主题，按当前 ``dev`` 的 1 页轻量样例结构重做，不整分支合并，也不搬入旧分支的
图片压力语料或 5 页 visual baseline。

已补齐内容：

1. 新增 ``document-cjk-table-wrap-page-flow-text``，覆盖 CJK 表格换行、重复表头、
   合并看板表头、纵向合并阶段块、cant-split 尾行、页眉页脚占位符和稳定检索键
   ``TF-101`` / ``TF-202`` / ``TF-303`` / ``FE-TF-921`` / ``TF-A-05`` /
   ``TF-B-03`` / ``FE-TF-999`` / ``ABC 123``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并将总样例数推进到
   77；该样例保持 ``expected_pages`` 为 1，不声明旧分支的 5 张图片压力假设。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK font gate，并注册
   ``pdf_cjk_table_wrap_page_flow_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_cjk_table_wrap_page_flow_contract_test.ps1``，用纯文本静态契约确认
   样例生成器、manifest、manifest parser 测试和 CMake 分类一致。

该补丁只表示 CJK table wrap page-flow 的低资源契约入口已进入 ``dev``。真实 PDF 中
多页表格换行、图片环绕、锚点恢复全宽和页眉页脚视觉质量，仍需等源码提交推送且工作区
干净后再通过受控 PDF 可视化验证确认。

PDF CJK multi anchor table-flow 契约入口
------------------------------------------------

本轮继续从旧 PDF 分支的 ``document-cjk-multi-anchor-table-flow-text`` 5 页样例中提取
低风险主题，按当前 ``dev`` 的 1 页轻量样例结构重做。该批次不整分支合并，不搬入旧
分支的图片压力语料，也不声明旧分支的 5 页 visual baseline 已经进入当前主线。

已补齐内容：

1. 新增 ``document-cjk-multi-anchor-table-flow-text``，覆盖 CJK 多锚点文本流、
   first/even/default 页眉页脚占位符、合并看板表头、纵向合并锚点块、重复表头、
   cant-split 行、styled CJK run 和稳定检索键
   ``MA-101`` / ``MA-202`` / ``MA-303`` / ``FE-MA-901`` /
   ``FE-MA-951`` / ``FE-MA-971`` / ``FE-MA-981`` / ``FE-MA-999`` /
   ``ABC 123``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并将总样例数推进到
   78；该样例保持 ``expected_pages`` 为 1，避免低资源阶段扩大 PDF 渲染成本。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK font gate，并注册
   ``pdf_cjk_multi_anchor_table_flow_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_cjk_multi_anchor_table_flow_contract_test.ps1``，用纯文本静态契约确认
   样例生成器、manifest、manifest parser 测试和 CMake 分类保持一致。

该补丁只表示 CJK multi-anchor table-flow 的低资源契约入口已进入 ``dev``。真实 PDF 中
多锚点跨页表格流、图片环绕、header/footer 变体和 copy/search 视觉质量，仍需等源码
提交推送且工作区干净后再通过受控 PDF 可视化验证确认。

2026-05-20 PDF CJK vertical merge wrap cant-split 正式 ID 契约搬入
------------------------------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的 CJK vertical merge wrap
样例中提取低风险主题，按当前 ``dev`` 的 1 页轻量样例结构重做。不整分支合并，
不搬入旧分支图片语料，也不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、
浏览器或 PDF 渲染。

已补齐内容：

1. 新增 ``document-cjk-vertical-merge-wrap-cant-split-text``，覆盖 first/even/default
   页眉页脚占位符、纵向合并、横向合并、重复表头、cant-split 行、styled CJK run 和
   稳定检索键 ``VM-101`` / ``VM-202`` / ``VM-303`` / ``FE-VM-901`` /
   ``FE-VM-921`` / ``CVM-04`` / ``FE-VM-999`` / ``ABC 123``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并将总样例数推进到
   89；该样例保持 ``expected_pages`` 为 1，避免低资源阶段扩大 PDF 渲染成本。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK font gate，并注册
   ``pdf_cjk_vertical_merge_wrap_cant_split_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_cjk_vertical_merge_wrap_cant_split_contract_test.ps1``，用纯文本静态契约
   确认样例生成器、manifest、manifest parser 测试和 CMake 分类保持一致。

该补丁只表示 CJK vertical merge wrap cant-split 的低资源正式 ID 契约入口已进入
``dev``。真实多页纵向合并、整块迁移、小页高分页、页眉页脚文本层和 PDFium 可复制性，
仍需等源码提交推送且工作区干净后再通过受控 PDF 可视化验证确认。

2026-05-20 PDF table vertical merged cant-split 正式 ID 契约搬入
---------------------------------------------------------------

本轮继续从旧 PDF 分支中最后一个 manifest 缺口 ``document-table-vertical-merged-cant-split-text``
提取低风险主题，按当前 ``dev`` 的 1 页轻量样例结构重做。不整分支合并，不搬入旧分支
4 页 visual baseline，也不执行 CMake、CTest、Ninja、MSBuild、Word、LibreOffice、浏览器
或 PDF 渲染。

已补齐内容：

1. 新增 ``document-table-vertical-merged-cant-split-text``，覆盖非 CJK 表格的纵向合并
   owner block、横向合并看板表头、重复表头、cant-split 行、页眉页脚占位符和稳定检索键
   ``VMC-01`` / ``VMC-02`` / ``VMC-03`` / ``VMC-04`` / ``VMC-05``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并将总样例数推进到
   90；该样例保持 ``expected_pages`` 为 1，不声明旧分支的 4 页 ``visual_expected_pages``。
3. ``test/CMakeLists.txt`` 注册
   ``pdf_document_table_vertical_merged_cant_split_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_document_table_vertical_merged_cant_split_contract_test.ps1``，用纯文本静态契约
   确认样例生成器、manifest、manifest parser 测试和 CMake 注册一致。

该补丁只表示旧 PDF manifest 中最后一个明确缺失样例 ID 已按低资源方式进入 ``dev``。
真实跨页纵向合并、重复表头、cant-split 落页和页码视觉质量，仍需等源码提交推送且工作区
干净后再通过受控 PDF 可视化验证确认。

2026-05-20 PDF 参考分支剩余差异审计
------------------------------------

本轮在删除 FeatherDoc 自动化任务后，重新以低资源方式只读复核 ``dev`` 与远端参考分支。
当前 ``dev`` 与 ``origin/dev`` 对齐，工作区干净，最新主线提交为
``20ba35c Add PDF table vertical merged cant split contract``。

已确认结论：

1. ``origin/codex/pdf-cjk-copy-search-gate`` 的 manifest 样例数为 70；当前
   ``dev`` 的 manifest 样例数为 90；该分支相对 ``dev`` 的 ``missing_in_dev`` 为 0。
2. ``origin/codex/pdf-cjk-bullet-fallback`` 的 manifest 样例数为 73；当前
   ``dev`` 的 manifest 样例数为 90；该分支相对 ``dev`` 的 ``missing_in_dev`` 为 0。
3. 当前 ``dev`` 已具备 PDF import、``import-pdf`` CLI、PDF import JSON diagnostics、
   ``--min-table-continuation-confidence``、字体子集、HarfBuzz text shaper、
   shaped glyph writer、CJK copy/search text layer gate 和 ``cli-cjk-font-source`` gate
   等主线入口。
4. ``96bac82``、``3be71b2``、``1fb55a1``、``b52bf60`` 和 ``11dc255`` 等旧分支提交
   对应的核心方向，已在当前 ``dev`` 中通过新结构或轻量契约等价覆盖；继续整分支合并会
   回退当前 manifest、CMake、visual gate 和文档状态记录。

因此，PDF CJK 两个参考分支中可在低资源阶段继续拆入的明确 manifest 功能缺口已经处理完毕。
剩余差异主要是旧提交历史、大批重型 visual baseline、过期 gate 结构或需要 PDF 渲染验证的
质量证据。后续不应继续盲目搬代码，下一步最小风险动作是：保持 ``dev`` 干净并已推送，
在资源允许时只对当前 ``dev`` 执行受控 PDF 可视化验证，验证完成后再决定是否归档旧分支。

2026-05-20 受控 PDF 可视化烟测
------------------------------

用户关闭 Edge 后，本轮重新做了资源预检。当前没有 Edge、Chrome、Firefox、CMake、CTest、
Ninja、MSBuild、Word、LibreOffice 或 PDF 渲染残留进程；仍有外部 ``node`` 和
``powershell`` 进程，但不是本任务启动，未擅自关闭。

验证边界：

1. 当前 ``build`` 目录没有 ``CTestTestfile.cmake``，也没有可复用的 PDF regression 输出，
   因此未运行完整 ``scripts/run_pdf_visual_release_gate.ps1``。该脚本会先调用
   ``ctest``，并可能创建 ``.venv-pdf-visual-smoke`` 或安装渲染依赖，不适合在本轮低资源
   前提下直接启动。
2. 已复用既有 ``.venv-word-visual-smoke``，确认其中已有 ``PIL`` 和 ``fitz``，没有新建虚拟
   环境，也没有安装依赖。
3. 已用 60 秒超时分别运行并通过两个静态 PDF visual gate 契约：
   ``pdf_visual_release_gate_style_baselines_test.ps1`` 和
   ``pdf_visual_release_gate_text_shaping_baselines_test.ps1``。
4. 已复用既有两个小 PDF：
   ``output/word-visual-smoke-minimal-20260519/table_visual_smoke.pdf`` 和
   ``output/word-visual-smoke-rerun-20260519/table_visual_smoke.pdf``，用
   ``scripts/render_pdf_pages.py`` 渲染到
   ``output/pdf-controlled-visual-smoke-20260520``，并用
   ``scripts/check_pdf_text_layer.py`` 完成文本层提取。
5. 人工查看了两个 contact sheet 和两个原始 ``page-01.png``；页面非空，页数为 1，
   标题、正文和小表格均可见。该检查证明本机当前 PDF 渲染链路可用，但不等同于完整
   PDF release gate 或 PDF CJK 样例库可视化验收。

下一步若要进入完整 PDF release gate，最小风险前置条件是先准备或复用一个有效的 PDF
构建目录，让 ``pdf_cli_export``、``pdf_regression_`` 和相关 PDF 输出存在；否则会触发
构建或直接失败。

2026-05-20 PDF 可视化发布门禁预检
-----------------------------------

本轮补充了低资源预检入口 ``scripts/check_pdf_visual_release_gate_preflight.ps1``。
该脚本用于判断当前机器是否已经具备运行完整
``scripts/run_pdf_visual_release_gate.ps1`` 的前置条件，而不是直接启动完整门禁。

预检内容包括：

1. build 目录、``CMakeCache.txt`` 和 ``CTestTestfile.cmake`` 是否存在。
2. ``ctest -N`` 是否能列出 ``pdf_cli_export``、``pdf_regression_``、
   ``pdf_visual_release_gate_style_baselines`` 和
   ``pdf_visual_release_gate_text_shaping_baselines``。
3. ``pdf_cli_export`` 的两个 baseline PDF 是否已存在。
4. manifest 中 ``expect_visual_baseline`` 和 ``expect_cjk`` 样例对应的 PDF 输出是否已存在。
5. ``render_pdf_pages.py``、``check_pdf_text_layer.py`` 和
   ``build_image_contact_sheet.py`` 等 helper 是否存在。
6. 是否能复用已有 ``FEATHERDOC_RENDER_PYTHON_EXECUTABLE``、
   ``.venv-word-visual-smoke`` 或 ``tmp/render-venv`` 中同时具备 ``PIL`` 与 ``fitz`` 的
   Python。

该预检默认只报告 ``ready`` / ``not_ready`` 并输出 JSON；只有传入 ``-Strict`` 时，
缺少前置条件才会用非零退出。它不会创建虚拟环境、安装依赖、运行 PDF 渲染或触发构建。
配套新增 ``test/pdf_visual_release_gate_preflight_test.ps1``，用 fake build、fake ctest
和 fake render Python 固定脚本契约，并在 ``test/CMakeLists.txt`` 中注册为 60 秒静态
测试。

随后 ``scripts/run_pdf_visual_release_gate.ps1`` 也接入同一预检：默认先运行严格预检，
只有前置条件完整时才继续进入 ``ctest``、文本层检查和 PNG 渲染阶段。需要只做前置检查时
传 ``-PreflightOnly``；只有在已经人工确认 build 输出完整时，才允许传 ``-SkipPreflight``
绕过保护。

2026-05-20 PDF visual release gate 预检治理报告
----------------------------------------------

本轮继续把 PDF visual release gate 的剩余风险纳入 release governance，而不是继续盲目
搬运旧 PDF 分支。新增 ``scripts/write_pdf_visual_release_gate_preflight_governance_report.ps1``，
用于把 ``check_pdf_visual_release_gate_preflight.ps1`` 的 summary 转成 release blocker
rollup 可消费的治理报告。

该脚本保持低资源边界：

1. 可以读取既有 ``-PreflightJson``，也可以调用只读预检生成 summary。
2. 不运行 CMake、完整 CTest、Ninja、MSBuild、Word、LibreOffice、浏览器或 PDF 渲染。
3. 不创建虚拟环境，不安装依赖，不生成新的 PDF baseline。
4. 当预检为 ``not_ready`` 时，输出
   ``pdf_visual_release_gate_preflight.build_outputs_missing`` 阻塞项，并携带
   ``source_schema``、``source_report_display``、``source_json_display``、
   ``repair_strategy``、``repair_hint`` 和 ``command_template``。

配套新增 ``test/pdf_visual_release_gate_preflight_governance_report_test.ps1``，用 fake
``ready`` / ``not_ready`` summary 验证：

1. ``not_ready`` 会生成 release blocker 和 action item。
2. ``ready`` 不生成阻塞项。
3. 生成的 summary 可以被 ``scripts/build_release_blocker_rollup_report.ps1`` 聚合。

当前结论不变：PDF 参考分支的 manifest 明确缺口已经收敛到 0，但完整 PDF visual
release gate 仍需要可复用 CMake build 目录、CTest 注册和当前 ``dev`` 生成的 PDF 输出。完整
门禁通过前，不能把远端 PDF 参考分支视为可立即清理。

2026-05-20 受控 PDF smoke 检查器落地
------------------------------------

本轮继续推进可视化验证，但仍保持低资源边界：未运行 CMake、CTest、Ninja、MSBuild、
Word、LibreOffice、浏览器或新的 PDF 渲染。新增并推送
``ee81820 Add controlled PDF visual smoke checker``，把前一轮已经生成的
``output/pdf-controlled-visual-smoke-20260520`` 证据变成可重复检查的机器契约。

新增检查器覆盖：

1. ``minimal`` 和 ``rerun`` 两组 smoke case 是否都有 ``summary.json``、页面 PNG、
   contact sheet、``text-summary.json`` 和 ``text.txt``。
2. PNG 是否可解码，尺寸是否满足阈值，非白像素比例是否足够证明页面不是空白。
3. 文本层是否包含 ``FeatherDoc Word visual smoke input`` 和
   ``This minimal document is generated for local visual validation preflight.``。
4. 文本层页数是否与渲染 summary 页数一致。

已用 60 秒超时包装并通过：

* ``test/pdf_controlled_visual_smoke_check_test.ps1``：验证正常 fixture 通过、空白 PNG
  fixture 失败。
* ``scripts/check_pdf_controlled_visual_smoke.ps1``：复核真实既有 smoke 产物，结果为
  ``pass``。
* ``scripts/compare_pdf_reference_branch_manifest.ps1 -FailOnMissingInDev``：再次确认
  ``origin/codex/pdf-cjk-copy-search-gate`` 的 70 个样例 ID 和
  ``origin/codex/pdf-cjk-bullet-fallback`` 的 73 个样例 ID 在当前 ``dev`` 中缺口均为 0。

因此，当前状态可以更精确地表述为：PDF CJK 参考分支的 manifest 功能入口已经被当前
``dev`` 覆盖；已有 smoke 产物也有机器可重复检查的非空页面和文本层证据。但这仍不是完整
PDF visual release gate。完整门禁仍被 build / CTest 注册 / 当前 ``dev`` 生成的 PDF
baseline 输出缺口阻塞。
