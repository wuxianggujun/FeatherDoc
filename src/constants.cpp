#include <constants.hpp>

namespace featherdoc {
const char *document_error_category::name() const noexcept {
    return "featherdoc.document";
}

std::string document_error_category::message(int condition) const {
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

auto document_category() noexcept -> const std::error_category & {
    static const document_error_category category{};
    return category;
}

auto make_error_code(document_errc error) noexcept -> std::error_code {
    return {static_cast<int>(error), document_category()};
}
} // namespace featherdoc
