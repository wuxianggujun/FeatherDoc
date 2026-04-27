/*
 * FeatherDoc
 * Original upstream author: Amir Mohamadi (@amiremohamadi)
 * Current fork branding, licensing, and maintenance notes: see README,
 * LICENSE, and LICENSE.upstream-mit.
 * Licensing: see LICENSE and LICENSE.upstream-mit.
 */

#ifndef FEATHERDOC_HPP
#define FEATHERDOC_HPP

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
#include <utility>
#include <unordered_set>
#include <vector>

#include <constants.hpp>
#include <featherdoc_iterator.hpp>
#include <pugixml.hpp>
#include <zip.h>

namespace featherdoc {
class Document;

// Run contains runs in a paragraph
class Run {
  private:
    friend class IteratorHelper;
    friend class Document;
    // Store the parent node (a paragraph)
    pugi::xml_node parent;
    // And store current node also
    pugi::xml_node current;

  public:
    Run();
    Run(pugi::xml_node, pugi::xml_node);
    void set_parent(pugi::xml_node);
    void set_current(pugi::xml_node);

    [[nodiscard]] std::string get_text() const;
    [[nodiscard]] bool set_text(const std::string &) const;
    [[nodiscard]] bool set_text(const char *) const;
    [[nodiscard]] std::optional<std::string> style_id() const;
    [[nodiscard]] std::optional<std::string> font_family() const;
    [[nodiscard]] std::optional<std::string> east_asia_font_family() const;
    [[nodiscard]] std::optional<std::string> language() const;
    [[nodiscard]] std::optional<std::string> east_asia_language() const;
    [[nodiscard]] std::optional<std::string> bidi_language() const;
    [[nodiscard]] std::optional<bool> rtl() const;
    [[nodiscard]] bool set_font_family(std::string_view font_family) const;
    [[nodiscard]] bool set_east_asia_font_family(std::string_view font_family) const;
    [[nodiscard]] bool set_language(std::string_view language) const;
    [[nodiscard]] bool set_east_asia_language(std::string_view language) const;
    [[nodiscard]] bool set_bidi_language(std::string_view language) const;
    [[nodiscard]] bool set_rtl(bool enabled = true) const;
    [[nodiscard]] bool clear_font_family() const;
    [[nodiscard]] bool clear_east_asia_font_family() const;
    [[nodiscard]] bool clear_primary_language() const;
    [[nodiscard]] bool clear_language() const;
    [[nodiscard]] bool clear_east_asia_language() const;
    [[nodiscard]] bool clear_bidi_language() const;
    [[nodiscard]] bool clear_rtl() const;
    [[nodiscard]] bool remove();
    Run insert_run_before(const std::string &,
                          featherdoc::formatting_flag =
                              featherdoc::formatting_flag::none);
    Run insert_run_after(const std::string &,
                         featherdoc::formatting_flag =
                             featherdoc::formatting_flag::none);
    Run insert_run_like_before();
    Run insert_run_like_after();

    Run &next();
    [[nodiscard]] bool has_next() const;
};

// Paragraph contains a paragraph
// and stores runs
class Paragraph {
  private:
    friend class IteratorHelper;
    friend class Document;
    friend class TemplatePart;
    // Store parent node (usually the body node)
    pugi::xml_node parent;
    // And store current node also
    pugi::xml_node current;
    // A paragraph consists of runs
    Run run;

  public:
    Paragraph();
    Paragraph(pugi::xml_node, pugi::xml_node);
    void set_parent(pugi::xml_node);
    void set_current(pugi::xml_node);

    Paragraph &next();
    [[nodiscard]] bool has_next() const;
    [[nodiscard]] std::optional<bool> bidi() const;
    [[nodiscard]] bool set_bidi(bool enabled = true) const;
    [[nodiscard]] bool clear_bidi() const;
    [[nodiscard]] bool set_text(const std::string &) const;
    [[nodiscard]] bool set_text(const char *) const;
    [[nodiscard]] bool remove();

    Run &runs();
    Run add_run(const std::string &,
                featherdoc::formatting_flag = featherdoc::formatting_flag::none);
    Run add_run(const char *,
                featherdoc::formatting_flag = featherdoc::formatting_flag::none);
    Paragraph insert_paragraph_before(const std::string &,
                                      featherdoc::formatting_flag =
                                          featherdoc::formatting_flag::none);
    Paragraph insert_paragraph_after(const std::string &,
                                     featherdoc::formatting_flag =
                                         featherdoc::formatting_flag::none);
    Paragraph insert_paragraph_like_before();
    Paragraph insert_paragraph_like_after();
};

// TableCell contains one or more paragraphs
class TableCell {
  private:
    friend class IteratorHelper;
    pugi::xml_node parent;
    pugi::xml_node current;

    Paragraph paragraph;

  public:
    TableCell();
    TableCell(pugi::xml_node, pugi::xml_node);

    void set_parent(pugi::xml_node);
    void set_current(pugi::xml_node);

    Paragraph &paragraphs();
    [[nodiscard]] std::string get_text() const;
    [[nodiscard]] bool set_text(const std::string &) const;
    [[nodiscard]] bool set_text(const char *) const;
    [[nodiscard]] bool remove();
    TableCell insert_cell_before();
    TableCell insert_cell_after();
    [[nodiscard]] std::optional<std::uint32_t> width_twips() const;
    [[nodiscard]] bool set_width_twips(std::uint32_t width_twips);
    [[nodiscard]] bool clear_width();
    [[nodiscard]] std::size_t column_span() const;
    [[nodiscard]] bool merge_right(std::size_t additional_cells = 1U);
    [[nodiscard]] bool merge_down(std::size_t additional_rows = 1U);
    [[nodiscard]] bool unmerge_right();
    [[nodiscard]] bool unmerge_down();
    [[nodiscard]] std::optional<featherdoc::cell_vertical_alignment>
    vertical_alignment() const;
    [[nodiscard]] bool set_vertical_alignment(
        featherdoc::cell_vertical_alignment alignment);
    [[nodiscard]] bool clear_vertical_alignment();
    [[nodiscard]] std::optional<featherdoc::cell_text_direction>
    text_direction() const;
    [[nodiscard]] bool set_text_direction(
        featherdoc::cell_text_direction direction);
    [[nodiscard]] bool clear_text_direction();
    [[nodiscard]] std::optional<std::string> fill_color() const;
    [[nodiscard]] bool set_fill_color(std::string_view fill_color);
    [[nodiscard]] bool clear_fill_color();
    [[nodiscard]] std::optional<std::uint32_t> margin_twips(
        featherdoc::cell_margin_edge edge) const;
    [[nodiscard]] bool set_margin_twips(featherdoc::cell_margin_edge edge,
                                        std::uint32_t margin_twips);
    [[nodiscard]] bool clear_margin(featherdoc::cell_margin_edge edge);
    [[nodiscard]] bool set_border(featherdoc::cell_border_edge edge,
                                  featherdoc::border_definition border);
    [[nodiscard]] bool clear_border(featherdoc::cell_border_edge edge);

    TableCell &next();
    [[nodiscard]] bool has_next() const;
};

// TableRow consists of one or more TableCells
class TableRow {
    friend class IteratorHelper;
    pugi::xml_node parent;
    pugi::xml_node current;

    TableCell cell;

  public:
    TableRow();
    TableRow(pugi::xml_node, pugi::xml_node);
    void set_parent(pugi::xml_node);
    void set_current(pugi::xml_node);

    TableCell &cells();
    [[nodiscard]] std::optional<TableCell> find_cell(std::size_t cell_index);
    [[nodiscard]] std::optional<std::uint32_t> height_twips() const;
    [[nodiscard]] std::optional<featherdoc::row_height_rule> height_rule() const;
    [[nodiscard]] bool set_height_twips(std::uint32_t height_twips,
                                        featherdoc::row_height_rule height_rule);
    [[nodiscard]] bool clear_height();
    [[nodiscard]] bool cant_split() const;
    [[nodiscard]] bool set_cant_split();
    [[nodiscard]] bool clear_cant_split();
    [[nodiscard]] bool repeats_header() const;
    [[nodiscard]] bool set_repeats_header();
    [[nodiscard]] bool clear_repeats_header();
    [[nodiscard]] bool set_texts(const std::vector<std::string> &texts);
    [[nodiscard]] bool set_texts(std::initializer_list<std::string> texts);
    [[nodiscard]] bool remove();
    TableRow insert_row_before();
    TableRow insert_row_after();
    TableCell append_cell();

