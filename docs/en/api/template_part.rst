TemplatePart
============

``featherdoc::TemplatePart`` is the shared editing surface for document body,
header, footer, and section-resolved parts. Use it when a workflow needs the
same template operation to work outside the main body, especially bookmark
filling, content-control replacement, and schema validation.

Part Basics
-----------

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
     - Iterate paragraph content in the part.
   * - ``append_paragraph(text, formatting)``
     - ``Paragraph``
     - Append a paragraph with optional run formatting.
   * - ``tables()``
     - ``Table``
     - Iterate table content in the part.
   * - ``append_table(row_count, column_count)``
     - ``Table``
     - Append a table with the requested shape.

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
     - Enumerate bookmark slots in this part.
   * - ``find_bookmark(bookmark_name) const``
     - ``std::optional<bookmark_summary>``
     - Find one bookmark by name.
   * - ``replace_bookmark_text(bookmark_name, replacement)``
     - ``std::size_t``
     - Replace bookmark text and return the number of matched slots.
   * - ``fill_bookmarks(bindings)``
     - ``bookmark_fill_result``
     - Fill multiple bookmarks in one call and report misses.
   * - ``replace_bookmark_with_paragraphs(bookmark_name, paragraphs)``
     - ``std::size_t``
     - Replace a bookmark with paragraph content.
   * - ``replace_bookmark_with_table(bookmark_name, rows)``
     - ``std::size_t``
     - Replace a bookmark with a generated table.
   * - ``replace_bookmark_with_image(bookmark_name, image_path)``
     - ``std::size_t``
     - Replace a bookmark with an image run.
   * - ``set_bookmark_block_visibility(bookmark_name, visible)``
     - ``std::size_t``
     - Show or hide the block that contains a bookmark.

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
   * - ``find_content_controls_by_tag(tag) const``
     - ``std::vector<content_control_summary>``
     - Find controls by business tag.
   * - ``find_content_controls_by_alias(alias) const``
     - ``std::vector<content_control_summary>``
     - Find controls by visible alias.
   * - ``replace_content_control_text_by_tag(tag, replacement)``
     - ``std::size_t``
     - Replace plain text in matching controls.
   * - ``replace_content_control_with_paragraphs_by_tag(tag, paragraphs)``
     - ``std::size_t``
     - Replace matching controls with paragraph content.
   * - ``replace_content_control_with_table_by_tag(tag, rows)``
     - ``std::size_t``
     - Replace matching controls with a table.
   * - ``replace_content_control_with_image_by_tag(tag, image_path)``
     - ``std::size_t``
     - Replace matching controls with an image.
   * - ``set_content_control_form_state_by_tag(tag, options)``
     - ``std::size_t``
     - Update checkbox, date, dropdown, combo-box, lock, or binding state.

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
     - Validate required bookmark and content-control slots.
   * - ``validate_template_schema(schema) const``
     - ``template_schema_validation_result``
     - Validate this part against a structured template schema.
   * - ``scan_template_schema(options = {})``
     - ``std::optional<template_schema_scan_result>``
     - Scan the current part into schema-oriented metadata.
   * - ``build_template_schema_patch_from_scan(baseline, options = {})``
     - ``std::optional<template_schema_patch>``
     - Compare a baseline schema scan with the current part.
   * - ``onboard_template(options = {})``
     - ``std::optional<template_onboarding_result>``
     - Run the template onboarding workflow and collect schema guidance.

Example
-------

.. code-block:: cpp

   auto part = doc.body_template();

   auto result = part.fill_bookmarks({
       {"invoice_no", "INV-2026-001"},
       {"customer_name", "Ada Lovelace"},
   });

   part.replace_content_control_text_by_tag("status", "approved");

   if (!result.missing_bookmarks.empty()) {
       return 1;
   }

   return doc.save_as("filled.docx") ? 1 : 0;

The complete legacy page remains available at :doc:`../../api/templates`.
