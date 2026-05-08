#ifndef FEATHERDOC_PDF_DOCUMENT_ADAPTER_TEXT_HPP
#define FEATHERDOC_PDF_DOCUMENT_ADAPTER_TEXT_HPP

#include <featherdoc/pdf/pdf_font_resolver.hpp>
#include <featherdoc/pdf/pdf_layout.hpp>
#include <featherdoc/pdf/pdf_text_metrics.hpp>

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc::pdf::detail {

struct ResolvedRunStyle {
    std::string font_family{"Helvetica"};
    std::string east_asia_font_family;
    PdfResolvedFont font;
    double font_size_points{12.0};
    PdfRgbColor fill_color{0.0, 0.0, 0.0};
    bool bold{false};
    bool italic{false};
    bool underline{false};
    double vertical_shift_points{0.0};
    bool strikethrough{false};
    bool rtl{false};
};

struct TextFragment {
    std::string text;
    PdfResolvedFont font;
    double font_size_points{12.0};
    PdfRgbColor fill_color{0.0, 0.0, 0.0};
    bool bold{false};
    bool italic{false};
    bool underline{false};
    double vertical_shift_points{0.0};
    bool strikethrough{false};
    bool rtl{false};
};

struct TextToken {
    std::string text;
    bool space{false};
    bool hard_break{false};
    PdfResolvedFont font;
    double font_size_points{12.0};
    PdfRgbColor fill_color{0.0, 0.0, 0.0};
    bool bold{false};
    bool italic{false};
    bool underline{false};
    double vertical_shift_points{0.0};
    bool strikethrough{false};
    bool rtl{false};
};

struct LineState {
    std::vector<TextFragment> fragments;
    double width_points{0.0};
    double height_points{0.0};
    double baseline_offset_points{0.0};
    bool bidi{false};

    [[nodiscard]] bool empty() const noexcept {
        return this->fragments.empty();
    }
};

[[nodiscard]] PdfTextMetricsOptions
metrics_options_for(const PdfResolvedFont &font);

[[nodiscard]] double measure_text(std::string_view text,
                                  double font_size_points,
                                  const PdfResolvedFont &font);

[[nodiscard]] double line_height_points_for(const LineState &line,
                                            double fallback_points,
                                            double default_font_size_points);

[[nodiscard]] double
line_baseline_offset_points_for(const LineState &line,
                                double default_font_size_points) noexcept;

[[nodiscard]] double
content_height_points_for(const std::vector<LineState> &lines,
                          double fallback_line_height_points,
                          double default_font_size_points);

[[nodiscard]] bool line_contains_rtl_fragments(const LineState &line) noexcept;

[[nodiscard]] std::vector<TextToken>
tokenize_run_text(std::string_view text, const ResolvedRunStyle &style);

[[nodiscard]] std::vector<LineState>
wrap_run_tokens(const std::vector<TextToken> &tokens, double max_width_points);

[[nodiscard]] std::vector<LineState> wrap_run_tokens_with_line_widths(
    const std::vector<TextToken> &tokens,
    const std::function<double(std::size_t)> &max_width_for_line);

} // namespace featherdoc::pdf::detail

#endif // FEATHERDOC_PDF_DOCUMENT_ADAPTER_TEXT_HPP
