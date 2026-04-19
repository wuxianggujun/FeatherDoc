#include "featherdoc.hpp"
#include "image_helpers.hpp"
#include "xml_helpers.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <utility>

namespace featherdoc {
auto find_table_index_by_bookmark_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view bookmark_name) -> std::optional<std::size_t>;
}

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};
constexpr auto document_relationships_xml_entry =
    std::string_view{"word/_rels/document.xml.rels"};
constexpr auto unavailable_template_part_detail =
    std::string_view{"template part is not available"};
constexpr auto page_number_field_instruction = std::string_view{" PAGE "};
constexpr auto total_pages_field_instruction = std::string_view{" NUMPAGES "};

auto template_part_block_container(pugi::xml_document &xml_document) -> pugi::xml_node {
    if (const auto body = xml_document.child("w:document").child("w:body");
        body != pugi::xml_node{}) {
        return body;
    }

    if (const auto header = xml_document.child("w:hdr"); header != pugi::xml_node{}) {
        return header;
    }

    if (const auto footer = xml_document.child("w:ftr"); footer != pugi::xml_node{}) {
        return footer;
    }

    return {};
}

auto last_block_paragraph(pugi::xml_node container) -> pugi::xml_node {
    pugi::xml_node paragraph;
    for (auto child = container.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:p") {
            paragraph = child;
        }
    }

    return paragraph;
}

auto paragraph_is_effectively_empty(pugi::xml_node paragraph) -> bool {
    if (paragraph == pugi::xml_node{} ||
        std::string_view{paragraph.name()} != "w:p") {
        return false;
    }

    for (auto child = paragraph.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:pPr") {
            return false;
        }
    }

    return true;
}

auto select_field_paragraph(pugi::xml_node container) -> pugi::xml_node {
    if (container == pugi::xml_node{}) {
        return {};
    }

    if (auto paragraph = last_block_paragraph(container);
        paragraph_is_effectively_empty(paragraph)) {
        return paragraph;
    }

    return featherdoc::detail::append_paragraph_node(container);
}

auto append_simple_field_run(pugi::xml_node paragraph, std::string_view instruction)
    -> bool {
    if (paragraph == pugi::xml_node{}) {
        return false;
    }

    auto field = paragraph.append_child("w:fldSimple");
    if (field == pugi::xml_node{}) {
        return false;
    }

    auto instruction_attribute = field.append_attribute("w:instr");
    if (instruction_attribute == pugi::xml_attribute{}) {
        return false;
    }
    if (!instruction_attribute.set_value(std::string{instruction}.c_str())) {
        return false;
    }

    auto run = field.append_child("w:r");
    if (run == pugi::xml_node{}) {
        return false;
    }

    auto run_properties = run.append_child("w:rPr");
    if (run_properties == pugi::xml_node{}) {
        return false;
    }
    if (run_properties.append_child("w:noProof") == pugi::xml_node{}) {
        return false;
    }

    auto text = run.append_child("w:t");
    if (text == pugi::xml_node{}) {
        return false;
    }

    return text.text().set("1");
}

auto read_optional_string_attribute(pugi::xml_node node, const char *attribute_name)
    -> std::optional<std::string> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto attribute = node.attribute(attribute_name);
    if (attribute == pugi::xml_attribute{} || attribute.value()[0] == '\0') {
        return std::nullopt;
    }

    return std::string{attribute.value()};
}

auto read_on_off_value(pugi::xml_node node) -> std::optional<bool> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto attribute = node.attribute("w:val");
    if (attribute == pugi::xml_attribute{} || attribute.value()[0] == '\0') {
        return true;
    }

    const auto value = std::string_view{attribute.value()};
    return value != "0" && value != "false" && value != "off";
}

auto read_part_paragraph_style_id(pugi::xml_node paragraph_properties)
    -> std::optional<std::string> {
    return read_optional_string_attribute(paragraph_properties.child("w:pStyle"), "w:val");
}

auto read_part_paragraph_bidi(pugi::xml_node paragraph_properties) -> std::optional<bool> {
    return read_on_off_value(paragraph_properties.child("w:bidi"));
}

auto parse_u32_attribute_value(const char *text) -> std::optional<std::uint32_t> {
    if (text == nullptr || *text == '\0') {
        return std::nullopt;
    }

    char *end = nullptr;
    const auto value = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' ||
        value > static_cast<unsigned long>(std::numeric_limits<std::uint32_t>::max())) {
        return std::nullopt;
    }

    return static_cast<std::uint32_t>(value);
}

