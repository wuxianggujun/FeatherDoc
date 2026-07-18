#ifndef FEATHERDOC_PACKAGE_RELATIONSHIPS_MCE_HELPERS_HPP
#define FEATHERDOC_PACKAGE_RELATIONSHIPS_MCE_HELPERS_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

#include <pugixml.hpp>

#include <constants.hpp>

namespace featherdoc {
struct document_error_info;
}

namespace featherdoc::detail {

inline constexpr unsigned int package_relationships_xml_parse_options =
    pugi::parse_default | pugi::parse_ws_pcdata | pugi::parse_pi |
    pugi::parse_comments | pugi::parse_declaration;

struct package_relationships_mce_limits final {
    std::size_t maximum_elements{1'000'000U};
    std::size_t maximum_output_attributes{262'144U};
    std::size_t maximum_hoisted_bindings_per_element{4'096U};
    std::size_t maximum_namespace_work_bytes{64U * 1024U * 1024U};
};

enum class package_relationships_mce_status : std::uint8_t {
    success = 0U,
    invalid_namespace_markup,
    invalid_mce_markup,
    mismatch,
    resource_limit,
    allocation_failure,
};

struct package_relationships_mce_result final {
    package_relationships_mce_status status{
        package_relationships_mce_status::success};
    std::string detail;

    [[nodiscard]] explicit operator bool() const noexcept {
        return status == package_relationships_mce_status::success;
    }
};

// Applies ECMA-376 Part 3 markup-compatibility preprocessing using an
// application configuration that understands only the OPC Relationships
// namespace. The input document is replaced only after a complete sanitized
// document has been constructed and validated successfully.
[[nodiscard]] package_relationships_mce_result
preprocess_package_relationships_mce(pugi::xml_document &document);
[[nodiscard]] package_relationships_mce_result
preprocess_package_relationships_mce(
    pugi::xml_document &document,
    const package_relationships_mce_limits &limits);
[[nodiscard]] std::error_code package_relationships_mce_error_code(
    package_relationships_mce_status status) noexcept;
[[nodiscard]] std::error_code set_package_relationships_mce_last_error(
    featherdoc::document_error_info &error_info,
    const package_relationships_mce_result &result,
    std::string_view entry_name);

} // namespace featherdoc::detail

#endif
