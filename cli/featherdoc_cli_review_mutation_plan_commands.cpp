#include "featherdoc_cli_review_mutation_plan_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_review_mutation_plan.hpp"
#include "featherdoc_cli_review_mutation_plan_parse.hpp"
#include "featherdoc_cli_review_output.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto is_review_mutation_plan_command(std::string_view command) -> bool {
    return command == "build-review-mutation-plan" ||
           command == "preview-review-mutation-plan" ||
           command == "apply-review-mutation-plan";
}

auto run_review_mutation_plan_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "build-review-mutation-plan") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "build-review-mutation-plan expects an input path",
                              json_output);
            return 2;
        }

        std::optional<path_type> request_file_path;
        std::optional<path_type> output_plan_path;
        for (std::size_t index = 2U; index < arguments.size(); ++index) {
            const auto argument = arguments[index];
            if (argument == "--request-file") {
                if (request_file_path.has_value()) {
                    print_parse_error(command, "duplicate --request-file option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command,
                                      "missing path after --request-file",
                                      json_output);
                    return 2;
                }
                request_file_path =
                    path_type(std::string(arguments[index + 1U]));
                ++index;
                continue;
            }
            if (argument == "--output-plan") {
                if (output_plan_path.has_value()) {
                    print_parse_error(command, "duplicate --output-plan option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command,
                                      "missing path after --output-plan",
                                      json_output);
                    return 2;
                }
                output_plan_path = path_type(std::string(arguments[index + 1U]));
                ++index;
                continue;
            }
            if (argument == "--json") {
                continue;
            }

            print_parse_error(command, "unknown option: " + std::string(argument),
                              json_output);
            return 2;
        }

        if (!request_file_path.has_value()) {
            print_parse_error(command, "missing --request-file <request.json>",
                              json_output);
            return 2;
        }

        std::vector<review_mutation_plan_build_request_operation> requests;
        std::string error_message;
        if (!read_review_mutation_plan_build_request_file(
                *request_file_path, requests, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        std::vector<review_mutation_plan_operation> operations;
        std::vector<review_mutation_plan_build_resolution> resolutions;
        std::size_t failed_operation_index = 0U;
        std::size_t failed_matches_count = 0U;
        std::size_t failed_raw_matches_count = 0U;
        if (!build_review_mutation_plan_operations(
                doc, requests, operations, resolutions, error_message,
                failed_operation_index, failed_matches_count,
                failed_raw_matches_count)) {
            if (json_output) {
                write_json_review_mutation_plan_build_failure(
                    std::cout, "resolve", error_message, failed_operation_index,
                    failed_matches_count, failed_raw_matches_count);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (output_plan_path.has_value() &&
            !write_review_mutation_plan_file(*output_plan_path, operations,
                                             error_message)) {
            if (json_output) {
                write_json_review_mutation_plan_build_failure(
                    std::cout, "write", error_message, 0U, 0U, 0U);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (json_output) {
            write_json_review_mutation_plan_build_result(
                std::cout, operations, resolutions, output_plan_path);
        } else if (output_plan_path.has_value()) {
            std::cout << "command: " << command << '\n'
                      << "output_plan_path: " << output_plan_path->string()
                      << '\n'
                      << "operations_count: " << operations.size() << '\n';
        } else {
            write_json_review_mutation_plan_document(std::cout, operations);
            std::cout << '\n';
        }
        return 0;
    }

    if (command == "preview-review-mutation-plan") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "preview-review-mutation-plan expects an input path",
                              json_output);
            return 2;
        }

        std::optional<path_type> plan_file_path;
        for (std::size_t index = 2U; index < arguments.size(); ++index) {
            const auto argument = arguments[index];
            if (argument == "--plan-file") {
                if (plan_file_path.has_value()) {
                    print_parse_error(command, "duplicate --plan-file option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command, "missing path after --plan-file",
                                      json_output);
                    return 2;
                }
                plan_file_path = path_type(std::string(arguments[index + 1U]));
                ++index;
                continue;
            }
            if (argument == "--json") {
                continue;
            }

            print_parse_error(command, "unknown option: " + std::string(argument),
                              json_output);
            return 2;
        }

        if (!plan_file_path.has_value()) {
            print_parse_error(command, "missing --plan-file <plan.json>",
                              json_output);
            return 2;
        }

        std::vector<review_mutation_plan_operation> operations;
        std::string error_message;
        if (!read_review_mutation_plan_file(*plan_file_path, operations,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto results =
            preview_review_mutation_plan_operations(doc, operations);
        const auto failed_count =
            static_cast<std::size_t>(std::count_if(
                results.begin(), results.end(),
                [](const auto &result) { return !result.ok; }));

        if (json_output) {
            write_json_review_mutation_plan_preview(std::cout, results);
        } else {
            print_review_mutation_plan_preview(std::cout, results);
        }
        return failed_count == 0U ? 0 : 1;
    }

    if (command == "apply-review-mutation-plan") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "apply-review-mutation-plan expects an input path",
                              json_output);
            return 2;
        }

        std::optional<path_type> plan_file_path;
        std::optional<path_type> output_path;
        for (std::size_t index = 2U; index < arguments.size(); ++index) {
            const auto argument = arguments[index];
            if (argument == "--plan-file") {
                if (plan_file_path.has_value()) {
                    print_parse_error(command, "duplicate --plan-file option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command, "missing path after --plan-file",
                                      json_output);
                    return 2;
                }
                plan_file_path = path_type(std::string(arguments[index + 1U]));
                ++index;
                continue;
            }
            if (argument == "--output") {
                if (output_path.has_value()) {
                    print_parse_error(command, "duplicate --output option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command, "missing path after --output",
                                      json_output);
                    return 2;
                }
                output_path = path_type(std::string(arguments[index + 1U]));
                ++index;
                continue;
            }
            if (argument == "--json") {
                continue;
            }

            print_parse_error(command, "unknown option: " + std::string(argument),
                              json_output);
            return 2;
        }

        if (!plan_file_path.has_value()) {
            print_parse_error(command, "missing --plan-file <plan.json>",
                              json_output);
            return 2;
        }

        std::vector<review_mutation_plan_operation> operations;
        std::string error_message;
        if (!read_review_mutation_plan_file(*plan_file_path, operations,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto preview_results =
            preview_review_mutation_plan_operations(doc, operations);
        const auto failed_count =
            static_cast<std::size_t>(std::count_if(
                preview_results.begin(), preview_results.end(),
                [](const auto &result) { return !result.ok; }));
        if (failed_count != 0U) {
            if (json_output) {
                write_json_review_mutation_plan_apply_failure(
                    std::cout, "preflight",
                    "review mutation plan preflight failed", preview_results);
            } else {
                std::cerr << "review mutation plan preflight failed\n";
                print_review_mutation_plan_preview(std::cerr, preview_results);
            }
            return 1;
        }

        if (find_review_mutation_plan_overlap(operations, error_message)) {
            if (json_output) {
                write_json_review_mutation_plan_apply_failure(
                    std::cout, "validate", error_message, preview_results);
            } else {
                std::cerr << error_message << '\n';
                print_review_mutation_plan_preview(std::cerr, preview_results);
            }
            return 1;
        }

        std::size_t applied_count = 0U;
        if (!apply_review_mutation_plan_operations(
                doc, operations, applied_count, error_message)) {
            if (json_output) {
                write_json_review_mutation_plan_apply_failure(
                    std::cout, "mutate", error_message, preview_results);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (!save_document(doc, output_path, command, json_output)) {
            return 1;
        }

        if (json_output) {
            write_json_review_mutation_plan_apply(
                std::cout, doc, output_path, preview_results, applied_count);
        } else {
            print_simple_document_mutation_result(command, output_path,
                                                  applied_count);
        }
        return 0;
    }



    return 2;
}

} // namespace featherdoc_cli
