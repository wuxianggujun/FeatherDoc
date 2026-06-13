#include "featherdoc_cli_style_refactor_commands_detail.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_refactor_options_parse.hpp"
#include "featherdoc_cli_style_refactor_output.hpp"
#include "featherdoc_cli_style_refactor_plan_parse.hpp"

#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {

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

} // namespace featherdoc_cli
