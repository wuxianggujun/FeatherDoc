Paragraph And Run
=================

``featherdoc::Paragraph`` is the editing handle for a Word paragraph.
``featherdoc::Run`` is the editing handle for text inside a paragraph with its
own run-level formatting, language, and direction metadata.

Paragraph Text And Iteration
----------------------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``next()``
     - ``Paragraph &``
     - Advance the paragraph iterator to the next paragraph.
   * - ``has_next() const``
     - ``bool``
     - Check whether the iterator can advance.
   * - ``set_text(const std::string &text) const``
     - ``bool``
     - Replace all text in the current paragraph.
   * - ``remove()``
     - ``bool``
     - Remove the current paragraph.
   * - ``runs()``
     - ``Run &``
     - Access the run iterator for the current paragraph.
   * - ``add_run(text, formatting_flag formatting = none)``
     - ``Run``
     - Append a run with optional basic formatting.
   * - ``insert_paragraph_before(text, formatting)``
     - ``Paragraph``
     - Insert a paragraph before the current paragraph.
   * - ``insert_paragraph_after(text, formatting)``
     - ``Paragraph``
     - Insert a paragraph after the current paragraph.
   * - ``insert_paragraph_like_before()``
     - ``Paragraph``
     - Insert a paragraph before this one and copy paragraph properties.
   * - ``insert_paragraph_like_after()``
     - ``Paragraph``
     - Insert a paragraph after this one and copy paragraph properties.

Paragraph Layout
----------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``bidi() const``
     - ``std::optional<bool>``
     - Inspect right-to-left paragraph layout metadata.
   * - ``set_bidi(bool enabled = true) const``
     - ``bool``
     - Enable or disable right-to-left paragraph layout.
   * - ``clear_bidi() const``
     - ``bool``
     - Remove explicit right-to-left paragraph metadata.
   * - ``alignment() const``
     - ``std::optional<paragraph_alignment>``
     - Read paragraph alignment.
   * - ``set_alignment(paragraph_alignment alignment) const``
     - ``bool``
     - Set paragraph alignment.
   * - ``clear_alignment() const``
     - ``bool``
     - Remove explicit paragraph alignment.
   * - ``indent_left_twips() const`` / ``indent_right_twips() const``
     - ``std::optional<std::uint32_t>``
     - Read left or right indent in twips.
   * - ``first_line_indent_twips() const`` / ``hanging_indent_twips() const``
     - ``std::optional<std::uint32_t>``
     - Read first-line or hanging indent.
   * - ``set_indent_left_twips(indent_twips) const``
     - ``bool``
     - Set the left indent in twips.
   * - ``set_indent_right_twips(indent_twips) const``
     - ``bool``
     - Set the right indent in twips.
   * - ``set_first_line_indent_twips(indent_twips) const``
     - ``bool``
     - Set the first-line indent.
   * - ``set_hanging_indent_twips(indent_twips) const``
     - ``bool``
     - Set the hanging indent.

Run Text And Styling
--------------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``get_text() const``
     - ``std::string``
     - Read the current run text.
   * - ``set_text(const std::string &text) const``
     - ``bool``
     - Replace the current run text.
   * - ``remove()``
     - ``bool``
     - Remove the current run.
   * - ``insert_run_before(text, formatting)``
     - ``Run``
     - Insert a run before the current run.
   * - ``insert_run_after(text, formatting)``
     - ``Run``
     - Insert a run after the current run.
   * - ``insert_run_like_before()``
     - ``Run``
     - Insert a run before this one and copy run properties.
   * - ``insert_run_like_after()``
     - ``Run``
     - Insert a run after this one and copy run properties.
   * - ``style_id() const``
     - ``std::optional<std::string>``
     - Read the run style id.
   * - ``font_family() const``
     - ``std::optional<std::string>``
     - Read the primary run font family.
   * - ``set_font_family(font_family) const``
     - ``bool``
     - Set the primary run font family.
   * - ``text_color() const``
     - ``std::optional<std::string>``
     - Read the run text color value.
   * - ``bold() const`` / ``italic() const`` / ``underline() const``
     - ``std::optional<bool>``
     - Inspect basic run styling.
   * - ``strikethrough() const``
     - ``std::optional<bool>``
     - Inspect strikethrough styling.
   * - ``superscript() const`` / ``subscript() const``
     - ``std::optional<bool>``
     - Inspect vertical script styling.
   * - ``font_size_points() const``
     - ``std::optional<double>``
     - Read the run font size in points.

Run Language And Direction
--------------------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``language() const``
     - ``std::optional<std::string>``
     - Read the primary language metadata.
   * - ``east_asia_language() const``
     - ``std::optional<std::string>``
     - Read East Asian language metadata.
   * - ``bidi_language() const``
     - ``std::optional<std::string>``
     - Read bidirectional language metadata.
   * - ``set_language(language) const``
     - ``bool``
     - Set the primary language metadata.
   * - ``set_east_asia_language(language) const``
     - ``bool``
     - Set East Asian language metadata.
   * - ``set_bidi_language(language) const``
     - ``bool``
     - Set bidirectional language metadata.
   * - ``rtl() const``
     - ``std::optional<bool>``
     - Inspect right-to-left run metadata.
   * - ``set_rtl(bool enabled = true) const``
     - ``bool``
     - Enable or disable right-to-left run metadata.

Example
-------

.. code-block:: cpp

   auto paragraph = doc.body_template().append_paragraph("Title");
   paragraph.set_alignment(featherdoc::paragraph_alignment::center);

   auto emphasized = paragraph.add_run(
       " approved",
       featherdoc::formatting_flag::bold);
   emphasized.set_language("en-US");

   auto &run = paragraph.runs();
   if (run.has_next()) {
       run.next().set_font_family("Aptos");
   }
