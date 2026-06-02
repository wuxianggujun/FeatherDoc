Templates, Bookmarks, And Content Controls
==========================================

``TemplatePart`` exposes body/header/footer editing through the same content
operations. It is also the main API for bookmark filling, content-control
replacement, and schema-oriented template workflows.

TemplatePart Basics
-------------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``entry_name() const``
     - ``std::string_view``
     - Return the package entry name for this part.
   * - ``paragraphs()``
     - ``Paragraph``
     - Iterate paragraphs in this part.
   * - ``append_paragraph(text, formatting)``
     - ``Paragraph``
     - Append a paragraph.
   * - ``tables()``
     - ``Table``
     - Iterate tables in this part.
   * - ``append_table(row_count, column_count)``
     - ``Table``
     - Append a table.

Bookmarks
---------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``list_bookmarks() const``
     - ``std::vector<bookmark_summary>``
     - Enumerate bookmarks.
   * - ``find_bookmark(std::string_view bookmark_name) const``
     - ``std::optional<bookmark_summary>``
     - Find a bookmark by name.
   * - ``replace_bookmark_text(bookmark_name, replacement)``
     - ``std::size_t``
     - Replace text inside matching bookmarks.
   * - ``fill_bookmarks(std::span<const bookmark_text_binding> bindings)``
     - ``bookmark_fill_result``
     - Fill multiple bookmarks in one call.
   * - ``replace_bookmark_with_paragraphs(bookmark_name, paragraphs)``
     - ``std::size_t``
     - Replace a bookmark with paragraph content.
   * - ``replace_bookmark_with_table(bookmark_name, rows)``
     - ``std::size_t``
     - Replace a bookmark with a table.
   * - ``replace_bookmark_with_image(bookmark_name, image_path)``
     - ``std::size_t``
     - Replace a bookmark with an image.
   * - ``set_bookmark_block_visibility(bookmark_name, visible)``
     - ``std::size_t``
     - Show or hide a bookmarked block.

Content Controls
----------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``list_content_controls() const``
     - ``std::vector<content_control_summary>``
     - Enumerate structured document tags.
   * - ``find_content_controls_by_tag(std::string_view tag) const``
     - ``std::vector<content_control_summary>``
     - Find controls by tag.
   * - ``find_content_controls_by_alias(std::string_view alias) const``
     - ``std::vector<content_control_summary>``
     - Find controls by alias.
   * - ``replace_content_control_text_by_tag(tag, replacement)``
     - ``std::size_t``
     - Replace plain text by tag.
   * - ``replace_content_control_with_paragraphs_by_tag(tag, paragraphs)``
     - ``std::size_t``
     - Replace a content control with paragraphs.
   * - ``replace_content_control_with_table_by_tag(tag, rows)``
     - ``std::size_t``
     - Replace a content control with a table.
   * - ``replace_content_control_with_image_by_tag(tag, image_path)``
     - ``std::size_t``
     - Replace a content control with an image.
   * - ``set_content_control_form_state_by_tag(tag, options)``
     - ``std::size_t``
     - Set checkbox, date, dropdown, combo-box, lock, or data-binding state.

Template Schema
---------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``validate_template(requirements) const``
     - ``template_validation_result``
     - Validate required bookmark/content-control slots.
   * - ``validate_template_schema(schema) const``
     - ``template_schema_validation_result``
     - Validate against a structured schema.
   * - ``scan_template_schema(options = {})``
     - ``std::optional<template_schema_scan_result>``
     - Generate a schema-like scan from a document.
   * - ``build_template_schema_patch_from_scan(baseline, options = {})``
     - ``std::optional<template_schema_patch>``
     - Build a patch between a baseline and current scan.
   * - ``onboard_template(options = {})``
     - ``std::optional<template_onboarding_result>``
     - Run the onboarding workflow for a template.

Example
-------

.. code-block:: cpp

   auto body = doc.body_template();
   body.fill_bookmarks({
       {"invoice_no", "INV-2026-001"},
       {"customer_name", "Ada Lovelace"},
   });
   body.replace_content_control_text_by_tag("status", "approved");

