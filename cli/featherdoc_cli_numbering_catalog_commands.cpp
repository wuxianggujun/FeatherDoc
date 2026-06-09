#include "featherdoc_cli_numbering_catalog_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_numbering_catalog_diff.hpp"
#include "featherdoc_cli_numbering_catalog_lint.hpp"
#include "featherdoc_cli_numbering_catalog_options_parse.hpp"
#include "featherdoc_cli_numbering_catalog_output.hpp"
#include "featherdoc_cli_numbering_catalog_parse.hpp"
#include "featherdoc_cli_numbering_catalog_patch_apply.hpp"
#include "featherdoc_cli_numbering_catalog_patch_parse.hpp"
#include "featherdoc_cli_numbering_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_schema_options_parse.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {

auto is_numbering_catalog_command(std::string_view command) -> bool {
    return command == "export-numbering-catalog" ||
           command == "import-numbering-catalog" ||
           command == "patch-numbering-catalog" ||
           command == "lint-numbering-catalog" ||
           command == "diff-numbering-catalog" ||
           command == "check-numbering-catalog";
}

auto run_numbering_catalog_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "export-numbering-catalog") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "export-numbering-catalog expects an input path",
                              json_output);
            return 2;
        }

        export_numbering_catalog_options options;
        std::string error_message;
        if (!parse_export_numbering_catalog_options(arguments, 2U, options,
                                                    error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto catalog = doc.export_numbering_catalog();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "export", error_info,
                                  options.json_output);
            return 1;
        }

        if (!options.output_path.has_value()) {
            write_json_numbering_catalog(std::cout, catalog);
            std::cout << '\n';
            return 0;
        }

        if (!write_numbering_catalog_file(*options.output_path, catalog,
                                          error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write numbering catalog output",
                                     error_info, options.json_output);
            return 1;
        }

        print_exported_numbering_catalog_summary(catalog, options.output_path,
                                                 options.json_output);
        return 0;
    }

    if (command == "import-numbering-catalog") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(
                command,
                "import-numbering-catalog expects an input path and --catalog-file <catalog.json>",
                json_output);
            return 2;
        }

        import_numbering_catalog_options options;
        std::string error_message;
        if (!parse_import_numbering_catalog_options(arguments, 2U, options,
                                                    error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        featherdoc::numbering_catalog catalog;
        if (!read_numbering_catalog_file(*options.catalog_path, catalog,
                                         error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto summary = doc.import_numbering_catalog(catalog);
        if (!static_cast<bool>(summary)) {
            report_document_error(command, "import", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&summary, &options](std::ostream &stream) {
                    stream << ",\"catalog_file\":";
                    write_json_string(stream, options.catalog_path->string());
                    stream << ',';
                    write_json_numbering_catalog_import_summary(stream, summary);
                });
        }

        return 0;
    }

    if (command == "patch-numbering-catalog") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command,
                              "patch-numbering-catalog expects a catalog path and "
                              "--patch-file <patch.json>",
                              json_output);
            return 2;
        }

        patch_numbering_catalog_options options;
        std::string error_message;
        if (!parse_patch_numbering_catalog_options(arguments, 2U, options,
                                                   error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        featherdoc::numbering_catalog catalog;
        if (!read_numbering_catalog_file(path_type(std::string(arguments[1])),
                                         catalog, error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        numbering_catalog_patch_document patch;
        if (!read_numbering_catalog_patch_file(*options.patch_path, patch,
                                               error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        numbering_catalog_patch_summary summary;
        if (!apply_numbering_catalog_patch(catalog, patch, summary,
                                           error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "patch",
                                     "failed to patch numbering catalog",
                                     error_info, options.json_output);
            return 1;
        }

        if (!options.output_path.has_value()) {
            write_json_numbering_catalog(std::cout, catalog);
            std::cout << '\n';
            return 0;
        }

        if (!write_numbering_catalog_file(*options.output_path, catalog,
                                          error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write patched numbering catalog output",
                                     error_info, options.json_output);
            return 1;
        }

        print_patched_numbering_catalog_summary(catalog, summary,
                                                options.output_path,
                                                options.json_output);
        return 0;
    }

    if (command == "lint-numbering-catalog") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command,
                              "lint-numbering-catalog expects a catalog path",
                              json_output);
            return 2;
        }

        lint_template_schema_options options;
        std::string error_message;
        if (!parse_lint_template_schema_options(arguments, 2U, options,
                                                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        featherdoc::numbering_catalog catalog;
        if (!read_numbering_catalog_file(path_type(std::string(arguments[1])),
                                         catalog, error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        const auto lint = lint_numbering_catalog(catalog);
        print_linted_numbering_catalog_result(lint, options.json_output);
        if (!lint.clean()) {
            return 1;
        }
        return 0;
    }

    if (command == "diff-numbering-catalog") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U || arguments[1].starts_with("--") ||
            arguments[2].starts_with("--")) {
            print_parse_error(
                command,
                "diff-numbering-catalog expects left and right catalog paths",
                json_output);
            return 2;
        }

        diff_template_schema_options options;
        std::string error_message;
        if (!parse_diff_template_schema_options(arguments, 3U, options,
                                                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        featherdoc::numbering_catalog left;
        if (!read_numbering_catalog_file(path_type(std::string(arguments[1])), left,
                                         error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        featherdoc::numbering_catalog right;
        if (!read_numbering_catalog_file(path_type(std::string(arguments[2])), right,
                                         error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        const auto result = diff_numbering_catalogs(left, right);
        print_numbering_catalog_diff_result(result, options.json_output);
        if (options.fail_on_diff && !result.equal()) {
            return 1;
        }
        return 0;
    }

    if (command == "check-numbering-catalog") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(
                command,
                "check-numbering-catalog expects an input path and --catalog-file <catalog.json>",
                json_output);
            return 2;
        }

        check_numbering_catalog_options options;
        std::string error_message;
        if (!parse_check_numbering_catalog_options(arguments, 2U, options,
                                                   error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        featherdoc::numbering_catalog baseline;
        if (!read_numbering_catalog_file(*options.catalog_path, baseline,
                                         error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }
        const auto baseline_lint = lint_numbering_catalog(baseline);

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto generated = doc.export_numbering_catalog();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "export", error_info,
                                  options.json_output);
            return 1;
        }
        const auto generated_lint = lint_numbering_catalog(generated);

        if (options.output_path.has_value() &&
            !write_numbering_catalog_file(*options.output_path, generated,
                                          error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(
                command, "output",
                "failed to write generated numbering catalog output",
                error_info, options.json_output);
            return 1;
        }

        const auto result = diff_numbering_catalogs(baseline, generated);
        print_checked_numbering_catalog_result(
            *options.catalog_path, baseline_lint, generated_lint, result,
            options.output_path, options.json_output);
        if (!baseline_lint.clean() || !generated_lint.clean() ||
            !result.equal()) {
            return 1;
        }
        return 0;
    }
    print_parse_error(command, "unknown numbering catalog command",
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
