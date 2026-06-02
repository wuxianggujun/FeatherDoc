Edit Plan Operations
====================

``scripts/edit_document_from_plan.ps1`` accepts JSON edit plans for scripted
document rewrites. This page is the contract index for operation names that are
implemented by the dispatcher and covered by regression tests.

This legacy root-level page is kept for compatibility. Use the structured
language-local API references for fields, aliases, result shape, and JSON
examples:

* :doc:`../en/api/edit_plan_operations`
* :doc:`../zh-CN/api/edit_plan_operations`

Review And Revision Operations
------------------------------

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
----------------

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
-----------------------------------

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
-----------------------

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
----------------------------

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
--------------------

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
