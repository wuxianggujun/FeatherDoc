Sections And Page Setup
=======================

Section APIs live on ``featherdoc::Document`` because they edit document-level
structure, page setup, and relationships to header and footer parts. Header and
footer content can then be edited through ``featherdoc::TemplatePart`` or the
paragraph handles returned by the section helpers.

Typed Signature Guide
---------------------

.. FDOC_EN_SECTIONS_TYPED_SIGNATURE_GUIDE

Section indexes are zero-based. ``reference_kind`` selects the default,
first-page, or even-page header/footer reference. Methods returning
``TemplatePart`` or ``Paragraph &`` may point at an unavailable part unless you
use an ``ensure_*`` helper first.

.. list-table::
   :header-rows: 1
   :widths: 38 34 28

   * - Signature
     - Parameters
     - Return semantics
   * - ``bool append_section(bool inherit_header_footer = true)``
     - ``inherit_header_footer``: copy previous section header/footer references.
     - ``true`` when a section was appended.
   * - ``bool insert_section(std::size_t section_index, bool inherit_header_footer = true)``
     - ``section_index``: insertion point. ``inherit_header_footer``: copy nearby references.
     - ``true`` when the section was inserted.
   * - ``bool move_section(std::size_t source_section_index, std::size_t target_section_index)``
     - ``source_section_index``: section to move. ``target_section_index``: destination after removal.
     - ``true`` when the section order changed.
   * - ``std::optional<section_page_setup> get_section_page_setup(std::size_t section_index) const``
     - ``section_index``: target section.
     - Empty when the section or setup cannot be resolved.
   * - ``bool set_section_page_setup(std::size_t section_index, const section_page_setup &setup)``
     - ``section_index``: target section. ``setup``: size, margins, orientation, and page-number data.
     - ``true`` when page setup was written.
   * - ``TemplatePart section_header_template(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index``: target section. ``reference_kind``: default, first, or even reference.
     - Template part handle for the resolved header.
   * - ``Paragraph &ensure_section_footer_paragraphs(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index`` and ``reference_kind``: footer target to create or reuse.
     - Paragraph entry for the ensured footer part.
   * - ``bool remove_section_header_reference(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index`` and ``reference_kind``: reference to detach.
     - ``true`` when the section reference was removed.
   * - ``bool replace_section_footer_text(std::size_t section_index, std::string_view replacement_text, section_reference_kind reference_kind = default_reference)``
     - ``replacement_text``: full footer text for the resolved part.
     - ``true`` when footer text was replaced.

Section Info Types
------------------

.. list-table::
   :header-rows: 1
   :widths: 34 26 40

   * - Type
     - Key fields
     - Purpose
   * - ``featherdoc::section_page_setup``
     - ``orientation``, ``width_twips``, ``height_twips``, ``margins``
     - Stores page size, orientation, margins, and optional page number start.
   * - ``featherdoc::page_margins``
     - ``top_twips``, ``left_twips``, ``header_twips``, ``footer_twips``
     - Stores section margins in twips.
   * - ``featherdoc::sections_inspection_summary``
     - ``section_count``, ``header_count``, ``footer_count``
     - Summarizes section and related-part state across the document.
   * - ``featherdoc::section_inspection_summary``
     - ``index``, ``header``, ``footer``
     - Describes one section and its resolved header/footer links.

Section Structure
-----------------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - Method
     - Returns
     - Purpose
   * - ``append_section(inherit_header_footer = true)``
     - ``bool``
     - Append a section break, optionally inheriting header/footer references.
   * - ``insert_section(section_index, inherit_header_footer = true)``
     - ``bool``
     - Insert a section before a target section index.
   * - ``remove_section(section_index)``
     - ``bool``
     - Remove one section.
   * - ``move_section(source_section_index, target_section_index)``
     - ``bool``
     - Move a section to a new index.
   * - ``section_count() const``
     - ``std::size_t``
     - Count sections in the document.
   * - ``inspect_sections()``
     - ``sections_inspection_summary``
     - Inspect section count and header/footer linkage.
   * - ``inspect_section(section_index)``
     - ``std::optional<section_inspection_summary>``
     - Inspect one section.

Page Setup
----------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - Method
     - Returns
     - Purpose
   * - ``get_section_page_setup(section_index) const``
     - ``std::optional<section_page_setup>``
     - Read page size, margins, orientation, and page numbering metadata.
   * - ``set_section_page_setup(section_index, setup)``
     - ``bool``
     - Write page setup metadata for one section.

Headers And Footers
-------------------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - Method
     - Returns
     - Purpose
   * - ``header_count() const`` / ``footer_count() const``
     - ``std::size_t``
     - Count physical header and footer parts.
   * - ``header_template(index = 0U)`` / ``footer_template(index = 0U)``
     - ``TemplatePart``
     - Edit a physical header or footer part.
   * - ``section_header_template(section_index, reference_kind)``
     - ``TemplatePart``
     - Edit the header resolved for one section and reference kind.
   * - ``section_footer_template(section_index, reference_kind)``
     - ``TemplatePart``
     - Edit the footer resolved for one section and reference kind.
   * - ``header_paragraphs(index = 0U)`` / ``footer_paragraphs(index = 0U)``
     - ``Paragraph &``
     - Edit physical header or footer paragraphs.
   * - ``section_header_paragraphs(section_index, reference_kind)``
     - ``Paragraph &``
     - Edit resolved section header paragraphs.
   * - ``section_footer_paragraphs(section_index, reference_kind)``
     - ``Paragraph &``
     - Edit resolved section footer paragraphs.
   * - ``ensure_section_header_paragraphs(section_index, reference_kind)``
     - ``Paragraph &``
     - Create or reuse a section header part, then return paragraph access.
   * - ``ensure_section_footer_paragraphs(section_index, reference_kind)``
     - ``Paragraph &``
     - Create or reuse a section footer part, then return paragraph access.
   * - ``assign_section_header_paragraphs(section_index, header_index, reference_kind)``
     - ``Paragraph &``
     - Attach an existing header part to one section.
   * - ``assign_section_footer_paragraphs(section_index, footer_index, reference_kind)``
     - ``Paragraph &``
     - Attach an existing footer part to one section.
   * - ``remove_section_header_reference(section_index, reference_kind)``
     - ``bool``
     - Remove one section header reference.
   * - ``remove_section_footer_reference(section_index, reference_kind)``
     - ``bool``
     - Remove one section footer reference.
   * - ``replace_section_header_text(section_index, replacement_text, reference_kind)``
     - ``bool``
     - Replace text in a section header.
   * - ``replace_section_footer_text(section_index, replacement_text, reference_kind)``
     - ``bool``
     - Replace text in a section footer.
   * - ``copy_section_header_references(source_section_index, target_section_index)``
     - ``bool``
     - Copy header references between sections.
   * - ``copy_section_footer_references(source_section_index, target_section_index)``
     - ``bool``
     - Copy footer references between sections.

Example
-------

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
