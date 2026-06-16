struct block_bookmark_placeholder final {
    pugi::xml_node paragraph;
    pugi::xml_node bookmark_start;
    pugi::xml_node bookmark_end;
};

struct table_row_bookmark_placeholder final {
    pugi::xml_node table;
    pugi::xml_node row;
    std::size_t cell_count;
};

struct bookmark_block_placeholder final {
    pugi::xml_node container;
    pugi::xml_node start_paragraph;
    pugi::xml_node end_paragraph;
};

void collect_named_bookmark_starts(pugi::xml_node node, std::string_view bookmark_name,
                                   std::vector<pugi::xml_node> &bookmark_starts) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:bookmarkStart" &&
            std::string_view{child.attribute("w:name").value()} == bookmark_name) {
            bookmark_starts.push_back(child);
        }

        collect_named_bookmark_starts(child, bookmark_name, bookmark_starts);
    }
}

void collect_bookmark_starts_in_order(pugi::xml_node node,
                                      std::vector<pugi::xml_node> &bookmark_starts) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:bookmarkStart" &&
            child.attribute("w:name").value()[0] != '\0') {
            bookmark_starts.push_back(child);
        }

        collect_bookmark_starts_in_order(child, bookmark_starts);
    }
}

auto find_matching_bookmark_end(pugi::xml_node bookmark_start) -> pugi::xml_node {
    const auto bookmark_id = std::string_view{bookmark_start.attribute("w:id").value()};
    if (bookmark_id.empty()) {
        return {};
    }

    for (auto node = bookmark_start.next_sibling(); node != pugi::xml_node{};
         node = node.next_sibling()) {
        if (std::string_view{node.name()} == "w:bookmarkEnd" &&
            std::string_view{node.attribute("w:id").value()} == bookmark_id) {
            return node;
        }
    }

    return {};
}

auto next_node_in_document_order(pugi::xml_node node) -> pugi::xml_node {
    if (node == pugi::xml_node{}) {
        return {};
    }

    if (auto child = node.first_child(); child != pugi::xml_node{}) {
        return child;
    }

    for (auto current = node; current != pugi::xml_node{}; current = current.parent()) {
        if (auto sibling = current.next_sibling(); sibling != pugi::xml_node{}) {
            return sibling;
        }
    }

    return {};
}

auto find_top_level_table_ancestor(pugi::xml_node node, pugi::xml_node container)
    -> pugi::xml_node {
    for (auto current = node; current != pugi::xml_node{} && current != container;
         current = current.parent()) {
        if (std::string_view{current.name()} == "w:tbl" && current.parent() == container) {
            return current;
        }
    }

    return {};
}

auto find_top_level_table_index(pugi::xml_node container, pugi::xml_node table_node)
    -> std::optional<std::size_t> {
    if (container == pugi::xml_node{} || table_node == pugi::xml_node{} ||
        std::string_view{table_node.name()} != "w:tbl" || table_node.parent() != container) {
        return std::nullopt;
    }

    std::size_t table_index = 0U;
    for (auto child = container.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (child == table_node) {
            return table_index;
        }
        if (std::string_view{child.name()} == "w:tbl") {
            ++table_index;
        }
    }

    return std::nullopt;
}

auto find_first_top_level_table_after_node(pugi::xml_node container, pugi::xml_node node,
                                           pugi::xml_node stop_before = {})
    -> pugi::xml_node {
    for (auto current = next_node_in_document_order(node); current != pugi::xml_node{};
         current = next_node_in_document_order(current)) {
        if (current == stop_before) {
            break;
        }
        if (std::string_view{current.name()} == "w:tbl" && current.parent() == container) {
            return current;
        }
    }

    return {};
}

auto selector_uses_text_matching(
    const featherdoc::template_table_selector &selector) -> bool {
    return selector.after_paragraph_text.has_value() ||
           !selector.header_cell_texts.empty();
}

