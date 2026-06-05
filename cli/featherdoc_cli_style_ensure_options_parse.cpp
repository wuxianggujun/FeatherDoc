#include "featherdoc_cli_style_ensure_options_parse.hpp"
#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {

namespace {

using path_type = std::filesystem::path;

template <typename Definition>
auto parse_style_catalog_option(const std::vector<std::string_view> &arguments,
                                std::size_t &index, Definition &definition,
                                std::string &error_message)
    -> option_parse_result {
    const auto argument = arguments[index];
    if (argument == "--name") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --name";
            return option_parse_result::error;
        }

        definition.name = std::string(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--based-on") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --based-on";
            return option_parse_result::error;
        }

        definition.based_on = std::string(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--custom") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --custom";
            return option_parse_result::error;
        }

        bool value = false;
        if (!parse_bool(arguments[index + 1U], value)) {
            error_message = "invalid --custom value: " +
                            std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        definition.is_custom = value;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--semi-hidden") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --semi-hidden";
            return option_parse_result::error;
        }

        bool value = false;
        if (!parse_bool(arguments[index + 1U], value)) {
            error_message = "invalid --semi-hidden value: " +
                            std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        definition.is_semi_hidden = value;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--unhide-when-used") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --unhide-when-used";
            return option_parse_result::error;
        }

        bool value = false;
        if (!parse_bool(arguments[index + 1U], value)) {
            error_message = "invalid --unhide-when-used value: " +
                            std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        definition.is_unhide_when_used = value;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--quick-format") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --quick-format";
            return option_parse_result::error;
        }

        bool value = false;
        if (!parse_bool(arguments[index + 1U], value)) {
            error_message = "invalid --quick-format value: " +
                            std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        definition.is_quick_format = value;
        ++index;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

auto split_colon_fields(std::string_view text) -> std::vector<std::string_view> {
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

auto parse_table_style_fill_option(std::string_view text,
                                   featherdoc::table_style_definition &definition,
                                   std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-fill value: expected <region>:<color>";
        return false;
    }

    auto *region = ensure_table_style_region_option(definition, fields[0],
                                                    "--style-fill", error_message);
    if (region == nullptr) {
        return false;
    }
    region->fill_color = std::string{fields[1]};
    return true;
}

auto parse_table_style_text_color_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-text-color value: expected <region>:<color>";
        return false;
    }

    auto *region = ensure_table_style_region_option(
        definition, fields[0], "--style-text-color", error_message);
    if (region == nullptr) {
        return false;
    }
    region->text_color = std::string{fields[1]};
    return true;
}

auto parse_table_style_bold_option(std::string_view text,
                                     featherdoc::table_style_definition &definition,
                                     std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-bold value: expected <region>:true|false";
        return false;
    }

    auto *region = ensure_table_style_region_option(definition, fields[0],
                                                    "--style-bold", error_message);
    if (region == nullptr) {
        return false;
    }

    auto value = false;
    if (!parse_bool(fields[1], value)) {
        error_message = "invalid --style-bold value: " + std::string{fields[1]};
        return false;
    }

    region->bold = value;
    return true;
}

auto parse_table_style_italic_option(std::string_view text,
                                       featherdoc::table_style_definition &definition,
                                       std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-italic value: expected <region>:true|false";
        return false;
    }

    auto *region = ensure_table_style_region_option(definition, fields[0],
                                                    "--style-italic", error_message);
    if (region == nullptr) {
        return false;
    }

    auto value = false;
    if (!parse_bool(fields[1], value)) {
        error_message = "invalid --style-italic value: " + std::string{fields[1]};
        return false;
    }

    region->italic = value;
    return true;
}

auto parse_table_style_font_size_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-font-size value: expected <region>:<points>";
        return false;
    }

    auto *region = ensure_table_style_region_option(definition, fields[0],
                                                    "--style-font-size", error_message);
    if (region == nullptr) {
        return false;
    }

    auto value = std::uint32_t{};
    if (!parse_uint32(fields[1], value) || value == 0U) {
        error_message = "invalid --style-font-size points: " + std::string{fields[1]};
        return false;
    }

    region->font_size_points = value;
    return true;
}

