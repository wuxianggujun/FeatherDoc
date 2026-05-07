#ifndef FEATHERDOC_PDF_DOCUMENT_ADAPTER_TABLE_BORDERS_HPP
#define FEATHERDOC_PDF_DOCUMENT_ADAPTER_TABLE_BORDERS_HPP

#include <featherdoc.hpp>
#include <featherdoc/pdf/pdf_layout.hpp>

#include "pdf_document_adapter_table_layout.hpp"

namespace featherdoc::pdf::detail {

[[nodiscard]] bool
has_resolved_cell_border(const featherdoc::table_inspection_summary &table,
                         const TableCellLayout &cell);

void emit_cell_border_line(PdfPageLayout &page,
                           const featherdoc::table_inspection_summary &table,
                           const TableCellLayout &cell,
                           featherdoc::cell_border_edge edge, double row_top,
                           double row_bottom);

} // namespace featherdoc::pdf::detail

#endif // FEATHERDOC_PDF_DOCUMENT_ADAPTER_TABLE_BORDERS_HPP
