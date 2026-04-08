#include "featherdoc.hpp"
#include "image_helpers.hpp"
#include "xml_helpers.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>
#include <utility>

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};
constexpr auto document_relationships_xml_entry =
    std::string_view{"word/_rels/document.xml.rels"};
constexpr auto unavailable_template_part_detail =
    std::string_view{"template part is not available"};

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

auto set_last_error(featherdoc::document_error_info &error_info,
                    std::error_code code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    error_info.code = code;
    error_info.detail = std::move(detail);
    error_info.entry_name = std::move(entry_name);
    error_info.xml_offset = std::move(xml_offset);
    return code;
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    featherdoc::document_errc code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    return set_last_error(error_info, featherdoc::make_error_code(code),
                          std::move(detail), std::move(entry_name), std::move(xml_offset));
}

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

        auto replacement_text = replacement_run.append_child("w:t");
        if (replacement_text == pugi::xml_node{}) {
            return false;
        }

        const std::string replacement_buffer{replacement};
        featherdoc::detail::update_xml_space_attribute(replacement_text,
                                                       replacement_buffer.c_str());
        return replacement_text.text().set(replacement_buffer.c_str());
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

auto collect_block_bookmark_placeholders(featherdoc::document_error_info &last_error_info,
                                         pugi::xml_document &document,
                                         std::string_view entry_name,
                                         std::string_view bookmark_name,
                                         std::vector<block_bookmark_placeholder> &placeholders)
    -> bool {
    std::vector<pugi::xml_node> bookmark_starts;
    collect_named_bookmark_starts(document, bookmark_name, bookmark_starts);

    for (const auto bookmark_start : bookmark_starts) {
        const auto paragraph = bookmark_start.parent();
        if (paragraph == pugi::xml_node{} || std::string_view{paragraph.name()} != "w:p") {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "block bookmark replacement requires the bookmark to live directly "
                           "inside a paragraph",
                           std::string{entry_name});
            return false;
        }

        const auto bookmark_end = find_matching_bookmark_end(bookmark_start);
        if (bookmark_end == pugi::xml_node{} || bookmark_end.parent() != paragraph) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "block bookmark replacement requires matching bookmark markers "
                           "inside the same paragraph",
                           std::string{entry_name});
            return false;
        }

        if (!paragraph_is_block_placeholder(paragraph, bookmark_start, bookmark_end)) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "block bookmark replacement requires the target bookmark to occupy "
                           "its own paragraph",
                           std::string{entry_name});
            return false;
        }

        placeholders.push_back({paragraph, bookmark_start, bookmark_end});
    }

    return true;
}

auto collect_bookmark_block_placeholders(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name,
    std::vector<bookmark_block_placeholder> &placeholders) -> bool {
    std::vector<pugi::xml_node> bookmark_starts;
    collect_named_bookmark_starts(document, bookmark_name, bookmark_starts);

    for (const auto bookmark_start : bookmark_starts) {
        const auto start_paragraph = bookmark_start.parent();
        if (start_paragraph == pugi::xml_node{} ||
            std::string_view{start_paragraph.name()} != "w:p") {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark block visibility requires the start marker to live "
                           "directly inside a paragraph",
                           std::string{entry_name});
            return false;
        }

        const auto bookmark_end = find_matching_bookmark_end_in_document_order(bookmark_start);
        if (bookmark_end == pugi::xml_node{}) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark block visibility requires a matching bookmark end marker",
                           std::string{entry_name});
            return false;
        }

        const auto end_paragraph = bookmark_end.parent();
        if (end_paragraph == pugi::xml_node{} ||
            std::string_view{end_paragraph.name()} != "w:p") {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark block visibility requires the end marker to live "
                           "directly inside a paragraph",
                           std::string{entry_name});
            return false;
        }

        if (start_paragraph == end_paragraph) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark block visibility requires start and end markers to live "
                           "in separate paragraphs",
                           std::string{entry_name});
            return false;
        }

        const auto container = start_paragraph.parent();
        if (container == pugi::xml_node{} || end_paragraph.parent() != container) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark block visibility requires both marker paragraphs to "
                           "share the same parent container",
                           std::string{entry_name});
            return false;
        }

        if (!paragraph_is_bookmark_marker(start_paragraph, bookmark_start) ||
            !paragraph_is_bookmark_marker(end_paragraph, bookmark_end)) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark block visibility requires both markers to occupy "
                           "their own paragraphs",
                           std::string{entry_name});
            return false;
        }

        placeholders.push_back({container, start_paragraph, end_paragraph});
    }

    return true;
}

