/*
 * FeatherDoc experimental PDF layout model.
 *
 * These types are pure data. They intentionally do not depend on PDFio,
 * PDFium, or any future native PDF backend.
 */

#ifndef FEATHERDOC_PDF_PDF_LAYOUT_HPP
#define FEATHERDOC_PDF_PDF_LAYOUT_HPP

#include <string>
#include <vector>

namespace featherdoc::pdf {

struct PdfPageSize {
    double width_points{595.275590551};
    double height_points{841.88976378};

    [[nodiscard]] static constexpr PdfPageSize a4_portrait() {
        return {};
    }

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
    double font_size_points{12.0};
    PdfRgbColor fill_color{0.0, 0.0, 0.0};
    bool unicode{false};
};

struct PdfRectangle {
    PdfRect bounds;
    PdfRgbColor stroke_color{0.0, 0.0, 0.0};
    PdfRgbColor fill_color{1.0, 1.0, 1.0};
    double line_width_points{1.0};
    bool stroke{true};
    bool fill{false};
};

struct PdfPageLayout {
    PdfPageSize size{};
    std::vector<PdfRectangle> rectangles;
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

}  // namespace featherdoc::pdf

#endif  // FEATHERDOC_PDF_PDF_LAYOUT_HPP
