#include "document_styles_refactor_plan_methods.inc"

#include "document_styles_refactor_apply_restore_methods.inc"

bool Document::rename_style(std::string_view old_style_id,
                            std::string_view new_style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before renaming styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (old_style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "old style id must not be empty when renaming styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (new_style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "new style id must not be empty when renaming styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto style = find_style_node(styles_root, old_style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{old_style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (find_style_node(styles_root, new_style_id) != pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target style id '" + std::string{new_style_id} +
                           "' already exists in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (read_on_off_attribute(style, "w:default")) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default style id '" + std::string{old_style_id} +
                           "' cannot be renamed",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::document_xml_parse_failed,
                       "word/document.xml does not contain a w:document root",
                       std::string{document_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    auto changed = true;
    ensure_attribute_value(style, "w:styleId", new_style_id);

    changed = rewrite_style_metadata_references_in_styles(
                  styles_root, old_style_id, new_style_id) ||
              changed;

    changed = rewrite_style_references_in_tree(document_root, old_style_id,
                                               new_style_id) ||
              changed;

    for (const auto &part : this->header_parts) {
        if (part) {
            changed =
                rewrite_style_references_in_tree(part->xml.child("w:hdr"),
                                                 old_style_id, new_style_id) ||
                changed;
        }
    }

    for (const auto &part : this->footer_parts) {
        if (part) {
            changed =
                rewrite_style_references_in_tree(part->xml.child("w:ftr"),
                                                 old_style_id, new_style_id) ||
                changed;
        }
    }

    if (changed) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::merge_style(std::string_view source_style_id,
                           std::string_view target_style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before merging styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (source_style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "source style id must not be empty when merging styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (target_style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target style id must not be empty when merging styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (source_style_id == target_style_id) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "source and target style ids must be different when merging styles",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto source_style = find_style_node(styles_root, source_style_id);
    if (source_style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "source style id '" + std::string{source_style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto target_style = find_style_node(styles_root, target_style_id);
    if (target_style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target style id '" + std::string{target_style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (read_on_off_attribute(source_style, "w:default")) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default style id '" + std::string{source_style_id} +
                           "' cannot be merged",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto source_summary = summarize_style_node(source_style);
    const auto target_summary = summarize_style_node(target_style);
    if (source_summary.kind != target_summary.kind ||
        source_summary.type_name != target_summary.type_name) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "source style id '" + std::string{source_style_id} +
                           "' has type '" + source_summary.type_name +
                           "' but target style id '" +
                           std::string{target_style_id} + "' has type '" +
                           target_summary.type_name + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::document_xml_parse_failed,
                       "word/document.xml does not contain a w:document root",
                       std::string{document_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    rewrite_style_metadata_references_in_styles(styles_root, source_style_id,
                                                target_style_id);
    rewrite_style_references_in_tree(document_root, source_style_id,
                                     target_style_id);

    for (const auto &part : this->header_parts) {
        if (part) {
            rewrite_style_references_in_tree(part->xml.child("w:hdr"),
                                             source_style_id, target_style_id);
        }
    }

    for (const auto &part : this->footer_parts) {
        if (part) {
            rewrite_style_references_in_tree(part->xml.child("w:ftr"),
                                             source_style_id, target_style_id);
        }
    }

    if (!styles_root.remove_child(source_style)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "failed to remove source style id '" +
                           std::string{source_style_id} +
                           "' from word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

#include "document_styles_prune_methods.inc"
