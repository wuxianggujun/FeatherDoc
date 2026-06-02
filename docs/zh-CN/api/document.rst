Document
========

``featherdoc::Document`` 是 ``.docx`` 文档包的根对象。通常先用它打开或创建
文档，再通过正文、页眉、页脚、分节和检查 API 完成编辑与验证。

生命周期
--------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - 方法
     - 返回值
     - 用途
   * - ``Document()``
     - ``Document``
     - 创建空句柄；调用 ``open()`` 前需要先设置路径。
   * - ``explicit Document(std::filesystem::path path)``
     - ``Document``
     - 创建绑定到指定路径的文档句柄。
   * - ``create_empty()``
     - ``std::error_code``
     - 初始化一个新的空 ``.docx`` 文档包。
   * - ``set_path(std::filesystem::path path)``
     - ``void``
     - 替换 ``open()`` 和 ``save()`` 使用的路径。
   * - ``open()``
     - ``std::error_code``
     - 加载当前路径对应的 ``.docx`` 文档包。
   * - ``save() const``
     - ``std::error_code``
     - 将修改保存回当前路径。
   * - ``save_as(std::filesystem::path path) const``
     - ``std::error_code``
     - 将修改另存为新路径。

模板部件入口
------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - 方法
     - 返回值
     - 用途
   * - ``body_template()``
     - ``TemplatePart``
     - 通过模板部件 API 访问正文。
   * - ``header_template(std::size_t index = 0U)``
     - ``TemplatePart``
     - 按物理页眉部件下标访问页眉。
   * - ``footer_template(std::size_t index = 0U)``
     - ``TemplatePart``
     - 按物理页脚部件下标访问页脚。
   * - ``section_header_template(section_index, reference_kind)``
     - ``TemplatePart``
     - 访问某个分节和引用类型解析后的页眉。
   * - ``section_footer_template(section_index, reference_kind)``
     - ``TemplatePart``
     - 访问某个分节和引用类型解析后的页脚。

分节与检查
----------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - 方法
     - 返回值
     - 用途
   * - ``append_section(bool inherit_header_footer = true)``
     - ``bool``
     - 在文档末尾追加分节。
   * - ``insert_section(std::size_t section_index, bool inherit_header_footer = true)``
     - ``bool``
     - 在指定分节前插入新分节。
   * - ``remove_section(std::size_t section_index)``
     - ``bool``
     - 删除指定分节。
   * - ``inspect_sections()``
     - ``sections_inspection_summary``
     - 返回分节、页眉和页脚检查结果。
   * - ``inspect_body_blocks()``
     - ``std::vector<body_block_inspection_summary>``
     - 检查正文中段落和表格的顺序。
   * - ``compare_semantic(const Document &other, document_semantic_diff_options options = {}) const``
     - ``std::optional<document_semantic_diff_result>``
     - 比较两个文档的语义内容差异。

示例
----

.. code-block:: cpp

   featherdoc::Document doc{"template.docx"};
   if (doc.open()) {
       return 1;
   }

   doc.body_template().replace_content_control_text_by_tag("customer", "Ada");
   return doc.save_as("filled.docx") ? 1 : 0;

英文完整页面仍保留在 :doc:`../../api/document`。
