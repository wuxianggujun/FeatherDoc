struct custom_xml_package_part {
    std::string entry_name;
    std::string store_item_id;
    pugi::xml_document xml;
};

auto ascii_lower_copy(std::string_view value) -> std::string {
    std::string lowered;
    lowered.reserve(value.size());
    for (const char ch : value) {
        lowered.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch))));
    }
    return lowered;
}

auto starts_with_view(std::string_view value, std::string_view prefix) -> bool {
    return value.size() >= prefix.size() && value.substr(0U, prefix.size()) == prefix;
}

auto ends_with_view(std::string_view value, std::string_view suffix) -> bool {
    return value.size() >= suffix.size() &&
           value.substr(value.size() - suffix.size()) == suffix;
}

auto xml_local_name(std::string_view qualified_name) -> std::string_view {
    const auto colon = qualified_name.find(':');
    if (colon == std::string_view::npos) {
        return qualified_name;
    }
    return qualified_name.substr(colon + 1U);
}

auto clean_xpath_segment(std::string_view segment) -> std::string_view {
    const auto bracket = segment.find('[');
    if (bracket != std::string_view::npos) {
        segment = segment.substr(0U, bracket);
    }
    return segment;
}

auto normalize_store_item_id(std::string_view store_item_id) -> std::string {
    return ascii_lower_copy(store_item_id);
}

auto read_zip_text_entry(zip_t *archive, const std::string &entry_name,
                         std::string &content) -> bool {
    content.clear();
    if (archive == nullptr || zip_entry_open(archive, entry_name.c_str()) != 0) {
        return false;
    }

    void *buffer = nullptr;
    size_t buffer_size = 0U;
    const auto read_result = zip_entry_read(archive, &buffer, &buffer_size);
    if (read_result < 0) {
        zip_entry_close(archive);
        return false;
    }

    if (buffer != nullptr && buffer_size > 0U) {
        content.assign(static_cast<const char *>(buffer), buffer_size);
    }
    std::free(buffer);
    return zip_entry_close(archive) == 0;
}

auto collect_custom_xml_item_entries(zip_t *archive) -> std::vector<std::string> {
    std::vector<std::string> entries;
    if (archive == nullptr) {
        return entries;
    }

    const auto total_entries = zip_entries_total(archive);
    if (total_entries < 0) {
        return entries;
    }

    for (ssize_t index = 0; index < total_entries; ++index) {
        if (zip_entry_openbyindex(archive, static_cast<std::size_t>(index)) != 0) {
            continue;
        }
        const char *entry_name = zip_entry_name(archive);
        if (entry_name != nullptr) {
            const std::string_view name{entry_name};
            const auto slash = name.find_last_of('/');
            const auto file_name = slash == std::string_view::npos
                                       ? name
                                       : name.substr(slash + 1U);
            if (starts_with_view(name, "customXml/") &&
                name.find("/_rels/") == std::string_view::npos &&
                starts_with_view(file_name, "item") &&
                !starts_with_view(file_name, "itemProps") &&
                ends_with_view(file_name, ".xml")) {
                entries.emplace_back(name);
            }
        }
        zip_entry_close(archive);
    }

    return entries;
}

auto custom_xml_relationships_entry(std::string_view item_entry_name) -> std::string {
    const auto slash = item_entry_name.find_last_of('/');
    if (slash == std::string_view::npos) {
        return "_rels/" + std::string{item_entry_name} + ".rels";
    }
    return std::string{item_entry_name.substr(0U, slash + 1U)} + "_rels/" +
           std::string{item_entry_name.substr(slash + 1U)} + ".rels";
}

auto resolve_custom_xml_relationship_target(std::string_view item_entry_name,
                                            std::string_view target) -> std::string {
    if (target.empty()) {
        return {};
    }
    if (target.front() == '/') {
        return std::string{target.substr(1U)};
    }

    std::string base;
    const auto slash = item_entry_name.find_last_of('/');
    if (slash != std::string_view::npos) {
        base = std::string{item_entry_name.substr(0U, slash + 1U)};
    }

    std::string remaining{target};
    while (starts_with_view(remaining, "../")) {
        remaining.erase(0U, 3U);
        if (!base.empty() && base.back() == '/') {
            base.pop_back();
        }
        const auto parent_slash = base.find_last_of('/');
        base = parent_slash == std::string::npos ? std::string{} : base.substr(0U, parent_slash + 1U);
    }

    return base + remaining;
}

