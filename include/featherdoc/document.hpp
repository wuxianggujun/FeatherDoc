#pragma once

#include <featherdoc/document_core.hpp>
#include <featherdoc/reviews_fields.hpp>
#include <featherdoc/styles_numbering.hpp>
#include <featherdoc/tables.hpp>
#include <featherdoc/template_part.hpp>
#include <featherdoc/templates.hpp>
#include <featherdoc/text.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

#include <pugixml.hpp>

namespace featherdoc {
// Document contains whole the docx file
// and stores paragraphs
class Document {
  private:
    friend class Table;

    struct xml_part_state {
        std::string relationship_id;
        std::string entry_name;
        std::string relationships_entry_name;
        pugi::xml_document xml;
        pugi::xml_document relationships;
        Paragraph paragraph;
        bool has_relationships_part{false};
        bool relationships_dirty{false};
    };

    struct image_part_state {
        std::string owner_entry_name;
        std::string entry_name;
        std::string content_type;
        std::string data;
    };

    [[nodiscard]] std::error_code ensure_content_types_loaded();
    [[nodiscard]] std::error_code ensure_settings_loaded();
    [[nodiscard]] std::error_code ensure_settings_part_attached();
    [[nodiscard]] std::error_code ensure_numbering_loaded();
    [[nodiscard]] std::error_code ensure_numbering_part_attached();
    [[nodiscard]] std::error_code ensure_styles_loaded();
    [[nodiscard]] std::error_code ensure_styles_part_attached();
    [[nodiscard]] std::optional<featherdoc::style_refactor_restore_result>
    restore_style_refactor(
        const std::vector<featherdoc::style_refactor_rollback_entry>
            &rollback_entries,
        bool apply_changes);
    [[nodiscard]] std::error_code ensure_even_and_odd_headers_enabled();
    [[nodiscard]] std::optional<bool> inspect_even_and_odd_headers_enabled();
    [[nodiscard]] std::optional<bool> inspect_update_fields_on_open_enabled();
    [[nodiscard]] pugi::xml_node
    section_properties(std::size_t section_index) const;
    [[nodiscard]] pugi::xml_node
    ensure_section_properties(std::size_t section_index);
    [[nodiscard]] bool
    ensure_title_page_enabled(pugi::xml_node section_properties);
    Paragraph &related_part_paragraphs_by_relationship_id(
        std::string_view relationship_id,
        std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *part_root_name);
    Paragraph &section_related_part_paragraphs(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind,
        std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *reference_name, const char *part_root_name);
    [[nodiscard]] xml_part_state *section_related_part_state(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind,
        std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *reference_name);
    Paragraph &ensure_section_related_part_paragraphs(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind,
        std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *part_root_name, const char *reference_name,
        const char *relationship_type, const char *content_type);
    Paragraph &assign_section_related_part_paragraphs(
        std::size_t section_index, std::size_t part_index,
        featherdoc::section_reference_kind reference_kind,
        std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *part_root_name, const char *reference_name);
    [[nodiscard]] bool remove_section_related_part_reference(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind,
        const char *reference_name);
    [[nodiscard]] std::error_code ensure_removal_prerequisites_loaded();
    void cleanup_first_page_section_markers();
    [[nodiscard]] std::error_code cleanup_even_and_odd_headers_setting();
    [[nodiscard]] bool
    remove_related_part(std::size_t part_index,
                        std::vector<std::unique_ptr<xml_part_state>> &parts,
                        const char *reference_name);
    [[nodiscard]] bool
    move_related_part(std::size_t source_index, std::size_t target_index,
                      std::vector<std::unique_ptr<xml_part_state>> &parts,
                      const char *relationship_type);
    [[nodiscard]] bool
    copy_section_related_part_references(std::size_t source_section_index,
                                         std::size_t target_section_index,
                                         const char *reference_name);
    Paragraph &ensure_related_part_paragraphs(
        std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *part_root_name, const char *reference_name,
        const char *relationship_type, const char *content_type);
    [[nodiscard]] xml_part_state *
    find_related_part_state(std::string_view entry_name);
    [[nodiscard]] const xml_part_state *
    find_related_part_state(std::string_view entry_name) const;
    [[nodiscard]] std::uint32_t next_drawing_object_id() const;
    [[nodiscard]] bool
    append_inline_image_part(std::string image_data, std::string extension,
                             std::string content_type, std::string display_name,
                             std::uint32_t width_px, std::uint32_t height_px);
    [[nodiscard]] bool
    append_inline_image_part(pugi::xml_node parent,
                             pugi::xml_node insert_before,
                             std::string image_data, std::string extension,
                             std::string content_type, std::string display_name,
                             std::uint32_t width_px, std::uint32_t height_px);
    [[nodiscard]] bool append_inline_image_part(
        pugi::xml_document &xml_document, std::string_view xml_entry_name,
        pugi::xml_document &relationships_document,
        std::string_view relationships_entry_name, bool &has_relationships_part,
        bool &relationships_dirty, pugi::xml_node parent,
        pugi::xml_node insert_before, std::string image_data,
        std::string extension, std::string content_type,
        std::string display_name, std::uint32_t width_px,
        std::uint32_t height_px);
    [[nodiscard]] bool append_drawing_image_part(
        pugi::xml_document &xml_document, std::string_view xml_entry_name,
        pugi::xml_document &relationships_document,
        std::string_view relationships_entry_name, bool &has_relationships_part,
        bool &relationships_dirty, pugi::xml_node parent,
        pugi::xml_node insert_before, std::string image_data,
        std::string extension, std::string content_type,
        std::string display_name, std::uint32_t width_px,
        std::uint32_t height_px,
        std::optional<featherdoc::floating_image_options> floating_options);
    [[nodiscard]] bool append_floating_image_part(
        std::string image_data, std::string extension, std::string content_type,
        std::string display_name, std::uint32_t width_px,
        std::uint32_t height_px, featherdoc::floating_image_options options);
    [[nodiscard]] bool append_floating_image_part(
        pugi::xml_node parent, pugi::xml_node insert_before,
        std::string image_data, std::string extension, std::string content_type,
        std::string display_name, std::uint32_t width_px,
        std::uint32_t height_px, featherdoc::floating_image_options options);
    [[nodiscard]] bool append_floating_image_part(
        pugi::xml_document &xml_document, std::string_view xml_entry_name,
        pugi::xml_document &relationships_document,
        std::string_view relationships_entry_name, bool &has_relationships_part,
        bool &relationships_dirty, pugi::xml_node parent,
        pugi::xml_node insert_before, std::string image_data,
        std::string extension, std::string content_type,
        std::string display_name, std::uint32_t width_px,
        std::uint32_t height_px, featherdoc::floating_image_options options);
    [[nodiscard]] std::vector<drawing_image_info>
    drawing_images_in_part(std::string_view entry_name) const;
    [[nodiscard]] std::vector<featherdoc::hyperlink_summary>
    list_hyperlinks_in_part(pugi::xml_document &xml_document,
                            std::string_view entry_name) const;
    [[nodiscard]] std::size_t
    append_hyperlink_in_part(pugi::xml_document &xml_document,
                             std::string_view entry_name, std::string_view text,
                             std::string_view target);
    [[nodiscard]] bool
    replace_hyperlink_in_part(pugi::xml_document &xml_document,
                              std::string_view entry_name,
                              std::size_t hyperlink_index,
                              std::string_view text, std::string_view target);
    [[nodiscard]] bool
    remove_hyperlink_in_part(pugi::xml_document &xml_document,
                             std::string_view entry_name,
                             std::size_t hyperlink_index);
    [[nodiscard]] bool
    ensure_review_notes_part(featherdoc::review_note_kind kind);
    [[nodiscard]] bool ensure_comments_part();
    [[nodiscard]] bool ensure_comments_extended_loaded();
    [[nodiscard]] bool ensure_comments_extended_part();
    [[nodiscard]] bool extract_drawing_image_from_part(
        std::string_view entry_name, std::size_t image_index,
        const std::filesystem::path &output_path) const;
    [[nodiscard]] bool remove_drawing_image_in_part(std::string_view entry_name,
                                                    std::size_t image_index);
    [[nodiscard]] bool
    replace_drawing_image_in_part(std::string_view entry_name,
                                  std::size_t image_index,
                                  const std::filesystem::path &image_path);
    [[nodiscard]] std::vector<inline_image_info>
    inline_images_in_part(std::string_view entry_name) const;
    [[nodiscard]] bool extract_inline_image_from_part(
        std::string_view entry_name, std::size_t image_index,
        const std::filesystem::path &output_path) const;
    [[nodiscard]] bool remove_inline_image_in_part(std::string_view entry_name,
                                                   std::size_t image_index);
    [[nodiscard]] bool
    replace_inline_image_in_part(std::string_view entry_name,
                                 std::size_t image_index,
                                 const std::filesystem::path &image_path);
    [[nodiscard]] std::size_t replace_bookmark_with_image_in_part(
        pugi::xml_document &xml_document, std::string_view entry_name,
        std::string_view bookmark_name, const std::filesystem::path &image_path,
        std::optional<std::pair<std::uint32_t, std::uint32_t>> dimensions);
    [[nodiscard]] std::size_t replace_content_control_with_image_in_part(
        pugi::xml_document &xml_document, std::string_view entry_name,
        std::string_view value, bool match_tag,
        const std::filesystem::path &image_path,
        std::optional<std::pair<std::uint32_t, std::uint32_t>> dimensions);
    [[nodiscard]] std::size_t replace_bookmark_with_floating_image_in_part(
        pugi::xml_document &xml_document, std::string_view entry_name,
        std::string_view bookmark_name, const std::filesystem::path &image_path,
        std::optional<std::pair<std::uint32_t, std::uint32_t>> dimensions,
        featherdoc::floating_image_options options);

