#include "featherdoc_cli_errors.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_usage.hpp"

#include <iostream>
#include <string>
#include <system_error>

namespace featherdoc_cli {

void print_error_info(const featherdoc::document_error_info &error_info) {
    const auto &error = error_info.code;
    std::cerr << (error ? error.message() : std::string{"operation failed"});
    if (!error_info.detail.empty()) {
        std::cerr << ": " << error_info.detail;
    }
    if (!error_info.entry_name.empty()) {
        std::cerr << " [entry=" << error_info.entry_name << ']';
    }
    if (error_info.xml_offset.has_value()) {
        std::cerr << " [xml_offset=" << *error_info.xml_offset << ']';
    }
    std::cerr << '\n';
}

void print_parse_error(const std::string &message) {
    std::cerr << message << '\n';
    print_usage(std::cerr);
}

void write_json_command_error(
    std::ostream &stream, std::string_view command, std::string_view stage,
    std::string_view message,
    const featherdoc::document_error_info *error_info) {
    stream << "{\"command\":";
    write_json_string(stream, command);
    stream << ",\"ok\":false,\"stage\":";
    write_json_string(stream, stage);
    stream << ",\"message\":";
    write_json_string(stream, message);

    if (error_info != nullptr) {
        if (!error_info->detail.empty()) {
            stream << ",\"detail\":";
            write_json_string(stream, error_info->detail);
        }
        if (!error_info->entry_name.empty()) {
            stream << ",\"entry\":";
            write_json_string(stream, error_info->entry_name);
        }
        if (error_info->xml_offset.has_value()) {
            stream << ",\"xml_offset\":" << *error_info->xml_offset;
        }
    }

    stream << "}\n";
}

void print_parse_error(std::string_view command, const std::string &message,
                       bool json_output) {
    if (json_output) {
        write_json_command_error(std::cerr, command, "parse", message);
        return;
    }

    print_parse_error(message);
}

namespace {

auto portable_json_error_message(const std::error_code &code) -> std::string {
    if (code == std::make_error_code(std::errc::invalid_argument)) {
        return "invalid argument";
    }
    if (code == std::make_error_code(std::errc::result_out_of_range)) {
        return "result out of range";
    }
    if (code == std::make_error_code(std::errc::not_supported)) {
        return "Operation not supported";
    }
    return code.message();
}

} // namespace

auto report_operation_failure(std::string_view command, std::string_view stage,
                              std::string_view fallback_message,
                              const featherdoc::document_error_info &error_info,
                              bool json_output) -> bool {
    if (json_output) {
        const auto message = error_info.code
                                 ? portable_json_error_message(error_info.code)
                                 : std::string{fallback_message};
        write_json_command_error(std::cerr, command, stage, message, &error_info);
        return false;
    }

    if (error_info.code) {
        print_error_info(error_info);
        return false;
    }

    std::cerr << fallback_message << '\n';
    return false;
}

auto report_document_error(std::string_view command, std::string_view stage,
                           const featherdoc::document_error_info &error_info,
                           bool json_output) -> bool {
    return report_operation_failure(command, stage, "operation failed", error_info,
                                    json_output);
}

} // namespace featherdoc_cli
