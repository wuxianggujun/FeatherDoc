#include "featherdoc_cli_run_style_properties_commands.hpp"

#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_run_properties_options_parse.hpp"
#include "featherdoc_cli_run_style_properties_support.hpp"

#include <featherdoc.hpp>

#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_inspect_default_run_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command,
                          "inspect-default-run-properties expects an input path",
                          json_output);
        return 2;
    }

    inspect_default_run_properties_options options;
    std::string error_message;
    if (!parse_inspect_default_run_properties_options(arguments, 2U, options,
                                                      error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    run_properties_summary summary{};
    summary.font_family = doc.default_run_font_family();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.east_asia_font_family = doc.default_run_east_asia_font_family();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.language = doc.default_run_language();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.east_asia_language = doc.default_run_east_asia_language();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.bidi_language = doc.default_run_bidi_language();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.rtl = doc.default_run_rtl();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.paragraph_bidi = doc.default_paragraph_bidi();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    if (options.json_output) {
        std::cout << "{\"default_run_properties\":";
        write_json_run_properties_summary(std::cout, summary);
        std::cout << "}\n";
    } else {
        print_run_properties_summary(std::cout, summary);
    }

    return 0;
}

auto run_set_default_run_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(
            command,
            "set-default-run-properties expects an input path and mutation options",
            json_output);
        return 2;
    }

    set_default_run_properties_options options;
    std::string error_message;
    if (!parse_set_default_run_properties_options(arguments, 2U, options,
                                                  error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    if (options.font_family.has_value() &&
        !doc.set_default_run_font_family(*options.font_family)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.east_asia_font_family.has_value() &&
        !doc.set_default_run_east_asia_font_family(*options.east_asia_font_family)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.language.has_value() &&
        !doc.set_default_run_language(*options.language)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.east_asia_language.has_value() &&
        !doc.set_default_run_east_asia_language(*options.east_asia_language)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.bidi_language.has_value() &&
        !doc.set_default_run_bidi_language(*options.bidi_language)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.rtl.has_value() && !doc.set_default_run_rtl(*options.rtl)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.paragraph_bidi.has_value() &&
        !doc.set_default_paragraph_bidi(*options.paragraph_bidi)) {
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
            [&options](std::ostream &stream) {
                stream << ",\"font_family\":";
                write_json_optional_string(stream, options.font_family);
                stream << ",\"east_asia_font_family\":";
                write_json_optional_string(stream, options.east_asia_font_family);
                stream << ",\"language\":";
                write_json_optional_string(stream, options.language);
                stream << ",\"east_asia_language\":";
                write_json_optional_string(stream, options.east_asia_language);
                stream << ",\"bidi_language\":";
                write_json_optional_string(stream, options.bidi_language);
                stream << ",\"rtl\":";
                write_json_optional_bool(stream, options.rtl);
                stream << ",\"paragraph_bidi\":";
                write_json_optional_bool(stream, options.paragraph_bidi);
            });
    }

    return 0;
}

auto run_clear_default_run_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(
            command,
            "clear-default-run-properties expects an input path and clear options",
            json_output);
        return 2;
    }

    clear_default_run_properties_options options;
    std::string error_message;
    if (!parse_clear_default_run_properties_options(arguments, 2U, options,
                                                    error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    std::vector<std::string> cleared_fields;
    if (options.clear_font_family) {
        if (!doc.clear_default_run_font_family()) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("font_family");
    }

    if (options.clear_east_asia_font_family) {
        if (!doc.clear_default_run_east_asia_font_family()) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("east_asia_font_family");
    }

    if (options.clear_primary_language) {
        if (!doc.clear_default_run_primary_language()) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("primary_language");
    }

    if (options.clear_language) {
        if (!doc.clear_default_run_language()) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("language");
    }

    if (options.clear_east_asia_language) {
        if (!doc.clear_default_run_east_asia_language()) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("east_asia_language");
    }

    if (options.clear_bidi_language) {
        if (!doc.clear_default_run_bidi_language()) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("bidi_language");
    }

    if (options.clear_rtl) {
        if (!doc.clear_default_run_rtl()) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("rtl");
    }

    if (options.clear_paragraph_bidi) {
        if (!doc.clear_default_paragraph_bidi()) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("paragraph_bidi");
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&cleared_fields](std::ostream &stream) {
                stream << ",\"cleared\":";
                write_json_cleared_fields(stream, cleared_fields);
            });
    }

    return 0;
}

} // namespace featherdoc_cli
