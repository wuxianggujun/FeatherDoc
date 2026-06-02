Table, TableRow, And TableCell
==============================

``featherdoc::Table`` edits Word table structure and table-level metadata.
``featherdoc::TableRow`` edits row metadata and visible cell text.
``featherdoc::TableCell`` edits cell content, size, merge state, borders,
shading, margins, and paragraphs.

Indexing, Units, And Return Semantics
-------------------------------------

Table APIs use zero-based indexes. ``row_index`` addresses visible table rows,
``cell_index`` addresses visible cells in that row, and ``grid_column`` addresses
the resolved Word grid column after spans are considered. Width, indent, margin,
and spacing values use twips. Methods returning ``std::optional<T>`` return an
empty value when the target cannot be resolved; methods returning ``bool`` report
whether the table XML was changed.

Typed Signature Guide
---------------------

.. list-table::
   :header-rows: 1
   :widths: 38 34 28

   * - Signature
     - Parameters
     - Return semantics
   * - ``std::optional<TableRow> find_row(std::size_t row_index)``
     - ``row_index``: zero-based visible row index.
     - Empty when the row does not exist.
   * - ``std::optional<TableCell> find_cell(std::size_t row_index, std::size_t cell_index)``
     - ``row_index``: row index. ``cell_index``: visible cell index in that row.
     - Empty when the row or cell cannot be resolved.
   * - ``std::optional<TableCell> find_cell_by_grid_column(std::size_t row_index, std::size_t grid_column)``
     - ``row_index``: row index. ``grid_column``: resolved Word grid column.
     - Empty when the grid column maps to no visible cell.
   * - ``bool set_cell_text(std::size_t row_index, std::size_t cell_index, const std::string &text)``
     - ``row_index`` and ``cell_index``: visible cell location. ``text``: replacement text.
     - ``true`` when the target cell text was replaced.
   * - ``bool set_rows_texts(std::size_t start_row_index, const std::vector<std::vector<std::string>> &rows)``
     - ``start_row_index``: first target row. ``rows``: row/cell text matrix.
     - ``true`` when every addressed cell in the block was updated.
   * - ``bool set_cell_block_texts(std::size_t start_row_index, std::size_t start_cell_index, const std::vector<std::vector<std::string>> &rows)``
     - ``start_row_index`` and ``start_cell_index``: block origin. ``rows``: rectangular text matrix.
     - ``true`` when the addressed rectangular block was updated.
   * - ``bool set_column_width_twips(std::size_t column_index, std::uint32_t width_twips)``
     - ``column_index``: zero-based grid column. ``width_twips``: column width in twips.
     - ``true`` when the grid column width was updated.
   * - ``bool set_border(table_border_edge edge, border_definition border)``
     - ``edge``: table border edge. ``border``: style, size, color, and spacing.
     - ``true`` when the border definition was written.
   * - ``bool TableRow::set_texts(const std::vector<std::string> &texts)``
     - ``texts``: visible cell texts for the current row.
     - ``true`` when all provided cells were updated.
   * - ``bool TableCell::merge_right(std::size_t additional_cells = 1U)``
     - ``additional_cells``: count of cells to merge to the right.
     - ``true`` when horizontal merge metadata was applied.
   * - ``bool TableCell::merge_down(std::size_t additional_rows = 1U)``
     - ``additional_rows``: count of rows to merge downward.
     - ``true`` when vertical merge metadata was applied.
   * - ``bool TableCell::set_margin_twips(cell_margin_edge edge, std::uint32_t margin_twips)``
     - ``edge``: cell margin edge. ``margin_twips``: margin value in twips.
     - ``true`` when the cell margin was updated.

