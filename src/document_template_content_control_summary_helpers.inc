auto content_control_kind_from_node(pugi::xml_node content_control)
    -> featherdoc::content_control_kind {
    const auto parent = content_control.parent();
    const auto parent_name = std::string_view{parent.name()};
    if (parent_name == "w:p") {
        return featherdoc::content_control_kind::run;
    }
    if (parent_name == "w:tbl") {
        return featherdoc::content_control_kind::table_row;
    }
    if (parent_name == "w:tr") {
        return featherdoc::content_control_kind::table_cell;
    }
    if (parent_name == "w:body" || parent_name == "w:hdr" ||
        parent_name == "w:ftr" || parent_name == "w:tc" ||
        parent_name == "w:sdtContent") {
        return featherdoc::content_control_kind::block;
    }

    return featherdoc::content_control_kind::unknown;
}

auto optional_child_attribute(pugi::xml_node parent, const char *child_name,
                              const char *attribute_name)
    -> std::optional<std::string> {
    const auto child = parent.child(child_name);
    if (child == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto value = std::string_view{child.attribute(attribute_name).value()};
    if (value.empty()) {
        return std::nullopt;
    }

    return std::string{value};
}

auto optional_attribute(pugi::xml_node node, const char *attribute_name)
    -> std::optional<std::string> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto value = std::string_view{node.attribute(attribute_name).value()};
    if (value.empty()) {
        return std::nullopt;
    }

    return std::string{value};
}

auto optional_attribute_any(pugi::xml_node node,
                            std::initializer_list<const char *> attribute_names)
    -> std::optional<std::string> {
    for (const auto *attribute_name : attribute_names) {
        if (auto value = optional_attribute(node, attribute_name)) {
            return value;
        }
    }

    return std::nullopt;
}

auto optional_child_attribute_any(pugi::xml_node parent,
                                  std::initializer_list<const char *> child_names,
                                  std::initializer_list<const char *> attribute_names)
    -> std::optional<std::string> {
    for (const auto *child_name : child_names) {
        const auto child = parent.child(child_name);
        if (child == pugi::xml_node{}) {
            continue;
        }
        for (const auto *attribute_name : attribute_names) {
            if (auto value = optional_attribute(child, attribute_name)) {
                return value;
            }
        }
    }

    return std::nullopt;
}

auto parse_word_bool(std::string_view value) -> std::optional<bool> {
    if (value == "1" || value == "true" || value == "on") {
        return true;
    }
    if (value == "0" || value == "false" || value == "off") {
        return false;
    }
    return std::nullopt;
}

auto child_any(pugi::xml_node parent,
               std::initializer_list<const char *> child_names) -> pugi::xml_node {
    for (const auto *child_name : child_names) {
        const auto child = parent.child(child_name);
        if (child != pugi::xml_node{}) {
            return child;
        }
    }

    return {};
}

auto content_control_form_kind_from_properties(pugi::xml_node properties)
    -> featherdoc::content_control_form_kind {
    if (child_any(properties, {"w:checkBox", "w14:checkbox"}) !=
        pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::checkbox;
    }
    if (properties.child("w:dropDownList") != pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::drop_down_list;
    }
    if (properties.child("w:comboBox") != pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::combo_box;
    }
    if (properties.child("w:date") != pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::date;
    }
    if (properties.child("w:picture") != pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::picture;
    }
    if (properties.child("w:text") != pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::plain_text;
    }
    if (properties.child("w:repeatingSection") != pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::repeating_section;
    }
    if (properties.child("w:group") != pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::group;
    }
    if (properties.child("w:richText") != pugi::xml_node{}) {
        return featherdoc::content_control_form_kind::rich_text;
    }

    return featherdoc::content_control_form_kind::rich_text;
}

auto list_item_attribute(pugi::xml_node item,
                         std::initializer_list<const char *> attribute_names)
    -> std::string {
    for (const auto *attribute_name : attribute_names) {
        const auto value = std::string_view{item.attribute(attribute_name).value()};
        if (!value.empty()) {
            return std::string{value};
        }
    }

    return {};
}

