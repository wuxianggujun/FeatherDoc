Document
========

``featherdoc::Document`` is the root handle for a ``.docx`` package. Use it to
open and save files, access body/header/footer template parts, inspect sections,
and call document-wide editing APIs.

Common Tasks
------------

Start here when you already know the workflow you need:

* Open and save a document: use ``Document(path)``, ``open()``, ``save()``, and
  ``save_as(path)``.
* Fill a body template: use ``body_template()`` or the document-level
  ``replace_content_control_text_by_tag(...)`` shortcut.
* Work on headers or footers: use ``header_template(...)``,
  ``footer_template(...)``, ``section_header_template(...)``, or
  ``section_footer_template(...)``.
* Edit body content directly: use ``paragraphs()``, ``tables()``,
  ``append_table(...)``, and the image append methods.
* Inspect structure before changing it: use ``inspect_sections()``,
  ``inspect_body_blocks()``, ``list_bookmarks()``, ``list_content_controls()``,
  and ``last_error()``.

Success And Failure Semantics
-----------------------------

.. list-table::
   :header-rows: 1
   :widths: 24 36 40

   * - Return shape
     - Success
     - Failure or no-op
   * - ``std::error_code``
     - Empty error code means the package operation completed.
     - Non-empty error code means the operation failed; inspect
       ``last_error()`` for detail and package entry context.
   * - ``bool``
     - ``true`` means the requested document XML or package metadata changed.
     - ``false`` means the target was unavailable, invalid, or already had no
       applicable change.
   * - ``std::size_t``
     - Non-zero value is the number of matched items changed or appended.
     - ``0`` means no matching bookmark, content control, comment target,
       hyperlink, or note target was found.
   * - ``std::optional<T>``
     - Contains the requested inspection or settings value.
     - Empty means the section, setting, or semantic comparison could not be
       resolved.
   * - Handle objects
     - Returned handles such as ``TemplatePart``, ``Paragraph``, and ``Table``
       are the next editing entry point.
     - Check handle-specific validity rules before assuming the target exists.

Short Example
-------------

.. code-block:: cpp

   featherdoc::Document doc{"template.docx"};
   if (doc.open()) return 1;

   doc.replace_content_control_text_by_tag("customer", "Ada");
   return doc.save_as("filled.docx") ? 1 : 0;

Lifecycle
---------

Open, create, save, and inspect the current package state.

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - Method
     - Parameters
     - Returns
     - Purpose
   * - ``Document()``
     - None.
     - ``Document``
     - Create an empty handle; call ``set_path()`` before ``open()``.
   * - ``explicit Document(std::filesystem::path path)``
     - ``path``: source or target ``.docx`` path.
     - ``Document``
     - Create a handle bound to a file path.
   * - ``create_empty()``
     - None.
     - ``std::error_code``
     - Initialize a new empty document package.
   * - ``set_path(std::filesystem::path path)``
     - ``path``: replacement path used by ``open()`` and ``save()``.
     - ``void``
     - Replace the current path and reset loaded package state.
   * - ``path() const``
     - None.
     - ``const std::filesystem::path &``
     - Return the path currently bound to the handle.
   * - ``open()``
     - None.
     - ``std::error_code``
     - Load the current ``.docx`` package.
   * - ``save() const``
     - None.
     - ``std::error_code``
     - Save changes back to the current path.
   * - ``save_as(std::filesystem::path path) const``
     - ``path``: output ``.docx`` path; it must not be empty.
     - ``std::error_code``
     - Save changes to a new path.
   * - ``is_open() const``
     - None.
     - ``bool``
     - Return whether ``open()`` or ``create_empty()`` has loaded a package.
   * - ``last_error() const noexcept``
     - None.
     - ``const document_error_info &``
     - Inspect the latest failure ``code``, ``detail``, ``entry_name``, and
       optional XML offset.
   * - ``enable_update_fields_on_open()``
     - None.
     - ``bool``
     - Add ``w:updateFields`` so Word updates fields when it opens the file.
   * - ``clear_update_fields_on_open()``
     - None.
     - ``bool``
     - Remove the update-fields-on-open setting.
   * - ``update_fields_on_open_enabled()``
     - None.
     - ``std::optional<bool>``
     - Inspect the update-fields-on-open setting; empty means the setting could
       not be read.

Template Part Access
--------------------