Table Structure And Lookup
--------------------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``next()``
     - ``Table &``
     - Advance the table iterator to the next table.
   * - ``has_next() const``
     - ``bool``
     - Check whether the table iterator can advance.
   * - ``rows()``
     - ``TableRow &``
     - Access the row iterator for this table.
   * - ``find_row(row_index)``
     - ``std::optional<TableRow>``
     - Look up a row by zero-based row index.
   * - ``find_cell(row_index, cell_index)``
     - ``std::optional<TableCell>``
     - Look up a visible cell by row and visible cell index.
   * - ``find_cell_by_grid_column(row_index, grid_column)``
     - ``std::optional<TableCell>``
     - Look up a cell by resolved grid column.
   * - ``set_cell_text(row_index, cell_index, text)``
     - ``bool``
     - Replace visible cell text.
   * - ``set_cell_text_by_grid_column(row_index, grid_column, text)``
     - ``bool``
     - Replace cell text by resolved grid column.
   * - ``set_row_texts(row_index, texts)``
     - ``bool``
     - Replace visible cell text for one row.
   * - ``set_rows_texts(start_row_index, rows)``
     - ``bool``
     - Replace a rectangular row block.
   * - ``set_cell_block_texts(start_row_index, start_cell_index, rows)``
     - ``bool``
     - Replace a rectangular cell block from a row/cell origin.
   * - ``append_row(std::size_t cell_count = 1U)``
     - ``TableRow``
     - Append a row with the requested number of cells.