auto summarize_part_paragraph_numbering(pugi::xml_node paragraph_properties)
    -> std::optional<featherdoc::paragraph_inspection_summary::numbering_summary> {
    const auto numbering_properties = paragraph_properties.child("w:numPr");
    if (numbering_properties == pugi::xml_node{}) {
        return std::nullopt;
    }

    auto summary = featherdoc::paragraph_inspection_summary::numbering_summary{};
    summary.level = parse_u32_attribute_value(
        numbering_properties.child("w:ilvl").attribute("w:val").value());
    summary.num_id = parse_u32_attribute_value(
        numbering_properties.child("w:numId").attribute("w:val").value());
    return summary;
}

auto enrich_part_paragraph_numbering(featherdoc::Document *owner,
                                     featherdoc::paragraph_inspection_summary &summary) -> void {
    if (owner == nullptr || !summary.numbering.has_value() ||
        !summary.numbering->num_id.has_value()) {
        return;
    }

    const auto lookup = owner->find_numbering_instance(*summary.numbering->num_id);
    if (!lookup.has_value()) {
        return;
    }

    summary.numbering->definition_id = lookup->definition_id;
    if (!lookup->definition_name.empty()) {
        summary.numbering->definition_name = lookup->definition_name;
    }
}

auto summarize_part_paragraph_node(pugi::xml_node paragraph_node, std::size_t paragraph_index)
    -> featherdoc::paragraph_inspection_summary {
    auto summary = featherdoc::paragraph_inspection_summary{};
    summary.index = paragraph_index;

    const auto paragraph_properties = paragraph_node.child("w:pPr");
    summary.style_id = read_part_paragraph_style_id(paragraph_properties);
    summary.bidi = read_part_paragraph_bidi(paragraph_properties);
    summary.numbering = summarize_part_paragraph_numbering(paragraph_properties);

    auto paragraph_handle = featherdoc::Paragraph{paragraph_node.parent(), paragraph_node};
    for (auto run = paragraph_handle.runs(); run.has_next(); run.next()) {
        ++summary.run_count;
        summary.text += run.get_text();
    }

    return summary;
}

auto summarize_part_run_handle(const featherdoc::Run &run_handle, std::size_t run_index)
    -> featherdoc::run_inspection_summary {
    auto summary = featherdoc::run_inspection_summary{};
    summary.index = run_index;
    summary.text = run_handle.get_text();
    summary.style_id = run_handle.style_id();
    summary.font_family = run_handle.font_family();
    summary.east_asia_font_family = run_handle.east_asia_font_family();
    summary.language = run_handle.language();
    summary.east_asia_language = run_handle.east_asia_language();
    summary.bidi_language = run_handle.bidi_language();
    summary.rtl = run_handle.rtl();
    return summary;
}

auto summarize_part_table_handle(featherdoc::Table table_handle, std::size_t table_index)
    -> featherdoc::table_inspection_summary {
    auto summary = featherdoc::table_inspection_summary{};
    summary.index = table_index;
    summary.style_id = table_handle.style_id();
    summary.width_twips = table_handle.width_twips();

    bool first_row = true;
    for (auto row = table_handle.rows(); row.has_next(); row.next()) {
        ++summary.row_count;

        std::size_t row_column_count = 0U;
        bool first_cell = true;
        for (auto cell = row.cells(); cell.has_next(); cell.next()) {
            row_column_count += cell.column_span();

            if (!first_row) {
                if (first_cell) {
                    summary.text.push_back('\n');
                } else {
                    summary.text.push_back('\t');
                }
            } else if (!first_cell) {
                summary.text.push_back('\t');
            }

            summary.text += cell.get_text();
            first_cell = false;
        }

        summary.column_count = std::max(summary.column_count, row_column_count);
        first_row = false;
    }

    summary.column_widths.reserve(summary.column_count);
    for (std::size_t column_index = 0U; column_index < summary.column_count; ++column_index) {
        summary.column_widths.push_back(table_handle.column_width_twips(column_index));
    }

    return summary;
}

