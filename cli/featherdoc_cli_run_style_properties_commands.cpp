#include "featherdoc_cli_run_style_properties_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_run_properties_options_parse.hpp"
#include "featherdoc_cli_text.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace featherdoc_cli {
namespace {

struct run_properties_summary {
    std::optional<std::string> font_family;
    std::optional<std::string> east_asia_font_family;
    std::optional<std::string> language;
    std::optional<std::string> east_asia_language;
    std::optional<std::string> bidi_language;
    std::optional<bool> rtl;
    std::optional<bool> paragraph_bidi;
};

struct paragraph_style_properties_summary {
    std::optional<std::string> based_on;
    std::optional<std::string> next_style;
    std::optional<std::uint32_t> outline_level;
};

struct materialized_style_run_property_summary {
    std::string field;
    std::string source_style_id;
};

auto open_document_for_command(std::string_view command,
                               const std::vector<std::string_view> &arguments,
                               featherdoc::Document &doc,
                               bool json_output) -> bool {
    return open_document(path_type(std::string(arguments[1])), doc, command,
                         json_output);
}

void write_json_run_properties_summary(std::ostream &stream,
                                       const run_properties_summary &summary) {
    stream << "{\"font_family\":";
    write_json_optional_string(stream, summary.font_family);
    stream << ",\"east_asia_font_family\":";
    write_json_optional_string(stream, summary.east_asia_font_family);
    stream << ",\"language\":";
    write_json_optional_string(stream, summary.language);
    stream << ",\"east_asia_language\":";
    write_json_optional_string(stream, summary.east_asia_language);
    stream << ",\"bidi_language\":";
    write_json_optional_string(stream, summary.bidi_language);
    stream << ",\"rtl\":";
    write_json_optional_bool(stream, summary.rtl);
    stream << ",\"paragraph_bidi\":";
    write_json_optional_bool(stream, summary.paragraph_bidi);
    stream << '}';
}

void print_run_properties_summary(std::ostream &stream,
                                  const run_properties_summary &summary) {
    stream << "font_family: ";
    if (summary.font_family.has_value()) {
        stream << *summary.font_family;
    } else {
        stream << "none";
    }
    stream << '\n';

    stream << "east_asia_font_family: ";
    if (summary.east_asia_font_family.has_value()) {
        stream << *summary.east_asia_font_family;
    } else {
        stream << "none";
    }
    stream << '\n';

    stream << "language: ";
    if (summary.language.has_value()) {
        stream << *summary.language;
    } else {
        stream << "none";
    }
    stream << '\n';

    stream << "east_asia_language: ";
    if (summary.east_asia_language.has_value()) {
        stream << *summary.east_asia_language;
    } else {
        stream << "none";
    }
    stream << '\n';

    stream << "bidi_language: ";
    if (summary.bidi_language.has_value()) {
        stream << *summary.bidi_language;
    } else {
        stream << "none";
    }
    stream << '\n';

    stream << "rtl: ";
    if (summary.rtl.has_value()) {
        stream << yes_no(*summary.rtl);
    } else {
        stream << "none";
    }
    stream << '\n';

    stream << "paragraph_bidi: ";
    if (summary.paragraph_bidi.has_value()) {
        stream << yes_no(*summary.paragraph_bidi);
    } else {
        stream << "none";
    }
    stream << '\n';
}

void write_json_paragraph_style_properties_summary(
    std::ostream &stream, const paragraph_style_properties_summary &summary) {
    stream << "{\"based_on\":";
    write_json_optional_string(stream, summary.based_on);
    stream << ",\"next_style\":";
    write_json_optional_string(stream, summary.next_style);
    stream << ",\"outline_level\":";
    if (summary.outline_level.has_value()) {
        stream << *summary.outline_level;
    } else {
        stream << "null";
    }
    stream << '}';
}

void print_paragraph_style_properties_summary(
    std::ostream &stream, const paragraph_style_properties_summary &summary) {
    stream << "based_on: ";
    if (summary.based_on.has_value()) {
        stream << *summary.based_on;
    } else {
        stream << "none";
    }
    stream << '\n';

    stream << "next_style: ";
    if (summary.next_style.has_value()) {
        stream << *summary.next_style;
    } else {
        stream << "none";
    }
    stream << '\n';

    stream << "outline_level: ";
    if (summary.outline_level.has_value()) {
        stream << *summary.outline_level;
    } else {
        stream << "none";
    }
    stream << '\n';
}

void write_json_cleared_fields(std::ostream &stream,
                               const std::vector<std::string> &cleared_fields) {
    stream << '[';
    for (std::size_t index = 0U; index < cleared_fields.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_string(stream, cleared_fields[index]);
    }
    stream << ']';
}

void append_materialized_style_run_property(
    std::vector<materialized_style_run_property_summary> &materialized_properties,
    std::string_view current_style_id, std::string_view field_name,
    const featherdoc::resolved_style_string_property &property) {
    if (!property.value.has_value() || !property.source_style_id.has_value() ||
        *property.source_style_id == current_style_id) {
        return;
    }

    materialized_properties.push_back(
        {std::string(field_name), *property.source_style_id});
}

void append_materialized_style_run_property(
    std::vector<materialized_style_run_property_summary> &materialized_properties,
    std::string_view current_style_id, std::string_view field_name,
    const featherdoc::resolved_style_bool_property &property) {
    if (!property.value.has_value() || !property.source_style_id.has_value() ||
        *property.source_style_id == current_style_id) {
        return;
    }

    materialized_properties.push_back(
        {std::string(field_name), *property.source_style_id});
}

auto collect_materialized_style_run_properties(
    std::string_view style_id,
    const featherdoc::resolved_style_properties_summary &resolved)
    -> std::vector<materialized_style_run_property_summary> {
    auto properties = std::vector<materialized_style_run_property_summary>{};
    append_materialized_style_run_property(properties, style_id, "font_family",
                                          resolved.run_font_family);
    append_materialized_style_run_property(properties, style_id,
                                          "east_asia_font_family",
                                          resolved.run_east_asia_font_family);
    append_materialized_style_run_property(properties, style_id, "language",
                                          resolved.run_language);
    append_materialized_style_run_property(properties, style_id,
                                          "east_asia_language",
                                          resolved.run_east_asia_language);
    append_materialized_style_run_property(properties, style_id, "bidi_language",
                                          resolved.run_bidi_language);
    append_materialized_style_run_property(properties, style_id, "rtl",
                                          resolved.run_rtl);
    append_materialized_style_run_property(properties, style_id,
                                          "paragraph_bidi",
                                          resolved.paragraph_bidi);
    return properties;
}

void write_json_materialized_style_run_properties(
    std::ostream &stream,
    const std::vector<materialized_style_run_property_summary>
        &materialized_properties) {
    stream << '[';
    for (std::size_t index = 0U; index < materialized_properties.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }

        stream << "{\"field\":";
        write_json_string(stream, materialized_properties[index].field);
        stream << ",\"source_style_id\":";
        write_json_string(stream, materialized_properties[index].source_style_id);
        stream << '}';
    }
    stream << ']';
}