auto count_named_children(pugi::xml_node parent, std::string_view child_name)
    -> std::size_t {
    std::size_t count = 0U;
    for (auto child = parent.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == child_name) {
            ++count;
        }
    }
    return count;
}

auto rewrite_paragraph_plain_text(pugi::xml_node paragraph, std::string_view text) -> bool {
    if (paragraph == pugi::xml_node{}) {
        return false;
    }

    pugi::xml_document run_properties_storage;
    pugi::xml_node run_properties;
    for (auto child = paragraph.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:r") {
            const auto source_run_properties = child.child("w:rPr");
            if (source_run_properties != pugi::xml_node{}) {
                run_properties = run_properties_storage.append_copy(source_run_properties);
                break;
            }
        }
    }

    for (auto child = paragraph.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        if (std::string_view{child.name()} != "w:pPr") {
            paragraph.remove_child(child);
        }
        child = next;
    }

    if (text.empty()) {
        return true;
    }

    auto run = paragraph.append_child("w:r");
    if (run == pugi::xml_node{}) {
        return false;
    }

    if (run_properties != pugi::xml_node{} && run.append_copy(run_properties) == pugi::xml_node{}) {
        return false;
    }

    auto text_node = run.append_child("w:t");
    if (text_node == pugi::xml_node{}) {
        return false;
    }

    const std::string text_buffer{text};
    featherdoc::detail::update_xml_space_attribute(text_node, text_buffer.c_str());
    return text_node.text().set(text_buffer.c_str());
}

auto rewrite_table_cell_plain_text(pugi::xml_node cell, std::string_view text) -> bool {
    if (cell == pugi::xml_node{}) {
        return false;
    }

    pugi::xml_node paragraph;
    for (auto child = cell.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        const auto child_name = std::string_view{child.name()};
        if (child_name == "w:tcPr") {
            child = next;
            continue;
        }

        if (child_name == "w:p" && paragraph == pugi::xml_node{}) {
            paragraph = child;
            child = next;
            continue;
        }

        cell.remove_child(child);
        child = next;
    }

    if (paragraph == pugi::xml_node{}) {
        paragraph = featherdoc::detail::append_paragraph_node(cell);
        if (paragraph == pugi::xml_node{}) {
            return false;
        }
    }

    return rewrite_paragraph_plain_text(paragraph, text);
}

auto collect_table_row_bookmark_placeholders(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name,
    std::string_view bookmark_name, std::vector<table_row_bookmark_placeholder> &placeholders)
    -> bool {
    std::vector<block_bookmark_placeholder> block_placeholders;
    if (!collect_block_bookmark_placeholders(last_error_info, document, entry_name,
                                             bookmark_name,
                                             block_placeholders)) {
        return false;
    }

    for (const auto &placeholder : block_placeholders) {
        const auto cell = placeholder.paragraph.parent();
        if (cell == pugi::xml_node{} || std::string_view{cell.name()} != "w:tc") {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "table row replacement requires the bookmark paragraph to live "
                           "inside a table cell",
                           std::string{entry_name});
            return false;
        }

        const auto row = cell.parent();
        if (row == pugi::xml_node{} || std::string_view{row.name()} != "w:tr") {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "table row replacement requires the bookmark paragraph to live "
                           "inside a table row",
                           std::string{entry_name});
            return false;
        }

        const auto table = row.parent();
        if (table == pugi::xml_node{} || std::string_view{table.name()} != "w:tbl") {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "table row replacement requires the bookmark row to belong to a "
                           "table",
                           std::string{entry_name});
            return false;
        }

        for (const auto &existing : placeholders) {
            if (existing.row == row) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::invalid_argument),
                               "table row replacement requires each template row to contain "
                               "the target bookmark at most once",
                               std::string{entry_name});
                return false;
            }
        }

        const auto cell_count = count_named_children(row, "w:tc");
        if (cell_count == 0U) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "table row replacement requires the template row to contain at "
                           "least one cell",
                           std::string{entry_name});
            return false;
        }

        placeholders.push_back({table, row, cell_count});
    }

    return true;
}

