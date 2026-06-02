Table、TableRow 和 TableCell
============================

``featherdoc::Table`` 用于编辑 Word 表格结构和表格级元数据。
``featherdoc::TableRow`` 用于编辑行元数据和可见单元格文本。
``featherdoc::TableCell`` 用于编辑单元格内容、宽度、合并状态、边框、
底色、边距和内部段落。

索引、单位和返回语义
--------------------

.. FDOC_ZH_CN_TABLE_INDEX_UNITS_RETURN_SEMANTICS

表格 API 使用从 0 开始的索引。``row_index`` 表示可见表格行，
``cell_index`` 表示该行内的可见单元格，``grid_column`` 表示考虑跨列后的
Word 网格列。宽度、缩进、边距和间距都使用 twips。返回
``std::optional<T>`` 的方法在目标无法解析时返回空值；返回 ``bool`` 的方法
表示表格 XML 是否成功修改。

类型化签名导读
--------------

.. FDOC_ZH_CN_TABLE_TYPED_SIGNATURE_GUIDE

.. list-table::
   :header-rows: 1
   :widths: 38 34 28

   * - 签名
     - 参数
     - 返回语义
   * - ``std::optional<TableRow> find_row(std::size_t row_index)``
     - ``row_index``：从 0 开始的可见行索引。
     - 目标行不存在时返回空值。
   * - ``std::optional<TableCell> find_cell(std::size_t row_index, std::size_t cell_index)``
     - ``row_index``：行索引。``cell_index``：该行内的可见单元格索引。
     - 行或单元格无法解析时返回空值。
   * - ``std::optional<TableCell> find_cell_by_grid_column(std::size_t row_index, std::size_t grid_column)``
     - ``row_index``：行索引。``grid_column``：解析后的 Word 网格列。
     - 网格列无法映射到可见单元格时返回空值。
   * - ``bool set_cell_text(std::size_t row_index, std::size_t cell_index, const std::string &text)``
     - ``row_index`` 和 ``cell_index``：可见单元格位置。``text``：替换文本。
     - 目标单元格文本成功替换时返回 ``true``。
   * - ``bool set_rows_texts(std::size_t start_row_index, const std::vector<std::vector<std::string>> &rows)``
     - ``start_row_index``：首个目标行。``rows``：行/单元格文本矩阵。
     - 所有目标单元格成功更新时返回 ``true``。
   * - ``bool set_cell_block_texts(std::size_t start_row_index, std::size_t start_cell_index, const std::vector<std::vector<std::string>> &rows)``
     - ``start_row_index`` 和 ``start_cell_index``：块起点。``rows``：矩形文本矩阵。
     - 目标矩形块成功更新时返回 ``true``。
   * - ``bool set_column_width_twips(std::size_t column_index, std::uint32_t width_twips)``
     - ``column_index``：从 0 开始的网格列。``width_twips``：列宽，单位 twips。
     - 网格列宽成功更新时返回 ``true``。
   * - ``bool set_border(table_border_edge edge, border_definition border)``
     - ``edge``：表格边框边。``border``：样式、尺寸、颜色和间距。
     - 边框定义成功写入时返回 ``true``。
   * - ``bool TableRow::set_texts(const std::vector<std::string> &texts)``
     - ``texts``：当前行可见单元格文本。
     - 所有提供的单元格成功更新时返回 ``true``。
   * - ``bool TableCell::merge_right(std::size_t additional_cells = 1U)``
     - ``additional_cells``：向右合并的单元格数量。
     - 横向合并元数据成功应用时返回 ``true``。
   * - ``bool TableCell::merge_down(std::size_t additional_rows = 1U)``
     - ``additional_rows``：向下合并的行数。
     - 纵向合并元数据成功应用时返回 ``true``。
   * - ``bool TableCell::set_margin_twips(cell_margin_edge edge, std::uint32_t margin_twips)``
     - ``edge``：单元格边距边。``margin_twips``：边距值，单位 twips。
     - 单元格边距成功更新时返回 ``true``。

