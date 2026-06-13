std::optional<std::uint32_t> Table::width_twips() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto width_node = this->current.child("w:tblPr").child("w:tblW");
    if (width_node == pugi::xml_node{} ||
        std::string_view{width_node.attribute("w:type").value()} != "dxa") {
        return std::nullopt;
    }

    return parse_unsigned_attribute(width_node, "w:w");
}

bool Table::set_width_twips(std::uint32_t width_twips) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto width_node = ensure_table_width_node(this->current);
    if (width_node == pugi::xml_node{}) {
        return false;
    }

    const auto width_text = std::to_string(width_twips);
    ensure_attribute_value(width_node, "w:w", width_text.c_str());
    ensure_attribute_value(width_node, "w:type", "dxa");
    return true;
}

bool Table::clear_width() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    const auto width_node = table_properties.child("w:tblW");
    return width_node == pugi::xml_node{} || table_properties.remove_child(width_node);
}

std::optional<std::uint32_t> Table::column_width_twips(std::size_t column_index) const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto column_count = current_table_column_count(this->current);
    if (column_index >= column_count) {
        return std::nullopt;
    }

    const auto grid_column = find_table_grid_column(this->current, column_index);
    if (grid_column == pugi::xml_node{} || grid_column.attribute("w:w") == pugi::xml_attribute{}) {
        return std::nullopt;
    }

    return parse_unsigned_attribute(grid_column, "w:w");
}

bool Table::set_column_width_twips(std::size_t column_index, std::uint32_t width_twips) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto column_count = current_table_column_count(this->current);
    if (column_index >= column_count) {
        return false;
    }

    ensure_table_grid_columns(this->current, column_count);
    const auto grid_column = find_table_grid_column(this->current, column_index);
    if (grid_column == pugi::xml_node{}) {
        return false;
    }

    const auto width_text = std::to_string(width_twips);
    ensure_attribute_value(grid_column, "w:w", width_text.c_str());
    synchronize_fixed_layout_cell_widths_from_grid(this->current);
    return true;
}

bool Table::clear_column_width(std::size_t column_index) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto column_count = current_table_column_count(this->current);
    if (column_index >= column_count) {
        return false;
    }

    auto grid_column = find_table_grid_column(this->current, column_index);
    if (grid_column == pugi::xml_node{} || grid_column.attribute("w:w") == pugi::xml_attribute{}) {
        return true;
    }

    if (!grid_column.remove_attribute("w:w")) {
        return false;
    }

    clear_fixed_layout_cell_widths_covering_column(this->current, column_index);
    return true;
}

std::optional<featherdoc::table_layout_mode> Table::layout_mode() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto layout_node = this->current.child("w:tblPr").child("w:tblLayout");
    const auto layout_type = std::string_view{layout_node.attribute("w:type").value()};
    if (layout_type.empty()) {
        return std::nullopt;
    }

    return parse_table_layout_mode(layout_type);
}

bool Table::set_layout_mode(featherdoc::table_layout_mode layout_mode) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto layout_node = ensure_table_layout_node(this->current);
    if (layout_node == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(layout_node, "w:type", to_xml_table_layout_mode(layout_mode));
    if (layout_mode == featherdoc::table_layout_mode::fixed) {
        synchronize_fixed_layout_cell_widths_from_grid(this->current);
    }
    return true;
}

bool Table::clear_layout_mode() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    const auto layout_node = table_properties.child("w:tblLayout");
    return layout_node == pugi::xml_node{} || table_properties.remove_child(layout_node);
}

std::optional<featherdoc::table_alignment> Table::alignment() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto alignment_node = this->current.child("w:tblPr").child("w:jc");
    const auto alignment_text = std::string_view{alignment_node.attribute("w:val").value()};
    if (alignment_text.empty()) {
        return std::nullopt;
    }

    return parse_table_alignment(alignment_text);
}

bool Table::set_alignment(featherdoc::table_alignment alignment) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto alignment_node = ensure_table_alignment_node(this->current);
    if (alignment_node == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(alignment_node, "w:val", to_xml_table_alignment(alignment));
    return true;
}

bool Table::clear_alignment() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    const auto alignment_node = table_properties.child("w:jc");
    return alignment_node == pugi::xml_node{} || table_properties.remove_child(alignment_node);
}

std::optional<std::uint32_t> Table::indent_twips() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto indent_node = this->current.child("w:tblPr").child("w:tblInd");
    if (indent_node == pugi::xml_node{}) {
        return std::nullopt;
    }

    if (const auto indent_type = std::string_view{indent_node.attribute("w:type").value()};
        !indent_type.empty() && indent_type != "dxa") {
        return std::nullopt;
    }

    return parse_unsigned_attribute(indent_node, "w:w");
}

