#include "featherdoc_cli_style_refactor_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_options_parse.hpp"
#include "featherdoc_cli_style_output.hpp"
#include "featherdoc_cli_style_refactor_options_parse.hpp"
#include "featherdoc_cli_style_refactor_output.hpp"
#include "featherdoc_cli_style_refactor_plan.hpp"
#include "featherdoc_cli_style_refactor_plan_parse.hpp"
#include "featherdoc_cli_style_refactor_restore_output.hpp"
#include "featherdoc_cli_style_refactor_rollback_parse.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

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

auto run_apply_style_refactor_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command, "apply-style-refactor expects an input path",
                          json_output);
        return 2;
    }

    auto options = style_refactor_apply_options{};
    std::string error_message;
    if (!parse_style_refactor_apply_options(arguments, 2U, options,
                                            error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (options.plan_file_path.has_value() &&
        !read_style_refactor_plan_file(*options.plan_file_path,
                                       options.requests, error_message)) {
        print_parse_error(command, error_message, options.json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto result = doc.apply_style_refactor(options.requests);
    if (!result.has_value()) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!result->applied()) {
        inspect_style_refactor_apply_result(*result, options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.rollback_plan_path.has_value() &&
        !write_style_refactor_rollback_plan_file(*options.rollback_plan_path,
                                                 *result, error_message)) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::io_error);
        error_info.detail = std::move(error_message);
        report_operation_failure(
            command, "output",
            "failed to write style refactor rollback output", error_info,
            options.json_output);
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&result, &options](std::ostream &stream) {
                write_json_style_refactor_apply_result_fields(stream, *result);
                if (options.plan_file_path.has_value()) {
                    stream << ",\"plan_file\":";
                    write_json_string(stream,
                                      options.plan_file_path->string());
                }
                if (options.rollback_plan_path.has_value()) {
                    stream << ",\"rollback_plan_file\":";
                    write_json_string(stream,
                                      options.rollback_plan_path->string());
                }
            });
        return 0;
    }

    inspect_style_refactor_apply_result(*result, false);
    return 0;
}

auto run_restore_style_merge_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command, "restore-style-merge expects an input path",
                          json_output);
        return 2;
    }

    auto options = style_merge_restore_options{};
    std::string error_message;
    if (!parse_style_merge_restore_options(arguments, 2U, options,
                                           error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    auto rollback_entries =
        std::vector<featherdoc::style_refactor_rollback_entry>{};
    if (!read_style_refactor_rollback_file(*options.rollback_plan_path,
                                           options.entry_indexes,
                                           options.source_style_ids,
                                           options.target_style_ids,
                                           rollback_entries, error_message)) {
        print_parse_error(command, error_message, options.json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto result = options.dry_run
        ? doc.plan_style_refactor_restore(rollback_entries)
        : doc.restore_style_refactor(rollback_entries);
    if (!result.has_value()) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!result->restored()) {
        inspect_style_refactor_restore_result(*result, options.json_output,
                                              &options);
        return 1;
    }

    if (options.dry_run) {
        inspect_style_refactor_restore_result(*result, options.json_output,
                                              &options);
        return 0;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&result, &options](std::ostream &stream) {
                write_json_style_refactor_restore_result_fields(stream, *result);
                stream << ",\"rollback_plan_file\":";
                write_json_string(stream, options.rollback_plan_path->string());
                write_json_style_refactor_restore_selection(stream, options);
            });
        return 0;
    }

    inspect_style_refactor_restore_result(*result, false);
    return 0;
}

auto run_rename_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "rename-style expects an input path, an old style id, and a new style id",
            json_output);
        return 2;
    }

    const auto old_style_id = std::string(arguments[2]);
    const auto new_style_id = std::string(arguments[3]);
    rename_style_options options;
    std::string error_message;
    if (!parse_rename_style_options(arguments, 4U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!doc.rename_style(old_style_id, new_style_id)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    const auto style = doc.find_style(new_style_id);
    if (!style.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&old_style_id, &new_style_id, &style](std::ostream &stream) {
                stream << ",\"old_style_id\":";
                write_json_string(stream, old_style_id);
                stream << ",\"new_style_id\":";
                write_json_string(stream, new_style_id);
                stream << ",\"style\":";
                write_json_style_summary(stream, *style);
            });
        return 0;
    }

    inspect_style(*style, std::nullopt, false);
    return 0;
}

