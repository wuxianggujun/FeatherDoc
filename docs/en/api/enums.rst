Enums And Common Option Types
=============================

Common enum and option types are shared by document, paragraph, table, image,
template, field, and review APIs. Use this page as the language-local map for
parameter values before opening the complete legacy reference.

Formatting
----------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Type
     - Values
   * - ``formatting_flag``
     - ``none``, ``bold``, ``italic``, ``underline``, ``strikethrough``,
       ``superscript``, ``subscript``, ``smallcaps``, ``shadow``.

Document Layout
---------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Type
     - Values
   * - ``section_reference_kind``
     - ``default_reference``, ``first_page``, ``even_page``.
   * - ``page_orientation``
     - ``portrait``, ``landscape``.
   * - ``paragraph_alignment``
     - ``left``, ``center``, ``right``, ``justified``, ``distribute``.
   * - ``paragraph_line_spacing_rule``
     - ``automatic``, ``at_least``, ``exact``.

Tables
------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Type
     - Values
   * - ``table_layout_mode``
     - ``autofit``, ``fixed``.
   * - ``table_alignment``
     - ``left``, ``center``, ``right``.
   * - ``row_height_rule``
     - ``automatic``, ``at_least``, ``exact``.
   * - ``cell_vertical_alignment``
     - ``top``, ``center``, ``bottom``, ``both``.
   * - ``cell_vertical_merge``
     - ``none``, ``restart``, ``continue_merge``.
   * - ``cell_border_edge``
     - ``top``, ``left``, ``bottom``, ``right``.
   * - ``table_border_edge``
     - ``top``, ``left``, ``bottom``, ``right``, ``inside_horizontal``,
       ``inside_vertical``.
   * - ``border_style``
     - ``none``, ``single``, ``double_line``, ``dashed``, ``dotted``, ``thick``.

Templates And Review
--------------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Type
     - Purpose
   * - ``bookmark_kind``
     - Bookmark classification for inspection summaries.
   * - ``content_control_kind``
     - Content-control classification for inspection summaries.
   * - ``content_control_form_kind``
     - Form field classification such as checkbox, date, dropdown, and combo
       box.
   * - ``field_kind``
     - Field classification such as generic, TOC, REF, PAGE, NUMPAGES, SEQ,
       and related field families.
   * - ``review_note_kind``
     - Footnote, endnote, or comment note kind.
   * - ``revision_kind``
     - Insertion, deletion, and related revision kinds.

The complete legacy page remains available at :doc:`../../api/enums`.

.. FDOC_API_ENUMS_EN_MARKER