bool Table::set_indent_twips(std::uint32_t indent_twips) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto indent_node = ensure_table_indent_node(this->current);
    if (indent_node == pugi::xml_node{}) {
        return false;
    }

    const auto indent_text = std::to_string(indent_twips);
    ensure_attribute_value(indent_node, "w:w", indent_text.c_str());
    ensure_attribute_value(indent_node, "w:type", "dxa");
    return true;
}

bool Table::clear_indent() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    const auto indent_node = table_properties.child("w:tblInd");
    return indent_node == pugi::xml_node{} || table_properties.remove_child(indent_node);
}

std::optional<std::uint32_t> Table::cell_spacing_twips() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto spacing = this->current.child("w:tblPr").child("w:tblCellSpacing");
    if (spacing == pugi::xml_node{}) {
        return std::nullopt;
    }

    if (const auto spacing_type = std::string_view{spacing.attribute("w:type").value()};
        !spacing_type.empty() && spacing_type != "dxa") {
        return std::nullopt;
    }

    return parse_unsigned_attribute(spacing, "w:w");
}

bool Table::set_cell_spacing_twips(std::uint32_t spacing_twips) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto spacing = ensure_table_cell_spacing_node(this->current);
    if (spacing == pugi::xml_node{}) {
        return false;
    }

    const auto spacing_text = std::to_string(spacing_twips);
    ensure_attribute_value(spacing, "w:w", spacing_text.c_str());
    ensure_attribute_value(spacing, "w:type", "dxa");
    return true;
}

bool Table::clear_cell_spacing() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    const auto spacing = table_properties.child("w:tblCellSpacing");
    return spacing == pugi::xml_node{} || table_properties.remove_child(spacing);
}

void set_optional_unsigned_attribute(pugi::xml_node node, const char *name,
                                     std::optional<std::uint32_t> value) {
    if (node == pugi::xml_node{}) {
        return;
    }

    if (value.has_value()) {
        const auto text = std::to_string(*value);
        ensure_attribute_value(node, name, text.c_str());
        return;
    }

    if (node.attribute(name) != pugi::xml_attribute{}) {
        node.remove_attribute(name);
    }
}

std::optional<featherdoc::table_position> Table::position() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto position_node = this->current.child("w:tblPr").child("w:tblpPr");
    if (position_node == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto position = featherdoc::table_position{};
    if (const auto horizontal_reference = parse_table_position_horizontal_reference(
            std::string_view{position_node.attribute("w:horzAnchor").value()})) {
        position.horizontal_reference = *horizontal_reference;
    }
    if (const auto horizontal_offset = parse_signed_attribute(position_node, "w:tblpX")) {
        position.horizontal_offset_twips = *horizontal_offset;
    }
    if (const auto horizontal_spec = parse_table_position_horizontal_spec(
            std::string_view{position_node.attribute("w:tblpXSpec").value()})) {
        position.horizontal_spec = *horizontal_spec;
    }
    if (const auto vertical_reference = parse_table_position_vertical_reference(
            std::string_view{position_node.attribute("w:vertAnchor").value()})) {
        position.vertical_reference = *vertical_reference;
    }
    if (const auto vertical_offset = parse_signed_attribute(position_node, "w:tblpY")) {
        position.vertical_offset_twips = *vertical_offset;
    }
    if (const auto vertical_spec = parse_table_position_vertical_spec(
            std::string_view{position_node.attribute("w:tblpYSpec").value()})) {
        position.vertical_spec = *vertical_spec;
    }
    position.left_from_text_twips =
        parse_unsigned_attribute(position_node, "w:leftFromText");
    position.right_from_text_twips =
        parse_unsigned_attribute(position_node, "w:rightFromText");
    position.top_from_text_twips =
        parse_unsigned_attribute(position_node, "w:topFromText");
    position.bottom_from_text_twips =
        parse_unsigned_attribute(position_node, "w:bottomFromText");
    if (const auto overlap = parse_table_overlap(
            std::string_view{position_node.attribute("w:tblOverlap").value()})) {
        position.overlap = *overlap;
    }

    return position;
}

