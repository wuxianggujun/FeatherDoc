#include "featherdoc.hpp"

#include "document_semantic_diff_values.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace featherdoc {

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};

auto set_last_error(featherdoc::document_error_info &error_info,
                    std::error_code code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    error_info = {code, std::move(detail), std::move(entry_name), xml_offset};
    return code;
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    featherdoc::document_errc code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    return set_last_error(error_info, featherdoc::make_error_code(code),
                          std::move(detail), std::move(entry_name), xml_offset);
}



#include "document_semantic_diff_template_part_helpers.inc"


struct semantic_field_value {
    std::string path;
    std::string value;
};

auto is_semantic_field_name(std::string_view text) -> bool {
    if (text.empty()) {
        return false;
    }
    for (const auto ch : text) {
        const auto valid = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                           (ch >= '0' && ch <= '9') || ch == '_';
        if (!valid) {
            return false;
        }
    }
    return true;
}

auto try_expand_semicolon_fields(std::string_view path, std::string_view value,
                                 std::vector<semantic_field_value> &fields)
    -> bool {
    if (value.find(';') == std::string_view::npos) {
        return false;
    }

    auto expanded = std::vector<semantic_field_value>{};
    std::size_t start = 0U;
    while (start <= value.size()) {
        const auto end = value.find(';', start);
        const auto segment = value.substr(
            start, end == std::string_view::npos ? std::string_view::npos
                                                 : end - start);
        if (segment.empty()) {
            return false;
        }
        const auto equals = segment.find('=');
        if (equals == std::string_view::npos || equals == 0U) {
            return false;
        }
        const auto name = segment.substr(0U, equals);
        if (!is_semantic_field_name(name)) {
            return false;
        }
        auto child_path = std::string{path};
        child_path.push_back('.');
        child_path.append(name.data(), name.size());
        expanded.push_back(semantic_field_value{
            std::move(child_path),
            std::string{segment.substr(equals + 1U)}});
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1U;
    }

    if (expanded.size() < 2U) {
        return false;
    }
    fields.insert(fields.end(), expanded.begin(), expanded.end());
    return true;
}

