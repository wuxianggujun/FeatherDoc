/*
 * FeatherDoc experimental PDF text shaping helpers.
 *
 * This bridge is intentionally independent from PdfDocumentLayout for now. It
 * lets tests and future layout code request a HarfBuzz-shaped glyph run without
 * changing the current string-based PDF writer path in the same step.
 */

#ifndef FEATHERDOC_PDF_PDF_TEXT_SHAPER_HPP
#define FEATHERDOC_PDF_PDF_TEXT_SHAPER_HPP

#include <featherdoc/pdf/pdf_glyph_run.hpp>

#include <filesystem>
#include <string>
#include <string_view>

namespace featherdoc::pdf {

struct PdfTextShaperOptions {
    std::filesystem::path font_file_path;
    double font_size_points{12.0};
    PdfGlyphDirection direction{PdfGlyphDirection::unknown};
    std::string script_tag;
};

[[nodiscard]] bool pdf_text_shaper_has_harfbuzz() noexcept;

[[nodiscard]] PdfGlyphRun
shape_pdf_text(std::string_view text, const PdfTextShaperOptions &options);

} // namespace featherdoc::pdf

#endif // FEATHERDOC_PDF_PDF_TEXT_SHAPER_HPP
