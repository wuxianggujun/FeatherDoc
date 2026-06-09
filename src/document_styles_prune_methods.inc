std::optional<featherdoc::style_prune_plan>
Document::plan_prune_unused_styles() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before planning unused "
                       "style pruning",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::document_xml_parse_failed,
                       "word/document.xml does not contain a w:document root",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    auto plan = featherdoc::style_prune_plan{};
    auto style_nodes = std::unordered_map<std::string, pugi::xml_node>{};
    auto protected_style_ids = std::unordered_set<std::string>{};
    auto traversal_queue = std::vector<std::string>{};

    const auto protect_style_id = [&](std::string_view style_id) {
        if (style_id.empty()) {
            return;
        }
        auto [iterator, inserted] = protected_style_ids.emplace(style_id);
        if (inserted) {
            traversal_queue.push_back(*iterator);
        }
    };

    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        const auto style_id =
            std::string_view{style.attribute("w:styleId").value()};
        if (style_id.empty()) {
            continue;
        }
        ++plan.scanned_style_count;
        style_nodes.emplace(std::string{style_id}, style);
        if (read_on_off_attribute(style, "w:default") ||
            !read_on_off_attribute(style, "w:customStyle")) {
            protect_style_id(style_id);
        }
    }

    auto directly_used_style_ids = std::unordered_set<std::string>{};
    collect_style_references_in_tree(document_root, directly_used_style_ids);
    for (const auto &part : this->header_parts) {
        if (part) {
            collect_style_references_in_tree(part->xml.child("w:hdr"),
                                             directly_used_style_ids);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part) {
            collect_style_references_in_tree(part->xml.child("w:ftr"),
                                             directly_used_style_ids);
        }
    }
    for (const auto &style_id : directly_used_style_ids) {
        protect_style_id(style_id);
    }

    for (std::size_t index = 0U; index < traversal_queue.size(); ++index) {
        const auto iterator = style_nodes.find(traversal_queue[index]);
        if (iterator == style_nodes.end()) {
            continue;
        }

        auto dependencies = std::vector<std::string>{};
        append_style_dependency_ids(iterator->second, dependencies);
        for (const auto &dependency : dependencies) {
            if (style_nodes.contains(dependency)) {
                protect_style_id(dependency);
            }
        }
    }

    plan.protected_style_count = protected_style_ids.size();

    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        const auto style_id = std::string{style.attribute("w:styleId").value()};
        if (!style_id.empty() &&
            read_on_off_attribute(style, "w:customStyle") &&
            !read_on_off_attribute(style, "w:default") &&
            !protected_style_ids.contains(style_id)) {
            plan.removable_style_ids.push_back(style_id);
        }
    }

    this->last_error_info.clear();
    return plan;
}

std::optional<featherdoc::style_prune_summary> Document::prune_unused_styles() {
    const auto plan = this->plan_prune_unused_styles();
    if (!plan.has_value()) {
        return std::nullopt;
    }

    auto summary = featherdoc::style_prune_summary{};
    summary.scanned_style_count = plan->scanned_style_count;
    summary.protected_style_count = plan->protected_style_count;

    if (!plan->removable_style_ids.empty()) {
        if (const auto error = this->ensure_styles_part_attached()) {
            return std::nullopt;
        }
        auto styles_root = this->styles.child("w:styles");
        for (const auto &style_id : plan->removable_style_ids) {
            const auto style = find_style_node(styles_root, style_id);
            if (style == pugi::xml_node{}) {
                continue;
            }
            if (styles_root.remove_child(style)) {
                summary.removed_style_ids.push_back(style_id);
            }
        }
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return summary;
}
