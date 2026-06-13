文档接口主线现状记录：PDF CJK 迁移存档
=======================

本文件从 ``document_api_mainline_status_zh.rst`` 拆分而来，保留历史推进记录。

原始行号范围：484-1046。

----

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

2026-05-19 PDF CJK copy/search matrix 正式 ID 契约搬入
------------------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的
``document-cjk-copy-search-matrix-text`` 3 页样例中提取低风险主题，按当前 ``dev`` 的
1 页轻量结构重做。该批次不整分支合并，不搬入旧分支的大矩阵正文，也不声明旧分支的
3 页 visual baseline 已进入当前主线。

已搬入内容：

1. 新增 ``document-cjk-copy-search-matrix-text``，覆盖 CJK copy/search 文本层锚点、
   first/even/default 页眉页脚占位符、重复表头、表格检索键和稳定检索键
   ``CS-101`` / ``CS-202`` / ``CS-303`` / ``FE-CS-901`` /
   ``FE-CS-921`` / ``FE-CS-931`` / ``FE-CS-941`` / ``FE-CS-999`` /
   ``ABC 123``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并将总样例数推进到
   79；该样例保持 ``expected_pages`` 为 1，避免低资源阶段扩大 PDF 渲染成本。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK font gate，并注册
   ``pdf_cjk_copy_search_matrix_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_cjk_copy_search_matrix_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake 分类保持一致。

该补丁只表示 CJK copy/search matrix 的低资源正式 ID 契约入口已进入 ``dev``。真实
多页 copy/search、页眉页脚文本层和 PDFium 可复制性，仍需等源码提交推送且工作区干净后
再通过受控 PDF 可视化验证确认。

2026-05-19 PDF CJK font embed matrix 正式 ID 契约搬入
-----------------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的
``document-cjk-font-embed-matrix-text`` 3 页样例中提取低风险主题，按当前 ``dev`` 的
1 页轻量结构重做。该批次不整分支合并，不搬入旧分支的大矩阵正文，也不声明旧分支的
3 页 visual baseline 已进入当前主线。

已搬入内容：

1. 新增 ``document-cjk-font-embed-matrix-text``，覆盖 CJK 字体嵌入、styled run 大小字
   切换、first/even/default 页眉页脚占位符、重复表头、表格字体检索键和稳定检索键
   ``FM-101`` / ``FM-202`` / ``FM-303`` / ``FE-FM-901`` /
   ``FE-FM-921`` / ``FE-FM-922`` / ``FE-FM-933`` / ``FE-FM-971`` /
   ``FE-FM-999`` / ``ABC 123``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并将总样例数推进到
   80；该样例保持 ``expected_pages`` 为 1，避免低资源阶段扩大 PDF 渲染成本。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK font gate，并注册
   ``pdf_cjk_font_embed_matrix_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_cjk_font_embed_matrix_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake 分类保持一致。

该补丁只表示 CJK font embed matrix 的低资源正式 ID 契约入口已进入 ``dev``。真实多页
字体嵌入、字号切换、页眉页脚文本层和 PDFium 可复制性，仍需等源码提交推送且工作区
干净后再通过受控 PDF 可视化验证确认。

2026-05-19 PDF CJK anchor font matrix boundary 正式 ID 契约搬入
---------------------------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的
``document-cjk-anchor-font-matrix-boundary-text`` 4 页样例中提取低风险主题，按当前
``dev`` 的 1 页轻量结构重做。该批次不整分支合并，不搬入旧分支的图片压力语料，也不
声明旧分支的 4 页 visual baseline 已进入当前主线。

已搬入内容：

1. 新增 ``document-cjk-anchor-font-matrix-boundary-text``，覆盖 CJK 锚点字体矩阵、
   styled run 大小字切换、first/even/default 页眉页脚占位符、重复表头、
   cant-split 尾行、边界表格和稳定检索键 ``AM-101`` / ``AM-202`` /
   ``AM-303`` / ``FE-AM-901`` / ``FE-AM-921`` / ``AM-A-04`` /
   ``AM-B-04`` / ``FE-AM-999`` / ``ABC 123``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并将总样例数推进到
   81；该样例保持 ``expected_pages`` 为 1，避免低资源阶段扩大 PDF 渲染成本。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK font gate，并注册
   ``pdf_cjk_anchor_font_matrix_boundary_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_cjk_anchor_font_matrix_boundary_contract_test.ps1``，用纯文本静态契约
   确认样例生成器、manifest、manifest parser 测试和 CMake 分类保持一致。

