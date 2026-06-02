API Reference
=============

The public C++ API is exposed primarily through ``include/featherdoc.hpp`` and
the ``featherdoc`` namespace. The pages below group methods by the object you
work with instead of forcing readers through one long text page.

.. toctree::
   :maxdepth: 2

   document
   content_blocks
   tables
   templates
   fields_links_reviews
   styles_numbering
   images_sections
   enums

Object Map
----------

* ``featherdoc::Document`` is the root object for loading, saving, inspecting,
  and mutating a ``.docx`` package.
* ``featherdoc::Paragraph`` and ``featherdoc::Run`` edit text content.
* ``featherdoc::Table``, ``TableRow``, and ``TableCell`` edit table structure,
  text, widths, borders, alignment, and merge state.
* ``featherdoc::TemplatePart`` applies the same content operations to body,
  header, footer, and section-specific parts.
* Summary structs such as ``content_control_summary`` and
  ``table_inspection_summary`` expose read-only inspection results.

