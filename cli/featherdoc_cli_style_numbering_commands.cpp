#include "featherdoc_cli_style_numbering_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_inspect_style_options_parse.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_numbering_catalog_parse.hpp"
#include "featherdoc_cli_numbering_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_numbering_audit.hpp"
#include "featherdoc_cli_style_output.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_json_style_numbering_audit_issue(
    std::ostream &stream, const style_numbering_audit_issue &issue) {
    stream << "{\"kind\":";
    write_json_string(stream, style_numbering_audit_issue_kind_name(issue.kind));
    stream << ",\"style_id\":";
    write_json_string(stream, issue.style_id);
    stream << ",\"style_name\":";
    write_json_string(stream, issue.style_name);
    stream << ",\"based_on\":";
    write_json_optional_string(stream, issue.based_on);
    stream << ",\"num_id\":";
    write_json_optional_u32(stream, issue.num_id);
    stream << ",\"level\":";
    write_json_optional_u32(stream, issue.level);
    stream << ",\"definition_id\":";
    write_json_optional_u32(stream, issue.definition_id);
    stream << ",\"definition_name\":";
    write_json_optional_string(stream, issue.definition_name);
    stream << ",\"message\":";
    write_json_string(stream, issue.message);
    stream << '}';
}

void write_json_style_numbering_repair_suggestion(
    std::ostream &stream,
    const style_numbering_repair_suggestion &suggestion) {
    stream << "{\"action\":";
    write_json_string(stream,
                      style_numbering_repair_action_name(suggestion.action));
    stream << ",\"issue_kind\":";
    write_json_string(
        stream, style_numbering_audit_issue_kind_name(suggestion.issue_kind));
    stream << ",\"style_id\":";
    write_json_string(stream, suggestion.style_id);
    stream << ",\"target_definition_id\":";
    write_json_optional_u32(stream, suggestion.target_definition_id);
    stream << ",\"target_definition_name\":";
    write_json_optional_string(stream, suggestion.target_definition_name);
    stream << ",\"target_level\":";
    write_json_optional_u32(stream, suggestion.target_level);
    stream << ",\"rationale\":";
    write_json_string(stream, suggestion.rationale);
    stream << ",\"command_template\":";
    write_json_string(stream, suggestion.command_template);
    stream << ",\"applyable\":"
           << json_bool(style_numbering_repair_suggestion_applyable(
                  suggestion));
    stream << '}';
}

void print_style_numbering_repair_suggestion(
    std::ostream &stream,
    const style_numbering_repair_suggestion &suggestion) {
    stream << "action=" << style_numbering_repair_action_name(suggestion.action)
           << " issue_kind="
           << style_numbering_audit_issue_kind_name(suggestion.issue_kind)
           << " style_id=" << suggestion.style_id
           << " target_definition_id=";
    if (suggestion.target_definition_id.has_value()) {
        stream << *suggestion.target_definition_id;
    } else {
        stream << "none";
    }
    stream << " target_definition_name=";
    if (suggestion.target_definition_name.has_value()) {
        stream << *suggestion.target_definition_name;
    } else {
        stream << "none";
    }
    stream << " target_level=";
    if (suggestion.target_level.has_value()) {
        stream << *suggestion.target_level;
    } else {
        stream << "none";
    }
    stream << " applyable="
           << yes_no(style_numbering_repair_suggestion_applyable(
                  suggestion))
           << " command=" << suggestion.command_template
           << " rationale=" << suggestion.rationale;
}

void print_style_numbering_audit_issue(
    std::ostream &stream, const style_numbering_audit_issue &issue) {
    stream << "kind=" << style_numbering_audit_issue_kind_name(issue.kind)
           << " style_id=" << issue.style_id
           << " num_id=";
    if (issue.num_id.has_value()) {
        stream << *issue.num_id;
    } else {
        stream << "none";
    }
    stream << " level=";
    if (issue.level.has_value()) {
        stream << *issue.level;
    } else {
        stream << "none";
    }
    stream << " definition_id=";
    if (issue.definition_id.has_value()) {
        stream << *issue.definition_id;
    } else {
        stream << "none";
    }
    stream << " definition_name=";
    if (issue.definition_name.has_value()) {
        stream << *issue.definition_name;
    } else {
        stream << "none";
    }
    stream << " message=" << issue.message;
}

