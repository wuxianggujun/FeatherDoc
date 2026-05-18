分支恢复与治理同步记录
========================

本文记录 2026-05-18 的工作区恢复、分支状态和后续合并策略，避免继续在错误目录或过期
``codex/*`` 分支上推进。

当前工作区
----------

主工作区已经恢复并继续使用：

.. code-block:: text

   C:\Users\wuxianggujun\CodeSpace\CMakeProjects\FeatherDoc

该目录是从远端 ``dev`` 分支恢复后的有效仓库。此前临时使用的
``FeatherDoc-dev`` 已不再作为后续命令示例中的主路径。当前基线为：

.. code-block:: text

   dev -> 91162e5 Add PDF font glyph fallback guard

此前同名空目录只包含 ``doc`` 等残留内容，不能作为源码工作区。后续开发、提交和
推送均以当前 ``FeatherDoc`` 仓库为准。

最近一组已推送到 ``origin/dev`` 的 PDF 低风险基础补丁为：

.. code-block:: text

   1c5cd2b Add PDFium prebuilt provider option
   df2e4f6 Ignore temporary PDFium probe directories
   6760162 Fail fast when PDFium depot_tools bootstrap is unavailable
   bf945ae Document PDFium source bootstrap guard
   779b351 Add PDFium auto provider selection
   91162e5 Add PDF font glyph fallback guard

分支现状
--------

``dev`` 是当前开发主线，已经包含本轮文档治理、发布治理同步、PDFium provider
基础治理和 PDF 字体缺字形回退提交：

.. code-block:: text

   91162e5 Add PDF font glyph fallback guard

``master`` 暂不承载本轮治理变更。此前误先进入 ``master`` 的治理同步提交已经通过普通
``revert`` 撤回，未强推、未改写历史：

.. code-block:: text

   b33f97d revert: keep governance sync off master until release

``master`` 后续只应通过既有发布流程从 ``dev`` 合入。

``codex/*`` 分支处理策略
------------------------

当前远端仍保留的 ``codex/*`` 分支只有以下四个：

.. code-block:: text

   origin/codex/pdf-cjk-bullet-fallback
   origin/codex/pdf-cjk-copy-search-gate
   origin/codex/release-governance-rollup-details
   origin/codex/release-governance-warning-entrypoints

其中 PDF 分支不能整分支合入。它们包含大量早期 PDF CJK copy/search、bullet
fallback、表格流、字体矩阵和视觉 gate 变更，直接合并会与当前 ``dev`` 中已经
更完整的 PDF text shaping、manifest、visual gate 和文档状态互相覆盖。已经从
这些分支安全摘入的只是 PDFium provider / bootstrap 与字体缺字形回退相关的小补丁：

.. code-block:: text

   1c5cd2b Add PDFium prebuilt provider option
   df2e4f6 Ignore temporary PDFium probe directories
   6760162 Fail fast when PDFium depot_tools bootstrap is unavailable
   bf945ae Document PDFium source bootstrap guard
   779b351 Add PDFium auto provider selection
   91162e5 Add PDF font glyph fallback guard

后续如果继续推进 PDF，应从当前 ``dev`` 重新实现小块能力，而不是回放旧分支的大提交。

治理类分支需要按风险从小到大处理：

.. code-block:: text

   codex/release-governance-rollup-details
   codex/release-governance-warning-entrypoints

优先审查 ``codex/release-governance-rollup-details``，因为它和当前文档治理、
release rollup、schema calibration 主线最接近。审查结果显示该分支已经落后于
当前 ``dev``：直接 merge 和逐提交 cherry-pick 都会在 release governance 脚本、
测试和文档中产生多处冲突，并且有覆盖 ``00a5d5a`` 新治理契约的风险。因此该分支
不应直接合并；后续只允许手工摘录仍有价值的小块逻辑，并以当前 ``dev`` 的契约测试
为准。

``codex/release-governance-warning-entrypoints`` 也不应整分支合并。该分支包含
warning metadata、style merge review、restore audit、pipeline detail 等多组历史任务。
预检结果显示大多数提交与当前 ``dev`` 冲突。已经安全摘录的片段如下：

.. code-block:: text

   8a3084e Ignore local test work directories
   f9cc0e0 Guard release warning docs contract

``8a3084e`` 只补充 ``.gitignore`` 中的 ``.tmp/`` 临时目录忽略规则，避免本地小步
测试工作目录误入仓库。已通过：

.. code-block:: powershell

   git check-ignore -v .tmp\probe.txt

``f9cc0e0`` 只增强 ``check_release_metadata_docs.ps1`` 和对应 PowerShell 回归，确保
release metadata 文档继续覆盖 governance warning contract。已通过：

.. code-block:: powershell

   powershell -NoProfile -ExecutionPolicy Bypass -File .\test\check_release_metadata_docs_test.ps1 -RepoRoot . -WorkingDir .\tmp\light-test-warning-docs-contract

低资源合并准则
--------------

每次合并前后都遵循以下约束：

* 不运行 CMake、CTest、Ninja、MSBuild。
* 不启动 Word、LibreOffice、浏览器或 PDF 渲染。
* 不再使用旧空目录或临时目录作为源码工作区。
* 不整分支合并 PDF 深水区分支。
* 每个 PowerShell 测试单独设置 60 秒超时。
* 合并完成后先推送 ``dev``，``master`` 等发布确认后再处理。

下一步最小动作
--------------

1. 保留 ``codex/release-governance-rollup-details`` 作为只读参考，不直接合并。
2. 保留 ``codex/release-governance-warning-entrypoints`` 作为只读参考，不直接合并。
3. 保留 ``codex/pdf-cjk-copy-search-gate`` 和 ``codex/pdf-cjk-bullet-fallback`` 作为
   PDF 参考库存，不直接合并。
4. 若需要继续治理主线，从当前 ``dev`` 新建任务分支，小步手工摘录旧分支中仍有价值的
   schema calibration 或 release rollup 片段。
5. 每次摘录后运行文档治理和发布治理相关轻量测试。
6. 推送 ``dev``，``master`` 只在发布确认后从 ``dev`` 合入。

当前剩余分支复核结论
--------------------

截至本轮复核，``codex/release-governance-warning-entrypoints`` 中
还能低风险、低资源摘入的内容只剩 ``48d65e7 Run pipeline final-rollup failure
regression in CTest`` 这一类 CTest 注册项。按照当前约束，``CTest`` 不执行，且这类
提交只会增加维护面，不再作为本轮合并目标。

``codex/release-governance-rollup-details`` 也不再继续摘入。它和当前 ``dev`` 的
治理契约重叠太深，继续处理只会放大冲突，不符合“复用已有结果、关闭不用进程、
低资源推进”的要求。

PDF 分支的当前结论相同：``codex/pdf-cjk-copy-search-gate`` 和
``codex/pdf-cjk-bullet-fallback`` 仍有参考价值，但剩余差异太大，且会与当前
``dev`` 已有 PDF 样例、manifest、文字层检查、provider 选择逻辑和字体回退逻辑重叠。
其中 East Asia 字体缺字形 fallback 已按当前 ``dev`` 的 synthetic bold / italic 与
text shaping 结构重做为 ``91162e5``。下一步最小风险动作是只读复核其中是否还有独立
文档契约或小型配置补丁；若有，再按当前 ``dev`` 重做并单独提交。

本轮继续复核 ``codex/pdf-cjk-bullet-fallback`` 中的
``3be71b2 Fix CJK table header fitting and gate coverage``。该提交的大部分内容
仍涉及样例、manifest、视觉 gate 和字体矩阵，不能整体摘入；但其中 PDF 表格分页判断
可以按当前 ``dev`` 结构低风险重做：分页和重复表头适配时，不再只用当前行高度估算，
而是通过 ``spanned_row_bottom`` 计算纵向合并或跨行单元格的实际输出高度，避免跨行
表格在页底被低估。该修复只更新 ``pdf_document_adapter_tables.cpp``，没有引入旧分支
的大量 gate 或 manifest 差异。
