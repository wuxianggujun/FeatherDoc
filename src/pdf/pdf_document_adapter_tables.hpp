#ifndef FEATHERDOC_PDF_DOCUMENT_ADAPTER_TABLES_HPP
#define FEATHERDOC_PDF_DOCUMENT_ADAPTER_TABLES_HPP

#include <featherdoc.hpp>
#include <featherdoc/pdf/pdf_document_adapter.hpp>

#include "pdf_document_adapter_text.hpp"

#include <functional>
#include <string_view>
#include <vector>

namespace featherdoc::pdf::detail {

struct TableRenderContext {
    std::function<ResolvedRunStyle(const featherdoc::run_inspection_summary &)>
        resolve_run_style;
    std::function<std::vector<LineState>(std::string_view, double)>
        wrap_plain_text;
    std::function<PdfPageLayout &()> new_page;
};

void emit_table(featherdoc::Document &document, PdfPageLayout *&page,
                double &current_y,
                const featherdoc::table_inspection_summary &table,
                const PdfDocumentAdapterOptions &options,
                double max_width_points, double line_height_points,
                const TableRenderContext &context);

} // namespace featherdoc::pdf::detail

#endif // FEATHERDOC_PDF_DOCUMENT_ADAPTER_TABLES_HPP
