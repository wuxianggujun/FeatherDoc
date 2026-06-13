#include "featherdoc_cli_template_paragraph_inspect_commands_detail.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_template_paragraph_inspect_output.hpp"
#include "featherdoc_cli_template_part_selection.hpp"

#include <featherdoc.hpp>

#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace featherdoc_cli {

auto run_inspect_template_paragraphs_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command,
                          "inspect-template-paragraphs expects an input path",
                          json_output);
        return 2;
    }

    template_inspect_paragraphs_options options;
    std::string error_message;
    if (!parse_template_inspect_paragraphs_options(arguments, 2U, options,
                                                   error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    selected_template_part selected;
    if (!select_template_part(doc, options.part, options.part_index,
                              options.section_index, options.reference_kind,
                              selected, error_message)) {
        report_operation_failure(command, "inspect", error_message,
                                 doc.last_error(), options.json_output);
        return 1;
    }

    if (options.paragraph_index.has_value()) {
        const auto paragraph =
            selected.part.inspect_paragraph(*options.paragraph_index);
        if (!paragraph.has_value()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail =
                "paragraph index '" +
                std::to_string(*options.paragraph_index) +
                "' is out of range";
            error_info.entry_name = std::string(selected.part.entry_name());
            report_operation_failure(command, "inspect",
                                     "paragraph index is out of range",
                                     error_info, options.json_output);
            return 1;
        }

        inspect_template_paragraph(selected, *paragraph, options.json_output);
        return 0;
    }

    const auto paragraphs = selected.part.inspect_paragraphs();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info,
                              options.json_output);
        return 1;
    }

    inspect_template_paragraphs(selected, paragraphs, options.json_output);
    return 0;
}

} // namespace featherdoc_cli
