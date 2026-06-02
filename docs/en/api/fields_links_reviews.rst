Fields, Links, And Review Notes
===============================

Field, hyperlink, note, comment, and revision APIs are available on
``featherdoc::Document``. Field and hyperlink operations that can be scoped to a
body, header, footer, or section part are also available on
``featherdoc::TemplatePart``.

Summary Types
-------------

.. list-table::
   :header-rows: 1
   :widths: 34 26 40

   * - Type
     - Key fields
     - Purpose
   * - ``featherdoc::field_summary``
     - ``kind``, ``instruction``, ``result_text``, ``dirty``, ``locked``
     - Describes a simple or complex Word field.
   * - ``featherdoc::hyperlink_summary``
     - ``text``, ``target``, ``anchor``, ``external``
     - Describes an internal or external hyperlink.
   * - ``featherdoc::review_note_summary``
     - ``kind``, ``author``, ``date``, ``resolved``, ``text``
     - Describes footnotes, endnotes, comments, and comment replies.
   * - ``featherdoc::revision_summary``
     - ``kind``, ``author``, ``part_entry_name``, ``text``
     - Describes tracked insertions, deletions, and related revision records.

Fields
------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - Method
     - Returns
     - Purpose
   * - ``list_fields() const``
     - ``std::vector<field_summary>``
     - Enumerate fields in the selected document part.
   * - ``append_field(instruction, result_text, state)``
     - ``bool``
     - Append a simple field instruction and optional display result.
   * - ``append_complex_field(instruction, result_text, options)``
     - ``bool``
     - Append a complex Word field.
   * - ``append_page_number_field(state = {})``
     - ``bool``
     - Append a PAGE field.
   * - ``append_total_pages_field(state = {})``
     - ``bool``
     - Append a NUMPAGES field.
   * - ``append_table_of_contents_field(options, result_text)``
     - ``bool``
     - Append a TOC field.
   * - ``append_reference_field(bookmark_name, options, result_text)``
     - ``bool``
     - Append a REF field that points at a bookmark.
   * - ``replace_field(field_index, instruction, result_text)``
     - ``bool``
     - Replace a field instruction and result by index.

Hyperlinks
----------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - Method
     - Returns
     - Purpose
   * - ``list_hyperlinks() const``
     - ``std::vector<hyperlink_summary>``
     - Enumerate hyperlinks.
   * - ``append_hyperlink(text, target)``
     - ``std::size_t``
     - Append a hyperlink and return the created hyperlink index.
   * - ``replace_hyperlink(hyperlink_index, text, target)``
     - ``bool``
     - Replace hyperlink text and target.
   * - ``remove_hyperlink(hyperlink_index)``
     - ``bool``
     - Remove one hyperlink.

Notes, Comments, And Revisions
------------------------------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - Method
     - Returns
     - Purpose
   * - ``list_footnotes() const`` / ``list_endnotes() const``
     - ``std::vector<review_note_summary>``
     - Enumerate footnotes or endnotes.
   * - ``append_footnote(reference_text, note_text)``
     - ``std::size_t``
     - Append a footnote.
   * - ``append_endnote(reference_text, note_text)``
     - ``std::size_t``
     - Append an endnote.
   * - ``replace_footnote(note_index, note_text)``
     - ``bool``
     - Replace one footnote.
   * - ``remove_endnote(note_index)``
     - ``bool``
     - Remove one endnote.
   * - ``list_comments() const``
     - ``std::vector<review_note_summary>``
     - Enumerate comments.
   * - ``append_comment(selected_text, comment_text, author, initials, date)``
     - ``std::size_t``
     - Append a comment anchored to selected text.
   * - ``append_text_range_comment(start_paragraph_index, start_text_offset, end_paragraph_index, end_text_offset, comment_text, author, initials, date)``
     - ``std::size_t``
     - Comment a text range.
   * - ``set_comment_resolved(comment_index, resolved)``
     - ``bool``
     - Mark a comment resolved or unresolved.
   * - ``list_revisions() const``
     - ``std::vector<revision_summary>``
     - Enumerate tracked revisions.
   * - ``accept_revision(revision_index)`` / ``reject_revision(revision_index)``
     - ``bool``
     - Resolve one revision.
   * - ``accept_all_revisions()`` / ``reject_all_revisions()``
     - ``std::size_t``
     - Resolve all tracked revisions.

Example
-------

.. code-block:: cpp

   featherdoc::Document doc{"review.docx"};
   doc.open();

   auto body = doc.body_template();
   body.append_page_number_field();
   body.append_hyperlink("FeatherDoc", "https://github.com/wuxianggujun/FeatherDoc");

   for (const auto &comment : doc.list_comments()) {
       if (!comment.resolved) {
           doc.set_comment_resolved(comment.index, true);
       }
   }

The complete legacy page remains available at
:doc:`../../api/fields_links_reviews`.
