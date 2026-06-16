#pragma once

#include <optional>
#include <string>

#include <featherdoc/document_core.hpp>
#include <featherdoc/reviews_fields.hpp>
#include <featherdoc/styles_numbering.hpp>
#include <featherdoc/tables.hpp>
#include <featherdoc/templates.hpp>

namespace featherdoc {

[[nodiscard]] auto semantic_paragraph_value(
    const featherdoc::paragraph_inspection_summary &paragraph) -> std::string;

[[nodiscard]] auto semantic_table_value(
    const featherdoc::table_inspection_summary &table) -> std::string;

[[nodiscard]] auto semantic_image_value(
    const featherdoc::drawing_image_info &image,
    const featherdoc::document_semantic_diff_options &options) -> std::string;

[[nodiscard]] auto semantic_content_control_value(
    const featherdoc::content_control_summary &content_control,
    const featherdoc::document_semantic_diff_options &options) -> std::string;

[[nodiscard]] auto semantic_field_summary_value(
    const featherdoc::field_summary &field) -> std::string;

[[nodiscard]] auto semantic_style_summary_value(
    const featherdoc::style_summary &style) -> std::string;

[[nodiscard]] auto semantic_numbering_definition_summary_value(
    const featherdoc::numbering_definition_summary &definition) -> std::string;

[[nodiscard]] auto semantic_review_note_summary_value(
    const featherdoc::review_note_summary &note) -> std::string;

[[nodiscard]] auto semantic_revision_summary_value(
    const featherdoc::revision_summary &revision) -> std::string;

[[nodiscard]] auto semantic_section_value(
    const featherdoc::section_inspection_summary &section,
    const std::optional<featherdoc::section_page_setup> &page_setup)
    -> std::string;

} // namespace featherdoc
