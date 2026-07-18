auto node_style_reference_matches(pugi::xml_node node,
                                  const char *properties_name,
                                  const char *style_name,
                                  std::string_view style_id) -> bool {
    const auto style_value =
        node.child(properties_name).child(style_name).attribute("w:val");
    if (style_value == pugi::xml_attribute{}) {
        return false;
    }

    return std::string_view{style_value.value()} == style_id;
}

enum class style_reference_rewrite_status {
    no_match,
    rewritten,
    allocation_failure,
};

auto rewrite_style_reference_value_checked(pugi::xml_attribute value_attribute,
                                           std::string_view old_style_id,
                                           std::string_view new_style_id)
    -> style_reference_rewrite_status {
    if (value_attribute == pugi::xml_attribute{} ||
        std::string_view{value_attribute.value()} != old_style_id) {
        return style_reference_rewrite_status::no_match;
    }

    return value_attribute.set_value(new_style_id.data(), new_style_id.size())
               ? style_reference_rewrite_status::rewritten
               : style_reference_rewrite_status::allocation_failure;
}

auto rewrite_style_reference_value(pugi::xml_attribute value_attribute,
                                   std::string_view old_style_id,
                                   std::string_view new_style_id) -> bool {
    return rewrite_style_reference_value_checked(value_attribute, old_style_id,
                                                 new_style_id) ==
           style_reference_rewrite_status::rewritten;
}

auto rewrite_style_reference_child_checked(pugi::xml_node node,
                                           const char *properties_name,
                                           const char *style_name,
                                           std::string_view old_style_id,
                                           std::string_view new_style_id)
    -> style_reference_rewrite_status {
    return rewrite_style_reference_value_checked(
        node.child(properties_name).child(style_name).attribute("w:val"),
        old_style_id, new_style_id);
}

auto rewrite_style_reference_child(pugi::xml_node node,
                                   const char *properties_name,
                                   const char *style_name,
                                   std::string_view old_style_id,
                                   std::string_view new_style_id) -> bool {
    return rewrite_style_reference_child_checked(
               node, properties_name, style_name, old_style_id, new_style_id) ==
           style_reference_rewrite_status::rewritten;
}

auto rewrite_style_metadata_reference(pugi::xml_node style,
                                      const char *child_name,
                                      std::string_view old_style_id,
                                      std::string_view new_style_id) -> bool {
    return rewrite_style_reference_value(
        style.child(child_name).attribute("w:val"), old_style_id, new_style_id);
}

auto combine_style_reference_rewrite_status(
    style_reference_rewrite_status aggregate,
    style_reference_rewrite_status current) -> style_reference_rewrite_status {
    if (aggregate == style_reference_rewrite_status::allocation_failure ||
        current == style_reference_rewrite_status::allocation_failure) {
        return style_reference_rewrite_status::allocation_failure;
    }
    if (aggregate == style_reference_rewrite_status::rewritten ||
        current == style_reference_rewrite_status::rewritten) {
        return style_reference_rewrite_status::rewritten;
    }
    return style_reference_rewrite_status::no_match;
}

auto rewrite_style_metadata_references_in_styles_checked(
    pugi::xml_node styles_root, std::string_view old_style_id,
    std::string_view new_style_id) -> style_reference_rewrite_status {
    auto status = style_reference_rewrite_status::no_match;
    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        for (const auto *child_name : {"w:basedOn", "w:next", "w:link"}) {
            status = combine_style_reference_rewrite_status(
                status, rewrite_style_reference_value_checked(
                            style.child(child_name).attribute("w:val"),
                            old_style_id, new_style_id));
            if (status == style_reference_rewrite_status::allocation_failure) {
                return status;
            }
        }
    }
    return status;
}

auto rewrite_style_metadata_references_in_styles(pugi::xml_node styles_root,
                                                 std::string_view old_style_id,
                                                 std::string_view new_style_id)
    -> bool {
    return rewrite_style_metadata_references_in_styles_checked(
               styles_root, old_style_id, new_style_id) ==
           style_reference_rewrite_status::rewritten;
}

void collect_style_reference_value(pugi::xml_attribute value_attribute,
                                   std::unordered_set<std::string> &style_ids) {
    if (value_attribute == pugi::xml_attribute{} ||
        value_attribute.value()[0] == '\0') {
        return;
    }

    style_ids.emplace(value_attribute.value());
}

void collect_style_reference_child(pugi::xml_node node,
                                   const char *properties_name,
                                   const char *style_name,
                                   std::unordered_set<std::string> &style_ids) {
    collect_style_reference_value(
        node.child(properties_name).child(style_name).attribute("w:val"),
        style_ids);
}

void collect_style_references_in_tree(
    pugi::xml_node node, std::unordered_set<std::string> &style_ids) {
    if (node == pugi::xml_node{}) {
        return;
    }

    for (auto current = node; current != pugi::xml_node{};
         current = featherdoc::detail::next_xml_node_preorder(node, current)) {
        const auto node_name = std::string_view{current.name()};
        if (node_name == "w:p") {
            collect_style_reference_child(current, "w:pPr", "w:pStyle",
                                          style_ids);
        } else if (node_name == "w:r") {
            collect_style_reference_child(current, "w:rPr", "w:rStyle",
                                          style_ids);
        } else if (node_name == "w:tbl") {
            collect_style_reference_child(current, "w:tblPr", "w:tblStyle",
                                          style_ids);
        }
    }
}

void append_style_dependency_ids(pugi::xml_node style,
                                 std::vector<std::string> &dependencies) {
    for (const auto *child_name : {"w:basedOn", "w:next", "w:link"}) {
        const auto value = style.child(child_name).attribute("w:val");
        if (value == pugi::xml_attribute{} || value.value()[0] == '\0') {
            continue;
        }
        dependencies.emplace_back(value.value());
    }
}

auto rewrite_style_references_in_tree_checked(pugi::xml_node node,
                                              std::string_view old_style_id,
                                              std::string_view new_style_id)
    -> style_reference_rewrite_status {
    if (node == pugi::xml_node{}) {
        return style_reference_rewrite_status::no_match;
    }

    auto status = style_reference_rewrite_status::no_match;
    for (auto current = node; current != pugi::xml_node{};
         current = featherdoc::detail::next_xml_node_preorder(node, current)) {
        const auto node_name = std::string_view{current.name()};
        if (node_name == "w:p") {
            status = combine_style_reference_rewrite_status(
                status,
                rewrite_style_reference_child_checked(
                    current, "w:pPr", "w:pStyle", old_style_id, new_style_id));
        } else if (node_name == "w:r") {
            status = combine_style_reference_rewrite_status(
                status,
                rewrite_style_reference_child_checked(
                    current, "w:rPr", "w:rStyle", old_style_id, new_style_id));
        } else if (node_name == "w:tbl") {
            status = combine_style_reference_rewrite_status(
                status, rewrite_style_reference_child_checked(
                            current, "w:tblPr", "w:tblStyle", old_style_id,
                            new_style_id));
        }
        if (status == style_reference_rewrite_status::allocation_failure) {
            return status;
        }
    }

    return status;
}

auto rewrite_style_references_in_tree(pugi::xml_node node,
                                      std::string_view old_style_id,
                                      std::string_view new_style_id) -> bool {
    return rewrite_style_references_in_tree_checked(node, old_style_id,
                                                    new_style_id) ==
           style_reference_rewrite_status::rewritten;
}