auto parse_semantic_field_values(std::string_view value)
    -> std::vector<semantic_field_value> {
    auto top_level = std::vector<semantic_field_value>{};
    std::size_t start = 0U;
    while (start <= value.size()) {
        const auto end = value.find('\n', start);
        const auto line = value.substr(
            start, end == std::string_view::npos ? std::string_view::npos
                                                 : end - start);
        const auto equals = line.find('=');
        if (equals != std::string_view::npos &&
            is_semantic_field_name(line.substr(0U, equals))) {
            top_level.push_back(semantic_field_value{
                std::string{line.substr(0U, equals)},
                std::string{line.substr(equals + 1U)}});
        } else if (!top_level.empty()) {
            top_level.back().value.push_back('\n');
            top_level.back().value.append(line.data(), line.size());
        } else if (!line.empty()) {
            top_level.push_back(semantic_field_value{"value", std::string{line}});
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1U;
    }

    auto fields = std::vector<semantic_field_value>{};
    for (const auto &field : top_level) {
        if (!try_expand_semicolon_fields(field.path, field.value, fields)) {
            fields.push_back(field);
        }
    }
    return fields;
}

auto find_semantic_field_value(const std::vector<semantic_field_value> &fields,
                               std::string_view path) -> std::optional<std::string> {
    for (const auto &field : fields) {
        if (field.path == path) {
            return field.value;
        }
    }
    return std::nullopt;
}

auto semantic_field_changes(std::string_view left_value, std::string_view right_value)
    -> std::vector<featherdoc::document_semantic_diff_field_change> {
    const auto left_fields = parse_semantic_field_values(left_value);
    const auto right_fields = parse_semantic_field_values(right_value);
    auto changes = std::vector<featherdoc::document_semantic_diff_field_change>{};

    for (const auto &left_field : left_fields) {
        const auto right_field = find_semantic_field_value(right_fields, left_field.path);
        if (!right_field.has_value() || *right_field != left_field.value) {
            changes.push_back(featherdoc::document_semantic_diff_field_change{
                left_field.path, left_field.value,
                right_field.value_or(std::string{})});
        }
    }

    for (const auto &right_field : right_fields) {
        if (!find_semantic_field_value(left_fields, right_field.path).has_value()) {
            changes.push_back(featherdoc::document_semantic_diff_field_change{
                right_field.path, std::string{}, right_field.value});
        }
    }

    return changes;
}

auto make_semantic_sequence_change(std::string_view field)
    -> featherdoc::document_semantic_diff_change {
    auto change = featherdoc::document_semantic_diff_change{};
    change.field = std::string{field};
    return change;
}

void append_semantic_added_change(
    std::string_view field, const std::vector<std::string> &right_values,
    std::size_t right_index,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    ++summary.added_count;
    auto change = make_semantic_sequence_change(field);
    change.kind = featherdoc::document_semantic_diff_change_kind::added;
    change.right_index = right_index;
    change.right_value = right_values[right_index];
    changes.push_back(std::move(change));
}

void append_semantic_removed_change(
    std::string_view field, const std::vector<std::string> &left_values,
    std::size_t left_index,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    ++summary.removed_count;
    auto change = make_semantic_sequence_change(field);
    change.kind = featherdoc::document_semantic_diff_change_kind::removed;
    change.left_index = left_index;
    change.left_value = left_values[left_index];
    changes.push_back(std::move(change));
}

void append_semantic_changed_change(
    std::string_view field, const std::vector<std::string> &left_values,
    const std::vector<std::string> &right_values, std::size_t left_index,
    std::size_t right_index,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    ++summary.changed_count;
    auto change = make_semantic_sequence_change(field);
    change.kind = featherdoc::document_semantic_diff_change_kind::changed;
    change.left_index = left_index;
    change.right_index = right_index;
    change.left_value = left_values[left_index];
    change.right_value = right_values[right_index];
    change.field_changes = semantic_field_changes(change.left_value, change.right_value);
    changes.push_back(std::move(change));
}

void compare_semantic_values_by_index(
    const std::vector<std::string> &left_values,
    const std::vector<std::string> &right_values, std::string_view field,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    const auto item_count = std::max(left_values.size(), right_values.size());
    for (std::size_t item_index = 0; item_index < item_count; ++item_index) {
        if (item_index >= left_values.size()) {
            append_semantic_added_change(field, right_values, item_index, summary,
                                         changes);
            continue;
        }

        if (item_index >= right_values.size()) {
            append_semantic_removed_change(field, left_values, item_index, summary,
                                           changes);
            continue;
        }

        if (left_values[item_index] != right_values[item_index]) {
            append_semantic_changed_change(field, left_values, right_values,
                                           item_index, item_index, summary,
                                           changes);
        } else {
            ++summary.unchanged_count;
        }
    }
}

auto can_align_semantic_values(
    std::size_t left_count, std::size_t right_count,
    const featherdoc::document_semantic_diff_options &options) -> bool {
    if (!options.align_sequences_by_content) {
        return false;
    }

    const auto row_count = left_count + 1U;
    const auto column_count = right_count + 1U;
    if (row_count == 0U || column_count == 0U) {
        return false;
    }
    return row_count <= options.alignment_cell_limit / column_count;
}

void compare_semantic_values_by_content_alignment(
    const std::vector<std::string> &left_values,
    const std::vector<std::string> &right_values, std::string_view field,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    const auto left_count = left_values.size();
    const auto right_count = right_values.size();
    const auto column_count = right_count + 1U;
    auto cost = std::vector<std::size_t>((left_count + 1U) * column_count, 0U);
    const auto cost_index = [column_count](std::size_t left_position,
                                           std::size_t right_position) {
        return left_position * column_count + right_position;
    };

    for (std::size_t left_position = 1U; left_position <= left_count;
         ++left_position) {
        cost[cost_index(left_position, 0U)] = left_position;
    }
    for (std::size_t right_position = 1U; right_position <= right_count;
         ++right_position) {
        cost[cost_index(0U, right_position)] = right_position;
    }

    for (std::size_t left_position = 1U; left_position <= left_count;
         ++left_position) {
        for (std::size_t right_position = 1U; right_position <= right_count;
             ++right_position) {
            const auto substitution_cost =
                left_values[left_position - 1U] == right_values[right_position - 1U]
                    ? 0U
                    : 1U;
            auto best_cost = cost[cost_index(left_position - 1U,
                                             right_position - 1U)] +
                             substitution_cost;
            const auto removal_cost =
                cost[cost_index(left_position - 1U, right_position)] + 1U;
            if (removal_cost < best_cost) {
                best_cost = removal_cost;
            }
            const auto addition_cost =
                cost[cost_index(left_position, right_position - 1U)] + 1U;
            if (addition_cost < best_cost) {
                best_cost = addition_cost;
            }
            cost[cost_index(left_position, right_position)] = best_cost;
        }
    }

    auto aligned_changes = std::vector<featherdoc::document_semantic_diff_change>{};
    std::size_t left_position = left_count;
    std::size_t right_position = right_count;
    while (left_position > 0U || right_position > 0U) {
        const auto current_cost = cost[cost_index(left_position, right_position)];
        if (left_position > 0U && right_position > 0U) {
            const auto left_index = left_position - 1U;
            const auto right_index = right_position - 1U;
            const auto substitution_cost =
                left_values[left_index] == right_values[right_index] ? 0U : 1U;
            const auto diagonal_cost =
                cost[cost_index(left_position - 1U, right_position - 1U)] +
                substitution_cost;
            if (current_cost == diagonal_cost) {
                if (substitution_cost == 0U) {
                    ++summary.unchanged_count;
                } else {
                    append_semantic_changed_change(field, left_values, right_values,
                                                   left_index, right_index, summary,
                                                   aligned_changes);
                }
                --left_position;
                --right_position;
                continue;
            }
        }

        if (left_position > 0U &&
            current_cost == cost[cost_index(left_position - 1U, right_position)] +
                                1U) {
            append_semantic_removed_change(field, left_values, left_position - 1U,
                                           summary, aligned_changes);
            --left_position;
            continue;
        }

        append_semantic_added_change(field, right_values, right_position - 1U,
                                     summary, aligned_changes);
        --right_position;
    }

    std::reverse(aligned_changes.begin(), aligned_changes.end());
    changes.insert(changes.end(), aligned_changes.begin(), aligned_changes.end());
}

template <typename Item, typename Fingerprint>
void compare_semantic_sequence(
    const std::vector<Item> &left_items, const std::vector<Item> &right_items,
    std::string_view field, Fingerprint fingerprint,
    const featherdoc::document_semantic_diff_options &options,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    summary.left_count = left_items.size();
    summary.right_count = right_items.size();

    auto left_values = std::vector<std::string>{};
    auto right_values = std::vector<std::string>{};
    left_values.reserve(left_items.size());
    right_values.reserve(right_items.size());
    for (const auto &left_item : left_items) {
        left_values.push_back(fingerprint(left_item));
    }
    for (const auto &right_item : right_items) {
        right_values.push_back(fingerprint(right_item));
    }

    if (can_align_semantic_values(left_values.size(), right_values.size(), options)) {
        compare_semantic_values_by_content_alignment(left_values, right_values, field,
                                                     summary, changes);
        return;
    }

    compare_semantic_values_by_index(left_values, right_values, field, summary,
                                     changes);
}

template <typename Item, typename Key, typename Fingerprint>
void compare_semantic_sequence_by_key(
    const std::vector<Item> &left_items, const std::vector<Item> &right_items,
    std::string_view field, Key key, Fingerprint fingerprint,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    summary.left_count = left_items.size();
    summary.right_count = right_items.size();

    auto matched_right_items = std::vector<bool>(right_items.size(), false);
    for (std::size_t left_index = 0U; left_index < left_items.size(); ++left_index) {
        const auto left_key = key(left_items[left_index]);
        auto right_index = std::optional<std::size_t>{};
        for (std::size_t candidate_index = 0U; candidate_index < right_items.size();
             ++candidate_index) {
            if (!matched_right_items[candidate_index] &&
                key(right_items[candidate_index]) == left_key) {
                right_index = candidate_index;
                break;
            }
        }

        if (!right_index.has_value()) {
            auto values = std::vector<std::string>{fingerprint(left_items[left_index])};
            append_semantic_removed_change(field, values, 0U, summary, changes);
            changes.back().left_index = left_index;
            continue;
        }

        matched_right_items[*right_index] = true;
        const auto left_value = fingerprint(left_items[left_index]);
        const auto right_value = fingerprint(right_items[*right_index]);
        if (left_value != right_value) {
            auto left_values = std::vector<std::string>{left_value};
            auto right_values = std::vector<std::string>{right_value};
            append_semantic_changed_change(field, left_values, right_values, 0U, 0U,
                                           summary, changes);
            changes.back().left_index = left_index;
            changes.back().right_index = *right_index;
        } else {
            ++summary.unchanged_count;
        }
    }

    for (std::size_t right_index = 0U; right_index < right_items.size(); ++right_index) {
        if (!matched_right_items[right_index]) {
            auto values = std::vector<std::string>{fingerprint(right_items[right_index])};
            append_semantic_added_change(field, values, 0U, summary, changes);
            changes.back().right_index = right_index;
        }
    }
}

} // namespace

