旧 Codex 分支库存记录
======================

记录日期：2026-05-18

本文记录当前仍保留在远端的 ``codex/*`` 分支，避免误以为这些分支已经全部合入
``dev``。结论是：这些分支都还有未合并提交，不能在没有归档或明确废弃决定的
情况下直接删除。


当前原则
--------

* ``dev`` 是开发主线。
* ``master`` 只在发布确认后从 ``dev`` 合入。
* 旧 ``codex/*`` 分支只作为参考库存，不再整分支 merge。
* 删除远端分支前，必须先确认该分支功能已经合入、归档，或明确废弃。
* PDF 深水区分支继续冻结；当前文档治理阶段不把 PDF 扩展作为主线。


已确认摘入 ``dev`` 的内容
-------------------------

已从旧分支安全摘入的内容只有小范围治理片段：

.. code-block:: text

   8a3084e Ignore local test work directories
   f9cc0e0 Guard release warning docs contract

其中 ``8a3084e`` 已形成当前 ``dev`` 上的：

.. code-block:: text

   2ccaae5 Ignore local test work directories

``f9cc0e0`` 已形成当前 ``dev`` 上的：

.. code-block:: text

   436902e Guard release warning docs contract

之后的文档治理提交，例如 ``0d0b7bc`` 和 ``b5a806d``，是在当前 ``dev`` 上直接
开发，不是从旧分支 merge 或 cherry-pick。


分支清单
--------

``codex/release-governance-warning-entrypoints``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

状态：

* 未完整合并。
* 相对当前 ``dev`` 仍有大量独有治理提交。
* 其中一部分 release warning docs contract 已经被手工摘入。

代表性未合并内容包括：

* release governance blocker / warning / action item 明细展示。
* style merge review / restore audit 相关脚本与测试。
* pipeline final-rollup / fail-on-blocker 的 CTest 注册。
* release bundle、reviewer checklist、handoff 细节扩展。

处理建议：

* 暂不删除。
* 不整分支合并。
* 后续如继续推进治理主线，只允许从当前 ``dev`` 出发，手工复核并重做仍有价值的
  小块功能。


``codex/release-governance-rollup-details``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

状态：

* 未完整合并。
* 之前已预检，直接 merge 和逐提交 cherry-pick 都会产生多处冲突。
* 与当前 ``dev`` 的文档治理和 release metadata 契约重叠较深。

代表性未合并内容包括：

* release governance rollup details。
* schema confidence calibration 进入 release rollup。
* onboarding governance / content-control governance / handoff detail 透传。
* 部分 PDF unicode / RTL 相关覆盖。

处理建议：

* 暂不删除。
* 不直接合并。
* 后续若需要其中能力，应以当前 ``dev`` 的脚本和测试为准重新实现，而不是回放旧提交。


``codex/pdf-cjk-copy-search-gate``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

状态：

* 未合并。
* 属于 PDF 深水区分支。
* 相对当前 ``dev`` 包含大量 PDF CJK copy/search、font matrix、table flow 和视觉 gate
  相关提交。

代表性未合并内容包括：

* CJK PDF copy/search release gate。
* 多脚本字体矩阵样例。
* wrapped / merged / repeated table flow PDF regression samples。
* PDF text-layer 检查脚本和 manifest 扩展。

处理建议：

* 暂不删除，作为 PDF 参考库存保留。
* 当前文档治理阶段不合并。
* 只有当 PDF 被重新明确为主线时，再单独开低风险计划处理。


``codex/pdf-cjk-bullet-fallback``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

状态：

* 未合并。
* 基于 ``codex/pdf-cjk-copy-search-gate`` 继续扩展。
* 属于 PDF 深水区分支。

代表性未合并内容包括：

* CJK bullet / numbered list fallback。
* East Asia font fallback。
* CLI PDF export coverage。
* CJK table header fitting 和 gate coverage。

处理建议：

* 暂不删除，作为 PDF 参考库存保留。
* 当前不合并。
* 后续如果恢复 PDF 主线，应优先从该分支和
  ``codex/pdf-cjk-copy-search-gate`` 的关系开始梳理，避免重复处理相同 PDF 样例。


下一步建议
----------

1. 保留这四个远端分支，直到明确完成归档或废弃决策。
2. 继续在 ``dev`` 上推进文档功能主线，不再从旧分支直接整合。
3. 如果确实要清理 GitHub 分支列表，先创建只读归档记录，例如 tag、bundle 或文本清单，
   再删除远端分支。
4. 每次从旧分支借鉴功能，都应在当前 ``dev`` 上重新实现、重新测试，并在提交说明中
   写明来源和取舍。
