TemplatePart
============

``featherdoc::TemplatePart`` 是正文、页眉和页脚部件的统一编辑入口。
它把同一组模板操作应用到不同的 ``.docx`` 包部件上，适合处理书签填充、
内容控件替换，以及部件级模板校验。

类型化签名导读
--------------

.. FDOC_ZH_CN_TEMPLATE_PART_TYPED_SIGNATURE_GUIDE

``TemplatePart`` 用于部件级编辑。只有 ``operator bool()`` 返回 ``true`` 时，
它才指向可用的包内 XML 部件。返回 ``std::size_t`` 的方法表示匹配并修改了
多少个书签或内容控件；返回 ``bool`` 的方法表示请求的 XML 修改是否成功应用。

.. list-table::
   :header-rows: 1
   :widths: 38 34 28

   * - 签名
     - 参数
     - 返回语义
   * - ``explicit operator bool() const noexcept``
     - 无。
     - 指向可用包内 XML 部件时为 ``true``。
   * - ``std::string_view entry_name() const noexcept``
     - 无。
     - 包内条目名，例如 ``word/document.xml``。
   * - ``Paragraph append_paragraph(const std::string &text = {}, formatting_flag formatting = formatting_flag::none)``
     - ``text``：插入段落文本。``formatting``：可选首个 run 格式。
     - 返回当前部件中新建的段落句柄。
   * - ``Table append_table(std::size_t row_count = 1U, std::size_t column_count = 1U)``
     - ``row_count`` 和 ``column_count``：初始表格形状。
     - 返回当前部件中新建的表格句柄。
   * - ``bookmark_fill_result fill_bookmarks(std::span<const bookmark_text_binding> bindings)``
     - ``bindings``：书签名和文本绑定。
     - 返回匹配、替换和缺失书签统计。
   * - ``std::size_t replace_bookmark_text(const std::string &bookmark_name, const std::string &replacement)``
     - ``bookmark_name``：目标书签。``replacement``：插入文本。
     - 返回被替换的书签实例数量。
   * - ``std::size_t replace_content_control_text_by_tag(std::string_view tag, std::string_view replacement)``
     - ``tag``：内容控件 tag。``replacement``：插入文本。
     - 返回被替换的匹配控件数量。
   * - ``std::size_t replace_content_control_with_table_by_tag(std::string_view tag, const std::vector<std::vector<std::string>> &rows)``
     - ``tag``：目标控件。``rows``：生成表格的行/单元格文本矩阵。
     - 返回被替换为表格的匹配控件数量。
   * - ``template_validation_result validate_template(std::span<const template_slot_requirement> requirements) const``
     - ``requirements``：当前部件必需的书签/内容控件槽位。
     - 返回缺失和类型不匹配槽位的校验结果。
   * - ``std::size_t append_hyperlink(std::string_view text, std::string_view target)``
     - ``text``：超链接显示文本。``target``：URL 或关系目标。
     - 返回创建的超链接数量；``0`` 表示未追加。

部件基础
--------

每个 ``TemplatePart`` 都对应文档包中的一个 XML 部件。正文通常来自
``Document::body_template()``，页眉和页脚则来自对应的 header/footer 入口。

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - 方法
     - 返回值
     - 用途
   * - ``entry_name()``
     - ``std::string_view``
     - 返回当前部件在 ``.docx`` 包中的条目名。
   * - ``paragraphs()``
     - ``Paragraph``
     - 遍历当前部件中的段落。
   * - ``append_paragraph(text, formatting)``
     - ``Paragraph``
     - 在当前部件末尾追加段落。
   * - ``tables()``
     - ``Table``
     - 遍历当前部件中的表格。
   * - ``append_table(row_count, column_count)``
     - ``Table``
     - 在当前部件末尾追加表格。

书签
----

