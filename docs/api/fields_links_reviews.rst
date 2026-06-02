Fields, Links, And Review Notes
===============================

These APIs are available on ``Document`` and, where part-specific editing is
supported, on ``TemplatePart``.

Fields
------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``list_fields() const``
     - ``std::vector<field_summary>``
     - Enumerate fields in a template part.
   * - ``append_field(instruction, result_text, state)``
     - ``bool``
     - Append a simple field.
   * - ``append_complex_field(instruction, result_text, options)``
     - ``bool``
     - Append a complex Word field.
   * - ``append_page_number_field(state = {})``
     - ``bool``
     - Append PAGE.
   * - ``append_total_pages_field(state = {})``
     - ``bool``
     - Append NUMPAGES.
   * - ``append_table_of_contents_field(options, result_text)``
     - ``bool``
     - Append TOC.
   * - ``append_reference_field(bookmark_name, options, result_text)``
     - ``bool``
     - Append REF.
   * - ``replace_field(field_index, instruction, result_text)``
     - ``bool``
     - Replace a field by index.

Hyperlinks
----------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``list_hyperlinks() const``
     - ``std::vector<hyperlink_summary>``
     - Enumerate hyperlinks.
   * - ``append_hyperlink(text, target)``
     - ``std::size_t``
     - Append a hyperlink.
   * - ``replace_hyperlink(hyperlink_index, text, target)``
     - ``bool``
     - Replace hyperlink text and target.
   * - ``remove_hyperlink(hyperlink_index)``
     - ``bool``
     - Remove a hyperlink.

Footnotes And Endnotes
----------------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``list_footnotes() const`` / ``list_endnotes() const``
     - ``std::vector<review_note_summary>``
     - Enumerate notes.
   * - ``append_footnote(reference_text, note_text)``
     - ``std::size_t``
     - Append a footnote.
   * - ``append_endnote(reference_text, note_text)``
     - ``std::size_t``
     - Append an endnote.
   * - ``replace_footnote(note_index, note_text)``
     - ``bool``
     - Replace a footnote.
   * - ``remove_endnote(note_index)``
     - ``bool``
     - Remove an endnote.

Comments And Revisions
----------------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``list_comments() const``
     - ``std::vector<review_note_summary>``
     - Enumerate comments.
   * - ``append_comment(selected_text, comment_text, author, initials, date)``
     - ``std::size_t``
     - Append a comment.
   * - ``append_text_range_comment(start_paragraph_index, start_text_offset, end_paragraph_index, end_text_offset, comment_text, author, initials, date)``
     - ``std::size_t``
     - Comment a text range.
   * - ``set_comment_resolved(comment_index, resolved)``
     - ``bool``
     - Mark a comment resolved/unresolved.
   * - ``list_revisions() const``
     - ``std::vector<revision_summary>``
     - Enumerate revisions.
   * - ``append_insertion_revision(text, author, date)``
     - ``std::size_t``
     - Append an insertion revision.
   * - ``append_deletion_revision(text, author, date)``
     - ``std::size_t``
     - Append a deletion revision.
   * - ``accept_revision(revision_index)`` / ``reject_revision(revision_index)``
     - ``bool``
     - Resolve one revision.
   * - ``accept_all_revisions()`` / ``reject_all_revisions()``
     - ``std::size_t``
     - Resolve all revisions.

