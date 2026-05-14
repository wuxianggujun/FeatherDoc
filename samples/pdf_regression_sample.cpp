#include <featherdoc/pdf/pdf_document_adapter.hpp>
#include <featherdoc/pdf/pdf_parser.hpp>
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

[[nodiscard]] ScenarioResult build_mixed_cjk_punctuation_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: mixed CJK punctuation text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, utf8_from_u8(u8"中英混排标点样本"), 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        true, "CJK Regression Font", font_path));
    page.text_runs.push_back(make_text_run(
        72.0, 692.0, utf8_from_u8(u8"FeatherDoc，PDF；office 123。"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));
    page.text_runs.push_back(make_text_run(
        72.0, 666.0, utf8_from_u8(u8"括号（中文）与引号“mixed”。"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.18, 0.12, 0.08}, false, false, false,
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

[[nodiscard]] ScenarioResult build_latin_ligature_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: Latin ligature text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "office affinity flow", 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 692.0, "file fixture flings", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 666.0, "efficient workflow", 12.0,
        featherdoc::pdf::PdfRgbColor{0.18, 0.12, 0.08}, false, false, false,
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

    if (!append_document_text_paragraph(
            document,
            "Executive summary paragraph one keeps the opening section readable "
            "while PDFium verifies a normal body flow.") ||
        !append_document_text_paragraph(
            document,
            "Executive summary paragraph two adds a second line of realistic "
            "report content and keeps the section header visible.") ||
        !append_document_text_paragraph(
            document,
            "Executive summary paragraph three closes the portrait section with "
            "a stable footer marker.")) {
        return sample;
    }

    auto section0_header = document.ensure_section_header_paragraphs(0U);
    auto section0_footer = document.ensure_section_footer_paragraphs(0U);
    if (!section0_header.has_next() ||
        !section0_header.set_text("Portrait section header") ||
        !section0_footer.has_next() ||
        !section0_footer.set_text("Portrait section footer")) {
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
    if (!appendix_header.has_next() ||
        !appendix_header.set_text("Landscape appendix header") ||
        !appendix_footer.has_next() ||
        !appendix_footer.set_text("Landscape appendix footer")) {
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

    if (!append_document_text_paragraph(
            document,
            "Appendix paragraph one keeps the second section on a wider page.") ||
        !append_document_text_paragraph(
            document,
            "Appendix paragraph two verifies that the layout still emits the "
            "correct text after the section break.") ||
        !append_document_text_paragraph(
            document,
            "Appendix paragraph three closes the wider section and provides a "
            "stable footer anchor.")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: sectioned report text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
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
    } else if (config.scenario == "mixed_cjk_punctuation_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario mixed_cjk_punctuation_text\n";
            return 1;
        }
        sample = build_mixed_cjk_punctuation_text_sample(cjk_font);
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
    } else if (config.scenario == "punctuation_text") {
        sample = build_punctuation_text_sample();
    } else if (config.scenario == "latin_ligature_text") {
        sample = build_latin_ligature_text_sample();
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
    std::string text_error;
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
