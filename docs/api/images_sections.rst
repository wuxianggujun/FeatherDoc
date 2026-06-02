Images And Sections
===================

Image APIs are available on ``Document`` and ``TemplatePart``. Section APIs live
on ``Document`` because they modify document-level structure and relationships.

Images
------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``TemplatePart::append_image(image_path)``
     - ``bool``
     - Append an inline image using source dimensions.
   * - ``TemplatePart::append_image(image_path, width_px, height_px)``
     - ``bool``
     - Append an inline image with explicit pixel size.
   * - ``TemplatePart::append_floating_image(image_path, options)``
     - ``bool``
     - Append a floating image.
   * - ``Document::drawing_images() const``
     - ``std::vector<drawing_image_info>``
     - Enumerate drawing images.
   * - ``extract_drawing_image(image_index, output_path) const``
     - ``bool``
     - Extract one drawing image.
   * - ``replace_drawing_image(image_index, image_path)``
     - ``bool``
     - Replace one drawing image.
   * - ``inline_images() const``
     - ``std::vector<inline_image_info>``
     - Enumerate inline images.
   * - ``extract_inline_image(image_index, output_path) const``
     - ``bool``
     - Extract one inline image.
   * - ``replace_inline_image(image_index, image_path)``
     - ``bool``
     - Replace one inline image.

Sections And Headers
--------------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``header_count() const`` / ``footer_count() const``
     - ``std::size_t``
     - Count physical header/footer parts.
   * - ``header_paragraphs(std::size_t index = 0U)``
     - ``Paragraph &``
     - Edit physical header paragraphs.
   * - ``footer_paragraphs(std::size_t index = 0U)``
     - ``Paragraph &``
     - Edit physical footer paragraphs.
   * - ``section_header_paragraphs(section_index, reference_kind)``
     - ``Paragraph &``
     - Edit section-resolved header paragraphs.
   * - ``ensure_section_header_paragraphs(section_index, reference_kind)``
     - ``Paragraph &``
     - Create or reuse section header paragraphs.
   * - ``assign_section_header_paragraphs(section_index, header_index, reference_kind)``
     - ``Paragraph &``
     - Attach an existing header part to a section.
   * - ``remove_section_header_reference(section_index, reference_kind)``
     - ``bool``
     - Remove a section header reference.
   * - ``replace_section_header_text(section_index, replacement_text, reference_kind)``
     - ``bool``
     - Replace section header text.
   * - ``copy_section_header_references(source_section_index, target_section_index)``
     - ``bool``
     - Copy header references between sections.

Page Setup
----------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Struct / Method
     - Key fields
     - Purpose
   * - ``section_page_setup``
     - ``orientation``, ``page_width_twips``, ``page_height_twips``, ``margins``
     - Holds Word section page setup.
   * - ``page_margins``
     - ``top_twips``, ``right_twips``, ``bottom_twips``, ``left_twips``
     - Holds page margins.
   * - ``get_section_page_setup(section_index) const``
     - returns ``std::optional<section_page_setup>``
     - Read setup for a section.
   * - ``set_section_page_setup(section_index, setup)``
     - returns ``bool``
     - Write setup for a section.

