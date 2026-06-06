#pragma once

#include <constants.hpp>
#include <featherdoc/templates.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace featherdoc {

struct document_error_info {
    std::error_code code{};
    std::string detail;
    std::string entry_name;
    std::optional<std::ptrdiff_t> xml_offset;

    explicit operator bool() const noexcept {
        return static_cast<bool>(this->code);
    }

    void clear() {
        this->code.clear();
        this->detail.clear();
        this->entry_name.clear();
        this->xml_offset.reset();
    }
};

struct bookmark_text_binding {
    std::string bookmark_name;
    std::string text;
};

struct bookmark_fill_result {
    std::size_t requested{};
    std::size_t matched{};
    std::size_t replaced{};
    std::vector<std::string> missing_bookmarks;

    explicit operator bool() const noexcept {
        return this->missing_bookmarks.empty();
    }
};

struct bookmark_block_visibility_binding {
    std::string bookmark_name;
    bool visible{true};
};

struct bookmark_block_visibility_result {
    std::size_t requested{};
    std::size_t matched{};
    std::size_t kept{};
    std::size_t removed{};
    std::vector<std::string> missing_bookmarks;

    explicit operator bool() const noexcept {
        return this->missing_bookmarks.empty();
    }
};

enum class content_control_kind : std::uint8_t {
    block = 0U,
    run,
    table_row,
    table_cell,
    unknown,
};

enum class content_control_form_kind : std::uint8_t {
    rich_text = 0U,
    plain_text,
    picture,
    checkbox,
    drop_down_list,
    combo_box,
    date,
    repeating_section,
    group,
    unknown,
};

struct content_control_list_item {
    std::string display_text;
    std::string value;
};

struct content_control_summary {
    std::size_t index{};
    featherdoc::content_control_kind kind{
        featherdoc::content_control_kind::unknown};
    featherdoc::content_control_form_kind form_kind{
        featherdoc::content_control_form_kind::rich_text};
    std::optional<std::string> tag;
    std::optional<std::string> alias;
    std::optional<std::string> id;
    std::optional<std::string> lock;
    std::optional<std::string> data_binding_store_item_id;
    std::optional<std::string> data_binding_xpath;
    std::optional<std::string> data_binding_prefix_mappings;
    std::optional<bool> checked;
    std::optional<std::string> date_format;
    std::optional<std::string> date_locale;
    std::optional<std::size_t> selected_list_item;
    std::vector<featherdoc::content_control_list_item> list_items;
    bool showing_placeholder{false};
    std::string text;

    [[nodiscard]] bool has_tag() const noexcept {
        return this->tag.has_value() && !this->tag->empty();
    }

    [[nodiscard]] bool has_alias() const noexcept {
        return this->alias.has_value() && !this->alias->empty();
    }
};

struct content_control_form_state_options {
    std::optional<std::string> lock;
    bool clear_lock{false};
    bool clear_data_binding{false};
    std::optional<std::string> data_binding_store_item_id;
    std::optional<std::string> data_binding_xpath;
    std::optional<std::string> data_binding_prefix_mappings;
    std::optional<bool> checked;
    std::optional<std::string> selected_list_item;
    std::optional<std::string> date_text;
    std::optional<std::string> date_format;
    std::optional<std::string> date_locale;
};

struct custom_xml_data_binding_sync_item {
    std::string part_entry_name;
    std::size_t content_control_index{};
    std::optional<std::string> tag;
    std::optional<std::string> alias;
    std::string store_item_id;
    std::string xpath;
    std::string previous_text;
    std::string value;
};

struct custom_xml_data_binding_sync_issue {
    std::string part_entry_name;
    std::optional<std::size_t> content_control_index;
    std::optional<std::string> tag;
    std::optional<std::string> alias;
    std::string store_item_id;
    std::string xpath;
    std::string reason;
};