auto resolve_style_properties_for_command(
    std::string_view command, featherdoc::Document &doc,
    const std::string &style_id, bool json_output)
    -> std::optional<featherdoc::resolved_style_properties_summary> {
    const auto resolved = doc.resolve_style_properties(style_id);
    if (!resolved.has_value()) {
        report_document_error(command, "inspect", doc.last_error(), json_output);
    }
    return resolved;
}

enum class style_rebase_kind {
    character,
    paragraph,
};

auto run_rebase_style_based_on_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::string_view usage_message, style_rebase_kind kind) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(command, std::string(usage_message), json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    const auto based_on = std::string(arguments[3]);
    rebase_style_based_on_options options;
    std::string error_message;
    if (!parse_rebase_style_based_on_options(arguments, 4U, options,
                                             error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    const auto resolved =
        resolve_style_properties_for_command(command, doc, style_id,
                                             options.json_output);
    if (!resolved.has_value()) {
        return 1;
    }

    const auto preserved_properties =
        collect_materialized_style_run_properties(style_id, *resolved);

    const auto rebased =
        kind == style_rebase_kind::character
            ? doc.rebase_character_style_based_on(style_id, based_on)
            : doc.rebase_paragraph_style_based_on(style_id, based_on);
    if (!rebased) {
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
            [&style_id, &based_on, &preserved_properties](std::ostream &stream) {
                stream << ",\"style_id\":";
                write_json_string(stream, style_id);
                stream << ",\"based_on\":";
                write_json_string(stream, based_on);
                stream << ",\"preserved\":";
                write_json_materialized_style_run_properties(stream,
                                                             preserved_properties);
            });
    }

    return 0;
}

} // namespace

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

