#pragma once

#include "doctest.h"

#include <featherdoc/pdf/pdf_document_adapter.hpp>
#include <featherdoc/pdf/pdf_text_metrics.hpp>
#include <featherdoc/pdf/pdf_text_shaper.hpp>
#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
#include <featherdoc/pdf/pdf_parser.hpp>
#endif
#include <featherdoc/pdf/pdf_writer.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

auto utf8_from_u8(std::u8string_view text) -> std::string {
    return {reinterpret_cast<const char *>(text.data()), text.size()};
}

[[nodiscard]] std::string tiny_png_data() {
    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U,
        0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x00U, 0x00U, 0x01U, 0x08U, 0x02U, 0x00U, 0x00U, 0x00U, 0x90U,
        0x77U, 0x53U, 0xDEU, 0x00U, 0x00U, 0x00U, 0x0CU, 0x49U, 0x44U, 0x41U,
        0x54U, 0x08U, 0xD7U, 0x63U, 0xF8U, 0xCFU, 0xC0U, 0x00U, 0x00U, 0x03U,
        0x01U, 0x01U, 0x00U, 0x18U, 0xDDU, 0x8DU, 0xB0U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U, 0x82U,
    };

    return {reinterpret_cast<const char *>(tiny_png_bytes),
            sizeof(tiny_png_bytes)};
}

[[nodiscard]] std::string tiny_rgba_png_data() {
    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U,
        0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x02U,
        0x00U, 0x00U, 0x00U, 0x02U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x72U,
        0xB6U, 0x0DU, 0x24U, 0x00U, 0x00U, 0x00U, 0x16U, 0x49U, 0x44U, 0x41U,
        0x54U, 0x78U, 0xDAU, 0x63U, 0xF8U, 0xCFU, 0xC0U, 0xF0U, 0x1FU, 0x08U,
        0x1BU, 0x18U, 0x80U, 0xB4U, 0xC3U, 0x7FU, 0x20U, 0x0FU, 0x00U, 0x3EU,
        0x1AU, 0x06U, 0xBBU, 0x82U, 0x99U, 0xA3U, 0xF4U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U, 0x82U,
    };

    return {reinterpret_cast<const char *>(tiny_png_bytes),
            sizeof(tiny_png_bytes)};
}

[[nodiscard]] std::string quadrant_png_data() {
    constexpr unsigned char quadrant_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U,
        0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x08U,
        0x00U, 0x00U, 0x00U, 0x04U, 0x08U, 0x02U, 0x00U, 0x00U, 0x00U, 0x3CU,
        0xAFU, 0xE9U, 0xA7U, 0x00U, 0x00U, 0x00U, 0x1DU, 0x49U, 0x44U, 0x41U,
        0x54U, 0x78U, 0xDAU, 0x63U, 0xF8U, 0xCFU, 0xC0U, 0x00U, 0x47U, 0x48U,
        0xCCU, 0xFFU, 0x0CU, 0x38U, 0x25U, 0x18U, 0x4EU, 0x20U, 0xD0U, 0xFFU,
        0x3BU, 0x08U, 0x84U, 0x53U, 0x02U, 0x00U, 0x8FU, 0xCEU, 0x25U, 0x09U,
        0x87U, 0xCFU, 0x36U, 0xFCU, 0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U,
        0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U, 0x82U,
    };

    return {reinterpret_cast<const char *>(quadrant_png_bytes),
            sizeof(quadrant_png_bytes)};
}

void write_binary_file(const std::filesystem::path &path,
                       std::string_view data) {
    std::ofstream stream(path, std::ios::binary);
    REQUIRE(stream.good());
    stream.write(data.data(), static_cast<std::streamsize>(data.size()));
    REQUIRE(stream.good());
}

[[nodiscard]] std::string read_binary_file(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    REQUIRE(stream.good());
    return {std::istreambuf_iterator<char>{stream},
            std::istreambuf_iterator<char>{}};
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

[[nodiscard]] std::vector<std::filesystem::path> candidate_cjk_fonts() {
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
#endif

    return candidates;
}

[[nodiscard]] std::vector<std::filesystem::path> candidate_latin_fonts() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured =
            environment_value("FEATHERDOC_TEST_PROPORTIONAL_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/arial.ttf");
    candidates.emplace_back("C:/Windows/Fonts/segoeui.ttf");
    candidates.emplace_back("C:/Windows/Fonts/calibri.ttf");
#endif

    return candidates;
}

[[nodiscard]] std::vector<std::filesystem::path> candidate_rtl_fonts() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured = environment_value("FEATHERDOC_TEST_RTL_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/arial.ttf");
    candidates.emplace_back("C:/Windows/Fonts/segoeui.ttf");
    candidates.emplace_back("C:/Windows/Fonts/times.ttf");
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

[[nodiscard]] std::filesystem::path make_temp_font_file(std::string_view name) {
    const auto path = std::filesystem::current_path() / name;
    std::ofstream stream(path, std::ios::binary);
    stream << "featherdoc adapter font mapping fixture";
    return path;
}

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
[[nodiscard]] std::string
collect_text(const featherdoc::pdf::PdfParsedDocument &document) {
    std::string text;
    for (const auto &page : document.pages) {
        for (const auto &span : page.text_spans) {
            text += span.text;
        }
    }
    return text;
}

[[nodiscard]] std::string
collect_text(const featherdoc::pdf::PdfParsedPage &page) {
    std::string text;
    for (const auto &span : page.text_spans) {
        text += span.text;
    }
    return text;
}
#endif

[[nodiscard]] std::string
collect_layout_text(const featherdoc::pdf::PdfDocumentLayout &layout) {
    std::string text;
    for (const auto &page : layout.pages) {
        for (const auto &text_run : page.text_runs) {
            text += text_run.text;
        }
    }
    return text;
}

featherdoc::Paragraph append_body_paragraph(featherdoc::Document &document,
                                            std::string_view text) {
    auto paragraph = document.paragraphs();
    while (paragraph.has_next()) {
        paragraph.next();
    }

    return paragraph.insert_paragraph_after(std::string{text});
}

} // namespace