struct custom_xml_data_binding_sync_result {
    std::size_t scanned_content_controls{};
    std::size_t bound_content_controls{};
    std::size_t synced_content_controls{};
    std::vector<featherdoc::custom_xml_data_binding_sync_item> synced_items;
    std::vector<featherdoc::custom_xml_data_binding_sync_issue> issues;

    [[nodiscard]] bool has_issues() const noexcept {
        return !this->issues.empty();
    }
};

enum class page_orientation : std::uint8_t {
    portrait = 0U,
    landscape,
};

struct page_margins {
    std::uint32_t top_twips{};
    std::uint32_t bottom_twips{};
    std::uint32_t left_twips{};
    std::uint32_t right_twips{};
    std::uint32_t header_twips{};
    std::uint32_t footer_twips{};
};

struct section_page_setup {
    featherdoc::page_orientation orientation{
        featherdoc::page_orientation::portrait};
    std::uint32_t width_twips{};
    std::uint32_t height_twips{};
    featherdoc::page_margins margins{};
    std::optional<std::uint32_t> page_number_start;
};

struct inline_image_info {
    std::size_t index{};
    std::string relationship_id;
    std::string entry_name;
    std::string display_name;
    std::string content_type;
    std::uint32_t width_px{};
    std::uint32_t height_px{};
    std::optional<std::size_t> body_block_index;
    std::optional<std::size_t> paragraph_index;
};

enum class drawing_image_placement {
    inline_object = 0,
    anchored_object,
};

enum class floating_image_horizontal_reference : std::uint8_t {
    page = 0U,
    margin,
    column,
    character,
};

enum class floating_image_vertical_reference : std::uint8_t {
    page = 0U,
    margin,
    paragraph,
    line,
};

enum class floating_image_wrap_mode : std::uint8_t {
    none = 0U,
    square,
    top_bottom,
};

struct floating_image_crop {
    std::uint32_t left_per_mille{0};
    std::uint32_t top_per_mille{0};
    std::uint32_t right_per_mille{0};
    std::uint32_t bottom_per_mille{0};
};

struct floating_image_options {
    featherdoc::floating_image_horizontal_reference horizontal_reference{
        featherdoc::floating_image_horizontal_reference::column};
    std::int32_t horizontal_offset_px{0};
    featherdoc::floating_image_vertical_reference vertical_reference{
        featherdoc::floating_image_vertical_reference::paragraph};
    std::int32_t vertical_offset_px{0};
    bool behind_text{false};
    bool allow_overlap{true};
    std::uint32_t z_order{0};
    featherdoc::floating_image_wrap_mode wrap_mode{
        featherdoc::floating_image_wrap_mode::none};
    std::uint32_t wrap_distance_left_px{0};
    std::uint32_t wrap_distance_right_px{0};
    std::uint32_t wrap_distance_top_px{0};
    std::uint32_t wrap_distance_bottom_px{0};
    std::optional<featherdoc::floating_image_crop> crop;
};

struct drawing_image_info {
    std::size_t index{};
    drawing_image_placement placement{drawing_image_placement::inline_object};
    std::string relationship_id;
    std::string entry_name;
    std::string display_name;
    std::string content_type;
    std::uint32_t width_px{};
    std::uint32_t height_px{};
    std::optional<featherdoc::floating_image_options> floating_options;
    std::optional<std::size_t> body_block_index;
    std::optional<std::size_t> paragraph_index;
};

struct paragraph_inspection_summary {
    struct numbering_summary {
        std::optional<std::uint32_t> num_id;
        std::optional<std::uint32_t> level;
        std::optional<std::uint32_t> definition_id;
        std::optional<std::string> definition_name;
    };

    std::size_t index{0};
    std::optional<std::string> style_id;
    std::optional<bool> bidi;
    std::optional<featherdoc::paragraph_alignment> alignment;
    std::optional<std::uint32_t> indent_left_twips;
    std::optional<std::uint32_t> indent_right_twips;
    std::optional<std::uint32_t> first_line_indent_twips;
    std::optional<std::uint32_t> hanging_indent_twips;
    std::optional<numbering_summary> numbering;
    std::size_t run_count{0};
    std::string text;
};

