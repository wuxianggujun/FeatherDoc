#include "featherdoc_cli_paragraph_run_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_paragraph_run_options_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <featherdoc.hpp>

#include <cstdint>
#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

struct body_run_target {
    std::uint32_t paragraph_index = 0U;
    std::uint32_t run_index = 0U;
};

auto report_body_paragraph_run_failure(std::string_view command,
                                       std::string_view summary,
                                       std::string detail, bool json_output)
    -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

auto parse_body_paragraph_index(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t argument_index, std::uint32_t &paragraph_index,
    bool json_output) -> bool {
    if (parse_uint32(arguments[argument_index], paragraph_index)) {
        return true;
    }

    print_parse_error(command,
                      "invalid paragraph index: " +
                          std::string(arguments[argument_index]),
                      json_output);
    return false;
}

auto parse_body_run_index(std::string_view command,
                          const std::vector<std::string_view> &arguments,
                          std::size_t argument_index, std::uint32_t &run_index,
                          bool json_output) -> bool {
    if (parse_uint32(arguments[argument_index], run_index)) {
        return true;
    }

    print_parse_error(command,
                      "invalid run index: " +
                          std::string(arguments[argument_index]),
                      json_output);
    return false;
}

auto parse_body_run_target(std::string_view command,
                           const std::vector<std::string_view> &arguments,
                           std::string_view usage_message,
                           body_run_target &target, bool json_output) -> bool {
    if (arguments.size() < 4U) {
        print_parse_error(command, std::string(usage_message), json_output);
        return false;
    }

    if (!parse_body_paragraph_index(command, arguments, 2U,
                                    target.paragraph_index, json_output)) {
        return false;
    }

    return parse_body_run_index(command, arguments, 3U, target.run_index,
                                json_output);
}

auto resolve_body_paragraph_for_command(std::string_view command,
                                        featherdoc::Document &doc,
                                        std::uint32_t paragraph_index,
                                        featherdoc::Paragraph &paragraph,
                                        bool json_output) -> bool {
    paragraph = doc.paragraphs();
    for (std::uint32_t current_index = 0U;
         current_index < paragraph_index && paragraph.has_next();
         ++current_index) {
        paragraph.next();
    }

    if (paragraph.has_next()) {
        return true;
    }

    return report_body_paragraph_run_failure(
        command, "paragraph index is out of range",
        "paragraph index '" + std::to_string(paragraph_index) +
            "' is out of range",
        json_output);
}

auto resolve_body_run_for_command(std::string_view command,
                                  featherdoc::Document &doc,
                                  const body_run_target &target,
                                  featherdoc::Run &run, bool json_output)
    -> bool {
    featherdoc::Paragraph paragraph;
    if (!resolve_body_paragraph_for_command(command, doc,
                                            target.paragraph_index, paragraph,
                                            json_output)) {
        return false;
    }

    run = paragraph.runs();
    for (std::uint32_t current_index = 0U;
         current_index < target.run_index && run.has_next(); ++current_index) {
        run.next();
    }

    if (run.has_next()) {
        return true;
    }

    return report_body_paragraph_run_failure(
        command, "run index is out of range",
        "run index '" + std::to_string(target.run_index) +
            "' is out of range for paragraph index '" +
            std::to_string(target.paragraph_index) + "'",
        json_output);
}

} // namespace

auto run_set_paragraph_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "set-paragraph-style expects an input path, a paragraph index, and a style id",
            json_output);
        return 2;
    }

    std::uint32_t paragraph_index = 0U;
    if (!parse_body_paragraph_index(command, arguments, 2U, paragraph_index,
                                    json_output)) {
        return 2;
    }

    const auto style_id = arguments[3];
    set_paragraph_style_options options;
    std::string error_message;
    if (!parse_set_paragraph_style_options(arguments, 4U, options,
                                           error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Paragraph paragraph;
    if (!resolve_body_paragraph_for_command(command, doc, paragraph_index,
                                            paragraph, options.json_output)) {
        return 1;
    }

    if (!doc.set_paragraph_style(paragraph, style_id)) {
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
            [paragraph_index, style_id](std::ostream &stream) {
                stream << ",\"paragraph_index\":" << paragraph_index
                       << ",\"style_id\":";
                write_json_string(stream, style_id);
            });
    }

    return 0;
}

auto run_clear_paragraph_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "clear-paragraph-style expects an input path and a paragraph index",
            json_output);
        return 2;
    }

    std::uint32_t paragraph_index = 0U;
    if (!parse_body_paragraph_index(command, arguments, 2U, paragraph_index,
                                    json_output)) {
        return 2;
    }

    clear_paragraph_style_options options;
    std::string error_message;
    if (!parse_clear_paragraph_style_options(arguments, 3U, options,
                                             error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Paragraph paragraph;
    if (!resolve_body_paragraph_for_command(command, doc, paragraph_index,
                                            paragraph, options.json_output)) {
        return 1;
    }

    if (!doc.clear_paragraph_style(paragraph)) {
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
            [paragraph_index](std::ostream &stream) {
                stream << ",\"paragraph_index\":" << paragraph_index;
            });
    }

    return 0;
}