auto summarize_part_table_cell_handle(featherdoc::TableCell cell_handle, std::size_t row_index,
                                      std::size_t cell_index, std::size_t column_index)
    -> featherdoc::table_cell_inspection_summary {
    auto summary = featherdoc::table_cell_inspection_summary{};
    summary.row_index = row_index;
    summary.cell_index = cell_index;
    summary.column_index = column_index;
    summary.column_span = cell_handle.column_span();
    summary.width_twips = cell_handle.width_twips();
    summary.vertical_alignment = cell_handle.vertical_alignment();
    summary.text_direction = cell_handle.text_direction();
    summary.text = cell_handle.get_text();

    for (auto paragraph = cell_handle.paragraphs(); paragraph.has_next(); paragraph.next()) {
        ++summary.paragraph_count;
    }

    return summary;
}

auto collect_part_table_cell_summaries(featherdoc::Table table_handle)
    -> std::vector<featherdoc::table_cell_inspection_summary> {
    auto summaries = std::vector<featherdoc::table_cell_inspection_summary>{};
    auto row = table_handle.rows();
    for (std::size_t row_index = 0U; row.has_next(); ++row_index, row.next()) {
        std::size_t column_index = 0U;
        auto cell = row.cells();
        for (std::size_t cell_index = 0U; cell.has_next(); ++cell_index, cell.next()) {
            auto summary =
                summarize_part_table_cell_handle(cell, row_index, cell_index, column_index);
            column_index += summary.column_span;
            summaries.push_back(std::move(summary));
        }
    }

    return summaries;
}

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

void collect_named_bookmark_counts(
    pugi::xml_node node, std::unordered_map<std::string, std::size_t> &bookmark_counts) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:bookmarkStart") {
            const auto bookmark_name = std::string_view{child.attribute("w:name").value()};
            if (!bookmark_name.empty()) {
                ++bookmark_counts[std::string{bookmark_name}];
            }
        }

        collect_named_bookmark_counts(child, bookmark_counts);
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
                                 pugi::xml_document &document,
                                 std::string_view entry_name)
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

    const auto summaries =
        summarize_bookmarks_in_part(last_error_info, document, entry_name);
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

    return featherdoc::detail::set_plain_text_run_content(run, text);
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

auto validate_placeholder_requirement(
    pugi::xml_document &document, std::string_view entry_name,
    const featherdoc::template_slot_requirement &requirement) -> bool {
    featherdoc::document_error_info validation_error_info;

    switch (requirement.kind) {
    case featherdoc::template_slot_kind::text:
        return true;
    case featherdoc::template_slot_kind::table_rows: {
        std::vector<table_row_bookmark_placeholder> placeholders;
        return collect_table_row_bookmark_placeholders(
            validation_error_info, document, entry_name, requirement.bookmark_name,
            placeholders);
    }
    case featherdoc::template_slot_kind::table:
    case featherdoc::template_slot_kind::image:
    case featherdoc::template_slot_kind::floating_image:
    case featherdoc::template_slot_kind::block: {
        std::vector<block_bookmark_placeholder> placeholders;
        return collect_block_bookmark_placeholders(
            validation_error_info, document, entry_name, requirement.bookmark_name,
            placeholders);
    }
    }

    return false;
}

auto validate_template_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name,
    std::span<const featherdoc::template_slot_requirement> requirements)
    -> featherdoc::template_validation_result {
    featherdoc::template_validation_result result;

    std::unordered_map<std::string, std::size_t> bookmark_counts;
    collect_named_bookmark_counts(document, bookmark_counts);

    std::unordered_set<std::string> missing_seen;
    std::unordered_set<std::string> duplicates_seen;
    std::unordered_set<std::string> malformed_seen;
    for (const auto &requirement : requirements) {
        if (requirement.bookmark_name.empty()) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template slot bookmark name must not be empty",
                           std::string{entry_name});
            return result;
        }

        const auto count_it = bookmark_counts.find(requirement.bookmark_name);
        const auto bookmark_count =
            count_it == bookmark_counts.end() ? 0U : count_it->second;

        if (requirement.required && bookmark_count == 0U &&
            missing_seen.emplace(requirement.bookmark_name).second) {
            result.missing_required.push_back(requirement.bookmark_name);
        }

        if (bookmark_count > 1U &&
            duplicates_seen.emplace(requirement.bookmark_name).second) {
            result.duplicate_bookmarks.push_back(requirement.bookmark_name);
        }

        if (bookmark_count == 0U) {
            continue;
        }

        if (!validate_placeholder_requirement(document, entry_name, requirement) &&
            malformed_seen.emplace(requirement.bookmark_name).second) {
            result.malformed_placeholders.push_back(requirement.bookmark_name);
        }
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

