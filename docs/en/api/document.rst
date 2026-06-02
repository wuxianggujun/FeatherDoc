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
   * - ``open()``
     - ``std::error_code``
     - Load the current ``.docx`` package.
   * - ``save() const``
     - ``std::error_code``
     - Save changes back to the current path.
   * - ``save_as(std::filesystem::path path) const``
     - ``std::error_code``
     - Save changes to a new path.

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

Sections And Inspection
-----------------------

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
   * - ``inspect_sections()``
     - ``sections_inspection_summary``
     - Return section/header/footer inspection data.
   * - ``inspect_body_blocks()``
     - ``std::vector<body_block_inspection_summary>``
     - Inspect paragraph/table order in the body.
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

The complete legacy page remains available at :doc:`../../api/document`.
