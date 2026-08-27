/*
 * FeatherDoc experimental PDF writer facade.
 *
 * This header intentionally does not expose PDFio types. Keep the dependency
 * replaceable while the DOCX-to-PDF renderer is still being built.
 */

#ifndef FEATHERDOC_PDF_PDF_WRITER_HPP
#define FEATHERDOC_PDF_PDF_WRITER_HPP

#include <featherdoc/pdf/pdf_interfaces.hpp>

#include <filesystem>

namespace featherdoc::pdf {

class PdfioGenerator final : public IPdfGenerator {
public:
    [[nodiscard]] PdfWriteResult
    write(const PdfDocumentLayout &layout,
          const std::filesystem::path &output_path,
          const PdfWriterOptions &options) override;
};

[[nodiscard]] PdfDocumentLayout
make_pdfio_probe_layout(const PdfWriterOptions &options = {});

[[nodiscard]] PdfWriteResult
write_pdfio_document(const std::filesystem::path &output_path,
                     const PdfDocumentLayout &layout,
                     const PdfWriterOptions &options = {});

// First integration probe for the PDF byte writer layer.
// It is not a DOCX renderer yet; it only proves that FeatherDoc can own
// PDF byte generation through the optional PDFio-backed module.
[[nodiscard]] PdfWriteResult
write_pdfio_probe_document(const std::filesystem::path &output_path,
                           const PdfWriterOptions &options = {});

}  // namespace featherdoc::pdf

#endif  // FEATHERDOC_PDF_PDF_WRITER_HPP
