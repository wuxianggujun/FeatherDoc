void clear_node_children(pugi::xml_node node) {
    for (auto child = node.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        node.remove_child(child);
        child = next;
    }
}

auto ensure_content_control_content_node(pugi::xml_node content_control)
    -> pugi::xml_node {
    auto content = content_control.child("w:sdtContent");
    if (content != pugi::xml_node{}) {
        return content;
    }

    return featherdoc::detail::checked_append_xml_element(content_control,
                                                          "w:sdtContent");
}

auto append_text_table_cell(pugi::xml_node parent, std::string_view text)
    -> bool {
    auto cell =
        featherdoc::detail::checked_append_xml_element(parent, "w:tc");
    if (cell == pugi::xml_node{}) {
        return false;
    }

    return featherdoc::detail::append_plain_text_paragraph(cell, text);
}

void clear_content_control_placeholder_flag(pugi::xml_node content_control) {
    if (auto properties = content_control.child("w:sdtPr");
        properties != pugi::xml_node{}) {
        for (auto placeholder = properties.child("w:showingPlcHdr");
             placeholder != pugi::xml_node{};) {
            const auto next = placeholder.next_sibling("w:showingPlcHdr");
            properties.remove_child(placeholder);
            placeholder = next;
        }
    }
}

auto append_content_control_plain_text(
    pugi::xml_node content, featherdoc::content_control_kind kind,
    std::string_view replacement) -> bool {
    if (content == pugi::xml_node{}) {
        return false;
    }

    switch (kind) {
    case featherdoc::content_control_kind::run: {
        auto run =
            featherdoc::detail::checked_append_xml_element(content, "w:r");
        return run != pugi::xml_node{} &&
               featherdoc::detail::set_plain_text_run_content(run,
                                                               replacement);
    }
    case featherdoc::content_control_kind::table_row: {
        auto row =
            featherdoc::detail::checked_append_xml_element(content, "w:tr");
        return row != pugi::xml_node{} &&
               append_text_table_cell(row, replacement);
    }
    case featherdoc::content_control_kind::table_cell:
        return append_text_table_cell(content, replacement);
    case featherdoc::content_control_kind::block:
    case featherdoc::content_control_kind::unknown:
        return featherdoc::detail::append_plain_text_paragraph(content,
                                                               replacement);
    }

    return false;
}

auto rewrite_content_control_plain_text(pugi::xml_node content_control,
                                        std::string_view replacement)
    -> bool {
    if (content_control == pugi::xml_node{}) {
        return false;
    }

    auto content = ensure_content_control_content_node(content_control);
    if (content == pugi::xml_node{}) {
        return false;
    }

    const auto kind = content_control_kind_from_node(content_control);
    clear_node_children(content);
    if (!append_content_control_plain_text(content, kind, replacement)) {
        return false;
    }

    clear_content_control_placeholder_flag(content_control);
    return true;
}
