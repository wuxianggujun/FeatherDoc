#include <featherdoc/fwd.hpp>
#include <featherdoc/document.hpp>
#include <featherdoc/document_core.hpp>
#include <featherdoc/reviews_fields.hpp>
#include <featherdoc/styles_numbering.hpp>
#include <featherdoc/tables.hpp>
#include <featherdoc/template_part.hpp>
#include <featherdoc/templates.hpp>
#include <featherdoc/text.hpp>
#include <featherdoc/core.hpp>

#include <type_traits>

int main() {
    static_assert(std::is_class_v<featherdoc::Document>);
    static_assert(std::is_class_v<featherdoc::Paragraph>);
    static_assert(std::is_class_v<featherdoc::Run>);
    static_assert(std::is_class_v<featherdoc::Table>);
    static_assert(std::is_class_v<featherdoc::TemplatePart>);
    static_assert(std::is_enum_v<featherdoc::style_kind>);
    static_assert(std::is_enum_v<featherdoc::style_refactor_action>);
    static_assert(std::is_enum_v<featherdoc::bookmark_kind>);
    static_assert(std::is_enum_v<featherdoc::template_slot_kind>);
    static_assert(std::is_enum_v<featherdoc::template_schema_part_kind>);
    static_assert(std::is_enum_v<featherdoc::template_onboarding_issue_severity>);
    static_assert(std::is_enum_v<featherdoc::field_kind>);
    static_assert(std::is_enum_v<featherdoc::review_note_kind>);
    static_assert(std::is_enum_v<featherdoc::revision_kind>);

    featherdoc::numbering_definition numbering_definition{};
    featherdoc::style_summary style{};
    featherdoc::style_usage_report style_usage{};
    featherdoc::style_refactor_plan style_refactor{};
    featherdoc::style_prune_plan style_prune{};
    featherdoc::paragraph_style_definition paragraph_style{};
    featherdoc::character_style_definition character_style{};
    featherdoc::table_style_definition table_style{};
    featherdoc::table_style_quality_fix_plan table_style_quality{};
    featherdoc::numbering_catalog numbering_catalog{};
    featherdoc::bookmark_fill_result bookmark_fill{};
    featherdoc::content_control_summary content_control{};
    featherdoc::section_page_setup page_setup{};
    featherdoc::floating_image_options floating_image{};
    featherdoc::paragraph_inspection_summary paragraph_summary{};
    featherdoc::run_inspection_summary run_summary{};
    featherdoc::document_semantic_diff_result semantic_diff{};
    featherdoc::sections_inspection_summary sections{};
    featherdoc::border_inspection_summary border{};
    featherdoc::table_position table_position{};
    featherdoc::table_inspection_summary table_summary{};
    featherdoc::template_table_selector table_selector{};
    featherdoc::table_cell_inspection_summary cell_summary{};
    featherdoc::bookmark_summary bookmark{};
    featherdoc::template_slot_requirement slot_requirement{};
    featherdoc::template_validation_result validation{};
    featherdoc::template_schema_patch patch{};
    featherdoc::template_onboarding_result onboarding{};
    featherdoc::field_summary field{};
    featherdoc::table_of_contents_field_options toc_options{};
    featherdoc::review_note_summary review_note{};
    featherdoc::revision_summary revision{};
    featherdoc::text_range_preview preview{};
    featherdoc::document_error_info error_info{};
    featherdoc::document_semantic_diff_options diff_options{};
    featherdoc::template_schema schema{};
    featherdoc::Paragraph paragraph{};
    featherdoc::Run run{};
    featherdoc::TemplatePart template_part{};
    featherdoc::Document doc;

    (void)numbering_definition;
    (void)style;
    (void)style_usage;
    (void)style_refactor;
    (void)style_prune;
    (void)paragraph_style;
    (void)character_style;
    (void)table_style;
    (void)table_style_quality;
    (void)numbering_catalog;
    (void)bookmark_fill;
    (void)content_control;
    (void)page_setup;
    (void)floating_image;
    (void)paragraph_summary;
    (void)run_summary;
    (void)semantic_diff;
    (void)sections;
    (void)border;
    (void)table_position;
    (void)table_summary;
    (void)table_selector;
    (void)cell_summary;
    (void)bookmark;
    (void)slot_requirement;
    (void)validation;
    (void)patch;
    (void)onboarding;
    (void)field;
    (void)toc_options;
    (void)review_note;
    (void)revision;
    (void)preview;
    (void)error_info;
    (void)diff_options;
    (void)schema;
    (void)paragraph;
    (void)run;
    (void)template_part;
    (void)doc;
    return 0;
}
