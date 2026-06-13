auto read_optional_string_attribute(pugi::xml_node node, const char *attribute_name)
    -> std::optional<std::string> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto attribute = node.attribute(attribute_name);
    if (attribute == pugi::xml_attribute{} || attribute.value()[0] == '\0') {
        return std::nullopt;
    }

    return std::string{attribute.value()};
}

auto read_on_off_value(pugi::xml_node node) -> std::optional<bool> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto attribute = node.attribute("w:val");
    if (attribute == pugi::xml_attribute{} || attribute.value()[0] == '\0') {
        return true;
    }

    const auto value = std::string_view{attribute.value()};
    return value != "0" && value != "false" && value != "off";
}

auto read_part_paragraph_style_id(pugi::xml_node paragraph_properties)
    -> std::optional<std::string> {
    return read_optional_string_attribute(paragraph_properties.child("w:pStyle"), "w:val");
}

auto read_part_paragraph_bidi(pugi::xml_node paragraph_properties) -> std::optional<bool> {
    return read_on_off_value(paragraph_properties.child("w:bidi"));
}

auto parse_u32_attribute_value(const char *text) -> std::optional<std::uint32_t>;

auto read_part_paragraph_alignment(pugi::xml_node paragraph_properties)
    -> std::optional<featherdoc::paragraph_alignment> {
    const auto value = std::string_view{
        paragraph_properties.child("w:jc").attribute("w:val").value()};
    if (value == "left" || value == "start") {
        return featherdoc::paragraph_alignment::left;
    }
    if (value == "center") {
        return featherdoc::paragraph_alignment::center;
    }
    if (value == "right" || value == "end") {
        return featherdoc::paragraph_alignment::right;
    }
    if (value == "both" || value == "mediumKashida" ||
        value == "highKashida" || value == "lowKashida") {
        return featherdoc::paragraph_alignment::justified;
    }
    if (value == "distribute") {
        return featherdoc::paragraph_alignment::distribute;
    }

    return std::nullopt;
}

auto read_part_paragraph_indent_twips(pugi::xml_node paragraph_properties,
                                      const char *attribute_name)
    -> std::optional<std::uint32_t> {
    return parse_u32_attribute_value(
        paragraph_properties.child("w:ind").attribute(attribute_name).value());
}

auto parse_u32_attribute_value(const char *text) -> std::optional<std::uint32_t> {
    if (text == nullptr || *text == '\0') {
        return std::nullopt;
    }

    char *end = nullptr;
    const auto value = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' ||
        value > static_cast<unsigned long>(std::numeric_limits<std::uint32_t>::max())) {
        return std::nullopt;
    }

    return static_cast<std::uint32_t>(value);
}

