auto normalize_related_word_entry_name(std::string_view target) -> std::string {
    std::string normalized{target};
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    if (!normalized.empty() && normalized.front() == '/') {
        normalized.erase(normalized.begin());
    } else if (normalized.rfind("word/", 0U) != 0U) {
        normalized.insert(0U, "word/");
    }
    return std::filesystem::path{normalized}.lexically_normal().generic_string();
}

auto related_part_entry_for_type(const pugi::xml_document &relationships_document,
                                 std::string_view relationship_type,
                                 std::string_view fallback_entry) -> std::string {
    const auto relationships = relationships_document.child("Relationships");
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Type").value()} !=
            relationship_type) {
            continue;
        }
        const auto target = std::string_view{relationship.attribute("Target").value()};
        if (!target.empty()) {
            return normalize_related_word_entry_name(target);
        }
    }
    return std::string{fallback_entry};
}

auto optional_related_part_entry_for_type(
    const pugi::xml_document &relationships_document,
    std::string_view relationship_type) -> std::optional<std::string> {
    const auto relationships = relationships_document.child("Relationships");
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Type").value()} !=
            relationship_type) {
            continue;
        }
        const auto target =
            std::string_view{relationship.attribute("Target").value()};
        if (!target.empty()) {
            return normalize_related_word_entry_name(target);
        }
    }
    return std::nullopt;
}

auto read_docx_entry_text(const std::filesystem::path &document_path,
                          std::string_view entry_name,
                          std::string &content,
                          featherdoc::document_error_info &last_error_info)
    -> bool {
    int zip_error = 0;
    zip_t *zip = zip_openwitherror(document_path.string().c_str(), 0, 'r', &zip_error);
    if (zip == nullptr) {
        set_last_error(last_error_info, std::make_error_code(std::errc::io_error),
                       "failed to open source DOCX archive",
                       document_path.string());
        return false;
    }

    if (zip_entry_open(zip, entry_name.data()) != 0) {
        zip_close(zip);
        content.clear();
        return true;
    }

    void *buffer = nullptr;
    size_t buffer_size = 0U;
    const auto read_result = zip_entry_read(zip, &buffer, &buffer_size);
    const auto close_result = zip_entry_close(zip);
    zip_close(zip);
    if (read_result < 0 || close_result != 0) {
        if (buffer != nullptr) {
            std::free(buffer);
        }
        set_last_error(last_error_info, std::make_error_code(std::errc::io_error),
                       "failed to read source DOCX XML part",
                       std::string{entry_name});
        return false;
    }

    content.assign(static_cast<const char *>(buffer), buffer_size);
    std::free(buffer);
    return true;
}

auto load_optional_docx_xml_part(
    const std::filesystem::path &document_path, std::string_view entry_name,
    pugi::xml_document &xml_document,
    featherdoc::document_error_info &last_error_info) -> bool {
    std::string xml_text;
    if (!read_docx_entry_text(document_path, entry_name, xml_text, last_error_info)) {
        return false;
    }
    if (xml_text.empty()) {
        xml_document.reset();
        last_error_info.clear();
        return true;
    }

    const auto parse_result = xml_document.load_buffer(xml_text.data(), xml_text.size());
    if (!parse_result) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       parse_result.description(), std::string{entry_name},
                       parse_result.offset >= 0
                           ? std::optional<std::ptrdiff_t>{parse_result.offset}
                           : std::nullopt);
        return false;
    }

    last_error_info.clear();
    return true;
}

auto xml_child_attribute_string(pugi::xml_node node, const char *attribute_name)
    -> std::optional<std::string> {
    const auto value = std::string_view{node.attribute(attribute_name).value()};
    if (value.empty()) {
        return std::nullopt;
    }
    return std::string{value};
}
