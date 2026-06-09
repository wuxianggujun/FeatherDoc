#include "document_template_content_control_replacement.hpp"

#include "featherdoc.hpp"
#include "xml_helpers.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include <zip.h>

namespace {

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
                          std::move(detail), std::move(entry_name),
                          std::move(xml_offset));
}

void ensure_attribute_value(pugi::xml_node node, const char *name,
                            std::string_view value) {
    if (node == pugi::xml_node{}) {
        return;
    }

    auto attribute = node.attribute(name);
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute(name);
    }
    if (attribute != pugi::xml_attribute{}) {
        attribute.set_value(value.data(), value.size());
    }
}

#include "document_template_content_control_helpers.inc"

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

auto rewrite_paragraph_plain_text(pugi::xml_node paragraph, std::string_view text)
    -> bool {
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
                run_properties =
                    run_properties_storage.append_copy(source_run_properties);
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

    if (run_properties != pugi::xml_node{} &&
        run.append_copy(run_properties) == pugi::xml_node{}) {
        return false;
    }

    return featherdoc::detail::set_plain_text_run_content(run, text);
}

auto rewrite_table_cell_plain_text(pugi::xml_node cell, std::string_view text)
    -> bool {
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

auto append_plain_text_table(pugi::xml_node parent,
                             const std::vector<std::vector<std::string>> &rows)
    -> bool {
    auto table = parent.append_child("w:tbl");
    if (table == pugi::xml_node{}) {
        return false;
    }

    for (const auto &row_values : rows) {
        auto row = table.append_child("w:tr");
        if (row == pugi::xml_node{}) {
            return false;
        }

        for (const auto &cell_text : row_values) {
            if (!append_text_table_cell(row, cell_text)) {
                return false;
            }
        }
    }

    return true;
}

auto validate_plain_text_table_rows(
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name,
    const std::vector<std::vector<std::string>> &rows) -> bool {
    if (rows.empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "replacement table must contain at least one row",
                       std::string{entry_name});
        return false;
    }

    for (const auto &row : rows) {
        if (row.empty()) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "replacement table rows must contain at least one cell",
                           std::string{entry_name});
            return false;
        }
    }

    return true;
}

void set_content_control_replacement_error(
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name, std::string_view detail) {
    set_last_error(last_error_info,
                   std::make_error_code(std::errc::invalid_argument),
                   std::string{detail}, std::string{entry_name});
}

auto rewrite_content_control_with_paragraphs(
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name, pugi::xml_node content_control,
    const std::vector<std::string> &paragraphs) -> bool {
    auto content = ensure_content_control_content_node(content_control);
    if (content == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to resolve content control content for paragraph replacement",
                       std::string{entry_name});
        return false;
    }

    const auto kind = content_control_kind_from_node(content_control);
    if (kind == featherdoc::content_control_kind::run) {
        set_content_control_replacement_error(
            last_error_info, entry_name,
            "paragraph replacement requires a block or table-cell content control");
        return false;
    }
    if (kind == featherdoc::content_control_kind::table_row) {
        set_content_control_replacement_error(
            last_error_info, entry_name,
            "paragraph replacement cannot target a table-row content control");
        return false;
    }

    clear_node_children(content);

    if (kind == featherdoc::content_control_kind::table_cell) {
        auto cell = content.append_child("w:tc");
        if (cell == pugi::xml_node{}) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to append a replacement content control cell",
                           std::string{entry_name});
            return false;
        }

        if (paragraphs.empty()) {
            if (!featherdoc::detail::append_plain_text_paragraph(cell, {})) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to append an empty replacement paragraph",
                               std::string{entry_name});
                return false;
            }
        }

        for (const auto &paragraph_text : paragraphs) {
            if (!featherdoc::detail::append_plain_text_paragraph(
                    cell, paragraph_text)) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to append a replacement paragraph",
                               std::string{entry_name});
                return false;
            }
        }

        clear_content_control_placeholder_flag(content_control);
        return true;
    }

    for (const auto &paragraph_text : paragraphs) {
        if (!featherdoc::detail::append_plain_text_paragraph(
                content, paragraph_text)) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to append a replacement paragraph",
                           std::string{entry_name});
            return false;
        }
    }

    clear_content_control_placeholder_flag(content_control);
    return true;
}

