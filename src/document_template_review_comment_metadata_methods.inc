bool Document::set_comment_metadata(
    std::size_t comment_index,
    const featherdoc::comment_metadata_update &metadata) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before setting comment metadata",
                       std::string{document_xml_entry});
        return false;
    }
    const auto has_change = metadata.author.has_value() ||
                            metadata.initials.has_value() ||
                            metadata.date.has_value() ||
                            metadata.clear_author ||
                            metadata.clear_initials ||
                            metadata.clear_date;
    if (!has_change) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment metadata update must contain at least one change",
                       std::string{comments_xml_entry});
        return false;
    }
    if ((metadata.author.has_value() && metadata.clear_author) ||
        (metadata.initials.has_value() && metadata.clear_initials) ||
        (metadata.date.has_value() && metadata.clear_date)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment metadata cannot be set and cleared in the same update",
                       std::string{comments_xml_entry});
        return false;
    }
    if (!this->ensure_comments_part()) {
        return false;
    }

    auto root = this->comments.child("w:comments");
    auto comment = comment_by_summary_index(root, comment_index);
    if (comment == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "comment index is out of range",
                       std::string{comments_xml_entry});
        return false;
    }

    const auto apply_string_metadata =
        [](pugi::xml_node target, const char *name,
           const std::optional<std::string> &value, bool clear) {
            if (value.has_value()) {
                ensure_attribute_value(target, name, *value);
            } else if (clear) {
                target.remove_attribute(name);
            }
        };
    apply_string_metadata(comment, "w:author", metadata.author,
                          metadata.clear_author);
    apply_string_metadata(comment, "w:initials", metadata.initials,
                          metadata.clear_initials);
    apply_string_metadata(comment, "w:date", metadata.date,
                          metadata.clear_date);

    this->comments_dirty = true;
    this->last_error_info.clear();
    return true;
}
