TemplatePart
============

``featherdoc::TemplatePart`` is the shared editing surface for document body,
header, footer, and section-resolved parts. Use it when a workflow needs the
same template operation to work outside the main body, especially bookmark
filling, content-control replacement, and part-local template validation.

Common Tasks
------------

Use this page as a part-local reference:

* Fill named bookmarks: call ``list_bookmarks()``, ``find_bookmark(...)``,
  ``replace_bookmark_text(...)``, or ``fill_bookmarks(...)``.
* Fill content controls: use ``find_content_controls_by_tag(...)`` and
  ``replace_content_control_text_by_tag(...)``.
* Replace a template slot with generated content: use the bookmark or
  content-control replacement methods for paragraphs, tables, rows, and images.
* Edit header/footer content with the same API as the body: get a
  ``TemplatePart`` from ``Document`` and then call ``paragraphs()``,
  ``tables()``, ``append_paragraph(...)``, or ``append_table(...)``.
* Validate a local template surface: use ``validate_template(...)`` before
  writing values.

Success And Failure Semantics
-----------------------------

.. list-table::
   :header-rows: 1
   :widths: 24 36 40

   * - Return shape
     - Success
     - Failure or no-op
   * - ``operator bool()``
     - ``true`` means the part points at an available package XML part.
     - ``false`` means the body/header/footer part could not be resolved.
   * - ``std::size_t``
     - Non-zero value is the count of matching slots changed or appended.
     - ``0`` means no matching bookmark, content control, hyperlink, or image
       insertion target was found.
   * - ``bool``
     - ``true`` means the requested XML mutation was applied.
     - ``false`` means the target was unavailable, unsupported, or unchanged.
   * - ``bookmark_fill_result``
     - Inspect ``matched`` and ``replaced`` for successful fills.
     - Inspect ``missing_bookmarks`` before treating the template as complete.
   * - ``template_validation_result``
     - Truthy result means required slots passed validation.
     - Missing or mismatched slots are listed in the result details.

Short Example
-------------

.. code-block:: cpp

   auto body = doc.body_template();
   if (!body) return 1;

   body.replace_bookmark_text("invoice_no", "INV-2026-001");
   body.replace_content_control_text_by_tag("status", "approved");

Typed Signature Guide
---------------------

.. FDOC_EN_TEMPLATE_PART_TYPED_SIGNATURE_GUIDE

Use ``TemplatePart`` for part-local editing. It is valid when
``operator bool()`` returns ``true``. Methods returning ``std::size_t`` report
how many matching bookmarks or content controls were changed. Methods returning
``bool`` report whether the requested XML mutation was applied.

.. list-table::
   :header-rows: 1
   :widths: 38 34 28

   * - Signature
     - Parameters
     - Return semantics
   * - ``explicit operator bool() const noexcept``
     - None.
     - ``true`` when the part points at an available package XML part.
   * - ``std::string_view entry_name() const noexcept``
     - None.
     - Package entry name such as ``word/document.xml``.
   * - ``Paragraph append_paragraph(const std::string &text = {}, formatting_flag formatting = formatting_flag::none)``
     - ``text``: inserted paragraph text. ``formatting``: optional first-run formatting.
     - New paragraph handle in the current part.
   * - ``Table append_table(std::size_t row_count = 1U, std::size_t column_count = 1U)``
     - ``row_count`` and ``column_count``: initial table shape.
     - New table handle in the current part.
   * - ``bookmark_fill_result fill_bookmarks(std::span<const bookmark_text_binding> bindings)``
     - ``bindings``: bookmark-name/text pairs.
     - Matched, replaced, and missing-bookmark counts.
   * - ``std::size_t replace_bookmark_text(const std::string &bookmark_name, const std::string &replacement)``
     - ``bookmark_name``: target bookmark. ``replacement``: inserted text.
     - Number of bookmark instances replaced.
   * - ``std::size_t replace_content_control_text_by_tag(std::string_view tag, std::string_view replacement)``
     - ``tag``: content-control tag. ``replacement``: inserted text.
     - Number of matching controls replaced.
   * - ``std::size_t replace_content_control_with_table_by_tag(std::string_view tag, const std::vector<std::vector<std::string>> &rows)``
     - ``tag``: target control. ``rows``: generated row/cell text matrix.
     - Number of matching controls replaced with a table.
   * - ``template_validation_result validate_template(std::span<const template_slot_requirement> requirements) const``
     - ``requirements``: required bookmark/content-control slots for this part.
     - Validation result with missing and mismatched slots.
   * - ``std::size_t append_hyperlink(std::string_view text, std::string_view target)``
     - ``text``: visible hyperlink text. ``target``: URL or relationship target.
     - Created hyperlink count; ``0`` means no hyperlink was added.

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

Template Validation
-------------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``validate_template(requirements) const``
     - ``template_validation_result``
     - Validate required bookmark and content-control slots.

Document-Level Schema Workflows
-------------------------------

``validate_template_schema(...)``, ``scan_template_schema(...)``,
``build_template_schema_patch_from_scan(...)``, and ``onboard_template(...)``
are ``featherdoc::Document`` APIs because they can aggregate body, header,
footer, and section-targeted template slots. Use ``TemplatePart`` for part-local
slot filling and ``validate_template(...)`` checks, then use ``Document`` for
whole-document schema governance.

Example
-------

.. code-block:: cpp

   auto part = doc.body_template();

   auto result = part.fill_bookmarks({
       {"invoice_no", "INV-2026-001"},
       {"customer_name", "Ada Lovelace"},
   });

   part.replace_content_control_text_by_tag("status", "approved");
   auto validation = part.validate_template({
       {"invoice_no", featherdoc::template_slot_kind::text, true},
       {"status", featherdoc::template_slot_kind::text, false},
   });

   if (!result.missing_bookmarks.empty() || !validation) {
       return 1;
   }

   return doc.save_as("filled.docx") ? 1 : 0;
