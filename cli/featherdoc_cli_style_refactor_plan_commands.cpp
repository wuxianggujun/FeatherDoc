#include "featherdoc_cli_style_refactor_commands_detail.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_refactor_options_parse.hpp"
#include "featherdoc_cli_style_refactor_output.hpp"
#include "featherdoc_cli_style_refactor_plan.hpp"

#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {

auto run_plan_style_refactor_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command, "plan-style-refactor expects an input path",
                          json_output);
        return 2;
    }

    auto options = style_refactor_plan_options{};
    std::string error_message;
    if (!parse_style_refactor_plan_options(arguments, 2U, options,
                                           error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto plan = doc.plan_style_refactor(options.requests);
    if (!plan.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.output_plan_path.has_value() &&
        !write_style_refactor_plan_file(*options.output_plan_path, *plan,
                                        error_message)) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::io_error);
        error_info.detail = std::move(error_message);
        report_operation_failure(command, "output",
                                 "failed to write style refactor plan output",
                                 error_info, options.json_output);
        return 1;
    }

    inspect_style_refactor_plan(*plan, options.json_output);
    return plan->clean() ? 0 : 1;
}

auto run_suggest_style_merges_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command, "suggest-style-merges expects an input path",
                          json_output);
        return 2;
    }

    auto options = style_merge_suggestion_options{};
    std::string error_message;
    if (!parse_style_merge_suggestion_options(arguments, 2U, options,
                                              error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    auto plan = doc.suggest_style_merges();
    if (!plan.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }
    plan = filter_style_refactor_plan_by_style_ids(
        std::move(*plan), options.source_style_ids, options.target_style_ids);

    auto min_confidence = options.min_confidence;
    if (!min_confidence.has_value() && options.confidence_profile.has_value()) {
        if (*options.confidence_profile == "recommended") {
            min_confidence =
                plan->suggestion_confidence_summary().recommended_min_confidence;
        } else {
            min_confidence =
                style_merge_suggestion_confidence_profile_min_confidence(
                    *options.confidence_profile);
        }
    }
    if (min_confidence.has_value()) {
        plan = filter_style_refactor_plan_by_min_confidence(std::move(*plan),
                                                            *min_confidence);
    }

    if (options.output_plan_path.has_value() &&
        !write_style_refactor_plan_file(*options.output_plan_path, *plan,
                                        error_message, command)) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::io_error);
        error_info.detail = std::move(error_message);
        report_operation_failure(command, "output",
                                 "failed to write style merge suggestions",
                                 error_info, options.json_output);
        return 1;
    }

    inspect_style_refactor_plan(*plan, options.json_output, command,
                                options.fail_on_suggestion);
    const auto suggestion_gate_failed =
        options.fail_on_suggestion && plan->operation_count != 0U;
    return plan->clean() && !suggestion_gate_failed ? 0 : 1;
}

} // namespace featherdoc_cli