bool Table::set_position(featherdoc::table_position position) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto position_node = ensure_table_position_node(this->current);
    if (position_node == pugi::xml_node{}) {
        return false;
    }

    const auto horizontal_offset = std::to_string(position.horizontal_offset_twips);
    const auto vertical_offset = std::to_string(position.vertical_offset_twips);
    ensure_attribute_value(position_node, "w:horzAnchor",
                           to_xml_table_position_horizontal_reference(
                               position.horizontal_reference));
    ensure_attribute_value(position_node, "w:tblpX", horizontal_offset.c_str());
    if (position.horizontal_spec.has_value()) {
        ensure_attribute_value(
            position_node, "w:tblpXSpec",
            to_xml_table_position_horizontal_spec(*position.horizontal_spec));
    } else if (position_node.attribute("w:tblpXSpec") != pugi::xml_attribute{}) {
        position_node.remove_attribute("w:tblpXSpec");
    }
    ensure_attribute_value(position_node, "w:vertAnchor",
                           to_xml_table_position_vertical_reference(
                               position.vertical_reference));
    ensure_attribute_value(position_node, "w:tblpY", vertical_offset.c_str());
    if (position.vertical_spec.has_value()) {
        ensure_attribute_value(
            position_node, "w:tblpYSpec",
            to_xml_table_position_vertical_spec(*position.vertical_spec));
    } else if (position_node.attribute("w:tblpYSpec") != pugi::xml_attribute{}) {
        position_node.remove_attribute("w:tblpYSpec");
    }
    set_optional_unsigned_attribute(position_node, "w:leftFromText",
                                    position.left_from_text_twips);
    set_optional_unsigned_attribute(position_node, "w:rightFromText",
                                    position.right_from_text_twips);
    set_optional_unsigned_attribute(position_node, "w:topFromText",
                                    position.top_from_text_twips);
    set_optional_unsigned_attribute(position_node, "w:bottomFromText",
                                    position.bottom_from_text_twips);
    if (position.overlap.has_value()) {
        ensure_attribute_value(position_node, "w:tblOverlap",
                               to_xml_table_overlap(*position.overlap));
    } else if (position_node.attribute("w:tblOverlap") != pugi::xml_attribute{}) {
        position_node.remove_attribute("w:tblOverlap");
    }
    return true;
}

bool Table::clear_position() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    const auto position_node = table_properties.child("w:tblpPr");
    return position_node == pugi::xml_node{} ||
           table_properties.remove_child(position_node);
}

std::optional<std::uint32_t> Table::cell_margin_twips(
    featherdoc::cell_margin_edge edge) const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto margin =
        this->current.child("w:tblPr").child("w:tblCellMar").child(to_xml_margin_name(edge));
    if (margin == pugi::xml_node{}) {
        return std::nullopt;
    }

    if (const auto margin_type = std::string_view{margin.attribute("w:type").value()};
        !margin_type.empty() && margin_type != "dxa") {
        return std::nullopt;
    }

    return parse_unsigned_attribute(margin, "w:w");
}

bool Table::set_cell_margin_twips(featherdoc::cell_margin_edge edge,
                                  std::uint32_t margin_twips) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto margin = ensure_table_cell_margin_node(this->current, to_xml_margin_name(edge));
    if (margin == pugi::xml_node{}) {
        return false;
    }

    const auto margin_text = std::to_string(margin_twips);
    ensure_attribute_value(margin, "w:w", margin_text.c_str());
    ensure_attribute_value(margin, "w:type", "dxa");
    return true;
}

bool Table::clear_cell_margin(featherdoc::cell_margin_edge edge) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    auto margins = table_properties.child("w:tblCellMar");
    if (margins == pugi::xml_node{}) {
        return true;
    }

    if (const auto margin = margins.child(to_xml_margin_name(edge)); margin != pugi::xml_node{}) {
        margins.remove_child(margin);
    }

    remove_empty_container(table_properties, "w:tblCellMar");
    return true;
}

std::optional<std::string> Table::style_id() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto style_node = this->current.child("w:tblPr").child("w:tblStyle");
    const auto style_text = std::string_view{style_node.attribute("w:val").value()};
    if (style_text.empty()) {
        return std::nullopt;
    }

    return std::string{style_text};
}

bool Table::set_style_id(std::string_view style_id) {
    if (this->current == pugi::xml_node{} || style_id.empty()) {
        return false;
    }

    if (this->owner != nullptr && this->owner->ensure_styles_part_attached()) {
        return false;
    }

    const auto style_node = ensure_table_style_node(this->current);
    if (style_node == pugi::xml_node{}) {
        return false;
    }

    const auto style_text = std::string{style_id};
    ensure_attribute_value(style_node, "w:val", style_text.c_str());
    return true;
}

bool Table::clear_style_id() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    const auto style_node = table_properties.child("w:tblStyle");
    return style_node == pugi::xml_node{} || table_properties.remove_child(style_node);
}