    friend class IteratorHelper;
    friend class TemplatePart;
    std::filesystem::path document_path;
    Paragraph paragraph;
    Paragraph detached_paragraph;
    Table table;
    pugi::xml_document document;
    pugi::xml_document document_relationships;
    pugi::xml_document content_types;
    pugi::xml_document settings;
    pugi::xml_document numbering;
    pugi::xml_document styles;
    pugi::xml_document footnotes;
    pugi::xml_document endnotes;
    pugi::xml_document comments;
    pugi::xml_document comments_extended;
    std::vector<std::unique_ptr<xml_part_state>> header_parts;
    std::vector<std::unique_ptr<xml_part_state>> footer_parts;
    std::vector<image_part_state> image_parts;
    bool flag_is_open{false};
    bool has_source_archive{false};
    bool has_document_relationships_part{false};
    bool has_settings_part{false};
    bool has_numbering_part{false};
    bool has_styles_part{false};
    bool has_footnotes_part{false};
    bool has_endnotes_part{false};
    bool has_comments_part{false};
    bool has_comments_extended_part{false};
    bool document_relationships_dirty{false};
    bool content_types_loaded{false};
    bool content_types_dirty{false};
    bool settings_loaded{false};
    bool settings_dirty{false};
    bool numbering_loaded{false};
    bool numbering_dirty{false};
    bool styles_loaded{false};
    bool styles_dirty{false};
    bool footnotes_loaded{false};
    bool footnotes_dirty{false};
    bool endnotes_loaded{false};
    bool endnotes_dirty{false};
    bool comments_loaded{false};
    bool comments_dirty{false};
    bool comments_extended_loaded{false};
    bool comments_extended_dirty{false};
    mutable std::unordered_set<std::string> removed_related_part_entries;
    std::unordered_set<std::string> removed_archive_entries;
    mutable document_error_info last_error_info;

