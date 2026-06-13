#include "featherdoc_cli_style_refactor_options_parse.hpp"

#include "featherdoc_cli_style_refactor_pair_parse.hpp"

#include <filesystem>
#include <utility>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_style_refactor_plan_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    style_refactor_plan_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--rename" || argument == "--merge") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after " + std::string(argument);
                return false;
            }

            auto request = featherdoc::style_refactor_request{};
            const auto action = argument == "--rename"
                                    ? featherdoc::style_refactor_action::rename
                                    : featherdoc::style_refactor_action::merge;
            if (!parse_style_refactor_pair(arguments[index + 1U], action, request,
                                           error_message)) {
                return false;
            }
            options.requests.push_back(std::move(request));
            ++index;
            continue;
        }

        if (argument == "--output-plan") {
            if (options.output_plan_path.has_value()) {
                error_message = "duplicate --output-plan option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output-plan";
                return false;
            }

            options.output_plan_path = path_type(std::string(arguments[index + 1U]));
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

    if (options.requests.empty()) {
        error_message = "plan-style-refactor expects at least one --rename or --merge option";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