表格结构与查找
--------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - 方法
     - 返回值
     - 用途
   * - ``next()``
     - ``Table &``
     - 将表格迭代器推进到下一个表格。
   * - ``has_next() const``
     - ``bool``
     - 检查表格迭代器是否还能继续推进。
   * - ``rows()``
     - ``TableRow &``
     - 访问当前表格的行迭代器。
   * - ``find_row(row_index)``
     - ``std::optional<TableRow>``
     - 按从 0 开始的行索引查找行。
   * - ``find_cell(row_index, cell_index)``
     - ``std::optional<TableCell>``
     - 按行索引和可见单元格索引查找单元格。
   * - ``find_cell_by_grid_column(row_index, grid_column)``
     - ``std::optional<TableCell>``
     - 按解析后的网格列查找单元格。
   * - ``set_cell_text(row_index, cell_index, text)``
     - ``bool``
     - 替换可见单元格文本。
   * - ``set_cell_text_by_grid_column(row_index, grid_column, text)``
     - ``bool``
     - 按解析后的网格列替换单元格文本。
   * - ``set_row_texts(row_index, texts)``
     - ``bool``
     - 替换某一行的可见单元格文本。
   * - ``set_rows_texts(start_row_index, rows)``
     - ``bool``
     - 从指定行开始替换矩形行块。
   * - ``set_cell_block_texts(start_row_index, start_cell_index, rows)``
     - ``bool``
     - 从指定行和单元格开始替换矩形单元格块。
   * - ``append_row(std::size_t cell_count = 1U)``
     - ``TableRow``
     - 追加一行，并指定单元格数量。

表格布局、样式与边框
--------------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - 方法
     - 返回值
     - 用途
   * - ``width_twips() const``
     - ``std::optional<std::uint32_t>``
     - 读取表格宽度。
   * - ``set_width_twips(width_twips)``
     - ``bool``
     - 设置表格宽度，单位是 twips。
   * - ``column_width_twips(column_index) const``
     - ``std::optional<std::uint32_t>``
     - 读取某一网格列宽度。
   * - ``set_column_width_twips(column_index, width_twips)``
     - ``bool``
     - 设置某一网格列宽度。
   * - ``layout_mode() const``
     - ``std::optional<table_layout_mode>``
     - 读取自动适应或固定布局模式。
   * - ``set_layout_mode(table_layout_mode layout_mode)``
     - ``bool``
     - 设置表格布局模式。
   * - ``alignment() const``
     - ``std::optional<table_alignment>``
     - 读取表格对齐方式。
   * - ``set_alignment(table_alignment alignment)``
     - ``bool``
     - 设置表格对齐方式。
   * - ``indent_twips() const``
     - ``std::optional<std::uint32_t>``
     - 读取表格缩进。
   * - ``set_indent_twips(indent_twips)``
     - ``bool``
     - 设置表格缩进。
   * - ``cell_spacing_twips() const``
     - ``std::optional<std::uint32_t>``
     - 读取单元格间距。
   * - ``set_cell_spacing_twips(spacing_twips)``
     - ``bool``
     - 设置单元格间距。
   * - ``position() const``
     - ``std::optional<table_position>``
     - 读取浮动表格位置元数据。
   * - ``set_position(table_position position)``
     - ``bool``
     - 设置浮动表格位置元数据。
   * - ``style_id() const``
     - ``std::optional<std::string>``
     - 读取表格样式 ID。
   * - ``set_style_id(style_id)``
     - ``bool``
     - 设置表格样式 ID。
   * - ``style_look() const``
     - ``std::optional<table_style_look>``
     - 读取表格样式外观标记。
   * - ``set_style_look(table_style_look style_look)``
     - ``bool``
     - 设置表格样式外观标记。
   * - ``border(table_border_edge edge) const``
     - ``std::optional<border_inspection_summary>``
     - 读取某个表格边框。
   * - ``set_border(table_border_edge edge, border_definition border)``
     - ``bool``
     - 设置某个表格边框。

