/*
 * FeatherDoc experimental Document-to-PDF layout adapter.
 *
 * This layer translates the public FeatherDoc document inspection model into
 * the backend-neutral PDF layout data consumed by PDF generators.
 */

#ifndef FEATHERDOC_PDF_PDF_DOCUMENT_ADAPTER_HPP
#define FEATHERDOC_PDF_PDF_DOCUMENT_ADAPTER_HPP

#include <featherdoc.hpp>
#include <featherdoc/pdf/pdf_layout.hpp>

#include <string>

namespace featherdoc::pdf {

struct PdfDocumentAdapterOptions {
    PdfPageSize page_size{};
    PdfDocumentMetadata metadata{"FeatherDoc document PDF probe", "FeatherDoc"};
    double margin_left_points{72.0};
    double margin_right_points{72.0};
    double margin_top_points{72.0};
    double margin_bottom_points{72.0};
    double font_size_points{12.0};
    double line_height_points{16.0};
    double paragraph_spacing_after_points{6.0};
    std::string font_family{"Helvetica"};
};

[[nodiscard]] PdfDocumentLayout
layout_document_paragraphs(featherdoc::Document &document,
                           const PdfDocumentAdapterOptions &options = {});

} // namespace featherdoc::pdf

#endif // FEATHERDOC_PDF_PDF_DOCUMENT_ADAPTER_HPP
