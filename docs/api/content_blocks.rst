Paragraphs And Runs
===================

``featherdoc::Paragraph`` represents a Word paragraph. ``featherdoc::Run``
represents text inside a paragraph with its own run-level formatting.

Paragraph
---------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``next()``
     - ``Paragraph &``
     - Advance to the next paragraph handle.
   * - ``has_next() const``
     - ``bool``
     - Check whether iteration can continue.
   * - ``set_text(const std::string &text) const``
     - ``bool``
     - Replace paragraph text.
   * - ``remove()``
     - ``bool``
     - Remove the paragraph.
   * - ``runs()``
     - ``Run &``
     - Iterate runs inside the paragraph.
   * - ``add_run(text, formatting_flag formatting = none)``
     - ``Run``
     - Append a run to the paragraph.
   * - ``insert_paragraph_before(text, formatting)``
     - ``Paragraph``
     - Insert a paragraph before the current one.
   * - ``insert_paragraph_after(text, formatting)``
     - ``Paragraph``
     - Insert a paragraph after the current one.
   * - ``alignment() const``
     - ``std::optional<paragraph_alignment>``
     - Read paragraph alignment.
   * - ``set_alignment(paragraph_alignment alignment) const``
     - ``bool``
     - Set paragraph alignment.
   * - ``set_indent_left_twips(std::uint32_t indent_twips) const``
     - ``bool``
     - Set left indent in twentieths of a point.
   * - ``set_bidi(bool enabled = true) const``
     - ``bool``
     - Enable right-to-left paragraph layout.

Run
---

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``get_text() const``
     - ``std::string``
     - Read run text.
   * - ``set_text(const std::string &text) const``
     - ``bool``
     - Replace run text.
   * - ``remove()``
     - ``bool``
     - Remove the run.
   * - ``insert_run_before(text, formatting)``
     - ``Run``
     - Insert a run before the current run.
   * - ``insert_run_after(text, formatting)``
     - ``Run``
     - Insert a run after the current run.
   * - ``font_family() const``
     - ``std::optional<std::string>``
     - Read run font family.
   * - ``set_font_family(std::string_view font_family) const``
     - ``bool``
     - Set run font family.
   * - ``bold() const`` / ``italic() const`` / ``underline() const``
     - ``std::optional<bool>``
     - Inspect basic run styling.
   * - ``font_size_points() const``
     - ``std::optional<double>``
     - Read font size in points.
   * - ``set_language(std::string_view language) const``
     - ``bool``
     - Set primary language metadata.
   * - ``set_rtl(bool enabled = true) const``
     - ``bool``
     - Enable right-to-left run metadata.

Example
-------

.. code-block:: cpp

   auto &paragraph = doc.paragraphs();
   paragraph.set_alignment(featherdoc::paragraph_alignment::center);
   paragraph.add_run("Important", featherdoc::formatting_flag::bold);

