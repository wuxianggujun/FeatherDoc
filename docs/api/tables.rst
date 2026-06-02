Tables
======

Use ``Table``, ``TableRow``, and ``TableCell`` to edit table structure and cell
content while preserving Word table metadata.

Table
-----

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``rows()``
     - ``TableRow &``
     - Iterate rows.
   * - ``find_row(std::size_t row_index)``
     - ``std::optional<TableRow>``
     - Look up a row by index.
   * - ``find_cell(row_index, cell_index)``
     - ``std::optional<TableCell>``
     - Look up a cell by row and visible cell index.
   * - ``find_cell_by_grid_column(row_index, grid_column)``
     - ``std::optional<TableCell>``
     - Look up a cell by resolved grid column.
   * - ``set_cell_text(row_index, cell_index, text)``
     - ``bool``
     - Replace a cell's text.
   * - ``set_row_texts(row_index, texts)``
     - ``bool``
     - Replace visible cell text for one row.
   * - ``set_rows_texts(start_row_index, rows)``
     - ``bool``
     - Replace a rectangular row block.
   * - ``width_twips() const``
     - ``std::optional<std::uint32_t>``
     - Read table width.
   * - ``set_width_twips(std::uint32_t width_twips)``
     - ``bool``
     - Set table width.
   * - ``layout_mode() const``
     - ``std::optional<table_layout_mode>``
     - Read autofit/fixed layout mode.
   * - ``set_layout_mode(table_layout_mode layout_mode)``
     - ``bool``
     - Set table layout mode.
   * - ``set_border(table_border_edge edge, border_definition border)``
     - ``bool``
     - Set a table border edge.
   * - ``append_row(std::size_t cell_count = 1U)``
     - ``TableRow``
     - Append a row.

TableRow
--------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``cells()``
     - ``TableCell &``
     - Iterate cells.
   * - ``set_texts(const std::vector<std::string> &texts)``
     - ``bool``
     - Replace all visible cell texts in the row.
   * - ``height_twips() const``
     - ``std::optional<std::uint32_t>``
     - Read row height.
   * - ``set_height_twips(height_twips, row_height_rule)``
     - ``bool``
     - Set row height and rule.
   * - ``set_repeats_header()`` / ``clear_repeats_header()``
     - ``bool``
     - Toggle repeated header row metadata.
   * - ``append_cell()``
     - ``TableCell``
     - Append a cell to the row.

TableCell
---------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``paragraphs()``
     - ``Paragraph &``
     - Iterate paragraphs inside the cell.
   * - ``get_text() const``
     - ``std::string``
     - Read flattened cell text.
   * - ``set_text(const std::string &text) const``
     - ``bool``
     - Replace cell text.
   * - ``width_twips() const``
     - ``std::optional<std::uint32_t>``
     - Read cell width.
   * - ``set_width_twips(std::uint32_t width_twips)``
     - ``bool``
     - Set cell width.
   * - ``merge_right(std::size_t additional_cells = 1U)``
     - ``bool``
     - Horizontally merge with cells to the right.
   * - ``merge_down(std::size_t additional_rows = 1U)``
     - ``bool``
     - Vertically merge with cells below.
   * - ``unmerge_right()`` / ``unmerge_down()``
     - ``bool``
     - Remove merge state.
   * - ``set_fill_color(std::string_view fill_color)``
     - ``bool``
     - Set cell shading color.
   * - ``set_margin_twips(cell_margin_edge edge, std::uint32_t margin_twips)``
     - ``bool``
     - Set cell margin.

Example
-------

.. code-block:: cpp

   auto table = doc.append_table(2, 3);
   table.set_layout_mode(featherdoc::table_layout_mode::fixed);
   table.set_cell_text(0, 0, "SKU");
   table.find_cell(0, 0)->set_fill_color("D9EAF7");

