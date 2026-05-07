/*
 * FeatherDoc experimental PDF-to-Document importer facade.
 *
 * This is intentionally conservative: the first implementation imports only
 * text paragraphs from the PDFium-backed parse result into a plain DOCX model.
 */

#ifndef FEATHERDOC_PDF_PDF_DOCUMENT_IMPORTER_HPP
#define FEATHERDOC_PDF_PDF_DOCUMENT_IMPORTER_HPP

#include <featherdoc.hpp>
#include <featherdoc/pdf/pdf_interfaces.hpp>

#include <cstddef>
#include <filesystem>
#include <string>

namespace featherdoc::pdf {

struct PdfDocumentImportOptions {
    PdfParseOptions parse_options{};
};

struct PdfDocumentImportResult {
    bool success{false};
    std::string error_message;
    std::size_t paragraphs_imported{0U};
    PdfParsedDocument parsed_document;

    [[nodiscard]] explicit operator bool() const noexcept {
        return success;
    }
};

class PdfDocumentImporter {
public:
    [[nodiscard]] PdfDocumentImportResult
    import_text(const std::filesystem::path &input_path,
                featherdoc::Document &document,
                const PdfDocumentImportOptions &options = {});
};

[[nodiscard]] PdfDocumentImportResult
import_pdf_text_document(const std::filesystem::path &input_path,
                         featherdoc::Document &document,
                         const PdfDocumentImportOptions &options = {});

}  // namespace featherdoc::pdf

#endif  // FEATHERDOC_PDF_PDF_DOCUMENT_IMPORTER_HPP