auto parse_table_style_font_family_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    bool east_asia, std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    const auto option_name = east_asia ? "--style-east-asia-font-family"
                                      : "--style-font-family";
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = std::string{"invalid "} + option_name +
                        " value: expected <region>:<name>";
        return false;
    }

    auto *region = ensure_table_style_region_option(definition, fields[0],
                                                    option_name, error_message);
    if (region == nullptr) {
        return false;
    }

    if (east_asia) {
        region->east_asia_font_family = std::string{fields[1]};
    } else {
        region->font_family = std::string{fields[1]};
    }
    return true;
}

auto parse_table_style_cell_vertical_alignment_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-cell-vertical-alignment value: expected "
                        "<region>:<top|center|bottom|both>";
        return false;
    }

    auto *region = ensure_table_style_region_option(
        definition, fields[0], "--style-cell-vertical-alignment", error_message);
    if (region == nullptr) {
        return false;
    }

    auto alignment = featherdoc::cell_vertical_alignment::top;
    if (!parse_table_style_cell_vertical_alignment_text(fields[1], alignment)) {
        error_message =
            "invalid --style-cell-vertical-alignment value: " + std::string{fields[1]};
        return false;
    }

    region->cell_vertical_alignment = alignment;
    return true;
}

auto parse_table_style_cell_text_direction_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-cell-text-direction value: expected "
                        "<region>:<direction>";
        return false;
    }

    auto *region = ensure_table_style_region_option(
        definition, fields[0], "--style-cell-text-direction", error_message);
    if (region == nullptr) {
        return false;
    }

    auto direction = featherdoc::cell_text_direction::left_to_right_top_to_bottom;
    if (!parse_table_style_cell_text_direction_text(fields[1], direction)) {
        error_message =
            "invalid --style-cell-text-direction value: " + std::string{fields[1]};
        return false;
    }

    region->cell_text_direction = direction;
    return true;
}

auto parse_table_style_paragraph_alignment_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-paragraph-alignment value: expected "
                        "<region>:<left|center|right|justified|distribute>";
        return false;
    }

    auto *region = ensure_table_style_region_option(
        definition, fields[0], "--style-paragraph-alignment", error_message);
    if (region == nullptr) {
        return false;
    }

    auto alignment = featherdoc::paragraph_alignment::left;
    if (!parse_table_style_paragraph_alignment_text(fields[1], alignment)) {
        error_message =
            "invalid --style-paragraph-alignment value: " + std::string{fields[1]};
        return false;
    }

    region->paragraph_alignment = alignment;
    return true;
}

auto ensure_table_style_paragraph_spacing_option(
    featherdoc::table_style_region_definition &region)
    -> featherdoc::table_style_paragraph_spacing_definition & {
    return region.paragraph_spacing.has_value() ? *region.paragraph_spacing
                                                : region.paragraph_spacing.emplace();
}

auto parse_table_style_paragraph_spacing_before_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-paragraph-spacing-before value: expected "
                        "<region>:<twips>";
        return false;
    }

    auto *region = ensure_table_style_region_option(
        definition, fields[0], "--style-paragraph-spacing-before", error_message);
    if (region == nullptr) {
        return false;
    }

    auto value = std::uint32_t{};
    if (!parse_uint32(fields[1], value)) {
        error_message = "invalid --style-paragraph-spacing-before twips: " +
                        std::string{fields[1]};
        return false;
    }

    ensure_table_style_paragraph_spacing_option(*region).before_twips = value;
    return true;
}

auto parse_table_style_paragraph_spacing_after_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 2U || fields[0].empty() || fields[1].empty()) {
        error_message = "invalid --style-paragraph-spacing-after value: expected "
                        "<region>:<twips>";
        return false;
    }

    auto *region = ensure_table_style_region_option(
        definition, fields[0], "--style-paragraph-spacing-after", error_message);
    if (region == nullptr) {
        return false;
    }

    auto value = std::uint32_t{};
    if (!parse_uint32(fields[1], value)) {
        error_message = "invalid --style-paragraph-spacing-after twips: " +
                        std::string{fields[1]};
        return false;
    }

    ensure_table_style_paragraph_spacing_option(*region).after_twips = value;
    return true;
}