auto replace_placeholder_with_table_rows(
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name, table_row_bookmark_placeholder placeholder,
    const std::vector<std::vector<std::string>> &rows) -> bool {
    if (placeholder.table == pugi::xml_node{} || placeholder.row == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the placeholder row for table row replacement",
                       std::string{entry_name});
        return false;
    }

    if (placeholder.row.parent() != placeholder.table) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the placeholder row parent table",
                       std::string{entry_name});
        return false;
    }

    for (const auto &row_values : rows) {
        if (row_values.size() != placeholder.cell_count) {
            set_last_error(
                last_error_info, std::make_error_code(std::errc::invalid_argument),
                "replacement table row cell count must exactly match the template row cell "
                "count",
                std::string{entry_name});
            return false;
        }
    }

    for (const auto &row_values : rows) {
        auto row_copy = placeholder.table.insert_copy_before(placeholder.row, placeholder.row);
        if (row_copy == pugi::xml_node{}) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to clone the template row before the bookmark row",
                           std::string{entry_name});
            return false;
        }

        std::size_t cell_index = 0U;
        for (auto cell = row_copy.child("w:tc"); cell != pugi::xml_node{};
             cell = featherdoc::detail::next_named_sibling(cell, "w:tc"), ++cell_index) {
            if (!rewrite_table_cell_plain_text(cell, row_values[cell_index])) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to rewrite a cloned template row cell",
                               std::string{entry_name});
                return false;
            }
        }
    }

    if (!placeholder.table.remove_child(placeholder.row)) {
        set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                       "failed to remove the bookmark placeholder row after table row "
                       "replacement",
                       std::string{entry_name});
        return false;
    }

    return true;
}

auto insert_table_before_placeholder(
    featherdoc::document_error_info &last_error_info, std::string_view entry_name,
    pugi::xml_node placeholder_paragraph,
    const std::vector<std::vector<std::string>> &rows) -> bool {
    auto parent = placeholder_paragraph.parent();
    if (placeholder_paragraph == pugi::xml_node{} || parent == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the placeholder paragraph for table replacement",
                       std::string{entry_name});
        return false;
    }

    auto table_node = parent.insert_child_before("w:tbl", placeholder_paragraph);
    if (table_node == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                       "failed to insert the replacement table before the bookmark paragraph",
                       std::string{entry_name});
        return false;
    }

    featherdoc::Table table(parent, table_node);
    for (const auto &row_values : rows) {
        auto row = table.append_row(row_values.size());
        if (!row.has_next()) {
            set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                           "failed to append a replacement table row",
                           std::string{entry_name});
            return false;
        }

        auto cell = row.cells();
        for (const auto &cell_text : row_values) {
            if (!cell.has_next()) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to resolve a replacement table cell",
                               std::string{entry_name});
                return false;
            }

            if (!cell_text.empty() && !cell.paragraphs().add_run(cell_text).has_next()) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to append replacement table text",
                               std::string{entry_name});
                return false;
            }

            cell.next();
        }
    }

    if (!parent.remove_child(placeholder_paragraph)) {
        set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                       "failed to remove the bookmark placeholder paragraph after table "
                       "replacement",
                       std::string{entry_name});
        return false;
    }

    return true;
}

auto replace_placeholder_with_plain_text_paragraphs(
    featherdoc::document_error_info &last_error_info, std::string_view entry_name,
    pugi::xml_node placeholder_paragraph,
    const std::vector<std::string> &paragraphs) -> bool {
    auto parent = placeholder_paragraph.parent();
    if (placeholder_paragraph == pugi::xml_node{} || parent == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the placeholder paragraph for paragraph "
                       "replacement",
                       std::string{entry_name});
        return false;
    }

    for (const auto &paragraph_text : paragraphs) {
        if (!featherdoc::detail::insert_plain_text_paragraph(
                parent, placeholder_paragraph, paragraph_text)) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to insert a replacement paragraph before the bookmark "
                           "placeholder",
                           std::string{entry_name});
            return false;
        }
    }

    if (!parent.remove_child(placeholder_paragraph)) {
        set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                       "failed to remove the bookmark placeholder paragraph after paragraph "
                       "replacement",
                       std::string{entry_name});
        return false;
    }

    return true;
}

auto replace_bookmark_text_in_part(featherdoc::document_error_info &last_error_info,
                                   pugi::xml_document &document, std::string_view entry_name,
                                   std::string_view bookmark_name,
                                   std::string_view replacement) -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    std::vector<pugi::xml_node> bookmark_starts;
    collect_named_bookmark_starts(document, bookmark_name, bookmark_starts);

    std::size_t replaced = 0U;
    for (const auto bookmark_start : bookmark_starts) {
        if (replace_bookmark_range(bookmark_start, replacement)) {
            ++replaced;
        }
    }

    last_error_info.clear();
    return replaced;
}