该补丁只表示 CJK anchor font matrix boundary 的低资源正式 ID 契约入口已进入
``dev``。真实多页锚点边界、图片后恢复全宽、页眉页脚文本层和 PDFium 可复制性，仍需
等源码提交推送且工作区干净后再通过受控 PDF 可视化验证确认。

2026-05-19 PDF CJK font search density flow 正式 ID 契约搬入
-------------------------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的
``document-cjk-font-search-density-flow-text`` 4 页样例中提取低风险主题，按当前
``dev`` 的 1 页轻量结构重做。该批次不整分支合并，不搬入旧分支的图片压力语料，也不
声明旧分支的 4 页 visual baseline 已进入当前主线。

已搬入内容：

1. 新增 ``document-cjk-font-search-density-flow-text``，覆盖 CJK 密排检索、styled run
   大小字切换、first/even/default 页眉页脚占位符、重复表头、cant-split 尾行、
   检索密度表和稳定检索键 ``SD-101`` / ``SD-202`` / ``SD-303`` /
   ``FE-SD-901`` / ``FE-SD-921`` / ``FE-SD-931`` / ``FE-SD-932`` /
   ``SD-A-04`` / ``SD-B-04`` / ``FE-SD-999`` / ``ABC 123``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并将总样例数推进到
   82；该样例保持 ``expected_pages`` 为 1，避免低资源阶段扩大 PDF 渲染成本。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK font gate，并注册
   ``pdf_cjk_font_search_density_flow_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_cjk_font_search_density_flow_contract_test.ps1``，用纯文本静态契约确认
   样例生成器、manifest、manifest parser 测试和 CMake 分类保持一致。

该补丁只表示 CJK font search density flow 的低资源正式 ID 契约入口已进入 ``dev``。
真实多页密排检索、图片后恢复全宽、页眉页脚文本层和 PDFium 可复制性，仍需等源码提交
推送且工作区干净后再通过受控 PDF 可视化验证确认。

2026-05-20 PDF CJK font embed wrap mix 正式 ID 契约搬入
-------------------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的
``document-cjk-font-embed-wrap-mix-text`` 4 页样例中提取低风险主题，按当前
``dev`` 的 1 页轻量结构重做。该批次不整分支合并，不搬入旧分支的图片压力语料，也不
声明旧分支的 4 页 visual baseline 已进入当前主线。

已搬入内容：

1. 新增 ``document-cjk-font-embed-wrap-mix-text``，覆盖 CJK 字体嵌入、styled run
   大小字切换、first/even/default 页眉页脚占位符、重复表头、cant-split 尾行、
   环绕混排表格和稳定检索键 ``WM-101`` / ``WM-202`` / ``WM-303`` /
   ``FE-WM-901`` / ``FE-WM-911`` / ``FE-WM-921`` / ``FE-WM-941`` /
   ``FE-WM-951`` / ``FE-WM-961`` / ``FE-WM-999`` / ``ABC 123``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并将总样例数推进到
   83；该样例保持 ``expected_pages`` 为 1，避免低资源阶段扩大 PDF 渲染成本。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK font gate，并注册
   ``pdf_cjk_font_embed_wrap_mix_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_cjk_font_embed_wrap_mix_contract_test.ps1``，用纯文本静态契约确认
   样例生成器、manifest、manifest parser 测试和 CMake 分类保持一致。

该补丁只表示 CJK font embed wrap mix 的低资源正式 ID 契约入口已进入 ``dev``。
真实多页字体嵌入环绕、图片后恢复全宽、页眉页脚文本层和 PDFium 可复制性，仍需等源码
提交推送且工作区干净后再通过受控 PDF 可视化验证确认。

2026-05-20 PDF CJK repeated key boundary flow 正式 ID 契约搬入
--------------------------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的
``document-cjk-repeated-key-boundary-flow-text`` 6 页样例中提取低风险主题，按当前
``dev`` 的 1 页轻量结构重做。该批次不整分支合并，不搬入旧分支的图片压力语料，也不
声明旧分支的 6 页 visual baseline 已进入当前主线。