auto custom_xml_props_entry_from_relationships(zip_t *archive,
                                               const std::string &item_entry_name)
    -> std::optional<std::string> {
    const auto relationships_entry = custom_xml_relationships_entry(item_entry_name);
    std::string relationships_text;
    if (!read_zip_text_entry(archive, relationships_entry, relationships_text)) {
        return std::nullopt;
    }

    pugi::xml_document relationships;
    if (!relationships.load_buffer(relationships_text.data(), relationships_text.size())) {
        return std::nullopt;
    }

    const auto root = relationships.child("Relationships");
    for (auto relationship = root.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        const auto type = std::string_view{relationship.attribute("Type").value()};
        if (!ends_with_view(type, "/customXmlProps")) {
            continue;
        }
        const auto target = std::string_view{relationship.attribute("Target").value()};
        const auto resolved = resolve_custom_xml_relationship_target(item_entry_name, target);
        if (!resolved.empty()) {
            return resolved;
        }
    }

    return std::nullopt;
}

auto infer_custom_xml_props_entry(std::string_view item_entry_name)
    -> std::optional<std::string> {
    const auto slash = item_entry_name.find_last_of('/');
    const auto directory = slash == std::string_view::npos
                               ? std::string_view{}
                               : item_entry_name.substr(0U, slash + 1U);
    const auto file_name = slash == std::string_view::npos
                               ? item_entry_name
                               : item_entry_name.substr(slash + 1U);
    if (!starts_with_view(file_name, "item") || !ends_with_view(file_name, ".xml")) {
        return std::nullopt;
    }
    const auto number = file_name.substr(4U, file_name.size() - 4U - 4U);
    if (number.empty()) {
        return std::nullopt;
    }
    return std::string{directory} + "itemProps" + std::string{number} + ".xml";
}

auto custom_xml_store_item_id_from_props(std::string_view props_xml)
    -> std::optional<std::string> {
    pugi::xml_document props;
    if (!props.load_buffer(props_xml.data(), props_xml.size())) {
        return std::nullopt;
    }
    const auto root = props.document_element();
    if (root == pugi::xml_node{}) {
        return std::nullopt;
    }
    return optional_attribute_any(root, {"ds:itemID", "itemID"});
}

auto load_custom_xml_package_parts(
    const std::filesystem::path &document_path,
    featherdoc::document_error_info &last_error_info)
    -> std::optional<std::unordered_map<std::string, custom_xml_package_part>> {
    std::unordered_map<std::string, custom_xml_package_part> parts;
    if (document_path.empty()) {
        return parts;
    }

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(document_path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                       &zip_error);
    if (archive == nullptr) {
        set_last_error(last_error_info, featherdoc::document_errc::archive_open_failed,
                       "failed to open archive for Custom XML sync: " +
                           std::to_string(zip_error));
        return std::nullopt;
    }

    const auto item_entries = collect_custom_xml_item_entries(archive);
    for (const auto &item_entry_name : item_entries) {
        auto props_entry_name =
            custom_xml_props_entry_from_relationships(archive, item_entry_name);
        if (!props_entry_name.has_value()) {
            props_entry_name = infer_custom_xml_props_entry(item_entry_name);
        }
        if (!props_entry_name.has_value()) {
            continue;
        }

        std::string props_xml;
        if (!read_zip_text_entry(archive, *props_entry_name, props_xml)) {
            continue;
        }
        auto store_item_id = custom_xml_store_item_id_from_props(props_xml);
        if (!store_item_id.has_value() || store_item_id->empty()) {
            continue;
        }

        std::string item_xml;
        if (!read_zip_text_entry(archive, item_entry_name, item_xml)) {
            continue;
        }
        pugi::xml_document item_document;
        if (!item_document.load_buffer(item_xml.data(), item_xml.size())) {
            continue;
        }

        custom_xml_package_part part;
        part.entry_name = item_entry_name;
        part.store_item_id = *store_item_id;
        part.xml.reset(item_document);
        parts[normalize_store_item_id(part.store_item_id)] = std::move(part);
    }

    zip_close(archive);
    return parts;
}

void collect_custom_xml_plain_text(std::string &text, pugi::xml_node node) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto type = child.type();
        if (type == pugi::node_pcdata || type == pugi::node_cdata) {
            text += child.value();
            continue;
        }
        collect_custom_xml_plain_text(text, child);
    }
}

auto custom_xml_node_text(pugi::xml_node node) -> std::string {
    std::string text;
    collect_custom_xml_plain_text(text, node);
    return text;
}

auto find_child_by_local_name(pugi::xml_node parent, std::string_view local_name)
    -> pugi::xml_node {
    for (auto child = parent.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (xml_local_name(child.name()) == local_name) {
            return child;
        }
    }
    return {};
}

