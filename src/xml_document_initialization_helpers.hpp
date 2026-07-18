#pragma once

#include "package_relationships_xml_helpers.hpp"
#include "xml_parse_error_helpers.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

#include <pugixml.hpp>

namespace featherdoc::detail {

enum class fixed_xml_initialization_status : std::uint8_t {
    success = 0U,
    allocation_failure,
    parse_failure,
};

struct fixed_xml_initialization_result final {
    fixed_xml_initialization_status status{
        fixed_xml_initialization_status::parse_failure};
    std::optional<pugi::xml_parse_result> parse_result;

    [[nodiscard]] explicit operator bool() const noexcept {
        return this->status == fixed_xml_initialization_status::success;
    }
};

inline auto
initialize_fixed_xml_document(pugi::xml_document &xml_document,
                              std::string_view xml_text,
                              unsigned int parse_options = pugi::parse_default)
    -> fixed_xml_initialization_result {
    static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>,
                  "fixed XML initialization requires atomic move publish");

    pugi::xml_document initialized_document;
    const auto parse_result = initialized_document.load_buffer(
        xml_text.data(), xml_text.size(), parse_options);
    if (parse_result) {
        xml_document = std::move(initialized_document);
        return {fixed_xml_initialization_status::success,
                std::optional<pugi::xml_parse_result>{parse_result}};
    }

    // pugixml can leave nodes allocated before reporting a parse/allocation
    // failure. Keep those nodes confined to the local document so the target
    // remains byte-for-byte unchanged.
    return {parse_result.status == pugi::status_out_of_memory
                ? fixed_xml_initialization_status::allocation_failure
                : fixed_xml_initialization_status::parse_failure,
            std::optional<pugi::xml_parse_result>{parse_result}};
}

inline auto
initialize_empty_relationships_document(pugi::xml_document &xml_document)
    -> fixed_xml_initialization_result {
    constexpr auto empty_relationships_xml = std::string_view{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
</Relationships>
)"};

    const auto document_state =
        inspect_package_relationships_document(xml_document);
    if (document_state == package_relationships_document_state::valid) {
        return {fixed_xml_initialization_status::success, std::nullopt};
    }
    if (document_state == package_relationships_document_state::invalid) {
        // Preserve an invalid source tree so a failed mutation can still be
        // inspected or saved byte-for-byte by the tolerant workflow.
        return {fixed_xml_initialization_status::parse_failure, std::nullopt};
    }

    return initialize_fixed_xml_document(xml_document, empty_relationships_xml);
}

inline auto set_fixed_xml_initialization_failure(
    document_error_info &error_info,
    const fixed_xml_initialization_result &initialization_result,
    std::error_code parse_error, std::string_view entry_name) noexcept
    -> std::error_code {
    if (initialization_result) {
        return {};
    }
    if (initialization_result.status ==
        fixed_xml_initialization_status::allocation_failure) {
        return set_xml_parse_allocation_failure(error_info, entry_name);
    }
    if (initialization_result.parse_result.has_value()) {
        return set_xml_parse_failure(error_info,
                                     *initialization_result.parse_result,
                                     parse_error, entry_name);
    }

    error_info.code = parse_error;
    error_info.detail.clear();
    error_info.entry_name.clear();
    error_info.xml_offset.reset();
    try {
        error_info.detail.assign(
            "fixed Relationships XML initialization was refused for an "
            "invalid existing DOM");
        error_info.entry_name.assign(entry_name);
    } catch (...) {
        return set_xml_parse_allocation_failure(error_info, entry_name);
    }
    return parse_error;
}

inline auto set_fixed_xml_initialization_failure(
    document_error_info &error_info,
    const fixed_xml_initialization_result &initialization_result,
    document_errc parse_error, std::string_view entry_name) noexcept
    -> std::error_code {
    return set_fixed_xml_initialization_failure(
        error_info, initialization_result, make_error_code(parse_error),
        entry_name);
}

} // namespace featherdoc::detail
