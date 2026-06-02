分节和页面设置 API
==================

分节 API 位于 ``featherdoc::Document`` 上，因为它们修改文档级结构、页面设置，
以及分节到页眉和页脚部件的关系。页眉页脚内容可以继续通过
``featherdoc::TemplatePart`` 或分节辅助方法返回的段落句柄来编辑。

分节信息类型
------------

.. list-table::
   :header-rows: 1
   :widths: 34 26 40

   * - 类型
     - 关键字段
     - 用途
   * - ``featherdoc::section_page_setup``
     - ``orientation``、``width_twips``、``height_twips``、``margins``
     - 保存页面尺寸、方向、边距和可选页码起始值。
   * - ``featherdoc::page_margins``
     - ``top_twips``、``left_twips``、``header_twips``、``footer_twips``
     - 保存分节边距，单位是 twips。
   * - ``featherdoc::sections_inspection_summary``
     - ``section_count``、``header_count``、``footer_count``
     - 汇总整篇文档的分节和相关页眉页脚部件状态。
   * - ``featherdoc::section_inspection_summary``
     - ``index``、``header``、``footer``
     - 描述一个分节及其解析后的页眉页脚链接。

分节结构
--------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - 方法
     - 返回值
     - 用途
   * - ``append_section(inherit_header_footer = true)``
     - ``bool``
     - 追加分节符，可选择继承页眉页脚引用。
   * - ``insert_section(section_index, inherit_header_footer = true)``
     - ``bool``
     - 在目标分节索引前插入分节。
   * - ``remove_section(section_index)``
     - ``bool``
     - 删除一个分节。
   * - ``move_section(source_section_index, target_section_index)``
     - ``bool``
     - 将分节移动到新的索引位置。
   * - ``section_count() const``
     - ``std::size_t``
     - 统计文档中的分节数量。
   * - ``inspect_sections()``
     - ``sections_inspection_summary``
     - 检查分节数量和页眉页脚链接。
   * - ``inspect_section(section_index)``
     - ``std::optional<section_inspection_summary>``
     - 检查一个分节。

页面设置
--------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - 方法
     - 返回值
     - 用途
   * - ``get_section_page_setup(section_index) const``
     - ``std::optional<section_page_setup>``
     - 读取页面尺寸、边距、方向和页码元数据。
   * - ``set_section_page_setup(section_index, setup)``
     - ``bool``
     - 写入一个分节的页面设置元数据。

页眉和页脚
----------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - 方法
     - 返回值
     - 用途
   * - ``header_count() const`` / ``footer_count() const``
     - ``std::size_t``
     - 统计物理页眉和页脚部件数量。
   * - ``header_template(index = 0U)`` / ``footer_template(index = 0U)``
     - ``TemplatePart``
     - 编辑一个物理页眉或页脚部件。
   * - ``section_header_template(section_index, reference_kind)``
     - ``TemplatePart``
     - 编辑某个分节和引用类型解析后的页眉。
   * - ``section_footer_template(section_index, reference_kind)``
     - ``TemplatePart``
     - 编辑某个分节和引用类型解析后的页脚。
   * - ``header_paragraphs(index = 0U)`` / ``footer_paragraphs(index = 0U)``
     - ``Paragraph &``
     - 编辑物理页眉或页脚段落。
   * - ``section_header_paragraphs(section_index, reference_kind)``
     - ``Paragraph &``
     - 编辑分节解析后的页眉段落。
   * - ``section_footer_paragraphs(section_index, reference_kind)``
     - ``Paragraph &``
     - 编辑分节解析后的页脚段落。
   * - ``ensure_section_header_paragraphs(section_index, reference_kind)``
     - ``Paragraph &``
     - 创建或复用分节页眉部件，并返回段落访问句柄。
   * - ``ensure_section_footer_paragraphs(section_index, reference_kind)``
     - ``Paragraph &``
     - 创建或复用分节页脚部件，并返回段落访问句柄。
   * - ``assign_section_header_paragraphs(section_index, header_index, reference_kind)``
     - ``Paragraph &``
     - 将已有页眉部件绑定到某个分节。
   * - ``assign_section_footer_paragraphs(section_index, footer_index, reference_kind)``
     - ``Paragraph &``
     - 将已有页脚部件绑定到某个分节。
   * - ``remove_section_header_reference(section_index, reference_kind)``
     - ``bool``
     - 移除一个分节页眉引用。
   * - ``remove_section_footer_reference(section_index, reference_kind)``
     - ``bool``
     - 移除一个分节页脚引用。
   * - ``replace_section_header_text(section_index, replacement_text, reference_kind)``
     - ``bool``
     - 替换分节页眉文本。
   * - ``replace_section_footer_text(section_index, replacement_text, reference_kind)``
     - ``bool``
     - 替换分节页脚文本。
   * - ``copy_section_header_references(source_section_index, target_section_index)``
     - ``bool``
     - 在分节之间复制页眉引用。
   * - ``copy_section_footer_references(source_section_index, target_section_index)``
     - ``bool``
     - 在分节之间复制页脚引用。

示例
----

.. code-block:: cpp

   featherdoc::Document doc{"template.docx"};
   doc.open();

   doc.append_section();

   auto setup = doc.get_section_page_setup(0);
   if (setup) {
       setup->orientation = featherdoc::page_orientation::landscape;
       doc.set_section_page_setup(0, *setup);
   }

   auto header = doc.section_header_template(
       0, featherdoc::section_reference_kind::default_reference);
   header.append_paragraph("Quarterly report");