auto summarize_part_paragraph_numbering(pugi::xml_node paragraph_properties)
    -> std::optional<featherdoc::paragraph_inspection_summary::numbering_summary> {
    const auto numbering_properties = paragraph_properties.child("w:numPr");
    if (numbering_properties == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto summary = featherdoc::paragraph_inspection_summary::numbering_summary{};
    summary.level = parse_u32_attribute_value(
        numbering_properties.child("w:ilvl").attribute("w:val").value());
    summary.num_id = parse_u32_attribute_value(
        numbering_properties.child("w:numId").attribute("w:val").value());
    return summary;
}

auto enrich_part_paragraph_numbering(featherdoc::Document *owner,
                                     featherdoc::paragraph_inspection_summary &summary) -> void {
    if (owner == nullptr || !summary.numbering.has_value() ||
        !summary.numbering->num_id.has_value()) {
        return;
    }

    const auto lookup = owner->find_numbering_instance(*summary.numbering->num_id);
    if (!lookup.has_value()) {
        return;
    }

    summary.numbering->definition_id = lookup->definition_id;
    if (!lookup->definition_name.empty()) {
        summary.numbering->definition_name = lookup->definition_name;
    }
}

auto summarize_part_paragraph_node(pugi::xml_node paragraph_node, std::size_t paragraph_index)
    -> featherdoc::paragraph_inspection_summary {
    auto summary = featherdoc::paragraph_inspection_summary{};
    summary.index = paragraph_index;

    const auto paragraph_properties = paragraph_node.child("w:pPr");
    summary.style_id = read_part_paragraph_style_id(paragraph_properties);
    summary.bidi = read_part_paragraph_bidi(paragraph_properties);
    summary.alignment = read_part_paragraph_alignment(paragraph_properties);
    summary.indent_left_twips =
        read_part_paragraph_indent_twips(paragraph_properties, "w:left");
    summary.indent_right_twips =
        read_part_paragraph_indent_twips(paragraph_properties, "w:right");
    summary.first_line_indent_twips =
        read_part_paragraph_indent_twips(paragraph_properties, "w:firstLine");
    summary.hanging_indent_twips =
        read_part_paragraph_indent_twips(paragraph_properties, "w:hanging");
    summary.numbering = summarize_part_paragraph_numbering(paragraph_properties);

    auto paragraph_handle = featherdoc::Paragraph{paragraph_node.parent(), paragraph_node};
    for (auto run = paragraph_handle.runs(); run.has_next(); run.next()) {
        ++summary.run_count;
        summary.text += run.get_text();
    }

    return summary;
}

auto summarize_part_run_handle(const featherdoc::Run &run_handle, std::size_t run_index)
    -> featherdoc::run_inspection_summary {
    auto summary = featherdoc::run_inspection_summary{};
    summary.index = run_index;
    summary.text = run_handle.get_text();
    summary.style_id = run_handle.style_id();
    summary.font_family = run_handle.font_family();
    summary.east_asia_font_family = run_handle.east_asia_font_family();
    summary.text_color = run_handle.text_color();
    summary.bold = run_handle.bold();
    summary.italic = run_handle.italic();
    summary.strikethrough = run_handle.strikethrough();
    summary.underline = run_handle.underline();
    summary.superscript = run_handle.superscript();
    summary.subscript = run_handle.subscript();
    summary.font_size_points = run_handle.font_size_points();
    summary.language = run_handle.language();
    summary.east_asia_language = run_handle.east_asia_language();
    summary.bidi_language = run_handle.bidi_language();
    summary.rtl = run_handle.rtl();
    return summary;
}

auto summarize_part_table_handle(featherdoc::Table table_handle, std::size_t table_index)
    -> featherdoc::table_inspection_summary {
    auto summary = featherdoc::table_inspection_summary{};
    summary.index = table_index;
    summary.style_id = table_handle.style_id();
    summary.width_twips = table_handle.width_twips();
    summary.alignment = table_handle.alignment();
    summary.indent_twips = table_handle.indent_twips();
    summary.cell_spacing_twips = table_handle.cell_spacing_twips();
    summary.cell_margin_top_twips =
        table_handle.cell_margin_twips(featherdoc::cell_margin_edge::top);
    summary.cell_margin_left_twips =
        table_handle.cell_margin_twips(featherdoc::cell_margin_edge::left);
    summary.cell_margin_bottom_twips =
        table_handle.cell_margin_twips(featherdoc::cell_margin_edge::bottom);
    summary.cell_margin_right_twips =
        table_handle.cell_margin_twips(featherdoc::cell_margin_edge::right);
    auto borders = featherdoc::table_borders_inspection_summary{};
    borders.top = table_handle.border(featherdoc::table_border_edge::top);
    borders.left = table_handle.border(featherdoc::table_border_edge::left);
    borders.bottom = table_handle.border(featherdoc::table_border_edge::bottom);
    borders.right = table_handle.border(featherdoc::table_border_edge::right);
    borders.inside_horizontal =
        table_handle.border(featherdoc::table_border_edge::inside_horizontal);
    borders.inside_vertical =
        table_handle.border(featherdoc::table_border_edge::inside_vertical);
    if (borders.top || borders.left || borders.bottom || borders.right ||
        borders.inside_horizontal || borders.inside_vertical) {
        summary.borders = std::move(borders);
    }

    bool first_row = true;
    for (auto row = table_handle.rows(); row.has_next(); row.next()) {
        ++summary.row_count;
        summary.row_height_twips.push_back(row.height_twips());
        summary.row_height_rules.push_back(row.height_rule());
        summary.row_cant_split.push_back(row.cant_split());
        summary.row_repeats_header.push_back(row.repeats_header());

        std::size_t row_column_count = 0U;
        bool first_cell = true;
        for (auto cell = row.cells(); cell.has_next(); cell.next()) {
            row_column_count += cell.column_span();

            if (!first_row) {
                if (first_cell) {
                    summary.text.push_back('\n');
                } else {
                    summary.text.push_back('\t');
                }
            } else if (!first_cell) {
                summary.text.push_back('\t');
            }

            summary.text += cell.get_text();
            first_cell = false;
        }

        summary.column_count = std::max(summary.column_count, row_column_count);
        first_row = false;
    }

    summary.column_widths.reserve(summary.column_count);
    for (std::size_t column_index = 0U; column_index < summary.column_count; ++column_index) {
        summary.column_widths.push_back(table_handle.column_width_twips(column_index));
    }

    return summary;
}

auto summarize_part_table_cell_handle(featherdoc::TableCell cell_handle, std::size_t row_index,
                                      std::size_t cell_index, std::size_t column_index)
    -> featherdoc::table_cell_inspection_summary {
    auto summary = featherdoc::table_cell_inspection_summary{};
    summary.row_index = row_index;
    summary.cell_index = cell_index;
    summary.column_index = column_index;
    summary.column_span = cell_handle.column_span();
    summary.row_span = cell_handle.row_span();
    summary.vertical_merge = cell_handle.vertical_merge();
    summary.width_twips = cell_handle.width_twips();
    summary.vertical_alignment = cell_handle.vertical_alignment();
    summary.text_direction = cell_handle.text_direction();
    summary.fill_color = cell_handle.fill_color();
    summary.margin_top_twips =
        cell_handle.margin_twips(featherdoc::cell_margin_edge::top);
    summary.margin_left_twips =
        cell_handle.margin_twips(featherdoc::cell_margin_edge::left);
    summary.margin_bottom_twips =
        cell_handle.margin_twips(featherdoc::cell_margin_edge::bottom);
    summary.margin_right_twips =
        cell_handle.margin_twips(featherdoc::cell_margin_edge::right);
    summary.border_top = cell_handle.border(featherdoc::cell_border_edge::top);
    summary.border_left = cell_handle.border(featherdoc::cell_border_edge::left);
    summary.border_bottom =
        cell_handle.border(featherdoc::cell_border_edge::bottom);
    summary.border_right =
        cell_handle.border(featherdoc::cell_border_edge::right);
    summary.text = cell_handle.get_text();

    for (auto paragraph = cell_handle.paragraphs(); paragraph.has_next(); paragraph.next()) {
        ++summary.paragraph_count;
    }

    return summary;
}

auto collect_part_table_cell_summaries(featherdoc::Table table_handle)
    -> std::vector<featherdoc::table_cell_inspection_summary> {
    auto summaries = std::vector<featherdoc::table_cell_inspection_summary>{};
    auto row = table_handle.rows();
    for (std::size_t row_index = 0U; row.has_next(); ++row_index, row.next()) {
        std::size_t column_index = 0U;
        auto cell = row.cells();
        for (std::size_t cell_index = 0U; cell.has_next(); ++cell_index, cell.next()) {
            auto summary =
                summarize_part_table_cell_handle(cell, row_index, cell_index, column_index);
            column_index += summary.column_span;
            summaries.push_back(std::move(summary));
        }
    }

    return summaries;
}
