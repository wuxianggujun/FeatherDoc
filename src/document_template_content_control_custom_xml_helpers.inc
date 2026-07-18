struct custom_xml_package_part {
    std::string entry_name;
    std::string store_item_id;
    pugi::xml_document xml;
};

auto ascii_lower_copy(std::string_view value) -> std::string {
    std::string lowered;
    lowered.reserve(value.size());
    for (const char ch : value) {
        lowered.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return lowered;
}

auto starts_with_view(std::string_view value, std::string_view prefix) -> bool {
    return value.size() >= prefix.size() &&
           value.substr(0U, prefix.size()) == prefix;
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

enum class custom_xml_entry_read_status {
    ok,
    missing,
    limit_exceeded,
    read_failed,
};

auto read_custom_xml_entry_text(
    zip_t *archive, const featherdoc::detail::archive_entry_catalog &catalog,
    std::string_view entry_name, std::string &content,
    const featherdoc::archive_limits &limits,
    featherdoc::document_error_info &last_error_info)
    -> custom_xml_entry_read_status {
    content.clear();
    if (archive == nullptr) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "cannot read a Custom XML entry from a null archive",
                       std::string{entry_name});
        return custom_xml_entry_read_status::read_failed;
    }
    if (featherdoc::detail::find_archive_entry_record(catalog, entry_name) ==
        nullptr) {
        return custom_xml_entry_read_status::missing;
    }
    if (featherdoc::detail::open_archive_entry_by_package_name(
            archive, catalog, entry_name) != 0) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::io_error),
            "failed to open Custom XML archive entry", std::string{entry_name});
        return custom_xml_entry_read_status::read_failed;
    }

    if (const auto limit_error =
            featherdoc::detail::enforce_open_zip_entry_xml_size_limit(
                archive, limits, entry_name, last_error_info)) {
        zip_entry_close(archive);
        return custom_xml_entry_read_status::limit_exceeded;
    }

    void *buffer = nullptr;
    size_t buffer_size = 0U;
    const auto read_result = zip_entry_read(archive, &buffer, &buffer_size);
    const auto close_result = zip_entry_close(archive);
    std::unique_ptr<void, decltype(&std::free)> buffer_guard{buffer, &std::free};
    if (read_result < 0 || close_result != 0 ||
        (buffer_size > 0U && buffer == nullptr)) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::io_error),
            "failed to read Custom XML archive entry", std::string{entry_name});
        return custom_xml_entry_read_status::read_failed;
    }

    if (buffer != nullptr && buffer_size > 0U) {
        content.assign(static_cast<const char *>(buffer), buffer_size);
    }
    return custom_xml_entry_read_status::ok;
}

auto collect_custom_xml_item_entries(
    const featherdoc::detail::archive_entry_catalog &catalog)
    -> std::vector<std::string> {
    std::vector<std::string> entries;
    for (const auto &record : catalog.records) {
        if (record.is_directory) {
            continue;
        }
        const std::string_view identity{record.identity_key};
        const auto slash = identity.find_last_of('/');
        const auto file_name = slash == std::string_view::npos
                                   ? identity
                                   : identity.substr(slash + 1U);
        if (starts_with_view(identity, "customxml/") &&
            identity.find("/_rels/") == std::string_view::npos &&
            starts_with_view(file_name, "item") &&
            !starts_with_view(file_name, "itemprops") &&
            ends_with_view(file_name, ".xml")) {
            entries.push_back(record.canonical_key);
        }
    }

    return entries;
}

auto custom_xml_relationships_entry(std::string_view item_entry_name)
    -> std::string {
    const auto slash = item_entry_name.find_last_of('/');
    if (slash == std::string_view::npos) {
        return "_rels/" + std::string{item_entry_name} + ".rels";
    }
    return std::string{item_entry_name.substr(0U, slash + 1U)} + "_rels/" +
           std::string{item_entry_name.substr(slash + 1U)} + ".rels";
}

