/*
 * FeatherDoc experimental PDF layout model.
 *
 * These types are pure data. They intentionally do not depend on PDFio,
 * PDFium, or any future native PDF backend.
 */

#ifndef FEATHERDOC_PDF_PDF_LAYOUT_HPP
#define FEATHERDOC_PDF_PDF_LAYOUT_HPP

#include <featherdoc/pdf/pdf_glyph_run.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace featherdoc::pdf {

struct PdfPageSize {
    double width_points{595.275590551};
    double height_points{841.88976378};

    [[nodiscard]] static constexpr PdfPageSize a4_portrait() { return {}; }

    [[nodiscard]] static constexpr PdfPageSize letter_portrait() {
        return {612.0, 792.0};
    }
};

struct PdfPoint {
    double x_points{0.0};
    double y_points{0.0};
};

struct PdfRect {
    double x_points{0.0};
    double y_points{0.0};
    double width_points{0.0};
    double height_points{0.0};
};

struct PdfRgbColor {
    double red{0.0};
    double green{0.0};
    double blue{0.0};
};

struct PdfTextRun {
    PdfPoint baseline_origin;
    std::string text;
    std::string font_family{"Helvetica"};
    std::filesystem::path font_file_path;
    double font_size_points{12.0};
    PdfRgbColor fill_color{0.0, 0.0, 0.0};
    bool bold{false};
    bool italic{false};
    bool underline{false};
    bool unicode{false};
    double rotation_degrees{0.0};
    bool synthetic_bold{false};
    bool synthetic_italic{false};
    PdfGlyphRun glyph_run;
};

struct PdfRectangle {
    PdfRect bounds;
    PdfRgbColor stroke_color{0.0, 0.0, 0.0};
    PdfRgbColor fill_color{1.0, 1.0, 1.0};
    double line_width_points{1.0};
    bool stroke{true};
    bool fill{false};
};

enum class PdfLineCap {
    butt,
    round,
    square,
};

struct PdfLine {
    PdfPoint start;
    PdfPoint end;
    PdfRgbColor stroke_color{0.0, 0.0, 0.0};
    double line_width_points{1.0};
    double dash_phase_points{0.0};
    double dash_on_points{0.0};
    double dash_off_points{0.0};
    PdfLineCap line_cap{PdfLineCap::butt};
};

struct PdfImage {
    PdfRect bounds;
    std::filesystem::path source_path;
    std::string content_type;
    std::string alt_text;
    bool interpolate{true};
    bool cleanup_source_after_write{false};
    bool draw_behind_text{true};
    std::uint32_t crop_left_per_mille{0U};
    std::uint32_t crop_top_per_mille{0U};
    std::uint32_t crop_right_per_mille{0U};
    std::uint32_t crop_bottom_per_mille{0U};
};

struct PdfPageLayout {
    PdfPageSize size{};
    std::size_t section_index{0};
    std::size_t section_page_index{0};
    std::vector<PdfRectangle> rectangles;
    std::vector<PdfLine> lines;
    std::vector<PdfImage> images;
    std::vector<PdfTextRun> text_runs;
};

struct PdfDocumentMetadata {
    std::string title;
    std::string creator{"FeatherDoc"};
};

struct PdfDocumentLayout {
    PdfDocumentMetadata metadata;
    std::vector<PdfPageLayout> pages;
};

} // namespace featherdoc::pdf

#endif // FEATHERDOC_PDF_PDF_LAYOUT_HPP
