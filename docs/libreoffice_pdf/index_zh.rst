LibreOffice PDF 学习目录
========================

这是一份面向 FeatherDoc 的 LibreOffice PDF 学习记录。

目标不是一口气读完 LibreOffice 全仓源码，而是先把和 PDF
相关的关键链路、入口、模块边界和可替换点整理清楚，方便后续
评估是否把本项目的 PDF 能力切换到 LibreOffice 方案。

当前这组文档先做第一轮粗扫，后面会按“导出 / 导入 / 渲染 /
迁移差距”继续补。


已知前提
--------

- LibreOffice 源码位于 ``D:\lo``
- 你已经做过一次全量编译
- 当前阶段只做文档整理，不急着改代码
- 重点关注 PDF 相关能力，而不是整个 Office 套件


建议的学习顺序
--------------

1. 先确认 PDF 入口在哪里。
2. 再梳理导出、导入、过滤器、配置项、命令行入口。
3. 然后记录和 FeatherDoc 现有 PDF 方案的差异。
4. 最后再决定哪些能力值得迁移。


目录
----

.. toctree::
   :maxdepth: 2

   study_plan_zh
   source_map_zh
   pdf_pipeline_notes_zh
   migration_gap_notes_zh


下一步建议
----------

优先补一份 ``study_plan_zh.rst``，把你准备从
LibreOffice 里学习的 PDF 功能拆成可执行清单。