auto rewrite_content_control_with_table_rows(
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name, pugi::xml_node content_control,
    const std::vector<std::vector<std::string>> &rows) -> bool {
    if (content_control_kind_from_node(content_control) !=
        featherdoc::content_control_kind::table_row) {
        set_content_control_replacement_error(
            last_error_info, entry_name,
            "table row replacement requires a table-row content control");
        return false;
    }

    auto content = ensure_content_control_content_node(content_control);
    if (content == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to resolve content control content for table row replacement",
                       std::string{entry_name});
        return false;
    }

    const auto template_row = content.child("w:tr");
    if (template_row == pugi::xml_node{}) {
        if (rows.empty()) {
            clear_node_children(content);
            clear_content_control_placeholder_flag(content_control);
            return true;
        }

        set_content_control_replacement_error(
            last_error_info, entry_name,
            "table row replacement requires the content control to contain a template row");
        return false;
    }

    const auto cell_count = count_named_children(template_row, "w:tc");
    if (cell_count == 0U && !rows.empty()) {
        set_content_control_replacement_error(
            last_error_info, entry_name,
            "table row replacement requires the template row to contain at least one cell");
        return false;
    }

    for (const auto &row_values : rows) {
        if (row_values.size() != cell_count) {
            set_content_control_replacement_error(
                last_error_info, entry_name,
                "replacement table row cell count must exactly match the template row cell count");
            return false;
        }
    }

    pugi::xml_document template_row_document;
    const auto template_row_copy = template_row_document.append_copy(template_row);
    if (template_row_copy == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to clone the content control template row",
                       std::string{entry_name});
        return false;
    }

    clear_node_children(content);
    for (const auto &row_values : rows) {
        auto row_copy = content.append_copy(template_row_copy);
        if (row_copy == pugi::xml_node{}) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to append a replacement content control row",
                           std::string{entry_name});
            return false;
        }

        std::size_t cell_index = 0U;
        for (auto cell = row_copy.child("w:tc"); cell != pugi::xml_node{};
             cell = featherdoc::detail::next_named_sibling(cell, "w:tc"),
             ++cell_index) {
            if (!rewrite_table_cell_plain_text(cell, row_values[cell_index])) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to rewrite a replacement content control row cell",
                               std::string{entry_name});
                return false;
            }
        }
    }

    clear_content_control_placeholder_flag(content_control);
    return true;
}

auto rewrite_content_control_with_table(
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name, pugi::xml_node content_control,
    const std::vector<std::vector<std::string>> &rows) -> bool {
    auto content = ensure_content_control_content_node(content_control);
    if (content == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to resolve content control content for table replacement",
                       std::string{entry_name});
        return false;
    }

    const auto kind = content_control_kind_from_node(content_control);
    if (kind == featherdoc::content_control_kind::run) {
        set_content_control_replacement_error(
            last_error_info, entry_name,
            "table replacement requires a block or table-cell content control");
        return false;
    }
    if (kind == featherdoc::content_control_kind::table_row) {
        set_content_control_replacement_error(
            last_error_info, entry_name,
            "table replacement cannot target a table-row content control");
        return false;
    }

    clear_node_children(content);

    if (kind == featherdoc::content_control_kind::table_cell) {
        auto cell = content.append_child("w:tc");
        if (cell == pugi::xml_node{} || !append_plain_text_table(cell, rows)) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to append a replacement table inside a content control cell",
                           std::string{entry_name});
            return false;
        }

        if (!featherdoc::detail::append_plain_text_paragraph(cell, {})) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to append a trailing replacement table paragraph",
                           std::string{entry_name});
            return false;
        }

        clear_content_control_placeholder_flag(content_control);
        return true;
    }

    if (!append_plain_text_table(content, rows)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append a replacement content control table",
                       std::string{entry_name});
        return false;
    }

    clear_content_control_placeholder_flag(content_control);
    return true;
}

