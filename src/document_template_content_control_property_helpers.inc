auto content_control_matches_tag_or_alias(pugi::xml_node content_control,
                                             std::string_view value,
                                             bool match_tag) -> bool {
    const auto properties = content_control.child("w:sdtPr");
    const auto candidate = optional_child_attribute(
        properties, match_tag ? "w:tag" : "w:alias", "w:val");
    return candidate.has_value() && *candidate == value;
}

auto ensure_content_control_properties_node(pugi::xml_node content_control)
    -> pugi::xml_node {
    auto properties = content_control.child("w:sdtPr");
    if (properties != pugi::xml_node{}) {
        return properties;
    }

    return content_control.prepend_child("w:sdtPr");
}

void clear_content_control_lock(pugi::xml_node properties) {
    for (auto lock = properties.child("w:lock"); lock != pugi::xml_node{};) {
        const auto next = lock.next_sibling("w:lock");
        properties.remove_child(lock);
        lock = next;
    }
}

bool set_content_control_lock(pugi::xml_node properties,
                              std::string_view lock_value) {
    clear_content_control_lock(properties);
    auto lock = properties.append_child("w:lock");
    if (lock == pugi::xml_node{}) {
        return false;
    }
    ensure_attribute_value(lock, "w:val", lock_value);
    return true;
}

void clear_content_control_data_binding(pugi::xml_node properties) {
    for (auto data_binding = properties.child("w:dataBinding");
         data_binding != pugi::xml_node{};) {
        const auto next = data_binding.next_sibling("w:dataBinding");
        properties.remove_child(data_binding);
        data_binding = next;
    }
}

auto content_control_data_binding_options_have_value(
    const featherdoc::content_control_form_state_options &options) -> bool {
    return options.data_binding_store_item_id.has_value() ||
           options.data_binding_xpath.has_value() ||
           options.data_binding_prefix_mappings.has_value();
}

auto validate_content_control_data_binding_options(
    const featherdoc::content_control_form_state_options &options,
    std::string &detail) -> bool {
    if (!content_control_data_binding_options_have_value(options)) {
        return true;
    }

    if (!options.data_binding_store_item_id.has_value() ||
        options.data_binding_store_item_id->empty() ||
        !options.data_binding_xpath.has_value() ||
        options.data_binding_xpath->empty()) {
        detail =
            "content control data binding requires non-empty store item id and xpath";
        return false;
    }

    if (options.data_binding_prefix_mappings.has_value() &&
        options.data_binding_prefix_mappings->empty()) {
        detail =
            "content control data binding prefix mappings must not be empty";
        return false;
    }

    return true;
}

bool set_content_control_data_binding(
    pugi::xml_node properties,
    const featherdoc::content_control_form_state_options &options) {
    clear_content_control_data_binding(properties);
    auto data_binding = properties.append_child("w:dataBinding");
    if (data_binding == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(data_binding, "w:storeItemID",
                           *options.data_binding_store_item_id);
    ensure_attribute_value(data_binding, "w:xpath", *options.data_binding_xpath);
    if (options.data_binding_prefix_mappings.has_value()) {
        ensure_attribute_value(data_binding, "w:prefixMappings",
                               *options.data_binding_prefix_mappings);
    }
    return true;
}