auto run_merge_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "merge-style expects an input path, a source style id, and a target style id",
            json_output);
        return 2;
    }

    const auto source_style_id = std::string(arguments[2]);
    const auto target_style_id = std::string(arguments[3]);
    merge_style_options options;
    std::string error_message;
    if (!parse_rename_style_options(arguments, 4U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!doc.merge_style(source_style_id, target_style_id)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    const auto style = doc.find_style(target_style_id);
    if (!style.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&source_style_id, &target_style_id, &style](
                std::ostream &stream) {
                stream << ",\"source_style_id\":";
                write_json_string(stream, source_style_id);
                stream << ",\"target_style_id\":";
                write_json_string(stream, target_style_id);
                stream << ",\"style\":";
                write_json_style_summary(stream, *style);
            });
        return 0;
    }

    inspect_style(*style, std::nullopt, false);
    return 0;
}

auto run_plan_prune_unused_styles_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command, "plan-prune-unused-styles expects an input path",
                          json_output);
        return 2;
    }

    for (std::size_t index = 2U; index < arguments.size(); ++index) {
        if (arguments[index] == "--json") {
            continue;
        }
        print_parse_error(command,
                          "unknown option: " + std::string(arguments[index]),
                          json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       json_output)) {
        return 1;
    }

    const auto plan = doc.plan_prune_unused_styles();
    if (!plan.has_value()) {
        report_document_error(command, "inspect", doc.last_error(), json_output);
        return 1;
    }

    if (json_output) {
        std::cout << "{\"command\":\"plan-prune-unused-styles\",\"ok\":true";
        write_json_style_prune_plan(std::cout, *plan);
        std::cout << "}\n";
        return 0;
    }

    print_style_prune_plan(*plan);
    return 0;
}

auto run_prune_unused_styles_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command, "prune-unused-styles expects an input path",
                          json_output);
        return 2;
    }

    prune_unused_styles_options options;
    std::string error_message;
    if (!parse_rename_style_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto summary = doc.prune_unused_styles();
    if (!summary.has_value()) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&summary](std::ostream &stream) {
                write_json_style_prune_summary(stream, *summary);
            });
        return 0;
    }

    print_style_prune_summary(*summary);
    return 0;
}

} // namespace

auto is_style_refactor_command(std::string_view command) -> bool {
    return command == "plan-style-refactor" ||
           command == "suggest-style-merges" ||
           command == "apply-style-refactor" ||
           command == "restore-style-merge" || command == "rename-style" ||
           command == "merge-style" ||
           command == "plan-prune-unused-styles" ||
           command == "prune-unused-styles";
}

auto run_style_refactor_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "plan-style-refactor") {
        return run_plan_style_refactor_command(command, arguments, doc);
    }
    if (command == "suggest-style-merges") {
        return run_suggest_style_merges_command(command, arguments, doc);
    }
    if (command == "apply-style-refactor") {
        return run_apply_style_refactor_command(command, arguments, doc);
    }
    if (command == "restore-style-merge") {
        return run_restore_style_merge_command(command, arguments, doc);
    }
    if (command == "rename-style") {
        return run_rename_style_command(command, arguments, doc);
    }
    if (command == "merge-style") {
        return run_merge_style_command(command, arguments, doc);
    }
    if (command == "plan-prune-unused-styles") {
        return run_plan_prune_unused_styles_command(command, arguments, doc);
    }
    if (command == "prune-unused-styles") {
        return run_prune_unused_styles_command(command, arguments, doc);
    }

    return 2;
}

} // namespace featherdoc_cli