Paragraph TemplatePart::paragraphs() {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    const auto container = template_part_block_container(*this->xml_document);
    if (container == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       this->entry_name_storage);
        return {};
    }

    this->last_error_info->clear();
    return {container, container.child("w:p")};
}

Paragraph TemplatePart::append_paragraph(const std::string &text,
                                         featherdoc::formatting_flag formatting) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    const auto container = template_part_block_container(*this->xml_document);
    if (container == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       this->entry_name_storage);
        return {};
    }

    const auto paragraph_node = detail::append_paragraph_node(container);
    if (paragraph_node == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append paragraph to template part",
                       this->entry_name_storage);
        return {};
    }

    auto paragraph = Paragraph(container, paragraph_node);
    if (!text.empty() && !paragraph.add_run(text, formatting).has_next()) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append paragraph text to template part",
                       this->entry_name_storage);
        return {};
    }

    this->last_error_info->clear();
    return paragraph;
}

bool TemplatePart::append_page_number_field() {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto container = template_part_block_container(*this->xml_document);
    if (container == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       this->entry_name_storage);
        return false;
    }

    const auto paragraph = select_field_paragraph(container);
    if (paragraph == pugi::xml_node{} ||
        !append_simple_field_run(paragraph, page_number_field_instruction)) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append page number field to template part",
                       this->entry_name_storage);
        return false;
    }

    this->last_error_info->clear();
    return true;
}

bool TemplatePart::append_total_pages_field() {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto container = template_part_block_container(*this->xml_document);
    if (container == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       this->entry_name_storage);
        return false;
    }

    const auto paragraph = select_field_paragraph(container);
    if (paragraph == pugi::xml_node{} ||
        !append_simple_field_run(paragraph, total_pages_field_instruction)) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append total pages field to template part",
                       this->entry_name_storage);
        return false;
    }

    this->last_error_info->clear();
    return true;
}

Table TemplatePart::tables() {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    const auto container = template_part_block_container(*this->xml_document);
    if (container == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       this->entry_name_storage);
        return {};
    }

    this->last_error_info->clear();
    auto table = Table(container, container.child("w:tbl"));
    table.set_owner(this->owner);
    return table;
}

Table TemplatePart::append_table(std::size_t row_count, std::size_t column_count) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    const auto container = template_part_block_container(*this->xml_document);
    if (container == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       this->entry_name_storage);
        return {};
    }

    const auto table_node = detail::append_table_node(container);
    auto created_table = Table(container, table_node);
    created_table.set_owner(this->owner);

    for (std::size_t row_index = 0; row_index < row_count; ++row_index) {
        created_table.append_row(column_count);
    }

    this->last_error_info->clear();
    return created_table;
}

std::vector<featherdoc::table_inspection_summary> TemplatePart::inspect_tables() {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    auto summaries = std::vector<featherdoc::table_inspection_summary>{};
    auto table_handle = this->tables();
    for (std::size_t table_index = 0U; table_handle.has_next();
         ++table_index, table_handle.next()) {
        summaries.push_back(summarize_part_table_handle(table_handle, table_index));
    }

    this->last_error_info->clear();
    return summaries;
}

std::optional<featherdoc::table_inspection_summary>
TemplatePart::inspect_table(std::size_t table_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    auto table_handle = this->tables();
    for (std::size_t current_index = 0U;
         current_index < table_index && table_handle.has_next(); ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        this->last_error_info->clear();
        return std::nullopt;
    }

    auto summary = summarize_part_table_handle(table_handle, table_index);
    this->last_error_info->clear();
    return summary;
}

std::vector<featherdoc::table_cell_inspection_summary>
TemplatePart::inspect_table_cells(std::size_t table_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    auto table_handle = this->tables();
    for (std::size_t current_index = 0U;
         current_index < table_index && table_handle.has_next(); ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        this->last_error_info->clear();
        return {};
    }

    auto summaries = collect_part_table_cell_summaries(table_handle);
    this->last_error_info->clear();
    return summaries;
}