enum class body_block_kind : std::uint8_t {
    paragraph = 0U,
    table,
};

struct body_block_inspection_summary {
    featherdoc::body_block_kind kind{featherdoc::body_block_kind::paragraph};
    std::size_t block_index{0};
    std::size_t item_index{0};
    std::size_t section_index{0};
};

enum class document_semantic_diff_change_kind : std::uint8_t {
    added = 0U,
    removed,
    changed,
};

struct document_semantic_diff_options {
    bool compare_paragraphs{true};
    bool compare_tables{true};
    bool compare_images{true};
    bool compare_content_controls{true};
    bool compare_fields{true};
    bool compare_styles{true};
    bool compare_numbering{true};
    bool compare_footnotes{true};
    bool compare_endnotes{true};
    bool compare_comments{true};
    bool compare_revisions{true};
    bool compare_sections{true};
    bool compare_template_parts{true};
    bool compare_resolved_section_template_parts{true};
    bool compare_image_relationship_ids{false};
    bool compare_content_control_ids{false};
    bool align_sequences_by_content{true};
    std::size_t alignment_cell_limit{250000U};
};

struct document_semantic_diff_category_summary {
    std::size_t left_count{0};
    std::size_t right_count{0};
    std::size_t added_count{0};
    std::size_t removed_count{0};
    std::size_t changed_count{0};
    std::size_t unchanged_count{0};

    [[nodiscard]] std::size_t change_count() const noexcept {
        return this->added_count + this->removed_count + this->changed_count;
    }

    [[nodiscard]] bool different() const noexcept {
        return this->change_count() != 0U;
    }
};

struct document_semantic_diff_field_change {
    std::string field_path;
    std::string left_value;
    std::string right_value;
};

struct document_semantic_diff_change {
    featherdoc::document_semantic_diff_change_kind kind{
        featherdoc::document_semantic_diff_change_kind::changed};
    std::optional<std::size_t> left_index;
    std::optional<std::size_t> right_index;
    std::string field;
    std::string left_value;
    std::string right_value;
    std::vector<featherdoc::document_semantic_diff_field_change> field_changes;
};

struct document_semantic_diff_part_result {
    featherdoc::template_schema_part_selector target{};
    std::string entry_name;
    std::optional<std::size_t> left_resolved_from_section_index;
    std::optional<std::size_t> right_resolved_from_section_index;
    featherdoc::document_semantic_diff_category_summary paragraphs;
    featherdoc::document_semantic_diff_category_summary tables;
    featherdoc::document_semantic_diff_category_summary images;
    featherdoc::document_semantic_diff_category_summary content_controls;
    featherdoc::document_semantic_diff_category_summary fields;
    std::vector<featherdoc::document_semantic_diff_change> paragraph_changes;
    std::vector<featherdoc::document_semantic_diff_change> table_changes;
    std::vector<featherdoc::document_semantic_diff_change> image_changes;
    std::vector<featherdoc::document_semantic_diff_change>
        content_control_changes;
    std::vector<featherdoc::document_semantic_diff_change> field_changes;

    [[nodiscard]] std::size_t change_count() const noexcept {
        return this->paragraphs.change_count() + this->tables.change_count() +
               this->images.change_count() +
               this->content_controls.change_count() +
               this->fields.change_count();
    }

    [[nodiscard]] bool different() const noexcept {
        return this->change_count() != 0U;
    }
};