auto parse_table_style_paragraph_line_spacing_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 3U || fields[0].empty() || fields[1].empty() ||
        fields[2].empty()) {
        error_message = "invalid --style-paragraph-line-spacing value: expected "
                        "<region>:<twips>:<auto|at_least|exact>";
        return false;
    }

    auto *region = ensure_table_style_region_option(
        definition, fields[0], "--style-paragraph-line-spacing", error_message);
    if (region == nullptr) {
        return false;
    }

    auto line_twips = std::uint32_t{};
    if (!parse_uint32(fields[1], line_twips)) {
        error_message = "invalid --style-paragraph-line-spacing twips: " +
                        std::string{fields[1]};
        return false;
    }

    auto line_rule = featherdoc::paragraph_line_spacing_rule::automatic;
    if (!parse_table_style_paragraph_line_spacing_rule_text(fields[2], line_rule)) {
        error_message = "invalid --style-paragraph-line-spacing rule: " +
                        std::string{fields[2]};
        return false;
    }

    auto &spacing = ensure_table_style_paragraph_spacing_option(*region);
    spacing.line_twips = line_twips;
    spacing.line_rule = line_rule;
    return true;
}

auto parse_table_style_margin_option(std::string_view text,
                                     featherdoc::table_style_definition &definition,
                                     std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if (fields.size() != 3U || fields[0].empty() || fields[1].empty() ||
        fields[2].empty()) {
        error_message =
            "invalid --style-margin value: expected <region>:<edge>:<twips>";
        return false;
    }

    auto *region = ensure_table_style_region_option(definition, fields[0],
                                                    "--style-margin", error_message);
    if (region == nullptr) {
        return false;
    }

    auto edge = featherdoc::cell_margin_edge::top;
    if (!parse_table_style_margin_edge_text(fields[1], edge)) {
        error_message = "invalid --style-margin edge: " + std::string{fields[1]};
        return false;
    }

    auto value = std::uint32_t{};
    if (!parse_uint32(fields[2], value)) {
        error_message = "invalid --style-margin twips: " + std::string{fields[2]};
        return false;
    }

    auto &margins = region->cell_margins.has_value() ? *region->cell_margins
                                                     : region->cell_margins.emplace();
    assign_table_style_margin(margins, edge, value);
    return true;
}

auto parse_table_style_border_option(std::string_view text,
                                     featherdoc::table_style_definition &definition,
                                     std::string &error_message) -> bool {
    const auto fields = split_colon_fields(text);
    if ((fields.size() != 5U && fields.size() != 6U) || fields[0].empty() ||
        fields[1].empty() || fields[2].empty() || fields[3].empty() ||
        fields[4].empty()) {
        error_message = "invalid --style-border value: expected "
                        "<region>:<edge>:<style>:<size>:<color>[:space]";
        return false;
    }

    auto *region = ensure_table_style_region_option(definition, fields[0],
                                                    "--style-border", error_message);
    if (region == nullptr) {
        return false;
    }

    auto edge = featherdoc::table_border_edge::top;
    if (!parse_table_style_border_edge_text(fields[1], edge)) {
        error_message = "invalid --style-border edge: " + std::string{fields[1]};
        return false;
    }

    auto style = featherdoc::border_style::single;
    if (!parse_table_style_border_style_text(fields[2], style)) {
        error_message = "invalid --style-border style: " + std::string{fields[2]};
        return false;
    }

    auto size = std::uint32_t{};
    if (!parse_uint32(fields[3], size)) {
        error_message = "invalid --style-border size: " + std::string{fields[3]};
        return false;
    }

    auto space = std::uint32_t{0U};
    if (fields.size() == 6U && !parse_uint32(fields[5], space)) {
        error_message = "invalid --style-border space: " + std::string{fields[5]};
        return false;
    }

    auto &borders = region->borders.has_value() ? *region->borders
                                                : region->borders.emplace();
    assign_table_style_border(
        borders, edge,
        featherdoc::border_definition{style, size, fields[4], space});
    return true;
}

template <typename Definition>
auto parse_run_style_option(const std::vector<std::string_view> &arguments,
                            std::size_t &index, Definition &definition,
                            std::string &error_message) -> option_parse_result {
    const auto argument = arguments[index];
    if (argument == "--run-font-family") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --run-font-family";
            return option_parse_result::error;
        }

        definition.run_font_family = std::string(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--run-east-asia-font-family") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --run-east-asia-font-family";
            return option_parse_result::error;
        }

        definition.run_east_asia_font_family =
            std::string(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--run-language") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --run-language";
            return option_parse_result::error;
        }

        definition.run_language = std::string(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--run-east-asia-language") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --run-east-asia-language";
            return option_parse_result::error;
        }

        definition.run_east_asia_language =
            std::string(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--run-bidi-language") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --run-bidi-language";
            return option_parse_result::error;
        }

        definition.run_bidi_language = std::string(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--run-rtl") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --run-rtl";
            return option_parse_result::error;
        }

        bool value = false;
        if (!parse_bool(arguments[index + 1U], value)) {
            error_message = "invalid --run-rtl value: " +
                            std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        definition.run_rtl = value;
        ++index;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

} // namespace

