#include <featherdoc/pdf/pdf_parser.hpp>

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

extern "C" {
#include <fpdf_text.h>
#include <fpdfview.h>
}

namespace featherdoc::pdf {
namespace {

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

struct PdfiumTextPageCloser {
    void operator()(FPDF_TEXTPAGE text_page) const {
        if (text_page != nullptr) {
            FPDFText_ClosePage(text_page);
        }
    }
};

using PdfiumDocumentPtr =
    std::unique_ptr<std::remove_pointer_t<FPDF_DOCUMENT>, PdfiumDocumentCloser>;
using PdfiumPagePtr =
    std::unique_ptr<std::remove_pointer_t<FPDF_PAGE>, PdfiumPageCloser>;
using PdfiumTextPagePtr =
    std::unique_ptr<std::remove_pointer_t<FPDF_TEXTPAGE>, PdfiumTextPageCloser>;

struct PdfiumLibrary {
    PdfiumLibrary() {
        FPDF_InitLibrary();
    }

    ~PdfiumLibrary() {
        FPDF_DestroyLibrary();
    }

    PdfiumLibrary(const PdfiumLibrary &) = delete;
    PdfiumLibrary &operator=(const PdfiumLibrary &) = delete;
};

void ensure_pdfium_initialized() {
    static PdfiumLibrary library;
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

void append_utf8(std::string &output, std::uint32_t codepoint) {
    if (codepoint <= 0x7F) {
        output.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        output.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        output.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
        output.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
}

[[nodiscard]] PdfParsedTextSpan parse_text_span(FPDF_TEXTPAGE text_page,
                                                int char_index,
                                                const PdfParseOptions &options) {
    PdfParsedTextSpan span;
    append_utf8(span.text,
                static_cast<std::uint32_t>(
                    FPDFText_GetUnicode(text_page, char_index)));
    span.font_size_points = FPDFText_GetFontSize(text_page, char_index);

    if (options.extract_geometry) {
        double left = 0.0;
        double right = 0.0;
        double bottom = 0.0;
        double top = 0.0;
        if (FPDFText_GetCharBox(text_page, char_index, &left, &right, &bottom,
                                &top)) {
            span.bounds = PdfRect{left, bottom, right - left, top - bottom};
        }
    }

    return span;
}

[[nodiscard]] double rect_bottom(const PdfRect &rect) noexcept {
    return rect.y_points;
}

[[nodiscard]] double rect_top(const PdfRect &rect) noexcept {
    return rect.y_points + rect.height_points;
}

[[nodiscard]] double rect_right(const PdfRect &rect) noexcept {
    return rect.x_points + rect.width_points;
}

void expand_rect(PdfRect &target, const PdfRect &candidate) {
    if (target.width_points <= 0.0 && target.height_points <= 0.0) {
        target = candidate;
        return;
    }

    const double left = (std::min)(target.x_points, candidate.x_points);
    const double bottom = (std::min)(rect_bottom(target), rect_bottom(candidate));
    const double right = (std::max)(rect_right(target), rect_right(candidate));
    const double top = (std::max)(rect_top(target), rect_top(candidate));
    target = PdfRect{left, bottom, right - left, top - bottom};
}

void append_span_to_line(PdfParsedTextLine &line, const PdfParsedTextSpan &span) {
    line.text += span.text;
    expand_rect(line.bounds, span.bounds);
    line.text_spans.push_back(span);
}

[[nodiscard]] bool should_start_new_line(const PdfParsedTextLine &line,
                                         const PdfParsedTextSpan &span) {
    if (line.text_spans.empty()) {
        return false;
    }

    const double line_height = (std::max)(line.bounds.height_points, 1.0);
    const double span_center_y = span.bounds.y_points + span.bounds.height_points / 2.0;
    const double line_center_y = line.bounds.y_points + line.bounds.height_points / 2.0;
    return std::abs(span_center_y - line_center_y) > line_height * 0.65;
}

[[nodiscard]] std::vector<PdfParsedTextLine>
group_text_spans_into_lines(const std::vector<PdfParsedTextSpan> &spans) {
    std::vector<PdfParsedTextLine> lines;

    for (const auto &span : spans) {
        if (span.text == "\r" || span.text == "\n") {
            if (!lines.empty() && !lines.back().text.empty()) {
                lines.emplace_back();
            }
            continue;
        }
        if (span.text.empty()) {
            continue;
        }

        if (lines.empty() || should_start_new_line(lines.back(), span)) {
            lines.emplace_back();
        }
        append_span_to_line(lines.back(), span);
    }

    lines.erase(std::remove_if(lines.begin(), lines.end(),
                               [](const PdfParsedTextLine &line) {
                                   return line.text.empty();
                               }),
                lines.end());
    return lines;
}

[[nodiscard]] bool should_start_new_paragraph(const PdfParsedTextLine &previous,
                                              const PdfParsedTextLine &current) {
    const double previous_height = (std::max)(previous.bounds.height_points, 1.0);
    const double vertical_gap = previous.bounds.y_points - rect_top(current.bounds);
    if (vertical_gap > previous_height * 0.85) {
        return true;
    }

    const double indent_delta = current.bounds.x_points - previous.bounds.x_points;
    return indent_delta > previous_height * 1.5;
}

void append_line_to_paragraph(PdfParsedParagraph &paragraph,
                              const PdfParsedTextLine &line) {
    if (!paragraph.text.empty()) {
        paragraph.text.push_back('\n');
    }
    paragraph.text += line.text;
    expand_rect(paragraph.bounds, line.bounds);
    paragraph.lines.push_back(line);
}

[[nodiscard]] std::vector<PdfParsedParagraph>
group_lines_into_paragraphs(const std::vector<PdfParsedTextLine> &lines) {
    std::vector<PdfParsedParagraph> paragraphs;
    for (const auto &line : lines) {
        if (paragraphs.empty() ||
            should_start_new_paragraph(paragraphs.back().lines.back(), line)) {
            paragraphs.emplace_back();
        }
        append_line_to_paragraph(paragraphs.back(), line);
    }
    return paragraphs;
}

void enrich_text_structure(PdfParsedPage &page) {
    page.text_lines = group_text_spans_into_lines(page.text_spans);
    page.paragraphs = group_lines_into_paragraphs(page.text_lines);
}

}  // namespace

PdfParseResult PdfiumParser::parse(const std::filesystem::path &input_path,
                                   const PdfParseOptions &options) {
    PdfParseResult result;

    if (!std::filesystem::exists(input_path)) {
        result.error_message = "PDF input does not exist: " + input_path.string();
        return result;
    }

    ensure_pdfium_initialized();

    const auto input_string = input_path.string();
    PdfiumDocumentPtr document(FPDF_LoadDocument(input_string.c_str(), nullptr));
    if (!document) {
        result.error_message =
            "Unable to load PDF: " + pdfium_error_to_string(FPDF_GetLastError());
        return result;
    }

    const int page_count = FPDF_GetPageCount(document.get());
    result.document.pages.reserve(static_cast<std::size_t>(page_count));

    for (int page_index = 0; page_index < page_count; ++page_index) {
        PdfiumPagePtr page(FPDF_LoadPage(document.get(), page_index));
        if (!page) {
            result.error_message =
                "Unable to load PDF page " + std::to_string(page_index);
            return result;
        }

        PdfParsedPage parsed_page;
        parsed_page.size =
            PdfPageSize{FPDF_GetPageWidth(page.get()), FPDF_GetPageHeight(page.get())};

        if (options.extract_text) {
            PdfiumTextPagePtr text_page(FPDFText_LoadPage(page.get()));
            if (!text_page) {
                result.error_message =
                    "Unable to load PDF text page " + std::to_string(page_index);
                return result;
            }

            const int char_count = FPDFText_CountChars(text_page.get());
            parsed_page.text_spans.reserve(static_cast<std::size_t>(char_count));
            for (int char_index = 0; char_index < char_count; ++char_index) {
                if (FPDFText_GetUnicode(text_page.get(), char_index) == 0) {
                    continue;
                }
                parsed_page.text_spans.push_back(
                    parse_text_span(text_page.get(), char_index, options));
            }
            if (options.extract_geometry) {
                enrich_text_structure(parsed_page);
            }
        }

        result.document.pages.push_back(std::move(parsed_page));
    }

    result.success = true;
    return result;
}

}  // namespace featherdoc::pdf
