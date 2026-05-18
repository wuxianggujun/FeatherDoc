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

优先审查并合入 ``codex/release-governance-rollup-details``，因为它和当前文档治理、
release rollup、schema calibration 主线最接近。合入后必须运行轻量 PowerShell
契约测试，并保持单个测试 60 秒超时。

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
2. 预检 ``codex/release-governance-rollup-details`` 与当前 ``dev`` 的冲突。
3. 若冲突可控，合并到 ``dev``。
4. 运行文档治理和发布治理相关轻量测试。
5. 推送 ``dev``。
