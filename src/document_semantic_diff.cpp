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
#include "document_semantic_diff_field_helpers.inc"
#include "document_semantic_diff_sequence_helpers.inc"

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