auto fill_bookmarks_in_part(featherdoc::document_error_info &last_error_info,
                            pugi::xml_document &document, std::string_view entry_name,
                            std::span<const featherdoc::bookmark_text_binding> bindings)
    -> featherdoc::bookmark_fill_result {
    featherdoc::bookmark_fill_result result;
    result.requested = bindings.size();

    std::unordered_set<std::string> seen_bookmarks;
    for (const auto &binding : bindings) {
        if (binding.bookmark_name.empty()) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark binding name must not be empty",
                           std::string{entry_name});
            return result;
        }

        const auto [_, inserted] = seen_bookmarks.emplace(binding.bookmark_name);
        if (!inserted) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "duplicate bookmark binding name: " + binding.bookmark_name,
                           std::string{entry_name});
            return result;
        }
    }

    for (const auto &binding : bindings) {
        const auto replaced = replace_bookmark_text_in_part(
            last_error_info, document, entry_name, binding.bookmark_name, binding.text);
        if (replaced == 0U) {
            result.missing_bookmarks.push_back(binding.bookmark_name);
            continue;
        }

        ++result.matched;
        result.replaced += replaced;
    }

    last_error_info.clear();
    return result;
}

auto set_bookmark_block_visibility_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name, bool visible)
    -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    std::vector<bookmark_block_placeholder> placeholders;
    if (!collect_bookmark_block_placeholders(last_error_info, document, entry_name,
                                             bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        last_error_info.clear();
        return 0U;
    }

    std::size_t replaced = 0U;
    for (auto it = placeholders.rbegin(); it != placeholders.rend(); ++it) {
        auto placeholder = *it;
        if (placeholder.container == pugi::xml_node{} ||
            placeholder.start_paragraph.parent() != placeholder.container ||
            placeholder.end_paragraph.parent() != placeholder.container) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "failed to resolve the bookmark block marker paragraphs",
                           std::string{entry_name});
            return 0U;
        }

        if (visible) {
            if (!placeholder.container.remove_child(placeholder.end_paragraph) ||
                !placeholder.container.remove_child(placeholder.start_paragraph)) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to remove bookmark block marker paragraphs",
                               std::string{entry_name});
                return 0U;
            }
        } else {
            auto node = placeholder.start_paragraph;
            bool removed_end = false;
            while (node != pugi::xml_node{}) {
                const auto next = node.next_sibling();
                const bool is_end = node == placeholder.end_paragraph;
                if (!placeholder.container.remove_child(node)) {
                    set_last_error(last_error_info,
                                   std::make_error_code(std::errc::not_enough_memory),
                                   "failed to remove nodes inside the bookmark block range",
                                   std::string{entry_name});
                    return 0U;
                }
                if (is_end) {
                    removed_end = true;
                    break;
                }
                node = next;
            }

            if (!removed_end) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::invalid_argument),
                               "failed to locate the bookmark block end marker while "
                               "removing the block",
                               std::string{entry_name});
                return 0U;
            }
        }

        ++replaced;
    }

    last_error_info.clear();
    return replaced;
}

auto apply_bookmark_block_visibility_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name,
    std::span<const featherdoc::bookmark_block_visibility_binding> bindings)
    -> featherdoc::bookmark_block_visibility_result {
    featherdoc::bookmark_block_visibility_result result;
    result.requested = bindings.size();

    std::unordered_set<std::string> seen_bookmarks;
    for (const auto &binding : bindings) {
        if (binding.bookmark_name.empty()) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark block visibility binding name must not be empty",
                           std::string{entry_name});
            return result;
        }

        const auto [_, inserted] = seen_bookmarks.emplace(binding.bookmark_name);
        if (!inserted) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "duplicate bookmark block visibility binding name: " +
                               binding.bookmark_name,
                           std::string{entry_name});
            return result;
        }
    }

    for (const auto &binding : bindings) {
        const auto replaced = set_bookmark_block_visibility_in_part(
            last_error_info, document, entry_name, binding.bookmark_name, binding.visible);
        if (replaced == 0U) {
            result.missing_bookmarks.push_back(binding.bookmark_name);
            continue;
        }

        ++result.matched;
        if (binding.visible) {
            result.kept += replaced;
        } else {
            result.removed += replaced;
        }
    }

    last_error_info.clear();
    return result;
}

