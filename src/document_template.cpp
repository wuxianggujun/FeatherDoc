#include "featherdoc.hpp"
#include "image_helpers.hpp"
#include "xml_helpers.hpp"

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

struct block_bookmark_placeholder final {
    pugi::xml_node paragraph;
    pugi::xml_node bookmark_start;
    pugi::xml_node bookmark_end;
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

auto collect_block_bookmark_placeholders(featherdoc::document_error_info &last_error_info,
                                         pugi::xml_document &document,
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
                           std::string{document_xml_entry});
            return false;
        }

        const auto bookmark_end = find_matching_bookmark_end(bookmark_start);
        if (bookmark_end == pugi::xml_node{} || bookmark_end.parent() != paragraph) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "block bookmark replacement requires matching bookmark markers "
                           "inside the same paragraph",
                           std::string{document_xml_entry});
            return false;
        }

        if (!paragraph_is_block_placeholder(paragraph, bookmark_start, bookmark_end)) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "block bookmark replacement requires the target bookmark to occupy "
                           "its own paragraph",
                           std::string{document_xml_entry});
            return false;
        }

        placeholders.push_back({paragraph, bookmark_start, bookmark_end});
    }

    return true;
}

auto insert_table_before_placeholder(
    featherdoc::document_error_info &last_error_info, pugi::xml_node placeholder_paragraph,
    const std::vector<std::vector<std::string>> &rows) -> bool {
    auto parent = placeholder_paragraph.parent();
    if (placeholder_paragraph == pugi::xml_node{} || parent == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the placeholder paragraph for table replacement",
                       std::string{document_xml_entry});
        return false;
    }

    auto table_node = parent.insert_child_before("w:tbl", placeholder_paragraph);
    if (table_node == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                       "failed to insert the replacement table before the bookmark paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    featherdoc::Table table(parent, table_node);
    for (const auto &row_values : rows) {
        auto row = table.append_row(row_values.size());
        if (!row.has_next()) {
            set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                           "failed to append a replacement table row",
                           std::string{document_xml_entry});
            return false;
        }

        auto cell = row.cells();
        for (const auto &cell_text : row_values) {
            if (!cell.has_next()) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to resolve a replacement table cell",
                               std::string{document_xml_entry});
                return false;
            }

            if (!cell_text.empty() && !cell.paragraphs().add_run(cell_text).has_next()) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to append replacement table text",
                               std::string{document_xml_entry});
                return false;
            }

            cell.next();
        }
    }

    if (!parent.remove_child(placeholder_paragraph)) {
        set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                       "failed to remove the bookmark placeholder paragraph after table "
                       "replacement",
                       std::string{document_xml_entry});
        return false;
    }

    return true;
}
} // namespace

namespace featherdoc {

std::size_t Document::replace_bookmark_text(const std::string &bookmark_name,
                                            const std::string &replacement) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before filling bookmark text");
        return 0U;
    }

    if (bookmark_name.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty",
                       std::string{document_xml_entry});
        return 0U;
    }

    std::vector<pugi::xml_node> bookmark_starts;
    collect_named_bookmark_starts(this->document, bookmark_name, bookmark_starts);

    std::size_t replaced = 0U;
    for (const auto bookmark_start : bookmark_starts) {
        if (replace_bookmark_range(bookmark_start, replacement)) {
            ++replaced;
        }
    }

    this->last_error_info.clear();
    return replaced;
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

    std::unordered_set<std::string> seen_bookmarks;
    for (const auto &binding : bindings) {
        if (binding.bookmark_name.empty()) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "bookmark binding name must not be empty",
                           std::string{document_xml_entry});
            return result;
        }

        const auto [_, inserted] = seen_bookmarks.emplace(binding.bookmark_name);
        if (!inserted) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "duplicate bookmark binding name: " + binding.bookmark_name,
                           std::string{document_xml_entry});
            return result;
        }
    }

    for (const auto &binding : bindings) {
        const auto replaced = this->replace_bookmark_text(binding.bookmark_name, binding.text);
        if (replaced == 0U) {
            result.missing_bookmarks.push_back(binding.bookmark_name);
            continue;
        }

        ++result.matched;
        result.replaced += replaced;
    }

    this->last_error_info.clear();
    return result;
}

bookmark_fill_result Document::fill_bookmarks(
    std::initializer_list<bookmark_text_binding> bindings) {
    return this->fill_bookmarks(
        std::span<const bookmark_text_binding>{bindings.begin(), bindings.size()});
}

std::size_t Document::replace_bookmark_with_table(
    std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before replacing a bookmark with a table");
        return 0U;
    }

    if (bookmark_name.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty",
                       std::string{document_xml_entry});
        return 0U;
    }

    if (rows.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "replacement table must contain at least one row",
                       std::string{document_xml_entry});
        return 0U;
    }

    for (const auto &row : rows) {
        if (row.empty()) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "replacement table rows must contain at least one cell",
                           std::string{document_xml_entry});
            return 0U;
        }
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_block_bookmark_placeholders(this->last_error_info, this->document,
                                             bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        this->last_error_info.clear();
        return 0U;
    }

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        if (!insert_table_before_placeholder(this->last_error_info, placeholder.paragraph,
                                             rows)) {
            return 0U;
        }
        ++replaced;
    }

    this->last_error_info.clear();
    return replaced;
}

std::size_t Document::replace_bookmark_with_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path) {
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
                       "bookmark name must not be empty",
                       std::string{document_xml_entry});
        return 0U;
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_block_bookmark_placeholders(this->last_error_info, this->document,
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

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        auto parent = placeholder.paragraph.parent();
        if (!this->append_inline_image_part(
                parent, placeholder.paragraph, std::string{image_info.data},
                std::string{image_info.extension}, std::string{image_info.content_type},
                image_path.filename().string(), image_info.width_px,
                image_info.height_px)) {
            return 0U;
        }

        if (!parent.remove_child(placeholder.paragraph)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to remove the bookmark placeholder paragraph after image "
                           "replacement",
                           std::string{document_xml_entry});
            return 0U;
        }

        ++replaced;
    }

    this->last_error_info.clear();
    return replaced;
}

std::size_t Document::replace_bookmark_with_image(
    std::string_view bookmark_name, const std::filesystem::path &image_path,
    std::uint32_t width_px, std::uint32_t height_px) {
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
                       "bookmark name must not be empty",
                       std::string{document_xml_entry});
        return 0U;
    }

    if (width_px == 0U || height_px == 0U) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "image width and height must both be greater than zero",
                       image_path.string());
        return 0U;
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_block_bookmark_placeholders(this->last_error_info, this->document,
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

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        auto parent = placeholder.paragraph.parent();
        if (!this->append_inline_image_part(
                parent, placeholder.paragraph, std::string{image_info.data},
                std::string{image_info.extension}, std::string{image_info.content_type},
                image_path.filename().string(), width_px, height_px)) {
            return 0U;
        }

        if (!parent.remove_child(placeholder.paragraph)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to remove the bookmark placeholder paragraph after image "
                           "replacement",
                           std::string{document_xml_entry});
            return 0U;
        }

        ++replaced;
    }

    this->last_error_info.clear();
    return replaced;
}

} // namespace featherdoc
