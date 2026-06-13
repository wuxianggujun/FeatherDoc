#include "featherdoc_cli_style_numbering_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_inspect_style_options_parse.hpp"
#include "featherdoc_cli_numbering_catalog_parse.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_numbering_audit.hpp"
#include "featherdoc_cli_style_numbering_output.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

auto run_audit_style_numbering_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command, "audit-style-numbering expects an input path",
                          json_output);
        return 2;
    }

    audit_style_numbering_options options;
    std::string error_message;
    if (!parse_audit_style_numbering_options(arguments, 2U, options,
                                             error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto styles = doc.list_styles();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "audit", error_info, options.json_output);
        return 1;
    }

    const auto definitions = doc.list_numbering_definitions();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "audit", error_info, options.json_output);
        return 1;
    }

    const auto result = audit_style_numbering(styles, definitions);
    inspect_style_numbering_audit(result, options.json_output);
    if (options.fail_on_issue && !style_numbering_audit_clean(result)) {
        return 1;
    }
    return 0;
}

auto run_repair_style_numbering_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command,
                          "repair-style-numbering expects an input path",
                          json_output);
        return 2;
    }

    repair_style_numbering_options options;
    std::string error_message;
    if (!parse_repair_style_numbering_options(arguments, 2U, options,
                                              error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    auto catalog = featherdoc::numbering_catalog{};
    if (options.catalog_path.has_value() &&
        !read_numbering_catalog_file(*options.catalog_path, catalog,
                                     error_message)) {
        print_parse_error(command, error_message, options.json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto styles = doc.list_styles();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "repair", error_info, options.json_output);
        return 1;
    }

    const auto definitions = doc.list_numbering_definitions();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "repair", error_info, options.json_output);
        return 1;
    }

    auto result = style_numbering_repair_result{};
    result.apply = options.apply;
    result.catalog_path = options.catalog_path;
    result.output_path = options.output_path;
    result.before = audit_style_numbering(styles, definitions);
    result.applyable_suggestions =
        collect_applyable_style_numbering_repair_suggestions(
            result.before.suggestions);

    if (options.apply) {
        if (options.catalog_path.has_value()) {
            const auto import_summary = doc.import_numbering_catalog(catalog);
            if (!static_cast<bool>(import_summary)) {
                report_document_error(command, "repair", doc.last_error(),
                                      options.json_output);
                return 1;
            }
            result.catalog_import = import_summary;

            const auto catalog_repaired_styles = doc.list_styles();
            if (const auto &error_info = doc.last_error(); error_info.code) {
                report_document_error(command, "repair", error_info,
                                      options.json_output);
                return 1;
            }
            const auto catalog_repaired_definitions =
                doc.list_numbering_definitions();
            if (const auto &error_info = doc.last_error(); error_info.code) {
                report_document_error(command, "repair", error_info,
                                      options.json_output);
                return 1;
            }
            const auto catalog_repaired_audit = audit_style_numbering(
                catalog_repaired_styles, catalog_repaired_definitions);
            result.applyable_suggestions =
                collect_applyable_style_numbering_repair_suggestions(
                    catalog_repaired_audit.suggestions);
        }

        for (const auto &suggestion : result.applyable_suggestions) {
            switch (suggestion.action) {
            case style_numbering_repair_action::clear_style_numbering:
                if (!doc.clear_paragraph_style_numbering(suggestion.style_id)) {
                    report_document_error(command, "repair", doc.last_error(),
                                          options.json_output);
                    return 1;
                }
                ++result.applied_count;
                break;
            case style_numbering_repair_action::align_with_based_on_numbering:
            case style_numbering_repair_action::relink_style_numbering:
                if (!suggestion.target_definition_id.has_value() ||
                    !suggestion.target_level.has_value()) {
                    break;
                }
                if (!doc.set_paragraph_style_numbering(
                        suggestion.style_id, *suggestion.target_definition_id,
                        *suggestion.target_level)) {
                    report_document_error(command, "repair", doc.last_error(),
                                          options.json_output);
                    return 1;
                }
                ++result.applied_count;
                break;
            case style_numbering_repair_action::add_numbering_level:
            case style_numbering_repair_action::rename_numbering_definition:
            case style_numbering_repair_action::manual_review:
                break;
            }
        }

        const auto repaired_styles = doc.list_styles();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "repair", error_info,
                                  options.json_output);
            return 1;
        }
        const auto repaired_definitions = doc.list_numbering_definitions();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "repair", error_info,
                                  options.json_output);
            return 1;
        }
        result.after = audit_style_numbering(repaired_styles,
                                             repaired_definitions);

        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }
    }

    inspect_style_numbering_repair(result, options.json_output);
    return 0;
}

} // namespace

auto is_style_numbering_command(std::string_view command) -> bool {
    return command == "audit-style-numbering" ||
           command == "repair-style-numbering";
}

auto run_style_numbering_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "audit-style-numbering") {
        return run_audit_style_numbering_command(command, arguments, doc);
    }
    if (command == "repair-style-numbering") {
        return run_repair_style_numbering_command(command, arguments, doc);
    }
    return 2;
}

} // namespace featherdoc_cli
