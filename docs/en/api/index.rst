English API Reference
=====================

This language-local API entry gives the ``/en/api/`` path a stable structure.
Use the object map below to jump directly to focused API pages.

Language
--------

* :doc:`../../zh-CN/api/index`
* :doc:`../index`

How To Read These Pages
-----------------------

.. FDOC_EN_API_HOW_TO_READ
.. FDOC_EN_API_PUBLIC_SIGNATURES
.. FDOC_EN_API_TYPED_PARAMETERS
.. FDOC_EN_API_RETURN_SEMANTICS

The API pages are organized around public C++ objects rather than one long
README-style page. Each focused page should expose:

* Full public signatures from ``include/featherdoc.hpp`` where the API is a
  C++ entry point.
* Typed parameters, default values, units, and zero-based index rules.
* Return values with success/failure semantics.
* Short examples that show the intended call shape.

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
     - :doc:`paragraph_run`
   * - ``featherdoc::Table`` / ``TableRow`` / ``TableCell``
     - Table structure, rows, cells, widths, borders, alignment, and merge state.
     - :doc:`table`
   * - ``featherdoc::inline_image_info`` / ``drawing_image_info``
     - Image metadata, inline and floating insertion, extraction, replacement, and removal.
     - :doc:`images`
   * - ``featherdoc::section_page_setup`` / ``page_margins``
     - Section structure, page setup, and section-resolved header/footer editing.
     - :doc:`sections`
   * - Fields, links, notes, comments, and revisions
     - Word fields, hyperlinks, footnotes, endnotes, comments, and tracked revisions.
     - :doc:`fields_links_reviews`
   * - Styles and numbering
     - Style inspection, style refactoring, numbering catalogs, and default formatting.
     - :doc:`styles_numbering`
   * - ``featherdoc::TemplatePart``
     - Shared operations over body, header, footer, and section-specific parts.
     - :doc:`template_part`
   * - Edit plan operations
     - JSON operation names accepted by ``scripts/edit_document_from_plan.ps1``.
     - :doc:`edit_plan_operations`
   * - PDF workflows
     - Experimental ``export-pdf`` and ``import-pdf`` CLI workflows, JSON
       diagnostics, and supported-scope limits.
     - :doc:`pdf_workflow`
   * - Inspection summaries
     - Read-only results and shared enum/option types used by content controls,
       tables, fields, images, and diffs.
     - :doc:`enums`

.. toctree::
   :maxdepth: 2
   :caption: Core Objects

   document
   paragraph_run
   table
   images
   sections
   fields_links_reviews
   styles_numbering
   template_part
   edit_plan_operations
   pdf_workflow
   enums