template <typename RewriteContentControl>
bool replace_content_control_by_tag_or_alias_in_node(
    featherdoc::document_error_info &last_error_info, pugi::xml_node node,
    std::string_view entry_name, std::string_view value, bool match_tag,
    std::size_t &replaced, RewriteContentControl rewrite_content_control) {
    for (auto child = node.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        if (std::string_view{child.name()} == "w:sdt" &&
            content_control_matches_tag_or_alias(child, value, match_tag)) {
            if (!rewrite_content_control(last_error_info, entry_name, child)) {
                return false;
            }
            ++replaced;
            child = next;
            continue;
        }

        if (!replace_content_control_by_tag_or_alias_in_node(
                last_error_info, child, entry_name, value, match_tag, replaced,
                rewrite_content_control)) {
            return false;
        }
        child = next;
    }

    return true;
}

} // namespace

namespace featherdoc {

auto replace_content_control_with_paragraphs_by_tag_or_alias_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view value, const std::vector<std::string> &paragraphs,
    bool match_tag) -> std::size_t {
    if (value.empty()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            match_tag ? "content control tag must not be empty"
                      : "content control alias must not be empty",
            std::string{entry_name});
        return 0U;
    }

    std::size_t replaced = 0U;
    const auto rewrite = [&paragraphs](
                             featherdoc::document_error_info &current_last_error,
                             std::string_view current_entry_name,
                             pugi::xml_node content_control) {
        return rewrite_content_control_with_paragraphs(
            current_last_error, current_entry_name, content_control, paragraphs);
    };
    if (!replace_content_control_by_tag_or_alias_in_node(
            last_error_info, document, entry_name, value, match_tag, replaced,
            rewrite)) {
        return replaced;
    }

    last_error_info.clear();
    return replaced;
}

auto replace_content_control_with_table_rows_by_tag_or_alias_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view value, const std::vector<std::vector<std::string>> &rows,
    bool match_tag) -> std::size_t {
    if (value.empty()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            match_tag ? "content control tag must not be empty"
                      : "content control alias must not be empty",
            std::string{entry_name});
        return 0U;
    }

    std::size_t replaced = 0U;
    const auto rewrite = [&rows](
                             featherdoc::document_error_info &current_last_error,
                             std::string_view current_entry_name,
                             pugi::xml_node content_control) {
        return rewrite_content_control_with_table_rows(
            current_last_error, current_entry_name, content_control, rows);
    };
    if (!replace_content_control_by_tag_or_alias_in_node(
            last_error_info, document, entry_name, value, match_tag, replaced,
            rewrite)) {
        return replaced;
    }

    last_error_info.clear();
    return replaced;
}

auto replace_content_control_with_table_by_tag_or_alias_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view value, const std::vector<std::vector<std::string>> &rows,
    bool match_tag) -> std::size_t {
    if (value.empty()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            match_tag ? "content control tag must not be empty"
                      : "content control alias must not be empty",
            std::string{entry_name});
        return 0U;
    }

    if (!validate_plain_text_table_rows(last_error_info, entry_name, rows)) {
        return 0U;
    }

    std::size_t replaced = 0U;
    const auto rewrite = [&rows](
                             featherdoc::document_error_info &current_last_error,
                             std::string_view current_entry_name,
                             pugi::xml_node content_control) {
        return rewrite_content_control_with_table(
            current_last_error, current_entry_name, content_control, rows);
    };
    if (!replace_content_control_by_tag_or_alias_in_node(
            last_error_info, document, entry_name, value, match_tag, replaced,
            rewrite)) {
        return replaced;
    }

    last_error_info.clear();
    return replaced;
}

} // namespace featherdoc
