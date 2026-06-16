#include "featherdoc_cli_style_inspect_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_inspect_style_options_parse.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_options_parse.hpp"
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

auto filter_numbered_paragraph_styles(
    const std::vector<featherdoc::style_summary> &styles)
    -> std::vector<featherdoc::style_summary> {
    auto filtered = std::vector<featherdoc::style_summary>{};
    for (const auto &style : styles) {
        if (style.kind == featherdoc::style_kind::paragraph &&
            style.numbering.has_value()) {
            filtered.push_back(style);
        }
    }
    return filtered;
}

void inspect_styles(const std::vector<featherdoc::style_summary> &styles,
                    bool json_output) {
    if (json_output) {
        std::cout << "{\"count\":" << styles.size() << ",\"styles\":[";
        for (std::size_t index = 0; index < styles.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_style_summary(std::cout, styles[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "styles: " << styles.size() << '\n';
    for (std::size_t index = 0; index < styles.size(); ++index) {
        std::cout << "style[" << index << "]: ";
        print_style_summary(std::cout, styles[index]);
        std::cout << '\n';
    }
}

void write_json_style_usage_report(
    std::ostream &stream, const featherdoc::style_usage_report &report) {
    stream << "{\"count\":" << report.style_count
           << ",\"used_style_count\":" << report.used_style_count
           << ",\"unused_style_count\":" << report.unused_style_count
           << ",\"total_reference_count\":" << report.total_reference_count
           << ",\"styles\":[";
    for (std::size_t index = 0; index < report.entries.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"style\":";
        write_json_style_summary(stream, report.entries[index].style);
        stream << ",\"usage\":";
        write_json_style_usage_summary(stream, report.entries[index].usage);
        stream << '}';
    }
    stream << "]}";
}

void inspect_style_usage_report(const featherdoc::style_usage_report &report,
                                bool json_output) {
    if (json_output) {
        write_json_style_usage_report(std::cout, report);
        std::cout << '\n';
        return;
    }

    std::cout << "styles: " << report.style_count << '\n'
              << "used_styles: " << report.used_style_count << '\n'
              << "unused_styles: " << report.unused_style_count << '\n'
              << "style_references: " << report.total_reference_count << '\n';
    for (std::size_t index = 0; index < report.entries.size(); ++index) {
        const auto &entry = report.entries[index];
        std::cout << "style[" << index << "]: ";
        print_style_summary(std::cout, entry.style);
        std::cout << " usage_total=" << entry.usage.total_count()
                  << " usage_body=" << entry.usage.body.total_count()
                  << " usage_header=" << entry.usage.header.total_count()
                  << " usage_footer=" << entry.usage.footer.total_count()
                  << '\n';
    }
}

void write_json_resolved_style_string_property(
    std::ostream &stream,
    const featherdoc::resolved_style_string_property &property) {
    stream << "{\"value\":";
    write_json_optional_string(stream, property.value);
    stream << ",\"source_style_id\":";
    write_json_optional_string(stream, property.source_style_id);
    stream << '}';
}

void write_json_resolved_style_bool_property(
    std::ostream &stream,
    const featherdoc::resolved_style_bool_property &property) {
    stream << "{\"value\":";
    write_json_optional_bool(stream, property.value);
    stream << ",\"source_style_id\":";
    write_json_optional_string(stream, property.source_style_id);
    stream << '}';
}

void write_json_resolved_style_properties_summary(
    std::ostream &stream,
    const featherdoc::resolved_style_properties_summary &summary) {
    stream << "{\"style_id\":";
    write_json_string(stream, summary.style_id);
    stream << ",\"type\":";
    write_json_string(stream, summary.type_name);
    stream << ",\"kind\":";
    write_json_string(stream, style_kind_name(summary.kind));
    stream << ",\"based_on\":";
    write_json_optional_string(stream, summary.based_on);
    stream << ",\"inheritance_chain\":";
    write_json_strings(stream, summary.inheritance_chain);
    stream << ",\"resolved_properties\":{\"font_family\":";
    write_json_resolved_style_string_property(stream, summary.run_font_family);
    stream << ",\"east_asia_font_family\":";
    write_json_resolved_style_string_property(
        stream, summary.run_east_asia_font_family);
    stream << ",\"language\":";
    write_json_resolved_style_string_property(stream, summary.run_language);
    stream << ",\"east_asia_language\":";
    write_json_resolved_style_string_property(
        stream, summary.run_east_asia_language);
    stream << ",\"bidi_language\":";
    write_json_resolved_style_string_property(stream, summary.run_bidi_language);
    stream << ",\"rtl\":";
    write_json_resolved_style_bool_property(stream, summary.run_rtl);
    stream << ",\"paragraph_bidi\":";
    write_json_resolved_style_bool_property(stream, summary.paragraph_bidi);
    stream << "}}";
}

void print_resolved_style_string_property(
    std::ostream &stream, std::string_view label,
    const featherdoc::resolved_style_string_property &property) {
    stream << label << ": ";
    if (property.value.has_value()) {
        stream << *property.value;
    } else {
        stream << "none";
    }
    stream << " (source=";
    if (property.source_style_id.has_value()) {
        stream << *property.source_style_id;
    } else {
        stream << "none";
    }
    stream << ")\n";
}

void print_resolved_style_bool_property(
    std::ostream &stream, std::string_view label,
    const featherdoc::resolved_style_bool_property &property) {
    stream << label << ": ";
    if (property.value.has_value()) {
        stream << yes_no(*property.value);
    } else {
        stream << "none";
    }
    stream << " (source=";
    if (property.source_style_id.has_value()) {
        stream << *property.source_style_id;
    } else {
        stream << "none";
    }
    stream << ")\n";
}

void inspect_resolved_style_properties(
    const featherdoc::resolved_style_properties_summary &summary,
    bool json_output) {
    if (json_output) {
        write_json_resolved_style_properties_summary(std::cout, summary);
        std::cout << '\n';
        return;
    }

    std::cout << "style_id: " << summary.style_id << '\n';
    std::cout << "type: " << summary.type_name << '\n';
    std::cout << "kind: " << style_kind_name(summary.kind) << '\n';
    std::cout << "based_on: ";
    if (summary.based_on.has_value()) {
        std::cout << *summary.based_on;
    } else {
        std::cout << "none";
    }
    std::cout << '\n';
    std::cout << "inheritance_chain: ";
    for (std::size_t index = 0U; index < summary.inheritance_chain.size();
         ++index) {
        if (index != 0U) {
            std::cout << " -> ";
        }
        std::cout << summary.inheritance_chain[index];
    }
    std::cout << '\n';
    print_resolved_style_string_property(std::cout, "font_family",
                                         summary.run_font_family);
    print_resolved_style_string_property(std::cout, "east_asia_font_family",
                                         summary.run_east_asia_font_family);
    print_resolved_style_string_property(std::cout, "language",
                                         summary.run_language);
    print_resolved_style_string_property(std::cout, "east_asia_language",
                                         summary.run_east_asia_language);
    print_resolved_style_string_property(std::cout, "bidi_language",
                                         summary.run_bidi_language);
    print_resolved_style_bool_property(std::cout, "rtl", summary.run_rtl);
    print_resolved_style_bool_property(std::cout, "paragraph_bidi",
                                       summary.paragraph_bidi);
}

auto run_inspect_styles_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-styles expects an input path",
                          json_output);
        return 2;
    }

    inspect_styles_options options;
    std::string error_message;
    if (!parse_inspect_styles_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (options.style_id.has_value()) {
        const auto style = doc.find_style(*options.style_id);
        if (!style.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        std::optional<featherdoc::style_usage_summary> usage;
        if (options.usage_output) {
            usage = doc.find_style_usage(*options.style_id);
            if (!usage.has_value()) {
                report_document_error(command, "inspect", doc.last_error(),
                                      options.json_output);
                return 1;
            }
        }

        inspect_style(*style, usage, options.json_output);
        return 0;
    }

    if (options.usage_output) {
        const auto report = doc.list_style_usage();
        if (!report.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        inspect_style_usage_report(*report, options.json_output);
        return 0;
    }

    const auto styles = doc.list_styles();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info,
                              options.json_output);
        return 1;
    }

    inspect_styles(styles, options.json_output);
    return 0;
}

auto run_inspect_style_inheritance_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "inspect-style-inheritance expects an input path and a style id",
            json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    inspect_style_inheritance_options options;
    std::string error_message;
    if (!parse_inspect_style_inheritance_options(arguments, 3U, options,
                                                 error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto resolved = doc.resolve_style_properties(style_id);
    if (!resolved.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }

    inspect_resolved_style_properties(*resolved, options.json_output);
    return 0;
}

auto run_inspect_style_numbering_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U || arguments[1].starts_with("--")) {
        print_parse_error(command,
                          "inspect-style-numbering expects an input path",
                          json_output);
        return 2;
    }

    inspect_options options;
    std::string error_message;
    if (!parse_inspect_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto styles = doc.list_styles();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info,
                              options.json_output);
        return 1;
    }

    inspect_styles(filter_numbered_paragraph_styles(styles),
                   options.json_output);
    return 0;
}

} // namespace

auto is_style_inspect_command(std::string_view command) -> bool {
    return command == "inspect-styles" ||
           command == "inspect-style-inheritance" ||
           command == "inspect-style-numbering";
}

auto run_style_inspect_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "inspect-styles") {
        return run_inspect_styles_command(command, arguments, doc);
    }
    if (command == "inspect-style-inheritance") {
        return run_inspect_style_inheritance_command(command, arguments, doc);
    }
    if (command == "inspect-style-numbering") {
        return run_inspect_style_numbering_command(command, arguments, doc);
    }

    print_parse_error(command,
                      "unsupported style inspect command: " +
                          std::string(command),
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
