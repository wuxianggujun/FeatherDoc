#include "featherdoc_cli_bookmark_text_options_parse_support.hpp"

#include "featherdoc_cli_template_inspect_options_parse.hpp"

#include <utility>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_bookmark_text_part_selection_option(
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

auto parse_bookmark_text_output_option(
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

    output_path = path_type(std::string(arguments[index + 1U]));
    ++index;
    return option_parse_result::matched;
}

auto parse_bookmark_text_binding_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::vector<bookmark_text_binding_source> &binding_sources,
    std::string &error_message) -> option_parse_result {
    const auto argument = arguments[index];
    if (argument != "--set" && argument != "--set-file") {
        return option_parse_result::not_matched;
    }
    if (index + 2U >= arguments.size()) {
        error_message = "missing value after " + std::string(argument);
        return option_parse_result::error;
    }

    const auto bookmark_name = std::string(arguments[index + 1U]);
    if (bookmark_name.empty()) {
        error_message = "bookmark name must not be empty";
        return option_parse_result::error;
    }

    bookmark_text_binding_source binding;
    binding.bookmark_name = bookmark_name;
    if (argument == "--set") {
        binding.source.text = std::string(arguments[index + 2U]);
    } else {
        binding.source.text_file = path_type(std::string(arguments[index + 2U]));
    }

    binding_sources.push_back(std::move(binding));
    index += 2U;
    return option_parse_result::matched;
}

} // namespace featherdoc_cli