void inspect_style_numbering_audit(
    const style_numbering_audit_result &result, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"audit-style-numbering\",\"clean\":"
                  << json_bool(style_numbering_audit_clean(result))
                  << ",\"paragraph_style_count\":"
                  << result.paragraph_style_count
                  << ",\"numbered_style_count\":"
                  << result.numbered_styles.size()
                  << ",\"issue_count\":" << result.issues.size()
                  << ",\"suggestion_count\":" << result.suggestions.size()
                  << ",\"styles\":[";
        for (std::size_t index = 0; index < result.numbered_styles.size();
             ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_style_summary(std::cout, result.numbered_styles[index]);
        }
        std::cout << "],\"issues\":[";
        for (std::size_t index = 0; index < result.issues.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_style_numbering_audit_issue(std::cout,
                                                   result.issues[index]);
        }
        std::cout << "],\"suggestions\":[";
        for (std::size_t index = 0; index < result.suggestions.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_style_numbering_repair_suggestion(
                std::cout, result.suggestions[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "clean: " << yes_no(style_numbering_audit_clean(result)) << '\n'
              << "paragraph_styles: " << result.paragraph_style_count << '\n'
              << "numbered_styles: " << result.numbered_styles.size() << '\n'
              << "issues: " << result.issues.size() << '\n'
              << "suggestions: " << result.suggestions.size() << '\n';
    for (std::size_t index = 0; index < result.numbered_styles.size(); ++index) {
        std::cout << "style[" << index << "]: ";
        print_style_summary(std::cout, result.numbered_styles[index]);
        std::cout << '\n';
    }
    for (std::size_t index = 0; index < result.issues.size(); ++index) {
        std::cout << "issue[" << index << "]: ";
        print_style_numbering_audit_issue(std::cout, result.issues[index]);
        std::cout << '\n';
    }
    for (std::size_t index = 0; index < result.suggestions.size(); ++index) {
        std::cout << "suggestion[" << index << "]: ";
        print_style_numbering_repair_suggestion(std::cout,
                                                result.suggestions[index]);
        std::cout << '\n';
    }
}

auto style_numbering_repair_skipped_suggestion_count(
    const style_numbering_repair_result &result) -> std::size_t {
    if (result.before.suggestions.size() < result.applyable_suggestions.size()) {
        return 0U;
    }
    return result.before.suggestions.size() - result.applyable_suggestions.size();
}

void inspect_style_numbering_repair(
    const style_numbering_repair_result &result, bool json_output) {
    const auto mode = result.apply ? std::string_view{"apply"}
                                   : std::string_view{"plan"};
    if (json_output) {
        std::cout << "{\"command\":\"repair-style-numbering\",\"mode\":";
        write_json_string(std::cout, mode);
        std::cout << ",\"ok\":true"
                  << ",\"before_clean\":"
                  << json_bool(style_numbering_audit_clean(result.before))
                  << ",\"before_issue_count\":" << result.before.issues.size()
                  << ",\"after_clean\":";
        if (result.after.has_value()) {
            std::cout << json_bool(style_numbering_audit_clean(*result.after));
        } else {
            std::cout << "null";
        }
        std::cout << ",\"after_issue_count\":";
        if (result.after.has_value()) {
            std::cout << result.after->issues.size();
        } else {
            std::cout << "null";
        }
        std::cout << ",\"suggestion_count\":"
                  << result.before.suggestions.size()
                  << ",\"applyable_count\":"
                  << result.applyable_suggestions.size()
                  << ",\"skipped_suggestion_count\":"
                  << style_numbering_repair_skipped_suggestion_count(result)
                  << ",\"applied_count\":" << result.applied_count
                  << ",\"catalog_file\":";
        if (result.catalog_path.has_value()) {
            write_json_string(std::cout, result.catalog_path->string());
        } else {
            std::cout << "null";
        }
        std::cout << ",\"catalog_import\":";
        if (result.catalog_import.has_value()) {
            std::cout << '{';
            write_json_numbering_catalog_import_summary(std::cout,
                                                        *result.catalog_import);
            std::cout << '}';
        } else {
            std::cout << "null";
        }
        std::cout << ",\"output_path\":";
        if (result.output_path.has_value()) {
            write_json_string(std::cout, result.output_path->string());
        } else {
            std::cout << "null";
        }
        std::cout << ",\"suggestions\":[";
        for (std::size_t index = 0; index < result.before.suggestions.size();
             ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_style_numbering_repair_suggestion(
                std::cout, result.before.suggestions[index]);
        }
        std::cout << "],\"applyable_suggestions\":[";
        for (std::size_t index = 0;
             index < result.applyable_suggestions.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_style_numbering_repair_suggestion(
                std::cout, result.applyable_suggestions[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "mode: " << mode << '\n'
              << "before_clean: "
              << yes_no(style_numbering_audit_clean(result.before)) << '\n'
              << "before_issues: " << result.before.issues.size() << '\n'
              << "suggestions: " << result.before.suggestions.size() << '\n'
              << "applyable_suggestions: "
              << result.applyable_suggestions.size() << '\n'
              << "skipped_suggestions: "
              << style_numbering_repair_skipped_suggestion_count(result) << '\n'
              << "applied: " << result.applied_count << '\n';
    if (result.catalog_path.has_value()) {
        std::cout << "catalog_file: " << result.catalog_path->string() << '\n';
    }
    if (result.catalog_import.has_value()) {
        std::cout << "catalog_imported_definitions: "
                  << result.catalog_import->imported_definition_count << '\n'
                  << "catalog_imported_instances: "
                  << result.catalog_import->imported_instance_count << '\n';
    }
    if (result.output_path.has_value()) {
        std::cout << "output_path: " << result.output_path->string() << '\n';
    }
    if (result.after.has_value()) {
        std::cout << "after_clean: "
                  << yes_no(style_numbering_audit_clean(*result.after)) << '\n'
                  << "after_issues: " << result.after->issues.size() << '\n';
    }
    for (std::size_t index = 0; index < result.before.suggestions.size();
         ++index) {
        std::cout << "suggestion[" << index << "]: ";
        print_style_numbering_repair_suggestion(
            std::cout, result.before.suggestions[index]);
        std::cout << '\n';
    }
}

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
