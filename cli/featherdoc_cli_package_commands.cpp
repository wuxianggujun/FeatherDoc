#include "featherdoc_cli_package_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"

#include <featherdoc.hpp>
#include <featherdoc/detail/path.hpp>

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

auto diagnostic_code_name(featherdoc::package_diagnostic_code code)
    -> std::string_view {
    using code_type = featherdoc::package_diagnostic_code;
    switch (code) {
    case code_type::missing_document_body:
        return "missing_document_body";
    case code_type::invalid_document_root:
        return "invalid_document_root";
    case code_type::missing_root_relationships:
        return "missing_root_relationships";
    case code_type::malformed_root_relationships:
        return "malformed_root_relationships";
    case code_type::invalid_root_relationships_root:
        return "invalid_root_relationships_root";
    case code_type::missing_main_document_relationship:
        return "missing_main_document_relationship";
    case code_type::invalid_main_document_relationship:
        return "invalid_main_document_relationship";
    case code_type::ambiguous_main_document_relationship:
        return "ambiguous_main_document_relationship";
    case code_type::missing_content_types:
        return "missing_content_types";
    case code_type::malformed_content_types:
        return "malformed_content_types";
    case code_type::invalid_content_types_root:
        return "invalid_content_types_root";
    case code_type::missing_main_document_content_type:
        return "missing_main_document_content_type";
    case code_type::invalid_main_document_content_type:
        return "invalid_main_document_content_type";
    case code_type::ambiguous_main_document_content_type:
        return "ambiguous_main_document_content_type";
    case code_type::duplicate_content_type_override:
        return "duplicate_content_type_override";
    case code_type::invalid_content_type_part_name:
        return "invalid_content_type_part_name";
    case code_type::duplicate_content_type_default:
        return "duplicate_content_type_default";
    case code_type::invalid_content_type_extension:
        return "invalid_content_type_extension";
    case code_type::invalid_content_type_media_type:
        return "invalid_content_type_media_type";
    case code_type::invalid_relationships_part:
        return "invalid_relationships_part";
    case code_type::invalid_document_relationship:
        return "invalid_document_relationship";
    case code_type::duplicate_singleton_relationship:
        return "duplicate_singleton_relationship";
    case code_type::dangling_relationship:
        return "dangling_relationship";
    }
    return "unknown";
}

auto severity_name(featherdoc::package_diagnostic_severity severity)
    -> std::string_view {
    return severity == featherdoc::package_diagnostic_severity::warning
               ? "warning"
               : "error";
}

void write_json_diagnostic(std::ostream &stream,
                           const featherdoc::package_diagnostic &diagnostic) {
    stream << "{\"code\":";
    write_json_string(stream, diagnostic_code_name(diagnostic.code));
    stream << ",\"severity\":";
    write_json_string(stream, severity_name(diagnostic.severity));
    stream << ",\"entry\":";
    write_json_string(stream, diagnostic.entry_name);
    stream << ",\"detail\":";
    write_json_string(stream, diagnostic.detail);
    stream << ",\"repairable\":" << json_bool(diagnostic.repairable) << '}';
}

void write_json_diagnostics(
    std::ostream &stream,
    const std::vector<featherdoc::package_diagnostic> &diagnostics) {
    stream << '[';
    for (std::size_t index = 0U; index < diagnostics.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_diagnostic(stream, diagnostics[index]);
    }
    stream << ']';
}

auto open_tolerant(std::string_view input_path, std::string_view command,
                   bool json_output, featherdoc::Document &document) -> bool {
    document.set_path(featherdoc::detail::path_from_utf8(input_path));
    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    if (document.open(options)) {
        return report_document_error(command, "open", document.last_error(),
                                     json_output);
    }
    return true;
}

