#pragma once

#include <constants.hpp>
#include <featherdoc/styles.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace featherdoc {

struct table_style_margins_definition {
    std::optional<std::uint32_t> top_twips;
    std::optional<std::uint32_t> left_twips;
    std::optional<std::uint32_t> bottom_twips;
    std::optional<std::uint32_t> right_twips;
};

struct table_style_borders_definition {
    std::optional<featherdoc::border_definition> top;
    std::optional<featherdoc::border_definition> left;
    std::optional<featherdoc::border_definition> bottom;
    std::optional<featherdoc::border_definition> right;
    std::optional<featherdoc::border_definition> inside_horizontal;
    std::optional<featherdoc::border_definition> inside_vertical;
};

struct table_style_paragraph_spacing_definition {
    std::optional<std::uint32_t> before_twips;
    std::optional<std::uint32_t> after_twips;
    std::optional<std::uint32_t> line_twips;
    std::optional<featherdoc::paragraph_line_spacing_rule> line_rule;
};

struct table_style_region_definition {
    std::optional<std::string> fill_color;
    std::optional<std::string> text_color;
    std::optional<bool> bold;
    std::optional<bool> italic;
    std::optional<std::uint32_t> font_size_points;
    std::optional<std::string> font_family;
    std::optional<std::string> east_asia_font_family;
    std::optional<featherdoc::cell_vertical_alignment> cell_vertical_alignment;
    std::optional<featherdoc::cell_text_direction> cell_text_direction;
    std::optional<featherdoc::paragraph_alignment> paragraph_alignment;
    std::optional<featherdoc::table_style_paragraph_spacing_definition>
        paragraph_spacing;
    std::optional<featherdoc::table_style_margins_definition> cell_margins;
    std::optional<featherdoc::table_style_borders_definition> borders;
};

struct table_style_definition {
    std::string name;
    std::optional<std::string> based_on;
    bool is_custom{true};
    bool is_semi_hidden{false};
    bool is_unhide_when_used{false};
    bool is_quick_format{false};
    std::optional<featherdoc::table_style_region_definition> whole_table;
    std::optional<featherdoc::table_style_region_definition> first_row;
    std::optional<featherdoc::table_style_region_definition> last_row;
    std::optional<featherdoc::table_style_region_definition> first_column;
    std::optional<featherdoc::table_style_region_definition> last_column;
    std::optional<featherdoc::table_style_region_definition> banded_rows;
    std::optional<featherdoc::table_style_region_definition> banded_columns;
    std::optional<featherdoc::table_style_region_definition> second_banded_rows;
    std::optional<featherdoc::table_style_region_definition>
        second_banded_columns;
};

struct table_style_border_summary {
    std::optional<std::string> style;
    std::optional<std::uint32_t> size_eighth_points;
    std::optional<std::string> color;
    std::optional<std::uint32_t> space_points;
};

struct table_style_borders_summary {
    std::optional<featherdoc::table_style_border_summary> top;
    std::optional<featherdoc::table_style_border_summary> left;
    std::optional<featherdoc::table_style_border_summary> bottom;
    std::optional<featherdoc::table_style_border_summary> right;
    std::optional<featherdoc::table_style_border_summary> inside_horizontal;
    std::optional<featherdoc::table_style_border_summary> inside_vertical;
};

struct table_style_region_summary {
    std::optional<std::string> fill_color;
    std::optional<std::string> text_color;
    std::optional<bool> bold;
    std::optional<bool> italic;
    std::optional<std::uint32_t> font_size_points;
    std::optional<std::string> font_family;
    std::optional<std::string> east_asia_font_family;
    std::optional<featherdoc::cell_vertical_alignment> cell_vertical_alignment;
    std::optional<featherdoc::cell_text_direction> cell_text_direction;
    std::optional<featherdoc::paragraph_alignment> paragraph_alignment;
    std::optional<featherdoc::table_style_paragraph_spacing_definition>
        paragraph_spacing;
    std::optional<featherdoc::table_style_margins_definition> cell_margins;
    std::optional<featherdoc::table_style_borders_summary> borders;
};

