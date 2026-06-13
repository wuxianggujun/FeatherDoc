auto find_checkbox_node(pugi::xml_node properties) -> pugi::xml_node {
    return child_any(properties, {"w14:checkbox", "w:checkBox", "w:checkBox"});
}

bool set_content_control_checkbox_state(pugi::xml_node content_control,
                                        pugi::xml_node properties,
                                        bool checked) {
    auto checkbox = find_checkbox_node(properties);
    if (checkbox == pugi::xml_node{}) {
        return false;
    }

    const auto checkbox_name = std::string_view{checkbox.name()};
    const auto checked_node_name = checkbox_name.rfind("w14:", 0) == 0
                                       ? "w14:checked"
                                       : "w:checked";
    const auto checked_attribute_name = checkbox_name.rfind("w14:", 0) == 0
                                            ? "w14:val"
                                            : "w:val";
    auto checked_node = checkbox.child(checked_node_name);
    if (checked_node == pugi::xml_node{}) {
        checked_node = checkbox.append_child(checked_node_name);
    }
    if (checked_node == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(checked_node, checked_attribute_name,
                           checked ? "1" : "0");
    return rewrite_content_control_plain_text(content_control,
                                              checked ? "\xE2\x98\x92"
                                                      : "\xE2\x98\x90");
}

auto find_content_control_list_parent(pugi::xml_node properties)
    -> pugi::xml_node {
    const auto drop_down = properties.child("w:dropDownList");
    if (drop_down != pugi::xml_node{}) {
        return drop_down;
    }

    return properties.child("w:comboBox");
}

bool set_content_control_selected_item(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_node content_control, pugi::xml_node properties,
    std::string_view entry_name, std::string_view selected_item) {
    const auto list_parent = find_content_control_list_parent(properties);
    if (list_parent == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "content control is not a drop-down or combo box",
                       std::string{entry_name});
        return false;
    }

    auto selected_display_text = std::string{};
    for (auto item = list_parent.child("w:listItem"); item != pugi::xml_node{};
         item = item.next_sibling("w:listItem")) {
        const auto display_text = list_item_attribute(
            item, {"w:displayText", "displayText", "w14:displayText"});
        const auto value =
            list_item_attribute(item, {"w:value", "value", "w14:value"});
        if (selected_item == display_text || selected_item == value) {
            selected_display_text = display_text.empty() ? value : display_text;
            break;
        }
    }

    if (selected_display_text.empty() &&
        std::string_view{list_parent.name()} == "w:comboBox") {
        selected_display_text = std::string{selected_item};
    }

    if (selected_display_text.empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "selected content-control list item was not found",
                       std::string{entry_name});
        return false;
    }

    if (!rewrite_content_control_plain_text(content_control, selected_display_text)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to rewrite content-control list value",
                       std::string{entry_name});
        return false;
    }

    return true;
}

bool set_content_control_date_state(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_node content_control, pugi::xml_node properties,
    std::string_view entry_name,
    const featherdoc::content_control_form_state_options &options) {
    auto date = properties.child("w:date");
    if (date == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "content control is not a date picker",
                       std::string{entry_name});
        return false;
    }

    if (options.date_format.has_value()) {
        auto date_format = date.child("w:dateFormat");
        if (date_format == pugi::xml_node{}) {
            date_format = date.append_child("w:dateFormat");
        }
        if (date_format == pugi::xml_node{}) {
            return false;
        }
        ensure_attribute_value(date_format, "w:val", *options.date_format);
    }

    if (options.date_locale.has_value()) {
        auto locale = date.child("w:lid");
        if (locale == pugi::xml_node{}) {
            locale = date.append_child("w:lid");
        }
        if (locale == pugi::xml_node{}) {
            return false;
        }
        ensure_attribute_value(locale, "w:val", *options.date_locale);
    }

    if (options.date_text.has_value() &&
        !rewrite_content_control_plain_text(content_control, *options.date_text)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to rewrite content-control date text",
                       std::string{entry_name});
        return false;
    }

    return true;
}

