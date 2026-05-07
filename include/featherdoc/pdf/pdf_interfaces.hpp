/*
 * FeatherDoc experimental PDF abstraction interfaces.
 *
 * Keep PDFio/PDFium behind these interfaces so the public API remains
 * replaceable while the Word/PDF conversion path evolves.
 */

#ifndef FEATHERDOC_PDF_PDF_INTERFACES_HPP
#define FEATHERDOC_PDF_PDF_INTERFACES_HPP

#include <featherdoc/pdf/pdf_layout.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace featherdoc::pdf {

struct PdfWriterOptions {
    PdfPageSize page_size{};
    std::string title{"FeatherDoc PDF writer probe"};
    std::string creator{"FeatherDoc"};
    bool subset_unicode_fonts{true};
};

struct PdfWriteResult {
    bool success{false};
    std::string error_message;
    std::uintmax_t bytes_written{0};

    [[nodiscard]] explicit operator bool() const noexcept {
        return success;
    }
};

class IPdfGenerator {
public:
    virtual ~IPdfGenerator() = default;

    [[nodiscard]] virtual PdfWriteResult
    write(const PdfDocumentLayout &layout,
          const std::filesystem::path &output_path,
          const PdfWriterOptions &options) = 0;
};

struct PdfParseOptions {
    bool extract_text{true};
    bool extract_geometry{true};
};

struct PdfParsedTextSpan {
    std::string text;
    PdfRect bounds;
    std::string font_name;
    double font_size_points{0.0};
};

struct PdfParsedTextLine {
    std::string text;
    PdfRect bounds;
    std::vector<PdfParsedTextSpan> text_spans;
};

struct PdfParsedParagraph {
    std::string text;
    PdfRect bounds;
    std::vector<PdfParsedTextLine> lines;
};

struct PdfParsedPage {
    PdfPageSize size{};
    std::vector<PdfParsedTextSpan> text_spans;
    std::vector<PdfParsedTextLine> text_lines;
    std::vector<PdfParsedParagraph> paragraphs;
};

struct PdfParsedDocument {
    std::vector<PdfParsedPage> pages;
};

struct PdfParseResult {
    bool success{false};
    std::string error_message;
    PdfParsedDocument document;

    [[nodiscard]] explicit operator bool() const noexcept {
        return success;
    }
};

class IPdfParser {
public:
    virtual ~IPdfParser() = default;

    [[nodiscard]] virtual PdfParseResult
    parse(const std::filesystem::path &input_path,
          const PdfParseOptions &options) = 0;
};

}  // namespace featherdoc::pdf

#endif  // FEATHERDOC_PDF_PDF_INTERFACES_HPP