std::optional<featherdoc::table_cell_inspection_summary>
TemplatePart::inspect_table_cell(std::size_t table_index, std::size_t row_index,
                                 std::size_t cell_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    const auto cells = this->inspect_table_cells(table_index);
    for (const auto &cell : cells) {
        if (cell.row_index == row_index && cell.cell_index == cell_index) {
            this->last_error_info->clear();
            return cell;
        }
    }

    this->last_error_info->clear();
    return std::nullopt;
}

auto find_table_index_by_bookmark_in_part(featherdoc::document_error_info &last_error_info,
                                          pugi::xml_document &document,
                                          std::string_view entry_name,
                                          std::string_view bookmark_name)
    -> std::optional<std::size_t> {
    if (bookmark_name.empty()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            "bookmark name must not be empty when resolving a table by bookmark",
            std::string{entry_name});
        return std::nullopt;
    }

    const auto container = template_part_block_container(document);
    if (container == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::vector<pugi::xml_node> bookmark_starts;
    collect_named_bookmark_starts(document, bookmark_name, bookmark_starts);
    if (bookmark_starts.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name '" + std::string{bookmark_name} +
                           "' was not found in " + std::string{entry_name},
                       std::string{entry_name});
        return std::nullopt;
    }

    for (const auto bookmark_start : bookmark_starts) {
        if (const auto table = find_top_level_table_ancestor(bookmark_start, container);
            table != pugi::xml_node{}) {
            if (const auto table_index = find_top_level_table_index(container, table)) {
                last_error_info.clear();
                return table_index;
            }
        }

        const auto bookmark_end = find_matching_bookmark_end_in_document_order(bookmark_start);
        if (bookmark_end != pugi::xml_node{}) {
            if (const auto table =
                    find_first_top_level_table_after_node(container, bookmark_start, bookmark_end);
                table != pugi::xml_node{}) {
                if (const auto table_index = find_top_level_table_index(container, table)) {
                    last_error_info.clear();
                    return table_index;
                }
            }

            if (const auto table = find_top_level_table_ancestor(bookmark_end, container);
                table != pugi::xml_node{}) {
                if (const auto table_index = find_top_level_table_index(container, table)) {
                    last_error_info.clear();
                    return table_index;
                }
            }
        }

        const auto search_anchor =
            bookmark_end != pugi::xml_node{} ? bookmark_end : bookmark_start;
        if (const auto table =
                find_first_top_level_table_after_node(container, search_anchor);
            table != pugi::xml_node{}) {
            if (const auto table_index = find_top_level_table_index(container, table)) {
                last_error_info.clear();
                return table_index;
            }
        }
    }

    set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                   "bookmark name '" + std::string{bookmark_name} +
                       "' did not resolve to a table in " + std::string{entry_name},
                   std::string{entry_name});
    return std::nullopt;
}

std::vector<featherdoc::paragraph_inspection_summary> TemplatePart::inspect_paragraphs() {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    auto summaries = std::vector<featherdoc::paragraph_inspection_summary>{};
    auto paragraph_handle = this->paragraphs();
    for (std::size_t paragraph_index = 0U; paragraph_handle.has_next();
         ++paragraph_index, paragraph_handle.next()) {
        auto summary = summarize_part_paragraph_node(paragraph_handle.current, paragraph_index);
        enrich_part_paragraph_numbering(this->owner, summary);
        summaries.push_back(std::move(summary));
    }

    this->last_error_info->clear();
    return summaries;
}

std::optional<featherdoc::paragraph_inspection_summary>
TemplatePart::inspect_paragraph(std::size_t paragraph_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next(); ++current_index) {
        paragraph_handle.next();
    }

    if (!paragraph_handle.has_next()) {
        this->last_error_info->clear();
        return std::nullopt;
    }

    auto summary = summarize_part_paragraph_node(paragraph_handle.current, paragraph_index);
    enrich_part_paragraph_numbering(this->owner, summary);
    this->last_error_info->clear();
    return summary;
}

std::vector<featherdoc::run_inspection_summary>
TemplatePart::inspect_paragraph_runs(std::size_t paragraph_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next(); ++current_index) {
        paragraph_handle.next();
    }

    if (!paragraph_handle.has_next()) {
        this->last_error_info->clear();
        return {};
    }

    auto summaries = std::vector<featherdoc::run_inspection_summary>{};
    auto run = paragraph_handle.runs();
    for (std::size_t run_index = 0U; run.has_next(); ++run_index, run.next()) {
        summaries.push_back(summarize_part_run_handle(run, run_index));
    }

    this->last_error_info->clear();
    return summaries;
}