void summarize_content_control_list_items(
    pugi::xml_node properties, std::string_view selected_text,
    featherdoc::content_control_summary &summary) {
    const auto list_parent =
        summary.form_kind == featherdoc::content_control_form_kind::drop_down_list
            ? properties.child("w:dropDownList")
            : properties.child("w:comboBox");
    if (list_parent == pugi::xml_node{}) {
        return;
    }

    for (auto item = list_parent.child("w:listItem"); item != pugi::xml_node{};
         item = item.next_sibling("w:listItem")) {
        auto list_item = featherdoc::content_control_list_item{};
        list_item.display_text = list_item_attribute(
            item, {"w:displayText", "displayText", "w14:displayText"});
        list_item.value =
            list_item_attribute(item, {"w:value", "value", "w14:value"});
        if (list_item.display_text.empty()) {
            list_item.display_text = list_item.value;
        }
        if (list_item.value.empty()) {
            list_item.value = list_item.display_text;
        }

        const auto item_index = summary.list_items.size();
        if (!summary.selected_list_item.has_value() &&
            (selected_text == list_item.display_text || selected_text == list_item.value)) {
            summary.selected_list_item = item_index;
        }
        summary.list_items.push_back(std::move(list_item));
    }
}

void collect_content_controls_in_order(
    pugi::xml_node node, std::vector<pugi::xml_node> &content_controls) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:sdt") {
            content_controls.push_back(child);
        }
        collect_content_controls_in_order(child, content_controls);
    }
}

auto summarize_content_control(pugi::xml_node content_control,
                               std::size_t index)
    -> featherdoc::content_control_summary {
    auto summary = featherdoc::content_control_summary{};
    summary.index = index;
    summary.kind = content_control_kind_from_node(content_control);

    const auto properties = content_control.child("w:sdtPr");
    summary.form_kind = content_control_form_kind_from_properties(properties);
    summary.tag = optional_child_attribute(properties, "w:tag", "w:val");
    summary.alias = optional_child_attribute(properties, "w:alias", "w:val");
    summary.id = optional_child_attribute(properties, "w:id", "w:val");
    summary.lock = optional_child_attribute(properties, "w:lock", "w:val");
    if (const auto data_binding = properties.child("w:dataBinding");
        data_binding != pugi::xml_node{}) {
        summary.data_binding_store_item_id = optional_attribute_any(
            data_binding, {"w:storeItemID", "storeItemID"});
        summary.data_binding_xpath =
            optional_attribute_any(data_binding, {"w:xpath", "xpath"});
        summary.data_binding_prefix_mappings = optional_attribute_any(
            data_binding, {"w:prefixMappings", "prefixMappings"});
    }
    summary.showing_placeholder =
        properties.child("w:showingPlcHdr") != pugi::xml_node{};

    const auto content = content_control.child("w:sdtContent");
    summary.text = featherdoc::detail::collect_plain_text_from_xml(
        content != pugi::xml_node{} ? content : content_control);

    if (summary.form_kind == featherdoc::content_control_form_kind::checkbox) {
        const auto checkbox = child_any(properties, {"w:checkBox", "w14:checkbox"});
        if (auto checked = optional_child_attribute_any(
                checkbox, {"w:checked", "w14:checked"}, {"w:val", "w14:val"})) {
            summary.checked = parse_word_bool(*checked);
        }
    }

    if (summary.form_kind == featherdoc::content_control_form_kind::date) {
        const auto date = properties.child("w:date");
        summary.date_format =
            optional_child_attribute(date, "w:dateFormat", "w:val");
        summary.date_locale = optional_child_attribute(date, "w:lid", "w:val");
    }

    if (summary.form_kind == featherdoc::content_control_form_kind::drop_down_list ||
        summary.form_kind == featherdoc::content_control_form_kind::combo_box) {
        summarize_content_control_list_items(properties, summary.text, summary);
    }

    return summary;
}

auto summarize_content_controls_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document)
    -> std::vector<featherdoc::content_control_summary> {
    std::vector<pugi::xml_node> content_control_nodes;
    collect_content_controls_in_order(document, content_control_nodes);

    auto summaries = std::vector<featherdoc::content_control_summary>{};
    summaries.reserve(content_control_nodes.size());
    for (std::size_t index = 0; index < content_control_nodes.size();
         ++index) {
        summaries.push_back(
            summarize_content_control(content_control_nodes[index], index));
    }

    last_error_info.clear();
    return summaries;
}

auto filter_content_controls_by_tag_or_alias(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view value, bool match_tag)
    -> std::vector<featherdoc::content_control_summary> {
    if (value.empty()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            match_tag ? "content control tag must not be empty"
                      : "content control alias must not be empty",
            std::string{entry_name});
        return {};
    }

    const auto summaries = summarize_content_controls_in_part(last_error_info,
                                                             document);
    auto matches = std::vector<featherdoc::content_control_summary>{};
    for (const auto &summary : summaries) {
        const auto &candidate = match_tag ? summary.tag : summary.alias;
        if (candidate.has_value() && *candidate == value) {
            matches.push_back(summary);
        }
    }

    last_error_info.clear();
    return matches;
}
