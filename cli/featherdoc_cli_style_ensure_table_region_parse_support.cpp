#include "featherdoc_cli_style_ensure_table_region_parse_support.hpp"

#include <optional>

namespace featherdoc_cli {
namespace {

auto table_style_region_option(
    featherdoc::table_style_definition &definition, std::string_view region_name)
    -> std::optional<featherdoc::table_style_region_definition> * {
    if (region_name == "whole_table" || region_name == "whole-table") {
        return &definition.whole_table;
    }
    if (region_name == "first_row" || region_name == "first-row") {
        return &definition.first_row;
    }
    if (region_name == "last_row" || region_name == "last-row") {
        return &definition.last_row;
    }
    if (region_name == "first_column" || region_name == "first-column") {
        return &definition.first_column;
    }
    if (region_name == "last_column" || region_name == "last-column") {
        return &definition.last_column;
    }
    if (region_name == "banded_rows" || region_name == "banded-rows") {
        return &definition.banded_rows;
    }
    if (region_name == "banded_columns" || region_name == "banded-columns") {
        return &definition.banded_columns;
    }
    if (region_name == "second_banded_rows" || region_name == "second-banded-rows" ||
        region_name == "banded_rows_2" || region_name == "banded-rows-2") {
        return &definition.second_banded_rows;
    }
    if (region_name == "second_banded_columns" ||
        region_name == "second-banded-columns" || region_name == "banded_columns_2" ||
        region_name == "banded-columns-2") {
        return &definition.second_banded_columns;
    }

    return nullptr;
}

} // namespace

auto split_table_style_region_option_fields(std::string_view text)
    -> std::vector<std::string_view> {
    auto fields = std::vector<std::string_view>{};
    std::size_t start = 0U;
    while (start <= text.size()) {
        const auto separator = text.find(':', start);
        if (separator == std::string_view::npos) {
            fields.push_back(text.substr(start));
            break;
        }

        fields.push_back(text.substr(start, separator - start));
        start = separator + 1U;
    }
    return fields;
}

auto ensure_table_style_region_option(
    featherdoc::table_style_definition &definition, std::string_view region_name,
    std::string_view option_name, std::string &error_message)
    -> featherdoc::table_style_region_definition * {
    auto *region = table_style_region_option(definition, region_name);
    if (region == nullptr) {
        error_message = "invalid " + std::string{option_name} +
                        " region: " + std::string{region_name};
        return nullptr;
    }
    if (!region->has_value()) {
        region->emplace();
    }
    return &(**region);
}

auto assign_table_style_margin(featherdoc::table_style_margins_definition &margins,
                               featherdoc::cell_margin_edge edge,
                               std::uint32_t value) -> void {
    switch (edge) {
    case featherdoc::cell_margin_edge::top:
        margins.top_twips = value;
        return;
    case featherdoc::cell_margin_edge::left:
        margins.left_twips = value;
        return;
    case featherdoc::cell_margin_edge::bottom:
        margins.bottom_twips = value;
        return;
    case featherdoc::cell_margin_edge::right:
        margins.right_twips = value;
        return;
    }
}

auto assign_table_style_border(featherdoc::table_style_borders_definition &borders,
                               featherdoc::table_border_edge edge,
                               featherdoc::border_definition border) -> void {
    switch (edge) {
    case featherdoc::table_border_edge::top:
        borders.top = border;
        return;
    case featherdoc::table_border_edge::left:
        borders.left = border;
        return;
    case featherdoc::table_border_edge::bottom:
        borders.bottom = border;
        return;
    case featherdoc::table_border_edge::right:
        borders.right = border;
        return;
    case featherdoc::table_border_edge::inside_horizontal:
        borders.inside_horizontal = border;
        return;
    case featherdoc::table_border_edge::inside_vertical:
        borders.inside_vertical = border;
        return;
    }
}

auto ensure_table_style_paragraph_spacing_option(
    featherdoc::table_style_region_definition &region)
    -> featherdoc::table_style_paragraph_spacing_definition & {
    return region.paragraph_spacing.has_value() ? *region.paragraph_spacing
                                                : region.paragraph_spacing.emplace();
}

} // namespace featherdoc_cli
