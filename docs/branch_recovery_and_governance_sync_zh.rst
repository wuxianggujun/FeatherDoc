分支恢复与治理同步记录
========================

本文记录 2026-05-18 的工作区恢复、分支状态和后续合并策略，避免继续在错误目录或过期
``codex/*`` 分支上推进。

当前工作区
----------

主工作区已经切换为：

.. code-block:: text

   C:\Users\wuxianggujun\CodeSpace\CMakeProjects\FeatherDoc-dev

该目录是从远端 ``dev`` 分支重新克隆的干净仓库，当前基线为：

.. code-block:: text

   dev -> 00a5d5a docs: sync release governance contracts on dev

旧目录：

.. code-block:: text

   C:\Users\wuxianggujun\CodeSpace\CMakeProjects\FeatherDoc

只作为事故现场和输出文件保留区。它缺少完整源码目录和 ``.git`` 元数据，不再作为开发、
提交或发布工作区。

分支现状
--------

``dev`` 是当前开发主线，已经包含本轮文档治理与发布治理同步提交：

.. code-block:: text

   00a5d5a docs: sync release governance contracts on dev

``master`` 暂不承载本轮治理变更。此前误先进入 ``master`` 的治理同步提交已经通过普通
``revert`` 撤回，未强推、未改写历史：

.. code-block:: text

   b33f97d revert: keep governance sync off master until release

``master`` 后续只应通过既有发布流程从 ``dev`` 合入。

``codex/*`` 分支处理策略
------------------------

已经完全合入 ``dev`` 的分支可以删除：

.. code-block:: text

   codex/bookmark-visual-regressions

暂不合入 PDF 深水区分支，除非重新明确把 PDF 作为主线：

.. code-block:: text

   codex/pdf-cjk-copy-search-gate
   codex/pdf-cjk-bullet-fallback

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
* 不触碰旧 ``FeatherDoc`` 目录。
* 不合并 PDF 深水区分支。
* 每个 PowerShell 测试单独设置 60 秒超时。
* 合并完成后先推送 ``dev``，``master`` 等发布确认后再处理。

下一步最小动作
--------------

1. 删除已经合入 ``dev`` 的旧分支 ``codex/bookmark-visual-regressions``。
2. 保留 ``codex/release-governance-rollup-details`` 作为只读参考，不直接合并。
3. 保留 ``codex/release-governance-warning-entrypoints`` 作为只读参考，不直接合并。
4. 若需要继续治理主线，从当前 ``dev`` 新建任务分支，小步手工摘录旧分支中仍有价值的
   schema calibration 或 release rollup 片段。
5. 每次摘录后运行文档治理和发布治理相关轻量测试。
6. 推送 ``dev``，``master`` 只在发布确认后从 ``dev`` 合入。