auto content_control_form_state_has_changes(
    const featherdoc::content_control_form_state_options &options) -> bool {
    return options.lock.has_value() || options.clear_lock ||
           options.clear_data_binding ||
           content_control_data_binding_options_have_value(options) ||
           options.checked.has_value() || options.selected_list_item.has_value() ||
           options.date_text.has_value() || options.date_format.has_value() ||
           options.date_locale.has_value();
}

bool apply_content_control_form_state(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_node content_control, std::string_view entry_name,
    const featherdoc::content_control_form_state_options &options) {
    auto properties = ensure_content_control_properties_node(content_control);
    if (properties == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create content-control properties",
                       std::string{entry_name});
        return false;
    }

    if (options.clear_lock) {
        clear_content_control_lock(properties);
    }
    if (options.lock.has_value() &&
        !set_content_control_lock(properties, *options.lock)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to set content-control lock",
                       std::string{entry_name});
        return false;
    }

    if (options.clear_data_binding) {
        clear_content_control_data_binding(properties);
    }
    if (content_control_data_binding_options_have_value(options) &&
        !set_content_control_data_binding(properties, options)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to set content-control data binding",
                       std::string{entry_name});
        return false;
    }

    if (options.checked.has_value() &&
        !set_content_control_checkbox_state(content_control, properties,
                                            *options.checked)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "content control is not a checkbox",
                       std::string{entry_name});
        return false;
    }

    if (options.selected_list_item.has_value() &&
        !set_content_control_selected_item(last_error_info, content_control,
                                           properties, entry_name,
                                           *options.selected_list_item)) {
        return false;
    }

    if ((options.date_text.has_value() || options.date_format.has_value() ||
         options.date_locale.has_value()) &&
        !set_content_control_date_state(last_error_info, content_control,
                                        properties, entry_name, options)) {
        return false;
    }

    clear_content_control_placeholder_flag(content_control);
    return true;
}

bool set_content_control_form_state_by_tag_or_alias_in_node(
    featherdoc::document_error_info &last_error_info, pugi::xml_node node,
    std::string_view entry_name, std::string_view value,
    const featherdoc::content_control_form_state_options &options,
    bool match_tag, std::size_t &updated) {
    for (auto child = node.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        if (std::string_view{child.name()} == "w:sdt" &&
            content_control_matches_tag_or_alias(child, value, match_tag)) {
            if (!apply_content_control_form_state(last_error_info, child,
                                                  entry_name, options)) {
                return false;
            }
            ++updated;
            child = next;
            continue;
        }

        if (!set_content_control_form_state_by_tag_or_alias_in_node(
                last_error_info, child, entry_name, value, options, match_tag,
                updated)) {
            return false;
        }
        child = next;
    }

    return true;
}

auto set_content_control_form_state_by_tag_or_alias_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view value,
    const featherdoc::content_control_form_state_options &options,
    bool match_tag) -> std::size_t {
    if (value.empty()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            match_tag ? "content control tag must not be empty"
                      : "content control alias must not be empty",
            std::string{entry_name});
        return 0U;
    }
    if (!content_control_form_state_has_changes(options)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "content control form-state options must include at least one change",
                       std::string{entry_name});
        return 0U;
    }
    if (options.clear_lock && options.lock.has_value()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "content control lock cannot be set and cleared together",
                       std::string{entry_name});
        return 0U;
    }
    if (options.clear_data_binding &&
        content_control_data_binding_options_have_value(options)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "content control data binding cannot be set and cleared together",
                       std::string{entry_name});
        return 0U;
    }
    std::string data_binding_detail;
    if (!validate_content_control_data_binding_options(options,
                                                       data_binding_detail)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       std::move(data_binding_detail), std::string{entry_name});
        return 0U;
    }

    std::size_t updated = 0U;
    if (!set_content_control_form_state_by_tag_or_alias_in_node(
            last_error_info, document, entry_name, value, options, match_tag,
            updated)) {
        return updated;
    }

    last_error_info.clear();
    return updated;
}