auto validate_template_table_selector(
    featherdoc::document_error_info &last_error_info, std::string_view entry_name,
    const featherdoc::template_table_selector &selector) -> bool {
    const auto uses_direct_target =
        selector.table_index.has_value() || selector.bookmark_name.has_value();
    const auto uses_text_target = selector_uses_text_matching(selector);

    if (selector.table_index.has_value() && selector.bookmark_name.has_value()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector cannot combine table_index and "
                       "bookmark_name",
                       std::string{entry_name});
        return false;
    }

    if (selector.bookmark_name.has_value() && selector.bookmark_name->empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector bookmark_name must not be empty",
                       std::string{entry_name});
        return false;
    }

    if (selector.after_paragraph_text.has_value() &&
        selector.after_paragraph_text->empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector after_paragraph_text must not be "
                       "empty",
                       std::string{entry_name});
        return false;
    }

    if (uses_direct_target && uses_text_target) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector cannot combine table_index or "
                       "bookmark_name with after_paragraph_text or "
                       "header_cell_texts",
                       std::string{entry_name});
        return false;
    }

    if (!uses_direct_target && !uses_text_target) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector must specify table_index, "
                       "bookmark_name, after_paragraph_text, or "
                       "header_cell_texts",
                       std::string{entry_name});
        return false;
    }

    if (selector.occurrence != 0U && !uses_text_target) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector occurrence requires "
                       "after_paragraph_text or header_cell_texts",
                       std::string{entry_name});
        return false;
    }

    if (selector.header_row_index != 0U && selector.header_cell_texts.empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector header_row_index requires "
                       "header_cell_texts",
                       std::string{entry_name});
        return false;
    }

    return true;
}

auto find_top_level_table_by_index(pugi::xml_node container, std::size_t table_index)
    -> pugi::xml_node {
    std::size_t current_table_index = 0U;
    for (auto child = container.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:tbl") {
            continue;
        }
        if (current_table_index == table_index) {
            return child;
        }
        ++current_table_index;
    }

    return {};
}

auto push_unique_table_node(std::vector<pugi::xml_node> &matches,
                            pugi::xml_node table_node) -> void {
    if (table_node == pugi::xml_node{}) {
        return;
    }
    if (std::find(matches.begin(), matches.end(), table_node) == matches.end()) {
        matches.push_back(table_node);
    }
}

auto top_level_paragraph_contains_text(pugi::xml_node paragraph_node,
                                       std::string_view text) -> bool {
    if (paragraph_node == pugi::xml_node{} ||
        std::string_view{paragraph_node.name()} != "w:p" || text.empty()) {
        return false;
    }

    return summarize_part_paragraph_node(paragraph_node, 0U).text.find(text) !=
           std::string::npos;
}

auto table_row_cell_texts(pugi::xml_node container, pugi::xml_node table_node,
                          std::size_t row_index)
    -> std::optional<std::vector<std::string>> {
    if (container == pugi::xml_node{} || table_node == pugi::xml_node{} ||
        std::string_view{table_node.name()} != "w:tbl" ||
        table_node.parent() != container) {
        return std::nullopt;
    }

    auto table = featherdoc::Table{container, table_node};
    auto row = table.rows();
    for (std::size_t current_row_index = 0U; row.has_next();
         ++current_row_index, row.next()) {
        if (current_row_index != row_index) {
            continue;
        }

        auto cell_texts = std::vector<std::string>{};
        auto cell = row.cells();
        while (cell.has_next()) {
            cell_texts.push_back(cell.get_text());
            cell.next();
        }
        return cell_texts;
    }

    return std::nullopt;
}

auto top_level_table_matches_selector_header(
    pugi::xml_node container, pugi::xml_node table_node,
    const featherdoc::template_table_selector &selector) -> bool {
    if (selector.header_cell_texts.empty()) {
        return true;
    }

    const auto header_cells =
        table_row_cell_texts(container, table_node, selector.header_row_index);
    return header_cells.has_value() &&
           *header_cells == selector.header_cell_texts;
}

auto find_first_matching_top_level_table_after_node(
    pugi::xml_node container, pugi::xml_node node,
    const featherdoc::template_table_selector &selector) -> pugi::xml_node {
    for (auto current = next_node_in_document_order(node); current != pugi::xml_node{};
         current = next_node_in_document_order(current)) {
        if (std::string_view{current.name()} == "w:tbl" &&
            current.parent() == container &&
            top_level_table_matches_selector_header(container, current, selector)) {
            return current;
        }
    }

    return {};
}

