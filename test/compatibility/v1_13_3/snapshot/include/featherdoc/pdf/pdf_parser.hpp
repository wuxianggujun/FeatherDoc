/*
 * FeatherDoc experimental PDF parser facade.
 *
 * This header intentionally does not expose PDFium types. Keep PDF parsing
 * replaceable and optional while PDF-to-Word support is being explored.
 */

#ifndef FEATHERDOC_PDF_PDF_PARSER_HPP
#define FEATHERDOC_PDF_PDF_PARSER_HPP

#include <featherdoc/pdf/pdf_interfaces.hpp>

#include <filesystem>

namespace featherdoc::pdf {

class PdfiumParser final : public IPdfParser {
public:
    [[nodiscard]] PdfParseResult
    parse(const std::filesystem::path &input_path,
          const PdfParseOptions &options) override;
};

}  // namespace featherdoc::pdf

#endif  // FEATHERDOC_PDF_PDF_PARSER_HPP
