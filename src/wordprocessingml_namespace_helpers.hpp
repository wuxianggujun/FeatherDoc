#ifndef FEATHERDOC_WORDPROCESSINGML_NAMESPACE_HELPERS_HPP
#define FEATHERDOC_WORDPROCESSINGML_NAMESPACE_HELPERS_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

#include <pugixml.hpp>

namespace featherdoc {
struct document_error_info;
}

namespace featherdoc::detail {

inline constexpr auto wordprocessingml_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/wordprocessingml/2006/main"};

enum class wordprocessingml_part_kind : std::uint8_t {
    main_document = 0U,
    header,
    footer,
    styles,
    numbering,
    settings,
    footnotes,
    endnotes,
    comments,
};

enum class wordprocessingml_namespace_status : std::uint8_t {
    success = 0U,
    invalid_namespace_markup,
    wrong_w_binding,
    duplicate_expanded_attribute,
    resource_limit,
    allocation_failure,
};

struct wordprocessingml_namespace_limits final {
    std::size_t maximum_depth{65'536U};
    std::size_t maximum_elements{1'000'000U};
    std::size_t maximum_attributes{1'000'000U};
};

struct wordprocessingml_namespace_result final {
    wordprocessingml_namespace_status status{
        wordprocessingml_namespace_status::success};
    bool changed{false};
    bool root_matches{false};
    std::string actual_root_namespace_uri;
    std::string actual_root_local_name;
    std::string detail;

    [[nodiscard]] explicit operator bool() const noexcept {
        return status == wordprocessingml_namespace_status::success;
    }
};

// Validates every QName by its expanded name and atomically canonicalizes
// names in the transitional WordprocessingML namespace to the internal w:
// prefix. Namespace declarations and attribute values are preserved because
// MCE and data-binding attributes can contain prefix-sensitive strings.
[[nodiscard]] wordprocessingml_namespace_result
canonicalize_wordprocessingml_part(
    pugi::xml_document &document, wordprocessingml_part_kind kind,
    const wordprocessingml_namespace_limits &limits = {});

[[nodiscard]] std::error_code wordprocessingml_namespace_error_code(
    wordprocessingml_namespace_status status) noexcept;
[[nodiscard]] std::error_code set_wordprocessingml_namespace_last_error(
    featherdoc::document_error_info &error_info,
    const wordprocessingml_namespace_result &result,
    std::string_view entry_name);
[[nodiscard]] std::error_code set_wordprocessingml_root_mismatch_last_error(
    featherdoc::document_error_info &error_info,
    const wordprocessingml_namespace_result &result,
    wordprocessingml_part_kind kind, std::string_view entry_name);

} // namespace featherdoc::detail

#endif
