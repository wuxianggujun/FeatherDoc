#ifndef FEATHERDOC_PDF_DOCUMENT_ADAPTER_RENDER_HPP
#define FEATHERDOC_PDF_DOCUMENT_ADAPTER_RENDER_HPP

#include <featherdoc/pdf/pdf_document_adapter.hpp>

#include "pdf_document_adapter_text.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace featherdoc::pdf::detail {

[[nodiscard]] double content_width(const PdfDocumentAdapterOptions &options);

[[nodiscard]] double twips_to_points(std::uint32_t twips) noexcept;

[[nodiscard]] double first_baseline_y(const PdfDocumentAdapterOptions &options);

[[nodiscard]] std::optional<PdfRgbColor>
parse_hex_rgb_color(std::string_view text) noexcept;

void emit_line_at(PdfPageLayout &page, const LineState &line,
                  double start_x_points, double baseline_y);

void emit_line_at(PdfPageLayout &page, const LineState &line,
                  double start_x_points, double baseline_y,
                  double rotation_degrees);

} // namespace featherdoc::pdf::detail

#endif // FEATHERDOC_PDF_DOCUMENT_ADAPTER_RENDER_HPP