auto custom_xml_props_entry_from_relationships(
    zip_t *archive, const featherdoc::detail::archive_entry_catalog &catalog,
    const std::string &item_entry_name,
    const featherdoc::archive_limits &limits,
    featherdoc::document_error_info &last_error_info, bool &fatal_read_error)
    -> std::optional<std::string> {
    const auto relationships_entry =
        custom_xml_relationships_entry(item_entry_name);
    std::string relationships_text;
    const auto read_status =
        read_custom_xml_entry_text(archive, catalog, relationships_entry,
                                   relationships_text, limits, last_error_info);
    if (read_status == custom_xml_entry_read_status::missing) {
        return std::nullopt;
    }
    if (read_status != custom_xml_entry_read_status::ok) {
        fatal_read_error = true;
        return std::nullopt;
    }

    pugi::xml_document relationships;
    const auto parse_result = relationships.load_buffer(
        relationships_text.data(), relationships_text.size(),
        featherdoc::detail::package_relationships_xml_parse_options);
    if (!parse_result) {
        (void)featherdoc::detail::set_xml_parse_failure(
            last_error_info, parse_result,
            featherdoc::document_errc::relationships_xml_parse_failed,
            relationships_entry);
        fatal_read_error = true;
        return std::nullopt;
    }

    const auto mce_result =
        featherdoc::detail::preprocess_package_relationships_mce(relationships);
    if (!mce_result) {
        (void)featherdoc::detail::set_package_relationships_mce_last_error(
            last_error_info, mce_result, relationships_entry);
        fatal_read_error = true;
        return std::nullopt;
    }

    if (featherdoc::detail::inspect_package_relationships_document(
            relationships) !=
        featherdoc::detail::package_relationships_document_state::valid) {
        set_last_error(
            last_error_info,
            featherdoc::document_errc::invalid_package_structure,
            "Custom XML relationships part does not contain a valid "
            "Relationships root in the package relationships namespace",
            relationships_entry);
        fatal_read_error = true;
        return std::nullopt;
    }

    const auto root =
        featherdoc::detail::package_relationships_root(relationships);
    constexpr auto custom_xml_properties_relationship_type = std::string_view{
        "http://schemas.openxmlformats.org/officeDocument/2006/"
        "relationships/customXmlProps"};
    std::optional<std::string> properties_entry;
    for (auto relationship =
             featherdoc::detail::first_package_relationship(root);
         relationship != pugi::xml_node{};
         relationship =
             featherdoc::detail::next_package_relationship(relationship)) {
        const auto type =
            std::string_view{relationship.attribute("Type").value()};
        if (type != custom_xml_properties_relationship_type) {
            continue;
        }
        if (properties_entry.has_value()) {
            set_last_error(last_error_info,
                           featherdoc::document_errc::invalid_package_structure,
                           "Custom XML item declares multiple customXmlProps "
                           "relationships",
                           relationships_entry);
            fatal_read_error = true;
            return std::nullopt;
        }
        if (std::string_view{relationship.attribute("TargetMode").value()} ==
            "External") {
            set_last_error(last_error_info,
                           featherdoc::document_errc::invalid_package_structure,
                           "Custom XML properties relationship must not use "
                           "TargetMode=External",
                           relationships_entry);
            fatal_read_error = true;
            return std::nullopt;
        }
        const auto target =
            std::string_view{relationship.attribute("Target").value()};
        const auto resolved =
            featherdoc::detail::resolve_internal_package_relationship_target(
                item_entry_name, target);
        if (!resolved) {
            set_last_error(
                last_error_info,
                featherdoc::document_errc::invalid_package_structure,
                std::string{featherdoc::detail::
                                package_relationship_target_error_message(
                                    resolved.error)},
                relationships_entry);
            fatal_read_error = true;
            return std::nullopt;
        }
        properties_entry = resolved.entry_name;
    }

    return properties_entry;
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
    const auto folded_file_name = ascii_lower_copy(file_name);
    if (!starts_with_view(folded_file_name, "item") ||
        !ends_with_view(folded_file_name, ".xml")) {
        return std::nullopt;
    }
    const auto number = file_name.substr(4U, file_name.size() - 4U - 4U);
    if (number.empty()) {
        return std::nullopt;
    }
    return std::string{directory} + "itemProps" + std::string{number} + ".xml";
}

