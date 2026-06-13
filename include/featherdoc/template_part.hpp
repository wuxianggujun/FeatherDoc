#pragma once

#include <featherdoc/document_core.hpp>
#include <featherdoc/reviews_fields.hpp>
#include <featherdoc/tables.hpp>
#include <featherdoc/templates.hpp>
#include <featherdoc/text.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <pugixml.hpp>

namespace featherdoc {

class Document;

class TemplatePart {
  private:
    friend class Document;
    Document *owner{nullptr};
    pugi::xml_document *xml_document{nullptr};
    document_error_info *last_error_info{nullptr};
    std::string entry_name_storage;

    TemplatePart(Document *owner, pugi::xml_document *xml_document,
                 document_error_info *last_error_info, std::string entry_name);

  public:
    TemplatePart();

    explicit operator bool() const noexcept;
    [[nodiscard]] std::string_view entry_name() const noexcept;
    Paragraph paragraphs();
    Paragraph append_paragraph(const std::string &text = {},
                               featherdoc::formatting_flag formatting =
                                   featherdoc::formatting_flag::none);
    Table tables();
    Table append_table(std::size_t row_count = 1U,
                       std::size_t column_count = 1U);
    [[nodiscard]] bool append_image(const std::filesystem::path &image_path);
    [[nodiscard]] bool append_image(const std::filesystem::path &image_path,
                                    std::uint32_t width_px,
                                    std::uint32_t height_px);
    [[nodiscard]] bool
    append_floating_image(const std::filesystem::path &image_path,
                          featherdoc::floating_image_options options = {});
    [[nodiscard]] bool
    append_floating_image(const std::filesystem::path &image_path,
                          std::uint32_t width_px, std::uint32_t height_px,
                          featherdoc::floating_image_options options = {});
    [[nodiscard]] bool
    append_page_number_field(featherdoc::field_state_options state = {});
    [[nodiscard]] bool
    append_total_pages_field(featherdoc::field_state_options state = {});
    [[nodiscard]] std::vector<featherdoc::field_summary> list_fields() const;
    [[nodiscard]] bool append_field(std::string_view instruction,
                                    std::string_view result_text = {},
                                    featherdoc::field_state_options state = {});
    [[nodiscard]] bool
    append_complex_field(std::string_view instruction,
                         std::string_view result_text = {},
                         featherdoc::complex_field_options options = {});
    [[nodiscard]] bool append_complex_field(
        std::span<const featherdoc::complex_field_instruction_fragment>
            instruction_fragments,
        std::string_view result_text = {},
        featherdoc::complex_field_options options = {});
    [[nodiscard]] bool append_complex_field(
        std::initializer_list<featherdoc::complex_field_instruction_fragment>
            instruction_fragments,
        std::string_view result_text = {},
        featherdoc::complex_field_options options = {});
    [[nodiscard]] bool append_table_of_contents_field(
        featherdoc::table_of_contents_field_options options = {},
        std::string_view result_text = "Update table of contents in Word");
    [[nodiscard]] bool
    append_reference_field(std::string_view bookmark_name,
                           featherdoc::reference_field_options options = {},
                           std::string_view result_text = {});
    [[nodiscard]] bool append_page_reference_field(
        std::string_view bookmark_name,
        featherdoc::page_reference_field_options options = {},
        std::string_view result_text = {});
    [[nodiscard]] bool append_style_reference_field(
        std::string_view style_name,
        featherdoc::style_reference_field_options options = {},
        std::string_view result_text = {});
    [[nodiscard]] bool append_document_property_field(
        std::string_view property_name,
        featherdoc::document_property_field_options options = {},
        std::string_view result_text = {});
    [[nodiscard]] bool
    append_date_field(featherdoc::date_field_options options = {},
                      std::string_view result_text = {});
    [[nodiscard]] bool
    append_hyperlink_field(std::string_view target,
                           featherdoc::hyperlink_field_options options = {},
                           std::string_view result_text = {});
    [[nodiscard]] bool
    append_sequence_field(std::string_view identifier,
                          featherdoc::sequence_field_options options = {},
                          std::string_view result_text = "1");
    [[nodiscard]] bool
    append_caption(std::string_view label, std::string_view caption_text,
                   featherdoc::caption_field_options options = {},
                   std::string_view number_result_text = "1");
    [[nodiscard]] bool
    append_index_field(featherdoc::index_field_options options = {},
                       std::string_view result_text = "Update index in Word");
    [[nodiscard]] bool
    append_index_entry_field(std::string_view entry_text,
                             featherdoc::index_entry_field_options options = {},
                             std::string_view result_text = {});
    [[nodiscard]] bool replace_field(std::size_t field_index,
                                     std::string_view instruction,
                                     std::string_view result_text = {});
    [[nodiscard]] std::vector<featherdoc::omml_summary> list_omml() const;
    [[nodiscard]] bool append_omml(std::string_view omml_xml);
    [[nodiscard]] bool replace_omml(std::size_t omml_index,
                                    std::string_view omml_xml);
    [[nodiscard]] bool remove_omml(std::size_t omml_index);
    [[nodiscard]] std::vector<featherdoc::table_inspection_summary>
    inspect_tables();
    [[nodiscard]] std::optional<featherdoc::table_inspection_summary>
    inspect_table(std::size_t table_index);
    [[nodiscard]] std::vector<featherdoc::table_cell_inspection_summary>
    inspect_table_cells(std::size_t table_index);
    [[nodiscard]] std::optional<featherdoc::table_cell_inspection_summary>
    inspect_table_cell(std::size_t table_index, std::size_t row_index,
                       std::size_t cell_index);
    [[nodiscard]] std::optional<featherdoc::table_cell_inspection_summary>
    inspect_table_cell_by_grid_column(std::size_t table_index,
                                      std::size_t row_index,
                                      std::size_t grid_column);
    [[nodiscard]] std::vector<featherdoc::paragraph_inspection_summary>
    inspect_paragraphs();
    [[nodiscard]] std::optional<featherdoc::paragraph_inspection_summary>
    inspect_paragraph(std::size_t paragraph_index);
    [[nodiscard]] std::vector<featherdoc::run_inspection_summary>
    inspect_paragraph_runs(std::size_t paragraph_index);
    [[nodiscard]] std::optional<featherdoc::run_inspection_summary>
    inspect_paragraph_run(std::size_t paragraph_index, std::size_t run_index);

