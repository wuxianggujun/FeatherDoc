#include "featherdoc_cli_style_refactor_commands_detail.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_refactor_options_parse.hpp"
#include "featherdoc_cli_style_refactor_restore_output.hpp"
#include "featherdoc_cli_style_refactor_rollback_parse.hpp"

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

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

} // namespace featherdoc_cli
