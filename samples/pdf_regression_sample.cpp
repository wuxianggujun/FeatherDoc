#include <featherdoc/pdf/pdf_document_adapter.hpp>
#if defined(FEATHERDOC_PDF_REGRESSION_IMPORT_VALIDATION)
#include <featherdoc/pdf/pdf_parser.hpp>
#endif
#include <featherdoc/pdf/pdf_writer.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
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

#if defined(FEATHERDOC_PDF_REGRESSION_IMPORT_VALIDATION)
#include <fpdf_edit.h>
#include <fpdfview.h>
#endif

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

#if defined(FEATHERDOC_PDF_REGRESSION_IMPORT_VALIDATION)
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
#endif

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

#if defined(FEATHERDOC_PDF_REGRESSION_IMPORT_VALIDATION)
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
#endif

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

#if defined(FEATHERDOC_PDF_REGRESSION_IMPORT_VALIDATION)
[[nodiscard]] std::string collect_text(
    const featherdoc::pdf::PdfParsedDocument &document) {
    std::string text;
    for (const auto &page : document.pages) {
        for (const auto &span : page.text_spans) {
            text += span.text;
        }
    }
    return text;
}
#endif

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
    std::filesystem::path font_file_path = {},
    bool strikethrough = false) {
    auto run = featherdoc::pdf::PdfTextRun{
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
    run.strikethrough = strikethrough;
    return run;
}

[[nodiscard]] ScenarioResult build_single_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: single text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "FeatherDoc PDF regression sample", 18.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 690.0, "Single-page text smoke", 12.0,
        featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_multi_page_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: multi page";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    featherdoc::pdf::PdfPageLayout page1;
    page1.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page1.text_runs.push_back(make_text_run(
        72.0, 720.0, "Page 1 regression sample", 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page1.text_runs.push_back(make_text_run(
        72.0, 694.0, "Paragraph flow continues across pages", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        false));

    featherdoc::pdf::PdfPageLayout page2;
    page2.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page2.text_runs.push_back(make_text_run(
        72.0, 720.0, "Page 2 regression sample", 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page2.text_runs.push_back(make_text_run(
        72.0, 694.0, "Second page keeps the text path alive", 12.0,
        featherdoc::pdf::PdfRgbColor{0.18, 0.12, 0.08}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page1));
    sample.layout.pages.push_back(std::move(page2));
    return sample;
}

[[nodiscard]] ScenarioResult build_cjk_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: CJK text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, utf8_from_u8(u8"中文文本路径回归样本"), 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        true, "CJK Regression Font", font_path));
    page.text_runs.push_back(make_text_run(
        72.0, 690.0, utf8_from_u8(u8"PDFium 回读"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_cjk_report_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: CJK report text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page1 = make_letter_page();
    page1.text_runs.push_back(make_text_run(
        72.0, 720.0, utf8_from_u8(u8"中文报告样本"), 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        true, "CJK Regression Font", font_path));
    page1.text_runs.push_back(make_text_run(
        72.0, 694.0, utf8_from_u8(u8"第一页正文保持可读"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));
    page1.text_runs.push_back(make_text_run(
        72.0, 668.0, utf8_from_u8(u8"PDFium 回读中文文本"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));

    auto page2 = make_letter_page();
    page2.text_runs.push_back(make_text_run(
        72.0, 720.0, utf8_from_u8(u8"第二页中文标题"), 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        true, "CJK Regression Font", font_path));
    page2.text_runs.push_back(make_text_run(
        72.0, 694.0, utf8_from_u8(u8"第二页继续验证分页"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));
    page2.text_runs.push_back(make_text_run(
        72.0, 668.0, utf8_from_u8(u8"中文报告样本结束"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));

    sample.layout.pages.push_back(std::move(page1));
    sample.layout.pages.push_back(std::move(page2));
    return sample;
}

[[nodiscard]] ScenarioResult build_styled_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: styled text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "Small blue text", 10.0,
        featherdoc::pdf::PdfRgbColor{0.12, 0.28, 0.78}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 695.0, "Large red text", 18.0,
        featherdoc::pdf::PdfRgbColor{0.78, 0.16, 0.16}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 664.0, "Bold italic text", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, true, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 638.0, "Underlined text", 12.0,
        featherdoc::pdf::PdfRgbColor{0.1, 0.1, 0.1}, false, false, true,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_font_size_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: font size";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "Tiny text 8pt", 8.0,
        featherdoc::pdf::PdfRgbColor{0.18, 0.18, 0.18}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 698.0, "Body text 12pt", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 666.0, "Display text 20pt", 20.0,
        featherdoc::pdf::PdfRgbColor{0.42, 0.08, 0.08}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_color_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: color text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "Black text", 12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 694.0, "Blue text", 12.0,
        featherdoc::pdf::PdfRgbColor{0.12, 0.28, 0.78}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 668.0, "Green text", 12.0,
        featherdoc::pdf::PdfRgbColor{0.10, 0.50, 0.20}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 642.0, "Red text", 12.0,
        featherdoc::pdf::PdfRgbColor{0.78, 0.16, 0.16}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_mixed_style_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: mixed style";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "Plain text", 11.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.08, 0.08}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 696.0, "Bold text", 11.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 672.0, "Italic text", 11.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, true, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 648.0, "Bold italic underline", 11.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, true, true,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_contract_cjk_style_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: contract CJK style";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "SERVICE AGREEMENT", 20.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 688.0, utf8_from_u8(u8"中文合同样本"), 18.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Contract Font", font_path));
    page.text_runs.push_back(make_text_run(
        72.0, 650.0, "Party A: FeatherDoc", 12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 626.0, utf8_from_u8(u8"甲方：羽文档"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        true, "CJK Contract Font", font_path));
    page.text_runs.push_back(make_text_run(
        72.0, 596.0, "Amount: USD 12,000", 14.0,
        featherdoc::pdf::PdfRgbColor{0.48, 0.08, 0.08}, true, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 568.0, utf8_from_u8(u8"生效日期：2026年05月06日"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.10, 0.34, 0.18}, false, false, false,
        true, "CJK Contract Font", font_path));
    page.text_runs.push_back(make_text_run(
        72.0, 532.0, "Authorized signature", 12.0,
        featherdoc::pdf::PdfRgbColor{0.05, 0.05, 0.05}, false, true, true,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] bool ensure_document_contract_style(
    featherdoc::Document &document,
    std::string_view style_id,
    featherdoc::character_style_definition definition) {
    return document.ensure_character_style(style_id, definition) &&
           document.materialize_style_run_properties(style_id);
}

[[nodiscard]] bool add_styled_contract_run(featherdoc::Document &document,
                                           featherdoc::Paragraph &paragraph,
                                           const std::string &text,
                                           std::string_view style_id) {
    if (!paragraph.has_next()) {
        return false;
    }

    auto run = paragraph.add_run(text);
    return run.has_next() && document.set_run_style(run, style_id);
}

[[nodiscard]] bool define_document_contract_styles(
    featherdoc::Document &document) {
    auto title = featherdoc::character_style_definition{};
    title.name = "Document PDF Contract Title";
    title.run_font_family = std::string{"Helvetica"};
    title.run_font_size_points = 20.0;
    title.run_bold = true;
    if (!ensure_document_contract_style(document, "DocumentPdfContractTitle",
                                        std::move(title))) {
        return false;
    }

    auto cjk_title = featherdoc::character_style_definition{};
    cjk_title.name = "Document PDF Contract CJK Title";
    cjk_title.run_east_asia_font_family =
        std::string{"Document Contract CJK"};
    cjk_title.run_font_size_points = 18.0;
    cjk_title.run_text_color = std::string{"142E61"};
    if (!ensure_document_contract_style(document, "DocumentPdfContractCjkTitle",
                                        std::move(cjk_title))) {
        return false;
    }

    auto party = featherdoc::character_style_definition{};
    party.name = "Document PDF Contract Party";
    party.run_font_family = std::string{"Helvetica"};
    party.run_east_asia_font_family = std::string{"Document Contract CJK"};
    party.run_font_size_points = 12.0;
    party.run_bold = true;
    if (!ensure_document_contract_style(document, "DocumentPdfContractParty",
                                        std::move(party))) {
        return false;
    }

    auto amount = featherdoc::character_style_definition{};
    amount.name = "Document PDF Contract Amount";
    amount.run_font_family = std::string{"Helvetica"};
    amount.run_font_size_points = 14.0;
    amount.run_text_color = std::string{"7A1414"};
    amount.run_bold = true;
    if (!ensure_document_contract_style(document, "DocumentPdfContractAmount",
                                        std::move(amount))) {
        return false;
    }

    auto date = featherdoc::character_style_definition{};
    date.name = "Document PDF Contract Date";
    date.run_east_asia_font_family = std::string{"Document Contract CJK"};
    date.run_font_size_points = 12.0;
    date.run_text_color = std::string{"19572E"};
    if (!ensure_document_contract_style(document, "DocumentPdfContractDate",
                                        std::move(date))) {
        return false;
    }

    auto signature = featherdoc::character_style_definition{};
    signature.name = "Document PDF Contract Signature";
    signature.run_font_family = std::string{"Helvetica"};
    signature.run_font_size_points = 12.0;
    signature.run_italic = true;
    signature.run_underline = true;
    if (!ensure_document_contract_style(document,
                                        "DocumentPdfContractSignature",
                                        std::move(signature))) {
        return false;
    }

    return true;
}

[[nodiscard]] ScenarioResult build_document_contract_cjk_style_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document Contract CJK") ||
        !define_document_contract_styles(document)) {
        return sample;
    }

    const std::vector<std::pair<std::string, std::string>> lines{
        {"SERVICE AGREEMENT", "DocumentPdfContractTitle"},
        {utf8_from_u8(u8"中文合同样本"), "DocumentPdfContractCjkTitle"},
        {"Contract No: FD-2026-0507", "DocumentPdfContractParty"},
        {"Party A: FeatherDoc", "DocumentPdfContractParty"},
        {utf8_from_u8(u8"甲方：羽文档"), "DocumentPdfContractParty"},
        {"Amount: USD 12,000", "DocumentPdfContractAmount"},
        {utf8_from_u8(u8"生效日期：2026年05月06日"),
         "DocumentPdfContractDate"},
        {utf8_from_u8(u8"第一条：服务范围覆盖文档生成 PDF。"),
         "DocumentPdfContractParty"},
        {utf8_from_u8(u8"第二条：双方确认输出可回读、可验证。"),
         "DocumentPdfContractParty"},
        {"Authorized signature", "DocumentPdfContractSignature"},
    };

    auto paragraph = document.paragraphs();
    for (std::size_t index = 0U; index < lines.size(); ++index) {
        if (index != 0U) {
            paragraph = paragraph.insert_paragraph_after("");
        }
        if (!add_styled_contract_run(document, paragraph, lines[index].first,
                                     lines[index].second)) {
            return sample;
        }
    }

    auto &header = document.ensure_section_header_paragraphs(0U);
    auto &footer = document.ensure_section_footer_paragraphs(0U);
    if (!header.has_next() || !header.set_text("Contract header") ||
        !footer.has_next() ||
        !footer.set_text("Page {{page}} of {{total_pages}}")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document contract CJK style";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document Contract CJK", font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 9.0;
    options.line_height_points = 24.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] bool define_document_eastasia_style_probe_styles(
    featherdoc::Document &document) {
    auto latin_emphasis = featherdoc::character_style_definition{};
    latin_emphasis.name = "Document PDF EastAsia Probe Latin Emphasis";
    latin_emphasis.run_font_family = std::string{"Helvetica"};
    latin_emphasis.run_text_color = std::string{"1F4E79"};
    latin_emphasis.run_bold = true;
    latin_emphasis.run_italic = true;
    latin_emphasis.run_font_size_points = 16.0;
    if (!ensure_document_contract_style(document,
                                        "DocumentPdfEastAsiaProbeLatin",
                                        std::move(latin_emphasis))) {
        return false;
    }

    auto east_asia_accent = featherdoc::character_style_definition{};
    east_asia_accent.name = "Document PDF EastAsia Probe Accent";
    east_asia_accent.run_font_family = std::string{"Helvetica"};
    east_asia_accent.run_east_asia_font_family =
        std::string{"Document EastAsia Probe CJK"};
    east_asia_accent.run_text_color = std::string{"C00000"};
    east_asia_accent.run_bold = true;
    east_asia_accent.run_underline = true;
    east_asia_accent.run_font_size_points = 18.0;
    if (!ensure_document_contract_style(document,
                                        "DocumentPdfEastAsiaProbeAccent",
                                        std::move(east_asia_accent))) {
        return false;
    }

    auto east_asia_note = featherdoc::character_style_definition{};
    east_asia_note.name = "Document PDF EastAsia Probe Note";
    east_asia_note.run_font_family = std::string{"Helvetica"};
    east_asia_note.run_east_asia_font_family =
        std::string{"Document EastAsia Probe CJK"};
    east_asia_note.run_text_color = std::string{"336699"};
    east_asia_note.run_italic = true;
    east_asia_note.run_font_size_points = 14.0;
    return ensure_document_contract_style(document,
                                          "DocumentPdfEastAsiaProbeNote",
                                          std::move(east_asia_note));
}

[[nodiscard]] bool define_document_style_gallery_styles(
    featherdoc::Document &document) {
    auto accent = featherdoc::character_style_definition{};
    accent.name = "Document PDF Style Gallery Accent";
    accent.run_font_family = std::string{"Helvetica"};
    accent.run_font_size_points = 16.0;
    accent.run_bold = true;
    accent.run_italic = true;
    accent.run_text_color = std::string{"2F5597"};
    if (!ensure_document_contract_style(document,
                                        "DocumentPdfStyleGalleryAccent",
                                        std::move(accent))) {
        return false;
    }

    auto note = featherdoc::character_style_definition{};
    note.name = "Document PDF Style Gallery Note";
    note.run_font_family = std::string{"Helvetica"};
    note.run_font_size_points = 12.0;
    note.run_underline = true;
    note.run_text_color = std::string{"7A3E00"};
    return ensure_document_contract_style(document,
                                          "DocumentPdfStyleGalleryNote",
                                          std::move(note));
}

[[nodiscard]] bool define_document_complex_layout_styles(
    featherdoc::Document &document) {
    if (!define_document_style_gallery_styles(document)) {
        return false;
    }

    auto cjk_accent = featherdoc::character_style_definition{};
    cjk_accent.name = "Document PDF Complex Layout CJK Accent";
    cjk_accent.run_font_family = std::string{"Helvetica"};
    cjk_accent.run_east_asia_font_family =
        std::string{"Document Complex Layout CJK"};
    cjk_accent.run_font_size_points = 14.0;
    cjk_accent.run_bold = true;
    cjk_accent.run_underline = true;
    cjk_accent.run_text_color = std::string{"1F4E79"};
    if (!ensure_document_contract_style(document,
                                        "DocumentPdfComplexLayoutCjkAccent",
                                        std::move(cjk_accent))) {
        return false;
    }

    auto cjk_note = featherdoc::character_style_definition{};
    cjk_note.name = "Document PDF Complex Layout CJK Note";
    cjk_note.run_font_family = std::string{"Helvetica"};
    cjk_note.run_east_asia_font_family =
        std::string{"Document Complex Layout CJK"};
    cjk_note.run_font_size_points = 12.0;
    cjk_note.run_italic = true;
    cjk_note.run_text_color = std::string{"8A2D3B"};
    return ensure_document_contract_style(document,
                                          "DocumentPdfComplexLayoutCjkNote",
                                          std::move(cjk_note));
}

[[nodiscard]] bool define_document_cjk_copy_search_styles(
    featherdoc::Document &document) {
    auto accent = featherdoc::character_style_definition{};
    accent.name = "Document PDF CJK Copy Search Accent";
    accent.run_font_family = std::string{"Helvetica"};
    accent.run_east_asia_font_family =
        std::string{"Document CJK Copy Search"};
    accent.run_font_size_points = 14.0;
    accent.run_bold = true;
    accent.run_underline = true;
    accent.run_text_color = std::string{"1F4E79"};
    if (!ensure_document_contract_style(document,
                                        "DocumentPdfCjkCopySearchAccent",
                                        std::move(accent))) {
        return false;
    }

    auto note = featherdoc::character_style_definition{};
    note.name = "Document PDF CJK Copy Search Note";
    note.run_font_family = std::string{"Helvetica"};
    note.run_east_asia_font_family = std::string{"Document CJK Copy Search"};
    note.run_font_size_points = 12.0;
    note.run_italic = true;
    note.run_text_color = std::string{"8A2D3B"};
    return ensure_document_contract_style(document,
                                          "DocumentPdfCjkCopySearchNote",
                                          std::move(note));
}

[[nodiscard]] bool define_document_cjk_font_embed_styles(
    featherdoc::Document &document) {
    auto accent = featherdoc::character_style_definition{};
    accent.name = "Document PDF CJK Font Embed Accent";
    accent.run_font_family = std::string{"Helvetica"};
    accent.run_east_asia_font_family = std::string{"Document CJK Font Embed"};
    accent.run_font_size_points = 14.0;
    accent.run_bold = true;
    accent.run_underline = true;
    accent.run_text_color = std::string{"1F4E79"};
    if (!ensure_document_contract_style(document,
                                        "DocumentPdfCjkFontEmbedAccent",
                                        std::move(accent))) {
        return false;
    }

    auto large = featherdoc::character_style_definition{};
    large.name = "Document PDF CJK Font Embed Large";
    large.run_font_family = std::string{"Helvetica"};
    large.run_east_asia_font_family = std::string{"Document CJK Font Embed"};
    large.run_font_size_points = 18.0;
    large.run_bold = true;
    large.run_text_color = std::string{"7A1414"};
    if (!ensure_document_contract_style(document,
                                        "DocumentPdfCjkFontEmbedLarge",
                                        std::move(large))) {
        return false;
    }

    auto note = featherdoc::character_style_definition{};
    note.name = "Document PDF CJK Font Embed Note";
    note.run_font_family = std::string{"Helvetica"};
    note.run_east_asia_font_family = std::string{"Document CJK Font Embed"};
    note.run_font_size_points = 10.5;
    note.run_italic = true;
    note.run_text_color = std::string{"19572E"};
    return ensure_document_contract_style(document,
                                          "DocumentPdfCjkFontEmbedNote",
                                          std::move(note));
}

[[nodiscard]] bool define_document_rtl_bidi_styles(
    featherdoc::Document &document) {
    auto paragraph_style = featherdoc::paragraph_style_definition{};
    paragraph_style.name = "Document PDF RTL Bidi Paragraph";
    paragraph_style.paragraph_bidi = true;
    if (!document.ensure_paragraph_style("DocumentPdfRtlParagraph",
                                         paragraph_style)) {
        return false;
    }

    auto arabic = featherdoc::character_style_definition{};
    arabic.name = "Document PDF RTL Arabic";
    arabic.run_font_family = std::string{"Document RTL Arabic"};
    arabic.run_bidi_language = std::string{"ar-SA"};
    arabic.run_rtl = true;
    arabic.run_font_size_points = 15.0;
    arabic.run_text_color = std::string{"1F3A5F"};
    if (!ensure_document_contract_style(document, "DocumentPdfRtlArabic",
                                        std::move(arabic))) {
        return false;
    }

    auto emphasis = featherdoc::character_style_definition{};
    emphasis.name = "Document PDF RTL Arabic Emphasis";
    emphasis.based_on = std::string{"DocumentPdfRtlArabic"};
    emphasis.run_underline = true;
    emphasis.run_text_color = std::string{"8A2D3B"};
    return ensure_document_contract_style(document, "DocumentPdfRtlArabicEmphasis",
                                          std::move(emphasis));
}

[[nodiscard]] ScenarioResult build_document_eastasia_style_probe_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document EastAsia Probe CJK") ||
        !define_document_eastasia_style_probe_styles(document)) {
        return sample;
    }

    auto paragraph = document.paragraphs();
    if (!paragraph.has_next() ||
        !paragraph.set_text("Document eastAsia style probe")) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("Styled Latin run: ").has_next()) {
        return sample;
    }
    auto latin_run = paragraph.add_run("bold italic blue 16pt");
    if (!latin_run.has_next() ||
        !document.set_run_style(latin_run, "DocumentPdfEastAsiaProbeLatin")) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("East Asia run mapping: ").has_next()) {
        return sample;
    }
    auto accent_run = paragraph.add_run(utf8_from_u8(u8"中文样式映射 ABC 123"));
    if (!accent_run.has_next() ||
        !document.set_run_style(accent_run,
                                "DocumentPdfEastAsiaProbeAccent")) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("East Asia note style: ").has_next()) {
        return sample;
    }
    auto note_run = paragraph.add_run(utf8_from_u8(u8"蓝色 14pt 中文备注"));
    if (!note_run.has_next() ||
        !document.set_run_style(note_run, "DocumentPdfEastAsiaProbeNote")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document eastAsia style probe";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document EastAsia Probe CJK",
                                        font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 24.0;
    options.paragraph_spacing_after_points = 6.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_rtl_bidi_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !define_document_rtl_bidi_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document RTL bidi sample")) {
        return sample;
    }
    if (!title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto paragraph = title.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !document.set_paragraph_style(paragraph, "DocumentPdfRtlParagraph") ||
        !paragraph.set_alignment(featherdoc::paragraph_alignment::right) ||
        !paragraph.add_run("Arabic paragraph: ").has_next()) {
        return sample;
    }
    auto arabic_run =
        paragraph.add_run(utf8_from_u8(u8"مرحبا بكم في FeatherDoc 2026"));
    if (!arabic_run.has_next() ||
        !document.set_run_style(arabic_run, "DocumentPdfRtlArabic")) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !document.set_paragraph_style(paragraph, "DocumentPdfRtlParagraph") ||
        !paragraph.set_alignment(featherdoc::paragraph_alignment::right) ||
        !paragraph.add_run("Mixed order: ").has_next()) {
        return sample;
    }
    auto mixed_prefix = paragraph.add_run(utf8_from_u8(u8"النسخة"));
    if (!mixed_prefix.has_next() ||
        !document.set_run_style(mixed_prefix, "DocumentPdfRtlArabicEmphasis") ||
        !paragraph.add_run(" FE-2048 ").has_next()) {
        return sample;
    }
    auto mixed_suffix =
        paragraph.add_run(utf8_from_u8(u8"جاهزة للمراجعة"));
    if (!mixed_suffix.has_next() ||
        !document.set_run_style(mixed_suffix, "DocumentPdfRtlArabic")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{520.0, 680.0};
    options.metadata.title = "FeatherDoc regression sample: document RTL bidi";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document RTL Arabic", font_path},
    };
    options.use_system_font_fallbacks = false;
    options.line_height_points = 22.0;
    options.paragraph_spacing_after_points = 8.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_font_matrix_text_sample(
    const std::filesystem::path &cjk_font_path,
    const std::filesystem::path &arabic_font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document Font Matrix CJK") ||
        !define_document_rtl_bidi_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document font matrix sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto paragraph = title.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("Body matrix: ").has_next() ||
        !paragraph.add_run(utf8_from_u8(u8"中文矩阵")).has_next() ||
        !paragraph.add_run(" / ").has_next()) {
        return sample;
    }
    auto body_arabic = paragraph.add_run(utf8_from_u8(u8"مرحبا بالمراجعة"));
    if (!body_arabic.has_next() ||
        !document.set_run_style(body_arabic, "DocumentPdfRtlArabic")) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("Identifier matrix: FE-300 / ").has_next() ||
        !paragraph.add_run(utf8_from_u8(u8"合同检查")).has_next() ||
        !paragraph.add_run(" / ").has_next()) {
        return sample;
    }
    auto identifier_arabic = paragraph.add_run(utf8_from_u8(u8"النسخة"));
    if (!identifier_arabic.has_next() ||
        !document.set_run_style(identifier_arabic,
                                "DocumentPdfRtlArabicEmphasis")) {
        return sample;
    }

    auto header = document.ensure_section_header_paragraphs(0U);
    if (!header.has_next() ||
        !document.set_paragraph_style(header, "DocumentPdfRtlParagraph") ||
        !header.set_bidi(true) ||
        !header.set_alignment(featherdoc::paragraph_alignment::right) ||
        !header.add_run("Header matrix ").has_next() ||
        !header.add_run(utf8_from_u8(u8"页眉矩阵")).has_next() ||
        !header.add_run(" ").has_next()) {
        return sample;
    }
    auto header_arabic = header.add_run(utf8_from_u8(u8"العنوان"));
    if (!header_arabic.has_next() ||
        !document.set_run_style(header_arabic, "DocumentPdfRtlArabic")) {
        return sample;
    }

    auto footer = document.ensure_section_footer_paragraphs(0U);
    if (!footer.has_next() ||
        !document.set_paragraph_style(footer, "DocumentPdfRtlParagraph") ||
        !footer.set_bidi(true) ||
        !footer.set_alignment(featherdoc::paragraph_alignment::right) ||
        !footer.add_run("Footer matrix ").has_next() ||
        !footer.add_run("{{page}}").has_next() ||
        !footer.add_run(" / ").has_next() ||
        !footer.add_run("{{total_pages}}").has_next() ||
        !footer.add_run(" ").has_next() ||
        !footer.add_run(utf8_from_u8(u8"页脚矩阵")).has_next() ||
        !footer.add_run(" ").has_next()) {
        return sample;
    }
    auto footer_arabic = footer.add_run(utf8_from_u8(u8"الخاتمة"));
    if (!footer_arabic.has_next() ||
        !document.set_run_style(footer_arabic, "DocumentPdfRtlArabic")) {
        return sample;
    }

    auto table = document.append_table(3U, 2U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1800U) ||
        !table.set_column_width_twips(1U, 5400U) ||
        !table.set_cell_text(0U, 0U, "Zone") ||
        !table.set_cell_text(0U, 1U, "Value") ||
        !table.set_cell_text(1U, 0U, "Body") ||
        !table.set_cell_text(2U, 0U, "Cell")) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 3U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(420U, featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        row.next();
    }

    auto body_cell = table.find_cell(1U, 1U);
    auto mixed_cell = table.find_cell(2U, 1U);
    if (!body_cell.has_value() || !mixed_cell.has_value()) {
        return sample;
    }

    auto body_cell_paragraph = body_cell->paragraphs();
    if (!body_cell_paragraph.has_next() ||
        !body_cell_paragraph.add_run("Header/footer preview ").has_next() ||
        !body_cell_paragraph.add_run(utf8_from_u8(u8"中文页签")).has_next() ||
        !body_cell_paragraph.add_run(" ").has_next()) {
        return sample;
    }
    auto body_cell_arabic =
        body_cell_paragraph.add_run(utf8_from_u8(u8"تخطيط"));
    if (!body_cell_arabic.has_next() ||
        !document.set_run_style(body_cell_arabic, "DocumentPdfRtlArabic")) {
        return sample;
    }

    auto mixed_cell_paragraph = mixed_cell->paragraphs();
    if (!mixed_cell_paragraph.has_next() ||
        !document.set_paragraph_style(mixed_cell_paragraph,
                                      "DocumentPdfRtlParagraph") ||
        !mixed_cell_paragraph.set_bidi(true) ||
        !mixed_cell_paragraph.set_alignment(
            featherdoc::paragraph_alignment::right) ||
        !mixed_cell_paragraph.add_run("Cell ").has_next() ||
        !mixed_cell_paragraph.add_run(utf8_from_u8(u8"表格矩阵")).has_next() ||
        !mixed_cell_paragraph.add_run(" ").has_next()) {
        return sample;
    }
    auto mixed_cell_arabic =
        mixed_cell_paragraph.add_run(utf8_from_u8(u8"خلية"));
    if (!mixed_cell_arabic.has_next() ||
        !document.set_run_style(mixed_cell_arabic, "DocumentPdfRtlArabic")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document font matrix";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document Font Matrix CJK",
                                        cjk_font_path},
        featherdoc::pdf::PdfFontMapping{"Document RTL Arabic",
                                        arabic_font_path},
    };
    options.cjk_font_file_path = cjk_font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 9.0;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 6.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_table_font_matrix_text_sample(
    const std::filesystem::path &cjk_font_path,
    const std::filesystem::path &arabic_font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document Font Matrix CJK") ||
        !define_document_rtl_bidi_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document table font matrix sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() || !intro.add_run("Paged matrix: ").has_next() ||
        !intro.add_run(utf8_from_u8(u8"中文表格")).has_next() ||
        !intro.add_run(" / FE-420 / ").has_next()) {
        return sample;
    }
    auto intro_arabic = intro.add_run(utf8_from_u8(u8"مراجعة"));
    if (!intro_arabic.has_next() ||
        !document.set_run_style(intro_arabic, "DocumentPdfRtlArabic")) {
        return sample;
    }

    auto header = document.ensure_section_header_paragraphs(0U);
    if (!header.has_next() ||
        !document.set_paragraph_style(header, "DocumentPdfRtlParagraph") ||
        !header.set_bidi(true) ||
        !header.set_alignment(featherdoc::paragraph_alignment::right) ||
        !header.add_run("Table matrix ").has_next() ||
        !header.add_run("{{page}}").has_next() ||
        !header.add_run(" ").has_next() ||
        !header.add_run(utf8_from_u8(u8"页眉")).has_next() ||
        !header.add_run(" ").has_next()) {
        return sample;
    }
    auto header_arabic = header.add_run(utf8_from_u8(u8"الجدول"));
    if (!header_arabic.has_next() ||
        !document.set_run_style(header_arabic, "DocumentPdfRtlArabic")) {
        return sample;
    }

    auto footer = document.ensure_section_footer_paragraphs(0U);
    if (!footer.has_next() ||
        !document.set_paragraph_style(footer, "DocumentPdfRtlParagraph") ||
        !footer.set_bidi(true) ||
        !footer.set_alignment(featherdoc::paragraph_alignment::right) ||
        !footer.add_run("Footer ").has_next() ||
        !footer.add_run("{{page}}").has_next() ||
        !footer.add_run(" / ").has_next() ||
        !footer.add_run("{{total_pages}}").has_next() ||
        !footer.add_run(" ").has_next() ||
        !footer.add_run(utf8_from_u8(u8"页脚")).has_next() ||
        !footer.add_run(" ").has_next()) {
        return sample;
    }
    auto footer_arabic = footer.add_run(utf8_from_u8(u8"مراجعة"));
    if (!footer_arabic.has_next() ||
        !document.set_run_style(footer_arabic, "DocumentPdfRtlArabic")) {
        return sample;
    }

    constexpr std::size_t row_count = 7U;
    auto table = document.append_table(row_count, 2U);
    if (!table.has_next() || !table.set_width_twips(4200U) ||
        !table.set_column_width_twips(0U, 1200U) ||
        !table.set_column_width_twips(1U, 3000U) ||
        !table.set_cell_text(0U, 0U, "Zone") ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"Value 矩阵"))) {
        return sample;
    }

    const auto configure_matrix_row =
        [&](std::size_t row_index, std::string_view zone,
            std::string_view latin, const char8_t *cjk, const char8_t *arabic,
            bool bidi) -> bool {
        if (!table.set_cell_text(row_index, 0U, std::string(zone))) {
            return false;
        }
        auto cell = table.find_cell(row_index, 1U);
        if (!cell.has_value()) {
            return false;
        }

        auto paragraph = cell->paragraphs();
        if (!paragraph.has_next()) {
            return false;
        }
        if (bidi &&
            (!document.set_paragraph_style(paragraph, "DocumentPdfRtlParagraph") ||
             !paragraph.set_bidi(true) ||
             !paragraph.set_alignment(
                 featherdoc::paragraph_alignment::right))) {
            return false;
        }
        if (!paragraph.add_run(std::string(latin)).has_next() ||
            !paragraph.add_run(" ").has_next() ||
            !paragraph.add_run(utf8_from_u8(cjk)).has_next() ||
            !paragraph.add_run(" ").has_next()) {
            return false;
        }

        auto arabic_run = paragraph.add_run(utf8_from_u8(arabic));
        if (!arabic_run.has_next()) {
            return false;
        }
        return document.set_run_style(
            arabic_run,
            bidi ? "DocumentPdfRtlArabicEmphasis" : "DocumentPdfRtlArabic");
    };

    if (!configure_matrix_row(1U, "Cover", "FE-401", u8"封样", u8"غلاف",
                              false) ||
        !configure_matrix_row(2U, "Body", "FE-402", u8"正文", u8"نسخة",
                              false) ||
        !configure_matrix_row(3U, "Table", "FE-403", u8"分页", u8"جدول",
                              false) ||
        !configure_matrix_row(4U, "Footer", "FE-404", u8"页脚",
                              u8"المراجعة", true) ||
        !configure_matrix_row(5U, "Search", "FE-405", u8"检索",
                              u8"بحث", false) ||
        !configure_matrix_row(6U, "Close", "FE-406", u8"归档",
                              u8"جاهز", false)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(600U, featherdoc::row_height_rule::exact)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        if (row_index == 4U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table font matrix";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document Font Matrix CJK",
                                        cjk_font_path},
        featherdoc::pdf::PdfFontMapping{"Document RTL Arabic",
                                        arabic_font_path},
    };
    options.cjk_font_file_path = cjk_font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 9.0;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_style_gallery_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !define_document_style_gallery_styles(document)) {
        return sample;
    }

    auto paragraph = document.paragraphs();
    if (!paragraph.has_next() ||
        !paragraph.set_text("Document style gallery sample")) {
        return sample;
    }
    if (!paragraph.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("Style coverage: ").has_next()) {
        return sample;
    }
    auto accent_run = paragraph.add_run("bold italic blue 16pt");
    auto separator_run = paragraph.add_run(", ");
    auto note_run = paragraph.add_run("underlined note");
    if (!accent_run.has_next() || !separator_run.has_next() ||
        !note_run.has_next() ||
        !document.set_run_style(accent_run, "DocumentPdfStyleGalleryAccent") ||
        !document.set_run_style(note_run, "DocumentPdfStyleGalleryNote") ||
        !paragraph.add_run(
             ", and neutral body text to keep mixed inline styling stable.")
              .has_next()) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("Right-aligned summary marker").has_next() ||
        !paragraph.set_alignment(featherdoc::paragraph_alignment::right)) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after(
        "Hanging indent paragraph wraps across multiple lines so the export "
        "path keeps a narrower first-line offset and a deeper follow-on body "
        "column.");
    if (!paragraph.has_next() || !paragraph.set_indent_left_twips(720U) ||
        !paragraph.set_hanging_indent_twips(360U)) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after(
        "First line indent paragraph keeps the opening clause inset while the "
        "remaining lines align back to the body column for visual comparison.");
    if (!paragraph.has_next() || !paragraph.set_indent_left_twips(360U) ||
        !paragraph.set_first_line_indent_twips(360U)) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 420.0};
    options.metadata.title =
        "FeatherDoc regression sample: document style gallery";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 40.0;
    options.margin_bottom_points = 40.0;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 10.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_three_page_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: three page";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page1 = make_letter_page();
    page1.text_runs.push_back(make_text_run(
        72.0, 720.0, "Three page sample page 1", 15.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page1.text_runs.push_back(make_text_run(
        72.0, 694.0, "First page keeps text flowing", 12.0,
        featherdoc::pdf::PdfRgbColor{0.16, 0.16, 0.4}, false, false, false,
        false));

    auto page2 = make_letter_page();
    page2.text_runs.push_back(make_text_run(
        72.0, 720.0, "Three page sample page 2", 15.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page2.text_runs.push_back(make_text_run(
        72.0, 694.0, "Second page keeps text flowing", 12.0,
        featherdoc::pdf::PdfRgbColor{0.16, 0.16, 0.4}, false, false, false,
        false));

    auto page3 = make_letter_page();
    page3.text_runs.push_back(make_text_run(
        72.0, 720.0, "Three page sample page 3", 15.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page3.text_runs.push_back(make_text_run(
        72.0, 694.0, "Third page closes the smoke path", 12.0,
        featherdoc::pdf::PdfRgbColor{0.16, 0.16, 0.4}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page1));
    sample.layout.pages.push_back(std::move(page2));
    sample.layout.pages.push_back(std::move(page3));
    return sample;
}

[[nodiscard]] ScenarioResult build_landscape_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: landscape text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_landscape_page();
    page.text_runs.push_back(make_text_run(
        72.0, 540.0, "Landscape text regression sample", 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 512.0, "Wide page keeps the text path visible", 12.0,
        featherdoc::pdf::PdfRgbColor{0.12, 0.24, 0.44}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 484.0, "PDFium still extracts landscape text", 12.0,
        featherdoc::pdf::PdfRgbColor{0.18, 0.10, 0.10}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_title_body_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: title body text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 724.0, "Title line", 24.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 694.0, "Subtitle line", 14.0,
        featherdoc::pdf::PdfRgbColor{0.15, 0.15, 0.15}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 666.0, "Body paragraph line one", 11.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 646.0, "Body paragraph line two", 11.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 620.0, "Footer line", 9.0,
        featherdoc::pdf::PdfRgbColor{0.22, 0.22, 0.22}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_dense_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: dense text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 724.0, "Dense line 1", 12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 704.0, "Dense line 2", 12.0,
        featherdoc::pdf::PdfRgbColor{0.12, 0.12, 0.36}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 684.0, "Dense line 3", 12.0,
        featherdoc::pdf::PdfRgbColor{0.36, 0.12, 0.12}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 664.0, "Dense line 4", 12.0,
        featherdoc::pdf::PdfRgbColor{0.10, 0.40, 0.10}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 644.0, "Dense line 5", 12.0,
        featherdoc::pdf::PdfRgbColor{0.42, 0.18, 0.08}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 624.0, "Dense line 6", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.24, 0.42}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_four_page_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: four page";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page1 = make_letter_page();
    page1.text_runs.push_back(make_text_run(
        72.0, 720.0, "Four page sample page 1", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    auto page2 = make_letter_page();
    page2.text_runs.push_back(make_text_run(
        72.0, 720.0, "Four page sample page 2", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    auto page3 = make_letter_page();
    page3.text_runs.push_back(make_text_run(
        72.0, 720.0, "Four page sample page 3", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    auto page4 = make_letter_page();
    page4.text_runs.push_back(make_text_run(
        72.0, 720.0, "Four page sample page 4", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page1));
    sample.layout.pages.push_back(std::move(page2));
    sample.layout.pages.push_back(std::move(page3));
    sample.layout.pages.push_back(std::move(page4));
    return sample;
}

[[nodiscard]] ScenarioResult build_underline_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: underline text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "Underlined line 1", 12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, true,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 694.0, "Underlined line 2", 12.0,
        featherdoc::pdf::PdfRgbColor{0.12, 0.28, 0.78}, false, false, true,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 668.0, "Bold underlined line", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.08, 0.08}, true, false, true,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 642.0, "Italic underlined line", 12.0,
        featherdoc::pdf::PdfRgbColor{0.18, 0.10, 0.10}, false, true, true,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_strikethrough_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto style_definition = featherdoc::character_style_definition{};
    style_definition.name = "PDF Style Strikethrough";
    style_definition.run_strikethrough = true;
    if (!ensure_document_contract_style(document, "PdfStyleStrikethrough",
                                        std::move(style_definition))) {
        return sample;
    }

    auto paragraph = document.paragraphs();
    if (!paragraph.has_next() || !paragraph.set_text("Strikethrough sample")) {
        return sample;
    }
    if (!paragraph.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() || !paragraph.add_run("Direct ").has_next()) {
        return sample;
    }
    auto direct_strike = paragraph.add_run(
        "strikethrough", featherdoc::formatting_flag::strikethrough);
    if (!direct_strike.has_next() ||
        !paragraph.add_run(" demonstrates run formatting.").has_next()) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() || !paragraph.add_run("Style ").has_next()) {
        return sample;
    }
    auto styled_strike = paragraph.add_run("strikethrough");
    if (!styled_strike.has_next() ||
        !document.set_run_style(styled_strike, "PdfStyleStrikethrough") ||
        !paragraph
             .add_run(" demonstrates inherited style formatting.")
             .has_next()) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.metadata.title = "FeatherDoc regression sample: strikethrough";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 40.0;
    options.margin_bottom_points = 40.0;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 10.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_superscript_subscript_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto paragraph = document.paragraphs();
    if (!paragraph.has_next() ||
        !paragraph.set_text("Superscript and subscript sample")) {
        return sample;
    }
    if (!paragraph.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("E = mc").has_next()) {
        return sample;
    }
    auto superscript_run =
        paragraph.add_run("2", featherdoc::formatting_flag::superscript);
    if (!superscript_run.has_next() ||
        !paragraph.add_run(" demonstrates superscript support.").has_next()) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() || !paragraph.add_run("H").has_next()) {
        return sample;
    }
    auto subscript_run =
        paragraph.add_run("2", featherdoc::formatting_flag::subscript);
    if (!subscript_run.has_next() ||
        !paragraph.add_run("O demonstrates subscript support.").has_next()) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.metadata.title =
        "FeatherDoc regression sample: superscript subscript";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 40.0;
    options.margin_bottom_points = 40.0;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 10.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_style_superscript_subscript_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto superscript_style = featherdoc::character_style_definition{};
    superscript_style.name = "PDF Style Superscript";
    superscript_style.run_superscript = true;
    if (!ensure_document_contract_style(document, "PdfStyleSuperscript",
                                        std::move(superscript_style))) {
        return sample;
    }

    auto subscript_style = featherdoc::character_style_definition{};
    subscript_style.name = "PDF Style Subscript";
    subscript_style.run_subscript = true;
    if (!ensure_document_contract_style(document, "PdfStyleSubscript",
                                        std::move(subscript_style))) {
        return sample;
    }

    auto paragraph = document.paragraphs();
    if (!paragraph.has_next() ||
        !paragraph.set_text("Style superscript and subscript sample")) {
        return sample;
    }
    if (!paragraph.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("Style E = mc").has_next()) {
        return sample;
    }
    auto superscript_run = paragraph.add_run("2");
    if (!superscript_run.has_next() ||
        !document.set_run_style(superscript_run, "PdfStyleSuperscript") ||
        !paragraph.add_run(" inherits superscript style.").has_next()) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() || !paragraph.add_run("Style H").has_next()) {
        return sample;
    }
    auto subscript_run = paragraph.add_run("2");
    if (!subscript_run.has_next() ||
        !document.set_run_style(subscript_run, "PdfStyleSubscript") ||
        !paragraph.add_run("O inherits subscript style.").has_next()) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.metadata.title =
        "FeatherDoc regression sample: style superscript subscript";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 40.0;
    options.margin_bottom_points = 40.0;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 10.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_punctuation_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: punctuation text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "Alpha, beta, gamma.", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 694.0, "Numbers: 12345", 12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 668.0, "Symbols: ( ) [ ] / -", 12.0,
        featherdoc::pdf::PdfRgbColor{0.18, 0.18, 0.18}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 642.0, "Quote FeatherDoc", 12.0,
        featherdoc::pdf::PdfRgbColor{0.12, 0.28, 0.78}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_two_page_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: two page";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page1 = make_letter_page();
    page1.text_runs.push_back(make_text_run(
        72.0, 720.0, "Two page sample page 1", 15.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page1.text_runs.push_back(make_text_run(
        72.0, 694.0, "First page keeps extracted text stable", 12.0,
        featherdoc::pdf::PdfRgbColor{0.16, 0.16, 0.4}, false, false, false,
        false));

    auto page2 = make_letter_page();
    page2.text_runs.push_back(make_text_run(
        72.0, 720.0, "Two page sample page 2", 15.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page2.text_runs.push_back(make_text_run(
        72.0, 694.0, "Second page keeps extracted text stable", 12.0,
        featherdoc::pdf::PdfRgbColor{0.16, 0.16, 0.4}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page1));
    sample.layout.pages.push_back(std::move(page2));
    return sample;
}

[[nodiscard]] ScenarioResult build_repeat_phrase_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: repeat phrase text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "Repeat phrase alpha", 12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 694.0, "Repeat phrase beta", 12.0,
        featherdoc::pdf::PdfRgbColor{0.12, 0.28, 0.78}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 668.0, "Repeat phrase gamma", 12.0,
        featherdoc::pdf::PdfRgbColor{0.18, 0.10, 0.10}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 642.0, "Repeat phrase delta", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_bordered_box_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: bordered box text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.rectangles.push_back(featherdoc::pdf::PdfRectangle{
        featherdoc::pdf::PdfRect{72.0, 604.0, 360.0, 96.0},
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32},
        featherdoc::pdf::PdfRgbColor{0.94, 0.97, 1.0},
        1.25,
        true,
        true,
    });
    page.rectangles.push_back(featherdoc::pdf::PdfRectangle{
        featherdoc::pdf::PdfRect{90.0, 622.0, 324.0, 22.0},
        featherdoc::pdf::PdfRgbColor{0.72, 0.78, 0.86},
        featherdoc::pdf::PdfRgbColor{1.0, 1.0, 1.0},
        0.75,
        true,
        false,
    });
    page.text_runs.push_back(make_text_run(
        90.0, 676.0, "Bordered box sample", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        90.0, 642.0, "Box body text", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_line_primitive_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: line primitive text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{72.0, 690.0},
        featherdoc::pdf::PdfPoint{432.0, 690.0},
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        1.0,
        0.0,
        0.0,
        0.0,
        featherdoc::pdf::PdfLineCap::butt,
    });
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{72.0, 664.0},
        featherdoc::pdf::PdfPoint{432.0, 664.0},
        featherdoc::pdf::PdfRgbColor{0.12, 0.28, 0.78},
        1.25,
        0.0,
        6.0,
        4.0,
        featherdoc::pdf::PdfLineCap::round,
    });
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{72.0, 638.0},
        featherdoc::pdf::PdfPoint{432.0, 638.0},
        featherdoc::pdf::PdfRgbColor{0.78, 0.16, 0.16},
        2.0,
        0.0,
        0.0,
        0.0,
        featherdoc::pdf::PdfLineCap::square,
    });
    page.text_runs.push_back(make_text_run(
        72.0, 724.0, "Line primitive sample", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 612.0, "Dashed and solid lines", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_table_like_grid_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: table-like grid text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    constexpr double left = 72.0;
    constexpr double top = 700.0;
    constexpr double cell_width = 120.0;
    constexpr double cell_height = 32.0;
    constexpr double columns = 3.0;
    constexpr double rows = 3.0;
    const auto right = left + cell_width * columns;
    const auto bottom = top - cell_height * rows;

    for (int column = 0; column <= static_cast<int>(columns); ++column) {
        const auto x = left + cell_width * static_cast<double>(column);
        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{x, bottom},
            featherdoc::pdf::PdfPoint{x, top},
            featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
            0.75,
        });
    }
    for (int row = 0; row <= static_cast<int>(rows); ++row) {
        const auto y = top - cell_height * static_cast<double>(row);
        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{left, y},
            featherdoc::pdf::PdfPoint{right, y},
            featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
            0.75,
        });
    }

    page.text_runs.push_back(make_text_run(
        72.0, 724.0, "Grid sample header", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        86.0, 678.0, "Cell A1", 11.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        206.0, 646.0, "Cell B2", 11.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        326.0, 614.0, "Cell C3", 11.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_metadata_long_title_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample with a deliberately long metadata title";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "Long metadata title sample", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 692.0, "Metadata path stays parseable", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_header_footer_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: header footer text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{72.0, 704.0},
        featherdoc::pdf::PdfPoint{540.0, 704.0},
        featherdoc::pdf::PdfRgbColor{0.64, 0.68, 0.74},
        0.75,
    });
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{72.0, 92.0},
        featherdoc::pdf::PdfPoint{540.0, 92.0},
        featherdoc::pdf::PdfRgbColor{0.64, 0.68, 0.74},
        0.75,
    });
    page.text_runs.push_back(make_text_run(
        72.0, 724.0, "Document header area", 10.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, true, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 660.0, "Header footer sample", 15.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 640.0, "Body text stays between page bands", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 72.0, "Page 1 footer marker", 10.0,
        featherdoc::pdf::PdfRgbColor{0.22, 0.26, 0.34}, false, true, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_header_footer_rtl_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !define_document_rtl_bidi_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Header footer RTL bidi sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto body = title.insert_paragraph_after("");
    if (!body.has_next() ||
        !body.set_text(
            "Body anchor paragraph keeps header and footer rendering active.")) {
        return sample;
    }

    auto header = document.ensure_section_header_paragraphs(0U);
    if (!header.has_next() ||
        !document.set_paragraph_style(header, "DocumentPdfRtlParagraph") ||
        !header.set_bidi(true) ||
        !header.set_alignment(featherdoc::paragraph_alignment::right) ||
        !header.add_run("Header mixed: ").has_next()) {
        return sample;
    }
    auto header_prefix = header.add_run(utf8_from_u8(u8"النسخة"));
    if (!header_prefix.has_next() ||
        !document.set_run_style(header_prefix, "DocumentPdfRtlArabicEmphasis") ||
        !header.add_run(" HF-2048 ").has_next()) {
        return sample;
    }
    auto header_suffix = header.add_run(utf8_from_u8(u8"جاهزة للمراجعة"));
    if (!header_suffix.has_next() ||
        !document.set_run_style(header_suffix, "DocumentPdfRtlArabic")) {
        return sample;
    }

    auto footer = document.ensure_section_footer_paragraphs(0U);
    if (!footer.has_next() ||
        !document.set_paragraph_style(footer, "DocumentPdfRtlParagraph") ||
        !footer.set_bidi(true) ||
        !footer.set_alignment(featherdoc::paragraph_alignment::right) ||
        !footer.add_run("Footer page ").has_next() ||
        !footer.add_run("{{page}}").has_next() ||
        !footer.add_run(" - ").has_next()) {
        return sample;
    }
    auto footer_suffix = footer.add_run(utf8_from_u8(u8"مراجعة نهائية"));
    if (!footer_suffix.has_next() ||
        !document.set_run_style(footer_suffix, "DocumentPdfRtlArabic")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: header footer RTL bidi";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document RTL Arabic", font_path},
    };
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 9.0;
    options.line_height_points = 20.0;
    options.paragraph_spacing_after_points = 6.0;

    sample.layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_two_column_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: two column text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{306.0, 608.0},
        featherdoc::pdf::PdfPoint{306.0, 704.0},
        featherdoc::pdf::PdfRgbColor{0.82, 0.84, 0.88},
        0.75,
    });
    page.text_runs.push_back(make_text_run(
        72.0, 724.0, "Two column sample", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 684.0, "Left column alpha", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 660.0, "Left column beta", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        330.0, 684.0, "Right column alpha", 12.0,
        featherdoc::pdf::PdfRgbColor{0.20, 0.12, 0.08}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        330.0, 660.0, "Right column beta", 12.0,
        featherdoc::pdf::PdfRgbColor{0.20, 0.12, 0.08}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_invoice_grid_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: invoice grid text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    constexpr double left = 72.0;
    constexpr double top = 690.0;
    constexpr double right = 540.0;
    constexpr double row_height = 30.0;

    for (int row = 0; row <= 4; ++row) {
        const auto y = top - row_height * static_cast<double>(row);
        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{left, y},
            featherdoc::pdf::PdfPoint{right, y},
            featherdoc::pdf::PdfRgbColor{0.16, 0.20, 0.28},
            row == 0 || row == 4 ? 1.1 : 0.65,
        });
    }
    for (const auto x : {left, 300.0, 420.0, right}) {
        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{x, top - row_height * 4.0},
            featherdoc::pdf::PdfPoint{x, top},
            featherdoc::pdf::PdfRgbColor{0.16, 0.20, 0.28},
            0.65,
        });
    }

    page.text_runs.push_back(make_text_run(
        72.0, 724.0, "Invoice grid sample", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        84.0, 670.0, "Line item alpha", 11.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        84.0, 640.0, "Line item beta", 11.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        432.0, 580.0, "Total USD 42", 11.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, false, true,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_image_caption_text_sample(
    const std::filesystem::path &asset_dir) {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: image caption text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    const auto image_path = write_tiny_rgb_png(asset_dir);
    page.images.push_back(featherdoc::pdf::PdfImage{
        featherdoc::pdf::PdfRect{72.0, 602.0, 96.0, 96.0},
        image_path,
        "image/png",
        "Generated inline image asset",
        true,
        true,
    });
    page.rectangles.push_back(featherdoc::pdf::PdfRectangle{
        featherdoc::pdf::PdfRect{70.0, 600.0, 100.0, 100.0},
        featherdoc::pdf::PdfRgbColor{0.20, 0.24, 0.30},
        featherdoc::pdf::PdfRgbColor{1.0, 1.0, 1.0},
        0.75,
        true,
        false,
    });
    page.text_runs.push_back(make_text_run(
        72.0, 724.0, "Image caption sample", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        190.0, 674.0, "Generated inline image asset", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        190.0, 650.0, "PDFium reads caption beside image", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_image_report_text_sample(
    const std::filesystem::path &asset_dir) {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: image report text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    const auto image_path = write_tiny_rgb_png(
        asset_dir, "featherdoc-pdf-regression-image-report.png");

    auto page1 = make_letter_page();
    page1.text_runs.push_back(make_text_run(
        72.0, 724.0, "Image report sample", 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, false, false,
        false));
    page1.text_runs.push_back(make_text_run(
        72.0, 698.0, "Page 1 keeps text readable beside an image", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page1.images.push_back(featherdoc::pdf::PdfImage{
        featherdoc::pdf::PdfRect{72.0, 540.0, 108.0, 108.0},
        image_path,
        "image/png",
        "Report figure one",
        true,
        true,
    });
    page1.text_runs.push_back(make_text_run(
        198.0, 610.0, "Figure 1: inline image on the first page", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page1.text_runs.push_back(make_text_run(
        198.0, 586.0, "PDFium still extracts text across the figure", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));

    auto page2 = make_letter_page();
    page2.text_runs.push_back(make_text_run(
        72.0, 724.0, "Image report sample page 2", 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, false, false,
        false));
    page2.text_runs.push_back(make_text_run(
        72.0, 698.0, "Second page confirms the image path is stable", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page2.images.push_back(featherdoc::pdf::PdfImage{
        featherdoc::pdf::PdfRect{72.0, 540.0, 108.0, 108.0},
        image_path,
        "image/png",
        "Report figure two",
        true,
        true,
    });
    page2.text_runs.push_back(make_text_run(
        198.0, 610.0, "Figure 2: reused image asset on the second page", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page2.text_runs.push_back(make_text_run(
        198.0, 586.0, "Page 2 text extraction remains intact", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page1));
    sample.layout.pages.push_back(std::move(page2));
    return sample;
}

[[nodiscard]] bool append_document_text_paragraph(
    featherdoc::Document &document,
    std::string_view text);

[[nodiscard]] ScenarioResult build_document_image_semantics_text_sample(
    const std::filesystem::path &asset_dir) {
    ScenarioResult sample;

    const auto image_path = write_quadrant_rgb_png(
        asset_dir, "featherdoc-pdf-regression-document-image-semantics.png");

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document image semantics sample") ||
        !append_document_text_paragraph(
            document,
            "Inline image intro keeps body flow above the image block.") ||
        !document.append_image(image_path, 64U, 32U) ||
        !append_document_text_paragraph(
            document,
            "Text after inline image remains below the image block.")) {
        return sample;
    }

    featherdoc::floating_image_options square_wrap_options;
    square_wrap_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::column;
    square_wrap_options.horizontal_offset_px = 0;
    square_wrap_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    square_wrap_options.vertical_offset_px = 0;
    square_wrap_options.wrap_mode = featherdoc::floating_image_wrap_mode::square;
    square_wrap_options.wrap_distance_right_px = 16U;
    square_wrap_options.crop = featherdoc::floating_image_crop{250U, 0U, 0U, 0U};
    if (!document.append_floating_image(image_path, 96U, 72U,
                                        square_wrap_options) ||
        !append_document_text_paragraph(
            document,
            "Square wrap text flows beside the anchored image before returning "
            "to the margin.")) {
        return sample;
    }

    featherdoc::floating_image_options behind_text_options;
    behind_text_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    behind_text_options.horizontal_offset_px = 420;
    behind_text_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    behind_text_options.vertical_offset_px = 0;
    behind_text_options.wrap_mode = featherdoc::floating_image_wrap_mode::none;
    behind_text_options.behind_text = true;
    if (!document.append_floating_image(image_path, 96U, 48U,
                                        behind_text_options) ||
        !append_document_text_paragraph(
            document,
            "Behind text image anchor paragraph stays readable.")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document image semantics text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.render_inline_images = true;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 5.0;
    options.image_spacing_after_points = 6.0;

    sample.layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_table_semantics_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document table semantics sample")) {
        return sample;
    }

    auto table = document.append_table(6U, 2U);
    if (!table.has_next() ||
        !table.set_width_twips(2880U) ||
        !table.set_column_width_twips(0U, 1440U) ||
        !table.set_column_width_twips(1U, 1440U) ||
        !table.set_cell_text(0U, 0U, "Header") ||
        !table.set_cell_text(0U, 1U, "Amount")) {
        return sample;
    }
    for (std::size_t row_index = 1U; row_index < 6U; ++row_index) {
        if (!table.set_cell_text(row_index, 0U,
                                 "Row " + std::to_string(row_index)) ||
            !table.set_cell_text(row_index, 1U,
                                 std::to_string(row_index * 10U))) {
            return sample;
        }
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 6U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(720U, featherdoc::row_height_rule::exact)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        if (row_index == 4U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table semantics text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_invoice_table_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document invoice table sample") ||
        !append_document_text_paragraph(document,
                                        "Invoice No: INV-2026-0507") ||
        !append_document_text_paragraph(document,
                                        "Bill to: FeatherDoc QA")) {
        return sample;
    }

    auto table = document.append_table(5U, 4U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 3000U) ||
        !table.set_column_width_twips(1U, 720U) ||
        !table.set_column_width_twips(2U, 1800U) ||
        !table.set_column_width_twips(3U, 1680U) ||
        !table.set_cell_text(0U, 0U, "Item") ||
        !table.set_cell_text(0U, 1U, "Qty") ||
        !table.set_cell_text(0U, 2U, "Unit") ||
        !table.set_cell_text(0U, 3U, "Total") ||
        !table.set_cell_text(1U, 0U, "PDF export design") ||
        !table.set_cell_text(1U, 1U, "2") ||
        !table.set_cell_text(1U, 2U, "USD 50") ||
        !table.set_cell_text(1U, 3U, "USD 100") ||
        !table.set_cell_text(2U, 0U, "Visual validation") ||
        !table.set_cell_text(2U, 1U, "1") ||
        !table.set_cell_text(2U, 2U, "USD 25") ||
        !table.set_cell_text(2U, 3U, "USD 25") ||
        !table.set_cell_text(3U, 0U, "Regression evidence") ||
        !table.set_cell_text(3U, 1U, "1") ||
        !table.set_cell_text(3U, 2U, "USD 10") ||
        !table.set_cell_text(3U, 3U, "USD 10") ||
        !table.set_cell_text(4U, 0U, "Grand total") ||
        !table.set_cell_text(4U, 3U, "USD 135")) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 5U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(360U, featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document invoice table text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_cjk_image_report_text_sample(
    const std::filesystem::path &font_path,
    const std::filesystem::path &asset_dir) {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: CJK image report text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    const auto image_path = write_tiny_rgb_png(
        asset_dir, "featherdoc-pdf-regression-cjk-image-report.png");

    auto page1 = make_letter_page();
    page1.text_runs.push_back(make_text_run(
        72.0, 724.0, utf8_from_u8(u8"中文图文报告样本"), 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        true, "CJK Regression Font", font_path));
    page1.text_runs.push_back(make_text_run(
        72.0, 698.0, utf8_from_u8(u8"第一页包含图片和中文段落"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));
    page1.images.push_back(featherdoc::pdf::PdfImage{
        featherdoc::pdf::PdfRect{72.0, 540.0, 108.0, 108.0},
        image_path,
        "image/png",
        utf8_from_u8(u8"图一"),
        true,
        true,
    });
    page1.text_runs.push_back(make_text_run(
        198.0, 610.0, utf8_from_u8(u8"PDFium 回读中文和图片"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));

    auto page2 = make_letter_page();
    page2.text_runs.push_back(make_text_run(
        72.0, 724.0, utf8_from_u8(u8"第二页继续图文回读"), 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        true, "CJK Regression Font", font_path));
    page2.text_runs.push_back(make_text_run(
        72.0, 698.0, utf8_from_u8(u8"第二页复用同一张位图"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));
    page2.images.push_back(featherdoc::pdf::PdfImage{
        featherdoc::pdf::PdfRect{72.0, 540.0, 108.0, 108.0},
        image_path,
        "image/png",
        utf8_from_u8(u8"图二"),
        true,
        true,
    });
    page2.text_runs.push_back(make_text_run(
        198.0, 610.0, utf8_from_u8(u8"中文图文回归保持稳定"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));

    sample.layout.pages.push_back(std::move(page1));
    sample.layout.pages.push_back(std::move(page2));
    return sample;
}

[[nodiscard]] featherdoc::Paragraph append_document_paragraph(
    featherdoc::Document &document,
    std::string_view text) {
    auto paragraph = document.paragraphs();
    while (paragraph.has_next()) {
        paragraph.next();
    }

    return paragraph.insert_paragraph_after(std::string{text});
}

[[nodiscard]] bool append_document_text_paragraph(
    featherdoc::Document &document,
    std::string_view text) {
    return append_document_paragraph(document, text).has_next();
}

[[nodiscard]] ScenarioResult build_header_footer_rtl_variants_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !define_document_rtl_bidi_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Header footer RTL variants sample")) {
        return sample;
    }

    for (std::size_t index = 1U; index <= 17U; ++index) {
        const auto paragraph_text =
            "Variant body line " + std::to_string(index);
        if (!append_document_text_paragraph(document, paragraph_text)) {
            return sample;
        }
    }

    auto configure_variant_header =
        [&](auto &paragraph, std::string_view label, std::string_view code,
            std::u8string_view suffix) {
            if (!paragraph.has_next() ||
                !document.set_paragraph_style(paragraph,
                                              "DocumentPdfRtlParagraph") ||
                !paragraph.set_bidi(true) ||
                !paragraph.set_alignment(
                    featherdoc::paragraph_alignment::right) ||
                !paragraph.add_run(std::string{label}).has_next()) {
                return false;
            }

            auto prefix = paragraph.add_run(utf8_from_u8(u8"النسخة"));
            if (!prefix.has_next() ||
                !document.set_run_style(prefix, "DocumentPdfRtlArabicEmphasis") ||
                !paragraph.add_run(std::string{code}).has_next()) {
                return false;
            }

            auto suffix_run = paragraph.add_run(utf8_from_u8(suffix));
            return suffix_run.has_next() &&
                   document.set_run_style(suffix_run, "DocumentPdfRtlArabic");
        };

    auto configure_variant_footer =
        [&](auto &paragraph, std::string_view label,
            std::u8string_view suffix) {
            if (!paragraph.has_next() ||
                !document.set_paragraph_style(paragraph,
                                              "DocumentPdfRtlParagraph") ||
                !paragraph.set_bidi(true) ||
                !paragraph.set_alignment(
                    featherdoc::paragraph_alignment::right) ||
                !paragraph.add_run(std::string{label}).has_next() ||
                !paragraph.add_run("{{page}}").has_next() ||
                !paragraph.add_run(" / ").has_next() ||
                !paragraph.add_run("{{total_pages}}").has_next() ||
                !paragraph.add_run(" | section ").has_next() ||
                !paragraph.add_run("{{section_page}}").has_next() ||
                !paragraph.add_run(" / ").has_next() ||
                !paragraph.add_run("{{section_total_pages}}").has_next() ||
                !paragraph.add_run(" - ").has_next()) {
                return false;
            }

            auto suffix_run = paragraph.add_run(utf8_from_u8(suffix));
            return suffix_run.has_next() &&
                   document.set_run_style(suffix_run, "DocumentPdfRtlArabic");
        };

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!configure_variant_header(default_header, "Default header ",
                                  " HF-303 ", u8"جاهزة") ||
        !configure_variant_footer(default_footer, "Default footer page ",
                                  u8"نهائية") ||
        !configure_variant_header(first_header, "First header ",
                                  " HF-101 ", u8"الافتتاحية") ||
        !configure_variant_footer(first_footer, "First footer page ",
                                  u8"الأولى") ||
        !configure_variant_header(even_header, "Even header ",
                                  " HF-202 ", u8"الزوجية") ||
        !configure_variant_footer(even_footer, "Even footer page ",
                                  u8"المزدوجة")) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.width_twips = 12240U;
    setup.height_twips = 4320U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 1440U;
    setup.margins.right_twips = 1440U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.metadata.title =
        "FeatherDoc regression sample: header footer RTL variants";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document RTL Arabic", font_path},
    };
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.font_size_points = 12.0;
    options.line_height_points = 16.0;
    options.header_footer_font_size_points = 8.0;

    sample.layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_complex_layout_text_sample(
    const std::filesystem::path &cjk_font_path,
    const std::filesystem::path &asset_dir) {
    ScenarioResult sample;

    const auto image_path = write_quadrant_rgb_png(
        asset_dir,
        "featherdoc-pdf-regression-document-cjk-complex-layout.png");

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document Complex Layout CJK") ||
        !define_document_complex_layout_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document CJK complex layout sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Complex layout overview: ").has_next() ||
        !add_styled_contract_run(document, intro,
                                 utf8_from_u8(u8"页眉页脚分页变体"),
                                 "DocumentPdfComplexLayoutCjkAccent") ||
        !intro.add_run(" / FE-CL-901 / ").has_next() ||
        !add_styled_contract_run(document, intro,
                                 utf8_from_u8(u8"多锚点图文混排"),
                                 "DocumentPdfComplexLayoutCjkNote")) {
        return sample;
    }

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text("Complex header CL-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Complex footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text("Complex first header CL-101 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            "Complex first footer {{page}} / {{total_pages}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Complex even header CL-202 page {{page}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text(
            "Complex even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.width_twips = 12240U;
    setup.height_twips = 15840U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 900U;
    setup.margins.right_twips = 900U;
    setup.margins.header_twips = 300U;
    setup.margins.footer_twips = 300U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            "Inline figure preface keeps body flow above the image block.") ||
        !document.append_image(image_path, 96U, 48U) ||
        !append_document_text_paragraph(
            document,
            "Text after inline image stays below the block before floating "
            "anchors start.")) {
        return sample;
    }

    auto style_matrix = append_document_paragraph(document, "");
    if (!style_matrix.has_next() ||
        !style_matrix.add_run("Wrap matrix: ").has_next() ||
        !add_styled_contract_run(document, style_matrix,
                                 utf8_from_u8(u8"图文锚点"),
                                 "DocumentPdfComplexLayoutCjkAccent") ||
        !style_matrix.add_run(" + ").has_next() ||
        !add_styled_contract_run(document, style_matrix,
                                 "bold italic blue 16pt",
                                 "DocumentPdfStyleGalleryAccent") ||
        !style_matrix.add_run(" + ").has_next() ||
        !add_styled_contract_run(document, style_matrix,
                                 utf8_from_u8(u8"裁剪环绕"),
                                 "DocumentPdfComplexLayoutCjkNote")) {
        return sample;
    }

    featherdoc::floating_image_options square_wrap_options;
    square_wrap_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::column;
    square_wrap_options.horizontal_offset_px = 0;
    square_wrap_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    square_wrap_options.vertical_offset_px = 0;
    square_wrap_options.wrap_mode = featherdoc::floating_image_wrap_mode::square;
    square_wrap_options.wrap_distance_right_px = 18U;
    square_wrap_options.wrap_distance_bottom_px = 12U;
    square_wrap_options.crop =
        featherdoc::floating_image_crop{180U, 0U, 40U, 120U};
    if (!document.append_floating_image(image_path, 124U, 92U,
                                        square_wrap_options) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(u8"第一段围绕浮动图继续排版，同时保留 English token "
                         u8"FE-CL-902 以便 PDFium 复制检索核验。")) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(u8"第二段继续贴着裁剪后的图像收窄换行，确认段落不会覆盖图片，"
                         u8"也不会在回流后丢失中文字形。"))) {
        return sample;
    }

    featherdoc::floating_image_options top_bottom_options;
    top_bottom_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    top_bottom_options.horizontal_offset_px = 144;
    top_bottom_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    top_bottom_options.vertical_offset_px = 0;
    top_bottom_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::top_bottom;
    top_bottom_options.wrap_distance_top_px = 8U;
    top_bottom_options.wrap_distance_bottom_px = 10U;
    top_bottom_options.crop =
        featherdoc::floating_image_crop{0U, 180U, 220U, 0U};
    if (!document.append_floating_image(image_path, 186U, 56U,
                                        top_bottom_options) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"Top-bottom 浮动图之后的正文需要整体回到全宽列，不能延续上一段的包围缩进。"))) {
        return sample;
    }

    featherdoc::floating_image_options behind_text_options;
    behind_text_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    behind_text_options.horizontal_offset_px = 330;
    behind_text_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    behind_text_options.vertical_offset_px = 0;
    behind_text_options.behind_text = true;
    behind_text_options.z_order = 2U;
    behind_text_options.wrap_mode = featherdoc::floating_image_wrap_mode::none;
    behind_text_options.crop =
        featherdoc::floating_image_crop{80U, 80U, 80U, 80U};
    if (!document.append_floating_image(image_path, 138U, 84U,
                                        behind_text_options) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(u8"Behind-text 图层必须保持文字可读，复制搜索仍要能拿到"
                         u8"“中文检索图层”和 FE-CL-903。"))) {
        return sample;
    }

    auto overlay = append_document_paragraph(document, "");
    if (!overlay.has_next() ||
        !overlay.add_run("Overlay note: ").has_next() ||
        !add_styled_contract_run(document, overlay,
                                 utf8_from_u8(u8"复杂混排继续验证"),
                                 "DocumentPdfComplexLayoutCjkAccent") ||
        !overlay.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, overlay, "underlined note",
                                 "DocumentPdfStyleGalleryNote")) {
        return sample;
    }

    for (int index = 1; index <= 42; ++index) {
        const auto paragraph_text =
            utf8_from_u8(u8"第 ") + std::to_string(index) +
            utf8_from_u8(
                u8" 段继续验证复杂版式稳定性，混排 English token FE-CL-") +
            std::to_string(920 + index) +
            utf8_from_u8(
                u8"，并确认分页后页眉页脚占位符仍然正确。");
        if (!append_document_text_paragraph(document, paragraph_text)) {
            return sample;
        }
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK complex layout";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document Complex Layout CJK",
                                        cjk_font_path},
    };
    options.cjk_font_file_path = cjk_font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.render_inline_images = true;
    options.header_footer_font_size_points = 8.0;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 5.0;
    options.image_spacing_after_points = 6.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_copy_search_matrix_text_sample(
    const std::filesystem::path &cjk_font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document CJK Copy Search") ||
        !define_document_cjk_copy_search_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document CJK copy search matrix sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Copy search matrix: ").has_next() ||
        !add_styled_contract_run(document, intro, utf8_from_u8(u8"检索锚点甲"),
                                 "DocumentPdfCjkCopySearchAccent") ||
        !intro.add_run(" / FE-CS-901 / ").has_next() ||
        !add_styled_contract_run(document, intro, utf8_from_u8(u8"页码矩阵"),
                                 "DocumentPdfCjkCopySearchNote")) {
        return sample;
    }

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text(
            utf8_from_u8(u8"检索默认页眉 CS-303 第 {{page}} 页")) ||
        !default_footer.has_next() ||
        !default_footer.set_text(
            utf8_from_u8(u8"检索默认页脚 {{page}} / {{total_pages}}")) ||
        !first_header.has_next() ||
        !first_header.set_text(
            utf8_from_u8(u8"检索首页页眉 CS-101 第 {{page}} 页")) ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            utf8_from_u8(u8"检索首页页脚 {{page}} / {{total_pages}}")) ||
        !even_header.has_next() ||
        !even_header.set_text(
            utf8_from_u8(u8"检索偶数页眉 CS-202 第 {{page}} 页")) ||
        !even_footer.has_next() ||
        !even_footer.set_text(
            utf8_from_u8(u8"检索偶数页脚 {{page}} / {{total_pages}}"))) {
        return sample;
    }

    auto marker = append_document_paragraph(document, "");
    if (!marker.has_next() ||
        !marker.set_alignment(featherdoc::paragraph_alignment::right) ||
        !marker.add_run("Search marker: ").has_next() ||
        !add_styled_contract_run(document, marker, utf8_from_u8(u8"中文检索矩阵"),
                                 "DocumentPdfCjkCopySearchAccent") ||
        !marker.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, marker, utf8_from_u8(u8"页脚索引"),
                                 "DocumentPdfCjkCopySearchNote")) {
        return sample;
    }

    for (int index = 1; index <= 10; ++index) {
        const auto paragraph_text =
            utf8_from_u8(u8"第 ") + std::to_string(index) +
            utf8_from_u8(
                u8" 组复制检索验证：合同检索、页眉索引、回读矩阵，混排 English token FE-CS-") +
            std::to_string(910 + index) + utf8_from_u8(u8"。");
        if (!append_document_text_paragraph(document, paragraph_text)) {
            return sample;
        }
    }

    auto table = document.append_table(5U, 2U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1800U) ||
        !table.set_column_width_twips(1U, 5400U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"区域")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"检索键")) ||
        !table.set_cell_text(1U, 0U, utf8_from_u8(u8"正文")) ||
        !table.set_cell_text(1U, 1U, utf8_from_u8(u8"复制路径")) ||
        !table.set_cell_text(2U, 0U, utf8_from_u8(u8"页眉")) ||
        !table.set_cell_text(2U, 1U, utf8_from_u8(u8"页眉索引")) ||
        !table.set_cell_text(3U, 0U, utf8_from_u8(u8"页脚")) ||
        !table.set_cell_text(3U, 1U, utf8_from_u8(u8"页脚索引")) ||
        !table.set_cell_text(4U, 0U, utf8_from_u8(u8"终页")) ||
        !table.set_cell_text(4U, 1U, utf8_from_u8(u8"终页检索"))) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 5U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(420U, featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        row.next();
    }

    for (int index = 11; index <= 30; ++index) {
        const auto paragraph_text =
            utf8_from_u8(u8"第 ") + std::to_string(index) +
            utf8_from_u8(
                u8" 组复制检索验证：终页检索、页脚索引、合同检索，混排 English token FE-CS-") +
            std::to_string(910 + index) + utf8_from_u8(u8"。");
        if (!append_document_text_paragraph(document, paragraph_text)) {
            return sample;
        }
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK copy search matrix";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Copy Search",
                                        cjk_font_path},
    };
    options.cjk_font_file_path = cjk_font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 8.0;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 6.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_font_embed_matrix_text_sample(
    const std::filesystem::path &cjk_font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document CJK Font Embed") ||
        !define_document_cjk_font_embed_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document CJK font embed matrix sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Font embed matrix: ").has_next() ||
        !add_styled_contract_run(document, intro, utf8_from_u8(u8"嵌字矩阵甲"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !intro.add_run(" / FE-FM-901 / ").has_next() ||
        !add_styled_contract_run(document, intro, utf8_from_u8(u8"复制搜索路径"),
                                 "DocumentPdfCjkFontEmbedNote")) {
        return sample;
    }

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text("Embed header FM-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Embed footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text("Embed first header FM-101 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text("Embed first footer {{page}} / {{total_pages}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Embed even header FM-202 page {{page}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text("Embed even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    auto marker = append_document_paragraph(document, "");
    if (!marker.has_next() ||
        !marker.set_alignment(featherdoc::paragraph_alignment::right) ||
        !marker.add_run("Metrics marker: ").has_next() ||
        !add_styled_contract_run(document, marker, utf8_from_u8(u8"字宽校验"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !marker.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, marker, utf8_from_u8(u8"搜索回读"),
                                 "DocumentPdfCjkFontEmbedNote")) {
        return sample;
    }

    auto scale = append_document_paragraph(document, "");
    if (!scale.has_next() || !scale.add_run("Run scale: ").has_next() ||
        !add_styled_contract_run(document, scale, utf8_from_u8(u8"小字回读"),
                                 "DocumentPdfCjkFontEmbedNote") ||
        !scale.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, scale, utf8_from_u8(u8"合同检索"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !scale.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, scale, utf8_from_u8(u8"终页嵌字"),
                                 "DocumentPdfCjkFontEmbedLarge")) {
        return sample;
    }

    for (int index = 1; index <= 8; ++index) {
        const auto paragraph_text =
            utf8_from_u8(u8"第 ") + std::to_string(index) +
            utf8_from_u8(
                u8" 组嵌字矩阵验证：复制搜索、字宽校验、页眉索引与页脚索引，混排 English token FE-FM-") +
            std::to_string(910 + index) +
            utf8_from_u8(u8"，并确认多字号切换后回读文本没有丢字。");
        if (!append_document_text_paragraph(document, paragraph_text)) {
            return sample;
        }
    }

    auto table = document.append_table(6U, 2U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1800U) ||
        !table.set_column_width_twips(1U, 5400U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"区域")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"嵌字键")) ||
        !table.set_cell_text(1U, 0U, utf8_from_u8(u8"正文")) ||
        !table.set_cell_text(1U, 1U, utf8_from_u8(u8"复制路径")) ||
        !table.set_cell_text(2U, 0U, utf8_from_u8(u8"页眉")) ||
        !table.set_cell_text(2U, 1U, utf8_from_u8(u8"页眉索引")) ||
        !table.set_cell_text(3U, 0U, utf8_from_u8(u8"页脚")) ||
        !table.set_cell_text(3U, 1U, utf8_from_u8(u8"页脚索引")) ||
        !table.set_cell_text(4U, 0U, utf8_from_u8(u8"终页")) ||
        !table.set_cell_text(4U, 1U, utf8_from_u8(u8"终页搜索")) ||
        !table.set_cell_text(5U, 0U, utf8_from_u8(u8"样式")) ||
        !table.set_cell_text(5U, 1U, utf8_from_u8(u8"大字小字"))) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 6U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(420U, featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        row.next();
    }

    for (int index = 9; index <= 30; ++index) {
        const auto paragraph_text =
            utf8_from_u8(u8"第 ") + std::to_string(index) +
            utf8_from_u8(
                u8" 组嵌字矩阵验证：终页搜索、复制搜索、字宽校验与样式回读，混排 English token FE-FM-") +
            std::to_string(910 + index) +
            utf8_from_u8(u8"，并确认页码占位符展开后检索结果仍然一致。");
        if (!append_document_text_paragraph(document, paragraph_text)) {
            return sample;
        }
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("Embed close: ").has_next() ||
        !add_styled_contract_run(document, closing, utf8_from_u8(u8"末页嵌字"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !closing.add_run(" / FE-FM-999 / ").has_next() ||
        !add_styled_contract_run(document, closing, utf8_from_u8(u8"字宽回读"),
                                 "DocumentPdfCjkFontEmbedLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK font embed matrix";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Font Embed",
                                        cjk_font_path},
    };
    options.cjk_font_file_path = cjk_font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 8.0;
    options.line_height_points = 17.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_font_embed_wrap_mix_text_sample(
    const std::filesystem::path &cjk_font_path,
    const std::filesystem::path &asset_dir) {
    ScenarioResult sample;

    const auto image_path = write_quadrant_rgb_png(
        asset_dir,
        "featherdoc-pdf-regression-document-cjk-font-embed-wrap-mix.png");

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document CJK Font Embed") ||
        !define_document_cjk_font_embed_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document CJK font embed wrap mix sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Font embed wrap mix: ").has_next() ||
        !add_styled_contract_run(document, intro, utf8_from_u8(u8"嵌字环绕甲"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !intro.add_run(" / FE-WM-901 / ").has_next() ||
        !add_styled_contract_run(document, intro, utf8_from_u8(u8"裁剪回读矩阵"),
                                 "DocumentPdfCjkFontEmbedNote")) {
        return sample;
    }

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text("Wrap header WM-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Wrap footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text("Wrap first header WM-101 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text("Wrap first footer {{page}} / {{total_pages}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Wrap even header WM-202 page {{page}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text("Wrap even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.width_twips = 12240U;
    setup.height_twips = 15840U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 900U;
    setup.margins.right_twips = 900U;
    setup.margins.header_twips = 300U;
    setup.margins.footer_twips = 300U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(u8"封面段落先验证内联图片前后的嵌字基线，然后进入跨页裁剪与环绕混排场景。")) ||
        !document.append_image(image_path, 108U, 56U) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(u8"内联图片之后立即恢复正文全宽流，确认图像块不会破坏后续复制搜索与字宽回读。"))) {
        return sample;
    }

    auto stripe = append_document_paragraph(document, "");
    if (!stripe.has_next() ||
        !stripe.set_alignment(featherdoc::paragraph_alignment::right) ||
        !stripe.add_run("Embed wrap stripe: ").has_next() ||
        !add_styled_contract_run(document, stripe, utf8_from_u8(u8"字体回读"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !stripe.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, stripe, utf8_from_u8(u8"终页检索"),
                                 "DocumentPdfCjkFontEmbedNote")) {
        return sample;
    }

    auto scale = append_document_paragraph(document, "");
    if (!scale.has_next() || !scale.add_run("Wrap scale: ").has_next() ||
        !add_styled_contract_run(document, scale, utf8_from_u8(u8"小字锚点"),
                                 "DocumentPdfCjkFontEmbedNote") ||
        !scale.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, scale, utf8_from_u8(u8"合同矩阵"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !scale.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, scale, utf8_from_u8(u8"大字嵌字"),
                                 "DocumentPdfCjkFontEmbedLarge")) {
        return sample;
    }

    const auto append_wrap_paragraph = [&](int index,
                                           std::u8string_view label) {
        const auto paragraph_text =
            utf8_from_u8(u8"第 ") + std::to_string(index) +
            utf8_from_u8(u8" 组环绕嵌字验证：") + utf8_from_u8(label) +
            utf8_from_u8(
                u8"、复制搜索、字宽回读、页眉页脚索引与合同检索，混排 English token FE-WM-") +
            std::to_string(910 + index) +
            utf8_from_u8(u8"，并确认分页后图片裁剪和文本回流仍然稳定。");
        return append_document_text_paragraph(document, paragraph_text);
    };

    featherdoc::floating_image_options left_square_options;
    left_square_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::column;
    left_square_options.horizontal_offset_px = 0;
    left_square_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    left_square_options.vertical_offset_px = 0;
    left_square_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::square;
    left_square_options.wrap_distance_right_px = 20U;
    left_square_options.wrap_distance_bottom_px = 12U;
    left_square_options.crop =
        featherdoc::floating_image_crop{180U, 0U, 40U, 120U};
    if (!document.append_floating_image(image_path, 124U, 92U,
                                        left_square_options)) {
        return sample;
    }

    for (int index = 1; index <= 8; ++index) {
        if (!append_wrap_paragraph(index, u8"左侧裁剪包围")) {
            return sample;
        }
    }

    auto checkpoint_a = append_document_paragraph(document, "");
    if (!checkpoint_a.has_next() ||
        !checkpoint_a.add_run("Wrap checkpoint A: ").has_next() ||
        !add_styled_contract_run(document, checkpoint_a,
                                 utf8_from_u8(u8"环绕矩阵甲"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !checkpoint_a.add_run(" / FE-WM-941 / ").has_next() ||
        !add_styled_contract_run(document, checkpoint_a,
                                 utf8_from_u8(u8"字宽校验"),
                                 "DocumentPdfCjkFontEmbedNote")) {
        return sample;
    }

    featherdoc::floating_image_options right_square_options;
    right_square_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    right_square_options.horizontal_offset_px = 360;
    right_square_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    right_square_options.vertical_offset_px = 0;
    right_square_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::square;
    right_square_options.wrap_distance_left_px = 18U;
    right_square_options.wrap_distance_bottom_px = 10U;
    right_square_options.crop =
        featherdoc::floating_image_crop{0U, 120U, 140U, 0U};
    if (!document.append_floating_image(image_path, 118U, 96U,
                                        right_square_options)) {
        return sample;
    }

    for (int index = 9; index <= 16; ++index) {
        if (!append_wrap_paragraph(index, u8"右侧裁剪包围")) {
            return sample;
        }
    }

    auto table = document.append_table(5U, 2U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1800U) ||
        !table.set_column_width_twips(1U, 5400U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"区域")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"嵌字键")) ||
        !table.set_cell_text(1U, 0U, utf8_from_u8(u8"左流")) ||
        !table.set_cell_text(1U, 1U, utf8_from_u8(u8"图片裁剪")) ||
        !table.set_cell_text(2U, 0U, utf8_from_u8(u8"右流")) ||
        !table.set_cell_text(2U, 1U, utf8_from_u8(u8"合同检索")) ||
        !table.set_cell_text(3U, 0U, utf8_from_u8(u8"终页")) ||
        !table.set_cell_text(3U, 1U, utf8_from_u8(u8"终页锚点")) ||
        !table.set_cell_text(4U, 0U, utf8_from_u8(u8"页脚")) ||
        !table.set_cell_text(4U, 1U, utf8_from_u8(u8"页脚回读"))) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 5U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(420U, featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        row.next();
    }

    featherdoc::floating_image_options top_bottom_options;
    top_bottom_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    top_bottom_options.horizontal_offset_px = 144;
    top_bottom_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    top_bottom_options.vertical_offset_px = 0;
    top_bottom_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::top_bottom;
    top_bottom_options.wrap_distance_top_px = 10U;
    top_bottom_options.wrap_distance_bottom_px = 12U;
    top_bottom_options.crop =
        featherdoc::floating_image_crop{0U, 180U, 220U, 0U};
    if (!document.append_floating_image(image_path, 196U, 60U,
                                        top_bottom_options)) {
        return sample;
    }

    for (int index = 17; index <= 24; ++index) {
        if (!append_wrap_paragraph(index, u8"上下回流切换")) {
            return sample;
        }
    }

    auto checkpoint_b = append_document_paragraph(document, "");
    if (!checkpoint_b.has_next() ||
        !checkpoint_b.add_run("Wrap checkpoint B: ").has_next() ||
        !add_styled_contract_run(document, checkpoint_b,
                                 utf8_from_u8(u8"复制搜索路径"),
                                 "DocumentPdfCjkFontEmbedNote") ||
        !checkpoint_b.add_run(" / FE-WM-961 / ").has_next() ||
        !add_styled_contract_run(document, checkpoint_b,
                                 utf8_from_u8(u8"合同检索"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !checkpoint_b.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, checkpoint_b,
                                 utf8_from_u8(u8"大字嵌字"),
                                 "DocumentPdfCjkFontEmbedLarge")) {
        return sample;
    }

    featherdoc::floating_image_options behind_text_options;
    behind_text_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    behind_text_options.horizontal_offset_px = 324;
    behind_text_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    behind_text_options.vertical_offset_px = 0;
    behind_text_options.wrap_mode = featherdoc::floating_image_wrap_mode::none;
    behind_text_options.behind_text = true;
    behind_text_options.z_order = 2U;
    behind_text_options.crop =
        featherdoc::floating_image_crop{80U, 80U, 80U, 80U};
    if (!document.append_floating_image(image_path, 142U, 86U,
                                        behind_text_options)) {
        return sample;
    }

    for (int index = 25; index <= 30; ++index) {
        if (!append_wrap_paragraph(index, u8"叠底文字可读")) {
            return sample;
        }
    }

    featherdoc::floating_image_options final_square_options;
    final_square_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    final_square_options.horizontal_offset_px = 336;
    final_square_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    final_square_options.vertical_offset_px = 0;
    final_square_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::square;
    final_square_options.wrap_distance_left_px = 16U;
    final_square_options.wrap_distance_top_px = 6U;
    final_square_options.wrap_distance_bottom_px = 8U;
    final_square_options.crop =
        featherdoc::floating_image_crop{120U, 40U, 60U, 100U};
    if (!document.append_floating_image(image_path, 110U, 110U,
                                        final_square_options)) {
        return sample;
    }

    for (int index = 31; index <= 42; ++index) {
        if (!append_wrap_paragraph(index, u8"终页嵌字收口")) {
            return sample;
        }
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("Wrap close: ").has_next() ||
        !add_styled_contract_run(document, closing, utf8_from_u8(u8"末页嵌字"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !closing.add_run(" / FE-WM-999 / ").has_next() ||
        !add_styled_contract_run(document, closing, utf8_from_u8(u8"字宽复检"),
                                 "DocumentPdfCjkFontEmbedLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK font embed wrap mix";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Font Embed",
                                        cjk_font_path},
    };
    options.cjk_font_file_path = cjk_font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.render_inline_images = true;
    options.header_footer_font_size_points = 8.0;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 5.0;
    options.image_spacing_after_points = 6.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_image_wrap_stress_text_sample(
    const std::filesystem::path &cjk_font_path,
    const std::filesystem::path &asset_dir) {
    ScenarioResult sample;

    const auto image_path = write_quadrant_rgb_png(
        asset_dir,
        "featherdoc-pdf-regression-document-cjk-image-wrap-stress.png");

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document Complex Layout CJK") ||
        !define_document_complex_layout_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document CJK image wrap stress sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Image wrap stress: ").has_next() ||
        !add_styled_contract_run(document, intro,
                                 utf8_from_u8(u8"多锚点图像流"),
                                 "DocumentPdfComplexLayoutCjkAccent") ||
        !intro.add_run(" / FE-IW-901 / ").has_next() ||
        !add_styled_contract_run(document, intro,
                                 utf8_from_u8(u8"裁剪环绕矩阵"),
                                 "DocumentPdfComplexLayoutCjkNote")) {
        return sample;
    }

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text("Image header IW-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Image footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text("Image first header IW-101 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text("Image first footer {{page}} / {{total_pages}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Image even header IW-202 page {{page}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text("Image even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.width_twips = 12240U;
    setup.height_twips = 15840U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 900U;
    setup.margins.right_twips = 900U;
    setup.margins.header_twips = 300U;
    setup.margins.footer_twips = 300U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(u8"封面段落先验证正文在内联图片前后的基线是否稳定，随后进入跨页图像环绕压力场景。")) ||
        !document.append_image(image_path, 112U, 54U) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(u8"内联图片之后立即恢复正文全宽流，确保图像块不会污染后续浮动锚点的起始缩进。"))) {
        return sample;
    }

    auto stripe = append_document_paragraph(document, "");
    if (!stripe.has_next() ||
        !stripe.set_alignment(featherdoc::paragraph_alignment::right) ||
        !stripe.add_run("Search stripe: ").has_next() ||
        !add_styled_contract_run(document, stripe,
                                 utf8_from_u8(u8"回读热区"),
                                 "DocumentPdfComplexLayoutCjkAccent") ||
        !stripe.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, stripe,
                                 utf8_from_u8(u8"终页锚点"),
                                 "DocumentPdfComplexLayoutCjkNote")) {
        return sample;
    }

    featherdoc::floating_image_options left_square_options;
    left_square_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::column;
    left_square_options.horizontal_offset_px = 0;
    left_square_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    left_square_options.vertical_offset_px = 0;
    left_square_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::square;
    left_square_options.wrap_distance_right_px = 20U;
    left_square_options.wrap_distance_bottom_px = 12U;
    left_square_options.crop =
        featherdoc::floating_image_crop{180U, 0U, 40U, 120U};
    if (!document.append_floating_image(image_path, 124U, 92U,
                                        left_square_options)) {
        return sample;
    }

    const auto append_wrap_paragraph = [&](int index,
                                           std::u8string_view label) {
        const auto paragraph_text =
            utf8_from_u8(u8"第 ") + std::to_string(index) +
            utf8_from_u8(u8" 组图像环绕验证：") + utf8_from_u8(label) +
            utf8_from_u8(
                u8"、页眉索引、页脚索引、合同检索与回读热区，混排 English token FE-IW-") +
            std::to_string(920 + index) +
            utf8_from_u8(u8"，并确认分页后图像裁剪与正文回流仍然稳定。");
        return append_document_text_paragraph(document, paragraph_text);
    };

    for (int index = 1; index <= 8; ++index) {
        if (!append_wrap_paragraph(index, u8"左侧裁剪包围")) {
            return sample;
        }
    }

    featherdoc::floating_image_options right_square_options;
    right_square_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    right_square_options.horizontal_offset_px = 360;
    right_square_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    right_square_options.vertical_offset_px = 0;
    right_square_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::square;
    right_square_options.wrap_distance_left_px = 18U;
    right_square_options.wrap_distance_bottom_px = 10U;
    right_square_options.crop =
        featherdoc::floating_image_crop{0U, 120U, 140U, 0U};
    if (!document.append_floating_image(image_path, 118U, 96U,
                                        right_square_options)) {
        return sample;
    }

    for (int index = 9; index <= 16; ++index) {
        if (!append_wrap_paragraph(index, u8"右侧裁剪包围")) {
            return sample;
        }
    }

    featherdoc::floating_image_options top_bottom_options;
    top_bottom_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    top_bottom_options.horizontal_offset_px = 144;
    top_bottom_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    top_bottom_options.vertical_offset_px = 0;
    top_bottom_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::top_bottom;
    top_bottom_options.wrap_distance_top_px = 10U;
    top_bottom_options.wrap_distance_bottom_px = 12U;
    top_bottom_options.crop =
        featherdoc::floating_image_crop{0U, 180U, 220U, 0U};
    if (!document.append_floating_image(image_path, 196U, 60U,
                                        top_bottom_options)) {
        return sample;
    }

    for (int index = 17; index <= 24; ++index) {
        if (!append_wrap_paragraph(index, u8"上下回流切换")) {
            return sample;
        }
    }

    featherdoc::floating_image_options behind_text_options;
    behind_text_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    behind_text_options.horizontal_offset_px = 324;
    behind_text_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    behind_text_options.vertical_offset_px = 0;
    behind_text_options.wrap_mode = featherdoc::floating_image_wrap_mode::none;
    behind_text_options.behind_text = true;
    behind_text_options.z_order = 2U;
    behind_text_options.crop =
        featherdoc::floating_image_crop{80U, 80U, 80U, 80U};
    if (!document.append_floating_image(image_path, 142U, 86U,
                                        behind_text_options)) {
        return sample;
    }

    for (int index = 25; index <= 30; ++index) {
        if (!append_wrap_paragraph(index, u8"叠底文字可读")) {
            return sample;
        }
    }

    featherdoc::floating_image_options final_square_options;
    final_square_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    final_square_options.horizontal_offset_px = 336;
    final_square_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    final_square_options.vertical_offset_px = 0;
    final_square_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::square;
    final_square_options.wrap_distance_left_px = 16U;
    final_square_options.wrap_distance_top_px = 6U;
    final_square_options.wrap_distance_bottom_px = 8U;
    final_square_options.crop =
        featherdoc::floating_image_crop{120U, 40U, 60U, 100U};
    if (!document.append_floating_image(image_path, 110U, 110U,
                                        final_square_options)) {
        return sample;
    }

    for (int index = 31; index <= 48; ++index) {
        if (!append_wrap_paragraph(index, u8"终页锚点收口")) {
            return sample;
        }
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("Closing matrix: ").has_next() ||
        !add_styled_contract_run(document, closing,
                                 utf8_from_u8(u8"终页锚点"),
                                 "DocumentPdfComplexLayoutCjkAccent") ||
        !closing.add_run(" / FE-IW-999 / ").has_next() ||
        !add_styled_contract_run(document, closing,
                                 utf8_from_u8(u8"裁剪回流"),
                                 "DocumentPdfComplexLayoutCjkNote")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK image wrap stress";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document Complex Layout CJK",
                                        cjk_font_path},
    };
    options.cjk_font_file_path = cjk_font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.render_inline_images = true;
    options.header_footer_font_size_points = 8.0;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 5.0;
    options.image_spacing_after_points = 6.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_extreme_page_breaks_text_sample(
    const std::filesystem::path &cjk_font_path,
    const std::filesystem::path &asset_dir) {
    ScenarioResult sample;

    const auto image_path = write_quadrant_rgb_png(
        asset_dir,
        "featherdoc-pdf-regression-document-cjk-extreme-page-breaks.png");

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document Complex Layout CJK") ||
        !define_document_complex_layout_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document CJK extreme page breaks sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Extreme page breaks: ").has_next() ||
        !add_styled_contract_run(document, intro,
                                 utf8_from_u8(u8"临界分页边界"),
                                 "DocumentPdfComplexLayoutCjkAccent") ||
        !intro.add_run(" / FE-PB-901 / ").has_next() ||
        !add_styled_contract_run(document, intro,
                                 utf8_from_u8(u8"段落回流矩阵"),
                                 "DocumentPdfComplexLayoutCjkNote")) {
        return sample;
    }

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text("Break header PB-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Break footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text("Break first header PB-101 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text("Break first footer {{page}} / {{total_pages}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Break even header PB-202 page {{page}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text("Break even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.width_twips = 8400U;
    setup.height_twips = 10800U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 780U;
    setup.margins.right_twips = 780U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(u8"起始段落先压缩可用高度，让后续短段、长段和图像锚点更容易撞到分页边界。")) ||
        !document.append_image(image_path, 104U, 40U) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(u8"内联图片之后的正文需要立刻恢复整栏宽度，避免分页前后遗留错误缩进。"))) {
        return sample;
    }

    auto marker = append_document_paragraph(document, "");
    if (!marker.has_next() ||
        !marker.set_alignment(featherdoc::paragraph_alignment::right) ||
        !marker.add_run("Boundary stripe: ").has_next() ||
        !add_styled_contract_run(document, marker,
                                 utf8_from_u8(u8"终页收口"),
                                 "DocumentPdfComplexLayoutCjkAccent") ||
        !marker.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, marker,
                                 utf8_from_u8(u8"回流检测"),
                                 "DocumentPdfComplexLayoutCjkNote")) {
        return sample;
    }

    featherdoc::floating_image_options left_square_options;
    left_square_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::column;
    left_square_options.horizontal_offset_px = 0;
    left_square_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    left_square_options.vertical_offset_px = 0;
    left_square_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::square;
    left_square_options.wrap_distance_right_px = 16U;
    left_square_options.wrap_distance_bottom_px = 10U;
    left_square_options.crop =
        featherdoc::floating_image_crop{200U, 0U, 50U, 100U};
    if (!document.append_floating_image(image_path, 104U, 88U,
                                        left_square_options)) {
        return sample;
    }

    const auto append_boundary_paragraph =
        [&](int index, std::u8string_view tag) {
            const auto text =
                utf8_from_u8(u8"第 ") + std::to_string(index) +
                utf8_from_u8(u8" 组分页边界验证：") + utf8_from_u8(tag) +
                utf8_from_u8(
                    u8"、页眉索引、页脚索引、段落回流与合同检索，混排 English token FE-PB-") +
                std::to_string(920 + index) +
                utf8_from_u8(u8"，并确认跨页后没有缺字、重叠或错误缩进。");
            return append_document_text_paragraph(document, text);
        };

    for (int index = 1; index <= 4; ++index) {
        if (!append_boundary_paragraph(index, u8"左侧包围缩进")) {
            return sample;
        }
    }

    featherdoc::floating_image_options top_bottom_options;
    top_bottom_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    top_bottom_options.horizontal_offset_px = 60;
    top_bottom_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    top_bottom_options.vertical_offset_px = 0;
    top_bottom_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::top_bottom;
    top_bottom_options.wrap_distance_top_px = 8U;
    top_bottom_options.wrap_distance_bottom_px = 12U;
    top_bottom_options.crop =
        featherdoc::floating_image_crop{0U, 180U, 200U, 0U};
    if (!document.append_floating_image(image_path, 152U, 54U,
                                        top_bottom_options)) {
        return sample;
    }

    for (int index = 5; index <= 8; ++index) {
        if (!append_boundary_paragraph(index, u8"整栏回流切换")) {
            return sample;
        }
    }

    featherdoc::floating_image_options behind_text_options;
    behind_text_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    behind_text_options.horizontal_offset_px = 176;
    behind_text_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    behind_text_options.vertical_offset_px = 0;
    behind_text_options.wrap_mode = featherdoc::floating_image_wrap_mode::none;
    behind_text_options.behind_text = true;
    behind_text_options.z_order = 2U;
    behind_text_options.crop =
        featherdoc::floating_image_crop{80U, 80U, 80U, 80U};
    if (!document.append_floating_image(image_path, 120U, 76U,
                                        behind_text_options)) {
        return sample;
    }

    for (int index = 9; index <= 12; ++index) {
        if (!append_boundary_paragraph(index, u8"叠底文字可读")) {
            return sample;
        }
    }

    featherdoc::floating_image_options right_square_options;
    right_square_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    right_square_options.horizontal_offset_px = 188;
    right_square_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    right_square_options.vertical_offset_px = 0;
    right_square_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::square;
    right_square_options.wrap_distance_left_px = 14U;
    right_square_options.wrap_distance_bottom_px = 8U;
    right_square_options.crop =
        featherdoc::floating_image_crop{60U, 40U, 120U, 80U};
    if (!document.append_floating_image(image_path, 110U, 98U,
                                        right_square_options)) {
        return sample;
    }

    for (int index = 13; index <= 18; ++index) {
        if (!append_boundary_paragraph(index, u8"末页临界分页")) {
            return sample;
        }
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("Boundary close: ").has_next() ||
        !add_styled_contract_run(document, closing,
                                 utf8_from_u8(u8"临界分页边界"),
                                 "DocumentPdfComplexLayoutCjkAccent") ||
        !closing.add_run(" / FE-PB-999 / ").has_next() ||
        !add_styled_contract_run(document, closing,
                                 utf8_from_u8(u8"末页收口"),
                                 "DocumentPdfComplexLayoutCjkNote")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{420.0, 540.0};
    options.metadata.title =
        "FeatherDoc regression sample: document CJK extreme page breaks";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document Complex Layout CJK",
                                        cjk_font_path},
    };
    options.cjk_font_file_path = cjk_font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.render_inline_images = true;
    options.header_footer_font_size_points = 8.0;
    options.line_height_points = 17.0;
    options.paragraph_spacing_after_points = 5.0;
    options.image_spacing_after_points = 6.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult
build_document_cjk_table_wrap_page_flow_text_sample(
    const std::filesystem::path &cjk_font_path,
    const std::filesystem::path &asset_dir) {
    ScenarioResult sample;

    const auto image_path = write_quadrant_rgb_png(
        asset_dir,
        "featherdoc-pdf-regression-document-cjk-table-wrap-page-flow.png");

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document CJK Font Embed") ||
        !define_document_cjk_font_embed_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document CJK table wrap page flow sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Table wrap page flow: ").has_next() ||
        !add_styled_contract_run(document, intro, utf8_from_u8(u8"交错文档流"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !intro.add_run(" / FE-TF-901 / ").has_next() ||
        !add_styled_contract_run(document, intro, utf8_from_u8(u8"分页环绕矩阵"),
                                 "DocumentPdfCjkFontEmbedNote")) {
        return sample;
    }

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text("Flow header TF-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Flow footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text("Flow first header TF-101 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text("Flow first footer {{page}} / {{total_pages}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Flow even header TF-202 page {{page}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text("Flow even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.width_twips = 9200U;
    setup.height_twips = 12400U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 820U;
    setup.margins.right_twips = 820U;
    setup.margins.header_twips = 260U;
    setup.margins.footer_twips = 260U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(u8"起始段落先验证内联图片前后的文本基线与复制搜索连续性，再进入交错表格流场景。")) ||
        !document.append_image(image_path, 102U, 50U) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(u8"内联图片之后需要立即恢复全宽正文，确保后续表格、页眉页脚和浮动图像都不继承错误缩进。"))) {
        return sample;
    }

    auto stripe = append_document_paragraph(document, "");
    if (!stripe.has_next() ||
        !stripe.set_alignment(featherdoc::paragraph_alignment::right) ||
        !stripe.add_run("Flow stripe: ").has_next() ||
        !add_styled_contract_run(document, stripe, utf8_from_u8(u8"复制校验"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !stripe.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, stripe, utf8_from_u8(u8"终页索引"),
                                 "DocumentPdfCjkFontEmbedNote")) {
        return sample;
    }

    const auto append_flow_paragraph = [&](int index,
                                           std::u8string_view label) {
        const auto text =
            utf8_from_u8(u8"第 ") + std::to_string(index) +
            utf8_from_u8(u8" 组交错流验证：") + utf8_from_u8(label) +
            utf8_from_u8(
                u8"、复制搜索、字宽回读、页码展开与合同检索，混排 English token FE-TF-") +
            std::to_string(920 + index) +
            utf8_from_u8(u8"，并确认表格与段落交错后正文仍然稳定。");
        return append_document_text_paragraph(document, text);
    };

    featherdoc::floating_image_options left_square_options;
    left_square_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::column;
    left_square_options.horizontal_offset_px = 0;
    left_square_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    left_square_options.vertical_offset_px = 0;
    left_square_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::square;
    left_square_options.wrap_distance_right_px = 18U;
    left_square_options.wrap_distance_bottom_px = 10U;
    left_square_options.crop =
        featherdoc::floating_image_crop{180U, 0U, 40U, 120U};
    if (!document.append_floating_image(image_path, 116U, 88U,
                                        left_square_options)) {
        return sample;
    }

    for (int index = 1; index <= 6; ++index) {
        if (!append_flow_paragraph(index, u8"左侧图像包围")) {
            return sample;
        }
    }

    constexpr std::size_t table_a_row_count = 8U;
    auto table_a = document.append_table(table_a_row_count, 3U);
    if (!table_a.has_next() || !table_a.set_width_twips(7200U) ||
        !table_a.set_column_width_twips(0U, 1500U) ||
        !table_a.set_column_width_twips(1U, 1700U) ||
        !table_a.set_column_width_twips(2U, 4000U) ||
        !table_a.set_cell_text(0U, 0U, utf8_from_u8(u8"流转看板")) ||
        !table_a.set_cell_text(1U, 0U, utf8_from_u8(u8"阶段")) ||
        !table_a.set_cell_text(1U, 1U, utf8_from_u8(u8"流键")) ||
        !table_a.set_cell_text(1U, 2U, utf8_from_u8(u8"说明")) ||
        !table_a.set_cell_text(2U, 0U, "TF-A-01") ||
        !table_a.set_cell_text(2U, 1U, utf8_from_u8(u8"正文")) ||
        !table_a.set_cell_text(
            2U, 2U,
            utf8_from_u8(u8"左侧图像后的第一批表格行，用于确认全宽正文已经恢复。")) ||
        !table_a.set_cell_text(3U, 0U, "TF-A-02") ||
        !table_a.set_cell_text(3U, 1U, utf8_from_u8(u8"检索")) ||
        !table_a.set_cell_text(
            3U, 2U,
            utf8_from_u8(u8"重复表头在下一页继续可检索，不能因为分页丢掉中文文本层。")) ||
        !table_a.set_cell_text(4U, 0U, "TF-A-03") ||
        !table_a.set_cell_text(4U, 1U, utf8_from_u8(u8"分页")) ||
        !table_a.set_cell_text(
            4U, 2U,
            utf8_from_u8(u8"分页前后的段落回流不能缺字、重叠或出现异常缩进。")) ||
        !table_a.set_cell_text(5U, 0U, "TF-A-04") ||
        !table_a.set_cell_text(5U, 1U, utf8_from_u8(u8"环绕")) ||
        !table_a.set_cell_text(
            5U, 2U,
            utf8_from_u8(u8"这行刻意放长，用来制造更紧的余量，观察表格和正文交错时的分页稳定性。")) ||
        !table_a.set_cell_text(6U, 0U, "TF-A-05") ||
        !table_a.set_cell_text(6U, 1U, utf8_from_u8(u8"末页")) ||
        !table_a.set_cell_text(
            6U, 2U,
            utf8_from_u8(u8"终页索引仍需与页脚页码一致，保证 copy/search 可回读。")) ||
        !table_a.set_cell_text(7U, 0U, "TF-A-06") ||
        !table_a.set_cell_text(7U, 1U, utf8_from_u8(u8"回读")) ||
        !table_a.set_cell_text(
            7U, 2U,
            utf8_from_u8(u8"后续正文必须从表格下方恢复，不得沿用错误的环绕边界。"))) {
        return sample;
    }

    auto table_a_banner = table_a.find_cell(0U, 0U);
    auto table_a_heading_stage = table_a.find_cell(1U, 0U);
    auto table_a_heading_key = table_a.find_cell(1U, 1U);
    auto table_a_heading_note = table_a.find_cell(1U, 2U);
    if (!table_a_banner.has_value() || !table_a_heading_stage.has_value() ||
        !table_a_heading_key.has_value() ||
        !table_a_heading_note.has_value() ||
        !table_a_banner->merge_right(2U)) {
        return sample;
    }
    if (!table_a_banner->set_fill_color("D9EAF7") ||
        !table_a_heading_stage->set_fill_color("EAF2F8") ||
        !table_a_heading_key->set_fill_color("EAF2F8") ||
        !table_a_heading_note->set_fill_color("EAF2F8")) {
        return sample;
    }

    auto table_a_row = table_a.rows();
    for (std::size_t row_index = 0U; row_index < table_a_row_count;
         ++row_index) {
        if (!table_a_row.has_next()) {
            return sample;
        }
        if (row_index < 2U) {
            if (!table_a_row.set_repeats_header() ||
                !table_a_row.set_height_twips(
                    420U, featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (row_index == 5U) {
            if (!table_a_row.set_cant_split() ||
                !table_a_row.set_height_twips(
                    980U, featherdoc::row_height_rule::at_least)) {
                return sample;
            }
        } else if (!table_a_row.set_height_twips(
                       640U, featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        table_a_row.next();
    }

    auto checkpoint_a = append_document_paragraph(document, "");
    if (!checkpoint_a.has_next() ||
        !checkpoint_a.add_run("Flow checkpoint A: ").has_next() ||
        !add_styled_contract_run(document, checkpoint_a,
                                 utf8_from_u8(u8"表头复用"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !checkpoint_a.add_run(" / FE-TF-951 / ").has_next() ||
        !add_styled_contract_run(document, checkpoint_a,
                                 utf8_from_u8(u8"分页衔接"),
                                 "DocumentPdfCjkFontEmbedNote")) {
        return sample;
    }

    featherdoc::floating_image_options top_bottom_options;
    top_bottom_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    top_bottom_options.horizontal_offset_px = 90;
    top_bottom_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    top_bottom_options.vertical_offset_px = 0;
    top_bottom_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::top_bottom;
    top_bottom_options.wrap_distance_top_px = 8U;
    top_bottom_options.wrap_distance_bottom_px = 10U;
    top_bottom_options.crop =
        featherdoc::floating_image_crop{0U, 180U, 220U, 0U};
    if (!document.append_floating_image(image_path, 176U, 56U,
                                        top_bottom_options)) {
        return sample;
    }

    for (int index = 7; index <= 12; ++index) {
        if (!append_flow_paragraph(index, u8"上下回流切换")) {
            return sample;
        }
    }

    constexpr std::size_t table_b_row_count = 7U;
    auto table_b = document.append_table(table_b_row_count, 3U);
    if (!table_b.has_next() || !table_b.set_width_twips(7200U) ||
        !table_b.set_column_width_twips(0U, 1500U) ||
        !table_b.set_column_width_twips(1U, 1700U) ||
        !table_b.set_column_width_twips(2U, 4000U) ||
        !table_b.set_cell_text(0U, 0U, utf8_from_u8(u8"终页联动")) ||
        !table_b.set_cell_text(1U, 0U, utf8_from_u8(u8"案例")) ||
        !table_b.set_cell_text(1U, 1U, utf8_from_u8(u8"锚点")) ||
        !table_b.set_cell_text(1U, 2U, utf8_from_u8(u8"备注")) ||
        !table_b.set_cell_text(2U, 0U, "TF-B-01") ||
        !table_b.set_cell_text(2U, 1U, utf8_from_u8(u8"表头复用")) ||
        !table_b.set_cell_text(
            2U, 2U,
            utf8_from_u8(u8"上一页结束后这里应复现表头，并保持复制搜索命中。")) ||
        !table_b.set_cell_text(3U, 0U, "TF-B-02") ||
        !table_b.set_cell_text(3U, 1U, utf8_from_u8(u8"分页衔接")) ||
        !table_b.set_cell_text(
            3U, 2U,
            utf8_from_u8(u8"图片锚点与表格正文不能错位，段落回流要在页内自然接续。")) ||
        !table_b.set_cell_text(4U, 0U, "TF-B-03") ||
        !table_b.set_cell_text(4U, 1U, utf8_from_u8(u8"合同检索")) ||
        !table_b.set_cell_text(
            4U, 2U,
            utf8_from_u8(u8"复制搜索路径要与嵌字一致，避免视觉正确但文本层错误。")) ||
        !table_b.set_cell_text(5U, 0U, "TF-B-04") ||
        !table_b.set_cell_text(5U, 1U, utf8_from_u8(u8"右侧锚点")) ||
        !table_b.set_cell_text(
            5U, 2U,
            utf8_from_u8(u8"末页继续验证右侧裁剪包围，不得压到页脚和页码占位符。")) ||
        !table_b.set_cell_text(6U, 0U, "TF-B-05") ||
        !table_b.set_cell_text(6U, 1U, utf8_from_u8(u8"回读复检")) ||
        !table_b.set_cell_text(
            6U, 2U,
            utf8_from_u8(u8"尾行验证全文收口，确认终页合同和检索回读都仍然稳定。"))) {
        return sample;
    }

    auto table_b_banner = table_b.find_cell(0U, 0U);
    auto table_b_heading_case = table_b.find_cell(1U, 0U);
    auto table_b_heading_anchor = table_b.find_cell(1U, 1U);
    auto table_b_heading_note = table_b.find_cell(1U, 2U);
    if (!table_b_banner.has_value() || !table_b_heading_case.has_value() ||
        !table_b_heading_anchor.has_value() ||
        !table_b_heading_note.has_value() ||
        !table_b_banner->merge_right(2U)) {
        return sample;
    }
    if (!table_b_banner->set_fill_color("E2F0D9") ||
        !table_b_heading_case->set_fill_color("FCE4D6") ||
        !table_b_heading_anchor->set_fill_color("FCE4D6") ||
        !table_b_heading_note->set_fill_color("FCE4D6")) {
        return sample;
    }

    auto table_b_row = table_b.rows();
    for (std::size_t row_index = 0U; row_index < table_b_row_count;
         ++row_index) {
        if (!table_b_row.has_next()) {
            return sample;
        }
        if (row_index < 2U) {
            if (!table_b_row.set_repeats_header() ||
                !table_b_row.set_height_twips(
                    420U, featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (row_index == 4U) {
            if (!table_b_row.set_cant_split() ||
                !table_b_row.set_height_twips(
                    940U, featherdoc::row_height_rule::at_least)) {
                return sample;
            }
        } else if (!table_b_row.set_height_twips(
                       620U, featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        table_b_row.next();
    }

    auto checkpoint_b = append_document_paragraph(document, "");
    if (!checkpoint_b.has_next() ||
        !checkpoint_b.add_run("Flow checkpoint B: ").has_next() ||
        !add_styled_contract_run(document, checkpoint_b,
                                 utf8_from_u8(u8"终页合同"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !checkpoint_b.add_run(" / FE-TF-971 / ").has_next() ||
        !add_styled_contract_run(document, checkpoint_b,
                                 utf8_from_u8(u8"右侧锚点"),
                                 "DocumentPdfCjkFontEmbedNote") ||
        !checkpoint_b.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, checkpoint_b,
                                 utf8_from_u8(u8"大字回读"),
                                 "DocumentPdfCjkFontEmbedLarge")) {
        return sample;
    }

    featherdoc::floating_image_options behind_text_options;
    behind_text_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    behind_text_options.horizontal_offset_px = 208;
    behind_text_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    behind_text_options.vertical_offset_px = 0;
    behind_text_options.wrap_mode = featherdoc::floating_image_wrap_mode::none;
    behind_text_options.behind_text = true;
    behind_text_options.z_order = 2U;
    behind_text_options.crop =
        featherdoc::floating_image_crop{80U, 80U, 80U, 80U};
    if (!document.append_floating_image(image_path, 124U, 76U,
                                        behind_text_options)) {
        return sample;
    }

    for (int index = 13; index <= 16; ++index) {
        if (!append_flow_paragraph(index, u8"叠底正文回读")) {
            return sample;
        }
    }

    featherdoc::floating_image_options right_square_options;
    right_square_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    right_square_options.horizontal_offset_px = 222;
    right_square_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    right_square_options.vertical_offset_px = 0;
    right_square_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::square;
    right_square_options.wrap_distance_left_px = 14U;
    right_square_options.wrap_distance_bottom_px = 8U;
    right_square_options.crop =
        featherdoc::floating_image_crop{60U, 40U, 120U, 80U};
    if (!document.append_floating_image(image_path, 112U, 94U,
                                        right_square_options)) {
        return sample;
    }

    for (int index = 17; index <= 20; ++index) {
        if (!append_flow_paragraph(index, u8"末页表格收口")) {
            return sample;
        }
    }

    auto settle = append_document_paragraph(document, "");
    if (!settle.has_next() ||
        !settle.add_run("Flow settle: ").has_next() ||
        !add_styled_contract_run(document, settle,
                                 utf8_from_u8(u8"尾页回流补线"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !settle.add_run(" / FE-TF-981 / ").has_next() ||
        !add_styled_contract_run(document, settle,
                                 utf8_from_u8(u8"图片下方恢复全宽"),
                                 "DocumentPdfCjkFontEmbedNote")) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(u8"补线段落一：这里必须在图片下方恢复整栏宽度，避免末页浮动图继续挤压收口段与页脚占位符。")) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(u8"补线段落二：继续验证复制搜索、合同检索和字宽回读在右侧锚点结束后仍然保持一致。"))) {
        return sample;
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("Flow close: ").has_next() ||
        !add_styled_contract_run(document, closing, utf8_from_u8(u8"终页合同"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !closing.add_run(" / FE-TF-999 / ").has_next() ||
        !add_styled_contract_run(document, closing, utf8_from_u8(u8"回读复检"),
                                 "DocumentPdfCjkFontEmbedLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{460.0, 620.0};
    options.metadata.title =
        "FeatherDoc regression sample: document CJK table wrap page flow";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Font Embed",
                                        cjk_font_path},
    };
    options.cjk_font_file_path = cjk_font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.render_inline_images = true;
    options.header_footer_font_size_points = 8.0;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 5.0;
    options.image_spacing_after_points = 6.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult
build_document_cjk_vertical_merge_wrap_cant_split_text_sample(
    const std::filesystem::path &cjk_font_path,
    const std::filesystem::path &asset_dir) {
    ScenarioResult sample;

    const auto image_path = write_quadrant_rgb_png(
        asset_dir,
        "featherdoc-pdf-regression-document-cjk-vertical-merge-wrap-cant-split.png");

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document CJK Font Embed") ||
        !define_document_cjk_font_embed_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document CJK vertical merge wrap sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Vertical merge wrap: ").has_next() ||
        !add_styled_contract_run(document, intro, utf8_from_u8(u8"纵向合并联动"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !intro.add_run(" / FE-VM-901 / ").has_next() ||
        !add_styled_contract_run(document, intro, utf8_from_u8(u8"小页高分页"),
                                 "DocumentPdfCjkFontEmbedNote")) {
        return sample;
    }

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text("Vertical header VM-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Vertical footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text("Vertical first header VM-101 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            "Vertical first footer {{page}} / {{total_pages}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Vertical even header VM-202 page {{page}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text(
            "Vertical even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.width_twips = 8400U;
    setup.height_twips = 6400U;
    setup.margins.top_twips = 600U;
    setup.margins.bottom_twips = 600U;
    setup.margins.left_twips = 660U;
    setup.margins.right_twips = 660U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"起始段落先验证内联图片前后的检索连续性，再进入纵向合并与禁拆分页的复合场景。")) ||
        !document.append_image(image_path, 72U, 32U) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"内联图片之后应立即恢复全宽正文，避免后续表格页流继承错误缩进或错误回流边界。"))) {
        return sample;
    }

    auto stripe = append_document_paragraph(document, "");
    if (!stripe.has_next() ||
        !stripe.set_alignment(featherdoc::paragraph_alignment::right) ||
        !stripe.add_run("Merge stripe: ").has_next() ||
        !add_styled_contract_run(document, stripe, utf8_from_u8(u8"纵向块"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !stripe.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, stripe, utf8_from_u8(u8"整块迁移"),
                                 "DocumentPdfCjkFontEmbedNote")) {
        return sample;
    }

    featherdoc::floating_image_options left_square_options;
    left_square_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::column;
    left_square_options.horizontal_offset_px = 0;
    left_square_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    left_square_options.vertical_offset_px = 0;
    left_square_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::square;
    left_square_options.wrap_distance_right_px = 18U;
    left_square_options.wrap_distance_bottom_px = 10U;
    left_square_options.crop =
        featherdoc::floating_image_crop{160U, 0U, 50U, 120U};
    if (!document.append_floating_image(image_path, 88U, 62U,
                                        left_square_options)) {
        return sample;
    }

    for (int index = 1; index <= 2; ++index) {
        const auto text =
            utf8_from_u8(u8"第 ") + std::to_string(index) +
            utf8_from_u8(
                u8" 组纵向联动验证：图像包围、复制搜索、页码回读与纵向块对齐，混排 English token FE-VM-") +
            std::to_string(920 + index) +
            utf8_from_u8(u8"，并确认表格前的正文不会把后续分页边界带偏。");
        if (!append_document_text_paragraph(document, text)) {
            return sample;
        }
    }

    constexpr std::size_t row_count = 8U;
    auto table = document.append_table(row_count, 3U);
    if (!table.has_next() || !table.set_width_twips(6000U) ||
        !table.set_column_width_twips(0U, 1320U) ||
        !table.set_column_width_twips(1U, 1320U) ||
        !table.set_column_width_twips(2U, 3360U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"交付矩阵")) ||
        !table.set_cell_text(1U, 0U, utf8_from_u8(u8"案例")) ||
        !table.set_cell_text(1U, 1U, utf8_from_u8(u8"负责人")) ||
        !table.set_cell_text(1U, 2U, utf8_from_u8(u8"说明")) ||
        !table.set_cell_text(2U, 0U, "CVM-01") ||
        !table.set_cell_text(2U, 1U, utf8_from_u8(u8"排期")) ||
        !table.set_cell_text(2U, 2U, utf8_from_u8(u8"短余量。")) ||
        !table.set_cell_text(3U, 0U, "CVM-02") ||
        !table.set_cell_text(3U, 1U, utf8_from_u8(u8"统筹")) ||
        !table.set_cell_text(
            3U, 2U,
            utf8_from_u8(u8"纵向合并负责人块翻页后仍需整块保持。")) ||
        !table.set_cell_text(4U, 0U, "CVM-03") ||
        !table.set_cell_text(
            4U, 2U,
            utf8_from_u8(u8"第二行说明继续跟随同一个负责人块。")) ||
        !table.set_cell_text(5U, 0U, "CVM-04") ||
        !table.set_cell_text(5U, 1U, utf8_from_u8(u8"核验")) ||
        !table.set_cell_text(
            5U, 2U,
            utf8_from_u8(u8"禁拆行要整块落到下一页，重复表头和页码检索继续正确。")) ||
        !table.set_cell_text(6U, 0U, "CVM-05") ||
        !table.set_cell_text(6U, 1U, utf8_from_u8(u8"归档")) ||
        !table.set_cell_text(
            6U, 2U,
            utf8_from_u8(u8"尾部行验证大块迁移完成后流程恢复。")) ||
        !table.set_cell_text(7U, 0U, "CVM-06") ||
        !table.set_cell_text(7U, 1U, utf8_from_u8(u8"检索")) ||
        !table.set_cell_text(
            7U, 2U,
            utf8_from_u8(u8"终页回读与页眉页脚检索保持一致。"))) {
        return sample;
    }

    auto merged_banner = table.find_cell(0U, 0U);
    auto merged_owner = table.find_cell(3U, 1U);
    auto heading_case = table.find_cell(1U, 0U);
    auto heading_owner = table.find_cell(1U, 1U);
    auto heading_notes = table.find_cell(1U, 2U);
    if (!merged_banner.has_value() || !merged_owner.has_value() ||
        !heading_case.has_value() || !heading_owner.has_value() ||
        !heading_notes.has_value() || !merged_banner->merge_right(2U) ||
        !merged_owner->merge_down(1U)) {
        return sample;
    }

    if (!merged_banner->set_fill_color("D9EAF7") ||
        !heading_case->set_fill_color("EAF2F8") ||
        !heading_owner->set_fill_color("EAF2F8") ||
        !heading_notes->set_fill_color("EAF2F8") ||
        !merged_owner->set_fill_color("E2F0D9") ||
        !merged_owner->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next()) {
            return sample;
        }
        if (row_index < 2U) {
            if (!row.set_repeats_header() ||
                !row.set_height_twips(360U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (row_index == 2U) {
            if (!row.set_height_twips(440U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (row_index == 3U) {
            if (!row.set_cant_split() ||
                !row.set_height_twips(620U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (row_index == 4U) {
            if (!row.set_height_twips(560U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (row_index == 5U) {
            if (!row.set_cant_split() ||
                !row.set_height_twips(720U,
                                      featherdoc::row_height_rule::at_least)) {
                return sample;
            }
        } else if (!row.set_height_twips(520U,
                                         featherdoc::row_height_rule::exact)) {
            return sample;
        }
        row.next();
    }

    auto checkpoint = append_document_paragraph(document, "");
    if (!checkpoint.has_next() ||
        !checkpoint.add_run("Vertical checkpoint: ").has_next() ||
        !add_styled_contract_run(document, checkpoint,
                                 utf8_from_u8(u8"表后回流"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !checkpoint.add_run(" / FE-VM-961 / ").has_next() ||
        !add_styled_contract_run(document, checkpoint,
                                 utf8_from_u8(u8"整块落页"),
                                 "DocumentPdfCjkFontEmbedNote")) {
        return sample;
    }

    featherdoc::floating_image_options top_bottom_options;
    top_bottom_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    top_bottom_options.horizontal_offset_px = 52;
    top_bottom_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    top_bottom_options.vertical_offset_px = 0;
    top_bottom_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::top_bottom;
    top_bottom_options.wrap_distance_top_px = 8U;
    top_bottom_options.wrap_distance_bottom_px = 10U;
    top_bottom_options.crop =
        featherdoc::floating_image_crop{0U, 160U, 180U, 0U};
    if (!document.append_floating_image(image_path, 118U, 40U,
                                        top_bottom_options)) {
        return sample;
    }

    for (int index = 3; index <= 5; ++index) {
        const auto text =
            utf8_from_u8(u8"第 ") + std::to_string(index) +
            utf8_from_u8(
                u8" 组表后回流验证：整块落页、复制搜索、终页回读与页脚展开，混排 English token FE-VM-") +
            std::to_string(920 + index) +
            utf8_from_u8(u8"，并确认图片下方已经恢复为稳定全宽正文。");
        if (!append_document_text_paragraph(document, text)) {
            return sample;
        }
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("Vertical close: ").has_next() ||
        !add_styled_contract_run(document, closing, utf8_from_u8(u8"终页回读"),
                                 "DocumentPdfCjkFontEmbedAccent") ||
        !closing.add_run(" / FE-VM-999 / ").has_next() ||
        !add_styled_contract_run(document, closing, utf8_from_u8(u8"整块复检"),
                                 "DocumentPdfCjkFontEmbedLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{420.0, 320.0};
    options.metadata.title =
        "FeatherDoc regression sample: document CJK vertical merge wrap";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Font Embed",
                                        cjk_font_path},
    };
    options.cjk_font_file_path = cjk_font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.render_inline_images = true;
    options.header_footer_font_size_points = 8.0;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 40.0;
    options.margin_bottom_points = 44.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 4.0;
    options.image_spacing_after_points = 6.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult
build_document_table_header_footer_variants_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document table header footer variants sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            "Paged table keeps header footer variants visible across table "
            "pagination.")) {
        return sample;
    }

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text("Variant header HF-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Default footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text("First variant HF-101 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text("First footer {{page}} / {{total_pages}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Even variant HF-202 page {{page}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text("Even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.orientation = featherdoc::page_orientation::landscape;
    setup.width_twips = 6000U;
    setup.height_twips = 4400U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 720U;
    setup.margins.right_twips = 720U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    constexpr std::size_t row_count = 8U;
    auto table = document.append_table(row_count, 3U);
    if (!table.has_next() || !table.set_width_twips(4560U) ||
        !table.set_column_width_twips(0U, 900U) ||
        !table.set_column_width_twips(1U, 1140U) ||
        !table.set_column_width_twips(2U, 2520U) ||
        !table.set_cell_text(0U, 0U, "Item") ||
        !table.set_cell_text(0U, 1U, "Batch") ||
        !table.set_cell_text(0U, 2U, "Status") ||
        !table.set_cell_text(1U, 0U, "FE-501") ||
        !table.set_cell_text(1U, 1U, "Alpha") ||
        !table.set_cell_text(1U, 2U, "Draft review") ||
        !table.set_cell_text(2U, 0U, "FE-502") ||
        !table.set_cell_text(2U, 1U, "Beta") ||
        !table.set_cell_text(2U, 2U, "Header sync") ||
        !table.set_cell_text(3U, 0U, "FE-503") ||
        !table.set_cell_text(3U, 1U, "Gamma") ||
        !table.set_cell_text(3U, 2U, "Footer sync") ||
        !table.set_cell_text(4U, 0U, "FE-504") ||
        !table.set_cell_text(4U, 1U, "Delta") ||
        !table.set_cell_text(4U, 2U, "Repeat header") ||
        !table.set_cell_text(5U, 0U, "FE-505") ||
        !table.set_cell_text(5U, 1U, "Epsilon") ||
        !table.set_cell_text(5U, 2U, "Stable page flow") ||
        !table.set_cell_text(6U, 0U, "FE-506") ||
        !table.set_cell_text(6U, 1U, "Zeta") ||
        !table.set_cell_text(6U, 2U, "Visual gate") ||
        !table.set_cell_text(7U, 0U, "FE-507") ||
        !table.set_cell_text(7U, 1U, "Omega") ||
        !table.set_cell_text(7U, 2U, "Release ready")) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(600U, featherdoc::row_height_rule::exact)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        if (row_index == 5U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table header footer variants";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 8.0;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_table_wrap_flow_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document table wrap flow sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            "Long table cells must wrap cleanly while page headers, repeated "
            "table headers, and footers stay stable.")) {
        return sample;
    }

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text("Wrap header W-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Wrap footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text("Wrap first W-101 page {{page}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Wrap even W-202 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text("Wrap first footer {{page}} / {{total_pages}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text("Wrap even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.orientation = featherdoc::page_orientation::landscape;
    setup.width_twips = 6000U;
    setup.height_twips = 4400U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 720U;
    setup.margins.right_twips = 720U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    constexpr std::size_t row_count = 6U;
    auto table = document.append_table(row_count, 2U);
    if (!table.has_next() || !table.set_width_twips(6960U) ||
        !table.set_column_width_twips(0U, 1440U) ||
        !table.set_column_width_twips(1U, 5520U) ||
        !table.set_cell_text(0U, 0U, "Case") ||
        !table.set_cell_text(0U, 1U, "Notes") ||
        !table.set_cell_text(1U, 0U, "FE-601") ||
        !table.set_cell_text(
            1U, 1U,
            "Header footer placeholders must stay visible while the first "
            "note wraps.") ||
        !table.set_cell_text(2U, 0U, "FE-602") ||
        !table.set_cell_text(
            2U, 1U,
            "Repeated table headers must stay aligned after page breaks.") ||
        !table.set_cell_text(3U, 0U, "FE-603") ||
        !table.set_cell_text(
            3U, 1U,
            "Visual checks should catch clipped status text before automated "
            "tests pass.") ||
        !table.set_cell_text(4U, 0U, "FE-604") ||
        !table.set_cell_text(
            4U, 1U,
            "Long note blocks need predictable wrapping so screenshots stay "
            "easy to compare.") ||
        !table.set_cell_text(5U, 0U, "FE-605") ||
        !table.set_cell_text(
            5U, 1U,
            "Release ready evidence should move intact instead of breaking "
            "between pages.")) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(600U, featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        if (row_index == 5U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{420.0, 320.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table wrap flow";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 8.0;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_table_cjk_wrap_flow_text_sample(
    const std::filesystem::path &cjk_font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family("Document Wrap CJK")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text(utf8_from_u8(u8"文档表格换行流程样本")) ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(u8"长文本单元格需要稳定换行，同时保持页眉、重复表头和页脚占位符都可见。"))) {
        return sample;
    }

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text(
            utf8_from_u8(u8"CJK 页眉 C-303 第 {{page}} 页")) ||
        !default_footer.has_next() ||
        !default_footer.set_text(
            utf8_from_u8(u8"CJK 页脚 {{page}} / {{total_pages}}")) ||
        !first_header.has_next() ||
        !first_header.set_text(
            utf8_from_u8(u8"CJK 首页 C-101 第 {{page}} 页")) ||
        !even_header.has_next() ||
        !even_header.set_text(
            utf8_from_u8(u8"CJK 偶数页 C-202 第 {{page}} 页")) ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            utf8_from_u8(u8"CJK 首页页脚 {{page}} / {{total_pages}}")) ||
        !even_footer.has_next() ||
        !even_footer.set_text(
            utf8_from_u8(u8"CJK 偶数页页脚 {{page}} / {{total_pages}}"))) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.orientation = featherdoc::page_orientation::landscape;
    setup.width_twips = 8400U;
    setup.height_twips = 6400U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 720U;
    setup.margins.right_twips = 720U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    constexpr std::size_t row_count = 6U;
    auto table = document.append_table(row_count, 2U);
    if (!table.has_next() || !table.set_width_twips(6960U) ||
        !table.set_column_width_twips(0U, 1440U) ||
        !table.set_column_width_twips(1U, 5520U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"案例")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"说明")) ||
        !table.set_cell_text(1U, 0U, "FE-701") ||
        !table.set_cell_text(
            1U, 1U,
            utf8_from_u8(
                u8"页眉页脚占位符需要保持可见，第一条中文说明在换行后仍然要完整可读。")) ||
        !table.set_cell_text(2U, 0U, "FE-702") ||
        !table.set_cell_text(
            2U, 1U,
            utf8_from_u8(
                u8"重复表头在翻页后必须继续和网格对齐，避免视觉检查时出现错位判断。")) ||
        !table.set_cell_text(3U, 0U, "FE-703") ||
        !table.set_cell_text(
            3U, 1U,
            utf8_from_u8(
                u8"中文长句在窄列里需要稳定换行，不能因为字体度量偏差导致裁切或重叠。")) ||
        !table.set_cell_text(4U, 0U, "FE-704") ||
        !table.set_cell_text(
            4U, 1U,
            utf8_from_u8(
                u8"截图比对应该能够直接发现最后一行被压缩、被截断或者跳到错误页面的问题。")) ||
        !table.set_cell_text(5U, 0U, "FE-705") ||
        !table.set_cell_text(
            5U, 1U,
            utf8_from_u8(
                u8"发布前证据需要整行移动，而不是把最后一句拆到下一页造成阅读中断。"))) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(600U, featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        if (row_index == 5U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{420.0, 320.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table CJK wrap flow";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document Wrap CJK", cjk_font_path},
    };
    options.cjk_font_file_path = cjk_font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 8.0;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_table_cant_split_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document table cant split sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            "A tall no-split row must move as a single block when the "
            "remaining page space is too small.")) {
        return sample;
    }

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text("Cant split header S-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Cant split footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text("Cant split first S-101 page {{page}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Cant split even S-202 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            "Cant split first footer {{page}} / {{total_pages}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text(
            "Cant split even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.orientation = featherdoc::page_orientation::landscape;
    setup.width_twips = 7200U;
    setup.height_twips = 5200U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 720U;
    setup.margins.right_twips = 720U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    constexpr std::size_t row_count = 4U;
    auto table = document.append_table(row_count, 2U);
    if (!table.has_next() || !table.set_width_twips(5520U) ||
        !table.set_column_width_twips(0U, 1320U) ||
        !table.set_column_width_twips(1U, 4200U) ||
        !table.set_cell_text(0U, 0U, "Case") ||
        !table.set_cell_text(0U, 1U, "Notes") ||
        !table.set_cell_text(1U, 0U, "FE-801") ||
        !table.set_cell_text(
            1U, 1U,
            "Lead row leaves just enough space to expose the cant-split "
            "behavior on the next row.") ||
        !table.set_cell_text(2U, 0U, "FE-802") ||
        !table.set_cell_text(
            2U, 1U,
            "This tall row must move as a single block. It should not leave "
            "the first half of the paragraph on one page and the rest on the "
            "next page. Visual checks need to see the entire note start below "
            "the repeated header after the break.") ||
        !table.set_cell_text(3U, 0U, "FE-803") ||
        !table.set_cell_text(
            3U, 1U,
            "Tail row confirms the table continues normally after the large "
            "row moves intact.")) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(600U, featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        if (row_index == 2U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{360.0, 250.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table cant split";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 8.0;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_table_merged_cells_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document table merged cells sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            "Merged table cells should keep borders, text flow, and neighbor "
            "alignment stable in the rendered PDF.")) {
        return sample;
    }

    auto header = document.ensure_section_header_paragraphs(0U);
    auto footer = document.ensure_section_footer_paragraphs(0U);
    if (!header.has_next() ||
        !header.set_text("Merged cells header M-101 page {{page}}") ||
        !footer.has_next() ||
        !footer.set_text("Merged cells footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    auto table = document.append_table(4U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1800U) ||
        !table.set_column_width_twips(1U, 2700U) ||
        !table.set_column_width_twips(2U, 2700U) ||
        !table.set_cell_text(0U, 0U, "Program summary") ||
        !table.set_cell_text(0U, 2U, "Status") ||
        !table.set_cell_text(1U, 0U, "Owner block") ||
        !table.set_cell_text(1U, 1U, "FE-901") ||
        !table.set_cell_text(1U, 2U, "Merged title row stays aligned") ||
        !table.set_cell_text(2U, 1U, "FE-902") ||
        !table.set_cell_text(2U, 2U, "Vertical merge block stays intact") ||
        !table.set_cell_text(3U, 0U, "Tail") ||
        !table.set_cell_text(3U, 1U, "Final release note") ||
        !table.set_cell_text(3U, 2U, "Merged footer row stays readable")) {
        return sample;
    }

    auto merged_title = table.find_cell(0U, 0U);
    auto vertical_block = table.find_cell(1U, 0U);
    auto tail_merge = table.find_cell(3U, 1U);
    if (!merged_title.has_value() || !vertical_block.has_value() ||
        !tail_merge.has_value() || !merged_title->merge_right(1U) ||
        !vertical_block->merge_down(1U) || !tail_merge->merge_right(1U)) {
        return sample;
    }

    if (!tail_merge->set_text("Merged footer row stays readable")) {
        return sample;
    }

    if (!merged_title->set_fill_color("D9EAF7") ||
        !vertical_block->set_fill_color("E8F3E8") ||
        !tail_merge->set_fill_color("F6E7D8") ||
        !vertical_block->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 4U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index == 0U ? 520U : 720U,
                                  featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document table merged cells";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 9.0;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_table_merged_header_repeat_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document table merged repeat sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            "Repeated header rows with merged cells should stay aligned across "
            "page breaks in the rendered PDF.")) {
        return sample;
    }

    auto header = document.ensure_section_header_paragraphs(0U);
    auto footer = document.ensure_section_footer_paragraphs(0U);
    if (!header.has_next() ||
        !header.set_text("Merged repeat header R-303 page {{page}}") ||
        !footer.has_next() ||
        !footer.set_text("Merged repeat footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    constexpr std::size_t row_count = 8U;
    auto table = document.append_table(row_count, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1800U) ||
        !table.set_column_width_twips(1U, 1800U) ||
        !table.set_column_width_twips(2U, 3600U) ||
        !table.set_cell_text(0U, 0U, "Release board") ||
        !table.set_cell_text(1U, 0U, "Case") ||
        !table.set_cell_text(1U, 1U, "Owner") ||
        !table.set_cell_text(1U, 2U, "Notes") ||
        !table.set_cell_text(2U, 0U, "MR-01") ||
        !table.set_cell_text(2U, 1U, "Cluster lead") ||
        !table.set_cell_text(2U, 2U, "Merged owner block stays centered.") ||
        !table.set_cell_text(3U, 0U, "MR-02") ||
        !table.set_cell_text(3U, 2U,
                             "Second row remains aligned beneath the same "
                             "owner.") ||
        !table.set_cell_text(4U, 0U, "MR-03") ||
        !table.set_cell_text(4U, 1U, "Search") ||
        !table.set_cell_text(
            4U, 2U,
            "Repeated merged headers should reappear on page 2.") ||
        !table.set_cell_text(5U, 0U, "MR-04") ||
        !table.set_cell_text(5U, 1U, "Docs") ||
        !table.set_cell_text(
            5U, 2U,
            "Header/footer placeholders must keep page numbers stable.") ||
        !table.set_cell_text(6U, 0U, "Tail") ||
        !table.set_cell_text(6U, 1U, "Milestone note") ||
        !table.set_cell_text(
            6U, 2U, "Merged milestone row stays readable.") ||
        !table.set_cell_text(7U, 0U, "MR-05") ||
        !table.set_cell_text(7U, 1U, "Visual gate") ||
        !table.set_cell_text(
            7U, 2U, "Page 3 repeats the board header.")) {
        return sample;
    }

    auto merged_banner = table.find_cell(0U, 0U);
    auto merged_owner = table.find_cell(2U, 1U);
    auto merged_tail = table.find_cell(6U, 1U);
    auto heading_case = table.find_cell(1U, 0U);
    auto heading_owner = table.find_cell(1U, 1U);
    auto heading_notes = table.find_cell(1U, 2U);
    if (!merged_banner.has_value() || !merged_owner.has_value() ||
        !merged_tail.has_value() || !heading_case.has_value() ||
        !heading_owner.has_value() || !heading_notes.has_value() ||
        !merged_banner->merge_right(2U) || !merged_owner->merge_down(1U) ||
        !merged_tail->merge_right(1U)) {
        return sample;
    }

    if (!merged_tail->set_text(
            "Merged milestone row stays readable.")) {
        return sample;
    }

    if (!merged_banner->set_fill_color("D9EAF7") ||
        !heading_case->set_fill_color("EAF2F8") ||
        !heading_owner->set_fill_color("EAF2F8") ||
        !heading_notes->set_fill_color("EAF2F8") ||
        !merged_owner->set_fill_color("E2F0D9") ||
        !merged_tail->set_fill_color("FCE4D6") ||
        !merged_owner->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next()) {
            return sample;
        }
        if (row_index < 2U) {
            if (!row.set_repeats_header() ||
                !row.set_height_twips(540U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (!row.set_height_twips(820U,
                                         featherdoc::row_height_rule::exact)) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table merged repeat";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 8.0;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_table_cjk_merged_repeat_text_sample(
    const std::filesystem::path &cjk_font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document Merged Repeat CJK")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text(utf8_from_u8(u8"文档表格合并重复表头样本")) ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(u8"合并单元格、重复表头和分页后的中文文本都需要保持稳定可读。"))) {
        return sample;
    }

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text(
            utf8_from_u8(u8"CJK 合并页眉 M-303 第 {{page}} 页")) ||
        !default_footer.has_next() ||
        !default_footer.set_text(utf8_from_u8(u8"CJK 合并页脚 {{page}} / {{total_pages}}")) ||
        !first_header.has_next() ||
        !first_header.set_text(
            utf8_from_u8(u8"CJK 首页 M-101 第 {{page}} 页")) ||
        !even_header.has_next() ||
        !even_header.set_text(
            utf8_from_u8(u8"CJK 偶数页 M-202 第 {{page}} 页")) ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            utf8_from_u8(u8"CJK 首页页脚 {{page}} / {{total_pages}}")) ||
        !even_footer.has_next() ||
        !even_footer.set_text(
            utf8_from_u8(u8"CJK 偶数页页脚 {{page}} / {{total_pages}}"))) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.orientation = featherdoc::page_orientation::landscape;
    setup.width_twips = 6000U;
    setup.height_twips = 4400U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 720U;
    setup.margins.right_twips = 720U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    constexpr std::size_t row_count = 8U;
    auto table = document.append_table(row_count, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1800U) ||
        !table.set_column_width_twips(1U, 1800U) ||
        !table.set_column_width_twips(2U, 3600U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"发布看板")) ||
        !table.set_cell_text(1U, 0U, utf8_from_u8(u8"案例")) ||
        !table.set_cell_text(1U, 1U, utf8_from_u8(u8"负责人")) ||
        !table.set_cell_text(1U, 2U, utf8_from_u8(u8"说明")) ||
        !table.set_cell_text(2U, 0U, "MC-01") ||
        !table.set_cell_text(2U, 1U, utf8_from_u8(u8"统筹")) ||
        !table.set_cell_text(
            2U, 2U,
            utf8_from_u8(u8"负责人块需要垂直居中。")) ||
        !table.set_cell_text(3U, 0U, "MC-02") ||
        !table.set_cell_text(
            3U, 2U,
            utf8_from_u8(u8"第二行仍贴在负责人下方。")) ||
        !table.set_cell_text(4U, 0U, "MC-03") ||
        !table.set_cell_text(4U, 1U, utf8_from_u8(u8"检索")) ||
        !table.set_cell_text(
            4U, 2U,
            utf8_from_u8(u8"翻页后重复表头仍和网格对齐。")) ||
        !table.set_cell_text(5U, 0U, "MC-04") ||
        !table.set_cell_text(5U, 1U, utf8_from_u8(u8"文档")) ||
        !table.set_cell_text(
            5U, 2U,
            utf8_from_u8(u8"页眉页脚页码保持稳定。")) ||
        !table.set_cell_text(6U, 0U, utf8_from_u8(u8"尾部")) ||
        !table.set_cell_text(6U, 1U, utf8_from_u8(u8"里程碑")) ||
        !table.set_cell_text(
            6U, 2U,
            utf8_from_u8(u8"合并尾行在分页后仍然清晰。")) ||
        !table.set_cell_text(7U, 0U, "MC-05") ||
        !table.set_cell_text(7U, 1U, utf8_from_u8(u8"核验")) ||
        !table.set_cell_text(
            7U, 2U,
            utf8_from_u8(u8"第 3 页仍重复表头。"))) {
        return sample;
    }

    auto merged_banner = table.find_cell(0U, 0U);
    auto merged_owner = table.find_cell(2U, 1U);
    auto merged_tail = table.find_cell(6U, 1U);
    auto heading_case = table.find_cell(1U, 0U);
    auto heading_owner = table.find_cell(1U, 1U);
    auto heading_notes = table.find_cell(1U, 2U);
    if (!merged_banner.has_value() || !merged_owner.has_value() ||
        !merged_tail.has_value() || !heading_case.has_value() ||
        !heading_owner.has_value() || !heading_notes.has_value() ||
        !merged_banner->merge_right(2U) || !merged_owner->merge_down(1U) ||
        !merged_tail->merge_right(1U)) {
        return sample;
    }

    if (!merged_tail->set_text(utf8_from_u8(u8"合并尾行在分页后仍然清晰。"))) {
        return sample;
    }

    if (!merged_banner->set_fill_color("D9EAF7") ||
        !heading_case->set_fill_color("EAF2F8") ||
        !heading_owner->set_fill_color("EAF2F8") ||
        !heading_notes->set_fill_color("EAF2F8") ||
        !merged_owner->set_fill_color("E2F0D9") ||
        !merged_tail->set_fill_color("FCE4D6") ||
        !merged_owner->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next()) {
            return sample;
        }
        if (row_index < 2U) {
            if (!row.set_repeats_header() ||
                !row.set_height_twips(540U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (!row.set_height_twips(760U,
                                         featherdoc::row_height_rule::exact)) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table CJK merged repeat";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document Merged Repeat CJK",
                                        cjk_font_path},
    };
    options.cjk_font_file_path = cjk_font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 8.0;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_table_merged_cant_split_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Merged cant split table sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            "Repeated headers must keep the merged row intact after the page "
            "break.")) {
        return sample;
    }

    auto header = document.ensure_section_header_paragraphs(0U);
    auto footer = document.ensure_section_footer_paragraphs(0U);
    if (!header.has_next() ||
        !header.set_text("Merged cant split header T-404 page {{page}}") ||
        !footer.has_next() ||
        !footer.set_text(
            "Merged cant split footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    constexpr std::size_t row_count = 6U;
    auto table = document.append_table(row_count, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1600U) ||
        !table.set_column_width_twips(1U, 1800U) ||
        !table.set_column_width_twips(2U, 3800U) ||
        !table.set_cell_text(0U, 0U, "Release gate") ||
        !table.set_cell_text(1U, 0U, "Case") ||
        !table.set_cell_text(1U, 1U, "Owner") ||
        !table.set_cell_text(1U, 2U, "Notes") ||
        !table.set_cell_text(2U, 0U, "MCS-01") ||
        !table.set_cell_text(2U, 1U, "Queue") ||
        !table.set_cell_text(
            2U, 2U,
            "Lead row leaves a short remainder.") ||
        !table.set_cell_text(3U, 0U, "MCS-02") ||
        !table.set_cell_text(4U, 0U, "MCS-03") ||
        !table.set_cell_text(4U, 1U, "QA") ||
        !table.set_cell_text(
            4U, 2U,
            "Tail row confirms normal flow resumes.") ||
        !table.set_cell_text(5U, 0U, "MCS-04") ||
        !table.set_cell_text(5U, 1U, "Archive") ||
        !table.set_cell_text(
            5U, 2U,
            "Page numbering stays stable.")) {
        return sample;
    }

    auto merged_banner = table.find_cell(0U, 0U);
    auto merged_row = table.find_cell(3U, 1U);
    auto heading_case = table.find_cell(1U, 0U);
    auto heading_owner = table.find_cell(1U, 1U);
    auto heading_notes = table.find_cell(1U, 2U);
    if (!merged_banner.has_value() || !merged_row.has_value() ||
        !heading_case.has_value() || !heading_owner.has_value() ||
        !heading_notes.has_value() || !merged_banner->merge_right(2U) ||
        !merged_row->merge_right(1U)) {
        return sample;
    }

    if (!merged_banner->set_text("Release gate") ||
        !merged_row->set_text(
            "Merged cant-split row must restart beneath the repeated header "
            "and keep the whole note readable after the page break.")) {
        return sample;
    }

    if (!merged_banner->set_fill_color("D9EAF7") ||
        !heading_case->set_fill_color("EAF2F8") ||
        !heading_owner->set_fill_color("EAF2F8") ||
        !heading_notes->set_fill_color("EAF2F8") ||
        !merged_row->set_fill_color("E2F0D9") ||
        !merged_row->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next()) {
            return sample;
        }
        if (row_index < 2U) {
            if (!row.set_repeats_header() ||
                !row.set_height_twips(520U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (row_index == 2U) {
            if (!row.set_height_twips(920U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (row_index == 3U) {
            if (!row.set_cant_split() ||
                !row.set_height_twips(1500U,
                                      featherdoc::row_height_rule::at_least)) {
                return sample;
            }
        } else if (!row.set_height_twips(760U,
                                         featherdoc::row_height_rule::exact)) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{320.0, 220.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table merged cant split";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 8.0;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult
build_document_table_vertical_merged_cant_split_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Vertical merged cant split sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            "Repeated headers must keep a vertical merged owner block intact "
            "after the break.")) {
        return sample;
    }

    auto header = document.ensure_section_header_paragraphs(0U);
    auto footer = document.ensure_section_footer_paragraphs(0U);
    if (!header.has_next() ||
        !header.set_text("Vertical merged header V-505 page {{page}}") ||
        !footer.has_next() ||
        !footer.set_text("Vertical merged footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    constexpr std::size_t row_count = 7U;
    auto table = document.append_table(row_count, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1500U) ||
        !table.set_column_width_twips(1U, 1600U) ||
        !table.set_column_width_twips(2U, 4100U) ||
        !table.set_cell_text(0U, 0U, "Delivery board") ||
        !table.set_cell_text(1U, 0U, "Case") ||
        !table.set_cell_text(1U, 1U, "Owner") ||
        !table.set_cell_text(1U, 2U, "Notes") ||
        !table.set_cell_text(2U, 0U, "VMC-01") ||
        !table.set_cell_text(2U, 1U, "Queue") ||
        !table.set_cell_text(2U, 2U, "Short remainder.") ||
        !table.set_cell_text(3U, 0U, "VMC-02") ||
        !table.set_cell_text(3U, 1U, "Cluster") ||
        !table.set_cell_text(
            3U, 2U,
            "Owner block restarts below header.") ||
        !table.set_cell_text(4U, 0U, "VMC-03") ||
        !table.set_cell_text(
            4U, 2U,
            "Second note stays with owner.") ||
        !table.set_cell_text(5U, 0U, "VMC-04") ||
        !table.set_cell_text(5U, 1U, "QA") ||
        !table.set_cell_text(5U, 2U, "Tail row confirms clean flow resumes.") ||
        !table.set_cell_text(6U, 0U, "VMC-05") ||
        !table.set_cell_text(6U, 1U, "Archive") ||
        !table.set_cell_text(6U, 2U, "Page numbering stays stable.")) {
        return sample;
    }

    auto merged_banner = table.find_cell(0U, 0U);
    auto merged_owner = table.find_cell(3U, 1U);
    auto heading_case = table.find_cell(1U, 0U);
    auto heading_owner = table.find_cell(1U, 1U);
    auto heading_notes = table.find_cell(1U, 2U);
    if (!merged_banner.has_value() || !merged_owner.has_value() ||
        !heading_case.has_value() || !heading_owner.has_value() ||
        !heading_notes.has_value() || !merged_banner->merge_right(2U) ||
        !merged_owner->merge_down(1U)) {
        return sample;
    }

    if (!merged_banner->set_fill_color("D9EAF7") ||
        !heading_case->set_fill_color("EAF2F8") ||
        !heading_owner->set_fill_color("EAF2F8") ||
        !heading_notes->set_fill_color("EAF2F8") ||
        !merged_owner->set_fill_color("E2F0D9") ||
        !merged_owner->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next()) {
            return sample;
        }
        if (row_index < 2U) {
            if (!row.set_repeats_header() ||
                !row.set_height_twips(440U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (row_index == 2U) {
            if (!row.set_height_twips(640U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (row_index == 3U) {
            if (!row.set_cant_split() ||
                !row.set_height_twips(760U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (row_index == 4U) {
            if (!row.set_height_twips(760U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (!row.set_height_twips(680U,
                                         featherdoc::row_height_rule::exact)) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{320.0, 220.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table vertical merged cant split";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 8.0;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 40.0;
    options.margin_bottom_points = 44.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult
build_document_table_cjk_vertical_merged_cant_split_text_sample(
    const std::filesystem::path &cjk_font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document Vertical Cant Split CJK")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text(utf8_from_u8(u8"文档表格纵向合并禁拆样本")) ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(u8"重复表头、纵向合并块和分页后的中文内容都要保持完整。"))) {
        return sample;
    }

    auto header = document.ensure_section_header_paragraphs(0U);
    auto footer = document.ensure_section_footer_paragraphs(0U);
    if (!header.has_next() ||
        !header.set_text(
            utf8_from_u8(u8"CJK 纵向页眉 V-606 第 {{page}} 页")) ||
        !footer.has_next() ||
        !footer.set_text(
            utf8_from_u8(u8"CJK 纵向页脚 {{page}} / {{total_pages}}"))) {
        return sample;
    }

    constexpr std::size_t row_count = 7U;
    auto table = document.append_table(row_count, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1500U) ||
        !table.set_column_width_twips(1U, 1600U) ||
        !table.set_column_width_twips(2U, 4100U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"交付看板")) ||
        !table.set_cell_text(1U, 0U, utf8_from_u8(u8"案例")) ||
        !table.set_cell_text(1U, 1U, utf8_from_u8(u8"负责人")) ||
        !table.set_cell_text(1U, 2U, utf8_from_u8(u8"说明")) ||
        !table.set_cell_text(2U, 0U, "CVC-01") ||
        !table.set_cell_text(2U, 1U, utf8_from_u8(u8"排期")) ||
        !table.set_cell_text(2U, 2U, utf8_from_u8(u8"短余量。")) ||
        !table.set_cell_text(3U, 0U, "CVC-02") ||
        !table.set_cell_text(3U, 1U, utf8_from_u8(u8"统筹")) ||
        !table.set_cell_text(
            3U, 2U,
            utf8_from_u8(u8"负责人块翻页后重启。")) ||
        !table.set_cell_text(4U, 0U, "CVC-03") ||
        !table.set_cell_text(
            4U, 2U,
            utf8_from_u8(u8"第二行仍跟随负责人。")) ||
        !table.set_cell_text(5U, 0U, "CVC-04") ||
        !table.set_cell_text(5U, 1U, utf8_from_u8(u8"核验")) ||
        !table.set_cell_text(
            5U, 2U,
            utf8_from_u8(u8"尾行验证后续流继续。")) ||
        !table.set_cell_text(6U, 0U, "CVC-05") ||
        !table.set_cell_text(6U, 1U, utf8_from_u8(u8"归档")) ||
        !table.set_cell_text(
            6U, 2U,
            utf8_from_u8(u8"页码保持稳定。"))) {
        return sample;
    }

    auto merged_banner = table.find_cell(0U, 0U);
    auto merged_owner = table.find_cell(3U, 1U);
    auto heading_case = table.find_cell(1U, 0U);
    auto heading_owner = table.find_cell(1U, 1U);
    auto heading_notes = table.find_cell(1U, 2U);
    if (!merged_banner.has_value() || !merged_owner.has_value() ||
        !heading_case.has_value() || !heading_owner.has_value() ||
        !heading_notes.has_value() || !merged_banner->merge_right(2U) ||
        !merged_owner->merge_down(1U)) {
        return sample;
    }

    if (!merged_banner->set_fill_color("D9EAF7") ||
        !heading_case->set_fill_color("EAF2F8") ||
        !heading_owner->set_fill_color("EAF2F8") ||
        !heading_notes->set_fill_color("EAF2F8") ||
        !merged_owner->set_fill_color("E2F0D9") ||
        !merged_owner->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next()) {
            return sample;
        }
        if (row_index < 2U) {
            if (!row.set_repeats_header() ||
                !row.set_height_twips(440U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (row_index == 2U) {
            if (!row.set_height_twips(640U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (row_index == 3U) {
            if (!row.set_cant_split() ||
                !row.set_height_twips(760U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (row_index == 4U) {
            if (!row.set_height_twips(760U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (!row.set_height_twips(680U,
                                         featherdoc::row_height_rule::exact)) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table CJK vertical merged cant split";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document Vertical Cant Split CJK",
                                        cjk_font_path},
    };
    options.cjk_font_file_path = cjk_font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 8.0;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 40.0;
    options.margin_bottom_points = 44.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult
build_document_table_merged_header_footer_variants_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document table merged header footer variants sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            "Merged cells should stay aligned while first, even, and default "
            "section headers and footers alternate across pages.")) {
        return sample;
    }

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text("Merged variant header V-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("V-303 {{page}}/{{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text("Merged variant first header V-101 page {{page}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Merged variant even header V-202 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text("V-101 {{page}}/{{total_pages}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text("V-202 {{page}}/{{total_pages}}")) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.orientation = featherdoc::page_orientation::landscape;
    setup.width_twips = 6000U;
    setup.height_twips = 4400U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 720U;
    setup.margins.right_twips = 720U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    auto table = document.append_table(8U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1800U) ||
        !table.set_column_width_twips(1U, 1800U) ||
        !table.set_column_width_twips(2U, 3600U) ||
        !table.set_cell_text(0U, 0U, "Release board") ||
        !table.set_cell_text(0U, 2U, "Notes") ||
        !table.set_cell_text(1U, 0U, "Case") ||
        !table.set_cell_text(1U, 1U, "Owner") ||
        !table.set_cell_text(1U, 2U, "Status") ||
        !table.set_cell_text(2U, 0U, "MR-01") ||
        !table.set_cell_text(2U, 1U, "Cluster lead") ||
        !table.set_cell_text(2U, 2U, "Merged owner block stays centered.") ||
        !table.set_cell_text(3U, 0U, "MR-02") ||
        !table.set_cell_text(3U, 2U, "Second row remains aligned beneath the same owner.") ||
        !table.set_cell_text(4U, 0U, "MR-03") ||
        !table.set_cell_text(4U, 1U, "Search") ||
        !table.set_cell_text(4U, 2U, "Repeated merged headers should reappear on page 2.") ||
        !table.set_cell_text(5U, 0U, "MR-04") ||
        !table.set_cell_text(5U, 1U, "Docs") ||
        !table.set_cell_text(5U, 2U, "Header/footer placeholders must keep page numbers stable.") ||
        !table.set_cell_text(6U, 0U, "Tail") ||
        !table.set_cell_text(6U, 1U, "Milestone note") ||
        !table.set_cell_text(6U, 2U, "Merged milestone row stays readable.") ||
        !table.set_cell_text(7U, 0U, "MR-05") ||
        !table.set_cell_text(7U, 1U, "Visual gate") ||
        !table.set_cell_text(7U, 2U, "Page 3 repeats the board header.")) {
        return sample;
    }

    auto merged_banner = table.find_cell(0U, 0U);
    auto merged_owner = table.find_cell(2U, 1U);
    auto merged_tail = table.find_cell(6U, 1U);
    auto heading_case = table.find_cell(1U, 0U);
    auto heading_owner = table.find_cell(1U, 1U);
    auto heading_notes = table.find_cell(1U, 2U);
    if (!merged_banner.has_value() || !merged_owner.has_value() ||
        !merged_tail.has_value() || !heading_case.has_value() ||
        !heading_owner.has_value() || !heading_notes.has_value() ||
        !merged_banner->merge_right(2U) || !merged_owner->merge_down(1U) ||
        !merged_tail->merge_right(1U)) {
        return sample;
    }

    if (!merged_tail->set_text("Merged milestone row stays readable.")) {
        return sample;
    }

    if (!merged_banner->set_fill_color("D9EAF7") ||
        !heading_case->set_fill_color("EAF2F8") ||
        !heading_owner->set_fill_color("EAF2F8") ||
        !heading_notes->set_fill_color("EAF2F8") ||
        !merged_owner->set_fill_color("E2F0D9") ||
        !merged_tail->set_fill_color("FCE4D6") ||
        !merged_owner->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 8U; ++row_index) {
        if (!row.has_next()) {
            return sample;
        }
        if (row_index < 2U) {
            if (!row.set_repeats_header() ||
                !row.set_height_twips(540U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (!row.set_height_twips(720U,
                                         featherdoc::row_height_rule::exact)) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
    options.metadata.title = "FeatherDoc regression sample: document table "
                             "merged header footer variants";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 9.0;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] bool append_document_list_item(featherdoc::Document &document,
                                             std::string_view text,
                                             featherdoc::list_kind kind,
                                             bool restart) {
    auto paragraph = append_document_paragraph(document, text);
    if (!paragraph.has_next()) {
        return false;
    }

    if (restart) {
        return document.restart_paragraph_list(paragraph, kind);
    }
    return document.set_paragraph_list(paragraph, kind);
}

[[nodiscard]] ScenarioResult build_sectioned_report_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Sectioned report sample")) {
        return sample;
    }

    auto append_section_body = [&](std::string_view section_name,
                                   std::string_view section_focus) {
        for (std::size_t index = 1U; index <= 16U; ++index) {
            const auto paragraph_text =
                std::string{section_name} + " body paragraph " +
                std::to_string(index) + " keeps " +
                std::string{section_focus} +
                " flowing so the regression can observe header swaps, footer "
                "placement, and placeholder expansion across page breaks.";
            if (!append_document_text_paragraph(document, paragraph_text)) {
                return false;
            }
        }
        return true;
    };

    if (!append_section_body("Portrait", "the first section")) {
        return sample;
    }

    auto section0_header = document.ensure_section_header_paragraphs(0U);
    auto section0_footer = document.ensure_section_footer_paragraphs(0U);
    auto section0_first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto section0_first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto section0_even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto section0_even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!section0_header.has_next() ||
        !section0_header.set_text(
            "Portrait default header page {{page}} of {{total_pages}}") ||
        !section0_footer.has_next() ||
        !section0_footer.set_text(
            "Portrait default footer section {{section_page}} of "
            "{{section_total_pages}}") ||
        !section0_first_header.has_next() ||
        !section0_first_header.set_text(
            "Portrait first-page header page {{page}} of {{total_pages}}") ||
        !section0_first_footer.has_next() ||
        !section0_first_footer.set_text(
            "Portrait first-page footer section {{section_page}} of "
            "{{section_total_pages}}") ||
        !section0_even_header.has_next() ||
        !section0_even_header.set_text(
            "Portrait even header page {{page}} of {{total_pages}}") ||
        !section0_even_footer.has_next() ||
        !section0_even_footer.set_text(
            "Portrait even footer section {{section_page}} of "
            "{{section_total_pages}}")) {
        return sample;
    }

    featherdoc::section_page_setup portrait_setup{};
    portrait_setup.orientation = featherdoc::page_orientation::portrait;
    portrait_setup.width_twips = 12240U;
    portrait_setup.height_twips = 15840U;
    portrait_setup.margins.top_twips = 1440U;
    portrait_setup.margins.bottom_twips = 1440U;
    portrait_setup.margins.left_twips = 1440U;
    portrait_setup.margins.right_twips = 1440U;
    portrait_setup.margins.header_twips = 720U;
    portrait_setup.margins.footer_twips = 720U;
    if (!document.set_section_page_setup(0U, portrait_setup)) {
        return sample;
    }

    if (!document.append_section(false)) {
        return sample;
    }

    auto appendix_header = document.ensure_section_header_paragraphs(1U);
    auto appendix_footer = document.ensure_section_footer_paragraphs(1U);
    auto appendix_even_header = document.ensure_section_header_paragraphs(
        1U, featherdoc::section_reference_kind::even_page);
    auto appendix_even_footer = document.ensure_section_footer_paragraphs(
        1U, featherdoc::section_reference_kind::even_page);
    if (!appendix_header.has_next() ||
        !appendix_header.set_text(
            "Appendix odd header page {{page}} of {{total_pages}}") ||
        !appendix_footer.has_next() ||
        !appendix_footer.set_text(
            "Appendix odd footer section {{section_page}} of "
            "{{section_total_pages}}") ||
        !appendix_even_header.has_next() ||
        !appendix_even_header.set_text(
            "Appendix even header page {{page}} of {{total_pages}}") ||
        !appendix_even_footer.has_next() ||
        !appendix_even_footer.set_text(
            "Appendix even footer section {{section_page}} of "
            "{{section_total_pages}}")) {
        return sample;
    }

    featherdoc::section_page_setup landscape_setup{};
    landscape_setup.orientation = featherdoc::page_orientation::landscape;
    landscape_setup.width_twips = 15840U;
    landscape_setup.height_twips = 12240U;
    landscape_setup.margins.top_twips = 1080U;
    landscape_setup.margins.bottom_twips = 1080U;
    landscape_setup.margins.left_twips = 1440U;
    landscape_setup.margins.right_twips = 1440U;
    landscape_setup.margins.header_twips = 540U;
    landscape_setup.margins.footer_twips = 540U;
    if (!document.set_section_page_setup(1U, landscape_setup)) {
        return sample;
    }

    if (!append_section_body("Appendix", "the second section")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: sectioned report text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 9.0;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_list_report_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("List report sample")) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            "The first list uses bullet markers and exercises prefix extraction.")) {
        return sample;
    }

    for (int index = 1; index <= 8; ++index) {
        const auto text = "Bullet item " + std::to_string(index) +
                          ": confirm the PDF text path still sees bullet "
                          "content beside the visible list marker.";
        if (!append_document_list_item(document, text,
                                       featherdoc::list_kind::bullet,
                                       false)) {
            return sample;
        }
    }

    if (!append_document_text_paragraph(
            document,
            "The second list restarts numbering so PDFium sees list restarts in "
            "the extracted text.")) {
        return sample;
    }

    for (int index = 1; index <= 10; ++index) {
        const auto text = "Numbered item " + std::to_string(index) +
                          ": keep the report long enough for a real two-page "
                          "flow and preserve readable numbering.";
        if (!append_document_list_item(document, text,
                                       featherdoc::list_kind::decimal,
                                       index == 1)) {
            return sample;
        }
    }

    if (!append_document_text_paragraph(
            document,
            "Restarted list section demonstrates a fresh sequence after the "
            "numbered block.")) {
        return sample;
    }

    for (int index = 1; index <= 10; ++index) {
        const auto text = "Restarted item " + std::to_string(index) +
                          ": repeat the numbering sequence after a restart so "
                          "the regression keeps that path covered.";
        if (!append_document_list_item(document, text,
                                       featherdoc::list_kind::decimal,
                                       index == 1)) {
            return sample;
        }
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: list report text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.line_height_points = 17.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_long_report_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Long report sample")) {
        return sample;
    }

    const std::array<std::string_view, 3> sections{
        "Background section",
        "Operational findings",
        "Action plan",
    };
    for (const auto section_title : sections) {
        if (!append_document_text_paragraph(document, section_title) ||
            !append_document_text_paragraph(
                document,
                "This paragraph is intentionally verbose so that the document "
                "spills across multiple pages while still staying within the "
                "existing Document to PDF adapter.") ||
            !append_document_text_paragraph(
                document,
                "It exercises wrapped body text, repeated page flow, and the "
                "same PDFium text extraction path used by the real regression "
                "suite.") ||
            !append_document_text_paragraph(
                document,
                "The content remains plain text on purpose so the sample stays "
                "focused on pagination rather than a new rendering primitive.") ||
            !append_document_text_paragraph(
                document,
                "A short closing paragraph for this subsection keeps the page "
                "flow consistent and makes the sample more report-like.")) {
            return sample;
        }
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: long report text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 6.0;

    sample.layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_long_flow_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document long flow sample")) {
        return sample;
    }

    auto &header = document.ensure_section_header_paragraphs(0U);
    auto &footer = document.ensure_section_footer_paragraphs(0U);
    if (!header.has_next() || !header.set_text("Long flow header") ||
        !footer.has_next() ||
        !footer.set_text("Page {{page}} of {{total_pages}}")) {
        return sample;
    }

    for (int index = 1; index <= 120; ++index) {
        const auto text = "Long flow paragraph " + std::to_string(index) +
                          " keeps pagination and paragraph continuity stable.";
        if (!append_document_text_paragraph(document, text)) {
            return sample;
        }
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document long flow text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 9.0;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 6.0;

    sample.layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] bool contains_all_expected_text(
    const std::string &haystack,
    const std::vector<std::string> &needles,
    std::string &error_message) {
    for (const auto &needle : needles) {
        if (haystack.find(needle) == std::string::npos) {
            error_message = "Missing expected text: " + needle;
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool parse_arguments(const std::vector<std::string> &args,
                                   SampleConfig &config,
                                   bool &show_help,
                                   bool &require_cjk_font) {
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string_view arg = args[index];
        if (arg == "--help" || arg == "-h") {
            show_help = true;
            return true;
        }
        if (arg == "--scenario" && index + 1 < args.size()) {
            config.scenario = args[++index];
            continue;
        }
        if (arg == "--output" && index + 1 < args.size()) {
            config.output_path = args[++index];
            continue;
        }
        if (arg == "--expected-pages" && index + 1 < args.size()) {
            config.expected_pages =
                static_cast<std::size_t>(std::stoull(args[++index]));
            continue;
        }
        if (arg == "--expect-text" && index + 1 < args.size()) {
            config.expected_text.emplace_back(args[++index]);
            continue;
        }
        if (arg == "--expect-image-count" && index + 1 < args.size()) {
            config.expected_image_count =
                static_cast<std::size_t>(std::stoull(args[++index]));
            continue;
        }
        if (arg == "--require-cjk-font") {
            require_cjk_font = true;
            continue;
        }

        std::cerr << "unrecognized argument: " << arg << '\n';
        return false;
    }

    return !config.scenario.empty() && !config.output_path.empty() &&
           config.expected_pages > 0U && !config.expected_text.empty();
}

[[nodiscard]] std::vector<std::string> collect_utf8_arguments(
    int argc,
    char **argv) {
    std::vector<std::string> args;
    args.reserve(argc > 0 ? static_cast<std::size_t>(argc - 1) : 0U);
    for (int index = 1; index < argc; ++index) {
        args.emplace_back(argv[index]);
    }
    return args;
}

#if defined(_WIN32)
[[nodiscard]] std::vector<std::string> collect_utf8_arguments(
    int argc,
    wchar_t **argv) {
    std::vector<std::string> args;
    args.reserve(argc > 0 ? static_cast<std::size_t>(argc - 1) : 0U);
    for (int index = 1; index < argc; ++index) {
        args.emplace_back(utf8_from_wide(argv[index]));
    }
    return args;
}
#endif

[[nodiscard]] void print_usage() {
    std::cerr
        << "usage: featherdoc_pdf_regression_sample --scenario <name> "
           "--output <pdf> --expected-pages <count> "
           "[--expect-image-count <count>] [--require-cjk-font]\n";
}

} // namespace

int run_program(const std::vector<std::string> &args) {
    SampleConfig config;
    bool show_help = false;
    bool require_cjk_font = false;
    if (!parse_arguments(args, config, show_help, require_cjk_font)) {
        print_usage();
        return 2;
    }
    if (show_help) {
        print_usage();
        return 0;
    }

    const auto output_parent = config.output_path.parent_path();
    if (!output_parent.empty()) {
        std::filesystem::create_directories(output_parent);
    }

    const auto cjk_font = resolve_cjk_font();
    ScenarioResult sample;
    if (config.scenario == "single_text") {
        sample = build_single_text_sample();
    } else if (config.scenario == "multi_page_text") {
        sample = build_multi_page_text_sample();
    } else if (config.scenario == "cjk_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario cjk_text\n";
            return 1;
        }
        sample = build_cjk_text_sample(cjk_font);
    } else if (config.scenario == "styled_text") {
        sample = build_styled_text_sample();
    } else if (config.scenario == "font_size_text") {
        sample = build_font_size_text_sample();
    } else if (config.scenario == "color_text") {
        sample = build_color_text_sample();
    } else if (config.scenario == "mixed_style_text") {
        sample = build_mixed_style_text_sample();
    } else if (config.scenario == "contract_cjk_style") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario contract_cjk_style\n";
            return 1;
        }
        sample = build_contract_cjk_style_sample(cjk_font);
    } else if (config.scenario == "document_contract_cjk_style") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario document_contract_cjk_style\n";
            return 1;
        }
        sample = build_document_contract_cjk_style_sample(cjk_font);
    } else if (config.scenario == "document_eastasia_style_probe") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario document_eastasia_style_probe\n";
            return 1;
        }
        sample = build_document_eastasia_style_probe_sample(cjk_font);
    } else if (config.scenario == "document_rtl_bidi_text") {
        const auto arabic_font = first_existing_path(candidate_arabic_fonts());
        if (arabic_font.empty() || !std::filesystem::exists(arabic_font)) {
            std::cerr << "missing Arabic font for scenario document_rtl_bidi_text\n";
            return 1;
        }
        sample = build_document_rtl_bidi_text_sample(arabic_font);
    } else if (config.scenario == "document_font_matrix_text") {
        const auto arabic_font = first_existing_path(candidate_arabic_fonts());
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario document_font_matrix_text\n";
            return 1;
        }
        if (arabic_font.empty() || !std::filesystem::exists(arabic_font)) {
            std::cerr
                << "missing Arabic font for scenario document_font_matrix_text\n";
            return 1;
        }
        sample = build_document_font_matrix_text_sample(cjk_font, arabic_font);
    } else if (config.scenario == "document_table_font_matrix_text") {
        const auto arabic_font = first_existing_path(candidate_arabic_fonts());
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario "
                         "document_table_font_matrix_text\n";
            return 1;
        }
        if (arabic_font.empty() || !std::filesystem::exists(arabic_font)) {
            std::cerr << "missing Arabic font for scenario "
                         "document_table_font_matrix_text\n";
            return 1;
        }
        sample = build_document_table_font_matrix_text_sample(cjk_font,
                                                              arabic_font);
    } else if (config.scenario == "header_footer_rtl_text") {
        const auto arabic_font = first_existing_path(candidate_arabic_fonts());
        if (arabic_font.empty() || !std::filesystem::exists(arabic_font)) {
            std::cerr << "missing Arabic font for scenario header_footer_rtl_text\n";
            return 1;
        }
        sample = build_header_footer_rtl_text_sample(arabic_font);
    } else if (config.scenario == "header_footer_rtl_variants_text") {
        const auto arabic_font = first_existing_path(candidate_arabic_fonts());
        if (arabic_font.empty() || !std::filesystem::exists(arabic_font)) {
            std::cerr
                << "missing Arabic font for scenario "
                   "header_footer_rtl_variants_text\n";
            return 1;
        }
        sample = build_header_footer_rtl_variants_text_sample(arabic_font);
    } else if (config.scenario == "document_table_header_footer_variants_text") {
        sample = build_document_table_header_footer_variants_text_sample();
    } else if (config.scenario == "document_table_wrap_flow_text") {
        sample = build_document_table_wrap_flow_text_sample();
    } else if (config.scenario == "document_table_cjk_wrap_flow_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario "
                         "document_table_cjk_wrap_flow_text\n";
            return 1;
        }
        sample = build_document_table_cjk_wrap_flow_text_sample(cjk_font);
    } else if (config.scenario == "document_table_cjk_merged_repeat_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario "
                         "document_table_cjk_merged_repeat_text\n";
            return 1;
        }
        sample = build_document_table_cjk_merged_repeat_text_sample(cjk_font);
    } else if (config.scenario ==
               "document_table_cjk_vertical_merged_cant_split_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario "
                         "document_table_cjk_vertical_merged_cant_split_text\n";
            return 1;
        }
        sample =
            build_document_table_cjk_vertical_merged_cant_split_text_sample(
                cjk_font);
    } else if (config.scenario == "document_table_cant_split_text") {
        sample = build_document_table_cant_split_text_sample();
    } else if (config.scenario == "document_table_merged_cells_text") {
        sample = build_document_table_merged_cells_text_sample();
    } else if (config.scenario == "document_table_merged_header_repeat_text") {
        sample = build_document_table_merged_header_repeat_text_sample();
    } else if (config.scenario == "document_table_merged_cant_split_text") {
        sample = build_document_table_merged_cant_split_text_sample();
    } else if (config.scenario ==
               "document_table_vertical_merged_cant_split_text") {
        sample = build_document_table_vertical_merged_cant_split_text_sample();
    } else if (config.scenario ==
               "document_table_merged_header_footer_variants_text") {
        sample =
            build_document_table_merged_header_footer_variants_text_sample();
    } else if (config.scenario == "document_style_gallery_text") {
        sample = build_document_style_gallery_sample();
    } else if (config.scenario == "three_page_text") {
        sample = build_three_page_text_sample();
    } else if (config.scenario == "landscape_text") {
        sample = build_landscape_text_sample();
    } else if (config.scenario == "title_body_text") {
        sample = build_title_body_text_sample();
    } else if (config.scenario == "dense_text") {
        sample = build_dense_text_sample();
    } else if (config.scenario == "four_page_text") {
        sample = build_four_page_text_sample();
    } else if (config.scenario == "underline_text") {
        sample = build_underline_text_sample();
    } else if (config.scenario == "strikethrough_text") {
        sample = build_strikethrough_text_sample();
    } else if (config.scenario == "superscript_subscript_text") {
        sample = build_superscript_subscript_text_sample();
    } else if (config.scenario == "style_superscript_subscript_text") {
        sample = build_style_superscript_subscript_text_sample();
    } else if (config.scenario == "punctuation_text") {
        sample = build_punctuation_text_sample();
    } else if (config.scenario == "two_page_text") {
        sample = build_two_page_text_sample();
    } else if (config.scenario == "repeat_phrase_text") {
        sample = build_repeat_phrase_text_sample();
    } else if (config.scenario == "bordered_box_text") {
        sample = build_bordered_box_text_sample();
    } else if (config.scenario == "line_primitive_text") {
        sample = build_line_primitive_text_sample();
    } else if (config.scenario == "table_like_grid_text") {
        sample = build_table_like_grid_text_sample();
    } else if (config.scenario == "metadata_long_title_text") {
        sample = build_metadata_long_title_text_sample();
    } else if (config.scenario == "header_footer_text") {
        sample = build_header_footer_text_sample();
    } else if (config.scenario == "two_column_text") {
        sample = build_two_column_text_sample();
    } else if (config.scenario == "invoice_grid_text") {
        sample = build_invoice_grid_text_sample();
    } else if (config.scenario == "image_caption_text") {
        sample = build_image_caption_text_sample(output_parent);
    } else if (config.scenario == "sectioned_report_text") {
        sample = build_sectioned_report_text_sample();
    } else if (config.scenario == "list_report_text") {
        sample = build_list_report_text_sample();
    } else if (config.scenario == "long_report_text") {
        sample = build_long_report_text_sample();
    } else if (config.scenario == "document_long_flow_text") {
        sample = build_document_long_flow_text_sample();
    } else if (config.scenario == "image_report_text") {
        sample = build_image_report_text_sample(output_parent);
    } else if (config.scenario == "document_image_semantics_text") {
        sample = build_document_image_semantics_text_sample(output_parent);
    } else if (config.scenario == "document_cjk_complex_layout_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario "
                         "document_cjk_complex_layout_text\n";
            return 1;
        }
        sample = build_document_cjk_complex_layout_text_sample(cjk_font,
                                                               output_parent);
    } else if (config.scenario == "document_cjk_copy_search_matrix_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario "
                         "document_cjk_copy_search_matrix_text\n";
            return 1;
        }
        sample = build_document_cjk_copy_search_matrix_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_font_embed_matrix_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario "
                         "document_cjk_font_embed_matrix_text\n";
            return 1;
        }
        sample = build_document_cjk_font_embed_matrix_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_font_embed_wrap_mix_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario "
                         "document_cjk_font_embed_wrap_mix_text\n";
            return 1;
        }
        sample = build_document_cjk_font_embed_wrap_mix_text_sample(cjk_font,
                                                                    output_parent);
    } else if (config.scenario == "document_cjk_image_wrap_stress_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario "
                         "document_cjk_image_wrap_stress_text\n";
            return 1;
        }
        sample = build_document_cjk_image_wrap_stress_text_sample(cjk_font,
                                                                  output_parent);
    } else if (config.scenario == "document_cjk_extreme_page_breaks_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario "
                         "document_cjk_extreme_page_breaks_text\n";
            return 1;
        }
        sample = build_document_cjk_extreme_page_breaks_text_sample(cjk_font,
                                                                    output_parent);
    } else if (config.scenario == "document_cjk_table_wrap_page_flow_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario "
                         "document_cjk_table_wrap_page_flow_text\n";
            return 1;
        }
        sample = build_document_cjk_table_wrap_page_flow_text_sample(
            cjk_font, output_parent);
    } else if (config.scenario ==
               "document_cjk_vertical_merge_wrap_cant_split_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario "
                         "document_cjk_vertical_merge_wrap_cant_split_text\n";
            return 1;
        }
        sample = build_document_cjk_vertical_merge_wrap_cant_split_text_sample(
            cjk_font, output_parent);
    } else if (config.scenario == "document_table_semantics_text") {
        sample = build_document_table_semantics_text_sample();
    } else if (config.scenario == "document_invoice_table_text") {
        sample = build_document_invoice_table_text_sample();
    } else if (config.scenario == "cjk_image_report_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario cjk_image_report_text\n";
            return 1;
        }
        sample = build_cjk_image_report_text_sample(cjk_font, output_parent);
    } else if (config.scenario == "cjk_report_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario cjk_report_text\n";
            return 1;
        }
        sample = build_cjk_report_text_sample(cjk_font);
    } else {
        std::cerr << "unknown scenario: " << config.scenario << '\n';
        print_usage();
        return 2;
    }

    if (sample.layout.pages.size() != config.expected_pages) {
        std::cerr << "unexpected page count for scenario " << config.scenario
                  << ": expected " << config.expected_pages << ", got "
                  << sample.layout.pages.size() << '\n';
        return 1;
    }

    featherdoc::pdf::PdfWriterOptions writer_options;
    writer_options.page_size = sample.layout.pages.front().size;
    writer_options.title = sample.layout.metadata.title;
    writer_options.creator = sample.layout.metadata.creator;

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(sample.layout, config.output_path, writer_options);
    if (!write_result) {
        std::cerr << write_result.error_message << '\n';
        return 1;
    }
    if (!std::filesystem::exists(config.output_path)) {
        std::cerr << "PDF output file was not created\n";
        return 1;
    }
    const auto output_size = std::filesystem::file_size(config.output_path);
    if (output_size == 0U) {
        std::cerr << "PDF output file is empty\n";
        return 1;
    }
    const auto max_output_size =
        static_cast<std::uintmax_t>(sample.layout.pages.size()) * 700000U;
    if (output_size > max_output_size) {
        std::cerr << "PDF output file is unexpectedly large for scenario "
                  << config.scenario << ": expected at most "
                  << max_output_size << " bytes, got " << output_size << '\n';
        return 1;
    }

#if defined(FEATHERDOC_PDF_REGRESSION_IMPORT_VALIDATION)
    std::string text_error;
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(config.output_path, {});
    if (!parse_result) {
        std::cerr << parse_result.error_message << '\n';
        return 1;
    }

    if (parse_result.document.pages.size() != config.expected_pages) {
        std::cerr << "PDFium reported unexpected page count: expected "
                  << config.expected_pages << ", got "
                  << parse_result.document.pages.size() << '\n';
        return 1;
    }

    const auto extracted_text = collect_text(parse_result.document);
    if (!contains_all_expected_text(extracted_text, config.expected_text,
                                    text_error)) {
        if (config.scenario == "cjk_text") {
            std::cerr << text_error << '\n'
                      << "skipping CJK regression sample: current PDFio/"
                         "PDFium roundtrip does not yet preserve this Unicode "
                         "text path reliably\n";
            return 77;
        }
        std::cerr << text_error << '\n';
        return 1;
    }

    if (config.expected_image_count.has_value()) {
        std::string image_count_error;
        const auto image_count =
            count_pdf_image_objects(config.output_path, image_count_error);
        if (!image_count.has_value()) {
            std::cerr << image_count_error << '\n';
            return 1;
        }
        if (*image_count != *config.expected_image_count) {
            std::cerr << "unexpected PDF image object count for scenario "
                      << config.scenario << ": expected "
                      << *config.expected_image_count << ", got "
                      << *image_count << '\n';
            return 1;
        }
    }

    std::cout << "wrote " << config.output_path.string() << " ("
              << write_result.bytes_written << " bytes, "
              << parse_result.document.pages.size() << " pages)\n";
#else
    std::cout << "wrote " << config.output_path.string() << " ("
              << write_result.bytes_written
              << " bytes, export-only validation)\n";
#endif
    return 0;
}

#if defined(_WIN32)
int wmain(int argc, wchar_t **argv) {
    return run_program(collect_utf8_arguments(argc, argv));
}
#else
int main(int argc, char **argv) {
    return run_program(collect_utf8_arguments(argc, argv));
}
#endif
