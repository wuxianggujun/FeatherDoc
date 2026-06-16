#include "featherdoc_cli_paragraph_run_commands_detail.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"

#include <featherdoc.hpp>

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

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

} // namespace

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

} // namespace featherdoc_cli
