#pragma once

#include <featherdoc/document_core.hpp>

#include <new>
#include <string_view>
#include <system_error>

#include <pugixml.hpp>

namespace featherdoc::detail {

inline auto set_xml_parse_allocation_failure(
    document_error_info &error_info, std::string_view entry_name = {}) noexcept
    -> std::error_code {
    const auto code = std::make_error_code(std::errc::not_enough_memory);
    error_info.code = code;
    error_info.detail.clear();
    error_info.entry_name.clear();
    error_info.xml_offset.reset();

    // A pugixml allocator can fail independently from the process allocator.
    // Preserve useful, non-sensitive context when possible, but never let
    // formatting a low-memory diagnostic replace the original failure.
    try {
        error_info.detail.assign("XML parser ran out of memory");
        error_info.entry_name.assign(entry_name);
    } catch (...) {
        error_info.detail.clear();
        error_info.entry_name.clear();
    }
    return code;
}

inline auto set_xml_parse_failure(
    document_error_info &error_info, const pugi::xml_parse_result &parse_result,
    std::error_code parse_error, std::string_view entry_name) noexcept
    -> std::error_code {
    if (parse_result.status == pugi::status_out_of_memory) {
        return set_xml_parse_allocation_failure(error_info, entry_name);
    }

    try {
        error_info.code = parse_error;
        error_info.detail.assign(parse_result.description());
        error_info.entry_name.assign(entry_name);
        error_info.xml_offset =
            parse_result.offset >= 0
                ? std::optional<std::ptrdiff_t>{parse_result.offset}
                : std::nullopt;
        return parse_error;
    } catch (...) {
        return set_xml_parse_allocation_failure(error_info, entry_name);
    }
}

inline auto set_xml_parse_failure(
    document_error_info &error_info, const pugi::xml_parse_result &parse_result,
    document_errc parse_error, std::string_view entry_name) noexcept
    -> std::error_code {
    return set_xml_parse_failure(error_info, parse_result,
                                 make_error_code(parse_error), entry_name);
}

} // namespace featherdoc::detail