    [[nodiscard]] bool has_next() const;
    TableRow &next();
};

// Table consists of one or more TableRow objects
class Table {
  private:
    friend class IteratorHelper;
    Document *owner{nullptr};
    pugi::xml_node parent;
    pugi::xml_node current;

    TableRow row;

  public:
    Table();
    Table(pugi::xml_node, pugi::xml_node);
    void set_owner(Document *);
    void set_parent(pugi::xml_node);
    void set_current(pugi::xml_node);

    Table &next();
    [[nodiscard]] bool has_next() const;

    TableRow &rows();
    [[nodiscard]] std::optional<TableRow> find_row(std::size_t row_index);
    [[nodiscard]] std::optional<TableCell> find_cell(std::size_t row_index,
                                                     std::size_t cell_index);
    [[nodiscard]] std::optional<std::uint32_t> width_twips() const;
    [[nodiscard]] bool set_width_twips(std::uint32_t width_twips);
    [[nodiscard]] bool clear_width();
    [[nodiscard]] bool set_cell_text(std::size_t row_index, std::size_t cell_index,
                                     const std::string &text);
    [[nodiscard]] bool set_row_texts(std::size_t row_index,
                                     const std::vector<std::string> &texts);
    [[nodiscard]] bool set_row_texts(std::size_t row_index,
                                     std::initializer_list<std::string> texts);
    [[nodiscard]] bool set_rows_texts(
        std::size_t start_row_index,
        const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] bool set_rows_texts(
        std::size_t start_row_index,
        std::initializer_list<std::initializer_list<std::string>> rows);
    [[nodiscard]] bool set_cell_block_texts(
        std::size_t start_row_index, std::size_t start_cell_index,
        const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] bool set_cell_block_texts(
        std::size_t start_row_index, std::size_t start_cell_index,
        std::initializer_list<std::initializer_list<std::string>> rows);
    [[nodiscard]] std::optional<std::uint32_t>
    column_width_twips(std::size_t column_index) const;
    [[nodiscard]] bool set_column_width_twips(std::size_t column_index,
                                              std::uint32_t width_twips);
    [[nodiscard]] bool clear_column_width(std::size_t column_index);
    [[nodiscard]] std::optional<featherdoc::table_layout_mode> layout_mode() const;
    [[nodiscard]] bool set_layout_mode(featherdoc::table_layout_mode layout_mode);
    [[nodiscard]] bool clear_layout_mode();
    [[nodiscard]] std::optional<featherdoc::table_alignment> alignment() const;
    [[nodiscard]] bool set_alignment(featherdoc::table_alignment alignment);
    [[nodiscard]] bool clear_alignment();
    [[nodiscard]] std::optional<std::uint32_t> indent_twips() const;
    [[nodiscard]] bool set_indent_twips(std::uint32_t indent_twips);
    [[nodiscard]] bool clear_indent();
    [[nodiscard]] std::optional<std::uint32_t> cell_spacing_twips() const;
    [[nodiscard]] bool set_cell_spacing_twips(std::uint32_t spacing_twips);
    [[nodiscard]] bool clear_cell_spacing();
    [[nodiscard]] std::optional<std::uint32_t> cell_margin_twips(
        featherdoc::cell_margin_edge edge) const;
    [[nodiscard]] bool set_cell_margin_twips(featherdoc::cell_margin_edge edge,
                                             std::uint32_t margin_twips);
    [[nodiscard]] bool clear_cell_margin(featherdoc::cell_margin_edge edge);
    [[nodiscard]] std::optional<std::string> style_id() const;
    [[nodiscard]] bool set_style_id(std::string_view style_id);
    [[nodiscard]] bool clear_style_id();
    [[nodiscard]] std::optional<featherdoc::table_style_look> style_look() const;
    [[nodiscard]] bool set_style_look(featherdoc::table_style_look style_look);
    [[nodiscard]] bool clear_style_look();
    [[nodiscard]] bool set_border(featherdoc::table_border_edge edge,
                                  featherdoc::border_definition border);
    [[nodiscard]] bool clear_border(featherdoc::table_border_edge edge);
    [[nodiscard]] bool remove();
    Table insert_table_before(std::size_t row_count = 1U,
                              std::size_t column_count = 1U);
    Table insert_table_after(std::size_t row_count = 1U,
                             std::size_t column_count = 1U);
    Table insert_table_like_before();
    Table insert_table_like_after();
    TableRow append_row(std::size_t cell_count = 1U);
};

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

enum class bookmark_kind : std::uint8_t {
    text = 0U,
    block,
    table_rows,
    block_range,
    malformed,
    mixed,
};

struct bookmark_summary {
    std::string bookmark_name;
    std::size_t occurrence_count{};
    featherdoc::bookmark_kind kind{featherdoc::bookmark_kind::text};

    [[nodiscard]] bool is_duplicate() const noexcept {
        return this->occurrence_count > 1U;
    }
};

enum class content_control_kind : std::uint8_t {
    block = 0U,
    run,
    table_row,
    table_cell,
    unknown,
};

struct content_control_summary {
    std::size_t index{};
    featherdoc::content_control_kind kind{featherdoc::content_control_kind::unknown};
    std::optional<std::string> tag;
    std::optional<std::string> alias;
    std::optional<std::string> id;
    bool showing_placeholder{false};
    std::string text;

    [[nodiscard]] bool has_tag() const noexcept {
        return this->tag.has_value() && !this->tag->empty();
    }

    [[nodiscard]] bool has_alias() const noexcept {
        return this->alias.has_value() && !this->alias->empty();
    }
};

enum class template_slot_kind : std::uint8_t {
    text = 0U,
    table_rows,
    table,
    image,
    floating_image,
    block,
};

enum class template_slot_source_kind : std::uint8_t {
    bookmark = 0U,
    content_control_tag,
    content_control_alias,
};

struct template_slot_requirement {
    std::string bookmark_name;
    featherdoc::template_slot_kind kind{featherdoc::template_slot_kind::text};
    bool required{true};
    std::optional<std::size_t> min_occurrences;
    std::optional<std::size_t> max_occurrences;
    featherdoc::template_slot_source_kind source{
        featherdoc::template_slot_source_kind::bookmark};
};

struct template_kind_mismatch {
    std::string bookmark_name;
    featherdoc::template_slot_kind expected_kind{featherdoc::template_slot_kind::text};
    featherdoc::bookmark_kind actual_kind{featherdoc::bookmark_kind::text};
    std::size_t occurrence_count{};
};

struct template_occurrence_mismatch {
    std::string bookmark_name;
    std::size_t actual_occurrences{};
    std::size_t min_occurrences{};
    std::optional<std::size_t> max_occurrences;
};

struct template_validation_result {
    std::vector<std::string> missing_required;
    std::vector<std::string> duplicate_bookmarks;
    std::vector<std::string> malformed_placeholders;
    std::vector<bookmark_summary> unexpected_bookmarks;
    std::vector<template_kind_mismatch> kind_mismatches;
    std::vector<template_occurrence_mismatch> occurrence_mismatches;