std::optional<featherdoc::table_style_look> Table::style_look() const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto table_look_node = this->current.child("w:tblPr").child("w:tblLook");
    if (table_look_node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto encoded_value =
        parse_short_hex_value(std::string_view{table_look_node.attribute("w:val").value()});
    auto style_look = featherdoc::table_style_look{};
    style_look.first_row = decode_table_style_look_flag(
        parse_xml_on_off_value(std::string_view{table_look_node.attribute("w:firstRow").value()}),
        encoded_value, table_style_look_first_row_bit, style_look.first_row);
    style_look.last_row = decode_table_style_look_flag(
        parse_xml_on_off_value(std::string_view{table_look_node.attribute("w:lastRow").value()}),
        encoded_value, table_style_look_last_row_bit, style_look.last_row);
    style_look.first_column = decode_table_style_look_flag(
        parse_xml_on_off_value(
            std::string_view{table_look_node.attribute("w:firstColumn").value()}),
        encoded_value, table_style_look_first_column_bit, style_look.first_column);
    style_look.last_column = decode_table_style_look_flag(
        parse_xml_on_off_value(
            std::string_view{table_look_node.attribute("w:lastColumn").value()}),
        encoded_value, table_style_look_last_column_bit, style_look.last_column);
    style_look.banded_rows = decode_table_style_look_flag(
        parse_xml_on_off_value(std::string_view{table_look_node.attribute("w:noHBand").value()}),
        encoded_value, table_style_look_no_hband_bit, style_look.banded_rows, true);
    style_look.banded_columns = decode_table_style_look_flag(
        parse_xml_on_off_value(std::string_view{table_look_node.attribute("w:noVBand").value()}),
        encoded_value, table_style_look_no_vband_bit, style_look.banded_columns, true);
    return style_look;
}

bool Table::set_style_look(featherdoc::table_style_look style_look) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto table_look_node = ensure_table_look_node(this->current);
    if (table_look_node == pugi::xml_node{}) {
        return false;
    }

    const auto encoded_value = format_short_hex(encode_table_style_look(style_look));
    ensure_attribute_value(table_look_node, "w:val", encoded_value.c_str());
    ensure_attribute_value(table_look_node, "w:firstRow", style_look.first_row ? "1" : "0");
    ensure_attribute_value(table_look_node, "w:lastRow", style_look.last_row ? "1" : "0");
    ensure_attribute_value(table_look_node, "w:firstColumn",
                           style_look.first_column ? "1" : "0");
    ensure_attribute_value(table_look_node, "w:lastColumn",
                           style_look.last_column ? "1" : "0");
    ensure_attribute_value(table_look_node, "w:noHBand", style_look.banded_rows ? "0" : "1");
    ensure_attribute_value(table_look_node, "w:noVBand",
                           style_look.banded_columns ? "0" : "1");
    return true;
}

bool Table::clear_style_look() {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    const auto table_look_node = table_properties.child("w:tblLook");
    return table_look_node == pugi::xml_node{} || table_properties.remove_child(table_look_node);
}

std::optional<featherdoc::border_inspection_summary>
Table::border(featherdoc::table_border_edge edge) const {
    if (this->current == pugi::xml_node{}) {
        return std::nullopt;
    }

    return read_border_inspection_summary(
        this->current.child("w:tblPr")
            .child("w:tblBorders")
            .child(to_xml_border_name(edge)));
}

bool Table::set_border(featherdoc::table_border_edge edge,
                       featherdoc::border_definition border) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_borders = ensure_table_borders_node(this->current);
    if (table_borders == pugi::xml_node{}) {
        return false;
    }

    const auto border_name = to_xml_border_name(edge);
    auto border_node = table_borders.child(border_name);
    if (border_node == pugi::xml_node{}) {
        border_node = table_borders.append_child(border_name);
    }
    if (border_node == pugi::xml_node{}) {
        return false;
    }

    apply_border_definition(border_node, border);
    return true;
}

bool Table::clear_border(featherdoc::table_border_edge edge) {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto table_properties = this->current.child("w:tblPr");
    if (table_properties == pugi::xml_node{}) {
        return true;
    }

    auto table_borders = table_properties.child("w:tblBorders");
    if (table_borders == pugi::xml_node{}) {
        return true;
    }

    if (const auto border_node = table_borders.child(to_xml_border_name(edge));
        border_node != pugi::xml_node{}) {
        table_borders.remove_child(border_node);
    }

    remove_empty_container(table_properties, "w:tblBorders");
    return true;
}
