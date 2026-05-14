/*
 * FeatherDoc experimental PDF text shaping helpers.
 *
 * This bridge is intentionally independent from PdfDocumentLayout for now. It
 * lets tests and future layout code request a HarfBuzz-shaped glyph run without
 * changing the current string-based PDF writer path in the same step.
 */

#ifndef FEATHERDOC_PDF_PDF_TEXT_SHAPER_HPP
#define FEATHERDOC_PDF_PDF_TEXT_SHAPER_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc::pdf {

struct PdfGlyphPosition {
    std::uint32_t glyph_id{0U};
    std::uint32_t cluster{0U};
    double x_advance_points{0.0};
    double y_advance_points{0.0};
    double x_offset_points{0.0};
    double y_offset_points{0.0};
};

struct PdfTextShaperOptions {
    std::filesystem::path font_file_path;
    double font_size_points{12.0};
};

struct PdfGlyphRun {
    std::vector<PdfGlyphPosition> glyphs;
    std::string text;
    std::filesystem::path font_file_path;
    double font_size_points{12.0};
    bool used_harfbuzz{false};
    std::string error_message;
};

[[nodiscard]] bool pdf_text_shaper_has_harfbuzz() noexcept;

[[nodiscard]] PdfGlyphRun
shape_pdf_text(std::string_view text, const PdfTextShaperOptions &options);

} // namespace featherdoc::pdf

#endif // FEATHERDOC_PDF_PDF_TEXT_SHAPER_HPP
