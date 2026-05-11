/*
 * FeatherDoc experimental PDF text metrics helpers.
 *
 * This layer stays deliberately small and backend-neutral. It uses FreeType
 * when a concrete font file is available, and keeps a portable fallback when
 * the caller only provides a family name.
 */

#ifndef FEATHERDOC_PDF_PDF_TEXT_METRICS_HPP
#define FEATHERDOC_PDF_PDF_TEXT_METRICS_HPP

#include <filesystem>
#include <string_view>

namespace featherdoc::pdf {

struct PdfTextMetricsOptions {
    std::string_view font_family;
    std::filesystem::path font_file_path;
};

[[nodiscard]] double measure_text_width_points(std::string_view text,
                                               double font_size_points,
                                               std::string_view font_family = {}) noexcept;

[[nodiscard]] double measure_text_width_points(
    std::string_view text,
    double font_size_points,
    const PdfTextMetricsOptions &options) noexcept;

[[nodiscard]] double estimate_line_height_points(double font_size_points,
                                                 std::string_view font_family = {}) noexcept;

[[nodiscard]] double estimate_line_height_points(
    double font_size_points,
    const PdfTextMetricsOptions &options) noexcept;

} // namespace featherdoc::pdf

#endif // FEATHERDOC_PDF_PDF_TEXT_METRICS_HPP
