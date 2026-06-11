#include "featherdoc_cli_run_properties_options_parse.hpp"

#include "featherdoc_cli_run_properties_common_options_parse.hpp"
#include "featherdoc_cli_run_properties_mutation_options_parse.hpp"

namespace featherdoc_cli {

auto parse_inspect_default_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_default_run_properties_options &options, std::string &error_message)
    -> bool {
    return parse_json_only_options(arguments, start_index, options,
                                   error_message);
}

auto parse_set_default_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_default_run_properties_options &options, std::string &error_message)
    -> bool {
    return parse_set_run_properties_options(
        arguments, start_index, options, error_message,
        "set-default-run-properties requires at least one mutation option");
}

auto parse_clear_default_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_default_run_properties_options &options, std::string &error_message)
    -> bool {
    return parse_clear_run_properties_options(
        arguments, start_index, options, error_message,
        "clear-default-run-properties requires at least one clear option");
}

auto parse_inspect_style_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_style_run_properties_options &options, std::string &error_message)
    -> bool {
    return parse_json_only_options(arguments, start_index, options,
                                   error_message);
}

auto parse_materialize_style_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    materialize_style_run_properties_options &options,
    std::string &error_message) -> bool {
    return parse_output_json_options(arguments, start_index, options,
                                     error_message);
}

auto parse_rebase_style_based_on_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    rebase_style_based_on_options &options, std::string &error_message) -> bool {
    return parse_output_json_options(arguments, start_index, options,
                                     error_message);
}

auto parse_set_style_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_style_run_properties_options &options, std::string &error_message)
    -> bool {
    return parse_set_run_properties_options(
        arguments, start_index, options, error_message,
        "set-style-run-properties requires at least one mutation option");
}

auto parse_clear_style_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_style_run_properties_options &options, std::string &error_message)
    -> bool {
    return parse_clear_run_properties_options(
        arguments, start_index, options, error_message,
        "clear-style-run-properties requires at least one clear option");
}

} // namespace featherdoc_cli
