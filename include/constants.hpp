#ifndef FEATHERDOC_CONSTANTS_HPP
#define FEATHERDOC_CONSTANTS_HPP

#include <cstdint>
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

enum class document_errc {
    success = 0,
    empty_path,
    document_not_open,
    archive_open_failed,
    document_xml_open_failed,
    document_xml_read_failed,
    document_xml_parse_failed,
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
    [[nodiscard]] const char *name() const noexcept override {
        return "featherdoc.document";
    }

    [[nodiscard]] std::string message(int condition) const override {
        switch (static_cast<document_errc>(condition)) {
        case document_errc::success:
            return "success";
        case document_errc::empty_path:
            return "document path is empty";
        case document_errc::document_not_open:
            return "document is not open";
        case document_errc::archive_open_failed:
            return "failed to open document archive";
        case document_errc::document_xml_open_failed:
            return "failed to open word/document.xml";
        case document_errc::document_xml_read_failed:
            return "failed to read word/document.xml";
        case document_errc::document_xml_parse_failed:
            return "failed to parse word/document.xml";
        case document_errc::output_archive_open_failed:
            return "failed to create output archive";
        case document_errc::output_document_xml_open_failed:
            return "failed to create output word/document.xml entry";
        case document_errc::output_document_xml_write_failed:
            return "failed to write output word/document.xml";
        case document_errc::source_archive_open_failed:
            return "failed to reopen source archive";
        case document_errc::source_archive_entries_failed:
            return "failed to enumerate source archive entries";
        case document_errc::source_entry_open_failed:
            return "failed to open source archive entry";
        case document_errc::source_entry_name_failed:
            return "failed to read source archive entry name";
        case document_errc::output_entry_open_failed:
            return "failed to create output archive entry";
        case document_errc::output_entry_write_failed:
            return "failed to copy archive entry data";
        case document_errc::source_entry_close_failed:
            return "failed to close source archive entry";
        }

        return "unknown FeatherDoc document error";
    }
};

[[nodiscard]] inline auto document_category() -> const std::error_category & {
    static const document_error_category category{};
    return category;
}

[[nodiscard]] inline auto make_error_code(document_errc error)
    -> std::error_code {
    return {static_cast<int>(error), document_category()};
}
} // namespace featherdoc

namespace std {
template <> struct is_error_code_enum<featherdoc::document_errc> : true_type {};
} // namespace std

#endif
