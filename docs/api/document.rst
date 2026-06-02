Document
========

``featherdoc::Document`` is the root handle for a ``.docx`` package. Use it to
open and save files, access body/header/footer template parts, inspect sections,
and call document-wide editing APIs.

Lifecycle
---------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``Document()``
     - ``Document``
     - Create an empty handle; call ``set_path()`` before ``open()``.
   * - ``explicit Document(std::filesystem::path path)``
     - ``Document``
     - Create a handle bound to a file path.
   * - ``create_empty()``
     - ``std::error_code``
     - Initialize a new empty document package.
   * - ``set_path(std::filesystem::path path)``
     - ``void``
     - Replace the path used by ``open()`` and ``save()``.
   * - ``path() const``
     - ``const std::filesystem::path &``
     - Return the current file path.
   * - ``open()``
     - ``std::error_code``
     - Load the current ``.docx`` package.
   * - ``save() const``
     - ``std::error_code``
     - Save changes back to the current path.
   * - ``save_as(std::filesystem::path path) const``
     - ``std::error_code``
     - Save changes to a new path.
   * - ``is_open() const``
     - ``bool``
     - Check whether the package is loaded.
   * - ``last_error() const``
     - ``const document_error_info &``
     - Inspect the last structured document error.

Template Part Access
--------------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``body_template()``
     - ``TemplatePart``
     - Access the document body through the template-part API.
   * - ``header_template(std::size_t index = 0U)``
     - ``TemplatePart``
     - Access a physical header part by index.
   * - ``footer_template(std::size_t index = 0U)``
     - ``TemplatePart``
     - Access a physical footer part by index.
   * - ``section_header_template(section_index, reference_kind)``
     - ``TemplatePart``
     - Access the resolved header for a section/reference kind.
   * - ``section_footer_template(section_index, reference_kind)``
     - ``TemplatePart``
     - Access the resolved footer for a section/reference kind.

Sections
--------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``append_section(bool inherit_header_footer = true)``
     - ``bool``
     - Add a section to the end of the document.
   * - ``insert_section(std::size_t section_index, bool inherit_header_footer = true)``
     - ``bool``
     - Insert a section before an existing section.
   * - ``remove_section(std::size_t section_index)``
     - ``bool``
     - Remove a section.
   * - ``move_section(source_section_index, target_section_index)``
     - ``bool``
     - Reorder sections.
   * - ``section_count() const``
     - ``std::size_t``
     - Count sections.
   * - ``inspect_sections()``
     - ``sections_inspection_summary``
     - Return section/header/footer inspection data.
   * - ``get_section_page_setup(std::size_t section_index) const``
     - ``std::optional<section_page_setup>``
     - Read page size, margins, and orientation.
   * - ``set_section_page_setup(section_index, setup)``
     - ``bool``
     - Write page size, margins, and orientation.

Body Iterators
--------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``paragraphs()``
     - ``Paragraph &``
     - Iterate and edit body paragraphs.
   * - ``tables()``
     - ``Table &``
     - Iterate and edit body tables.
   * - ``append_table(std::size_t row_count = 1U, std::size_t column_count = 1U)``
     - ``Table``
     - Append a table to the body.

Inspection And Diff
-------------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``inspect_body_blocks()``
     - ``std::vector<body_block_inspection_summary>``
     - Inspect paragraph/table order in the body.
   * - ``inspect_paragraphs()``
     - ``std::vector<paragraph_inspection_summary>``
     - Inspect body paragraphs.
   * - ``inspect_tables()``
     - ``std::vector<table_inspection_summary>``
     - Inspect body tables.
   * - ``compare_semantic(const Document &other, document_semantic_diff_options options = {}) const``
     - ``std::optional<document_semantic_diff_result>``
     - Compare semantic document content.

Example
-------

.. code-block:: cpp

   featherdoc::Document doc{"template.docx"};
   if (doc.open()) {
       return 1;
   }

   doc.body_template().replace_content_control_text_by_tag("customer", "Ada");
   return doc.save_as("filled.docx") ? 1 : 0;