    explicit operator bool() const noexcept {
        return this->missing_required.empty() &&
               this->duplicate_bookmarks.empty() &&
               this->malformed_placeholders.empty() &&
               this->unexpected_bookmarks.empty() &&
               this->kind_mismatches.empty() &&
               this->occurrence_mismatches.empty();
    }
};

enum class template_schema_part_kind : std::uint8_t {
    body = 0U,
    header,
    footer,
    section_header,
    section_footer,
};

struct template_schema_part_selector {
    featherdoc::template_schema_part_kind part{
        featherdoc::template_schema_part_kind::body};
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind{
        featherdoc::section_reference_kind::default_reference};
};

struct template_schema_entry {
    featherdoc::template_schema_part_selector target{};
    featherdoc::template_slot_requirement requirement{};
};

struct template_schema {
    std::vector<featherdoc::template_schema_entry> entries;
};

struct template_schema_slot_selector {
    featherdoc::template_schema_part_selector target{};
    std::string bookmark_name;
    featherdoc::template_slot_source_kind source{
        featherdoc::template_slot_source_kind::bookmark};
};

struct template_schema_slot_rename {
    featherdoc::template_schema_slot_selector slot{};
    std::string new_bookmark_name;
};

struct template_schema_slot_update {
    std::optional<featherdoc::template_slot_kind> kind;
    std::optional<bool> required;
    std::optional<std::size_t> min_occurrences;
    std::optional<std::size_t> max_occurrences;
    bool clear_min_occurrences{false};
    bool clear_max_occurrences{false};
};

struct template_schema_slot_patch_update {
    featherdoc::template_schema_slot_selector slot{};
    featherdoc::template_schema_slot_update update{};
};

struct template_schema_patch {
    std::vector<featherdoc::template_schema_entry> upsert_slots;
    std::vector<featherdoc::template_schema_part_selector> remove_targets;
    std::vector<featherdoc::template_schema_slot_selector> remove_slots;
    std::vector<featherdoc::template_schema_slot_rename> rename_slots;
    std::vector<featherdoc::template_schema_slot_patch_update> update_slots;
};

struct template_schema_normalization_summary {
    std::size_t original_slots{};
    std::size_t final_slots{};
    std::size_t duplicate_slots_removed{};
    bool reordered_or_normalized{false};

    [[nodiscard]] bool changed() const noexcept {
        return this->duplicate_slots_removed > 0U ||
               this->original_slots != this->final_slots ||
               this->reordered_or_normalized;
    }
};

struct template_schema_patch_summary {
    std::size_t removed_targets{};
    std::size_t removed_slots{};
    std::size_t renamed_slots{};
    std::size_t inserted_slots{};
    std::size_t replaced_slots{};

    [[nodiscard]] bool changed() const noexcept {
        return this->removed_targets > 0U || this->removed_slots > 0U ||
               this->renamed_slots > 0U || this->inserted_slots > 0U ||
               this->replaced_slots > 0U;
    }
};

[[nodiscard]] template_schema_normalization_summary normalize_template_schema(
    featherdoc::template_schema &schema);
[[nodiscard]] template_schema_patch_summary merge_template_schema(
    featherdoc::template_schema &base, const featherdoc::template_schema &overlay);
[[nodiscard]] template_schema_patch_summary apply_template_schema_patch(
    featherdoc::template_schema &schema, const featherdoc::template_schema_patch &patch);
[[nodiscard]] template_schema_patch_summary preview_template_schema_patch(
    const featherdoc::template_schema &schema,
    const featherdoc::template_schema_patch &patch);
[[nodiscard]] template_schema_patch_summary preview_template_schema_patch(
    const featherdoc::template_schema &left,
    const featherdoc::template_schema &right);
[[nodiscard]] featherdoc::template_schema_patch build_template_schema_patch(
    const featherdoc::template_schema &left, const featherdoc::template_schema &right);
[[nodiscard]] template_schema_patch_summary replace_template_schema_target(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_part_selector &target,
    std::span<const featherdoc::template_schema_entry> entries);
[[nodiscard]] template_schema_patch_summary replace_template_schema_target(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_part_selector &target,
    std::initializer_list<featherdoc::template_schema_entry> entries);
[[nodiscard]] template_schema_patch_summary upsert_template_schema_slot(
    featherdoc::template_schema &schema, const featherdoc::template_schema_entry &entry);
[[nodiscard]] template_schema_patch_summary remove_template_schema_target(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_part_selector &target);
[[nodiscard]] template_schema_patch_summary remove_template_schema_slot(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_slot_selector &slot);
[[nodiscard]] template_schema_patch_summary rename_template_schema_slot(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_slot_selector &slot,
    std::string_view new_bookmark_name);
[[nodiscard]] template_schema_patch_summary update_template_schema_slot(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_slot_selector &slot,
    const featherdoc::template_schema_slot_update &update);

struct template_schema_part_validation_result {
    featherdoc::template_schema_part_selector target{};
    std::string entry_name;
    bool available{false};
    featherdoc::template_validation_result validation{};

    explicit operator bool() const noexcept {
        return static_cast<bool>(this->validation);
    }
};

struct template_schema_validation_result {
    std::vector<featherdoc::template_schema_part_validation_result> part_results;

    explicit operator bool() const noexcept {
        for (const auto &part_result : this->part_results) {
            if (!static_cast<bool>(part_result)) {
                return false;
            }
        }

        return true;
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
};

struct numbering_level_definition {
    featherdoc::list_kind kind{};
    std::uint32_t start{1U};
    std::uint32_t level{0U};
    std::string text_pattern;
};

struct numbering_definition {
    std::string name;
    std::vector<featherdoc::numbering_level_definition> levels;
};

struct paragraph_style_numbering_link {
    std::string style_id;
    std::uint32_t level{0U};
};

struct numbering_level_override_summary {
    std::uint32_t level{};
    std::optional<std::uint32_t> start_override;
    std::optional<featherdoc::numbering_level_definition> level_definition;
};

struct numbering_instance_summary {
    std::uint32_t instance_id{};
    std::vector<featherdoc::numbering_level_override_summary> level_overrides;
};

enum class style_kind : std::uint8_t {
    paragraph = 0U,
    character,
    table,
    numbering,
    unknown,
};

struct style_summary {
    struct numbering_summary {
        std::optional<std::uint32_t> num_id;
        std::optional<std::uint32_t> level;
        std::optional<std::uint32_t> definition_id;
        std::optional<std::string> definition_name;
        std::optional<featherdoc::numbering_instance_summary> instance;
    };

    std::string style_id;
    std::string name;
    std::optional<std::string> based_on;
    featherdoc::style_kind kind{featherdoc::style_kind::unknown};
    std::string type_name;
    std::optional<numbering_summary> numbering;
    bool is_default{false};
    bool is_custom{false};
    bool is_semi_hidden{false};
    bool is_unhide_when_used{false};
    bool is_quick_format{false};
};

struct resolved_style_string_property {
    std::optional<std::string> value;
    std::optional<std::string> source_style_id;
};

struct resolved_style_bool_property {
    std::optional<bool> value;
    std::optional<std::string> source_style_id;
};

struct resolved_style_properties_summary {
    std::string style_id;
    std::string type_name;
    std::optional<std::string> based_on;
    featherdoc::style_kind kind{featherdoc::style_kind::unknown};
    std::vector<std::string> inheritance_chain;
    featherdoc::resolved_style_string_property run_font_family{};
    featherdoc::resolved_style_string_property run_east_asia_font_family{};
    featherdoc::resolved_style_string_property run_language{};
    featherdoc::resolved_style_string_property run_east_asia_language{};
    featherdoc::resolved_style_string_property run_bidi_language{};
    featherdoc::resolved_style_bool_property run_rtl{};
    featherdoc::resolved_style_bool_property paragraph_bidi{};
};

struct style_usage_breakdown {
    std::size_t paragraph_count{0};
    std::size_t run_count{0};
    std::size_t table_count{0};

    [[nodiscard]] std::size_t total_count() const {
        return paragraph_count + run_count + table_count;
    }
};

enum class style_usage_part_kind { body, header, footer };

enum class style_usage_hit_kind { paragraph, run, table };

struct style_usage_hit_reference {
    std::size_t section_index{0};
    featherdoc::section_reference_kind reference_kind{
        featherdoc::section_reference_kind::default_reference};
};

struct style_usage_hit {
    featherdoc::style_usage_part_kind part{featherdoc::style_usage_part_kind::body};
    featherdoc::style_usage_hit_kind kind{featherdoc::style_usage_hit_kind::paragraph};
    std::string entry_name;
    std::size_t ordinal{0};
    std::size_t node_ordinal{0};
    std::optional<std::size_t> section_index;
    std::vector<featherdoc::style_usage_hit_reference> references{};
};

struct style_usage_summary {
    std::string style_id;
    std::size_t paragraph_count{0};
    std::size_t run_count{0};
    std::size_t table_count{0};
    featherdoc::style_usage_breakdown body{};
    featherdoc::style_usage_breakdown header{};
    featherdoc::style_usage_breakdown footer{};
    std::vector<featherdoc::style_usage_hit> hits{};

