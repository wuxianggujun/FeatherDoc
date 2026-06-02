API 参考
========

中文 API 参考按对象和脚本操作分组。请从下面的对象地图直接进入具体页面，
避免在旧版根目录长文档里查找 API 能力。

语言
----

* :doc:`../index`
* :doc:`../../en/api/index`

如何阅读这些页面
----------------

.. FDOC_ZH_CN_API_HOW_TO_READ
.. FDOC_ZH_CN_API_PUBLIC_SIGNATURES
.. FDOC_ZH_CN_API_TYPED_PARAMETERS
.. FDOC_ZH_CN_API_RETURN_SEMANTICS

API 页面按公开 C++ 对象拆分，而不是继续维护一篇很长的 README 式文档。
每个专题页应尽量提供：

* 来自 ``include/featherdoc.hpp`` 的完整公开签名。
* 类型化参数、默认值、单位，以及从 0 开始的索引规则。
* 返回值语义，包括成功、失败和 ``std::optional`` 为空的含义。
* 能直接看出调用方式的短示例。

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
     - :doc:`table`
   * - ``featherdoc::inline_image_info`` / ``drawing_image_info``
     - 检查图片元数据，追加行内或浮动图片，并提取、替换或删除图片。
     - :doc:`images`
   * - ``featherdoc::section_page_setup`` / ``page_margins``
     - 管理分节结构、页面设置，以及分节解析后的页眉页脚。
     - :doc:`sections`
   * - 字段、链接、脚注、批注和修订
     - 管理 Word 字段、超链接、脚注、尾注、批注和跟踪修订。
     - :doc:`fields_links_reviews`
   * - 样式和编号
     - 检查样式、重构样式、管理编号目录和默认格式。
     - :doc:`styles_numbering`
   * - ``featherdoc::TemplatePart``
     - 把正文、页眉、页脚和分节部件统一纳入模板替换流程。
     - :doc:`template_part`
   * - 编辑计划操作
     - ``scripts/edit_document_from_plan.ps1`` 接收的 JSON operation 名称索引。
     - :doc:`edit_plan_operations`
   * - PDF 工作流
     - 实验性 ``export-pdf`` 和 ``import-pdf`` CLI 工作流、JSON 诊断和支持范围。
     - :doc:`pdf_workflow`
   * - inspection summary 结构体
     - 暴露内容控件、表格、字段、图片和语义差异等只读检查结果，以及通用枚举选项。
     - :doc:`enums`

.. toctree::
   :maxdepth: 2
   :caption: 中文核心对象

   document
   paragraph_run
   table
   images
   sections
   fields_links_reviews
   styles_numbering
   template_part
   edit_plan_operations
   pdf_workflow
   enums
