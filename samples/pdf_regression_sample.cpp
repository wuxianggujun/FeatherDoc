#include <featherdoc/pdf/pdf_document_adapter.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>
#include <featherdoc/pdf/pdf_writer.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <fpdf_edit.h>
#include <fpdfview.h>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {

struct SampleConfig {
    std::string scenario;
    std::filesystem::path output_path;
    std::size_t expected_pages{0};
    std::vector<std::string> expected_text;
    std::optional<std::size_t> expected_image_count;
};

struct ScenarioResult {
    featherdoc::pdf::PdfDocumentLayout layout;
};

struct PdfiumDocumentCloser {
    void operator()(FPDF_DOCUMENT document) const {
        if (document != nullptr) {
            FPDF_CloseDocument(document);
        }
    }
};

struct PdfiumPageCloser {
    void operator()(FPDF_PAGE page) const {
        if (page != nullptr) {
            FPDF_ClosePage(page);
        }
    }
};

using PdfiumDocumentPtr =
    std::unique_ptr<std::remove_pointer_t<FPDF_DOCUMENT>, PdfiumDocumentCloser>;
using PdfiumPagePtr =
    std::unique_ptr<std::remove_pointer_t<FPDF_PAGE>, PdfiumPageCloser>;

struct TextColumnSegment {
    double x_points{0.0};
    std::string text;
};

struct TextLineFragment {
    double x_points{0.0};
    double right_points{0.0};
    double height_points{0.0};
    std::string text;
};

[[nodiscard]] featherdoc::pdf::PdfPageLayout make_letter_page() {
    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    return page;
}

[[nodiscard]] featherdoc::pdf::PdfPageLayout make_letter_landscape_page() {
    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize{792.0, 612.0};
    return page;
}

[[nodiscard]] std::string utf8_from_wide(std::wstring_view text) {
#if defined(_WIN32)
    if (text.empty()) {
        return {};
    }

    const auto required_size = WideCharToMultiByte(
        CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0,
        nullptr, nullptr);
    if (required_size <= 0) {
        return {};
    }

    std::string result(static_cast<std::size_t>(required_size), '\0');
    const auto converted = WideCharToMultiByte(
        CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(),
        required_size, nullptr, nullptr);
    if (converted <= 0) {
        return {};
    }

    result.resize(static_cast<std::size_t>(converted));
    return result;
#else
    return std::string{text.begin(), text.end()};
#endif
}

[[nodiscard]] std::string utf8_from_u8(std::u8string_view text) {
    return {reinterpret_cast<const char *>(text.data()), text.size()};
}

[[nodiscard]] std::string utf8_from_path(const std::filesystem::path &path) {
    const auto utf8 = path.u8string();
    return {utf8.begin(), utf8.end()};
}

[[nodiscard]] std::string environment_value(const char *name) {
#if defined(_WIN32)
    char *value = nullptr;
    std::size_t size = 0;
    if (::_dupenv_s(&value, &size, name) != 0 || value == nullptr) {
        return {};
    }

    std::string result(value, size > 0 ? size - 1 : 0);
    std::free(value);
    return result;
#else
    const char *value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string{value};
#endif
}

[[nodiscard]] std::string pdfium_error_to_string(unsigned long error_code) {
    switch (error_code) {
    case FPDF_ERR_SUCCESS:
        return "success";
    case FPDF_ERR_UNKNOWN:
        return "unknown error";
    case FPDF_ERR_FILE:
        return "file access or format error";
    case FPDF_ERR_FORMAT:
        return "invalid PDF format";
    case FPDF_ERR_PASSWORD:
        return "password is required or incorrect";
    case FPDF_ERR_SECURITY:
        return "unsupported security scheme";
    case FPDF_ERR_PAGE:
        return "page not found or content error";
    default:
        return "unrecognized PDFium error " + std::to_string(error_code);
    }
}

[[nodiscard]] std::optional<std::size_t> count_pdf_image_objects(
    const std::filesystem::path &path, std::string &error_message) {
    const auto utf8_path = utf8_from_path(path);
    PdfiumDocumentPtr document{FPDF_LoadDocument(utf8_path.c_str(), nullptr)};
    if (!document) {
        error_message = "Unable to reopen PDF for image counting: " +
                        pdfium_error_to_string(FPDF_GetLastError());
        return std::nullopt;
    }

    const int page_count = FPDF_GetPageCount(document.get());
    if (page_count < 0) {
        error_message =
            "PDFium reported an invalid page count while counting images";
        return std::nullopt;
    }

    std::size_t image_count = 0U;
    for (int page_index = 0; page_index < page_count; ++page_index) {
        PdfiumPagePtr page{FPDF_LoadPage(document.get(), page_index)};
        if (!page) {
            error_message = "Unable to load PDF page " +
                            std::to_string(page_index) +
                            " while counting images";
            return std::nullopt;
        }

        const int object_count = FPDFPage_CountObjects(page.get());
        if (object_count < 0) {
            error_message =
                "PDFium reported an invalid page object count while counting "
                "images";
            return std::nullopt;
        }

        for (int object_index = 0; object_index < object_count; ++object_index) {
            const auto page_object = FPDFPage_GetObject(page.get(), object_index);
            if (page_object == nullptr) {
                continue;
            }
            if (FPDFPageObj_GetType(page_object) == FPDF_PAGEOBJ_IMAGE) {
                ++image_count;
            }
        }
    }

    return image_count;
}