auto replace_bookmark_with_paragraphs_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name,
    const std::vector<std::string> &paragraphs) -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_block_bookmark_placeholders(last_error_info, document, entry_name,
                                             bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        last_error_info.clear();
        return 0U;
    }

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        if (!replace_placeholder_with_plain_text_paragraphs(
                last_error_info, entry_name, placeholder.paragraph, paragraphs)) {
            return 0U;
        }
        ++replaced;
    }

    last_error_info.clear();
    return replaced;
}

auto remove_bookmark_block_in_part(featherdoc::document_error_info &last_error_info,
                                   pugi::xml_document &document,
                                   std::string_view entry_name,
                                   std::string_view bookmark_name)
    -> std::size_t {
    return replace_bookmark_with_paragraphs_in_part(last_error_info, document, entry_name,
                                                    bookmark_name, {});
}

auto replace_bookmark_with_table_rows_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name,
    const std::vector<std::vector<std::string>> &rows) -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    std::vector<table_row_bookmark_placeholder> placeholders;
    if (!collect_table_row_bookmark_placeholders(last_error_info, document, entry_name,
                                                 bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        last_error_info.clear();
        return 0U;
    }

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        if (!replace_placeholder_with_table_rows(last_error_info, entry_name, placeholder,
                                                 rows)) {
            return 0U;
        }
        ++replaced;
    }

    last_error_info.clear();
    return replaced;
}

auto replace_bookmark_with_table_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name,
    const std::vector<std::vector<std::string>> &rows) -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    if (rows.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "replacement table must contain at least one row",
                       std::string{entry_name});
        return 0U;
    }

    for (const auto &row : rows) {
        if (row.empty()) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "replacement table rows must contain at least one cell",
                           std::string{entry_name});
            return 0U;
        }
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_block_bookmark_placeholders(last_error_info, document, entry_name,
                                             bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        last_error_info.clear();
        return 0U;
    }

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        if (!insert_table_before_placeholder(last_error_info, entry_name,
                                             placeholder.paragraph, rows)) {
            return 0U;
        }
        ++replaced;
    }

    last_error_info.clear();
    return replaced;
}
} // namespace

namespace featherdoc {

TemplatePart::TemplatePart() = default;

TemplatePart::TemplatePart(Document *owner, pugi::xml_document *xml_document,
                           document_error_info *last_error_info, std::string entry_name)
    : owner(owner), xml_document(xml_document), last_error_info(last_error_info),
      entry_name_storage(std::move(entry_name)) {}

TemplatePart::operator bool() const noexcept { return this->xml_document != nullptr; }

std::string_view TemplatePart::entry_name() const noexcept {
    return this->entry_name_storage;
}

std::size_t TemplatePart::replace_bookmark_text(const std::string &bookmark_name,
                                                const std::string &replacement) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_bookmark_text_in_part(*this->last_error_info, *this->xml_document,
                                         this->entry_name_storage, bookmark_name,
                                         replacement);
}

std::size_t TemplatePart::replace_bookmark_text(const char *bookmark_name,
                                                const char *replacement) {
    if (bookmark_name == nullptr || replacement == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "bookmark name and replacement text must not be null",
                           this->entry_name_storage);
        }
        return 0U;
    }

    return this->replace_bookmark_text(std::string{bookmark_name},
                                       std::string{replacement});
}

bookmark_fill_result TemplatePart::fill_bookmarks(
    std::span<const bookmark_text_binding> bindings) {
    bookmark_fill_result result;
    result.requested = bindings.size();

    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return result;
    }

    return fill_bookmarks_in_part(*this->last_error_info, *this->xml_document,
                                  this->entry_name_storage, bindings);
}

bookmark_fill_result TemplatePart::fill_bookmarks(
    std::initializer_list<bookmark_text_binding> bindings) {
    return this->fill_bookmarks(
        std::span<const bookmark_text_binding>{bindings.begin(), bindings.size()});
}

std::size_t TemplatePart::replace_bookmark_with_paragraphs(
    std::string_view bookmark_name, const std::vector<std::string> &paragraphs) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_bookmark_with_paragraphs_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        bookmark_name, paragraphs);
}

std::size_t TemplatePart::replace_bookmark_with_table_rows(
    std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_bookmark_with_table_rows_in_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        bookmark_name, rows);
}

std::size_t TemplatePart::replace_bookmark_with_table(
    std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return replace_bookmark_with_table_in_part(*this->last_error_info, *this->xml_document,
                                               this->entry_name_storage, bookmark_name,
                                               rows);
}

