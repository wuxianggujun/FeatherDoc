English API Reference
=====================

This language-local API entry gives the ``/en/api/`` path a stable structure.
The complete legacy API reference remains available at :doc:`../../api/index`
while the language tree gains dedicated object pages.

Object Map
----------

.. list-table::
   :header-rows: 1
   :widths: 28 38 34

   * - Object
     - What it is for
     - Entry point
   * - ``featherdoc::Document``
     - Root handle for opening, saving, inspecting, and mutating ``.docx`` files.
     - :doc:`document`
   * - ``featherdoc::Paragraph`` / ``featherdoc::Run``
     - Text content and run-level formatting.
     - :doc:`../../api/content_blocks`
   * - ``featherdoc::Table`` / ``TableRow`` / ``TableCell``
     - Table structure, rows, cells, widths, borders, alignment, and merge state.
     - :doc:`../../api/tables`
   * - ``featherdoc::TemplatePart``
     - Shared operations over body, header, footer, and section-specific parts.
     - :doc:`template_part`
   * - Inspection summaries
     - Read-only results for content controls, tables, fields, images, and diffs.
     - :doc:`../../api/enums`

.. toctree::
   :maxdepth: 2
   :caption: Core Objects

   document
   template_part

Complete API Groups
-------------------

* :doc:`../../api/document`
* :doc:`../../api/content_blocks`
* :doc:`../../api/tables`
* :doc:`../../api/templates`
* :doc:`../../api/edit_plan_operations`
* :doc:`../../api/fields_links_reviews`
* :doc:`../../api/styles_numbering`
* :doc:`../../api/images_sections`
* :doc:`../../api/enums`
