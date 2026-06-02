分节和页面设置 API
==================

分节 API 位于 ``featherdoc::Document`` 上，因为它们修改文档级结构、页面设置，
以及分节到页眉和页脚部件的关系。页眉页脚内容可以继续通过
``featherdoc::TemplatePart`` 或分节辅助方法返回的段落句柄来编辑。

类型化签名导读
--------------

.. FDOC_ZH_CN_SECTIONS_TYPED_SIGNATURE_GUIDE

分节索引从 0 开始。``reference_kind`` 用于选择默认页、首页或偶数页页眉/页脚引用。
返回 ``TemplatePart`` 或 ``Paragraph &`` 的方法可能指向不可用部件；如果需要自动创建
部件，应优先使用 ``ensure_*`` 辅助方法。

.. list-table::
   :header-rows: 1
   :widths: 38 34 28

   * - 签名
     - 参数
     - 返回语义
   * - ``bool append_section(bool inherit_header_footer = true)``
     - ``inherit_header_footer``：是否复制上一分节的页眉页脚引用。
     - 分节追加成功时返回 ``true``。
   * - ``bool insert_section(std::size_t section_index, bool inherit_header_footer = true)``
     - ``section_index``：插入位置。``inherit_header_footer``：是否复制相邻引用。
     - 分节插入成功时返回 ``true``。
   * - ``bool move_section(std::size_t source_section_index, std::size_t target_section_index)``
     - ``source_section_index``：要移动的分节。``target_section_index``：移除后的目标位置。
     - 分节顺序修改成功时返回 ``true``。
   * - ``std::optional<section_page_setup> get_section_page_setup(std::size_t section_index) const``
     - ``section_index``：目标分节。
     - 分节或页面设置无法解析时返回空值。
   * - ``bool set_section_page_setup(std::size_t section_index, const section_page_setup &setup)``
     - ``section_index``：目标分节。``setup``：尺寸、边距、方向和页码数据。
     - 页面设置写入成功时返回 ``true``。
   * - ``TemplatePart section_header_template(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index``：目标分节。``reference_kind``：默认页、首页或偶数页引用。
     - 返回解析后的页眉模板部件句柄。
   * - ``Paragraph &ensure_section_footer_paragraphs(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index`` 和 ``reference_kind``：要创建或复用的页脚目标。
     - 返回已确保可用的页脚段落入口。
   * - ``bool remove_section_header_reference(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index`` 和 ``reference_kind``：要解除的引用。
     - 分节引用移除成功时返回 ``true``。
   * - ``bool replace_section_footer_text(std::size_t section_index, std::string_view replacement_text, section_reference_kind reference_kind = default_reference)``
     - ``replacement_text``：写入解析后页脚部件的完整文本。
     - 页脚文本替换成功时返回 ``true``。

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