Use template parts when the same operation should work on body, headers, or
footers.

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - Method
     - Parameters
     - Returns
     - Purpose
   * - ``body_template()``
     - None.
     - ``TemplatePart``
     - Access the document body through the template-part API.
   * - ``header_template(std::size_t index = 0U)``
     - ``index``: physical header part index.
     - ``TemplatePart``
     - Access a physical header part by index.
   * - ``footer_template(std::size_t index = 0U)``
     - ``index``: physical footer part index.
     - ``TemplatePart``
     - Access a physical footer part by index.
   * - ``section_header_template(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index``: zero-based section index. ``reference_kind``:
       default, first-page, or even-page header reference.
     - ``TemplatePart``
     - Access the resolved header for a section/reference kind.
   * - ``section_footer_template(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index``: zero-based section index. ``reference_kind``:
       default, first-page, or even-page footer reference.
     - ``TemplatePart``
     - Access the resolved footer for a section/reference kind.

Sections And Inspection
-----------------------

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - Method
     - Parameters
     - Returns
     - Purpose
   * - ``append_section(bool inherit_header_footer = true)``
     - ``inherit_header_footer``: copy the previous section references when
       ``true``.
     - ``bool``
     - Add a section to the end of the document.
   * - ``insert_section(std::size_t section_index, bool inherit_header_footer = true)``
     - ``section_index``: insertion point. ``inherit_header_footer``: copy
       surrounding header/footer references when ``true``.
     - ``bool``
     - Insert a section before an existing section.
   * - ``remove_section(std::size_t section_index)``
     - ``section_index``: section to remove.
     - ``bool``
     - Remove a section.
   * - ``move_section(std::size_t source_section_index, std::size_t target_section_index)``
     - ``source_section_index``: section to move. ``target_section_index``:
       destination index after removal.
     - ``bool``
     - Reorder sections without recreating the whole document.
   * - ``section_count() const noexcept``
     - None.
     - ``std::size_t``
     - Return the number of sections.
   * - ``header_count() const noexcept`` / ``footer_count() const noexcept``
     - None.
     - ``std::size_t``
     - Return physical header/footer part counts.
   * - ``inspect_sections()``
     - None.
     - ``sections_inspection_summary``
     - Return section/header/footer inspection data.
   * - ``inspect_section(std::size_t section_index)``
     - ``section_index``: zero-based section index.
     - ``std::optional<section_inspection_summary>``
     - Inspect one section; empty means the section is not available.
   * - ``get_section_page_setup(std::size_t section_index) const``
     - ``section_index``: section whose setup should be read.
     - ``std::optional<section_page_setup>``
     - Read page size, margins, orientation, and related setup.
   * - ``set_section_page_setup(std::size_t section_index, const section_page_setup &setup)``
     - ``section_index``: target section. ``setup``: page setup values.
     - ``bool``
     - Apply page setup to a section.
   * - ``replace_section_header_text(std::size_t section_index, std::string_view replacement_text, section_reference_kind reference_kind = default_reference)``
     - ``section_index``: target section. ``replacement_text``: full header
       text. ``reference_kind``: resolved header kind.
     - ``bool``
     - Replace section header text through the resolved header part.
   * - ``replace_section_footer_text(std::size_t section_index, std::string_view replacement_text, section_reference_kind reference_kind = default_reference)``
     - ``section_index``: target section. ``replacement_text``: full footer
       text. ``reference_kind``: resolved footer kind.
     - ``bool``
     - Replace section footer text through the resolved footer part.
   * - ``remove_section_header_reference(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index``: target section. ``reference_kind``: header reference
       kind to detach.
     - ``bool``
     - Remove the section reference only; it does not delete the physical part.
   * - ``remove_section_footer_reference(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index``: target section. ``reference_kind``: footer reference
       kind to detach.
     - ``bool``
     - Remove the footer reference only; it does not delete the physical part.
   * - ``inspect_body_blocks()``
     - None.
     - ``std::vector<body_block_inspection_summary>``
     - Inspect paragraph/table order in the body.
   * - ``compare_semantic(const Document &other, document_semantic_diff_options options = {}) const``
     - ``other``: document to compare against. ``options``: semantic diff
       toggles.
     - ``std::optional<document_semantic_diff_result>``
     - Compare semantic document content.

Template Filling Shortcuts
--------------------------

These methods operate from ``Document`` directly. Use them for common template
fills without first taking a ``TemplatePart``.

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - Method
     - Parameters
     - Returns
     - Purpose
   * - ``replace_bookmark_text(const std::string &bookmark_name, const std::string &replacement)``
     - ``bookmark_name``: bookmark to fill. ``replacement``: text to insert.
     - ``std::size_t``
     - Replace matching bookmark text and return the replacement count.
   * - ``fill_bookmarks(std::span<const bookmark_text_binding> bindings)``
     - ``bindings``: bookmark/text pairs to fill in one call.
     - ``bookmark_fill_result``
     - Fill multiple bookmarks and report requested, matched, replaced, and
       missing bookmarks.
   * - ``fill_bookmarks(std::initializer_list<bookmark_text_binding> bindings)``
     - ``bindings``: inline bookmark/text pairs.
     - ``bookmark_fill_result``
     - Fill a small bookmark set without constructing a separate container.
   * - ``list_bookmarks() const`` / ``find_bookmark(std::string_view bookmark_name) const``
     - ``bookmark_name``: bookmark to locate for the find overload.
     - ``std::vector<bookmark_summary>`` / ``std::optional<bookmark_summary>``
     - Inspect available bookmark slots before filling.
   * - ``replace_content_control_text_by_tag(std::string_view tag, std::string_view replacement)``
     - ``tag``: content-control tag. ``replacement``: text to insert.
     - ``std::size_t``
     - Replace matching content controls and return the replacement count.
   * - ``replace_content_control_text_by_alias(std::string_view alias, std::string_view replacement)``
     - ``alias``: content-control alias/title. ``replacement``: text to insert.
     - ``std::size_t``
     - Fill controls addressed by alias/title.
   * - ``list_content_controls() const``
     - None.
     - ``std::vector<content_control_summary>``
     - Inspect content-control tags, aliases, lock state, and data binding
       metadata before filling.
   * - ``find_content_controls_by_tag(std::string_view tag) const`` / ``find_content_controls_by_alias(std::string_view alias) const``
     - ``tag`` or ``alias``: lookup key for structured document tags.
     - ``std::vector<content_control_summary>``
     - Locate fill targets before mutating the document.
   * - ``replace_content_control_with_paragraphs_by_tag(std::string_view tag, const std::vector<std::string> &paragraphs)``
     - ``tag``: content-control tag. ``paragraphs``: replacement paragraph
       text.
     - ``std::size_t``
     - Replace matching controls with generated paragraphs.
   * - ``replace_content_control_with_table_rows_by_tag(std::string_view tag, const std::vector<std::vector<std::string>> &rows)``
     - ``tag``: content-control tag. ``rows``: row/cell text matrix appended
       as table rows.
     - ``std::size_t``
     - Replace matching controls with rows in the surrounding table context.
   * - ``replace_content_control_with_table_by_tag(std::string_view tag, const std::vector<std::vector<std::string>> &rows)``
     - ``tag``: content-control tag. ``rows``: generated table matrix.
     - ``std::size_t``
     - Replace matching controls with generated tables.
   * - ``replace_content_control_with_image_by_tag(std::string_view tag, const std::filesystem::path &image_path)``
     - ``tag``: content-control tag. ``image_path``: replacement image path.
     - ``std::size_t``
     - Replace matching controls with images.
   * - ``replace_content_control_with_image_by_tag(std::string_view tag, const std::filesystem::path &image_path, std::uint32_t width_px, std::uint32_t height_px)``
     - ``tag``: content-control tag. ``image_path``: replacement image path.
       ``width_px`` / ``height_px``: explicit image size.
     - ``std::size_t``
     - Replace matching controls with sized images.

Body, Tables, And Images
------------------------

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - Method
     - Parameters
     - Returns
     - Purpose
   * - ``paragraphs()``
     - None.
     - ``Paragraph &``
     - Return the body paragraph iterator/editing entry.
   * - ``tables()``
     - None.
     - ``Table &``
     - Return the body table iterator/editing entry.
   * - ``append_table(std::size_t row_count = 1U, std::size_t column_count = 1U)``
     - ``row_count`` and ``column_count``: initial table dimensions.
     - ``Table``
     - Append a table and return the created table handle.
   * - ``append_image(const std::filesystem::path &image_path)``
     - ``image_path``: image to append inline.
     - ``bool``
     - Append an inline image using natural dimensions.
   * - ``append_image(const std::filesystem::path &image_path, std::uint32_t width_px, std::uint32_t height_px)``
     - ``image_path``: image to append. ``width_px`` / ``height_px``:
       explicit image size.
     - ``bool``
     - Append an inline image with an explicit size.
   * - ``append_floating_image(const std::filesystem::path &image_path, floating_image_options options = {})``
     - ``image_path``: image to append. ``options``: floating position/wrap
       options.
     - ``bool``
     - Append a floating image.
   * - ``append_floating_image(const std::filesystem::path &image_path, std::uint32_t width_px, std::uint32_t height_px, floating_image_options options = {})``
     - ``image_path``: image to append. ``width_px`` / ``height_px``:
       explicit size. ``options``: floating position/wrap options.
     - ``bool``
     - Append a sized floating image.

Notes, Revisions, And Links
---------------------------

The detailed object pages cover these families, but the root ``Document`` API
also exposes document-wide entry points.

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - Method
     - Parameters
     - Returns
     - Purpose
   * - ``append_comment(std::string_view selected_text, std::string_view comment_text, author = {}, initials = {}, date = {})``
     - ``selected_text``: text to annotate. ``comment_text``: comment body.
       ``author`` / ``initials`` / ``date``: optional metadata.
     - ``std::size_t``
     - Add a comment; returns ``1`` on success and ``0`` when no matching text
       can be annotated.
   * - ``append_paragraph_text_comment(std::size_t paragraph_index, std::size_t text_offset, std::size_t text_length, std::string_view comment_text, ...)``
     - ``paragraph_index``: target paragraph. ``text_offset`` and
       ``text_length``: annotated range.
     - ``std::size_t``
     - Add a comment to a precise paragraph text range.
   * - ``append_text_range_comment(std::size_t start_paragraph_index, std::size_t start_text_offset, std::size_t end_paragraph_index, std::size_t end_text_offset, std::string_view comment_text, author = {}, initials = {}, date = {})``
     - Start/end paragraph indexes, text offsets, comment body, and optional
       metadata.
     - ``std::size_t``
     - Add a comment that spans a multi-paragraph text range.
   * - ``list_revisions() const``
     - None.
     - ``std::vector<revision_summary>``
     - Inspect tracked revisions.
   * - ``accept_revision(std::size_t revision_index)`` / ``reject_revision(std::size_t revision_index)``
     - ``revision_index``: revision returned by ``list_revisions()``.
     - ``bool``
     - Accept or reject one tracked revision.
   * - ``append_footnote(std::string_view reference_text, std::string_view note_text)``
     - ``reference_text``: marker text. ``note_text``: footnote body.
     - ``std::size_t``
     - Append a footnote.
   * - ``list_hyperlinks() const``
     - None.
     - ``std::vector<hyperlink_summary>``
     - Inspect hyperlink text and targets before editing.
   * - ``append_hyperlink(std::string_view text, std::string_view target)``
     - ``text``: visible text. ``target``: URL or relationship target.
     - ``std::size_t``
     - Append a hyperlink to the body.
   * - ``replace_hyperlink(std::size_t hyperlink_index, std::string_view text, std::string_view target)``
     - ``hyperlink_index``: item from ``list_hyperlinks()``. ``text`` and
       ``target``: replacement display text and URL/relationship target.
     - ``bool``
     - Replace an existing hyperlink.

Related API Families
--------------------

The root ``Document`` type also exposes these public API families. Use the
focused pages for the full method sets and examples:

* :doc:`sections`: section paragraphs, header/footer assignment, physical
  header/footer part removal, and section page setup.
* :doc:`fields_links_reviews`: comments, comment replies, resolved state,
  footnotes, endnotes, hyperlinks, and tracked revision mutations.
* :doc:`template_part`: template validation, schema scanning, onboarding,
  custom XML synchronization, and content-control form state helpers.
* :doc:`images`: inline/floating image listing, extraction, replacement, and
  removal.
* :doc:`paragraph_run` and :doc:`table`: paragraph, run, table, row, and cell
  editing entry points returned from ``Document``.
* :doc:`styles_numbering` and :doc:`enums`: style/numbering catalogs, shared
  options, and inspection summary types.

Examples
--------

Open, fill content controls, and save:

.. code-block:: cpp

   featherdoc::Document doc{"template.docx"};
   if (doc.open()) {
       return 1;
   }

   doc.body_template().replace_content_control_text_by_tag("customer", "Ada");
   return doc.save_as("filled.docx") ? 1 : 0;

Fill directly from ``Document`` and inspect failures:

.. code-block:: cpp

   featherdoc::Document doc{"template.docx"};
   if (auto error = doc.open()) {
       std::cerr << doc.last_error().detail << '\n';
       return 1;
   }

   const auto replaced =
       doc.replace_content_control_text_by_tag("customer", "Ada");
   if (replaced == 0) {
       return 1;
   }

   if (auto error = doc.save_as("filled.docx")) {
       std::cerr << doc.last_error().detail << '\n';
       return 1;
   }

   return 0;

Create a new document with a table and field-update setting:

.. code-block:: cpp

   featherdoc::Document doc;
   if (doc.create_empty()) {
       return 1;
   }

   doc.enable_update_fields_on_open();
   auto table = doc.append_table(2, 3);
   if (auto customer = table.find_cell(0, 0)) {
       customer->set_text("Customer");
   }
   if (auto status = table.find_cell(0, 1)) {
       status->set_text("Status");
   }

   return doc.save_as("report.docx") ? 1 : 0;