auto run_inspect_package(std::string_view command,
                         const std::vector<std::string_view> &arguments,
                         featherdoc::Document &document) -> int {
    const bool json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments.size() > (json_output ? 3U : 2U)) {
        print_parse_error(command,
                          "inspect-package expects <input.docx> [--json]",
                          json_output);
        return 2;
    }
    if (arguments.size() == 3U && arguments[2] != "--json") {
        print_parse_error(command,
                          "unknown option: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }
    if (!open_tolerant(arguments[1], command, json_output, document)) {
        return 1;
    }

    const auto &diagnostics = document.package_diagnostics();
    if (json_output) {
        std::cout << "{\"command\":";
        write_json_string(std::cout, command);
        std::cout << ",\"ok\":true,\"strict_valid\":"
                  << json_bool(diagnostics.empty()) << ",\"diagnostics\":";
        write_json_diagnostics(std::cout, diagnostics);
        std::cout << "}\n";
        return 0;
    }

    if (diagnostics.empty()) {
        std::cout << "package is strict-valid\n";
        return 0;
    }
    for (const auto &diagnostic : diagnostics) {
        std::cout << severity_name(diagnostic.severity) << ' '
                  << diagnostic_code_name(diagnostic.code)
                  << " [entry=" << diagnostic.entry_name
                  << "] repairable=" << (diagnostic.repairable ? "yes" : "no")
                  << ": " << diagnostic.detail << '\n';
    }
    return 0;
}

auto run_repair_package(std::string_view command,
                        const std::vector<std::string_view> &arguments,
                        featherdoc::Document &document) -> int {
    const bool json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(command,
                          "repair-package expects <input.docx> --output "
                          "<repaired.docx> [--json]",
                          json_output);
        return 2;
    }

    std::optional<std::string_view> output_value;
    bool json_option_seen = false;
    for (std::size_t index = 2U; index < arguments.size(); ++index) {
        if (arguments[index] == "--json") {
            if (json_option_seen) {
                print_parse_error(command, "duplicate --json option",
                                  json_output);
                return 2;
            }
            json_option_seen = true;
            continue;
        }
        if (arguments[index] == "--output") {
            if (output_value.has_value()) {
                print_parse_error(command, "duplicate --output option",
                                  json_output);
                return 2;
            }
            if (index + 1U >= arguments.size() ||
                arguments[index + 1U] == "--json" ||
                arguments[index + 1U] == "--output") {
                print_parse_error(command, "missing path after --output",
                                  json_output);
                return 2;
            }
            output_value = arguments[++index];
            continue;
        }
        print_parse_error(command,
                          "unknown option: " + std::string(arguments[index]),
                          json_output);
        return 2;
    }
    if (!output_value.has_value()) {
        print_parse_error(command, "repair-package requires --output <path>",
                          json_output);
        return 2;
    }

    const auto input_path = featherdoc::detail::path_from_utf8(arguments[1]);
    const auto output_path = featherdoc::detail::path_from_utf8(*output_value);
    std::error_code equivalent_error;
    if (std::filesystem::equivalent(input_path, output_path,
                                    equivalent_error)) {
        print_parse_error(command,
                          "repair-package output must differ from its input",
                          json_output);
        return 2;
    }

    if (!open_tolerant(arguments[1], command, json_output, document)) {
        return 1;
    }

    const auto report = document.repair_package();
    if (!report.has_value()) {
        report_document_error(command, "repair", document.last_error(),
                              json_output);
        return 1;
    }

    if (!save_document(document, output_path, command, json_output)) {
        return 1;
    }

    if (json_output) {
        std::cout << "{\"command\":";
        write_json_string(std::cout, command);
        std::cout << ",\"ok\":true,\"output\":";
        write_json_string(std::cout,
                          featherdoc::detail::path_to_utf8(output_path));
        std::cout << ",\"changed\":" << json_bool(report->changed())
                  << ",\"diagnostics_before\":";
        write_json_diagnostics(std::cout, report->diagnostics_before);
        std::cout << ",\"actions\":[";
        for (std::size_t index = 0U; index < report->actions.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            const auto &action = report->actions[index];
            std::cout << "{\"code\":";
            write_json_string(std::cout,
                              diagnostic_code_name(action.diagnostic_code));
            std::cout << ",\"entry\":";
            write_json_string(std::cout, action.entry_name);
            std::cout << ",\"detail\":";
            write_json_string(std::cout, action.detail);
            std::cout << '}';
        }
        std::cout << "]}\n";
    } else {
        std::cout << "repaired package written to "
                  << featherdoc::detail::path_to_utf8(output_path) << " ("
                  << report->actions.size() << " action(s))\n";
    }
    return 0;
}

} // namespace

auto is_package_command(std::string_view command) -> bool {
    return command == "inspect-package" || command == "repair-package";
}

auto run_package_command(std::string_view command,
                         const std::vector<std::string_view> &arguments,
                         featherdoc::Document &document) -> int {
    if (command == "inspect-package") {
        return run_inspect_package(command, arguments, document);
    }
    if (command == "repair-package") {
        return run_repair_package(command, arguments, document);
    }
    print_parse_error(command, "unsupported package command",
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
