#include "featherdoc_cli_content_control_options_parse_support.hpp"

#include "featherdoc_cli_template_inspect_options_parse.hpp"

#include <utility>

namespace featherdoc_cli {

auto parse_content_control_part_selection_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    validation_part_family &part, std::optional<std::size_t> &part_index,
    std::optional<std::size_t> &section_index,
    featherdoc::section_reference_kind &reference_kind, bool &has_part,
    bool &has_kind, bool reject_duplicate_part,
    std::string &error_message) -> option_parse_result {
    if (arguments[index] == "--part" && reject_duplicate_part && has_part) {
        error_message = "duplicate --part option";
        return option_parse_result::error;
    }

    return parse_template_part_selection_option(
        arguments, index, part, part_index, section_index, reference_kind,
        has_part, has_kind, error_message);
}

auto parse_content_control_selector_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::optional<std::string> &tag, std::optional<std::string> &alias,
    std::string &error_message) -> option_parse_result {
    const auto argument = arguments[index];
    if (argument != "--tag" && argument != "--alias") {
        return option_parse_result::not_matched;
    }

    auto &target = (argument == "--tag") ? tag : alias;
    if (target.has_value()) {
        error_message = "duplicate " + std::string(argument) + " option";
        return option_parse_result::error;
    }
    if (index + 1U >= arguments.size()) {
        error_message = "missing value after " + std::string(argument);
        return option_parse_result::error;
    }

    auto value = std::string(arguments[index + 1U]);
    if (value.empty()) {
        error_message = std::string(argument) + " expects a non-empty value";
        return option_parse_result::error;
    }

    target = std::move(value);
    ++index;
    return option_parse_result::matched;
}

auto parse_content_control_output_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::optional<std::filesystem::path> &output_path,
    std::string &error_message) -> option_parse_result {
    if (arguments[index] != "--output") {
        return option_parse_result::not_matched;
    }
    if (output_path.has_value()) {
        error_message = "duplicate --output option";
        return option_parse_result::error;
    }
    if (index + 1U >= arguments.size()) {
        error_message = "missing path after --output";
        return option_parse_result::error;
    }

    output_path = std::filesystem::path(std::string(arguments[index + 1U]));
    ++index;
    return option_parse_result::matched;
}

} // namespace featherdoc_cli