    [[nodiscard]] std::size_t total_count() const {
        return paragraph_count + run_count + table_count;
    }
};

struct style_usage_report_entry {
    featherdoc::style_summary style{};
    featherdoc::style_usage_summary usage{};
};

struct style_usage_report {
    std::size_t style_count{0};
    std::size_t used_style_count{0};
    std::size_t unused_style_count{0};
    std::size_t total_reference_count{0};
    std::vector<featherdoc::style_usage_report_entry> entries{};
};

enum class style_refactor_action { rename, merge };

struct style_refactor_request {
    featherdoc::style_refactor_action action{featherdoc::style_refactor_action::rename};
    std::string source_style_id;
    std::string target_style_id;
};

struct style_refactor_issue {
    std::string code;
    std::string message;
};

struct style_refactor_suggestion {
    std::string reason_code;
    std::string reason;
    std::uint32_t confidence{0};
    std::vector<std::string> evidence{};
    std::vector<std::string> differences{};
};

struct style_refactor_suggestion_confidence_summary {
    std::size_t suggestion_count{0};
    std::size_t exact_xml_match_count{0};
    std::size_t xml_difference_count{0};
    std::optional<std::uint32_t> min_confidence{};
    std::optional<std::uint32_t> max_confidence{};
    std::optional<std::uint32_t> recommended_min_confidence{};
    std::string recommendation;
};

struct style_refactor_operation_plan {
    featherdoc::style_refactor_action action{featherdoc::style_refactor_action::rename};
    std::string source_style_id;
    std::string target_style_id;
    std::optional<featherdoc::style_summary> source_style{};
    std::optional<featherdoc::style_summary> target_style{};
    std::optional<featherdoc::style_usage_summary> source_usage{};
    std::optional<featherdoc::style_refactor_suggestion> suggestion{};
    std::vector<featherdoc::style_refactor_issue> issues{};
    bool applyable{false};
};

struct style_refactor_plan {
    std::size_t operation_count{0};
    std::size_t applyable_count{0};
    std::size_t issue_count{0};
    std::vector<featherdoc::style_refactor_operation_plan> operations{};

    [[nodiscard]] featherdoc::style_refactor_suggestion_confidence_summary
    suggestion_confidence_summary() const {
        auto summary = featherdoc::style_refactor_suggestion_confidence_summary{};
        auto exact_xml_min_confidence = std::optional<std::uint32_t>{};

        for (const auto &operation : operations) {
            if (!operation.suggestion.has_value()) {
                continue;
            }

            const auto &suggestion = *operation.suggestion;
            ++summary.suggestion_count;
            if (!summary.min_confidence.has_value() ||
                suggestion.confidence < *summary.min_confidence) {
                summary.min_confidence = suggestion.confidence;
            }
            if (!summary.max_confidence.has_value() ||
                suggestion.confidence > *summary.max_confidence) {
                summary.max_confidence = suggestion.confidence;
            }

            if (suggestion.reason_code == "matching_style_signature_and_xml") {
                ++summary.exact_xml_match_count;
                if (!exact_xml_min_confidence.has_value() ||
                    suggestion.confidence < *exact_xml_min_confidence) {
                    exact_xml_min_confidence = suggestion.confidence;
                }
            } else if (suggestion.reason_code == "matching_resolved_style_signature") {
                ++summary.xml_difference_count;
            }
        }

        if (summary.suggestion_count == 0U) {
            summary.recommendation =
                "No automatic style merge suggestions are present.";
            return summary;
        }

        if (summary.exact_xml_match_count != 0U) {
            summary.recommended_min_confidence = exact_xml_min_confidence;
            summary.recommendation =
                summary.exact_xml_match_count == summary.suggestion_count
                    ? "All suggestions are exact style XML matches; use the "
                      "recommended minimum confidence for automation gates."
                    : "Use the recommended minimum confidence to keep exact "
                      "style XML matches and review lower-confidence XML "
                      "differences manually.";
            return summary;
        }

        summary.recommended_min_confidence = summary.max_confidence;
        summary.recommendation =
            "No exact style XML matches are present; keep these suggestions in "
            "manual review workflows.";
        return summary;
    }

    [[nodiscard]] bool clean() const noexcept {
        return issue_count == 0U;
    }
};

struct style_refactor_rollback_entry {
    featherdoc::style_refactor_action action{featherdoc::style_refactor_action::rename};
    std::string source_style_id;
    std::string target_style_id;
    bool automatic{false};
    bool restorable{false};
    std::string note;
    std::string source_style_xml;
    std::optional<featherdoc::style_usage_summary> source_usage{};
};

struct style_refactor_apply_result {
    featherdoc::style_refactor_plan plan{};
    std::size_t requested_count{0};
    std::size_t applied_count{0};
    std::vector<featherdoc::style_refactor_rollback_entry> rollback_entries{};
    bool changed{false};

    [[nodiscard]] std::size_t skipped_count() const noexcept {
        return requested_count >= applied_count ? requested_count - applied_count : 0U;
    }

    [[nodiscard]] bool applied() const noexcept {
        return plan.clean() && applied_count == requested_count;
    }
};

struct style_refactor_restore_issue {
    std::string code;
    std::string message;
    std::string suggestion;
};

struct style_refactor_restore_issue_summary {
    std::string code;
    std::size_t count{0};
    std::string suggestion;
};

struct style_refactor_restore_operation_result {
    featherdoc::style_refactor_action action{featherdoc::style_refactor_action::merge};
    std::string source_style_id;
    std::string target_style_id;
    bool restorable{false};
    bool restored{false};
    bool style_restored{false};
    std::size_t restored_reference_count{0};
    std::vector<featherdoc::style_refactor_restore_issue> issues{};
};

struct style_refactor_restore_result {
    std::size_t requested_count{0};
    std::size_t restored_count{0};
    std::size_t restored_style_count{0};
    std::size_t restored_reference_count{0};
    std::vector<featherdoc::style_refactor_restore_operation_result> operations{};
    bool changed{false};
    bool dry_run{false};

    [[nodiscard]] std::size_t skipped_count() const noexcept {
        return requested_count >= restored_count ? requested_count - restored_count : 0U;
    }

    [[nodiscard]] std::size_t issue_count() const noexcept {
        auto count = std::size_t{0};
        for (const auto &operation : operations) {
            count += operation.issues.size();
        }
        return count;
    }

    [[nodiscard]] std::vector<featherdoc::style_refactor_restore_issue_summary>
    issue_summary() const {
        auto summaries =
            std::vector<featherdoc::style_refactor_restore_issue_summary>{};
        for (const auto &operation : operations) {
            for (const auto &issue : operation.issues) {
                auto matched = false;
                for (auto &summary : summaries) {
                    if (summary.code == issue.code) {
                        ++summary.count;
                        matched = true;
                        break;
                    }
                }
                if (!matched) {
                    summaries.push_back({issue.code, 1U, issue.suggestion});
                }
            }
        }
        return summaries;
    }

    [[nodiscard]] bool clean() const noexcept {
        return issue_count() == 0U;
    }

    [[nodiscard]] bool restored() const noexcept {
        return clean() && restored_count == requested_count;
    }
};

struct style_prune_plan {
    std::size_t scanned_style_count{0};
    std::size_t protected_style_count{0};
    std::vector<std::string> removable_style_ids{};

    [[nodiscard]] bool changed() const noexcept {
        return !removable_style_ids.empty();
    }
};

struct style_prune_summary {
    std::size_t scanned_style_count{0};
    std::size_t protected_style_count{0};
    std::vector<std::string> removed_style_ids{};

