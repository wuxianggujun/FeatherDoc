struct string_xml_writer final : pugi::xml_writer {
    std::string text;

    void write(const void *data, size_t size) override {
        this->text.append(static_cast<const char *>(data), size);
    }
};

auto set_last_error(featherdoc::document_error_info &error_info,
                    std::error_code code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code;
auto set_last_error(featherdoc::document_error_info &error_info,
                    featherdoc::document_errc code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code;

auto relationships_document_allows_mutation(
    const pugi::xml_document &relationships_document,
    bool has_relationships_part, std::string_view relationships_entry_name,
    featherdoc::document_error_info &last_error_info) -> bool {
    if (featherdoc::detail::package_relationships_document_allows_mutation(
            relationships_document, has_relationships_part)) {
        return true;
    }
    set_last_error(
        last_error_info, featherdoc::document_errc::invalid_package_structure,
        std::string{relationships_entry_name} +
            " does not contain a valid Relationships root in the package "
            "relationships namespace; relationship mutations are disabled",
        std::string{relationships_entry_name});
    return false;
}

bool ensure_attribute_value(pugi::xml_node node, const char *name,
                            std::string_view value) {
    if (node == pugi::xml_node{}) {
        return false;
    }

    return featherdoc::detail::checked_set_xml_attribute_value(node, name,
                                                               value);
}

auto next_relationship_id(pugi::xml_node relationships) -> std::string {
    std::unordered_set<std::string> used_relationship_ids;
    for (auto relationship =
             featherdoc::detail::first_package_relationship(relationships);
         relationship != pugi::xml_node{};
         relationship =
             featherdoc::detail::next_package_relationship(relationship)) {
        const auto id = relationship.attribute("Id").value();
        if (*id != '\0') {
            used_relationship_ids.emplace(id);
        }
    }

    for (std::size_t next_index = 1U;; ++next_index) {
        auto candidate = "rId" + std::to_string(next_index);
        if (!used_relationship_ids.contains(candidate)) {
            return candidate;
        }
    }
}
