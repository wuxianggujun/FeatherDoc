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

#include <featherdoc/document_image_private_members.inc>
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
    [[nodiscard]] std::optional<bool> inspect_update_fields_on_open_enabled();
#include <featherdoc/document_sections_private_members.inc>
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
#include <featherdoc/document_sections_members.inc>
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
#include <featherdoc/document_image_members.inc>
};

} // namespace featherdoc
