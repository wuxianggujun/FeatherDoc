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
    case document_errc::relationships_xml_read_failed:
        return "failed to read word/_rels/document.xml.rels";
    case document_errc::relationships_xml_parse_failed:
        return "failed to parse word/_rels/document.xml.rels";
    case document_errc::related_part_open_failed:
        return "failed to open related document part";
    case document_errc::related_part_read_failed:
        return "failed to read related document part";
    case document_errc::related_part_parse_failed:
        return "failed to parse related document part";
    case document_errc::document_xml_open_failed:
        return "failed to open word/document.xml";
    case document_errc::document_xml_read_failed:
        return "failed to read word/document.xml";
    case document_errc::encrypted_document_unsupported:
        return "password-protected or encrypted .docx files are not supported";
    case document_errc::document_xml_parse_failed:
        return "failed to parse word/document.xml";
    case document_errc::content_types_xml_read_failed:
        return "failed to read [Content_Types].xml";
    case document_errc::content_types_xml_parse_failed:
        return "failed to parse [Content_Types].xml";
    case document_errc::numbering_xml_read_failed:
        return "failed to read word/numbering.xml";
    case document_errc::numbering_xml_parse_failed:
        return "failed to parse word/numbering.xml";
    case document_errc::styles_xml_read_failed:
        return "failed to read word/styles.xml";
    case document_errc::styles_xml_parse_failed:
        return "failed to parse word/styles.xml";
    case document_errc::image_file_read_failed:
        return "failed to read image file";
    case document_errc::image_format_unsupported:
        return "image format is not supported";
    case document_errc::image_size_read_failed:
        return "failed to determine image dimensions";
    case document_errc::settings_xml_read_failed:
        return "failed to read word/settings.xml";
    case document_errc::settings_xml_parse_failed:
        return "failed to parse word/settings.xml";
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
    case document_errc::invalid_package_structure:
        return "invalid DOCX package structure";
    case document_errc::archive_limit_exceeded:
        return "DOCX archive resource limit exceeded";
    case document_errc::output_archive_finalize_failed:
        return "failed to finalize output archive";
    case document_errc::output_replace_failed:
        return "failed to replace output document";
    case document_errc::identifier_space_exhausted:
        return "document identifier space exhausted";
    case document_errc::package_repair_not_possible:
        return "DOCX package cannot be repaired safely";
    case document_errc::package_repair_validation_failed:
        return "repaired DOCX package failed strict validation";
    case document_errc::output_file_sync_failed:
        return "failed to synchronize temporary output document";
    case document_errc::output_directory_sync_failed_after_replace:
        return "output document replaced but parent directory synchronization failed";
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
