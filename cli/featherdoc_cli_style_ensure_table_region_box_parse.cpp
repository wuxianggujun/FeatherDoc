#include "featherdoc_cli_style_ensure_table_region_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_ensure_table_region_parse_support.hpp"

#include <cstdint>
#include <string>

namespace featherdoc_cli {

auto parse_table_style_margin_option(std::string_view text,
                                     featherdoc::table_style_definition &definition,
                                     std::string &error_message) -> bool {
    const auto fields = split_table_style_region_option_fields(text);
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
    const auto fields = split_table_style_region_option_fields(text);
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

} // namespace featherdoc_cli
