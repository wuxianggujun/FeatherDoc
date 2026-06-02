Edit Plan Operations
====================

``scripts/edit_document_from_plan.ps1`` accepts JSON edit plans for scripted
``.docx`` rewrites. This page documents the plan shape, summary output, and the
most-used operation parameters so the language-local API page is useful without
opening the dispatcher script.

Plan Shape
----------

.. FDOC_EDIT_PLAN_PLAN_SHAPE

An edit plan can be either a JSON array or an object with an ``operations``
array. Every operation must provide ``op``. Hyphenated operation names are
normalized to underscores, so ``append-image`` and ``append_image`` route to the
same dispatcher branch.

.. code-block:: json

   {
     "operations": [
       {
         "op": "replace_text",
         "find": "Draft",
         "replace": "Final",
         "match_case": true
       },
       {
         "op": "append_hyperlink",
         "text": "FeatherDoc",
         "target": "https://github.com/wuxianggujun/FeatherDoc"
       }
     ]
   }

The input DOCX, output DOCX, optional ``summary_json`` path, build directory, and
``skip_build`` flag are command parameters of
``scripts/edit_document_from_plan.ps1``. Individual operations normally do not
set ``input_path`` or ``output_path``; ``apply_table_position_plan`` injects the
current ``input_path`` into the table-position plan before calling the CLI.

Execution Result
----------------

.. FDOC_EDIT_PLAN_EXECUTION_RESULT

The script writes a summary object with the run status and per-operation
records.

.. list-table::
   :header-rows: 1
   :widths: 28 72

   * - Field
     - Meaning
   * - ``status``
     - ``succeeded`` or ``failed`` for the whole edit-plan run.
   * - ``input_docx`` / ``output_docx``
     - Resolved source and destination DOCX paths.
   * - ``edit_plan``
     - Resolved JSON edit-plan path.
   * - ``summary_json``
     - Optional summary file path requested by the caller.
   * - ``operation_count``
     - Number of operation objects loaded from the plan.
   * - ``operations``
     - Per-operation records with ``index``, ``op``, ``command``,
       ``input_docx``, ``output_docx``, ``status``, ``exit_code``, ``output``,
       and optional ``error``.
   * - ``error``
     - Present only when the run fails before or during an operation.

Operation Reference
-------------------

.. FDOC_EDIT_PLAN_OPERATION_REFERENCE
.. FDOC_EDIT_PLAN_REQUIRED_FIELDS
.. FDOC_EDIT_PLAN_OPTIONAL_FIELDS

The table below focuses on operations that are most often used from JSON plans.
See the complete operation index further down for the full dispatcher surface.

Review And Revision Operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 24 28 30 18

   * - Operation
     - Required fields
     - Optional fields and aliases
     - Effect
   * - ``accept_all_revisions``
     - ``op``
     - None.
     - Calls ``accept-all-revisions`` for the whole document.
   * - ``set_comment_resolved``
     - ``op``. The dispatcher wraps fields into a review mutation plan.
     - ``comment_index``/``index``, ``resolved``, ``expected_text``/
       ``expected_anchor_text``, ``expected_comment_text``/
       ``expected_comment_body``, ``expected_resolved``,
       ``expected_parent_index``.
     - Calls ``apply-review-mutation-plan`` to update comment resolved state.
   * - ``append_comment``
     - ``selected_text``/``anchor_text`` and ``comment_text``/``text``/``body``.
     - ``author``/``comment_author``, ``initials``/``comment_initials``,
       ``date``/``comment_date``.
     - Adds a comment anchored to matching document text.
   * - ``apply_review_mutation_plan``
     - ``plan_file``/``plan_path``/``review_plan_file``/
       ``review_mutation_plan_file`` or inline ``review_plan``/``operations``.
     - Do not combine a plan file with an inline plan.
     - Replays comment mutation operations through
       ``apply-review-mutation-plan``.

Notes And Fields
~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 24 28 30 18

   * - Operation
     - Required fields
     - Optional fields and aliases
     - Effect
   * - ``append_page_number_field``
     - ``op``
     - ``part`` defaults to ``body``; ``part_index``/``index``, ``section``,
       ``kind``, ``dirty``, ``locked``.
     - Calls ``append-page-number-field`` in the selected document part.
   * - ``append_hyperlink_field``
     - ``bookmark_name``/``bookmark``/``name``.
     - ``result_text``/``text``, field state options, and part selection.
     - Appends a Word hyperlink field that targets a bookmark.
   * - ``set_update_fields_on_open``
     - ``op``
     - ``enabled`` can be supplied by the dedicated enable/disable aliases.
     - Sets the document-level update-fields-on-open flag.