auto find_attribute_by_local_name(pugi::xml_node node, std::string_view local_name)
    -> std::optional<std::string> {
    for (auto attribute = node.first_attribute(); attribute != pugi::xml_attribute{};
         attribute = attribute.next_attribute()) {
        if (xml_local_name(attribute.name()) == local_name) {
            return std::string{attribute.value()};
        }
    }
    return std::nullopt;
}

auto custom_xml_value_for_xpath(const custom_xml_package_part &part,
                                std::string_view xpath) -> std::optional<std::string> {
    if (xpath.empty() || xpath.front() != '/') {
        return std::nullopt;
    }

    std::vector<std::string_view> segments;
    std::size_t offset = 1U;
    while (offset <= xpath.size()) {
        const auto slash = xpath.find('/', offset);
        const auto end = slash == std::string_view::npos ? xpath.size() : slash;
        auto segment = clean_xpath_segment(xpath.substr(offset, end - offset));
        if (!segment.empty()) {
            segments.push_back(segment);
        }
        if (slash == std::string_view::npos) {
            break;
        }
        offset = slash + 1U;
    }
    if (segments.empty()) {
        return std::nullopt;
    }

    auto current = part.xml.document_element();
    if (current == pugi::xml_node{}) {
        return std::nullopt;
    }

    std::size_t segment_index = 0U;
    if (xml_local_name(current.name()) == xml_local_name(segments.front())) {
        segment_index = 1U;
    }

    for (; segment_index < segments.size(); ++segment_index) {
        const auto segment = segments[segment_index];
        if (segment == "text()") {
            return custom_xml_node_text(current);
        }
        if (!segment.empty() && segment.front() == '@') {
            return find_attribute_by_local_name(
                current, xml_local_name(segment.substr(1U)));
        }
        current = find_child_by_local_name(current, xml_local_name(segment));
        if (current == pugi::xml_node{}) {
            return std::nullopt;
        }
    }

    return custom_xml_node_text(current);
}

void append_custom_xml_sync_issue(
    featherdoc::custom_xml_data_binding_sync_result &result,
    const std::string &part_entry_name,
    const featherdoc::content_control_summary &summary,
    std::string_view reason) {
    featherdoc::custom_xml_data_binding_sync_issue issue;
    issue.part_entry_name = part_entry_name;
    issue.content_control_index = summary.index;
    issue.tag = summary.tag;
    issue.alias = summary.alias;
    issue.store_item_id = summary.data_binding_store_item_id.value_or(std::string{});
    issue.xpath = summary.data_binding_xpath.value_or(std::string{});
    issue.reason = std::string{reason};
    result.issues.push_back(std::move(issue));
}

bool sync_content_controls_from_custom_xml_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name,
    const std::unordered_map<std::string, custom_xml_package_part> &custom_xml_parts,
    featherdoc::custom_xml_data_binding_sync_result &result) {
    std::vector<pugi::xml_node> content_controls;
    collect_content_controls_in_order(document, content_controls);

    for (std::size_t index = 0U; index < content_controls.size(); ++index) {
        const auto content_control = content_controls[index];
        auto summary = summarize_content_control(content_control, index);
        ++result.scanned_content_controls;

        if (!summary.data_binding_store_item_id.has_value() ||
            summary.data_binding_store_item_id->empty() ||
            !summary.data_binding_xpath.has_value() ||
            summary.data_binding_xpath->empty()) {
            continue;
        }

        ++result.bound_content_controls;
        const auto part_it = custom_xml_parts.find(
            normalize_store_item_id(*summary.data_binding_store_item_id));
        if (part_it == custom_xml_parts.end()) {
            append_custom_xml_sync_issue(result, std::string{entry_name}, summary,
                                         "custom_xml_part_not_found");
            continue;
        }

        const auto value = custom_xml_value_for_xpath(part_it->second,
                                                      *summary.data_binding_xpath);
        if (!value.has_value()) {
            append_custom_xml_sync_issue(result, std::string{entry_name}, summary,
                                         "custom_xml_value_not_found");
            continue;
        }

        if (!rewrite_content_control_plain_text(content_control, *value)) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to synchronize content control text from Custom XML",
                           std::string{entry_name});
            return false;
        }

        featherdoc::custom_xml_data_binding_sync_item item;
        item.part_entry_name = std::string{entry_name};
        item.content_control_index = summary.index;
        item.tag = summary.tag;
        item.alias = summary.alias;
        item.store_item_id = *summary.data_binding_store_item_id;
        item.xpath = *summary.data_binding_xpath;
        item.previous_text = summary.text;
        item.value = *value;
        result.synced_items.push_back(std::move(item));
        ++result.synced_content_controls;
    }

    return true;
}
