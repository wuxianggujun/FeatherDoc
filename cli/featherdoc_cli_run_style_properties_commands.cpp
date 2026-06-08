#include "featherdoc_cli_run_style_properties_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_run_properties_options_parse.hpp"
#include "featherdoc_cli_run_style_properties_support.hpp"
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
