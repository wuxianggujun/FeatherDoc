/*
 * FeatherDoc experimental PDF glyph run data model.
 *
 * These types describe shaped text in PDF point units. They are backend-neutral
 * data and can be carried by layout before a writer starts consuming glyph IDs.
 */

#ifndef FEATHERDOC_PDF_PDF_GLYPH_RUN_HPP
#define FEATHERDOC_PDF_PDF_GLYPH_RUN_HPP

#include <cstdint>
#include <filesystem>
#include <string>
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

struct PdfGlyphRun {
    std::vector<PdfGlyphPosition> glyphs;
    std::string text;
    std::filesystem::path font_file_path;
    double font_size_points{12.0};
    bool used_harfbuzz{false};
    std::string error_message;
};

[[nodiscard]] inline double
glyph_run_x_advance_points(const PdfGlyphRun &glyph_run) noexcept {
    double advance_points = 0.0;
    for (const auto &glyph : glyph_run.glyphs) {
        advance_points += glyph.x_advance_points;
    }
    return advance_points;
}

} // namespace featherdoc::pdf

#endif // FEATHERDOC_PDF_PDF_GLYPH_RUN_HPP
