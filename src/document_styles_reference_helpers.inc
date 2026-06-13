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

auto rewrite_style_reference_value(pugi::xml_attribute value_attribute,
                                   std::string_view old_style_id,
                                   std::string_view new_style_id) -> bool {
    if (value_attribute == pugi::xml_attribute{} ||
        std::string_view{value_attribute.value()} != old_style_id) {
        return false;
    }

    value_attribute.set_value(std::string{new_style_id}.c_str());
    return true;
}

auto rewrite_style_reference_child(pugi::xml_node node,
                                   const char *properties_name,
                                   const char *style_name,
                                   std::string_view old_style_id,
                                   std::string_view new_style_id) -> bool {
    return rewrite_style_reference_value(
        node.child(properties_name).child(style_name).attribute("w:val"),
        old_style_id, new_style_id);
}

auto rewrite_style_metadata_reference(pugi::xml_node style,
                                      const char *child_name,
                                      std::string_view old_style_id,
                                      std::string_view new_style_id) -> bool {
    return rewrite_style_reference_value(
        style.child(child_name).attribute("w:val"), old_style_id, new_style_id);
}

auto rewrite_style_metadata_references_in_styles(
    pugi::xml_node styles_root, std::string_view old_style_id,
    std::string_view new_style_id) -> bool {
    auto changed = false;
    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        changed = rewrite_style_metadata_reference(
                      style, "w:basedOn", old_style_id, new_style_id) ||
                  changed;
        changed = rewrite_style_metadata_reference(
                      style, "w:next", old_style_id, new_style_id) ||
                  changed;
        changed = rewrite_style_metadata_reference(
                      style, "w:link", old_style_id, new_style_id) ||
                  changed;
    }
    return changed;
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

    const auto node_name = std::string_view{node.name()};
    if (node_name == "w:p") {
        collect_style_reference_child(node, "w:pPr", "w:pStyle", style_ids);
    } else if (node_name == "w:r") {
        collect_style_reference_child(node, "w:rPr", "w:rStyle", style_ids);
    } else if (node_name == "w:tbl") {
        collect_style_reference_child(node, "w:tblPr", "w:tblStyle",
                                      style_ids);
    }

    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        collect_style_references_in_tree(child, style_ids);
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

auto rewrite_style_references_in_tree(pugi::xml_node node,
                                      std::string_view old_style_id,
                                      std::string_view new_style_id) -> bool {
    if (node == pugi::xml_node{}) {
        return false;
    }

    auto changed = false;
    const auto node_name = std::string_view{node.name()};
    if (node_name == "w:p") {
        changed = rewrite_style_reference_child(node, "w:pPr", "w:pStyle",
                                                old_style_id, new_style_id) ||
                  changed;
    } else if (node_name == "w:r") {
        changed = rewrite_style_reference_child(node, "w:rPr", "w:rStyle",
                                                old_style_id, new_style_id) ||
                  changed;
    } else if (node_name == "w:tbl") {
        changed = rewrite_style_reference_child(node, "w:tblPr", "w:tblStyle",
                                                old_style_id, new_style_id) ||
                  changed;
    }

    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        changed = rewrite_style_references_in_tree(child, old_style_id,
                                                   new_style_id) ||
                  changed;
    }

    return changed;
}