Table Layout, Style, And Borders
--------------------------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``width_twips() const``
     - ``std::optional<std::uint32_t>``
     - Read table width.
   * - ``set_width_twips(width_twips)``
     - ``bool``
     - Set table width in twips.
   * - ``column_width_twips(column_index) const``
     - ``std::optional<std::uint32_t>``
     - Read one grid column width.
   * - ``set_column_width_twips(column_index, width_twips)``
     - ``bool``
     - Set one grid column width.
   * - ``layout_mode() const``
     - ``std::optional<table_layout_mode>``
     - Read autofit or fixed layout mode.
   * - ``set_layout_mode(table_layout_mode layout_mode)``
     - ``bool``
     - Set table layout mode.
   * - ``alignment() const``
     - ``std::optional<table_alignment>``
     - Read table alignment.
   * - ``set_alignment(table_alignment alignment)``
     - ``bool``
     - Set table alignment.
   * - ``indent_twips() const``
     - ``std::optional<std::uint32_t>``
     - Read table indent.
   * - ``set_indent_twips(indent_twips)``
     - ``bool``
     - Set table indent.
   * - ``cell_spacing_twips() const``
     - ``std::optional<std::uint32_t>``
     - Read spacing between table cells.
   * - ``set_cell_spacing_twips(spacing_twips)``
     - ``bool``
     - Set spacing between table cells.
   * - ``position() const``
     - ``std::optional<table_position>``
     - Read floating table position metadata.
   * - ``set_position(table_position position)``
     - ``bool``
     - Set floating table position metadata.
   * - ``style_id() const``
     - ``std::optional<std::string>``
     - Read table style id.
   * - ``set_style_id(style_id)``
     - ``bool``
     - Set table style id.
   * - ``style_look() const``
     - ``std::optional<table_style_look>``
     - Read table style look flags.
   * - ``set_style_look(table_style_look style_look)``
     - ``bool``
     - Set table style look flags.
   * - ``border(table_border_edge edge) const``
     - ``std::optional<border_inspection_summary>``
     - Read one table border edge.
   * - ``set_border(table_border_edge edge, border_definition border)``
     - ``bool``
     - Set one table border edge.

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
     - Access the cell iterator for this row.
   * - ``find_cell(cell_index)``
     - ``std::optional<TableCell>``
     - Look up a visible cell by index.
   * - ``find_cell_by_grid_column(grid_column)``
     - ``std::optional<TableCell>``
     - Look up a cell by resolved grid column.
   * - ``height_twips() const``
     - ``std::optional<std::uint32_t>``
     - Read row height.
   * - ``height_rule() const``
     - ``std::optional<row_height_rule>``
     - Read the row height rule.
   * - ``set_height_twips(height_twips, row_height_rule)``
     - ``bool``
     - Set row height and rule.
   * - ``cant_split() const``
     - ``bool``
     - Check whether the row is prevented from splitting across pages.
   * - ``set_cant_split()`` / ``clear_cant_split()``
     - ``bool``
     - Toggle row split prevention.
   * - ``repeats_header() const``
     - ``bool``
     - Check whether this row repeats as a header row.
   * - ``set_repeats_header()`` / ``clear_repeats_header()``
     - ``bool``
     - Toggle repeated header row metadata.
   * - ``set_texts(texts)``
     - ``bool``
     - Replace all visible cell text in this row.
   * - ``insert_row_before()`` / ``insert_row_after()``
     - ``TableRow``
     - Insert a row next to the current row.
   * - ``append_cell()``
     - ``TableCell``
     - Append a cell to this row.

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
     - Access paragraphs inside the cell.
   * - ``get_text() const``
     - ``std::string``
     - Read flattened cell text.
   * - ``set_text(const std::string &text) const``
     - ``bool``
     - Replace cell text.
   * - ``insert_cell_before()`` / ``insert_cell_after()``
     - ``TableCell``
     - Insert a cell next to the current cell.
   * - ``width_twips() const``
     - ``std::optional<std::uint32_t>``
     - Read cell width.
   * - ``set_width_twips(width_twips)``
     - ``bool``
     - Set cell width in twips.
   * - ``column_span() const``
     - ``std::size_t``
     - Read horizontal span.
   * - ``column_index() const``
     - ``std::optional<std::size_t>``
     - Read resolved grid column index.
   * - ``merge_right(additional_cells = 1U)``
     - ``bool``
     - Horizontally merge with cells to the right.
   * - ``merge_down(additional_rows = 1U)``
     - ``bool``
     - Vertically merge with cells below.
   * - ``unmerge_right()`` / ``unmerge_down()``
     - ``bool``
     - Remove horizontal or vertical merge state.
   * - ``vertical_merge() const``
     - ``cell_vertical_merge``
     - Read vertical merge state.
   * - ``row_span() const``
     - ``std::size_t``
     - Read vertical span.
   * - ``vertical_alignment() const``
     - ``std::optional<cell_vertical_alignment>``
     - Read cell vertical alignment.
   * - ``set_vertical_alignment(alignment)``
     - ``bool``
     - Set cell vertical alignment.
   * - ``text_direction() const``
     - ``std::optional<cell_text_direction>``
     - Read text direction.
   * - ``set_text_direction(direction)``
     - ``bool``
     - Set text direction.
   * - ``fill_color() const``
     - ``std::optional<std::string>``
     - Read cell shading color.
   * - ``set_fill_color(fill_color)``
     - ``bool``
     - Set cell shading color.
   * - ``margin_twips(cell_margin_edge edge) const``
     - ``std::optional<std::uint32_t>``
     - Read one cell margin.
   * - ``set_margin_twips(edge, margin_twips)``
     - ``bool``
     - Set one cell margin.
   * - ``border(cell_border_edge edge) const``
     - ``std::optional<border_inspection_summary>``
     - Read one cell border edge.
   * - ``set_border(cell_border_edge edge, border_definition border)``
     - ``bool``
     - Set one cell border edge.

Example
-------

.. code-block:: cpp

   auto table = doc.body_template().append_table(2, 3);
   table.set_layout_mode(featherdoc::table_layout_mode::fixed);
   table.set_width_twips(9000);
   table.set_row_texts(0, {"SKU", "Name", "Quantity"});

   auto header = table.find_row(0);
   if (header) {
       header->set_repeats_header();
   }

   auto sku = table.find_cell(0, 0);
   if (sku) {
       sku->set_fill_color("D9EAF7");
       sku->set_vertical_alignment(featherdoc::cell_vertical_alignment::center);
   }