std::size_t TemplatePart::replace_bookmark_with_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return this->owner->replace_bookmark_with_image_in_part(
        *this->xml_document, this->entry_name_storage, bookmark_name, image_path,
        std::nullopt);
}

std::size_t TemplatePart::replace_bookmark_with_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return this->owner->replace_bookmark_with_image_in_part(
        *this->xml_document, this->entry_name_storage, bookmark_name, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px});
}

std::size_t TemplatePart::replace_bookmark_with_floating_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    featherdoc::floating_image_options options) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return this->owner->replace_bookmark_with_floating_image_in_part(
        *this->xml_document, this->entry_name_storage, bookmark_name, image_path,
        std::nullopt, std::move(options));
}

std::size_t TemplatePart::replace_bookmark_with_floating_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px,
    featherdoc::floating_image_options options) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return this->owner->replace_bookmark_with_floating_image_in_part(
        *this->xml_document, this->entry_name_storage, bookmark_name, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px},
        std::move(options));
}

std::size_t TemplatePart::remove_bookmark_block(std::string_view bookmark_name) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return remove_bookmark_block_in_part(*this->last_error_info, *this->xml_document,
                                         this->entry_name_storage, bookmark_name);
}

std::size_t TemplatePart::set_bookmark_block_visibility(
    std::string_view bookmark_name, bool visible) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return 0U;
    }

    return set_bookmark_block_visibility_in_part(*this->last_error_info, *this->xml_document,
                                                 this->entry_name_storage, bookmark_name,
                                                 visible);
}

bookmark_block_visibility_result TemplatePart::apply_bookmark_block_visibility(
    std::span<const bookmark_block_visibility_binding> bindings) {
    bookmark_block_visibility_result result;
    result.requested = bindings.size();

    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return result;
    }

    return apply_bookmark_block_visibility_in_part(*this->last_error_info,
                                                   *this->xml_document,
                                                   this->entry_name_storage, bindings);
}

bookmark_block_visibility_result TemplatePart::apply_bookmark_block_visibility(
    std::initializer_list<bookmark_block_visibility_binding> bindings) {
    return this->apply_bookmark_block_visibility(
        std::span<const bookmark_block_visibility_binding>{bindings.begin(),
                                                           bindings.size()});
}

TemplatePart Document::body_template() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accessing the body template");
        return {this, nullptr, &this->last_error_info, {}};
    }

    this->last_error_info.clear();
    return {this, &this->document, &this->last_error_info,
            std::string{document_xml_entry}};
}

TemplatePart Document::header_template(std::size_t index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accessing a header template");
        return {this, nullptr, &this->last_error_info, {}};
    }

    if (index >= this->header_parts.size()) {
        this->last_error_info.clear();
        return {this, nullptr, &this->last_error_info, {}};
    }

    this->last_error_info.clear();
    return {this, &this->header_parts[index]->xml, &this->last_error_info,
            this->header_parts[index]->entry_name};
}

TemplatePart Document::footer_template(std::size_t index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accessing a footer template");
        return {this, nullptr, &this->last_error_info, {}};
    }

    if (index >= this->footer_parts.size()) {
        this->last_error_info.clear();
        return {this, nullptr, &this->last_error_info, {}};
    }

    this->last_error_info.clear();
    return {this, &this->footer_parts[index]->xml, &this->last_error_info,
            this->footer_parts[index]->entry_name};
}

TemplatePart Document::section_header_template(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accessing a section header "
                       "template");
        return {this, nullptr, &this->last_error_info, {}};
    }

    auto *part = this->section_related_part_state(section_index, reference_kind,
                                                  this->header_parts,
                                                  "w:headerReference");
    if (part == nullptr) {
        this->last_error_info.clear();
        return {this, nullptr, &this->last_error_info, {}};
    }

    this->last_error_info.clear();
    return {this, &part->xml, &this->last_error_info, part->entry_name};
}

TemplatePart Document::section_footer_template(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accessing a section footer "
                       "template");
        return {this, nullptr, &this->last_error_info, {}};
    }

    auto *part = this->section_related_part_state(section_index, reference_kind,
                                                  this->footer_parts,
                                                  "w:footerReference");
    if (part == nullptr) {
        this->last_error_info.clear();
        return {this, nullptr, &this->last_error_info, {}};
    }

    this->last_error_info.clear();
    return {this, &part->xml, &this->last_error_info, part->entry_name};
}

std::size_t Document::replace_bookmark_text(const std::string &bookmark_name,
                                            const std::string &replacement) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before filling bookmark text");
        return 0U;
    }

    return replace_bookmark_text_in_part(this->last_error_info, this->document,
                                         document_xml_entry, bookmark_name,
                                         replacement);
}