auto find_table_index_by_selector_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name,
    const featherdoc::template_table_selector &selector)
    -> std::optional<std::size_t> {
    if (!validate_template_table_selector(last_error_info, entry_name, selector)) {
        return std::nullopt;
    }

    const auto container = template_part_block_container(document);
    if (container == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       std::string{entry_name});
        return std::nullopt;
    }

    if (selector.table_index.has_value()) {
        if (find_top_level_table_by_index(container, *selector.table_index) ==
            pugi::xml_node{}) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "table index '" +
                               std::to_string(*selector.table_index) +
                               "' is out of range in " + std::string{entry_name},
                           std::string{entry_name});
            return std::nullopt;
        }

        last_error_info.clear();
        return selector.table_index;
    }

    if (selector.bookmark_name.has_value()) {
        return featherdoc::find_table_index_by_bookmark_in_part(
            last_error_info, document, entry_name, *selector.bookmark_name);
    }

    auto matches = std::vector<pugi::xml_node>{};
    if (selector.after_paragraph_text.has_value()) {
        for (auto child = container.first_child(); child != pugi::xml_node{};
             child = child.next_sibling()) {
            if (!top_level_paragraph_contains_text(child,
                                                   *selector.after_paragraph_text)) {
                continue;
            }

            const auto table =
                selector.header_cell_texts.empty()
                    ? find_first_top_level_table_after_node(container, child)
                    : find_first_matching_top_level_table_after_node(container,
                                                                     child,
                                                                     selector);
            push_unique_table_node(matches, table);
        }
    } else {
        for (auto child = container.first_child(); child != pugi::xml_node{};
             child = child.next_sibling()) {
            if (std::string_view{child.name()} == "w:tbl" &&
                top_level_table_matches_selector_header(container, child, selector)) {
                matches.push_back(child);
            }
        }
    }

    if (matches.empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector did not resolve to a table in " +
                           std::string{entry_name},
                       std::string{entry_name});
        return std::nullopt;
    }

    if (selector.occurrence >= matches.size()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            "template table selector occurrence '" +
                std::to_string(selector.occurrence) + "' is out of range for " +
                std::to_string(matches.size()) + " match(es) in " +
                std::string{entry_name},
            std::string{entry_name});
        return std::nullopt;
    }

    const auto resolved_index =
        find_top_level_table_index(container, matches[selector.occurrence]);
    if (!resolved_index.has_value()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table selector resolved an invalid table node in " +
                           std::string{entry_name},
                       std::string{entry_name});
        return std::nullopt;
    }

    last_error_info.clear();
    return resolved_index;
}

auto find_matching_bookmark_end_in_document_order(pugi::xml_node bookmark_start)
    -> pugi::xml_node {
    const auto bookmark_id = std::string_view{bookmark_start.attribute("w:id").value()};
    if (bookmark_id.empty()) {
        return {};
    }

    for (auto node = next_node_in_document_order(bookmark_start); node != pugi::xml_node{};
         node = next_node_in_document_order(node)) {
        if (std::string_view{node.name()} == "w:bookmarkEnd" &&
            std::string_view{node.attribute("w:id").value()} == bookmark_id) {
            return node;
        }
    }

    return {};
}

auto replace_bookmark_range(pugi::xml_node bookmark_start, std::string_view replacement)
    -> bool {
    auto parent = bookmark_start.parent();
    if (bookmark_start == pugi::xml_node{} || parent == pugi::xml_node{}) {
        return false;
    }

    const auto bookmark_end = find_matching_bookmark_end(bookmark_start);
    if (bookmark_end == pugi::xml_node{}) {
        return false;
    }

    for (auto node = bookmark_start.next_sibling();
         node != pugi::xml_node{} && node != bookmark_end;) {
        const auto next = node.next_sibling();
        parent.remove_child(node);
        node = next;
    }

    if (!replacement.empty()) {
        auto replacement_run = parent.insert_child_before("w:r", bookmark_end);
        if (replacement_run == pugi::xml_node{}) {
            return false;
        }
        return featherdoc::detail::set_plain_text_run_content(replacement_run, replacement);
    }

    return true;
}

auto is_ignorable_block_placeholder_node(pugi::xml_node node) -> bool {
    const auto name = std::string_view{node.name()};
    return name == "w:pPr" || name == "w:proofErr" || name == "w:bookmarkStart" ||
           name == "w:bookmarkEnd" || name == "w:permStart" || name == "w:permEnd" ||
           name == "w:commentRangeStart" || name == "w:commentRangeEnd" ||
           name == "w:moveFromRangeStart" || name == "w:moveFromRangeEnd" ||
           name == "w:moveToRangeStart" || name == "w:moveToRangeEnd";
}

auto paragraph_is_block_placeholder(pugi::xml_node paragraph, pugi::xml_node bookmark_start,
                                    pugi::xml_node bookmark_end) -> bool {
    if (paragraph == pugi::xml_node{} || bookmark_start == pugi::xml_node{} ||
        bookmark_end == pugi::xml_node{}) {
        return false;
    }

    bool inside_placeholder = false;
    for (auto child = paragraph.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (child == bookmark_start) {
            inside_placeholder = true;
            continue;
        }

        if (child == bookmark_end) {
            inside_placeholder = false;
            continue;
        }

        if (inside_placeholder) {
            continue;
        }

        if (!is_ignorable_block_placeholder_node(child)) {
            return false;
        }
    }

    return !inside_placeholder;
}