TableRow
--------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - 方法
     - 返回值
     - 用途
   * - ``cells()``
     - ``TableCell &``
     - 访问当前行的单元格迭代器。
   * - ``find_cell(cell_index)``
     - ``std::optional<TableCell>``
     - 按可见单元格索引查找单元格。
   * - ``find_cell_by_grid_column(grid_column)``
     - ``std::optional<TableCell>``
     - 按解析后的网格列查找单元格。
   * - ``height_twips() const``
     - ``std::optional<std::uint32_t>``
     - 读取行高。
   * - ``height_rule() const``
     - ``std::optional<row_height_rule>``
     - 读取行高规则。
   * - ``set_height_twips(height_twips, row_height_rule)``
     - ``bool``
     - 设置行高和行高规则。
   * - ``cant_split() const``
     - ``bool``
     - 检查当前行是否禁止跨页拆分。
   * - ``set_cant_split()`` / ``clear_cant_split()``
     - ``bool``
     - 开启或清除禁止跨页拆分。
   * - ``repeats_header() const``
     - ``bool``
     - 检查当前行是否作为重复表头。
   * - ``set_repeats_header()`` / ``clear_repeats_header()``
     - ``bool``
     - 开启或清除重复表头元数据。
   * - ``set_texts(texts)``
     - ``bool``
     - 替换当前行的所有可见单元格文本。
   * - ``insert_row_before()`` / ``insert_row_after()``
     - ``TableRow``
     - 在当前行前后插入行。
   * - ``append_cell()``
     - ``TableCell``
     - 向当前行追加单元格。

TableCell
---------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - 方法
     - 返回值
     - 用途
   * - ``paragraphs()``
     - ``Paragraph &``
     - 访问单元格内部段落。
   * - ``get_text() const``
     - ``std::string``
     - 读取单元格扁平化文本。
   * - ``set_text(const std::string &text) const``
     - ``bool``
     - 替换单元格文本。
   * - ``insert_cell_before()`` / ``insert_cell_after()``
     - ``TableCell``
     - 在当前单元格前后插入单元格。
   * - ``width_twips() const``
     - ``std::optional<std::uint32_t>``
     - 读取单元格宽度。
   * - ``set_width_twips(width_twips)``
     - ``bool``
     - 设置单元格宽度，单位是 twips。
   * - ``column_span() const``
     - ``std::size_t``
     - 读取横向跨列数量。
   * - ``column_index() const``
     - ``std::optional<std::size_t>``
     - 读取解析后的网格列索引。
   * - ``merge_right(additional_cells = 1U)``
     - ``bool``
     - 向右合并指定数量的单元格。
   * - ``merge_down(additional_rows = 1U)``
     - ``bool``
     - 向下合并指定数量的单元格。
   * - ``unmerge_right()`` / ``unmerge_down()``
     - ``bool``
     - 清除横向或纵向合并状态。
   * - ``vertical_merge() const``
     - ``cell_vertical_merge``
     - 读取纵向合并状态。
   * - ``row_span() const``
     - ``std::size_t``
     - 读取纵向跨行数量。
   * - ``vertical_alignment() const``
     - ``std::optional<cell_vertical_alignment>``
     - 读取单元格垂直对齐方式。
   * - ``set_vertical_alignment(alignment)``
     - ``bool``
     - 设置单元格垂直对齐方式。
   * - ``text_direction() const``
     - ``std::optional<cell_text_direction>``
     - 读取文字方向。
   * - ``set_text_direction(direction)``
     - ``bool``
     - 设置文字方向。
   * - ``fill_color() const``
     - ``std::optional<std::string>``
     - 读取单元格底色。
   * - ``set_fill_color(fill_color)``
     - ``bool``
     - 设置单元格底色。
   * - ``margin_twips(cell_margin_edge edge) const``
     - ``std::optional<std::uint32_t>``
     - 读取某个单元格边距。
   * - ``set_margin_twips(edge, margin_twips)``
     - ``bool``
     - 设置某个单元格边距。
   * - ``border(cell_border_edge edge) const``
     - ``std::optional<border_inspection_summary>``
     - 读取某个单元格边框。
   * - ``set_border(cell_border_edge edge, border_definition border)``
     - ``bool``
     - 设置某个单元格边框。

示例
----

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
