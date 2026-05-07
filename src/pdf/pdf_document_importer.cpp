#include <featherdoc/pdf/pdf_document_importer.hpp>

#include <featherdoc/pdf/pdf_parser.hpp>

#include <string>
#include <utility>

namespace featherdoc::pdf {
namespace {

[[nodiscard]] bool has_importable_text(const PdfParsedParagraph &paragraph) {
    return !paragraph.text.empty();
}

[[nodiscard]] bool append_imported_paragraph(featherdoc::Paragraph &cursor,
                                             bool &has_written_paragraph,
                                             const std::string &text) {
    if (!has_written_paragraph) {
        if (!cursor.has_next()) {
            return false;
        }
        if (!cursor.set_text(text)) {
            return false;
        }
        has_written_paragraph = true;
        return true;
    }

    cursor = cursor.insert_paragraph_after(text);
    return cursor.has_next();
}

[[nodiscard]] PdfDocumentImportResult
populate_document_from_parsed_pdf(featherdoc::Document &document,
                                  PdfParsedDocument parsed_document) {
    PdfDocumentImportResult result;
    result.parsed_document = std::move(parsed_document);

    const auto create_error = document.create_empty();
    if (create_error) {
        result.failure_kind = PdfDocumentImportFailureKind::document_create_failed;
        result.error_message =
            "Unable to create output Document: " + create_error.message();
        return result;
    }

    auto cursor = document.paragraphs();
    bool has_written_paragraph = false;

    for (const auto &page : result.parsed_document.pages) {
        for (const auto &paragraph : page.paragraphs) {
            if (!has_importable_text(paragraph)) {
                continue;
            }

            if (!append_imported_paragraph(cursor, has_written_paragraph,
                                           paragraph.text)) {
                result.failure_kind =
                    PdfDocumentImportFailureKind::document_population_failed;
                result.error_message =
                    "Unable to append parsed PDF text paragraph to Document";
                return result;
            }
            ++result.paragraphs_imported;
        }
    }

    if (result.paragraphs_imported == 0U) {
        result.failure_kind = PdfDocumentImportFailureKind::no_text_paragraphs;
        result.error_message =
            "PDF import currently supports text paragraphs only; no text "
            "paragraphs were detected";
        return result;
    }

    result.success = true;
    return result;
}

}  // namespace

PdfDocumentImportResult PdfDocumentImporter::import_text(
    const std::filesystem::path &input_path, featherdoc::Document &document,
    const PdfDocumentImportOptions &options) {
    if (!options.parse_options.extract_text) {
        PdfDocumentImportResult result;
        result.failure_kind = PdfDocumentImportFailureKind::extract_text_disabled;
        result.error_message =
            "PDF text import requires PdfParseOptions::extract_text=true";
        return result;
    }
    if (!options.parse_options.extract_geometry) {
        PdfDocumentImportResult result;
        result.failure_kind =
            PdfDocumentImportFailureKind::extract_geometry_disabled;
        result.error_message =
            "PDF text import requires PdfParseOptions::extract_geometry=true "
            "to group text into paragraphs";
        return result;
    }

    PdfiumParser parser;
    const auto parse_result = parser.parse(input_path, options.parse_options);
    if (!parse_result.success) {
        PdfDocumentImportResult result;
        result.failure_kind = PdfDocumentImportFailureKind::parse_failed;
        result.error_message = parse_result.error_message;
        return result;
    }

    return populate_document_from_parsed_pdf(document, parse_result.document);
}

PdfDocumentImportResult
import_pdf_text_document(const std::filesystem::path &input_path,
                         featherdoc::Document &document,
                         const PdfDocumentImportOptions &options) {
    PdfDocumentImporter importer;
    return importer.import_text(input_path, document, options);
}

}  // namespace featherdoc::pdf