Template Slots And Content Controls
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 24 28 30 18

   * - Operation
     - Required fields
     - Optional fields and aliases
     - Effect
   * - ``replace_content_control_text_by_tag``
     - ``text`` and exactly one selector: ``tag``/``content_control_tag`` or
       ``alias``/``content_control_alias``.
     - ``part`` defaults to ``body``; ``part_index``/``index``, ``section``,
       ``kind``.
     - Calls ``replace-content-control-text`` on the selected content control.
   * - ``replace_content_control_table_rows``
     - Selector fields and ``rows``.
     - Part selection fields; empty rows are accepted for this operation.
     - Replaces rows in a content-control table.
   * - ``replace_bookmark_text``
     - ``bookmark``/``bookmark_name`` and ``text``.
     - Bookmark selector options.
     - Replaces bookmark content with text.

Images, Links, And OMML
~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 24 28 30 18

   * - Operation
     - Required fields
     - Optional fields and aliases
     - Effect
   * - ``append_image``
     - ``image_path``/``image``/``path``/``file``.
     - Part selection; ``floating``; ``width``/``width_px``; ``height``/
       ``height_px``; floating placement fields such as
       ``horizontal_reference``, ``horizontal_offset``, ``vertical_reference``,
       ``vertical_offset``, ``behind_text``, ``allow_overlap``, ``z_order``,
       ``wrap_mode``, wrap distances, and crop values.
     - Calls ``append-image``. Defaults to inline image unless
       ``floating`` is true.
   * - ``append_hyperlink``
     - ``text`` and ``target``/``url``/``href``.
     - None.
     - Calls ``append-hyperlink`` to append a link.
   * - ``append_omml``
     - ``xml``/``omml``/``omml_xml``.
     - None.
     - Appends an OMML equation.

Text And Paragraph Mutations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 24 28 30 18

   * - Operation
     - Required fields
     - Optional fields and aliases
     - Effect
   * - ``replace_text``
     - ``find``/``search``/``old_text`` and ``replace``/``replacement``/
       ``new_text``/``text``.
     - ``match_case`` defaults to ``true``.
     - Replaces text in ``word/document.xml`` and returns replacement counts.
   * - ``set_text_style``
     - One target: ``paragraph_index``/``index``, ``text_contains``/
       ``contains``, or ``table_index`` + ``row_index`` + ``cell_index``; plus
       at least one style field.
     - ``bold``, ``font_family``/``font``/``latin_font_family``,
       ``east_asia_font_family`` aliases, ``language`` aliases,
       ``east_asia_language`` aliases, ``color``/``text_color``/
       ``font_color``, ``font_size_points`` aliases, or
       ``font_size_half_points`` aliases.
     - Applies run styling to paragraphs selected from the body or a table cell.
   * - ``set_paragraph_alignment``
     - Target ``paragraph_index``/``index`` or ``text_contains``/
       ``paragraph_text_contains``; ``alignment``/``horizontal_alignment``.
     - Alignment aliases: ``start`` = ``left``, ``centre`` = ``center``,
       ``end`` = ``right``, ``justify`` = ``both``.
     - Sets body paragraph horizontal alignment.

Tables And Numbering
~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 24 28 30 18

   * - Operation
     - Required fields
     - Optional fields and aliases
     - Effect
   * - ``set_table_col_width``
     - ``table_index``, ``column_index``/``col_index``/``column``, and
       ``width_twips``/``column_width_twips``/``width`` greater than zero.
     - None beyond the listed aliases.
     - Sets the body table grid column and cell width.
   * - ``merge_table_cell``
     - ``table_index``, ``row_index``, ``cell_index``, ``direction``/
       ``merge_direction``.
     - ``count``/``merge_count``.
     - Calls ``merge-table-cells`` from the selected table cell.
   * - ``unmerge_table_cell``
     - ``table_index``, ``row_index``, ``cell_index``, ``direction``/
       ``merge_direction``.
     - None.
     - Calls ``unmerge-table-cells`` from the selected table cell.
   * - ``apply_table_position_plan``
     - ``plan_file``/``plan_path``/``table_position_plan_file``/
       ``table_position_plan_path`` or inline ``table_position_plan``/``plan``.
     - ``dry_run`` must be omitted or ``false``; the script injects
       ``input_path`` before calling ``apply-table-position-plan``.
     - Applies table-position changes and writes the edit-plan output DOCX.
   * - ``import_numbering_catalog``
     - ``catalog_file``/``catalog_path``/``numbering_catalog_file``/
       ``numbering_catalog_path``.
     - None.
     - Calls ``import-numbering-catalog``.

JSON Examples
-------------

.. FDOC_EDIT_PLAN_JSON_EXAMPLES

Replace a content control, append an image, and set paragraph style:

.. code-block:: json

   {
     "operations": [
       {
         "op": "replace_content_control_text_by_tag",
         "tag": "customer_name",
         "text": "Ada Lovelace"
       },
       {
         "op": "append_image",
         "image_path": "assets/logo.png",
         "part": "body",
         "width": 320
       },
       {
         "op": "set_text_style",
         "text_contains": "Ada Lovelace",
         "bold": true,
         "font_family": "Aptos",
         "east_asia_font_family": "Microsoft YaHei",
         "color": "1F4E79",
         "font_size_points": 12
       }
     ]
   }

Update table layout and numbering:

.. code-block:: json

   {
     "operations": [
       {
         "op": "set_table_col_width",
         "table_index": 0,
         "column_index": 1,
         "width_twips": 2880
       },
       {
         "op": "apply_table_position_plan",
         "table_position_plan": {
           "tables": [
             {
               "table_index": 0,
               "alignment": "center"
             }
           ]
         }
       },
       {
         "op": "import_numbering_catalog",
         "catalog_file": "numbering.catalog.json"
       }
     ]
   }

Complete Operation Index
------------------------

Review And Revision Operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ``accept_all_revisions``
* ``reject_all_revisions``
* ``set_comment_resolved``
* ``set_comment_metadata``
* ``append_comment``
* ``append_comment_reply``
* ``replace_comment``
* ``remove_comment``
* ``apply_review_mutation_plan`` (CLI route: ``apply-review-mutation-plan``)
* ``append_insertion_revision``
* ``append_deletion_revision``
* ``insert_run_revision_after``
* ``delete_run_revision``
* ``replace_run_revision``
* ``accept_revision``
* ``reject_revision``
* ``set_revision_metadata``

Notes And Fields
~~~~~~~~~~~~~~~~

* ``append_footnote``
* ``replace_footnote``
* ``remove_footnote``
* ``append_endnote``
* ``replace_endnote``
* ``remove_endnote``
* ``append_page_number_field`` (CLI route: ``append-page-number-field``)
* ``append_total_pages_field``
* ``append_table_of_contents_field``
* ``append_document_property_field``
* ``append_field``
* ``append_date_field``
* ``append_reference_field``
* ``append_page_reference_field``
* ``append_style_reference_field``
* ``append_hyperlink_field``
* ``append_caption``
* ``append_index_entry_field``
* ``append_index_field``
* ``append_sequence_field``
* ``append_complex_field``
* ``replace_field``
* ``set_update_fields_on_open``
* ``enable_update_fields_on_open``
* ``disable_update_fields_on_open``
* ``clear_update_fields_on_open``

Template Slots And Content Controls
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ``replace_content_control_text``
* ``replace_content_control_text_by_tag``
* ``replace_content_control_text_by_alias``
* ``replace_content_control_paragraphs``
* ``replace_content_control_table``
* ``replace_content_control_table_rows``
* ``replace_content_control_image``
* ``set_content_control_form_state``
* ``sync_content_controls_from_custom_xml``
* ``sync_content_control_from_custom_xml``
* ``replace_bookmark_text``
* ``replace_bookmark_paragraphs``
* ``replace_bookmark_table_rows``
* ``replace_bookmark_table``
* ``remove_bookmark_block``
* ``delete_bookmark_block``
* ``replace_bookmark_image``
* ``replace_bookmark_floating_image``

Images, Links, And OMML
~~~~~~~~~~~~~~~~~~~~~~~

* ``append_image`` (CLI route: ``append-image``)
* ``append_floating_image``
* ``replace_image``
* ``remove_image``
* ``append_hyperlink`` (CLI route: ``append-hyperlink``)
* ``replace_hyperlink``
* ``remove_hyperlink``
* ``delete_hyperlink``
* ``append_omml`` (CLI route: ``append-omml``)
* ``replace_omml``
* ``remove_omml``
* ``delete_omml``

Text And Paragraph Mutations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ``insert_paragraph_text_revision``
* ``delete_paragraph_text_revision``
* ``replace_paragraph_text_revision``
* ``insert_text_range_revision``
* ``delete_text_range_revision``
* ``replace_text_range_revision``
* ``append_paragraph_text_comment``
* ``append_text_range_comment``
* ``set_paragraph_text_comment_range``
* ``set_text_range_comment_range``
* ``replace_text``
* ``replace_document_text``
* ``set_text_style``
* ``set_text_format``
* ``set_paragraph_text_style``
* ``delete_paragraph``
* ``remove_paragraph``
* ``set_paragraph_horizontal_alignment``
* ``set_paragraph_alignment``
* ``clear_paragraph_horizontal_alignment``
* ``clear_paragraph_alignment``
* ``set_paragraph_line_spacing``
* ``clear_paragraph_spacing``

Tables And Numbering
~~~~~~~~~~~~~~~~~~~~

* ``apply_table_position_plan`` (CLI route: ``apply-table-position-plan``)
* ``import_numbering_catalog``
* ``set_table_col_width``
* ``clear_table_col_width``
* ``clear_table_width``
* ``clear_table_alignment``
* ``clear_table_indent``
* ``clear_table_layout_mode``
* ``clear_table_style_id``
* ``clear_table_style_look``
* ``clear_table_cell_spacing``
* ``clear_table_default_cell_margin``
* ``clear_table_border``
* ``delete_table_row``
* ``delete_table_column``
* ``delete_table``
* ``insert_table_before``
* ``insert_table_like_before``
* ``merge_table_cell``
* ``unmerge_table_cells``
* ``unmerge_table_cell``

The complete legacy page remains available for compatibility at
:doc:`../../api/edit_plan_operations`.

.. FDOC_API_EDIT_PLAN_EN_MARKER
