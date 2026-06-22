#pragma once

#include <constants.hpp>
#include <featherdoc/text.hpp>
#include <featherdoc_iterator.hpp>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <pugixml.hpp>

namespace featherdoc {

class Document;

struct border_inspection_summary {
    featherdoc::border_style style{featherdoc::border_style::single};
    std::uint32_t size_eighth_points{4U};
    std::string color{"auto"};
    std::uint32_t space_points{0U};
};

struct table_position {
    featherdoc::table_position_horizontal_reference horizontal_reference{
        featherdoc::table_position_horizontal_reference::margin};
    std::int32_t horizontal_offset_twips{0};
    std::optional<featherdoc::table_position_horizontal_spec> horizontal_spec;
    featherdoc::table_position_vertical_reference vertical_reference{
        featherdoc::table_position_vertical_reference::paragraph};
    std::int32_t vertical_offset_twips{0};
    std::optional<featherdoc::table_position_vertical_spec> vertical_spec;
    std::optional<std::uint32_t> left_from_text_twips;
    std::optional<std::uint32_t> right_from_text_twips;
    std::optional<std::uint32_t> top_from_text_twips;
    std::optional<std::uint32_t> bottom_from_text_twips;
    std::optional<featherdoc::table_overlap> overlap;
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
    [[nodiscard]] std::optional<std::size_t> column_index() const;
    [[nodiscard]] bool merge_right(std::size_t additional_cells = 1U);
    [[nodiscard]] bool merge_down(std::size_t additional_rows = 1U);
    [[nodiscard]] bool unmerge_right();
    [[nodiscard]] bool unmerge_down();
    [[nodiscard]] featherdoc::cell_vertical_merge vertical_merge() const;
    [[nodiscard]] std::size_t row_span() const;
    [[nodiscard]] std::optional<featherdoc::cell_vertical_alignment>
    vertical_alignment() const;
    [[nodiscard]] bool
    set_vertical_alignment(featherdoc::cell_vertical_alignment alignment);
    [[nodiscard]] bool clear_vertical_alignment();
    [[nodiscard]] std::optional<featherdoc::cell_text_direction>
    text_direction() const;
    [[nodiscard]] bool
    set_text_direction(featherdoc::cell_text_direction direction);
    [[nodiscard]] bool clear_text_direction();
    [[nodiscard]] std::optional<std::string> fill_color() const;
    [[nodiscard]] bool set_fill_color(std::string_view fill_color);
    [[nodiscard]] bool clear_fill_color();
    [[nodiscard]] std::optional<std::uint32_t>
    margin_twips(featherdoc::cell_margin_edge edge) const;
    [[nodiscard]] bool set_margin_twips(featherdoc::cell_margin_edge edge,
                                        std::uint32_t margin_twips);
    [[nodiscard]] bool clear_margin(featherdoc::cell_margin_edge edge);
    [[nodiscard]] std::optional<featherdoc::border_inspection_summary>
    border(featherdoc::cell_border_edge edge) const;
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
    [[nodiscard]] std::optional<TableCell>
    find_cell_by_grid_column(std::size_t grid_column);
    [[nodiscard]] std::optional<std::uint32_t> height_twips() const;
    [[nodiscard]] std::optional<featherdoc::row_height_rule>
    height_rule() const;
    [[nodiscard]] bool
    set_height_twips(std::uint32_t height_twips,
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
    [[nodiscard]] std::optional<TableCell>
    find_cell_by_grid_column(std::size_t row_index, std::size_t grid_column);
    [[nodiscard]] std::optional<std::uint32_t> width_twips() const;
    [[nodiscard]] bool set_width_twips(std::uint32_t width_twips);
    [[nodiscard]] bool clear_width();
    [[nodiscard]] bool set_cell_text(std::size_t row_index,
                                     std::size_t cell_index,
                                     const std::string &text);
    [[nodiscard]] bool set_cell_text_by_grid_column(std::size_t row_index,
                                                    std::size_t grid_column,
                                                    const std::string &text);
    [[nodiscard]] bool set_row_texts(std::size_t row_index,
                                     const std::vector<std::string> &texts);
    [[nodiscard]] bool set_row_texts(std::size_t row_index,
                                     std::initializer_list<std::string> texts);
    [[nodiscard]] bool
    set_rows_texts(std::size_t start_row_index,
                   const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] bool set_rows_texts(
        std::size_t start_row_index,
        std::initializer_list<std::initializer_list<std::string>> rows);
    [[nodiscard]] bool
    set_cell_block_texts(std::size_t start_row_index,
                         std::size_t start_cell_index,
                         const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] bool set_cell_block_texts(
        std::size_t start_row_index, std::size_t start_cell_index,
        std::initializer_list<std::initializer_list<std::string>> rows);
    [[nodiscard]] std::optional<std::uint32_t>
    column_width_twips(std::size_t column_index) const;
    [[nodiscard]] bool set_column_width_twips(std::size_t column_index,
                                              std::uint32_t width_twips);
    [[nodiscard]] bool clear_column_width(std::size_t column_index);
    [[nodiscard]] std::optional<featherdoc::table_layout_mode>
    layout_mode() const;
    [[nodiscard]] bool
    set_layout_mode(featherdoc::table_layout_mode layout_mode);
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
    [[nodiscard]] std::optional<featherdoc::table_position> position() const;
    [[nodiscard]] bool set_position(featherdoc::table_position position);
    [[nodiscard]] bool clear_position();
    [[nodiscard]] std::optional<std::uint32_t>
    cell_margin_twips(featherdoc::cell_margin_edge edge) const;
    [[nodiscard]] bool set_cell_margin_twips(featherdoc::cell_margin_edge edge,
                                             std::uint32_t margin_twips);
    [[nodiscard]] bool clear_cell_margin(featherdoc::cell_margin_edge edge);
    [[nodiscard]] std::optional<std::string> style_id() const;
    [[nodiscard]] bool set_style_id(std::string_view style_id);
    [[nodiscard]] bool clear_style_id();
    [[nodiscard]] std::optional<featherdoc::table_style_look>
    style_look() const;
    [[nodiscard]] bool set_style_look(featherdoc::table_style_look style_look);
    [[nodiscard]] bool clear_style_look();
    [[nodiscard]] std::optional<featherdoc::border_inspection_summary>
    border(featherdoc::table_border_edge edge) const;
    [[nodiscard]] bool set_border(featherdoc::table_border_edge edge,
                                  featherdoc::border_definition border);
    [[nodiscard]] bool clear_border(featherdoc::table_border_edge edge);
    [[nodiscard]] bool remove();
    Table insert_table_before(std::size_t row_count = 1U,
                              std::size_t column_count = 1U);
    Table insert_table_after(std::size_t row_count = 1U,
                             std::size_t column_count = 1U);
    Paragraph insert_paragraph_after(
        const std::string &text = {},
        featherdoc::formatting_flag formatting =
            featherdoc::formatting_flag::none);
    Table insert_table_like_before();
    Table insert_table_like_after();
    TableRow append_row(std::size_t cell_count = 1U);
};

struct table_borders_inspection_summary {
    std::optional<featherdoc::border_inspection_summary> top;
    std::optional<featherdoc::border_inspection_summary> left;
    std::optional<featherdoc::border_inspection_summary> bottom;
    std::optional<featherdoc::border_inspection_summary> right;
    std::optional<featherdoc::border_inspection_summary> inside_horizontal;
    std::optional<featherdoc::border_inspection_summary> inside_vertical;
};

struct table_inspection_summary {
    std::size_t index{0};
    std::optional<std::string> style_id;
    std::optional<std::uint32_t> width_twips;
    std::optional<featherdoc::table_layout_mode> layout_mode;
    std::optional<featherdoc::table_alignment> alignment;
    std::optional<std::uint32_t> indent_twips;
    std::optional<std::uint32_t> cell_spacing_twips;
    std::optional<featherdoc::table_position> position;
    std::optional<std::uint32_t> cell_margin_top_twips;
    std::optional<std::uint32_t> cell_margin_left_twips;
    std::optional<std::uint32_t> cell_margin_bottom_twips;
    std::optional<std::uint32_t> cell_margin_right_twips;
    std::optional<featherdoc::table_borders_inspection_summary> borders;
    std::size_t row_count{0};
    std::size_t column_count{0};
    std::vector<std::optional<std::uint32_t>> column_widths;
    std::vector<std::optional<std::uint32_t>> row_height_twips;
    std::vector<std::optional<featherdoc::row_height_rule>> row_height_rules;
    std::vector<bool> row_cant_split;
    std::vector<bool> row_repeats_header;
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
    std::size_t row_span{1};
    featherdoc::cell_vertical_merge vertical_merge{
        featherdoc::cell_vertical_merge::none};
    std::size_t paragraph_count{0};
    std::optional<std::uint32_t> width_twips;
    std::optional<featherdoc::cell_vertical_alignment> vertical_alignment;
    std::optional<featherdoc::cell_text_direction> text_direction;
    std::optional<std::string> fill_color;
    std::optional<std::uint32_t> margin_top_twips;
    std::optional<std::uint32_t> margin_left_twips;
    std::optional<std::uint32_t> margin_bottom_twips;
    std::optional<std::uint32_t> margin_right_twips;
    std::optional<featherdoc::border_inspection_summary> border_top;
    std::optional<featherdoc::border_inspection_summary> border_left;
    std::optional<featherdoc::border_inspection_summary> border_bottom;
    std::optional<featherdoc::border_inspection_summary> border_right;
    std::string text;
};

} // namespace featherdoc