auto run_set_run_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    body_run_target target;
    if (!parse_body_run_target(
            command, arguments,
            "set-run-style expects an input path, a paragraph index, a run index, and a style id",
            target, json_output)) {
        return 2;
    }

    if (arguments.size() < 5U) {
        print_parse_error(
            command,
            "set-run-style expects an input path, a paragraph index, a run index, and a style id",
            json_output);
        return 2;
    }

    const auto style_id = arguments[4];
    set_run_style_options options;
    std::string error_message;
    if (!parse_set_run_style_options(arguments, 5U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Run run;
    if (!resolve_body_run_for_command(command, doc, target, run,
                                      options.json_output)) {
        return 1;
    }

    if (!doc.set_run_style(run, style_id)) {
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
            [target, style_id](std::ostream &stream) {
                stream << ",\"paragraph_index\":" << target.paragraph_index
                       << ",\"run_index\":" << target.run_index
                       << ",\"style_id\":";
                write_json_string(stream, style_id);
            });
    }

    return 0;
}

auto run_clear_run_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    body_run_target target;
    if (!parse_body_run_target(
            command, arguments,
            "clear-run-style expects an input path, a paragraph index, and a run index",
            target, json_output)) {
        return 2;
    }

    clear_run_style_options options;
    std::string error_message;
    if (!parse_clear_run_style_options(arguments, 4U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Run run;
    if (!resolve_body_run_for_command(command, doc, target, run,
                                      options.json_output)) {
        return 1;
    }

    if (!doc.clear_run_style(run)) {
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
            [target](std::ostream &stream) {
                stream << ",\"paragraph_index\":" << target.paragraph_index
                       << ",\"run_index\":" << target.run_index;
            });
    }

    return 0;
}

auto run_set_run_font_family_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    body_run_target target;
    if (!parse_body_run_target(
            command, arguments,
            "set-run-font-family expects an input path, a paragraph index, a run index, and a font family",
            target, json_output)) {
        return 2;
    }

    if (arguments.size() < 5U) {
        print_parse_error(
            command,
            "set-run-font-family expects an input path, a paragraph index, a run index, and a font family",
            json_output);
        return 2;
    }

    const auto font_family = arguments[4];
    set_run_font_family_options options;
    std::string error_message;
    if (!parse_set_run_font_family_options(arguments, 5U, options,
                                           error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Run run;
    if (!resolve_body_run_for_command(command, doc, target, run,
                                      options.json_output)) {
        return 1;
    }

    if (!run.set_font_family(font_family)) {
        report_body_paragraph_run_failure(command,
                                          "run font family must not be empty",
                                          "run font family must not be empty",
                                          options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [target, font_family](std::ostream &stream) {
                stream << ",\"paragraph_index\":" << target.paragraph_index
                       << ",\"run_index\":" << target.run_index
                       << ",\"font_family\":";
                write_json_string(stream, font_family);
            });
    }

    return 0;
}

auto run_clear_run_font_family_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    body_run_target target;
    if (!parse_body_run_target(
            command, arguments,
            "clear-run-font-family expects an input path, a paragraph index, and a run index",
            target, json_output)) {
        return 2;
    }

    clear_run_font_family_options options;
    std::string error_message;
    if (!parse_clear_run_font_family_options(arguments, 4U, options,
                                             error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Run run;
    if (!resolve_body_run_for_command(command, doc, target, run,
                                      options.json_output)) {
        return 1;
    }

    if (!run.clear_font_family()) {
        report_body_paragraph_run_failure(command,
                                          "target run handle is not valid",
                                          "target run handle is not valid",
                                          options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [target](std::ostream &stream) {
                stream << ",\"paragraph_index\":" << target.paragraph_index
                       << ",\"run_index\":" << target.run_index;
            });
    }

    return 0;
}

auto run_set_run_language_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    body_run_target target;
    if (!parse_body_run_target(
            command, arguments,
            "set-run-language expects an input path, a paragraph index, a run index, and a language",
            target, json_output)) {
        return 2;
    }

    if (arguments.size() < 5U) {
        print_parse_error(
            command,
            "set-run-language expects an input path, a paragraph index, a run index, and a language",
            json_output);
        return 2;
    }

    const auto language = arguments[4];
    set_run_language_options options;
    std::string error_message;
    if (!parse_set_run_language_options(arguments, 5U, options,
                                        error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Run run;
    if (!resolve_body_run_for_command(command, doc, target, run,
                                      options.json_output)) {
        return 1;
    }

    if (!run.set_language(language)) {
        report_body_paragraph_run_failure(command, "run language must not be empty",
                                          "run language must not be empty",
                                          options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [target, language](std::ostream &stream) {
                stream << ",\"paragraph_index\":" << target.paragraph_index
                       << ",\"run_index\":" << target.run_index
                       << ",\"language\":";
                write_json_string(stream, language);
            });
    }

    return 0;
}

auto run_clear_run_language_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    body_run_target target;
    if (!parse_body_run_target(
            command, arguments,
            "clear-run-language expects an input path, a paragraph index, and a run index",
            target, json_output)) {
        return 2;
    }

    clear_run_language_options options;
    std::string error_message;
    if (!parse_clear_run_language_options(arguments, 4U, options,
                                          error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Run run;
    if (!resolve_body_run_for_command(command, doc, target, run,
                                      options.json_output)) {
        return 1;
    }

    if (!run.clear_language()) {
        report_body_paragraph_run_failure(command,
                                          "target run handle is not valid",
                                          "target run handle is not valid",
                                          options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [target](std::ostream &stream) {
                stream << ",\"paragraph_index\":" << target.paragraph_index
                       << ",\"run_index\":" << target.run_index;
            });
    }

    return 0;
}

} // namespace featherdoc_cli