std::size_t Document::replace_bookmark_text(const char *bookmark_name,
                                            const char *replacement) {
    if (bookmark_name == nullptr || replacement == nullptr) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "bookmark name and replacement text must not be null",
                       std::string{document_xml_entry});
        return 0U;
    }

    return this->replace_bookmark_text(std::string{bookmark_name},
                                       std::string{replacement});
}

bookmark_fill_result Document::fill_bookmarks(
    std::span<const bookmark_text_binding> bindings) {
    bookmark_fill_result result;
    result.requested = bindings.size();

    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before filling bookmarks");
        return result;
    }

    return fill_bookmarks_in_part(this->last_error_info, this->document,
                                  document_xml_entry, bindings);
}

bookmark_fill_result Document::fill_bookmarks(
    std::initializer_list<bookmark_text_binding> bindings) {
    return this->fill_bookmarks(
        std::span<const bookmark_text_binding>{bindings.begin(), bindings.size()});
}

std::size_t Document::replace_bookmark_with_paragraphs(
    std::string_view bookmark_name, const std::vector<std::string> &paragraphs) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before replacing a bookmark with paragraphs");
        return 0U;
    }

    return replace_bookmark_with_paragraphs_in_part(
        this->last_error_info, this->document, document_xml_entry, bookmark_name,
        paragraphs);
}

std::size_t Document::replace_bookmark_with_table_rows(
    std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before replacing a bookmark with table rows");
        return 0U;
    }

    return replace_bookmark_with_table_rows_in_part(
        this->last_error_info, this->document, document_xml_entry, bookmark_name, rows);
}

std::size_t Document::replace_bookmark_with_table(
    std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a bookmark with a table");
        return 0U;
    }

    return replace_bookmark_with_table_in_part(this->last_error_info, this->document,
                                               document_xml_entry, bookmark_name, rows);
}

std::size_t Document::replace_bookmark_with_image_in_part(
    pugi::xml_document &xml_document, std::string_view entry_name,
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::optional<std::pair<std::uint32_t, std::uint32_t>> dimensions) {
    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;

    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a bookmark with an image");
        return 0U;
    }

    if (bookmark_name.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    if (dimensions.has_value() &&
        (dimensions->first == 0U || dimensions->second == 0U)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "image width and height must both be greater than zero",
                       image_path.string());
        return 0U;
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_block_bookmark_placeholders(this->last_error_info, xml_document, entry_name,
                                             bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        this->last_error_info.clear();
        return 0U;
    }

    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return 0U;
    }

    pugi::xml_document *relationships_document = nullptr;
    std::string_view relationships_entry_name;
    bool *has_relationships_part = nullptr;
    bool *relationships_dirty = nullptr;

    if (entry_name == document_xml_entry) {
        relationships_document = &this->document_relationships;
        relationships_entry_name = document_relationships_xml_entry;
        has_relationships_part = &this->has_document_relationships_part;
        relationships_dirty = &this->document_relationships_dirty;
    } else if (auto *part = this->find_related_part_state(entry_name); part != nullptr) {
        relationships_document = &part->relationships;
        relationships_entry_name = part->relationships_entry_name;
        has_relationships_part = &part->has_relationships_part;
        relationships_dirty = &part->relationships_dirty;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return 0U;
    }

    const auto replacement_width =
        dimensions.has_value() ? dimensions->first : image_info.width_px;
    const auto replacement_height =
        dimensions.has_value() ? dimensions->second : image_info.height_px;

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        auto parent = placeholder.paragraph.parent();
        if (!this->append_inline_image_part(
                xml_document, entry_name, *relationships_document,
                relationships_entry_name, *has_relationships_part, *relationships_dirty,
                parent, placeholder.paragraph, std::string{image_info.data},
                std::string{image_info.extension}, std::string{image_info.content_type},
                image_path.filename().string(), replacement_width,
                replacement_height)) {
            return 0U;
        }

        if (!parent.remove_child(placeholder.paragraph)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to remove the bookmark placeholder paragraph after image "
                           "replacement",
                           std::string{entry_name});
            return 0U;
        }

        ++replaced;
    }

    this->last_error_info.clear();
    return replaced;
}

