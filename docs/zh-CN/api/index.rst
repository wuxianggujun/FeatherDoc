API 参考
========

中文 API 参考会按英文 API 的对象分组逐步补齐。当前先提供稳定的中文导航，
并保留到英文完整 API 页的直达链接，避免中文读者只能从 README 或长文本里查找对象能力。

语言
----

* :doc:`../../api/index`
* :doc:`../index`

对象地图
--------

.. list-table::
   :header-rows: 1
   :widths: 28 38 34

   * - 对象
     - 可以用来做什么
     - 常用入口
   * - ``featherdoc::Document``
     - 加载、保存、检查和修改 ``.docx`` 包的根对象。
     - :doc:`document`
   * - ``featherdoc::Paragraph`` / ``featherdoc::Run``
     - 编辑正文文本和 run 级格式。
     - :doc:`paragraph_run`
   * - ``featherdoc::Table`` / ``TableRow`` / ``TableCell``
     - 编辑表格结构、宽度、边框、合并状态和单元格内容。
     - :doc:`../../api/tables`
   * - ``featherdoc::TemplatePart``
     - 把正文、页眉、页脚和分节部件统一纳入模板替换流程。
     - :doc:`template_part`
   * - inspection summary 结构体
     - 暴露内容控件、表格、字段、图片和语义差异等只读检查结果。
     - :doc:`../../api/enums`

.. toctree::
   :maxdepth: 2
   :caption: 中文核心对象

   document
   paragraph_run
   template_part

英文 API 页面
-------------

* :doc:`../../api/document`
* :doc:`../../api/content_blocks`
* :doc:`../../api/tables`
* :doc:`../../api/templates`
* :doc:`../../api/edit_plan_operations`
* :doc:`../../api/fields_links_reviews`
* :doc:`../../api/styles_numbering`
* :doc:`../../api/images_sections`
* :doc:`../../api/enums`
