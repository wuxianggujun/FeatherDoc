auto style_node_id(pugi::xml_node style) -> std::string_view {
    return style.attribute("w:styleId").value();
}

auto build_style_inheritance_chain(
    pugi::xml_node styles_root, pugi::xml_node style,
    featherdoc::document_error_info &last_error_info,
    std::vector<pugi::xml_node> &chain) -> bool {
    chain.clear();
    while (style != pugi::xml_node{}) {
        const auto current_style_id = style_node_id(style);
        for (const auto &visited_style : chain) {
            if (style_node_id(visited_style) == current_style_id) {
                set_last_error(
                    last_error_info,
                    std::make_error_code(std::errc::invalid_argument),
                    "style inheritance cycle detected at style '" +
                        std::string{current_style_id} + "'",
                    std::string{styles_xml_entry});
                return false;
            }
        }

        chain.push_back(style);

        const auto based_on = std::string_view{
            style.child("w:basedOn").attribute("w:val").value()};
        if (based_on.empty()) {
            return true;
        }

        const auto parent_style = find_style_node(styles_root, based_on);
        if (parent_style == pugi::xml_node{}) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{current_style_id} +
                               "' references missing basedOn style '" +
                               std::string{based_on} + "'",
                           std::string{styles_xml_entry});
            return false;
        }

        style = parent_style;
    }

    return true;
}

template <typename Reader>
auto resolve_style_string_property(const std::vector<pugi::xml_node> &chain,
                                   Reader &&reader)
    -> featherdoc::resolved_style_string_property {
    for (const auto &style : chain) {
        if (const auto value = reader(style); value.has_value()) {
            featherdoc::resolved_style_string_property property{};
            property.value = value;
            property.source_style_id = std::string{style_node_id(style)};
            return property;
        }
    }

    return {};
}

template <typename Reader>
auto resolve_style_bool_property(const std::vector<pugi::xml_node> &chain,
                                 Reader &&reader)
    -> featherdoc::resolved_style_bool_property {
    for (const auto &style : chain) {
        if (const auto value = reader(style); value.has_value()) {
            featherdoc::resolved_style_bool_property property{};
            property.value = value;
            property.source_style_id = std::string{style_node_id(style)};
            return property;
        }
    }

    return {};
}

template <typename Reader>
auto resolve_style_double_property(const std::vector<pugi::xml_node> &chain,
                                   Reader &&reader)
    -> featherdoc::resolved_style_double_property {
    for (const auto &style : chain) {
        if (const auto value = reader(style); value.has_value()) {
            featherdoc::resolved_style_double_property property{};
            property.value = value;
            property.source_style_id = std::string{style_node_id(style)};
            return property;
        }
    }

    return {};
}

auto read_on_off_attribute(pugi::xml_node node,
                           const char *attribute_name) -> bool {
    if (node == pugi::xml_node{}) {
        return false;
    }

    const auto attribute = node.attribute(attribute_name);
    if (attribute == pugi::xml_attribute{} || attribute.value()[0] == '\0') {
        return false;
    }

    const auto value = std::string_view{attribute.value()};
    return value != "0" && value != "false" && value != "off";
}

auto decode_style_kind(std::string_view type_name) -> featherdoc::style_kind {
    if (type_name == "paragraph") {
        return featherdoc::style_kind::paragraph;
    }
    if (type_name == "character") {
        return featherdoc::style_kind::character;
    }
    if (type_name == "table") {
        return featherdoc::style_kind::table;
    }
    if (type_name == "numbering") {
        return featherdoc::style_kind::numbering;
    }

    return featherdoc::style_kind::unknown;
}

auto parse_u32_attribute_value(const char *text)
    -> std::optional<std::uint32_t> {
    if (text == nullptr || *text == '\0') {
        return std::nullopt;
    }

    char *end = nullptr;
    const auto value = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' ||
        value > static_cast<unsigned long>(
                    std::numeric_limits<std::uint32_t>::max())) {
        return std::nullopt;
    }

    return static_cast<std::uint32_t>(value);
}

auto parse_half_point_size(const char *text) -> std::optional<double> {
    if (text == nullptr || *text == '\0') {
        return std::nullopt;
    }

    char *end = nullptr;
    const auto value = std::strtod(text, &end);
    if (end == text || *end != '\0' || value <= 0.0) {
        return std::nullopt;
    }

    return value / 2.0;
}

auto summarize_style_numbering(pugi::xml_node style)
    -> std::optional<featherdoc::style_summary::numbering_summary> {
    const auto numbering_properties = style.child("w:pPr").child("w:numPr");
    if (numbering_properties == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto summary = featherdoc::style_summary::numbering_summary{};
    summary.level = parse_u32_attribute_value(
        numbering_properties.child("w:ilvl").attribute("w:val").value());
    summary.num_id = parse_u32_attribute_value(
        numbering_properties.child("w:numId").attribute("w:val").value());
    return summary;
}

auto style_has_numbering(pugi::xml_node style) -> bool {
    return style.child("w:pPr").child("w:numPr") != pugi::xml_node{};
}

auto summarize_style_node(pugi::xml_node style) -> featherdoc::style_summary {
    auto summary = featherdoc::style_summary{};
    summary.style_id = style.attribute("w:styleId").value();
    summary.type_name = style.attribute("w:type").value();
    summary.kind = decode_style_kind(summary.type_name);

    if (const auto name = style.child("w:name").attribute("w:val");
        name != pugi::xml_attribute{}) {
        summary.name = name.value();
    }

    if (const auto based_on = style.child("w:basedOn").attribute("w:val");
        based_on != pugi::xml_attribute{} && based_on.value()[0] != '\0') {
        summary.based_on = std::string{based_on.value()};
    }

    summary.numbering = summarize_style_numbering(style);
    summary.is_default = read_on_off_attribute(style, "w:default");
    summary.is_custom = read_on_off_attribute(style, "w:customStyle");
    summary.is_semi_hidden =
        read_on_off_value(style.child("w:semiHidden")).value_or(false);
    summary.is_unhide_when_used =
        read_on_off_value(style.child("w:unhideWhenUsed")).value_or(false);
    summary.is_quick_format =
        read_on_off_value(style.child("w:qFormat")).value_or(false);
    return summary;
}
