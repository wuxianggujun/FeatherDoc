#include "featherdoc_cli_style_ensure_table_region_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_ensure_table_region_parse_support.hpp"

#include <cstdint>
#include <string>

namespace featherdoc_cli {

auto parse_table_style_fill_option(std::string_view text,
                                   featherdoc::table_style_definition &definition,
                                   std::string &error_message) -> bool {
    const auto fields = split_table_style_region_option_fields(text);
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
    const auto fields = split_table_style_region_option_fields(text);
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
    const auto fields = split_table_style_region_option_fields(text);
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
    const auto fields = split_table_style_region_option_fields(text);
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
    const auto fields = split_table_style_region_option_fields(text);
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
    const auto fields = split_table_style_region_option_fields(text);
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
    const auto fields = split_table_style_region_option_fields(text);
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
    const auto fields = split_table_style_region_option_fields(text);
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

} // namespace featherdoc_cli