std::optional<featherdoc::run_inspection_summary>
TemplatePart::inspect_paragraph_run(std::size_t paragraph_index, std::size_t run_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    const auto runs = this->inspect_paragraph_runs(paragraph_index);
    if (run_index >= runs.size()) {
        this->last_error_info->clear();
        return std::nullopt;
    }

    return runs[run_index];
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

std::vector<featherdoc::bookmark_summary> TemplatePart::list_bookmarks() const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return summarize_bookmarks_in_part(*this->last_error_info, *this->xml_document,
                                       this->entry_name_storage);
}

std::optional<featherdoc::bookmark_summary> TemplatePart::find_bookmark(
    std::string_view bookmark_name) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    return find_bookmark_in_part(*this->last_error_info, *this->xml_document,
                                 this->entry_name_storage, bookmark_name);
}

std::optional<std::size_t> TemplatePart::find_table_index_by_bookmark(
    std::string_view bookmark_name) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    return find_table_index_by_bookmark_in_part(*this->last_error_info,
                                                *this->xml_document,
                                                this->entry_name_storage,
                                                bookmark_name);
}

std::optional<std::size_t> TemplatePart::find_table_index(
    const featherdoc::template_table_selector &selector) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    return find_table_index_by_selector_in_part(*this->last_error_info,
                                                *this->xml_document,
                                                this->entry_name_storage,
                                                selector);
}

std::optional<featherdoc::Table> TemplatePart::find_table_by_bookmark(
    std::string_view bookmark_name) {
    const auto resolved_index = this->find_table_index_by_bookmark(bookmark_name);
    if (!resolved_index.has_value()) {
        return std::nullopt;
    }

    auto table_handle = this->tables();
    for (std::size_t current_index = 0U;
         current_index < *resolved_index && table_handle.has_next(); ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::state_not_recoverable),
                           "bookmark-resolved table index '" +
                               std::to_string(*resolved_index) +
                               "' is no longer present in template part",
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    this->last_error_info->clear();
    return table_handle;
}

std::optional<featherdoc::Table> TemplatePart::find_table(
    const featherdoc::template_table_selector &selector) {
    const auto resolved_index = this->find_table_index(selector);
    if (!resolved_index.has_value()) {
        return std::nullopt;
    }

    auto table_handle = this->tables();
    for (std::size_t current_index = 0U;
         current_index < *resolved_index && table_handle.has_next(); ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::state_not_recoverable),
                           "selector-resolved table index '" +
                               std::to_string(*resolved_index) +
                               "' is no longer present in template part",
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    this->last_error_info->clear();
    return table_handle;
}

template_validation_result TemplatePart::validate_template(
    std::span<const template_slot_requirement> requirements) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return validate_template_in_part(*this->last_error_info, *this->xml_document,
                                     this->entry_name_storage, requirements);
}

template_validation_result TemplatePart::validate_template(
    std::initializer_list<template_slot_requirement> requirements) const {
    return this->validate_template(
        std::span<const template_slot_requirement>{requirements.begin(),
                                                   requirements.size()});
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

std::vector<featherdoc::bookmark_summary> Document::list_bookmarks() const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing bookmarks",
                       std::string{document_xml_entry});
        return {};
    }

    return summarize_bookmarks_in_part(this->last_error_info,
                                       const_cast<pugi::xml_document &>(this->document),
                                       document_xml_entry);
}

std::optional<featherdoc::bookmark_summary> Document::find_bookmark(
    std::string_view bookmark_name) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading bookmark metadata",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    return find_bookmark_in_part(this->last_error_info,
                                 const_cast<pugi::xml_document &>(this->document),
                                 document_xml_entry, bookmark_name);
}

template_validation_result Document::validate_template(
    std::span<const template_slot_requirement> requirements) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before validating a template");
        return {};
    }

    return validate_template_in_part(this->last_error_info,
                                     const_cast<pugi::xml_document &>(this->document),
                                     document_xml_entry, requirements);
}

template_validation_result Document::validate_template(
    std::initializer_list<template_slot_requirement> requirements) const {
    return this->validate_template(
        std::span<const template_slot_requirement>{requirements.begin(),
                                                   requirements.size()});
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