  public:
    Document();
    explicit Document(std::filesystem::path);
    [[nodiscard]] std::error_code create_empty();
    [[nodiscard]] bool append_section(bool inherit_header_footer = true);
    [[nodiscard]] bool insert_section(std::size_t section_index,
                                      bool inherit_header_footer = true);
    [[nodiscard]] bool remove_section(std::size_t section_index);
    [[nodiscard]] bool move_section(std::size_t source_section_index,
                                    std::size_t target_section_index);
    void set_path(std::filesystem::path);
    [[nodiscard]] const std::filesystem::path &path() const;
    [[nodiscard]] std::error_code open();
    [[nodiscard]] bool enable_update_fields_on_open();
    [[nodiscard]] bool clear_update_fields_on_open();
    [[nodiscard]] std::optional<bool> update_fields_on_open_enabled();
    [[nodiscard]] std::error_code save() const;
    [[nodiscard]] std::error_code save_as(std::filesystem::path) const;
    [[nodiscard]] bool is_open() const;
    [[nodiscard]] const document_error_info &last_error() const noexcept;
    [[nodiscard]] std::size_t section_count() const noexcept;
    [[nodiscard]] std::size_t header_count() const noexcept;
    [[nodiscard]] std::size_t footer_count() const noexcept;
    [[nodiscard]] featherdoc::sections_inspection_summary inspect_sections();
    [[nodiscard]] std::optional<featherdoc::section_inspection_summary>
    inspect_section(std::size_t section_index);
    [[nodiscard]] std::optional<section_page_setup>
    get_section_page_setup(std::size_t section_index) const;
    [[nodiscard]] bool set_section_page_setup(std::size_t section_index,
                                              const section_page_setup &setup);
    [[nodiscard]] TemplatePart body_template();
    [[nodiscard]] TemplatePart header_template(std::size_t index = 0U);
    [[nodiscard]] TemplatePart footer_template(std::size_t index = 0U);
    [[nodiscard]] TemplatePart section_header_template(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    [[nodiscard]] TemplatePart section_footer_template(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    Paragraph &header_paragraphs(std::size_t index = 0U);
    Paragraph &footer_paragraphs(std::size_t index = 0U);
    Paragraph &section_header_paragraphs(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    Paragraph &section_footer_paragraphs(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    Paragraph &ensure_section_header_paragraphs(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    Paragraph &ensure_section_footer_paragraphs(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    Paragraph &assign_section_header_paragraphs(
        std::size_t section_index, std::size_t header_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    Paragraph &assign_section_footer_paragraphs(
        std::size_t section_index, std::size_t footer_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    [[nodiscard]] bool remove_section_header_reference(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    [[nodiscard]] bool remove_section_footer_reference(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    [[nodiscard]] bool remove_header_part(std::size_t index);
    [[nodiscard]] bool remove_footer_part(std::size_t index);
    [[nodiscard]] bool move_header_part(std::size_t source_index,
                                        std::size_t target_index);
    [[nodiscard]] bool move_footer_part(std::size_t source_index,
                                        std::size_t target_index);
    [[nodiscard]] bool
    copy_section_header_references(std::size_t source_section_index,
                                   std::size_t target_section_index);
    [[nodiscard]] bool
    copy_section_footer_references(std::size_t source_section_index,
                                   std::size_t target_section_index);
    [[nodiscard]] bool replace_section_header_text(
        std::size_t section_index, std::string_view replacement_text,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    [[nodiscard]] bool replace_section_footer_text(
        std::size_t section_index, std::string_view replacement_text,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    Paragraph &ensure_header_paragraphs();
    Paragraph &ensure_footer_paragraphs();
#include <featherdoc/document_reviews_fields_members.inc>
#include <featherdoc/document_templates_members.inc>
#include <featherdoc/document_styles_numbering_members.inc>
    [[nodiscard]] std::vector<featherdoc::table_inspection_summary>
    inspect_tables();
    [[nodiscard]] std::optional<featherdoc::table_inspection_summary>
    inspect_table(std::size_t table_index);
    [[nodiscard]] featherdoc::table_style_look_consistency_report
    check_table_style_look_consistency();
    [[nodiscard]] featherdoc::table_style_look_repair_report
    repair_table_style_look_consistency();
    [[nodiscard]] featherdoc::table_style_quality_audit_report
    audit_table_style_quality();
    [[nodiscard]] featherdoc::table_style_quality_fix_plan
    plan_table_style_quality_fixes();
    [[nodiscard]] std::vector<featherdoc::body_block_inspection_summary>
    inspect_body_blocks();
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
    [[nodiscard]] std::vector<featherdoc::omml_summary> list_omml() const;
    [[nodiscard]] bool append_omml(std::string_view omml_xml);
    [[nodiscard]] bool replace_omml(std::size_t omml_index,
                                    std::string_view omml_xml);
    [[nodiscard]] bool remove_omml(std::size_t omml_index);

    Paragraph &paragraphs();
    Table &tables();
    Table append_table(std::size_t row_count = 1U,
                       std::size_t column_count = 1U);
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
};

} // namespace featherdoc
