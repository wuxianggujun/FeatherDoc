TemplatePart
============

``featherdoc::TemplatePart`` 是正文、页眉和页脚部件的统一编辑入口。
它把同一组模板操作应用到不同的 ``.docx`` 包部件上，适合处理书签填充、
内容控件替换，以及基于 Schema 的模板校验和上架流程。

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

模板 Schema
-----------

模板 Schema API 用于把模板要求显式化。它可以校验必需的书签和内容控件，
也可以扫描现有模板，生成用于评审、补丁和上架的结构化结果。

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - 方法
     - 返回值
     - 用途
   * - ``validate_template(requirements) const``
     - ``template_validation_result``
     - 按必需槽位校验书签和内容控件。
   * - ``validate_template_schema(schema) const``
     - ``template_schema_validation_result``
     - 按结构化 Schema 校验模板。
   * - ``scan_template_schema(options = {})``
     - ``std::optional<template_schema_scan_result>``
     - 从当前部件扫描生成 Schema 风格的结果。
   * - ``build_template_schema_patch_from_scan(baseline, options = {})``
     - ``std::optional<template_schema_patch>``
     - 基于基线和当前扫描结果构建模板补丁。
   * - ``onboard_template(options = {})``
     - ``std::optional<template_onboarding_result>``
     - 执行模板上架流程，返回上架结果。

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

   auto schema_result = body.validate_template_schema(schema);
   auto onboarding = body.onboard_template();

   return doc.save_as("filled.docx") ? 1 : 0;

旧版完整页面仍保留在 :doc:`../../api/templates`。
