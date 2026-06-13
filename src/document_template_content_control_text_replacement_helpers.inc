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