std::optional<featherdoc::document_semantic_diff_result>
Document::compare_semantic(
    const Document &other,
    featherdoc::document_semantic_diff_options options) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before comparing documents",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    if (!other.is_open()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "right document must be opened before semantic comparison");
        return std::nullopt;
    }

    auto &left = const_cast<Document &>(*this);
    auto &right = const_cast<Document &>(other);
    auto result = featherdoc::document_semantic_diff_result{};

    if (options.compare_paragraphs) {
        const auto left_paragraphs = left.inspect_paragraphs();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_paragraphs = right.inspect_paragraphs();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(
            left_paragraphs, right_paragraphs, "paragraph", semantic_paragraph_value,
            options, result.paragraphs, result.paragraph_changes);
    }

    if (options.compare_tables) {
        const auto left_tables = left.inspect_tables();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_tables = right.inspect_tables();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_tables, right_tables, "table",
                                  semantic_table_value, options, result.tables,
                                  result.table_changes);
    }

    if (options.compare_images) {
        const auto left_images = this->drawing_images();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_images = other.drawing_images();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(
            left_images, right_images, "image",
            [&options](const featherdoc::drawing_image_info &image) {
                return semantic_image_value(image, options);
            },
            options, result.images, result.image_changes);
    }

    if (options.compare_content_controls) {
        const auto left_controls = this->list_content_controls();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_controls = other.list_content_controls();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(
            left_controls, right_controls, "content_control",
            [&options](const featherdoc::content_control_summary &content_control) {
                return semantic_content_control_value(content_control, options);
            },
            options, result.content_controls, result.content_control_changes);
    }

    if (options.compare_fields) {
        const auto left_fields = left.body_template().list_fields();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_fields = right.body_template().list_fields();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_fields, right_fields, "field",
                                  semantic_field_summary_value, options,
                                  result.fields, result.field_changes);
    }

    if (options.compare_styles) {
        const auto left_styles = left.list_styles();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_styles = right.list_styles();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence_by_key(
            left_styles, right_styles, "style",
            [](const featherdoc::style_summary &style) { return style.style_id; },
            semantic_style_summary_value, result.styles, result.style_changes);
    }

    if (options.compare_numbering) {
        const auto left_numbering = left.list_numbering_definitions();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_numbering = right.list_numbering_definitions();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence_by_key(
            left_numbering, right_numbering, "numbering",
            [](const featherdoc::numbering_definition_summary &definition) {
                return definition.definition_id;
            },
            semantic_numbering_definition_summary_value, result.numbering,
            result.numbering_changes);
    }

    if (options.compare_footnotes) {
        const auto left_footnotes = left.list_footnotes();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_footnotes = right.list_footnotes();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_footnotes, right_footnotes, "footnote",
                                  semantic_review_note_summary_value, options,
                                  result.footnotes, result.footnote_changes);
    }

    if (options.compare_endnotes) {
        const auto left_endnotes = left.list_endnotes();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_endnotes = right.list_endnotes();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_endnotes, right_endnotes, "endnote",
                                  semantic_review_note_summary_value, options,
                                  result.endnotes, result.endnote_changes);
    }

    if (options.compare_comments) {
        const auto left_comments = left.list_comments();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_comments = right.list_comments();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_comments, right_comments, "comment",
                                  semantic_review_note_summary_value, options,
                                  result.comments, result.comment_changes);
    }

    if (options.compare_revisions) {
        const auto left_revisions = left.list_revisions();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_revisions = right.list_revisions();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_revisions, right_revisions, "revision",
                                  semantic_revision_summary_value, options,
                                  result.revisions, result.revision_changes);
    }

    if (options.compare_template_parts) {
        auto left_parts = collect_semantic_template_parts(left, options);
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        auto right_parts = collect_semantic_template_parts(right, options);
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }

        for (const auto &part : left_parts) {
            if (is_counted_semantic_template_part(part.target)) {
                ++result.template_parts.left_count;
            }
        }
        for (const auto &part : right_parts) {
            if (is_counted_semantic_template_part(part.target)) {
                ++result.template_parts.right_count;
            }
        }

        auto matched_right_parts = std::vector<bool>(right_parts.size(), false);
        for (auto &left_part : left_parts) {
            const auto right_index = find_matching_semantic_template_part(
                right_parts, matched_right_parts, left_part.target);
            if (!right_index.has_value()) {
                if (is_counted_semantic_template_part(left_part.target)) {
                    ++result.template_parts.removed_count;
                }
                continue;
            }

            auto &right_part = right_parts[*right_index];
            auto part_result = featherdoc::document_semantic_diff_part_result{};
            part_result.target = left_part.target;
            part_result.entry_name = left_part.entry_name;
            if (right_part.entry_name != left_part.entry_name &&
                !right_part.entry_name.empty()) {
                part_result.entry_name += " -> ";
                part_result.entry_name += right_part.entry_name;
            }
            if (left_part.target.part ==
                    featherdoc::template_schema_part_kind::section_header ||
                left_part.target.part ==
                    featherdoc::template_schema_part_kind::section_footer) {
                part_result.left_resolved_from_section_index =
                    left_part.resolved_from_section_index;
                part_result.right_resolved_from_section_index =
                    right_part.resolved_from_section_index;
            }

            if (options.compare_paragraphs) {
                const auto left_paragraphs = left_part.part.inspect_paragraphs();
                if (this->last_error_info.code) {
                    return std::nullopt;
                }
                const auto right_paragraphs = right_part.part.inspect_paragraphs();
                if (other.last_error().code) {
                    this->last_error_info = other.last_error();
                    return std::nullopt;
                }
                compare_semantic_sequence(
                    left_paragraphs, right_paragraphs, "paragraph",
                    semantic_paragraph_value, options, part_result.paragraphs,
                    part_result.paragraph_changes);
            }

            if (options.compare_tables) {
                const auto left_tables = left_part.part.inspect_tables();
                if (this->last_error_info.code) {
                    return std::nullopt;
                }
                const auto right_tables = right_part.part.inspect_tables();
                if (other.last_error().code) {
                    this->last_error_info = other.last_error();
                    return std::nullopt;
                }
                compare_semantic_sequence(left_tables, right_tables, "table",
                                          semantic_table_value, options,
                                          part_result.tables,
                                          part_result.table_changes);
            }

            if (options.compare_images) {
                const auto left_images = left_part.part.drawing_images();
                if (this->last_error_info.code) {
                    return std::nullopt;
                }
                const auto right_images = right_part.part.drawing_images();
                if (other.last_error().code) {
                    this->last_error_info = other.last_error();
                    return std::nullopt;
                }
                compare_semantic_sequence(
                    left_images, right_images, "image",
                    [&options](const featherdoc::drawing_image_info &image) {
                        return semantic_image_value(image, options);
                    },
                    options, part_result.images, part_result.image_changes);
            }

            if (options.compare_content_controls) {
                const auto left_controls = left_part.part.list_content_controls();
                if (this->last_error_info.code) {
                    return std::nullopt;
                }
                const auto right_controls = right_part.part.list_content_controls();
                if (other.last_error().code) {
                    this->last_error_info = other.last_error();
                    return std::nullopt;
                }
                compare_semantic_sequence(
                    left_controls, right_controls, "content_control",
                    [&options](
                        const featherdoc::content_control_summary &content_control) {
                        return semantic_content_control_value(content_control, options);
                    },
                    options, part_result.content_controls,
                    part_result.content_control_changes);
            }

            if (options.compare_fields) {
                const auto left_fields = left_part.part.list_fields();
                if (this->last_error_info.code) {
                    return std::nullopt;
                }
                const auto right_fields = right_part.part.list_fields();
                if (other.last_error().code) {
                    this->last_error_info = other.last_error();
                    return std::nullopt;
                }
                compare_semantic_sequence(left_fields, right_fields, "field",
                                          semantic_field_summary_value, options,
                                          part_result.fields,
                                          part_result.field_changes);
            }

            if (is_counted_semantic_template_part(part_result.target)) {
                accumulate_semantic_template_part_summary(result.template_parts,
                                                          part_result);
            }
            result.template_part_results.push_back(std::move(part_result));
            matched_right_parts[*right_index] = true;
        }

        for (std::size_t index = 0U; index < right_parts.size(); ++index) {
            if (!matched_right_parts[index] &&
                is_counted_semantic_template_part(right_parts[index].target)) {
                ++result.template_parts.added_count;
            }
        }
    }

    if (options.compare_sections) {
        const auto left_sections = left.inspect_sections();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_sections = right.inspect_sections();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }

        auto left_section_values = std::vector<std::string>{};
        auto right_section_values = std::vector<std::string>{};
        left_section_values.reserve(left_sections.sections.size());
        right_section_values.reserve(right_sections.sections.size());
        for (const auto &section : left_sections.sections) {
            auto page_setup = this->get_section_page_setup(section.index);
            if (this->last_error_info.code) {
                return std::nullopt;
            }
            left_section_values.push_back(semantic_section_value(section, page_setup));
        }
        for (const auto &section : right_sections.sections) {
            auto page_setup = other.get_section_page_setup(section.index);
            if (other.last_error().code) {
                this->last_error_info = other.last_error();
                return std::nullopt;
            }
            right_section_values.push_back(semantic_section_value(section, page_setup));
        }

        result.sections.left_count = left_section_values.size();
        result.sections.right_count = right_section_values.size();
        if (can_align_semantic_values(left_section_values.size(),
                                      right_section_values.size(), options)) {
            compare_semantic_values_by_content_alignment(
                left_section_values, right_section_values, "section", result.sections,
                result.section_changes);
        } else {
            compare_semantic_values_by_index(left_section_values, right_section_values,
                                             "section", result.sections,
                                             result.section_changes);
        }
    }

    this->last_error_info.clear();
    return result;
}

} // namespace featherdoc