书签 API 适合处理合同、报价单、通知书等模板中的命名占位区域。可以只替换
文本，也可以把书签位置替换成段落、表格或图片。

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - 方法
     - 返回值
     - 用途
   * - ``list_bookmarks() const``
     - ``std::vector<bookmark_summary>``
     - 枚举当前部件中的书签。
   * - ``find_bookmark(std::string_view bookmark_name) const``
     - ``std::optional<bookmark_summary>``
     - 按书签名查找单个书签。
   * - ``replace_bookmark_text(bookmark_name, replacement)``
     - ``std::size_t``
     - 替换匹配书签内部的文本。
   * - ``fill_bookmarks(std::span<const bookmark_text_binding> bindings)``
     - ``bookmark_fill_result``
     - 一次填充多个书签，并返回填充结果。
   * - ``replace_bookmark_with_paragraphs(bookmark_name, paragraphs)``
     - ``std::size_t``
     - 用段落内容替换指定书签。
   * - ``replace_bookmark_with_table(bookmark_name, rows)``
     - ``std::size_t``
     - 用表格替换指定书签。
   * - ``replace_bookmark_with_image(bookmark_name, image_path)``
     - ``std::size_t``
     - 用图片替换指定书签。
   * - ``set_bookmark_block_visibility(bookmark_name, visible)``
     - ``std::size_t``
     - 显示或隐藏带书签的块级内容。

内容控件
--------

内容控件对应 Word 中的结构化文档标记。实际模板中通常使用 tag 作为稳定标识，
因此按 tag 替换文本、表格、图片或表单状态是最常见的工作流。

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - 方法
     - 返回值
     - 用途
   * - ``list_content_controls() const``
     - ``std::vector<content_control_summary>``
     - 枚举当前部件中的内容控件。
   * - ``find_content_controls_by_tag(std::string_view tag) const``
     - ``std::vector<content_control_summary>``
     - 按 tag 查找内容控件。
   * - ``find_content_controls_by_alias(std::string_view alias) const``
     - ``std::vector<content_control_summary>``
     - 按 alias 查找内容控件。
   * - ``replace_content_control_text_by_tag(tag, replacement)``
     - ``std::size_t``
     - 按 tag 替换内容控件中的纯文本。
   * - ``replace_content_control_with_paragraphs_by_tag(tag, paragraphs)``
     - ``std::size_t``
     - 按 tag 用段落替换内容控件。
   * - ``replace_content_control_with_table_by_tag(tag, rows)``
     - ``std::size_t``
     - 按 tag 用表格替换内容控件。
   * - ``replace_content_control_with_image_by_tag(tag, image_path)``
     - ``std::size_t``
     - 按 tag 用图片替换内容控件。
   * - ``set_content_control_form_state_by_tag(tag, options)``
     - ``std::size_t``
     - 设置复选框、日期、下拉框、组合框、锁定或数据绑定状态。

模板校验
--------

.. FDOC_ZH_CN_TEMPLATE_PART_TEMPLATE_VALIDATION

模板校验 API 用于把当前部件的必需槽位显式化，并检查必需书签和内容控件。

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - 方法
     - 返回值
     - 用途
   * - ``validate_template(requirements) const``
     - ``template_validation_result``
     - 按必需槽位校验书签和内容控件。

文档级 Schema 工作流
------------------------

.. FDOC_ZH_CN_TEMPLATE_PART_DOCUMENT_LEVEL_SCHEMA_WORKFLOWS

``validate_template_schema(...)``、``scan_template_schema(...)``、
``build_template_schema_patch_from_scan(...)`` 和 ``onboard_template(...)``
是 ``featherdoc::Document`` API，因为它们会聚合正文、页眉、页脚和分节目标槽位。
``TemplatePart`` 负责部件级填充和 ``validate_template(...)`` 校验；
整篇文档的 schema 治理应从 ``Document`` 入口执行。

示例
----

.. code-block:: cpp

   featherdoc::Document doc{"template.docx"};
   if (doc.open()) {
       return 1;
   }

   auto body = doc.body_template();
   body.fill_bookmarks({
       {"invoice_no", "INV-2026-001"},
       {"customer_name", "Ada Lovelace"},
   });
   body.replace_content_control_text_by_tag("status", "approved");

   auto validation = body.validate_template({
       {"invoice_no", featherdoc::template_slot_kind::text, true},
       {"status", featherdoc::template_slot_kind::text, false},
   });
   auto onboarding = doc.onboard_template();

   return (!validation || doc.save_as("filled.docx")) ? 1 : 0;