auto run_inspect_style_run_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "inspect-style-run-properties expects an input path and a style id",
            json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    inspect_style_run_properties_options options;
    std::string error_message;
    if (!parse_inspect_style_run_properties_options(arguments, 3U, options,
                                                    error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    run_properties_summary summary{};
    summary.font_family = doc.style_run_font_family(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.east_asia_font_family = doc.style_run_east_asia_font_family(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.language = doc.style_run_language(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.east_asia_language = doc.style_run_east_asia_language(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.bidi_language = doc.style_run_bidi_language(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.rtl = doc.style_run_rtl(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.paragraph_bidi = doc.style_paragraph_bidi(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    if (options.json_output) {
        std::cout << "{\"style_id\":";
        write_json_string(std::cout, style_id);
        std::cout << ",\"style_run_properties\":";
        write_json_run_properties_summary(std::cout, summary);
        std::cout << "}\n";
    } else {
        std::cout << "style_id: " << style_id << '\n';
        print_run_properties_summary(std::cout, summary);
    }

    return 0;
}

auto run_inspect_paragraph_style_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "inspect-paragraph-style-properties expects an input path and a style id",
            json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    inspect_paragraph_style_properties_options options;
    std::string error_message;
    if (!parse_inspect_paragraph_style_properties_options(arguments, 3U, options,
                                                          error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    paragraph_style_properties_summary summary{};
    const auto style = doc.find_style(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }
    if (!style.has_value()) {
        report_operation_failure(
            command, "inspect", "operation failed",
            featherdoc::document_error_info{
                std::make_error_code(std::errc::invalid_argument),
                "style id '" + style_id + "' was not found in word/styles.xml",
                "word/styles.xml", std::nullopt},
            options.json_output);
        return 1;
    }

    if (style->kind != featherdoc::style_kind::paragraph) {
        report_operation_failure(
            command, "inspect", "operation failed",
            featherdoc::document_error_info{
                std::make_error_code(std::errc::invalid_argument),
                "style id '" + style_id + "' is not a paragraph style",
                "word/styles.xml", std::nullopt},
            options.json_output);
        return 1;
    }
    summary.based_on = style->based_on;

    summary.next_style = doc.paragraph_style_next_style(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.outline_level = doc.paragraph_style_outline_level(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    if (options.json_output) {
        std::cout << "{\"style_id\":";
        write_json_string(std::cout, style_id);
        std::cout << ",\"paragraph_style_properties\":";
        write_json_paragraph_style_properties_summary(std::cout, summary);
        std::cout << "}\n";
    } else {
        std::cout << "style_id: " << style_id << '\n';
        print_paragraph_style_properties_summary(std::cout, summary);
    }

    return 0;
}

auto run_materialize_style_run_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "materialize-style-run-properties expects an input path and a style id",
            json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    materialize_style_run_properties_options options;
    std::string error_message;
    if (!parse_materialize_style_run_properties_options(arguments, 3U, options,
                                                        error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    const auto resolved =
        resolve_style_properties_for_command(command, doc, style_id,
                                             options.json_output);
    if (!resolved.has_value()) {
        return 1;
    }

    const auto materialized_properties =
        collect_materialized_style_run_properties(style_id, *resolved);

    if (!doc.materialize_style_run_properties(style_id)) {
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
            [&style_id, &materialized_properties](std::ostream &stream) {
                stream << ",\"style_id\":";
                write_json_string(stream, style_id);
                stream << ",\"materialized\":";
                write_json_materialized_style_run_properties(stream,
                                                             materialized_properties);
            });
    }

    return 0;
}

auto run_rebase_character_style_based_on_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    return run_rebase_style_based_on_command(
        command, arguments,
        "rebase-character-style-based-on expects an input path, a style id, and a target basedOn style id",
        style_rebase_kind::character);
}

auto run_rebase_paragraph_style_based_on_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    return run_rebase_style_based_on_command(
        command, arguments,
        "rebase-paragraph-style-based-on expects an input path, a style id, and a target basedOn style id",
        style_rebase_kind::paragraph);
}

auto run_set_paragraph_style_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "set-paragraph-style-properties expects an input path, a style id, and mutation options",
            json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    set_paragraph_style_properties_options options;
    std::string error_message;
    if (!parse_set_paragraph_style_properties_options(arguments, 3U, options,
                                                      error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    if (options.based_on.has_value() &&
        !doc.set_paragraph_style_based_on(style_id, *options.based_on)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.next_style.has_value() &&
        !doc.set_paragraph_style_next_style(style_id, *options.next_style)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.outline_level.has_value() &&
        !doc.set_paragraph_style_outline_level(style_id, *options.outline_level)) {
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
            [&style_id, &options](std::ostream &stream) {
                stream << ",\"style_id\":";
                write_json_string(stream, style_id);
                stream << ",\"based_on\":";
                write_json_optional_string(stream, options.based_on);
                stream << ",\"next_style\":";
                write_json_optional_string(stream, options.next_style);
                stream << ",\"outline_level\":";
                if (options.outline_level.has_value()) {
                    stream << *options.outline_level;
                } else {
                    stream << "null";
                }
            });
    }

    return 0;
}

auto run_clear_paragraph_style_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "clear-paragraph-style-properties expects an input path, a style id, and clear options",
            json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    clear_paragraph_style_properties_options options;
    std::string error_message;
    if (!parse_clear_paragraph_style_properties_options(arguments, 3U, options,
                                                        error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    std::vector<std::string> cleared_fields;
    if (options.clear_based_on) {
        if (!doc.clear_paragraph_style_based_on(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("based_on");
    }

    if (options.clear_next_style) {
        if (!doc.clear_paragraph_style_next_style(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("next_style");
    }

    if (options.clear_outline_level) {
        if (!doc.clear_paragraph_style_outline_level(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("outline_level");
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&style_id, &cleared_fields](std::ostream &stream) {
                stream << ",\"style_id\":";
                write_json_string(stream, style_id);
                stream << ",\"cleared\":";
                write_json_cleared_fields(stream, cleared_fields);
            });
    }

    return 0;
}

auto run_set_style_run_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "set-style-run-properties expects an input path, a style id, and mutation options",
            json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    set_style_run_properties_options options;
    std::string error_message;
    if (!parse_set_style_run_properties_options(arguments, 3U, options,
                                                error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    if (options.font_family.has_value() &&
        !doc.set_style_run_font_family(style_id, *options.font_family)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.east_asia_font_family.has_value() &&
        !doc.set_style_run_east_asia_font_family(style_id,
                                                 *options.east_asia_font_family)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.language.has_value() &&
        !doc.set_style_run_language(style_id, *options.language)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.east_asia_language.has_value() &&
        !doc.set_style_run_east_asia_language(style_id,
                                              *options.east_asia_language)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.bidi_language.has_value() &&
        !doc.set_style_run_bidi_language(style_id, *options.bidi_language)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.rtl.has_value() && !doc.set_style_run_rtl(style_id, *options.rtl)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.paragraph_bidi.has_value() &&
        !doc.set_style_paragraph_bidi(style_id, *options.paragraph_bidi)) {
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
            [&options, &style_id](std::ostream &stream) {
                stream << ",\"style_id\":";
                write_json_string(stream, style_id);
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

auto run_clear_style_run_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "clear-style-run-properties expects an input path, a style id, and clear options",
            json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    clear_style_run_properties_options options;
    std::string error_message;
    if (!parse_clear_style_run_properties_options(arguments, 3U, options,
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
        if (!doc.clear_style_run_font_family(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("font_family");
    }

    if (options.clear_east_asia_font_family) {
        if (!doc.clear_style_run_east_asia_font_family(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("east_asia_font_family");
    }

    if (options.clear_primary_language) {
        if (!doc.clear_style_run_primary_language(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("primary_language");
    }

    if (options.clear_language) {
        if (!doc.clear_style_run_language(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("language");
    }

    if (options.clear_east_asia_language) {
        if (!doc.clear_style_run_east_asia_language(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("east_asia_language");
    }

    if (options.clear_bidi_language) {
        if (!doc.clear_style_run_bidi_language(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("bidi_language");
    }

    if (options.clear_rtl) {
        if (!doc.clear_style_run_rtl(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("rtl");
    }

    if (options.clear_paragraph_bidi) {
        if (!doc.clear_style_paragraph_bidi(style_id)) {
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
            [&cleared_fields, &style_id](std::ostream &stream) {
                stream << ",\"style_id\":";
                write_json_string(stream, style_id);
                stream << ",\"cleared\":";
                write_json_cleared_fields(stream, cleared_fields);
            });
    }

    return 0;
}

} // namespace featherdoc_cli