    [[nodiscard]] std::size_t
    replace_bookmark_text(const std::string &bookmark_name,
                          const std::string &replacement);
    [[nodiscard]] std::size_t replace_bookmark_text(const char *bookmark_name,
                                                    const char *replacement);
    [[nodiscard]] bookmark_fill_result
    fill_bookmarks(std::span<const bookmark_text_binding> bindings);
    [[nodiscard]] bookmark_fill_result
    fill_bookmarks(std::initializer_list<bookmark_text_binding> bindings);
    [[nodiscard]] std::vector<bookmark_summary> list_bookmarks() const;
    [[nodiscard]] std::optional<bookmark_summary>
    find_bookmark(std::string_view bookmark_name) const;
    [[nodiscard]] std::vector<content_control_summary>
    list_content_controls() const;
    [[nodiscard]] std::vector<featherdoc::hyperlink_summary>
    list_hyperlinks() const;
    [[nodiscard]] std::size_t append_hyperlink(std::string_view text,
                                               std::string_view target);
    [[nodiscard]] bool replace_hyperlink(std::size_t hyperlink_index,
                                         std::string_view text,
                                         std::string_view target);
    [[nodiscard]] bool remove_hyperlink(std::size_t hyperlink_index);
    [[nodiscard]] std::vector<content_control_summary>
    find_content_controls_by_tag(std::string_view tag) const;
    [[nodiscard]] std::vector<content_control_summary>
    find_content_controls_by_alias(std::string_view alias) const;
    [[nodiscard]] std::size_t
    replace_content_control_text_by_tag(std::string_view tag,
                                        std::string_view replacement);
    [[nodiscard]] std::size_t
    replace_content_control_text_by_alias(std::string_view alias,
                                          std::string_view replacement);
    [[nodiscard]] std::size_t set_content_control_form_state_by_tag(
        std::string_view tag,
        const featherdoc::content_control_form_state_options &options);
    [[nodiscard]] std::size_t set_content_control_form_state_by_alias(
        std::string_view alias,
        const featherdoc::content_control_form_state_options &options);
    [[nodiscard]] std::size_t replace_content_control_with_paragraphs_by_tag(
        std::string_view tag, const std::vector<std::string> &paragraphs);
    [[nodiscard]] std::size_t replace_content_control_with_paragraphs_by_alias(
        std::string_view alias, const std::vector<std::string> &paragraphs);
    [[nodiscard]] std::size_t replace_content_control_with_table_rows_by_tag(
        std::string_view tag,
        const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_content_control_with_table_rows_by_alias(
        std::string_view alias,
        const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_content_control_with_table_by_tag(
        std::string_view tag,
        const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_content_control_with_table_by_alias(
        std::string_view alias,
        const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_content_control_with_image_by_tag(
        std::string_view tag, const std::filesystem::path &image_path);
    [[nodiscard]] std::size_t replace_content_control_with_image_by_tag(
        std::string_view tag, const std::filesystem::path &image_path,
        std::uint32_t width_px, std::uint32_t height_px);
    [[nodiscard]] std::size_t replace_content_control_with_image_by_alias(
        std::string_view alias, const std::filesystem::path &image_path);
    [[nodiscard]] std::size_t replace_content_control_with_image_by_alias(
        std::string_view alias, const std::filesystem::path &image_path,
        std::uint32_t width_px, std::uint32_t height_px);
    [[nodiscard]] std::optional<std::size_t>
    find_table_index_by_bookmark(std::string_view bookmark_name) const;
    [[nodiscard]] std::optional<std::size_t>
    find_table_index(const featherdoc::template_table_selector &selector) const;
    [[nodiscard]] std::optional<featherdoc::Table>
    find_table_by_bookmark(std::string_view bookmark_name);
    [[nodiscard]] std::optional<featherdoc::Table>
    find_table(const featherdoc::template_table_selector &selector);
    [[nodiscard]] template_validation_result validate_template(
        std::span<const template_slot_requirement> requirements) const;
    [[nodiscard]] template_validation_result validate_template(
        std::initializer_list<template_slot_requirement> requirements) const;
    [[nodiscard]] std::size_t replace_bookmark_with_paragraphs(
        std::string_view bookmark_name,
        const std::vector<std::string> &paragraphs);
    [[nodiscard]] std::size_t replace_bookmark_with_table_rows(
        std::string_view bookmark_name,
        const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_bookmark_with_table(
        std::string_view bookmark_name,
        const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t
    replace_bookmark_with_image(std::string_view bookmark_name,
                                const std::filesystem::path &image_path);
    [[nodiscard]] std::size_t replace_bookmark_with_image(
        std::string_view bookmark_name, const std::filesystem::path &image_path,
        std::uint32_t width_px, std::uint32_t height_px);
    [[nodiscard]] std::size_t replace_bookmark_with_floating_image(
        std::string_view bookmark_name, const std::filesystem::path &image_path,
        featherdoc::floating_image_options options = {});
    [[nodiscard]] std::size_t replace_bookmark_with_floating_image(
        std::string_view bookmark_name, const std::filesystem::path &image_path,
        std::uint32_t width_px, std::uint32_t height_px,
        featherdoc::floating_image_options options = {});
    [[nodiscard]] std::size_t
    remove_bookmark_block(std::string_view bookmark_name);
    [[nodiscard]] std::vector<drawing_image_info> drawing_images() const;
    [[nodiscard]] bool
    extract_drawing_image(std::size_t image_index,
                          const std::filesystem::path &output_path) const;
    [[nodiscard]] bool remove_drawing_image(std::size_t image_index);
    [[nodiscard]] bool
    replace_drawing_image(std::size_t image_index,
                          const std::filesystem::path &image_path);
    [[nodiscard]] std::vector<inline_image_info> inline_images() const;
    [[nodiscard]] bool
    extract_inline_image(std::size_t image_index,
                         const std::filesystem::path &output_path) const;
    [[nodiscard]] bool remove_inline_image(std::size_t image_index);
    [[nodiscard]] bool
    replace_inline_image(std::size_t image_index,
                         const std::filesystem::path &image_path);
    [[nodiscard]] std::size_t
    set_bookmark_block_visibility(std::string_view bookmark_name, bool visible);
    [[nodiscard]] bookmark_block_visibility_result
    apply_bookmark_block_visibility(
        std::span<const bookmark_block_visibility_binding> bindings);
    [[nodiscard]] bookmark_block_visibility_result
    apply_bookmark_block_visibility(
        std::initializer_list<bookmark_block_visibility_binding> bindings);
};

} // namespace featherdoc
