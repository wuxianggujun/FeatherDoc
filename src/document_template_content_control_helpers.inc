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

void clear_node_children(pugi::xml_node node) {
    for (auto child = node.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        node.remove_child(child);
        child = next;
    }
}

auto ensure_content_control_content_node(pugi::xml_node content_control)
    -> pugi::xml_node {
    auto content = content_control.child("w:sdtContent");
    if (content != pugi::xml_node{}) {
        return content;
    }

    return content_control.append_child("w:sdtContent");
}

auto append_text_table_cell(pugi::xml_node parent, std::string_view text)
    -> bool {
    auto cell = parent.append_child("w:tc");
    if (cell == pugi::xml_node{}) {
        return false;
    }

    return featherdoc::detail::append_plain_text_paragraph(cell, text);
}

void clear_content_control_placeholder_flag(pugi::xml_node content_control) {
    if (auto properties = content_control.child("w:sdtPr");
        properties != pugi::xml_node{}) {
        for (auto placeholder = properties.child("w:showingPlcHdr");
             placeholder != pugi::xml_node{};) {
            const auto next = placeholder.next_sibling("w:showingPlcHdr");
            properties.remove_child(placeholder);
            placeholder = next;
        }
    }
}

auto rewrite_content_control_plain_text(pugi::xml_node content_control,
                                        std::string_view replacement)
    -> bool {
    if (content_control == pugi::xml_node{}) {
        return false;
    }

    auto content = ensure_content_control_content_node(content_control);
    if (content == pugi::xml_node{}) {
        return false;
    }

    const auto kind = content_control_kind_from_node(content_control);
    clear_node_children(content);

    switch (kind) {
    case featherdoc::content_control_kind::run: {
        auto run = content.append_child("w:r");
        if (run == pugi::xml_node{}) {
            return false;
        }
        if (!featherdoc::detail::set_plain_text_run_content(run, replacement)) {
            return false;
        }
        break;
    }
    case featherdoc::content_control_kind::table_row: {
        auto row = content.append_child("w:tr");
        if (row == pugi::xml_node{} || !append_text_table_cell(row, replacement)) {
            return false;
        }
        break;
    }
    case featherdoc::content_control_kind::table_cell:
        if (!append_text_table_cell(content, replacement)) {
            return false;
        }
        break;
    case featherdoc::content_control_kind::block:
    case featherdoc::content_control_kind::unknown:
        if (!featherdoc::detail::append_plain_text_paragraph(content,
                                                            replacement)) {
            return false;
        }
        break;
    }

    if (auto properties = content_control.child("w:sdtPr");
        properties != pugi::xml_node{}) {
        for (auto placeholder = properties.child("w:showingPlcHdr");
             placeholder != pugi::xml_node{};) {
            const auto next = placeholder.next_sibling("w:showingPlcHdr");
            properties.remove_child(placeholder);
            placeholder = next;
        }
    }

    return true;
}

#include "document_template_content_control_custom_xml_helpers.inc"

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

bool replace_content_control_text_by_tag_or_alias_in_node(
    featherdoc::document_error_info &last_error_info, pugi::xml_node node,
    std::string_view entry_name, std::string_view value,
    std::string_view replacement, bool match_tag, std::size_t &replaced) {
    for (auto child = node.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        if (std::string_view{child.name()} == "w:sdt" &&
            content_control_matches_tag_or_alias(child, value, match_tag)) {
            if (!rewrite_content_control_plain_text(child, replacement)) {
                set_last_error(
                    last_error_info,
                    std::make_error_code(std::errc::not_enough_memory),
                    "failed to rewrite content control text",
                    std::string{entry_name});
                return false;
            }
            ++replaced;
            child = next;
            continue;
        }

        if (!replace_content_control_text_by_tag_or_alias_in_node(
                last_error_info, child, entry_name, value, replacement,
                match_tag, replaced)) {
            return false;
        }
        child = next;
    }

    return true;
}

auto replace_content_control_text_by_tag_or_alias_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view value, std::string_view replacement, bool match_tag)
    -> std::size_t {
    if (value.empty()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            match_tag ? "content control tag must not be empty"
                      : "content control alias must not be empty",
            std::string{entry_name});
        return 0U;
    }

    std::size_t replaced = 0U;
    if (!replace_content_control_text_by_tag_or_alias_in_node(
            last_error_info, document, entry_name, value, replacement,
            match_tag, replaced)) {
        return replaced;
    }

    last_error_info.clear();
    return replaced;
}


