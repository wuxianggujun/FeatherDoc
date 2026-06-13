#include "featherdoc_cli_style_refactor_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

#include <algorithm>
#include <filesystem>
#include <utility>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

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