auto custom_xml_store_item_id_from_props(
    std::string_view props_xml, std::string_view props_entry_name,
    featherdoc::document_error_info &last_error_info, bool &fatal_parse_error)
    -> std::optional<std::string> {
    pugi::xml_document props;
    const auto parse_result =
        props.load_buffer(props_xml.data(), props_xml.size());
    if (!parse_result) {
        if (parse_result.status == pugi::status_out_of_memory) {
            (void)featherdoc::detail::set_xml_parse_failure(
                last_error_info, parse_result,
                featherdoc::document_errc::invalid_package_structure,
                props_entry_name);
            fatal_parse_error = true;
        }
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
    const featherdoc::archive_limits &limits,
    featherdoc::package_validation_mode validation_mode,
    const featherdoc::detail::source_archive_fingerprint
        *expected_source_fingerprint,
    featherdoc::document_error_info &last_error_info)
    -> std::optional<std::unordered_map<std::string, custom_xml_package_part>> {
    std::unordered_map<std::string, custom_xml_package_part> parts;
    if (document_path.empty()) {
        return parts;
    }

    const auto archive_path = featherdoc::detail::path_to_utf8(document_path);
    const auto native_archive_path =
        featherdoc::detail::path_to_native_utf8(document_path);
    const auto opened_archive =
        featherdoc::detail::open_read_zip_archive_with_limits(
            native_archive_path, limits);
    zip_t *archive = opened_archive.archive;
    if (archive == nullptr) {
        if (expected_source_fingerprint != nullptr) {
            (void)featherdoc::detail::set_reopened_source_archive_open_error(
                last_error_info, archive_path, opened_archive,
                "synchronizing content controls from Custom XML");
            return std::nullopt;
        }
        if (opened_archive.zip_error == ZIP_EARCHLIMIT) {
            (void)featherdoc::detail::set_archive_reader_open_limit_error(
                last_error_info, archive_path, opened_archive.limit_violation);
            return std::nullopt;
        }
        set_last_error(last_error_info,
                       featherdoc::document_errc::archive_open_failed,
                       "failed to open archive for Custom XML sync: " +
                           std::to_string(opened_archive.zip_error));
        return std::nullopt;
    }
    featherdoc::detail::read_zip_archive_guard archive_guard{archive};

    featherdoc::detail::archive_entry_catalog catalog;
    const auto validation_error =
        expected_source_fingerprint == nullptr
            ? featherdoc::detail::validate_open_zip_archive_metadata(
                  archive, limits, last_error_info, validation_mode, &catalog)
            : featherdoc::detail::validate_reopened_source_archive_metadata(
                  archive, limits, last_error_info, validation_mode,
                  *expected_source_fingerprint, archive_path, &catalog);
    if (validation_error) {
        return std::nullopt;
    }

    const auto item_entries = collect_custom_xml_item_entries(catalog);
    for (const auto &item_entry_name : item_entries) {
        bool fatal_read_error = false;
        auto props_entry_name = custom_xml_props_entry_from_relationships(
            archive, catalog, item_entry_name, limits, last_error_info,
            fatal_read_error);
        if (fatal_read_error) {
            return std::nullopt;
        }
        if (!props_entry_name.has_value()) {
            props_entry_name = infer_custom_xml_props_entry(item_entry_name);
        }
        if (!props_entry_name.has_value()) {
            continue;
        }

        std::string props_xml;
        const auto props_read_status =
            read_custom_xml_entry_text(archive, catalog, *props_entry_name,
                                       props_xml, limits, last_error_info);
        if (props_read_status == custom_xml_entry_read_status::missing) {
            continue;
        }
        if (props_read_status != custom_xml_entry_read_status::ok) {
            return std::nullopt;
        }
        bool fatal_props_parse_error = false;
        auto store_item_id = custom_xml_store_item_id_from_props(
            props_xml, *props_entry_name, last_error_info,
            fatal_props_parse_error);
        if (fatal_props_parse_error) {
            return std::nullopt;
        }
        if (!store_item_id.has_value() || store_item_id->empty()) {
            continue;
        }

        std::string item_xml;
        const auto item_read_status =
            read_custom_xml_entry_text(archive, catalog, item_entry_name,
                                       item_xml, limits, last_error_info);
        if (item_read_status == custom_xml_entry_read_status::missing) {
            continue;
        }
        if (item_read_status != custom_xml_entry_read_status::ok) {
            return std::nullopt;
        }
        pugi::xml_document item_document;
        const auto item_parse_result =
            item_document.load_buffer(item_xml.data(), item_xml.size());
        if (!item_parse_result) {
            if (item_parse_result.status == pugi::status_out_of_memory) {
                (void)featherdoc::detail::set_xml_parse_failure(
                    last_error_info, item_parse_result,
                    featherdoc::document_errc::invalid_package_structure,
                    item_entry_name);
                return std::nullopt;
            }
            continue;
        }

        custom_xml_package_part part;
        part.entry_name = item_entry_name;
        part.store_item_id = *store_item_id;
        part.xml = std::move(item_document);
        parts[normalize_store_item_id(part.store_item_id)] = std::move(part);
    }

    if (const auto close_error = featherdoc::detail::close_read_archive(
            archive_guard, last_error_info,
            "failed to close source archive after loading Custom XML parts")) {
        return std::nullopt;
    }
    return parts;
}

void collect_custom_xml_plain_text(std::string &text, pugi::xml_node node) {
    for (auto current = node.first_child(); current != pugi::xml_node{};
         current = featherdoc::detail::next_xml_node_preorder(node, current)) {
        const auto type = current.type();
        if (type == pugi::node_pcdata || type == pugi::node_cdata) {
            text += current.value();
        }
    }
}

auto custom_xml_node_text(pugi::xml_node node) -> std::string {
    std::string text;
    collect_custom_xml_plain_text(text, node);
    return text;
}

auto find_child_by_local_name(pugi::xml_node parent,
                              std::string_view local_name) -> pugi::xml_node {
    for (auto child = parent.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (xml_local_name(child.name()) == local_name) {
            return child;
        }
    }
    return {};
}

auto find_attribute_by_local_name(pugi::xml_node node,
                                  std::string_view local_name)
    -> std::optional<std::string> {
    for (auto attribute = node.first_attribute();
         attribute != pugi::xml_attribute{};
         attribute = attribute.next_attribute()) {
        if (xml_local_name(attribute.name()) == local_name) {
            return std::string{attribute.value()};
        }
    }
    return std::nullopt;
}

auto custom_xml_value_for_xpath(const custom_xml_package_part &part,
                                std::string_view xpath)
    -> std::optional<std::string> {
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
    issue.store_item_id =
        summary.data_binding_store_item_id.value_or(std::string{});
    issue.xpath = summary.data_binding_xpath.value_or(std::string{});
    issue.reason = std::string{reason};
    result.issues.push_back(std::move(issue));
}

bool sync_content_controls_from_custom_xml_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    const std::unordered_map<std::string, custom_xml_package_part>
        &custom_xml_parts,
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
            append_custom_xml_sync_issue(result, std::string{entry_name},
                                         summary, "custom_xml_part_not_found");
            continue;
        }

        const auto value = custom_xml_value_for_xpath(
            part_it->second, *summary.data_binding_xpath);
        if (!value.has_value()) {
            append_custom_xml_sync_issue(result, std::string{entry_name},
                                         summary, "custom_xml_value_not_found");
            continue;
        }

        if (!rewrite_content_control_plain_text(content_control, *value)) {
            (void)featherdoc::detail::
                set_xml_document_mutation_allocation_failure(last_error_info,
                                                             entry_name);
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
