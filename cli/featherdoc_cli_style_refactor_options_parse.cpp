#include "featherdoc_cli_style_refactor_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_refactor_plan.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

namespace featherdoc_cli {

namespace {

using path_type = std::filesystem::path;

auto parse_style_refactor_pair(
    std::string_view text, featherdoc::style_refactor_action action,
    featherdoc::style_refactor_request &request, std::string &error_message) -> bool {
    const auto separator = text.find(':');
    if (separator == std::string_view::npos) {
        error_message = action == featherdoc::style_refactor_action::rename
                            ? "invalid --rename value: expected <old-style-id>:<new-style-id>"
                            : "invalid --merge value: expected <source-style-id>:<target-style-id>";
        return false;
    }

    const auto source = text.substr(0U, separator);
    const auto target = text.substr(separator + 1U);
    if (source.empty()) {
        error_message = action == featherdoc::style_refactor_action::rename
                            ? "invalid --rename value: old style id must not be empty"
                            : "invalid --merge value: source style id must not be empty";
        return false;
    }
    if (target.empty()) {
        error_message = action == featherdoc::style_refactor_action::rename
                            ? "invalid --rename value: new style id must not be empty"
                            : "invalid --merge value: target style id must not be empty";
        return false;
    }

    request.action = action;
    request.source_style_id = std::string(source);
    request.target_style_id = std::string(target);
    return true;
}

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

auto parse_style_refactor_apply_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    style_refactor_apply_options &options, std::string &error_message) -> bool {
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

        if (argument == "--plan-file") {
            if (options.plan_file_path.has_value()) {
                error_message = "duplicate --plan-file option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --plan-file";
                return false;
            }

            options.plan_file_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--rollback-plan") {
            if (options.rollback_plan_path.has_value()) {
                error_message = "duplicate --rollback-plan option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --rollback-plan";
                return false;
            }

            options.rollback_plan_path = path_type(std::string(arguments[index + 1U]));
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

    if (options.plan_file_path.has_value() && !options.requests.empty()) {
        error_message =
            "apply-style-refactor cannot combine --plan-file with --rename or --merge";
        return false;
    }

    if (!options.plan_file_path.has_value() && options.requests.empty()) {
        error_message =
            "apply-style-refactor expects --plan-file or at least one --rename or --merge option";
        return false;
    }

    return true;
}

auto parse_style_merge_restore_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    style_merge_restore_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--rollback-plan") {
            if (options.rollback_plan_path.has_value()) {
                error_message = "duplicate --rollback-plan option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --rollback-plan";
                return false;
            }

            options.rollback_plan_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--entry") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing index after --entry";
                return false;
            }

            std::size_t entry_index = 0U;
            if (!parse_index(arguments[index + 1U], entry_index)) {
                error_message = "--entry expects an unsigned integer index";
                return false;
            }
            if (std::find(options.entry_indexes.begin(), options.entry_indexes.end(),
                          entry_index) != options.entry_indexes.end()) {
                error_message = "duplicate --entry index";
                return false;
            }
            options.entry_indexes.push_back(entry_index);
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

        if (argument == "--dry-run" || argument == "--plan-only") {
            options.dry_run = true;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (!options.rollback_plan_path.has_value()) {
        error_message = "restore-style-merge expects --rollback-plan";
        return false;
    }
    if (!options.entry_indexes.empty() &&
        (!options.source_style_ids.empty() || !options.target_style_ids.empty())) {
        error_message = "restore-style-merge --entry cannot be combined with "
                        "--source-style or --target-style";
        return false;
    }
    if (options.dry_run && options.output_path.has_value()) {
        error_message = "restore-style-merge --dry-run cannot be combined with --output";
        return false;
    }
    if (!options.dry_run && !options.output_path.has_value()) {
        error_message = "restore-style-merge expects --output unless --dry-run is used";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