已搬入内容：

1. 新增 ``document-cjk-repeated-key-boundary-flow-text``，覆盖 CJK 重复检索键、
   styled run 大小字切换、first/even/default 页眉页脚占位符、重复表头、
   合并单元格、cant-split 行和稳定检索键 ``RK-101`` / ``RK-202`` /
   ``RK-303`` / ``RK-777`` / ``RK-A-01`` / ``RK-A-04`` / ``RK-A-06`` /
   ``FE-RK-901`` / ``FE-RK-911`` / ``FE-RK-941`` / ``FE-RK-951`` /
   ``FE-RK-961`` / ``FE-RK-981`` / ``FE-RK-999`` / ``ABC 123``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并将总样例数推进到
   84；该样例保持 ``expected_pages`` 为 1，避免低资源阶段扩大 PDF 渲染成本。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK font gate，并注册
   ``pdf_cjk_repeated_key_boundary_flow_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_cjk_repeated_key_boundary_flow_contract_test.ps1``，用纯文本静态契约确认
   样例生成器、manifest、manifest parser 测试和 CMake 分类保持一致。

该补丁只表示 CJK repeated key boundary flow 的低资源正式 ID 契约入口已进入 ``dev``。
真实多页重复键边界、图片环绕、页眉页脚文本层和 PDFium 可复制性，仍需等源码提交推送且
工作区干净后再通过受控 PDF 可视化验证确认。

2026-05-20 PDF CJK style overlay page flow 正式 ID 契约搬入
-----------------------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的
``document-cjk-style-overlay-page-flow-text`` 6 页样例中提取低风险主题，按当前
``dev`` 的 1 页轻量结构重做。该批次不整分支合并，不搬入旧分支的图片压力语料，也不
声明旧分支的 6 页 visual baseline 已进入当前主线。

已搬入内容：

1. 新增 ``document-cjk-style-overlay-page-flow-text``，覆盖 CJK style overlay、
   superscript、subscript、strikethrough、first/even/default 页眉页脚占位符、
   重复表头、合并单元格、cant-split 行、styled table cell 段落和稳定检索键
   ``SO-101`` / ``SO-202`` / ``SO-303`` / ``SO-888`` / ``SO-A-03`` /
   ``FE-SO-901`` / ``FE-SO-921`` / ``FE-SO-961`` / ``FE-SO-981`` /
   ``FE-SO-999`` / ``ABC 123``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并将总样例数推进到
   85；该样例保持 ``expected_pages`` 为 1，避免低资源阶段扩大 PDF 渲染成本。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK font gate，并注册
   ``pdf_cjk_style_overlay_page_flow_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_cjk_style_overlay_page_flow_contract_test.ps1``，用纯文本静态契约确认
   样例生成器、manifest、manifest parser 测试和 CMake 分类保持一致。

该补丁只表示 CJK style overlay page flow 的低资源正式 ID 契约入口已进入 ``dev``。
真实多页样式叠加、图片环绕、页眉页脚文本层和 PDFium 可复制性，仍需等源码提交推送且
工作区干净后再通过受控 PDF 可视化验证确认。

2026-05-20 PDF CJK complex layout 正式 ID 契约搬入
--------------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的
``document-cjk-complex-layout-text`` 3 页样例中提取低风险主题，按当前 ``dev`` 的
1 页轻量结构重做。该批次不整分支合并，不搬入旧分支的浮动图片、裁剪环绕压力语料，
也不声明旧分支的 3 页 visual baseline 已进入当前主线。

已搬入内容：

1. 新增 ``document-cjk-complex-layout-text``，覆盖 CJK 复杂版式概览、styled run、
   first/even/default 页眉页脚占位符、重复表头、合并单元格、cant-split 行和稳定
   检索键 ``CL-101`` / ``CL-202`` / ``CL-303`` / ``FE-CL-901`` /
   ``FE-CL-902`` / ``FE-CL-903`` / ``FE-CL-921`` / ``FE-CL-941`` /
   ``FE-CL-951`` / ``FE-CL-999`` / ``ABC 123``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并将总样例数推进到
   86；该样例保持 ``expected_pages`` 为 1，避免低资源阶段扩大 PDF 渲染成本。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK font gate，并注册
   ``pdf_cjk_complex_layout_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_cjk_complex_layout_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake 分类保持一致，并确认该低资源
   契约本身不调用图片或 floating image。