auto paragraph_is_bookmark_marker(pugi::xml_node paragraph, pugi::xml_node marker) -> bool {
    if (paragraph == pugi::xml_node{} || marker == pugi::xml_node{} || marker.parent() != paragraph) {
        return false;
    }

    for (auto child = paragraph.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (child == marker) {
            continue;
        }

        if (!is_ignorable_block_placeholder_node(child)) {
            return false;
        }
    }

    return true;
}

auto classify_bookmark_start_kind(pugi::xml_node bookmark_start)
    -> featherdoc::bookmark_kind {
    if (bookmark_start == pugi::xml_node{} ||
        std::string_view{bookmark_start.name()} != "w:bookmarkStart") {
        return featherdoc::bookmark_kind::malformed;
    }

    const auto start_parent = bookmark_start.parent();
    if (start_parent == pugi::xml_node{} || std::string_view{start_parent.name()} != "w:p") {
        return find_matching_bookmark_end_in_document_order(bookmark_start) != pugi::xml_node{}
                   ? featherdoc::bookmark_kind::text
                   : featherdoc::bookmark_kind::malformed;
    }

    if (const auto bookmark_end = find_matching_bookmark_end(bookmark_start);
        bookmark_end != pugi::xml_node{} && bookmark_end.parent() == start_parent) {
        if (!paragraph_is_block_placeholder(start_parent, bookmark_start, bookmark_end)) {
            return featherdoc::bookmark_kind::text;
        }

        const auto cell = start_parent.parent();
        const auto row = cell.parent();
        const auto table = row.parent();
        if (cell != pugi::xml_node{} && std::string_view{cell.name()} == "w:tc" &&
            row != pugi::xml_node{} && std::string_view{row.name()} == "w:tr" &&
            table != pugi::xml_node{} && std::string_view{table.name()} == "w:tbl") {
            return featherdoc::bookmark_kind::table_rows;
        }

        return featherdoc::bookmark_kind::block;
    }

    if (const auto bookmark_end = find_matching_bookmark_end_in_document_order(bookmark_start);
        bookmark_end != pugi::xml_node{}) {
        const auto end_parent = bookmark_end.parent();
        if (end_parent != pugi::xml_node{} && std::string_view{end_parent.name()} == "w:p" &&
            end_parent != start_parent && end_parent.parent() == start_parent.parent() &&
            paragraph_is_bookmark_marker(start_parent, bookmark_start) &&
            paragraph_is_bookmark_marker(end_parent, bookmark_end)) {
            return featherdoc::bookmark_kind::block_range;
        }

        return featherdoc::bookmark_kind::text;
    }

    return featherdoc::bookmark_kind::malformed;
}

auto summarize_bookmarks_in_part(featherdoc::document_error_info &last_error_info,
                                 pugi::xml_document &document)
    -> std::vector<featherdoc::bookmark_summary> {
    std::vector<pugi::xml_node> bookmark_starts;
    collect_bookmark_starts_in_order(document, bookmark_starts);

    std::vector<featherdoc::bookmark_summary> summaries;
    std::unordered_map<std::string, std::size_t> summary_indexes;
    for (const auto bookmark_start : bookmark_starts) {
        const auto bookmark_name =
            std::string_view{bookmark_start.attribute("w:name").value()};
        if (bookmark_name.empty()) {
            continue;
        }

        const auto kind = classify_bookmark_start_kind(bookmark_start);
        const auto [index_it, inserted] =
            summary_indexes.emplace(std::string{bookmark_name}, summaries.size());
        if (inserted) {
            summaries.push_back(
                {std::string{bookmark_name}, 0U, featherdoc::bookmark_kind::text});
        }

        auto &summary = summaries[index_it->second];
        ++summary.occurrence_count;
        if (summary.occurrence_count == 1U) {
            summary.kind = kind;
            continue;
        }

        if (summary.kind != kind) {
            summary.kind = featherdoc::bookmark_kind::mixed;
        }
    }

    last_error_info.clear();
    return summaries;
}

auto find_bookmark_in_part(featherdoc::document_error_info &last_error_info,
                           pugi::xml_document &document, std::string_view entry_name,
                           std::string_view bookmark_name)
    -> std::optional<featherdoc::bookmark_summary> {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty when reading bookmark metadata",
                       std::string{entry_name});
        return std::nullopt;
    }

    const auto summaries = summarize_bookmarks_in_part(last_error_info, document);
    for (const auto &summary : summaries) {
        if (summary.bookmark_name == bookmark_name) {
            last_error_info.clear();
            return summary;
        }
    }

    set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                   "bookmark name '" + std::string{bookmark_name} +
                       "' was not found in " + std::string{entry_name},
                   std::string{entry_name});
    return std::nullopt;
}