struct document_semantic_diff_result {
    featherdoc::document_semantic_diff_category_summary paragraphs;
    featherdoc::document_semantic_diff_category_summary tables;
    featherdoc::document_semantic_diff_category_summary images;
    featherdoc::document_semantic_diff_category_summary content_controls;
    featherdoc::document_semantic_diff_category_summary fields;
    featherdoc::document_semantic_diff_category_summary styles;
    featherdoc::document_semantic_diff_category_summary numbering;
    featherdoc::document_semantic_diff_category_summary footnotes;
    featherdoc::document_semantic_diff_category_summary endnotes;
    featherdoc::document_semantic_diff_category_summary comments;
    featherdoc::document_semantic_diff_category_summary revisions;
    featherdoc::document_semantic_diff_category_summary sections;
    featherdoc::document_semantic_diff_category_summary template_parts;
    std::vector<featherdoc::document_semantic_diff_change> paragraph_changes;
    std::vector<featherdoc::document_semantic_diff_change> table_changes;
    std::vector<featherdoc::document_semantic_diff_change> image_changes;
    std::vector<featherdoc::document_semantic_diff_change>
        content_control_changes;
    std::vector<featherdoc::document_semantic_diff_change> field_changes;
    std::vector<featherdoc::document_semantic_diff_change> style_changes;
    std::vector<featherdoc::document_semantic_diff_change> numbering_changes;
    std::vector<featherdoc::document_semantic_diff_change> footnote_changes;
    std::vector<featherdoc::document_semantic_diff_change> endnote_changes;
    std::vector<featherdoc::document_semantic_diff_change> comment_changes;
    std::vector<featherdoc::document_semantic_diff_change> revision_changes;
    std::vector<featherdoc::document_semantic_diff_change> section_changes;
    std::vector<featherdoc::document_semantic_diff_part_result>
        template_part_results;

    [[nodiscard]] std::size_t change_count() const noexcept {
        return this->paragraphs.change_count() + this->tables.change_count() +
               this->images.change_count() +
               this->content_controls.change_count() +
               this->fields.change_count() + this->styles.change_count() +
               this->numbering.change_count() + this->footnotes.change_count() +
               this->endnotes.change_count() + this->comments.change_count() +
               this->revisions.change_count() + this->sections.change_count() +
               this->template_parts.change_count();
    }

    [[nodiscard]] bool different() const noexcept {
        return this->change_count() != 0U;
    }
};

struct run_inspection_summary {
    std::size_t index{0};
    std::optional<std::string> style_id;
    std::optional<std::string> font_family;
    std::optional<std::string> east_asia_font_family;
    std::optional<std::string> text_color;
    std::optional<bool> bold;
    std::optional<bool> italic;
    std::optional<bool> strikethrough;
    std::optional<bool> underline;
    std::optional<bool> superscript;
    std::optional<bool> subscript;
    std::optional<double> font_size_points;
    std::optional<std::string> language;
    std::optional<std::string> east_asia_language;
    std::optional<std::string> bidi_language;
    std::optional<bool> rtl;
    std::string text;
};

struct section_part_inspection_summary {
    bool has_default{false};
    bool has_first{false};
    bool has_even{false};
    bool default_linked_to_previous{false};
    bool first_linked_to_previous{false};
    bool even_linked_to_previous{false};
    std::optional<std::string> default_entry_name;
    std::optional<std::string> first_entry_name;
    std::optional<std::string> even_entry_name;
    std::optional<std::string> resolved_default_entry_name;
    std::optional<std::string> resolved_first_entry_name;
    std::optional<std::string> resolved_even_entry_name;
    std::optional<std::size_t> resolved_default_section_index;
    std::optional<std::size_t> resolved_first_section_index;
    std::optional<std::size_t> resolved_even_section_index;
};

struct section_inspection_summary {
    std::size_t index{0};
    std::optional<bool> even_and_odd_headers_enabled;
    bool different_first_page_enabled{false};
    featherdoc::section_part_inspection_summary header;
    featherdoc::section_part_inspection_summary footer;
};

struct sections_inspection_summary {
    std::size_t section_count{0};
    std::size_t header_count{0};
    std::size_t footer_count{0};
    std::optional<bool> even_and_odd_headers_enabled;
    std::vector<featherdoc::section_inspection_summary> sections;
};

} // namespace featherdoc