auto parse_ensure_paragraph_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_paragraph_style_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        if (const auto result = parse_style_catalog_option(arguments, index,
                                                           options.definition,
                                                           error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        if (const auto result = parse_run_style_option(arguments, index,
                                                       options.definition,
                                                       error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        const auto argument = arguments[index];
        if (argument == "--next-style") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --next-style";
                return false;
            }

            options.definition.next_style = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--paragraph-bidi") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --paragraph-bidi";
                return false;
            }

            bool value = false;
            if (!parse_bool(arguments[index + 1U], value)) {
                error_message = "invalid --paragraph-bidi value: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.definition.paragraph_bidi = value;
            ++index;
            continue;
        }

        if (argument == "--outline-level") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --outline-level";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid outline level: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.definition.outline_level = value;
            ++index;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (options.definition.name.empty()) {
        error_message = "missing required --name <name>";
        return false;
    }

    return true;
}

auto parse_ensure_character_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_character_style_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        if (const auto result = parse_style_catalog_option(arguments, index,
                                                           options.definition,
                                                           error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        if (const auto result = parse_run_style_option(arguments, index,
                                                       options.definition,
                                                       error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        const auto argument = arguments[index];
        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (options.definition.name.empty()) {
        error_message = "missing required --name <name>";
        return false;
    }

    return true;
}

auto parse_ensure_table_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_table_style_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        if (const auto result = parse_style_catalog_option(arguments, index,
                                                           options.definition,
                                                           error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        const auto argument = arguments[index];
        if (argument == "--style-fill") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-fill";
                return false;
            }
            if (!parse_table_style_fill_option(arguments[index + 1U],
                                               options.definition,
                                               error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-text-color") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-text-color";
                return false;
            }
            if (!parse_table_style_text_color_option(arguments[index + 1U],
                                                     options.definition,
                                                     error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-bold") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-bold";
                return false;
            }
            if (!parse_table_style_bold_option(arguments[index + 1U],
                                               options.definition, error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-italic") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-italic";
                return false;
            }
            if (!parse_table_style_italic_option(arguments[index + 1U],
                                                 options.definition,
                                                 error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-font-size") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-font-size";
                return false;
            }
            if (!parse_table_style_font_size_option(arguments[index + 1U],
                                                    options.definition,
                                                    error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-font-family" ||
            argument == "--style-east-asia-font-family") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after " + std::string{argument};
                return false;
            }
            if (!parse_table_style_font_family_option(
                    arguments[index + 1U], options.definition,
                    argument == "--style-east-asia-font-family", error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-cell-vertical-alignment") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-cell-vertical-alignment";
                return false;
            }
            if (!parse_table_style_cell_vertical_alignment_option(
                    arguments[index + 1U], options.definition, error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-cell-text-direction") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-cell-text-direction";
                return false;
            }
            if (!parse_table_style_cell_text_direction_option(
                    arguments[index + 1U], options.definition, error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-paragraph-alignment") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-paragraph-alignment";
                return false;
            }
            if (!parse_table_style_paragraph_alignment_option(
                    arguments[index + 1U], options.definition, error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-paragraph-spacing-before") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-paragraph-spacing-before";
                return false;
            }
            if (!parse_table_style_paragraph_spacing_before_option(
                    arguments[index + 1U], options.definition, error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-paragraph-spacing-after") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-paragraph-spacing-after";
                return false;
            }
            if (!parse_table_style_paragraph_spacing_after_option(
                    arguments[index + 1U], options.definition, error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-paragraph-line-spacing") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-paragraph-line-spacing";
                return false;
            }
            if (!parse_table_style_paragraph_line_spacing_option(
                    arguments[index + 1U], options.definition, error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-margin") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-margin";
                return false;
            }
            if (!parse_table_style_margin_option(arguments[index + 1U],
                                                 options.definition,
                                                 error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-border") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-border";
                return false;
            }
            if (!parse_table_style_border_option(arguments[index + 1U],
                                                 options.definition,
                                                 error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (options.definition.name.empty()) {
        error_message = "missing required --name <name>";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