std::size_t Document::replace_bookmark_with_floating_image_in_part(
    pugi::xml_document &xml_document, std::string_view entry_name,
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::optional<std::pair<std::uint32_t, std::uint32_t>> dimensions,
    featherdoc::floating_image_options options) {
    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;

    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before replacing a bookmark with a floating image");
        return 0U;
    }

    if (bookmark_name.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    if (dimensions.has_value() &&
        (dimensions->first == 0U || dimensions->second == 0U)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "image width and height must both be greater than zero",
                       image_path.string());
        return 0U;
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_block_bookmark_placeholders(this->last_error_info, xml_document, entry_name,
                                             bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        this->last_error_info.clear();
        return 0U;
    }

    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return 0U;
    }

    pugi::xml_document *relationships_document = nullptr;
    std::string_view relationships_entry_name;
    bool *has_relationships_part = nullptr;
    bool *relationships_dirty = nullptr;

    if (entry_name == document_xml_entry) {
        relationships_document = &this->document_relationships;
        relationships_entry_name = document_relationships_xml_entry;
        has_relationships_part = &this->has_document_relationships_part;
        relationships_dirty = &this->document_relationships_dirty;
    } else if (auto *part = this->find_related_part_state(entry_name); part != nullptr) {
        relationships_document = &part->relationships;
        relationships_entry_name = part->relationships_entry_name;
        has_relationships_part = &part->has_relationships_part;
        relationships_dirty = &part->relationships_dirty;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return 0U;
    }

    const auto replacement_width =
        dimensions.has_value() ? dimensions->first : image_info.width_px;
    const auto replacement_height =
        dimensions.has_value() ? dimensions->second : image_info.height_px;

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        auto parent = placeholder.paragraph.parent();
        if (!this->append_floating_image_part(
                xml_document, entry_name, *relationships_document,
                relationships_entry_name, *has_relationships_part, *relationships_dirty,
                parent, placeholder.paragraph, std::string{image_info.data},
                std::string{image_info.extension}, std::string{image_info.content_type},
                image_path.filename().string(), replacement_width,
                replacement_height, options)) {
            return 0U;
        }

        if (!parent.remove_child(placeholder.paragraph)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to remove the bookmark placeholder paragraph after floating "
                           "image replacement",
                           std::string{entry_name});
            return 0U;
        }

        ++replaced;
    }

    this->last_error_info.clear();
    return replaced;
}

std::size_t Document::replace_bookmark_with_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path) {
    return this->replace_bookmark_with_image_in_part(
        this->document, document_xml_entry, bookmark_name, image_path, std::nullopt);
}

std::size_t Document::replace_bookmark_with_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px) {
    return this->replace_bookmark_with_image_in_part(
        this->document, document_xml_entry, bookmark_name, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px});
}

std::size_t Document::replace_bookmark_with_floating_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    featherdoc::floating_image_options options) {
    return this->replace_bookmark_with_floating_image_in_part(
        this->document, document_xml_entry, bookmark_name, image_path, std::nullopt,
        std::move(options));
}

std::size_t Document::replace_bookmark_with_floating_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px,
    featherdoc::floating_image_options options) {
    return this->replace_bookmark_with_floating_image_in_part(
        this->document, document_xml_entry, bookmark_name, image_path,
        std::pair<std::uint32_t, std::uint32_t>{width_px, height_px},
        std::move(options));
}

std::size_t Document::remove_bookmark_block(std::string_view bookmark_name) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing a bookmark block");
        return 0U;
    }

    return remove_bookmark_block_in_part(this->last_error_info, this->document,
                                         document_xml_entry, bookmark_name);
}

std::size_t Document::set_bookmark_block_visibility(std::string_view bookmark_name,
                                                    bool visible) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before changing bookmark block visibility");
        return 0U;
    }

    return set_bookmark_block_visibility_in_part(this->last_error_info, this->document,
                                                 document_xml_entry, bookmark_name, visible);
}

bookmark_block_visibility_result Document::apply_bookmark_block_visibility(
    std::span<const bookmark_block_visibility_binding> bindings) {
    bookmark_block_visibility_result result;
    result.requested = bindings.size();

    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before applying bookmark block visibility");
        return result;
    }

    return apply_bookmark_block_visibility_in_part(this->last_error_info, this->document,
                                                   document_xml_entry, bindings);
}

bookmark_block_visibility_result Document::apply_bookmark_block_visibility(
    std::initializer_list<bookmark_block_visibility_binding> bindings) {
    return this->apply_bookmark_block_visibility(
        std::span<const bookmark_block_visibility_binding>{bindings.begin(),
                                                           bindings.size()});
}

} // namespace featherdoc
