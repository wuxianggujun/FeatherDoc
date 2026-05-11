#ifndef FEATHERDOC_PDF_DOCUMENT_ADAPTER_PARAGRAPHS_HPP
#define FEATHERDOC_PDF_DOCUMENT_ADAPTER_PARAGRAPHS_HPP

#include <featherdoc.hpp>
#include <featherdoc/pdf/pdf_document_adapter.hpp>
#include <featherdoc/pdf/pdf_font_resolver.hpp>

#include "pdf_document_adapter_text.hpp"

#include <cstdint>
#include <map>
#include <string_view>
#include <vector>

namespace featherdoc::pdf::detail {

struct ParagraphNumberingState {
    std::map<std::uint32_t, std::map<std::uint32_t, std::uint32_t>> counters;
};

[[nodiscard]] ResolvedRunStyle
resolve_run_style(featherdoc::Document &document,
                  const featherdoc::run_inspection_summary &run,
                  const PdfDocumentAdapterOptions &options,
                  const PdfFontResolver &resolver);

[[nodiscard]] bool lines_contain_text(const std::vector<LineState> &lines);

[[nodiscard]] std::vector<LineState>
wrap_plain_text(featherdoc::Document &document, std::string_view text,
                const PdfDocumentAdapterOptions &options,
                const PdfFontResolver &resolver, double max_width_points);

[[nodiscard]] std::vector<TextToken>
paragraph_text_tokens(featherdoc::Document &document,
                      const featherdoc::paragraph_inspection_summary &paragraph,
                      const PdfDocumentAdapterOptions &options,
                      const PdfFontResolver &resolver,
                      ParagraphNumberingState &numbering_state);

[[nodiscard]] std::vector<LineState>
wrap_paragraph_runs(featherdoc::Document &document,
                    const featherdoc::paragraph_inspection_summary &paragraph,
                    const PdfDocumentAdapterOptions &options,
                    const PdfFontResolver &resolver, double max_width_points,
                    ParagraphNumberingState &numbering_state);

} // namespace featherdoc::pdf::detail

#endif // FEATHERDOC_PDF_DOCUMENT_ADAPTER_PARAGRAPHS_HPP