[[nodiscard]] std::vector<std::filesystem::path>
candidate_latin_fonts() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured =
            environment_value("FEATHERDOC_TEST_PROPORTIONAL_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }
    if (const auto configured = environment_value("FEATHERDOC_TEST_LATIN_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/arial.ttf");
    candidates.emplace_back("C:/Windows/Fonts/segoeui.ttf");
    candidates.emplace_back("C:/Windows/Fonts/calibri.ttf");
#elif defined(__APPLE__)
    candidates.emplace_back(
        "/System/Library/Fonts/Supplemental/Arial.ttf");
    candidates.emplace_back(
        "/System/Library/Fonts/Supplemental/Helvetica.ttf");
#else
    candidates.emplace_back(
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf");
    candidates.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
#endif

    return candidates;
}

[[nodiscard]] std::vector<std::filesystem::path>
candidate_cjk_fonts() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured = environment_value("FEATHERDOC_TEST_CJK_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/Deng.ttf");
    candidates.emplace_back("C:/Windows/Fonts/NotoSansSC-VF.ttf");
    candidates.emplace_back("C:/Windows/Fonts/msyh.ttc");
    candidates.emplace_back("C:/Windows/Fonts/simhei.ttf");
    candidates.emplace_back("C:/Windows/Fonts/simsun.ttc");
#elif defined(__APPLE__)
    candidates.emplace_back(
        "/System/Library/Fonts/PingFang.ttc");
    candidates.emplace_back(
        "/System/Library/Fonts/Heiti Light.ttc");
    candidates.emplace_back(
        "/System/Library/Fonts/STHeiti Light.ttc");
#else
    candidates.emplace_back(
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc");
    candidates.emplace_back(
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc");
    candidates.emplace_back("/usr/share/fonts/truetype/arphic/ukai.ttc");
    candidates.emplace_back("/usr/share/fonts/truetype/arphic/uming.ttc");
#endif

    return candidates;
}

[[nodiscard]] std::vector<std::filesystem::path> candidate_arabic_fonts() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured = environment_value("FEATHERDOC_TEST_ARABIC_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/NotoSansArabic-Regular.ttf");
    candidates.emplace_back("C:/Windows/Fonts/NotoNaskhArabic-Regular.ttf");
    candidates.emplace_back("C:/Windows/Fonts/NotoKufiArabic-Regular.ttf");
    candidates.emplace_back("C:/Windows/Fonts/Candarab.ttf");
    candidates.emplace_back("C:/Windows/Fonts/GARABD.TTF");
#elif defined(__APPLE__)
    candidates.emplace_back(
        "/System/Library/Fonts/Supplemental/Geeza Pro.ttc");
    candidates.emplace_back("/System/Library/Fonts/Geeza Pro.ttc");
    candidates.emplace_back(
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf");
#else
    candidates.emplace_back(
        "/usr/share/fonts/truetype/noto/NotoSansArabic-Regular.ttf");
    candidates.emplace_back(
        "/usr/share/fonts/truetype/noto/NotoNaskhArabic-Regular.ttf");
    candidates.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
#endif

    return candidates;
}

[[nodiscard]] std::filesystem::path
first_existing_path(const std::vector<std::filesystem::path> &candidates) {
    for (const auto &candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

[[nodiscard]] std::filesystem::path resolve_cjk_font() {
    return first_existing_path(candidate_cjk_fonts());
}

[[nodiscard]] std::filesystem::path resolve_latin_font() {
    return first_existing_path(candidate_latin_fonts());
}

[[nodiscard]] std::filesystem::path resolve_arabic_font() {
    return first_existing_path(candidate_arabic_fonts());
}

[[nodiscard]] std::filesystem::path write_tiny_rgb_png(
    const std::filesystem::path &asset_dir,
    std::string_view file_name) {
    static constexpr std::array<unsigned char, 69> tiny_png{
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00,
        0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x00, 0x00, 0x00, 0x90,
        0x77, 0x53, 0xDE, 0x00, 0x00, 0x00, 0x0C, 0x49, 0x44, 0x41,
        0x54, 0x78, 0x9C, 0x63, 0xF8, 0xFF, 0xFF, 0x3F, 0x00, 0x05,
        0xFE, 0x02, 0xFE, 0x0D, 0xEF, 0x46, 0xB8, 0x00, 0x00, 0x00,
        0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
    };

    if (!asset_dir.empty()) {
        std::filesystem::create_directories(asset_dir);
    }
    const auto png_path = asset_dir.empty()
                              ? std::filesystem::path{file_name}
                              : asset_dir / std::filesystem::path{file_name};
    std::ofstream output(png_path, std::ios::binary);
    output.write(reinterpret_cast<const char *>(tiny_png.data()),
                 static_cast<std::streamsize>(tiny_png.size()));
    return png_path;
}

[[nodiscard]] std::filesystem::path write_tiny_rgb_png(
    const std::filesystem::path &asset_dir) {
    return write_tiny_rgb_png(
        asset_dir, "featherdoc-pdf-regression-inline-image.png");
}

[[nodiscard]] std::filesystem::path write_quadrant_rgb_png(
    const std::filesystem::path &asset_dir,
    std::string_view file_name) {
    static constexpr std::array<unsigned char, 96> quadrant_png{
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00,
        0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x08,
        0x00, 0x00, 0x00, 0x04, 0x08, 0x02, 0x00, 0x00, 0x00, 0x3C,
        0xAF, 0xE9, 0xA7, 0x00, 0x00, 0x00, 0x1D, 0x49, 0x44, 0x41,
        0x54, 0x78, 0xDA, 0x63, 0xF8, 0xCF, 0xC0, 0x00, 0x47, 0x48,
        0xCC, 0xFF, 0x0C, 0x38, 0x25, 0x18, 0x4E, 0x20, 0xD0, 0xFF,
        0x3B, 0x08, 0x84, 0x53, 0x02, 0x00, 0x8F, 0xCE, 0x25, 0x09,
        0x87, 0xCF, 0x36, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
        0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
    };

    if (!asset_dir.empty()) {
        std::filesystem::create_directories(asset_dir);
    }
    const auto png_path = asset_dir.empty()
                              ? std::filesystem::path{file_name}
                              : asset_dir / std::filesystem::path{file_name};
    std::ofstream output(png_path, std::ios::binary);
    output.write(reinterpret_cast<const char *>(quadrant_png.data()),
                 static_cast<std::streamsize>(quadrant_png.size()));
    return png_path;
}

[[nodiscard]] double rect_right(const featherdoc::pdf::PdfRect &rect) {
    return rect.x_points + rect.width_points;
}

void append_text_segment(std::string &text, std::string_view segment) {
    if (segment.empty()) {
        return;
    }
    if (!text.empty() &&
        !std::isspace(static_cast<unsigned char>(text.back()))) {
        text.push_back(' ');
    }
    text.append(segment.data(), segment.size());
}

void append_span_to_fragment(
    TextLineFragment &fragment,
    const featherdoc::pdf::PdfParsedTextSpan &span) {
    if (!fragment.text.empty() && !span.text.empty() &&
        !std::isspace(static_cast<unsigned char>(fragment.text.back())) &&
        !std::isspace(static_cast<unsigned char>(span.text.front()))) {
        const double gap = span.bounds.x_points - fragment.right_points;
        const double gap_threshold =
            (std::max)(3.0, (std::max)(fragment.height_points,
                                       span.bounds.height_points) *
                                 0.25);
        if (gap > gap_threshold) {
            fragment.text.push_back(' ');
        }
    }
    fragment.text += span.text;
    fragment.right_points = (std::max)(fragment.right_points,
                                       rect_right(span.bounds));
    fragment.height_points =
        (std::max)(fragment.height_points, span.bounds.height_points);
}

[[nodiscard]] std::vector<TextLineFragment> split_line_into_fragments(
    const featherdoc::pdf::PdfParsedTextLine &line) {
    std::vector<TextLineFragment> fragments;
    for (const auto &span : line.text_spans) {
        if (span.text.empty()) {
            continue;
        }

        const double gap = fragments.empty()
                               ? 0.0
                               : span.bounds.x_points -
                                     fragments.back().right_points;
        const double split_threshold =
            (std::max)(8.0, (std::max)(line.bounds.height_points, 1.0) * 0.75);
        if (fragments.empty() || gap > split_threshold) {
            fragments.push_back(TextLineFragment{
                span.bounds.x_points, rect_right(span.bounds),
                span.bounds.height_points, {}});
        }
        append_span_to_fragment(fragments.back(), span);
    }
    return fragments;
}

[[nodiscard]] std::vector<TextColumnSegment> collect_column_segments(
    const std::vector<featherdoc::pdf::PdfParsedTextLine> &lines) {
    std::vector<TextColumnSegment> segments;
    for (const auto &line : lines) {
        for (const auto &fragment : split_line_into_fragments(line)) {
            auto matching_segment = std::find_if(
                segments.begin(), segments.end(),
                [&fragment](const TextColumnSegment &segment) {
                    return std::abs(segment.x_points - fragment.x_points) <=
                           8.0;
                });
            if (matching_segment == segments.end()) {
                segments.push_back(
                    TextColumnSegment{fragment.x_points, fragment.text});
            } else {
                append_text_segment(matching_segment->text, fragment.text);
            }
        }
    }

    segments.erase(
        std::remove_if(segments.begin(), segments.end(),
                       [](const TextColumnSegment &segment) {
                           return segment.text.size() < 2U;
                       }),
        segments.end());
    return segments;
}

[[nodiscard]] std::string collect_text(
    const featherdoc::pdf::PdfParsedDocument &document) {
    std::string text;

    for (const auto &page : document.pages) {
        if (!page.text_lines.empty()) {
            for (const auto &line : page.text_lines) {
                append_text_segment(text, line.text);
            }
        } else {
            for (const auto &span : page.text_spans) {
                text += span.text;
            }
        }

        for (const auto &segment : collect_column_segments(page.text_lines)) {
            append_text_segment(text, segment.text);
        }

        for (const auto &table : page.table_candidates) {
            for (std::size_t column_index = 0U;
                 column_index < table.column_anchor_x_points.size();
                 ++column_index) {
                std::vector<const featherdoc::pdf::PdfParsedTableCell *>
                    cells;
                for (const auto &row : table.rows) {
                    auto matching_cell = std::find_if(
                        row.cells.begin(), row.cells.end(),
                        [column_index](
                            const featherdoc::pdf::PdfParsedTableCell &cell) {
                            return cell.has_text &&
                                   cell.column_index <= column_index &&
                                   column_index <
                                       cell.column_index + cell.column_span;
                        });
                    if (matching_cell != row.cells.end()) {
                        cells.push_back(&*matching_cell);
                    }
                }

                cells.erase(std::unique(cells.begin(), cells.end()),
                            cells.end());
                for (const auto *cell : cells) {
                    append_text_segment(text, cell->text);
                }
            }
        }
    }
    return text;
}

[[nodiscard]] featherdoc::pdf::PdfTextRun make_text_run(
    double x_points,
    double y_points,
    std::string text,
    double font_size_points,
    featherdoc::pdf::PdfRgbColor fill_color,
    bool bold,
    bool italic,
    bool underline,
    bool unicode,
    std::string font_family = "Helvetica",
    std::filesystem::path font_file_path = {}) {
    if (!unicode && font_file_path.empty()) {
        static const auto latin_font = resolve_latin_font();
        if (!latin_font.empty()) {
            unicode = true;
            font_file_path = latin_font;
        }
    }

    return featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{x_points, y_points},
        std::move(text),
        std::move(font_family),
        std::move(font_file_path),
        font_size_points,
        fill_color,
        bold,
        italic,
        underline,
        unicode,
    };
}

[[nodiscard]] bool ensure_document_contract_style(
    featherdoc::Document &document,
    std::string_view style_id,
    featherdoc::character_style_definition definition);

#include "pdf_regression_sample_basic_text.inc"
#include "pdf_regression_sample_style_matrix.inc"
#include "pdf_regression_sample_primitives.inc"
#include "pdf_regression_sample_document_tables.inc"
#include "pdf_regression_sample_document_table_cjk_repeat.inc"
#include "pdf_regression_sample_table_merges.inc"
#include "pdf_regression_sample_sections_lists.inc"
#include "pdf_regression_sample_cjk_lite.inc"
#include "pdf_regression_sample_cjk_complex.inc"
#include "pdf_regression_sample_cjk_vertical_flow.inc"
#include "pdf_regression_sample_cjk_anchor_boundary.inc"
#include "pdf_regression_sample_cjk_repeated_vertical.inc"
#include "pdf_regression_sample_cjk_tail.inc"
#include "pdf_regression_sample_cjk_page_flow_tail.inc"

#include "pdf_regression_sample_runner_support.inc"
#include "pdf_regression_sample_runner.inc"

#if defined(_WIN32)
int wmain(int argc, wchar_t **argv) {
    return run_program(collect_utf8_arguments(argc, argv));
}
#else
int main(int argc, char **argv) {
    return run_program(collect_utf8_arguments(argc, argv));
}
#endif
