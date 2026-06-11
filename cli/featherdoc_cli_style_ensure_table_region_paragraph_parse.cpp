#include "featherdoc_cli_style_ensure_table_region_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_ensure_table_region_parse_support.hpp"

#include <cstdint>
#include <string>

namespace featherdoc_cli {

auto parse_table_style_paragraph_alignment_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_table_style_region_option_fields(text);
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

auto parse_table_style_paragraph_spacing_before_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool {
    const auto fields = split_table_style_region_option_fields(text);
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
    const auto fields = split_table_style_region_option_fields(text);
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
    const auto fields = split_table_style_region_option_fields(text);
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

} // namespace featherdoc_cli
