#include "featherdoc_cli_table_position_set_options_parse_detail.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace featherdoc_cli {
namespace {

[[nodiscard]] auto parse_table_position_twips_spacing_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::string_view option_name, std::string_view value_label,
    std::optional<std::uint32_t> &target,
    std::string &error_message) -> option_parse_result {
    if (arguments[index] != option_name) {
        return option_parse_result::not_matched;
    }
    if (target.has_value()) {
        error_message = "duplicate " + std::string(option_name) + " option";
        return option_parse_result::error;
    }
    if (index + 1U >= arguments.size()) {
        error_message = "missing value after " + std::string(option_name);
        return option_parse_result::error;
    }

    std::uint32_t value = 0U;
    if (!parse_uint32(arguments[index + 1U], value)) {
        error_message = "invalid " + std::string(value_label) + " twips: " +
                        std::string(arguments[index + 1U]);
        return option_parse_result::error;
    }
    target = value;
    ++index;
    return option_parse_result::matched;
}

} // namespace

auto parse_table_position_spacing_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    table_position_options &options, std::string &error_message)
    -> option_parse_result {
    auto result = parse_table_position_twips_spacing_option(
        arguments, index, "--left-from-text", "left-from-text",
        options.position.left_from_text_twips, error_message);
    if (result != option_parse_result::not_matched) {
        return result;
    }

    result = parse_table_position_twips_spacing_option(
        arguments, index, "--right-from-text", "right-from-text",
        options.position.right_from_text_twips, error_message);
    if (result != option_parse_result::not_matched) {
        return result;
    }

    result = parse_table_position_twips_spacing_option(
        arguments, index, "--top-from-text", "top-from-text",
        options.position.top_from_text_twips, error_message);
    if (result != option_parse_result::not_matched) {
        return result;
    }

    result = parse_table_position_twips_spacing_option(
        arguments, index, "--bottom-from-text", "bottom-from-text",
        options.position.bottom_from_text_twips, error_message);
    if (result != option_parse_result::not_matched) {
        return result;
    }

    if (arguments[index] == "--overlap") {
        if (options.position.overlap.has_value()) {
            error_message = "duplicate --overlap option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --overlap";
            return option_parse_result::error;
        }
        featherdoc::table_overlap overlap{};
        if (!parse_table_overlap_text(arguments[index + 1U], overlap)) {
            error_message =
                "invalid table overlap: " + std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }
        options.position.overlap = overlap;
        ++index;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

} // namespace featherdoc_cli
