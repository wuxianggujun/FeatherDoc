#ifndef FEATHERDOC_PDF_DOCUMENT_ADAPTER_HEADERS_HPP
#define FEATHERDOC_PDF_DOCUMENT_ADAPTER_HEADERS_HPP

#include <featherdoc.hpp>
#include <featherdoc/pdf/pdf_document_adapter.hpp>

#include "pdf_document_adapter_text.hpp"

#include <cstddef>
#include <functional>
#include <string_view>
#include <vector>

namespace featherdoc::pdf::detail {

struct HeaderFooterPageLayout {
    std::vector<LineState> header_lines;
    std::vector<LineState> footer_lines;
};

struct HeaderFooterLayout {
    HeaderFooterPageLayout default_page;
    HeaderFooterPageLayout first_page;
    HeaderFooterPageLayout even_page;
    bool different_first_page_enabled{false};
    bool even_and_odd_headers_enabled{false};
};

struct HeaderFooterRenderContext {
    std::function<std::vector<LineState>(
        std::string_view, const PdfDocumentAdapterOptions &, double)>
        wrap_text;
};

[[nodiscard]] double
header_footer_line_height_points(const PdfDocumentAdapterOptions &options);

[[nodiscard]] HeaderFooterLayout build_header_footer_layout(
    featherdoc::Document &document, std::size_t section_index,
    const PdfDocumentAdapterOptions &options, double max_width_points,
    const HeaderFooterRenderContext &context);

void emit_headers_and_footers(
    PdfDocumentLayout &layout, const std::vector<HeaderFooterLayout> &headers,
    const std::vector<PdfDocumentAdapterOptions> &section_options,
    const std::vector<double> &line_height_points);

} // namespace featherdoc::pdf::detail

#endif // FEATHERDOC_PDF_DOCUMENT_ADAPTER_HEADERS_HPP