    [[nodiscard]] bool changed() const noexcept {
        return !removed_style_ids.empty();
    }
};

struct paragraph_style_definition {
    std::string name;
    std::optional<std::string> based_on;
    std::optional<std::string> next_style;
    bool is_custom{true};
    bool is_semi_hidden{false};
    bool is_unhide_when_used{false};
    bool is_quick_format{false};
    std::optional<std::string> run_font_family;
    std::optional<std::string> run_east_asia_font_family;
    std::optional<std::string> run_language;
    std::optional<std::string> run_east_asia_language;
    std::optional<std::string> run_bidi_language;
    std::optional<bool> run_rtl;
    std::optional<bool> paragraph_bidi;
    std::optional<std::uint32_t> outline_level;
};

struct character_style_definition {
    std::string name;
    std::optional<std::string> based_on;
    bool is_custom{true};
    bool is_semi_hidden{false};
    bool is_unhide_when_used{false};
    bool is_quick_format{false};
    std::optional<std::string> run_font_family;
    std::optional<std::string> run_east_asia_font_family;
    std::optional<std::string> run_language;
    std::optional<std::string> run_east_asia_language;
    std::optional<std::string> run_bidi_language;
    std::optional<bool> run_rtl;
};

struct table_style_definition {
    std::string name;
    std::optional<std::string> based_on;
    bool is_custom{true};
    bool is_semi_hidden{false};
    bool is_unhide_when_used{false};
    bool is_quick_format{false};
};

struct numbering_definition_summary {
    std::uint32_t definition_id{};
    std::string name;
    std::vector<featherdoc::numbering_level_definition> levels;
    std::vector<std::uint32_t> instance_ids;
    std::vector<featherdoc::numbering_instance_summary> instances;
};

struct numbering_instance_lookup_summary {
    std::uint32_t definition_id{};
    std::string definition_name;
    featherdoc::numbering_instance_summary instance;
};

struct numbering_catalog_definition {
    featherdoc::numbering_definition definition{};
    std::vector<featherdoc::numbering_instance_summary> instances;
};

struct numbering_catalog {
    std::vector<featherdoc::numbering_catalog_definition> definitions;
};

struct imported_numbering_definition_summary {
    std::string name;
    std::uint32_t definition_id{};
    std::vector<std::uint32_t> instance_ids;
};

struct numbering_catalog_import_summary {
    std::size_t input_definition_count{};
    std::size_t imported_definition_count{};
    std::size_t imported_instance_count{};
    std::vector<featherdoc::imported_numbering_definition_summary> definitions;

    explicit operator bool() const noexcept {
        return this->input_definition_count == this->imported_definition_count;
    }
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
    std::optional<numbering_summary> numbering;
    std::size_t run_count{0};
    std::string text;
};

struct table_inspection_summary {
    std::size_t index{0};
    std::optional<std::string> style_id;
    std::optional<std::uint32_t> width_twips;
    std::size_t row_count{0};
    std::size_t column_count{0};
    std::vector<std::optional<std::uint32_t>> column_widths;
    std::string text;
};

struct template_table_selector {
    std::optional<std::size_t> table_index;
    std::optional<std::string> bookmark_name;
    std::optional<std::string> after_paragraph_text;
    std::vector<std::string> header_cell_texts;
    std::size_t header_row_index{0};
    std::size_t occurrence{0};
};

struct table_cell_inspection_summary {
    std::size_t row_index{0};
    std::size_t cell_index{0};
    std::size_t column_index{0};
    std::size_t column_span{1};
    std::size_t paragraph_count{0};
    std::optional<std::uint32_t> width_twips;
    std::optional<featherdoc::cell_vertical_alignment> vertical_alignment;
    std::optional<featherdoc::cell_text_direction> text_direction;
    std::string text;
};

struct run_inspection_summary {
    std::size_t index{0};
    std::optional<std::string> style_id;
    std::optional<std::string> font_family;
    std::optional<std::string> east_asia_font_family;
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
    Paragraph append_paragraph(
        const std::string &text = {},
        featherdoc::formatting_flag formatting = featherdoc::formatting_flag::none);
    Table tables();
    Table append_table(std::size_t row_count = 1U, std::size_t column_count = 1U);
    [[nodiscard]] bool append_image(const std::filesystem::path &image_path);
    [[nodiscard]] bool append_image(const std::filesystem::path &image_path,
                                    std::uint32_t width_px,
                                    std::uint32_t height_px);
    [[nodiscard]] bool append_floating_image(
        const std::filesystem::path &image_path,
        featherdoc::floating_image_options options = {});
    [[nodiscard]] bool append_floating_image(
        const std::filesystem::path &image_path, std::uint32_t width_px,
        std::uint32_t height_px,
        featherdoc::floating_image_options options = {});
    [[nodiscard]] bool append_page_number_field();
    [[nodiscard]] bool append_total_pages_field();
    [[nodiscard]] std::vector<featherdoc::table_inspection_summary> inspect_tables();
    [[nodiscard]] std::optional<featherdoc::table_inspection_summary>
    inspect_table(std::size_t table_index);
    [[nodiscard]] std::vector<featherdoc::table_cell_inspection_summary>
    inspect_table_cells(std::size_t table_index);
    [[nodiscard]] std::optional<featherdoc::table_cell_inspection_summary>
    inspect_table_cell(std::size_t table_index, std::size_t row_index, std::size_t cell_index);
    [[nodiscard]] std::vector<featherdoc::paragraph_inspection_summary> inspect_paragraphs();
    [[nodiscard]] std::optional<featherdoc::paragraph_inspection_summary>
    inspect_paragraph(std::size_t paragraph_index);
    [[nodiscard]] std::vector<featherdoc::run_inspection_summary>
    inspect_paragraph_runs(std::size_t paragraph_index);
    [[nodiscard]] std::optional<featherdoc::run_inspection_summary>
    inspect_paragraph_run(std::size_t paragraph_index, std::size_t run_index);