该补丁只表示 CJK complex layout 的低资源正式 ID 契约入口已进入 ``dev``。真实多页
复杂版式、浮动图片环绕、页眉页脚文本层和 PDFium 可复制性，仍需等源码提交推送且
工作区干净后再通过受控 PDF 可视化验证确认。

2026-05-20 PDF CJK image wrap stress 正式 ID 契约搬入
----------------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的
``document-cjk-image-wrap-stress-text`` 4 页图像压力样例中提取低风险主题，按当前
``dev`` 的 1 页轻量结构重做。该批次不整分支合并，不搬入旧分支的 inline image、
floating image、裁剪图片资产或 4 页 visual baseline，只保留可静态验证的文本层、
页眉页脚和表格治理契约。

已搬入内容：

1. 新增 ``document-cjk-image-wrap-stress-text``，覆盖 CJK 多锚点图像流主题、styled
   run、first/even/default 页眉页脚占位符、搜索热区、重复表头、合并单元格、
   cant-split 行和稳定检索键 ``IW-101`` / ``IW-202`` / ``IW-303`` /
   ``FE-IW-901`` / ``FE-IW-921`` / ``FE-IW-938`` / ``FE-IW-956`` /
   ``FE-IW-999`` / ``ABC 123``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并将总样例数推进到
   87；该样例保持 ``expected_pages`` 为 1，避免低资源阶段扩大 PDF 渲染成本。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK font gate，并注册
   ``pdf_cjk_image_wrap_stress_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_cjk_image_wrap_stress_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake 分类保持一致，并确认该低资源
   契约本身不调用图片或 floating image。

该补丁只表示 CJK image wrap stress 的低资源正式 ID 契约入口已进入 ``dev``。真实多页
图像环绕、裁剪回流、页眉页脚文本层和 PDFium 可复制性，仍需等源码提交推送且工作区
干净后再通过受控 PDF 可视化验证确认。

2026-05-20 PDF CJK extreme page breaks 正式 ID 契约搬入
-------------------------------------------------------

本轮继续从 ``origin/codex/pdf-cjk-copy-search-gate`` 的
``document-cjk-extreme-page-breaks-text`` 5 页极限分页样例中提取低风险主题，按当前
``dev`` 的 1 页轻量结构重做。该批次不整分支合并，不搬入旧分支的 inline image、
floating image、小页面压力参数或 5 页 visual baseline，只保留可静态验证的文本层、
页眉页脚、分页边界和表格治理契约。

已搬入内容：

1. 新增 ``document-cjk-extreme-page-breaks-text``，覆盖 CJK 临界分页边界、styled run、
   first/even/default 页眉页脚占位符、boundary stripe、重复表头、合并单元格、
   cant-split 行和稳定检索键 ``PB-101`` / ``PB-202`` / ``PB-303`` /
   ``FE-PB-901`` / ``FE-PB-921`` / ``FE-PB-932`` / ``FE-PB-938`` /
   ``FE-PB-999`` / ``ABC 123``。
2. ``test/pdf_regression_manifest.json`` 增加对应 manifest 条目，并将总样例数推进到
   88；该样例保持 ``expected_pages`` 为 1，避免低资源阶段扩大 PDF 渲染成本。
3. ``test/CMakeLists.txt`` 将该样例纳入 CJK font gate，并注册
   ``pdf_cjk_extreme_page_breaks_contract``，保持 60 秒超时。
4. 新增 ``test/pdf_cjk_extreme_page_breaks_contract_test.ps1``，用纯文本静态契约确认样例
   生成器、manifest、manifest parser 测试和 CMake 分类保持一致，并确认该低资源
   契约本身不调用图片或 floating image。

该补丁只表示 CJK extreme page breaks 的低资源正式 ID 契约入口已进入 ``dev``。真实多页
临界分页、图片环绕、页眉页脚文本层和 PDFium 可复制性，仍需等源码提交推送且工作区
干净后再通过受控 PDF 可视化验证确认。

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

