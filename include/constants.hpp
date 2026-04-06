#ifndef FEATHERDOC_CONSTANTS_HPP
#define FEATHERDOC_CONSTANTS_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

namespace featherdoc {
enum class formatting_flag : std::uint32_t {
    none = 0U,
    bold = 1U << 0U,
    italic = 1U << 1U,
    underline = 1U << 2U,
    strikethrough = 1U << 3U,
    superscript = 1U << 4U,
    subscript = 1U << 5U,
    smallcaps = 1U << 6U,
    shadow = 1U << 7U,
};

[[nodiscard]] constexpr auto to_underlying(formatting_flag value) noexcept
    -> std::uint32_t {
    return static_cast<std::uint32_t>(value);
}

[[nodiscard]] constexpr auto operator|(formatting_flag lhs,
                                       formatting_flag rhs) noexcept
    -> formatting_flag {
    return static_cast<formatting_flag>(to_underlying(lhs) |
                                        to_underlying(rhs));
}

[[nodiscard]] constexpr auto operator&(formatting_flag lhs,
                                       formatting_flag rhs) noexcept
    -> formatting_flag {
    return static_cast<formatting_flag>(to_underlying(lhs) &
                                        to_underlying(rhs));
}

constexpr auto operator|=(formatting_flag &lhs, formatting_flag rhs) noexcept
    -> formatting_flag & {
    lhs = lhs | rhs;
    return lhs;
}

[[nodiscard]] constexpr auto has_flag(formatting_flag value,
                                      formatting_flag flag) noexcept -> bool {
    return to_underlying(value & flag) != 0U;
}

enum class section_reference_kind : std::uint8_t {
    default_reference = 0U,
    first_page,
    even_page,
};

[[nodiscard]] constexpr auto to_xml_reference_type(
    section_reference_kind kind) noexcept -> std::string_view {
    switch (kind) {
    case section_reference_kind::default_reference:
        return "default";
    case section_reference_kind::first_page:
        return "first";
    case section_reference_kind::even_page:
        return "even";
    }

    return "default";
}

enum class document_errc {
    success = 0,
    empty_path,
    document_not_open,
    archive_open_failed,
    relationships_xml_read_failed,
    relationships_xml_parse_failed,
    related_part_open_failed,
    related_part_read_failed,
    related_part_parse_failed,
    document_xml_open_failed,
    document_xml_read_failed,
    encrypted_document_unsupported,
    document_xml_parse_failed,
    content_types_xml_read_failed,
    content_types_xml_parse_failed,
    image_file_read_failed,
    image_format_unsupported,
    image_size_read_failed,
    settings_xml_read_failed,
    settings_xml_parse_failed,
    output_archive_open_failed,
    output_document_xml_open_failed,
    output_document_xml_write_failed,
    source_archive_open_failed,
    source_archive_entries_failed,
    source_entry_open_failed,
    source_entry_name_failed,
    output_entry_open_failed,
    output_entry_write_failed,
    source_entry_close_failed,
};

class document_error_category final : public std::error_category {
  public:
    [[nodiscard]] const char *name() const noexcept override;
    [[nodiscard]] std::string message(int condition) const override;
};

[[nodiscard]] auto document_category() noexcept -> const std::error_category &;
[[nodiscard]] auto make_error_code(document_errc error) noexcept
    -> std::error_code;
} // namespace featherdoc

namespace std {
template <> struct is_error_code_enum<featherdoc::document_errc> : true_type {};
} // namespace std

#endif