    [[nodiscard]] std::size_t replace_bookmark_text(const std::string &bookmark_name,
                                                    const std::string &replacement);
    [[nodiscard]] std::size_t replace_bookmark_text(const char *bookmark_name,
                                                    const char *replacement);
    [[nodiscard]] bookmark_fill_result fill_bookmarks(
        std::span<const bookmark_text_binding> bindings);
    [[nodiscard]] bookmark_fill_result fill_bookmarks(
        std::initializer_list<bookmark_text_binding> bindings);
    [[nodiscard]] std::vector<bookmark_summary> list_bookmarks() const;
    [[nodiscard]] std::optional<bookmark_summary> find_bookmark(
        std::string_view bookmark_name) const;
    [[nodiscard]] std::vector<content_control_summary>
    list_content_controls() const;
    [[nodiscard]] std::vector<content_control_summary>
    find_content_controls_by_tag(std::string_view tag) const;
    [[nodiscard]] std::vector<content_control_summary>
    find_content_controls_by_alias(std::string_view alias) const;
    [[nodiscard]] std::size_t replace_content_control_text_by_tag(
        std::string_view tag, std::string_view replacement);
    [[nodiscard]] std::size_t replace_content_control_text_by_alias(
        std::string_view alias, std::string_view replacement);
    [[nodiscard]] std::optional<std::size_t> find_table_index_by_bookmark(
        std::string_view bookmark_name) const;
    [[nodiscard]] std::optional<std::size_t> find_table_index(
        const featherdoc::template_table_selector &selector) const;
    [[nodiscard]] std::optional<featherdoc::Table> find_table_by_bookmark(
        std::string_view bookmark_name);
    [[nodiscard]] std::optional<featherdoc::Table> find_table(
        const featherdoc::template_table_selector &selector);
    [[nodiscard]] template_validation_result validate_template(
        std::span<const template_slot_requirement> requirements) const;
    [[nodiscard]] template_validation_result validate_template(
        std::initializer_list<template_slot_requirement> requirements) const;
    [[nodiscard]] std::size_t replace_bookmark_with_paragraphs(
        std::string_view bookmark_name, const std::vector<std::string> &paragraphs);
    [[nodiscard]] std::size_t replace_bookmark_with_table_rows(
        std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_bookmark_with_table(
        std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_bookmark_with_image(
        std::string_view bookmark_name, const std::filesystem::path &image_path);
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
    [[nodiscard]] std::size_t remove_bookmark_block(
        std::string_view bookmark_name);
    [[nodiscard]] std::vector<drawing_image_info> drawing_images() const;
    [[nodiscard]] bool extract_drawing_image(
        std::size_t image_index, const std::filesystem::path &output_path) const;
    [[nodiscard]] bool remove_drawing_image(std::size_t image_index);
    [[nodiscard]] bool replace_drawing_image(std::size_t image_index,
                                             const std::filesystem::path &image_path);
    [[nodiscard]] std::vector<inline_image_info> inline_images() const;
    [[nodiscard]] bool extract_inline_image(
        std::size_t image_index, const std::filesystem::path &output_path) const;
    [[nodiscard]] bool remove_inline_image(std::size_t image_index);
    [[nodiscard]] bool replace_inline_image(std::size_t image_index,
                                            const std::filesystem::path &image_path);
    [[nodiscard]] std::size_t set_bookmark_block_visibility(
        std::string_view bookmark_name, bool visible);
    [[nodiscard]] bookmark_block_visibility_result apply_bookmark_block_visibility(
        std::span<const bookmark_block_visibility_binding> bindings);
    [[nodiscard]] bookmark_block_visibility_result apply_bookmark_block_visibility(
        std::initializer_list<bookmark_block_visibility_binding> bindings);
};

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
        const std::vector<featherdoc::style_refactor_rollback_entry> &rollback_entries,
        bool apply_changes);
    [[nodiscard]] std::error_code ensure_even_and_odd_headers_enabled();
    [[nodiscard]] std::optional<bool> inspect_even_and_odd_headers_enabled();
    [[nodiscard]] pugi::xml_node section_properties(std::size_t section_index) const;
    [[nodiscard]] pugi::xml_node ensure_section_properties(std::size_t section_index);
    [[nodiscard]] bool ensure_title_page_enabled(pugi::xml_node section_properties);
    Paragraph &related_part_paragraphs_by_relationship_id(
        std::string_view relationship_id, std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *part_root_name);
    Paragraph &section_related_part_paragraphs(
        std::size_t section_index, featherdoc::section_reference_kind reference_kind,
        std::vector<std::unique_ptr<xml_part_state>> &parts, const char *reference_name,
        const char *part_root_name);
    [[nodiscard]] xml_part_state *section_related_part_state(
        std::size_t section_index, featherdoc::section_reference_kind reference_kind,
        std::vector<std::unique_ptr<xml_part_state>> &parts, const char *reference_name);
    Paragraph &ensure_section_related_part_paragraphs(
        std::size_t section_index, featherdoc::section_reference_kind reference_kind,
        std::vector<std::unique_ptr<xml_part_state>> &parts, const char *part_root_name,
        const char *reference_name, const char *relationship_type, const char *content_type);
    Paragraph &assign_section_related_part_paragraphs(
        std::size_t section_index, std::size_t part_index,
        featherdoc::section_reference_kind reference_kind,
        std::vector<std::unique_ptr<xml_part_state>> &parts, const char *part_root_name,
        const char *reference_name);
    [[nodiscard]] bool remove_section_related_part_reference(
        std::size_t section_index, featherdoc::section_reference_kind reference_kind,
        const char *reference_name);
    [[nodiscard]] std::error_code ensure_removal_prerequisites_loaded();
    void cleanup_first_page_section_markers();
    [[nodiscard]] std::error_code cleanup_even_and_odd_headers_setting();
    [[nodiscard]] bool remove_related_part(
        std::size_t part_index, std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *reference_name);
    [[nodiscard]] bool move_related_part(
        std::size_t source_index, std::size_t target_index,
        std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *relationship_type);
    [[nodiscard]] bool copy_section_related_part_references(
        std::size_t source_section_index, std::size_t target_section_index,
        const char *reference_name);
    Paragraph &ensure_related_part_paragraphs(
        std::vector<std::unique_ptr<xml_part_state>> &parts, const char *part_root_name,
        const char *reference_name, const char *relationship_type, const char *content_type);
    [[nodiscard]] xml_part_state *find_related_part_state(std::string_view entry_name);
    [[nodiscard]] const xml_part_state *find_related_part_state(
        std::string_view entry_name) const;
    [[nodiscard]] std::uint32_t next_drawing_object_id() const;
    [[nodiscard]] bool append_inline_image_part(
        std::string image_data, std::string extension, std::string content_type,
        std::string display_name, std::uint32_t width_px, std::uint32_t height_px);
    [[nodiscard]] bool append_inline_image_part(
        pugi::xml_node parent, pugi::xml_node insert_before, std::string image_data,
        std::string extension, std::string content_type, std::string display_name,
        std::uint32_t width_px, std::uint32_t height_px);
    [[nodiscard]] bool append_inline_image_part(
        pugi::xml_document &xml_document, std::string_view xml_entry_name,
        pugi::xml_document &relationships_document, std::string_view relationships_entry_name,
        bool &has_relationships_part, bool &relationships_dirty, pugi::xml_node parent,
        pugi::xml_node insert_before, std::string image_data, std::string extension,
        std::string content_type, std::string display_name, std::uint32_t width_px,
        std::uint32_t height_px);
    [[nodiscard]] bool append_drawing_image_part(
        pugi::xml_document &xml_document, std::string_view xml_entry_name,
        pugi::xml_document &relationships_document, std::string_view relationships_entry_name,
        bool &has_relationships_part, bool &relationships_dirty, pugi::xml_node parent,
        pugi::xml_node insert_before, std::string image_data, std::string extension,
        std::string content_type, std::string display_name, std::uint32_t width_px,
        std::uint32_t height_px,
        std::optional<featherdoc::floating_image_options> floating_options);
    [[nodiscard]] bool append_floating_image_part(
        std::string image_data, std::string extension, std::string content_type,
        std::string display_name, std::uint32_t width_px, std::uint32_t height_px,
        featherdoc::floating_image_options options);
    [[nodiscard]] bool append_floating_image_part(
        pugi::xml_node parent, pugi::xml_node insert_before, std::string image_data,
        std::string extension, std::string content_type, std::string display_name,
        std::uint32_t width_px, std::uint32_t height_px,
        featherdoc::floating_image_options options);
    [[nodiscard]] bool append_floating_image_part(
        pugi::xml_document &xml_document, std::string_view xml_entry_name,
        pugi::xml_document &relationships_document, std::string_view relationships_entry_name,
        bool &has_relationships_part, bool &relationships_dirty, pugi::xml_node parent,
        pugi::xml_node insert_before, std::string image_data, std::string extension,
        std::string content_type, std::string display_name, std::uint32_t width_px,
        std::uint32_t height_px, featherdoc::floating_image_options options);
    [[nodiscard]] std::vector<drawing_image_info> drawing_images_in_part(
        std::string_view entry_name) const;
    [[nodiscard]] bool extract_drawing_image_from_part(
        std::string_view entry_name, std::size_t image_index,
        const std::filesystem::path &output_path) const;
    [[nodiscard]] bool remove_drawing_image_in_part(std::string_view entry_name,
                                                    std::size_t image_index);
    [[nodiscard]] bool replace_drawing_image_in_part(
        std::string_view entry_name, std::size_t image_index,
        const std::filesystem::path &image_path);
    [[nodiscard]] std::vector<inline_image_info> inline_images_in_part(
        std::string_view entry_name) const;
    [[nodiscard]] bool extract_inline_image_from_part(
        std::string_view entry_name, std::size_t image_index,
        const std::filesystem::path &output_path) const;
    [[nodiscard]] bool remove_inline_image_in_part(std::string_view entry_name,
                                                   std::size_t image_index);
    [[nodiscard]] bool replace_inline_image_in_part(
        std::string_view entry_name, std::size_t image_index,
        const std::filesystem::path &image_path);
    [[nodiscard]] std::size_t replace_bookmark_with_image_in_part(
        pugi::xml_document &xml_document, std::string_view entry_name,
        std::string_view bookmark_name, const std::filesystem::path &image_path,
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
    std::vector<std::unique_ptr<xml_part_state>> header_parts;
    std::vector<std::unique_ptr<xml_part_state>> footer_parts;
    std::vector<image_part_state> image_parts;
    bool flag_is_open{false};
    bool has_source_archive{false};
    bool has_document_relationships_part{false};
    bool has_settings_part{false};
    bool has_numbering_part{false};
    bool has_styles_part{false};
    bool document_relationships_dirty{false};
    bool content_types_loaded{false};
    bool content_types_dirty{false};
    bool settings_loaded{false};
    bool settings_dirty{false};
    bool numbering_loaded{false};
    bool numbering_dirty{false};
    bool styles_loaded{false};
    bool styles_dirty{false};
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
    [[nodiscard]] std::error_code save() const;
    [[nodiscard]] std::error_code save_as(std::filesystem::path) const;
    [[nodiscard]] bool is_open() const;
    [[nodiscard]] const document_error_info &last_error() const noexcept;
    [[nodiscard]] std::size_t section_count() const noexcept;
    [[nodiscard]] std::size_t header_count() const noexcept;
    [[nodiscard]] std::size_t footer_count() const noexcept;
    [[nodiscard]] featherdoc::sections_inspection_summary inspect_sections();
    [[nodiscard]] std::optional<featherdoc::section_inspection_summary> inspect_section(
        std::size_t section_index);
    [[nodiscard]] std::optional<section_page_setup> get_section_page_setup(
        std::size_t section_index) const;
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
    [[nodiscard]] bool copy_section_header_references(std::size_t source_section_index,
                                                      std::size_t target_section_index);
    [[nodiscard]] bool copy_section_footer_references(std::size_t source_section_index,
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
    [[nodiscard]] std::size_t replace_bookmark_text(const std::string &bookmark_name,
                                                    const std::string &replacement);
    [[nodiscard]] std::size_t replace_bookmark_text(const char *bookmark_name,
                                                    const char *replacement);
    [[nodiscard]] bookmark_fill_result fill_bookmarks(
        std::span<const bookmark_text_binding> bindings);
    [[nodiscard]] bookmark_fill_result fill_bookmarks(
        std::initializer_list<bookmark_text_binding> bindings);
    [[nodiscard]] std::vector<bookmark_summary> list_bookmarks() const;
    [[nodiscard]] std::optional<bookmark_summary> find_bookmark(
        std::string_view bookmark_name) const;
    [[nodiscard]] std::vector<content_control_summary>
    list_content_controls() const;
    [[nodiscard]] std::vector<content_control_summary>
    find_content_controls_by_tag(std::string_view tag) const;
    [[nodiscard]] std::vector<content_control_summary>
    find_content_controls_by_alias(std::string_view alias) const;
    [[nodiscard]] std::size_t replace_content_control_text_by_tag(
        std::string_view tag, std::string_view replacement);
    [[nodiscard]] std::size_t replace_content_control_text_by_alias(
        std::string_view alias, std::string_view replacement);
    [[nodiscard]] template_validation_result validate_template(
        std::span<const template_slot_requirement> requirements) const;
    [[nodiscard]] template_validation_result validate_template(
        std::initializer_list<template_slot_requirement> requirements) const;
    [[nodiscard]] featherdoc::template_schema_validation_result
    validate_template_schema(
        std::span<const featherdoc::template_schema_entry> entries) const;
    [[nodiscard]] featherdoc::template_schema_validation_result
    validate_template_schema(
        std::initializer_list<featherdoc::template_schema_entry> entries) const;
    [[nodiscard]] featherdoc::template_schema_validation_result
    validate_template_schema(const featherdoc::template_schema &schema) const;
    [[nodiscard]] std::size_t replace_bookmark_with_paragraphs(
        std::string_view bookmark_name, const std::vector<std::string> &paragraphs);
    [[nodiscard]] std::size_t replace_bookmark_with_table_rows(
        std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_bookmark_with_table(
        std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_bookmark_with_image(
        std::string_view bookmark_name, const std::filesystem::path &image_path);
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
    [[nodiscard]] std::size_t remove_bookmark_block(
        std::string_view bookmark_name);
    [[nodiscard]] std::size_t set_bookmark_block_visibility(
        std::string_view bookmark_name, bool visible);
    [[nodiscard]] bookmark_block_visibility_result apply_bookmark_block_visibility(
        std::span<const bookmark_block_visibility_binding> bindings);
    [[nodiscard]] bookmark_block_visibility_result apply_bookmark_block_visibility(
        std::initializer_list<bookmark_block_visibility_binding> bindings);
    [[nodiscard]] bool set_paragraph_list(
        Paragraph paragraph, featherdoc::list_kind kind, std::uint32_t level = 0U);
    [[nodiscard]] bool restart_paragraph_list(
        Paragraph paragraph, featherdoc::list_kind kind, std::uint32_t level = 0U);
    [[nodiscard]] bool clear_paragraph_list(Paragraph paragraph);
    [[nodiscard]] std::vector<featherdoc::numbering_definition_summary>
    list_numbering_definitions();
    [[nodiscard]] std::optional<featherdoc::numbering_definition_summary>
    find_numbering_definition(std::uint32_t definition_id);
    [[nodiscard]] std::optional<featherdoc::numbering_instance_lookup_summary>
    find_numbering_instance(std::uint32_t instance_id);
    [[nodiscard]] featherdoc::numbering_catalog export_numbering_catalog();
    [[nodiscard]] featherdoc::numbering_catalog_import_summary import_numbering_catalog(
        const featherdoc::numbering_catalog &catalog);
    [[nodiscard]] std::optional<std::uint32_t> ensure_numbering_definition(
        const featherdoc::numbering_definition &definition);
    [[nodiscard]] bool set_paragraph_numbering(Paragraph paragraph,
                                               std::uint32_t numbering_definition_id,
                                               std::uint32_t level = 0U);
    [[nodiscard]] std::optional<std::string> default_run_font_family();
    [[nodiscard]] std::optional<std::string> default_run_east_asia_font_family();
    [[nodiscard]] std::optional<std::string> default_run_language();
    [[nodiscard]] std::optional<std::string> default_run_east_asia_language();
    [[nodiscard]] std::optional<std::string> default_run_bidi_language();
    [[nodiscard]] std::optional<bool> default_run_rtl();
    [[nodiscard]] std::optional<bool> default_paragraph_bidi();
    [[nodiscard]] bool set_default_run_font_family(std::string_view font_family);
    [[nodiscard]] bool set_default_run_east_asia_font_family(std::string_view font_family);
    [[nodiscard]] bool set_default_run_language(std::string_view language);
    [[nodiscard]] bool set_default_run_east_asia_language(std::string_view language);
    [[nodiscard]] bool set_default_run_bidi_language(std::string_view language);
    [[nodiscard]] bool set_default_run_rtl(bool enabled = true);
    [[nodiscard]] bool set_default_paragraph_bidi(bool enabled = true);
    [[nodiscard]] bool clear_default_run_font_family();
    [[nodiscard]] bool clear_default_run_east_asia_font_family();
    [[nodiscard]] bool clear_default_run_primary_language();
    [[nodiscard]] bool clear_default_run_language();
    [[nodiscard]] bool clear_default_run_east_asia_language();
    [[nodiscard]] bool clear_default_run_bidi_language();
    [[nodiscard]] bool clear_default_run_rtl();
    [[nodiscard]] bool clear_default_paragraph_bidi();
    [[nodiscard]] std::vector<featherdoc::style_summary> list_styles();
    [[nodiscard]] std::optional<featherdoc::style_summary> find_style(
        std::string_view style_id);
    [[nodiscard]] std::optional<featherdoc::resolved_style_properties_summary>
    resolve_style_properties(std::string_view style_id);
    [[nodiscard]] std::optional<featherdoc::style_usage_summary> find_style_usage(
        std::string_view style_id);
    [[nodiscard]] std::optional<featherdoc::style_usage_report> list_style_usage();
    [[nodiscard]] std::optional<featherdoc::style_refactor_plan>
    plan_style_refactor(
        const std::vector<featherdoc::style_refactor_request> &requests);
    [[nodiscard]] std::optional<featherdoc::style_refactor_plan>
    suggest_style_merges();
    [[nodiscard]] std::optional<featherdoc::style_refactor_apply_result>
    apply_style_refactor(
        const std::vector<featherdoc::style_refactor_request> &requests);
    [[nodiscard]] std::optional<featherdoc::style_refactor_restore_result>
    plan_style_refactor_restore(
        const std::vector<featherdoc::style_refactor_rollback_entry> &rollback_entries);
    [[nodiscard]] std::optional<featherdoc::style_refactor_restore_result>
    restore_style_refactor(
        const std::vector<featherdoc::style_refactor_rollback_entry> &rollback_entries);
    [[nodiscard]] bool rename_style(std::string_view old_style_id,
                                    std::string_view new_style_id);
    [[nodiscard]] bool merge_style(std::string_view source_style_id,
                                   std::string_view target_style_id);
    [[nodiscard]] std::optional<featherdoc::style_prune_plan>
    plan_prune_unused_styles();
    [[nodiscard]] std::optional<featherdoc::style_prune_summary> prune_unused_styles();
    [[nodiscard]] bool ensure_paragraph_style(
        std::string_view style_id,
        const featherdoc::paragraph_style_definition &definition);
    [[nodiscard]] bool ensure_character_style(
        std::string_view style_id,
        const featherdoc::character_style_definition &definition);
    [[nodiscard]] bool ensure_table_style(
        std::string_view style_id,
        const featherdoc::table_style_definition &definition);
    [[nodiscard]] std::optional<std::string> style_run_font_family(std::string_view style_id);
    [[nodiscard]] std::optional<std::string> style_run_east_asia_font_family(
        std::string_view style_id);
    [[nodiscard]] std::optional<std::string> style_run_language(std::string_view style_id);
    [[nodiscard]] std::optional<std::string> style_run_east_asia_language(
        std::string_view style_id);
    [[nodiscard]] std::optional<std::string> style_run_bidi_language(
        std::string_view style_id);
    [[nodiscard]] std::optional<bool> style_run_rtl(std::string_view style_id);
    [[nodiscard]] std::optional<bool> style_paragraph_bidi(std::string_view style_id);
    [[nodiscard]] std::optional<std::string> paragraph_style_next_style(
        std::string_view style_id);
    [[nodiscard]] std::optional<std::uint32_t> paragraph_style_outline_level(
        std::string_view style_id);
    [[nodiscard]] bool materialize_style_run_properties(std::string_view style_id);
    [[nodiscard]] bool rebase_character_style_based_on(std::string_view style_id,
                                                       std::string_view based_on);
    [[nodiscard]] bool set_character_style_based_on(std::string_view style_id,
                                                    std::string_view based_on);
    [[nodiscard]] bool clear_character_style_based_on(std::string_view style_id);
    [[nodiscard]] bool rebase_paragraph_style_based_on(std::string_view style_id,
                                                       std::string_view based_on);
    [[nodiscard]] bool set_paragraph_style_based_on(std::string_view style_id,
                                                    std::string_view based_on);
    [[nodiscard]] bool clear_paragraph_style_based_on(std::string_view style_id);
    [[nodiscard]] bool set_paragraph_style_next_style(std::string_view style_id,
                                                      std::string_view next_style);
    [[nodiscard]] bool clear_paragraph_style_next_style(std::string_view style_id);
    [[nodiscard]] bool set_paragraph_style_outline_level(std::string_view style_id,
                                                         std::uint32_t outline_level);
    [[nodiscard]] bool clear_paragraph_style_outline_level(std::string_view style_id);
    [[nodiscard]] bool set_style_run_font_family(std::string_view style_id,
                                                 std::string_view font_family);
    [[nodiscard]] bool set_style_run_east_asia_font_family(std::string_view style_id,
                                                           std::string_view font_family);
    [[nodiscard]] bool set_style_run_language(std::string_view style_id,
                                              std::string_view language);
    [[nodiscard]] bool set_style_run_east_asia_language(std::string_view style_id,
                                                        std::string_view language);
    [[nodiscard]] bool set_style_run_bidi_language(std::string_view style_id,
                                                   std::string_view language);
    [[nodiscard]] bool set_style_run_rtl(std::string_view style_id,
                                         bool enabled = true);
    [[nodiscard]] bool set_style_paragraph_bidi(std::string_view style_id,
                                                bool enabled = true);
    [[nodiscard]] bool clear_style_run_font_family(std::string_view style_id);
    [[nodiscard]] bool clear_style_run_east_asia_font_family(std::string_view style_id);
    [[nodiscard]] bool clear_style_run_primary_language(std::string_view style_id);
    [[nodiscard]] bool clear_style_run_language(std::string_view style_id);
    [[nodiscard]] bool clear_style_run_east_asia_language(std::string_view style_id);
    [[nodiscard]] bool clear_style_run_bidi_language(std::string_view style_id);
    [[nodiscard]] bool clear_style_run_rtl(std::string_view style_id);
    [[nodiscard]] bool clear_style_paragraph_bidi(std::string_view style_id);
    [[nodiscard]] std::optional<std::uint32_t> ensure_style_linked_numbering(
        const featherdoc::numbering_definition &definition,
        const std::vector<featherdoc::paragraph_style_numbering_link> &style_links);
    [[nodiscard]] bool set_paragraph_style_numbering(
        std::string_view style_id, std::uint32_t numbering_definition_id,
        std::uint32_t level = 0U);
    [[nodiscard]] bool clear_paragraph_style_numbering(std::string_view style_id);
    [[nodiscard]] bool set_paragraph_style(Paragraph paragraph, std::string_view style_id);
    [[nodiscard]] bool clear_paragraph_style(Paragraph paragraph);
    [[nodiscard]] bool set_run_style(Run run, std::string_view style_id);
    [[nodiscard]] bool clear_run_style(Run run);
    [[nodiscard]] std::vector<featherdoc::table_inspection_summary> inspect_tables();
    [[nodiscard]] std::optional<featherdoc::table_inspection_summary>
    inspect_table(std::size_t table_index);
    [[nodiscard]] std::vector<featherdoc::table_cell_inspection_summary>
    inspect_table_cells(std::size_t table_index);
    [[nodiscard]] std::optional<featherdoc::table_cell_inspection_summary>
    inspect_table_cell(std::size_t table_index, std::size_t row_index, std::size_t cell_index);
    [[nodiscard]] std::vector<featherdoc::paragraph_inspection_summary> inspect_paragraphs();
    [[nodiscard]] std::optional<featherdoc::paragraph_inspection_summary>
    inspect_paragraph(std::size_t paragraph_index);
    [[nodiscard]] std::vector<featherdoc::run_inspection_summary>
    inspect_paragraph_runs(std::size_t paragraph_index);
    [[nodiscard]] std::optional<featherdoc::run_inspection_summary>
    inspect_paragraph_run(std::size_t paragraph_index, std::size_t run_index);

    Paragraph &paragraphs();
    Table &tables();
    Table append_table(std::size_t row_count = 1U, std::size_t column_count = 1U);
    [[nodiscard]] std::vector<drawing_image_info> drawing_images() const;
    [[nodiscard]] bool extract_drawing_image(
        std::size_t image_index, const std::filesystem::path &output_path) const;
    [[nodiscard]] bool remove_drawing_image(std::size_t image_index);
    [[nodiscard]] bool replace_drawing_image(std::size_t image_index,
                                             const std::filesystem::path &image_path);
    [[nodiscard]] std::vector<inline_image_info> inline_images() const;
    [[nodiscard]] bool extract_inline_image(
        std::size_t image_index, const std::filesystem::path &output_path) const;
    [[nodiscard]] bool remove_inline_image(std::size_t image_index);
    [[nodiscard]] bool replace_inline_image(std::size_t image_index,
                                            const std::filesystem::path &image_path);
    [[nodiscard]] bool append_image(const std::filesystem::path &image_path);
    [[nodiscard]] bool append_image(const std::filesystem::path &image_path,
                                    std::uint32_t width_px,
                                    std::uint32_t height_px);
    [[nodiscard]] bool append_floating_image(
        const std::filesystem::path &image_path,
        featherdoc::floating_image_options options = {});
    [[nodiscard]] bool append_floating_image(
        const std::filesystem::path &image_path, std::uint32_t width_px,
        std::uint32_t height_px,
        featherdoc::floating_image_options options = {});
};
} // namespace featherdoc

#endif
