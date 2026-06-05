#pragma once

#include <featherdoc.hpp>

#include <iosfwd>
#include <string>
#include <string_view>

namespace featherdoc_cli {

void print_error_info(const featherdoc::document_error_info &error_info);
void print_parse_error(const std::string &message);
void print_parse_error(std::string_view command, const std::string &message,
                       bool json_output);
void write_json_command_error(
    std::ostream &stream, std::string_view command, std::string_view stage,
    std::string_view message,
    const featherdoc::document_error_info *error_info = nullptr);
auto report_operation_failure(
    std::string_view command, std::string_view stage,
    std::string_view fallback_message,
    const featherdoc::document_error_info &error_info, bool json_output) -> bool;
auto report_document_error(
    std::string_view command, std::string_view stage,
    const featherdoc::document_error_info &error_info, bool json_output) -> bool;

} // namespace featherdoc_cli
