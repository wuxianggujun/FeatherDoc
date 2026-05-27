DOCX 功能 smoke 准入说明
========================

目标
----

``scripts/check_docx_functional_smoke_readiness.ps1`` 是一个低资源、只读的
DOCX 功能 smoke 准入入口。它用于确认当前仓库仍保留可追溯的文档功能证据链，
但不会启动 ``Word``、``LibreOffice``、浏览器、``CMake``、``CTest`` 或重新渲染
文档。

固定输出 schema 为 ``featherdoc.docx_functional_smoke_readiness.v1``，固定标记为
``docx_functional_smoke_readiness_trace``。

检查范围
--------

该入口覆盖以下文档功能证据：

* DOCX 包完整性：``samples/chinese_invoice_template.docx`` 和
  ``samples/my_test.docx`` 必须包含核心 OOXML entry。
* 段落与 run：段落插入、run 插入、样式 run 和 part paragraph 样例必须存在。
* 表格：表格插入、模板表格、单元格 margin、重复表头和渲染计划测试必须存在。
* 图片：part image、append image、content-control image replacement 和扩展图片格式
  样例必须存在。
* section / header / footer：section page setup、section part refs、part template
  validation、render-from-data 测试和 fixture helper 必须存在。
* content-control / Custom XML：rich/image replacement 样例、data-binding governance
  脚本与测试必须存在。
* 字段与页码：generic fields、page number fields 和 Word visual release gate contract
  测试必须存在。
* 模板数据渲染：中文发票模板、render data、mapping、orchestration 脚本与测试必须存在。
* Word visual gate 入口：``run_word_visual_smoke.ps1``、
  ``run_word_visual_release_gate.ps1`` 和 review report 测试必须存在。

可视化证据复用
--------------

资源受限时，该入口默认复用既有 Word visual smoke 产物：

* ``output/word-visual-smoke-minimal-20260519``
* ``output/word-visual-smoke-rerun-20260519``

每个产物必须包含：

* ``report/summary.json``
* ``report/review_result.json``
* ``table_visual_smoke.pdf``
* ``evidence/contact_sheet.png``
* ``evidence/pages/page-01.png``

脚本会对 contact sheet 和 page PNG 做像素抽检，确认不是空白图。该检查只能说明
持久化视觉证据可读、非空；如果 ``review_result.json`` 仍是
``pending_manual_review``，summary 会输出 ``verdict = pass_with_warnings``，并保留
``word_visual_smoke.pending_manual_review`` warning。

边界
----

``pass_with_warnings`` 表示 DOCX 功能 smoke 证据链完整，且复用的 PNG 视觉证据非空；
它不等于新鲜 ``Word COM`` 渲染已完成，也不等于人工/AI 截图级视觉审查已经给出
``pass``。

如果需要发布级 Word 视觉结论，仍应在资源允许时运行：

.. code-block:: powershell

   powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_word_visual_release_gate.ps1 -SkipBuild

低资源默认验证入口：

.. code-block:: powershell

   powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\check_docx_functional_smoke_readiness.ps1 `
     -OutputJson .\output\docx-functional-smoke-readiness-current\summary.json
