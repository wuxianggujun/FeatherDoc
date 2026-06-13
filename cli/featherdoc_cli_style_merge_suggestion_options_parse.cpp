#include "featherdoc_cli_style_refactor_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_refactor_plan.hpp"

#include <algorithm>
#include <filesystem>
#include <utility>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_style_merge_suggestion_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    style_merge_suggestion_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
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

        if (argument == "--min-confidence") {
            if (options.min_confidence.has_value()) {
                error_message = "duplicate --min-confidence option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --min-confidence";
                return false;
            }
            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value) || value > 100U) {
                error_message = "--min-confidence expects an integer from 0 to 100";
                return false;
            }
            options.min_confidence = value;
            ++index;
            continue;
        }

        if (argument == "--confidence-profile") {
            if (options.confidence_profile.has_value()) {
                error_message = "duplicate --confidence-profile option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --confidence-profile";
                return false;
            }
            const auto profile = std::string(arguments[index + 1U]);
            if (!style_merge_suggestion_confidence_profile_is_valid(profile)) {
                error_message =
                    "--confidence-profile expects recommended, strict, review, or exploratory";
                return false;
            }
            options.confidence_profile = profile;
            ++index;
            continue;
        }

        if (argument == "--source-style") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing style id after --source-style";
                return false;
            }
            auto style_id = std::string(arguments[index + 1U]);
            if (style_id.empty()) {
                error_message = "--source-style expects a non-empty style id";
                return false;
            }
            if (std::find(options.source_style_ids.begin(),
                          options.source_style_ids.end(), style_id) !=
                options.source_style_ids.end()) {
                error_message = "duplicate --source-style id";
                return false;
            }
            options.source_style_ids.push_back(std::move(style_id));
            ++index;
            continue;
        }

        if (argument == "--target-style") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing style id after --target-style";
                return false;
            }
            auto style_id = std::string(arguments[index + 1U]);
            if (style_id.empty()) {
                error_message = "--target-style expects a non-empty style id";
                return false;
            }
            if (std::find(options.target_style_ids.begin(),
                          options.target_style_ids.end(), style_id) !=
                options.target_style_ids.end()) {
                error_message = "duplicate --target-style id";
                return false;
            }
            options.target_style_ids.push_back(std::move(style_id));
            ++index;
            continue;
        }

        if (argument == "--fail-on-suggestion") {
            options.fail_on_suggestion = true;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (options.min_confidence.has_value() &&
        options.confidence_profile.has_value()) {
        error_message =
            "--confidence-profile cannot be combined with --min-confidence";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