struct table_style_definition_summary {
    featherdoc::style_summary style;
    std::optional<featherdoc::table_style_region_summary> whole_table;
    std::optional<featherdoc::table_style_region_summary> first_row;
    std::optional<featherdoc::table_style_region_summary> last_row;
    std::optional<featherdoc::table_style_region_summary> first_column;
    std::optional<featherdoc::table_style_region_summary> last_column;
    std::optional<featherdoc::table_style_region_summary> banded_rows;
    std::optional<featherdoc::table_style_region_summary> banded_columns;
    std::optional<featherdoc::table_style_region_summary> second_banded_rows;
    std::optional<featherdoc::table_style_region_summary> second_banded_columns;
};

struct table_style_region_audit_issue {
    std::string style_id;
    std::string style_name;
    std::string region;
    std::string issue_type;
    std::size_t property_count{0};
    std::string suggestion;
};

struct table_style_region_audit_report {
    std::size_t table_style_count{0};
    std::size_t region_count{0};
    std::vector<featherdoc::table_style_region_audit_issue> issues;

    [[nodiscard]] std::size_t issue_count() const noexcept {
        return this->issues.size();
    }

    [[nodiscard]] bool ok() const noexcept { return this->issues.empty(); }
};

struct table_style_inheritance_audit_issue {
    std::string style_id;
    std::string style_name;
    std::string based_on_style_id;
    std::string based_on_style_kind;
    std::string issue_type;
    std::vector<std::string> inheritance_chain;
    std::string suggestion;
};

struct table_style_inheritance_audit_report {
    std::size_t table_style_count{0};
    std::vector<featherdoc::table_style_inheritance_audit_issue> issues;

    [[nodiscard]] std::size_t issue_count() const noexcept {
        return this->issues.size();
    }

    [[nodiscard]] bool ok() const noexcept { return this->issues.empty(); }
};

struct table_style_look_consistency_issue {
    std::size_t table_index{0};
    std::string style_id;
    std::string issue_type;
    std::string region;
    std::string required_style_look_flag;
    std::optional<bool> actual_value;
    bool expected_value{true};
    std::string suggestion;
};

struct table_style_look_consistency_report {
    std::size_t table_count{0};
    std::vector<featherdoc::table_style_look_consistency_issue> issues;

    [[nodiscard]] std::size_t issue_count() const noexcept {
        return this->issues.size();
    }

    [[nodiscard]] bool ok() const noexcept { return this->issues.empty(); }
};

struct table_style_look_repair_report {
    featherdoc::table_style_look_consistency_report before;
    featherdoc::table_style_look_consistency_report after;
    std::size_t changed_table_count{0};

    [[nodiscard]] bool changed() const noexcept {
        return this->changed_table_count > 0U;
    }

    [[nodiscard]] bool ok() const noexcept { return this->after.ok(); }
};

struct table_style_quality_audit_report {
    featherdoc::table_style_region_audit_report region_audit;
    featherdoc::table_style_inheritance_audit_report inheritance_audit;
    featherdoc::table_style_look_consistency_report style_look;

    [[nodiscard]] std::size_t issue_count() const noexcept {
        return this->region_audit.issue_count() +
               this->inheritance_audit.issue_count() +
               this->style_look.issue_count();
    }

    [[nodiscard]] bool ok() const noexcept { return this->issue_count() == 0U; }
};

struct table_style_quality_fix_item {
    std::string source;
    std::string issue_type;
    std::optional<std::size_t> table_index;
    std::string style_id;
    std::string style_name;
    std::string region;
    std::string action;
    bool automatic{false};
    std::string command;
    std::string suggestion;
};

struct table_style_quality_fix_plan {
    featherdoc::table_style_quality_audit_report audit;
    std::vector<featherdoc::table_style_quality_fix_item> items;

    [[nodiscard]] std::size_t issue_count() const noexcept {
        return this->audit.issue_count();
    }

    [[nodiscard]] std::size_t automatic_fix_count() const noexcept {
        auto count = std::size_t{0U};
        for (const auto &item : this->items) {
            if (item.automatic) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] std::size_t manual_fix_count() const noexcept {
        return this->items.size() - this->automatic_fix_count();
    }

    [[nodiscard]] bool ok() const noexcept { return this->audit.ok(); }
};

} // namespace featherdoc
