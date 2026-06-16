#include "featherdoc_cli_document_content_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_document_mutation_options_parse.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_json_hyperlink_summary(
    std::ostream &stream, const featherdoc::hyperlink_summary &hyperlink) {
    stream << "{\"index\":" << hyperlink.index << ",\"text\":";
    write_json_string(stream, hyperlink.text);
    stream << ",\"relationship_id\":";
    write_json_optional_string(stream, hyperlink.relationship_id);
    stream << ",\"target\":";
    write_json_optional_string(stream, hyperlink.target);
    stream << ",\"anchor\":";
    write_json_optional_string(stream, hyperlink.anchor);
    stream << ",\"external\":" << json_bool(hyperlink.external) << '}';
}

void write_json_omml_summary(std::ostream &stream,
                             const featherdoc::omml_summary &formula) {
    stream << "{\"index\":" << formula.index
           << ",\"display\":" << json_bool(formula.display)
           << ",\"text\":";
    write_json_string(stream, formula.text);
    stream << ",\"xml\":";
    write_json_string(stream, formula.xml);
    stream << '}';
}

void print_hyperlink_summary(
    std::ostream &stream, const featherdoc::hyperlink_summary &hyperlink) {
    stream << "index=" << hyperlink.index << " text=";
    write_json_string(stream, hyperlink.text);
    stream << " relationship_id="
           << optional_display_value(hyperlink.relationship_id)
           << " target=" << optional_display_value(hyperlink.target)
           << " anchor=" << optional_display_value(hyperlink.anchor)
           << " external=" << yes_no(hyperlink.external);
}

void print_omml_summary(std::ostream &stream,
                        const featherdoc::omml_summary &formula) {
    stream << "index=" << formula.index
           << " display=" << yes_no(formula.display) << " text=";
    write_json_string(stream, formula.text);
}

void inspect_hyperlinks(
    const std::vector<featherdoc::hyperlink_summary> &hyperlinks,
    bool json_output) {
    if (json_output) {
        std::cout << "{\"count\":" << hyperlinks.size()
                  << ",\"hyperlinks\":[";
        for (std::size_t index = 0; index < hyperlinks.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_hyperlink_summary(std::cout, hyperlinks[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "hyperlinks: " << hyperlinks.size() << '\n';
    for (std::size_t index = 0; index < hyperlinks.size(); ++index) {
        std::cout << "hyperlink[" << index << "]: ";
        print_hyperlink_summary(std::cout, hyperlinks[index]);
        std::cout << '\n';
    }
}

void inspect_omml(const std::vector<featherdoc::omml_summary> &formulas,
                  bool json_output) {
    if (json_output) {
        std::cout << "{\"count\":" << formulas.size()
                  << ",\"formulas\":[";
        for (std::size_t index = 0; index < formulas.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_omml_summary(std::cout, formulas[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "formulas: " << formulas.size() << '\n';
    for (std::size_t index = 0; index < formulas.size(); ++index) {
        std::cout << "formula[" << index << "]: ";
        print_omml_summary(std::cout, formulas[index]);
        std::cout << '\n';
    }
}

void print_simple_document_mutation_result(
    std::string_view command, const std::optional<path_type> &output_path,
    std::size_t affected) {
    std::cout << "command: " << command << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "affected: " << affected << '\n';
}

void write_json_affected_result(std::ostream &stream, std::size_t affected) {
    stream << ",\"affected\":" << affected;
}

auto run_inspect_hyperlinks_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-hyperlinks expects an input path",
                          json_output);
        return 2;
    }
    if (arguments.size() > 3U ||
        (arguments.size() == 3U && arguments[2] != "--json")) {
        print_parse_error(command, "unknown option: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       json_output)) {
        return 1;
    }

    const auto hyperlinks = doc.list_hyperlinks();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, json_output);
        return 1;
    }

    inspect_hyperlinks(hyperlinks, json_output);
    return 0;
}

auto run_inspect_omml_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-omml expects an input path",
                          json_output);
        return 2;
    }
    if (arguments.size() > 3U ||
        (arguments.size() == 3U && arguments[2] != "--json")) {
        print_parse_error(command, "unknown option: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       json_output)) {
        return 1;
    }

    const auto formulas = doc.list_omml();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, json_output);
        return 1;
    }

    inspect_omml(formulas, json_output);
    return 0;
}

