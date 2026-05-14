#include "pdf_document_adapter_render.hpp"

#include <featherdoc/pdf/pdf_text_shaper.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace featherdoc::pdf::detail {
namespace {

constexpr double kPi = 3.14159265358979323846;

[[nodiscard]] int hex_digit_value(char value) noexcept {
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F') {
        return value - 'A' + 10;
    }
    return -1;
}

[[nodiscard]] PdfGlyphRun shape_fragment_text(const TextFragment &fragment) {
    if (fragment.text.empty() || fragment.font.font_file_path.empty()) {
        return {};
    }

    PdfTextShaperOptions shaper_options{fragment.font.font_file_path,
                                        fragment.font_size_points};
    shaper_options.direction = fragment.shaping_direction;
    shaper_options.script_tag = fragment.shaping_script_tag;
    auto glyph_run = shape_pdf_text(fragment.text, shaper_options);
    if (!glyph_run.used_harfbuzz || !glyph_run.error_message.empty() ||
        glyph_run.glyphs.empty()) {
        return {};
    }
    return glyph_run;
}

} // namespace

double content_width(const PdfDocumentAdapterOptions &options) {
    const auto width = options.page_size.width_points -
                       options.margin_left_points - options.margin_right_points;
    return std::max(1.0, width);
}

double twips_to_points(std::uint32_t twips) noexcept {
    return static_cast<double>(twips) / 20.0;
}

double first_baseline_y(const PdfDocumentAdapterOptions &options) {
    return options.page_size.height_points - options.margin_top_points -
           options.font_size_points;
}

std::optional<PdfRgbColor> parse_hex_rgb_color(std::string_view text) noexcept {
    if (!text.empty() && text.front() == '#') {
        text.remove_prefix(1U);
    }
    if (text.size() != 6U) {
        return std::nullopt;
    }

    int values[6]{};
    for (std::size_t index = 0U; index < text.size(); ++index) {
        values[index] = hex_digit_value(text[index]);
        if (values[index] < 0) {
            return std::nullopt;
        }
    }

    const auto red = values[0] * 16 + values[1];
    const auto green = values[2] * 16 + values[3];
    const auto blue = values[4] * 16 + values[5];
    return PdfRgbColor{
        static_cast<double>(red) / 255.0,
        static_cast<double>(green) / 255.0,
        static_cast<double>(blue) / 255.0,
    };
}

void emit_line_at(PdfPageLayout &page, const LineState &line,
                  double start_x_points, double baseline_y) {
    emit_line_at(page, line, start_x_points, baseline_y, 0.0);
}

void emit_line_at(PdfPageLayout &page, const LineState &line,
                  double start_x_points, double baseline_y,
                  double rotation_degrees) {
    const auto radians = rotation_degrees * kPi / 180.0;
    const auto x_advance = std::cos(radians);
    const auto y_advance = std::sin(radians);
    auto current_advance = 0.0;
    for (const auto &fragment : line.fragments) {
        if (!fragment.text.empty()) {
            auto glyph_run = shape_fragment_text(fragment);
            page.text_runs.push_back(PdfTextRun{
                PdfPoint{start_x_points + current_advance * x_advance,
                         baseline_y + current_advance * y_advance},
                fragment.text,
                fragment.font.font_family,
                fragment.font.font_file_path,
                fragment.font_size_points,
                fragment.fill_color,
                fragment.bold,
                fragment.italic,
                fragment.underline,
                fragment.font.unicode,
                rotation_degrees,
                fragment.font.synthetic_bold,
                fragment.font.synthetic_italic,
                std::move(glyph_run),
            });
        }
        current_advance +=
            measure_text(fragment.text, fragment.font_size_points,
                         fragment.font, fragment.shaping_direction,
                         fragment.shaping_script_tag);
    }
}

} // namespace featherdoc::pdf::detail