auto run_hyperlink_mutation_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    const bool append = command == "append-hyperlink";
    const bool remove = command == "remove-hyperlink";
    const std::size_t min_argument_count = append ? 2U : 3U;
    if (arguments.size() < min_argument_count) {
        print_parse_error(command,
                          append ? "append-hyperlink expects an input path"
                                 : std::string(command) +
                                       " expects an input path and hyperlink index",
                          json_output);
        return 2;
    }

    std::size_t hyperlink_index = 0U;
    if (!append && !parse_index(arguments[2], hyperlink_index)) {
        print_parse_error(command,
                          "invalid hyperlink index: " +
                              std::string(arguments[2]),
                          json_output);
        return 2;
    }

    hyperlink_mutation_options options;
    std::string error_message;
    if (!parse_hyperlink_mutation_options(
            arguments, append ? 2U : 3U, options, !remove, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    std::size_t affected = 0U;
    if (append) {
        affected = doc.append_hyperlink(options.text, options.target);
    } else if (remove) {
        affected = doc.remove_hyperlink(hyperlink_index) ? 1U : 0U;
    } else {
        affected = doc.replace_hyperlink(hyperlink_index, options.text,
                                         options.target)
                       ? 1U
                       : 0U;
    }
    if (affected == 0U) {
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
            [affected](std::ostream &stream) {
                write_json_affected_result(stream, affected);
            });
    } else {
        print_simple_document_mutation_result(command, options.output_path,
                                              affected);
    }
    return 0;
}

auto run_omml_mutation_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    const bool append = command == "append-omml";
    const bool remove = command == "remove-omml";
    const std::size_t min_argument_count = append ? 2U : 3U;
    if (arguments.size() < min_argument_count) {
        print_parse_error(command,
                          append ? "append-omml expects an input path"
                                 : std::string(command) +
                                       " expects an input path and formula index",
                          json_output);
        return 2;
    }

    std::size_t formula_index = 0U;
    if (!append && !parse_index(arguments[2], formula_index)) {
        print_parse_error(command,
                          "invalid formula index: " +
                              std::string(arguments[2]),
                          json_output);
        return 2;
    }

    omml_mutation_options options;
    std::string error_message;
    if (!parse_omml_mutation_options(arguments, append ? 2U : 3U,
                                     options, !remove, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    std::size_t affected = 0U;
    if (append) {
        affected = doc.append_omml(options.xml) ? 1U : 0U;
    } else if (remove) {
        affected = doc.remove_omml(formula_index) ? 1U : 0U;
    } else {
        affected = doc.replace_omml(formula_index, options.xml) ? 1U : 0U;
    }
    if (affected == 0U) {
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
            [affected](std::ostream &stream) {
                write_json_affected_result(stream, affected);
            });
    } else {
        print_simple_document_mutation_result(command, options.output_path,
                                              affected);
    }
    return 0;
}

} // namespace

auto is_document_content_command(std::string_view command) -> bool {
    return command == "inspect-hyperlinks" || command == "inspect-omml" ||
           command == "append-hyperlink" || command == "replace-hyperlink" ||
           command == "remove-hyperlink" || command == "append-omml" ||
           command == "replace-omml" || command == "remove-omml";
}

auto run_document_content_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "inspect-hyperlinks") {
        return run_inspect_hyperlinks_command(command, arguments, doc);
    }

    if (command == "inspect-omml") {
        return run_inspect_omml_command(command, arguments, doc);
    }

    if (command == "append-hyperlink" || command == "replace-hyperlink" ||
        command == "remove-hyperlink") {
        return run_hyperlink_mutation_command(command, arguments, doc);
    }

    if (command == "append-omml" || command == "replace-omml" ||
        command == "remove-omml") {
        return run_omml_mutation_command(command, arguments, doc);
    }

    return 2;
}

} // namespace featherdoc_cli
