#include <featherdoc/pdf/pdf_parser.hpp>

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

extern "C" {
#include <fpdf_edit.h>
#include <fpdf_text.h>
#include <fpdfview.h>
}

#ifndef MINIZ_HEADER_FILE_ONLY
#define MINIZ_HEADER_FILE_ONLY 1
#endif
#include <miniz.h>

namespace featherdoc::pdf {
namespace {

#include "pdfium_parser_text_objects.inc"
#include "pdfium_parser_text_structure.inc"
#include "pdfium_parser_table_candidates.inc"
#include "pdfium_parser_page_structure.inc"

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
    const auto actual_text_pages =
        options.extract_text ? extract_page_actual_text_spans(input_path)
                             : std::vector<std::vector<std::string>>{};

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
            if (parsed_page.text_spans.empty() && char_count > 0) {
                std::vector<unsigned short> page_text(
                    static_cast<std::size_t>(char_count) + 1U, 0U);
                const int copied = FPDFText_GetText(
                    text_page.get(), 0, char_count, page_text.data());
                if (copied > 1) {
                    PdfParsedTextSpan span;
                    for (int text_index = 0; text_index < copied - 1;
                         ++text_index) {
                        append_utf8(
                            span.text,
                            static_cast<std::uint32_t>(page_text[
                                static_cast<std::size_t>(text_index)]));
                    }
                    if (!span.text.empty()) {
                        parsed_page.text_spans.push_back(std::move(span));
                    }
                }
            }
            PdfParsedPage object_text_page;
            const auto *actual_text_spans =
                static_cast<std::size_t>(page_index) < actual_text_pages.size()
                    ? &actual_text_pages[static_cast<std::size_t>(page_index)]
                    : nullptr;
            append_page_object_text_spans(
                page.get(), text_page.get(), object_text_page, options,
                actual_text_spans == nullptr || actual_text_spans->empty());
            if (actual_text_spans != nullptr &&
                should_prefer_actual_text_spans(object_text_page.text_spans,
                                                *actual_text_spans)) {
                object_text_page.text_spans = build_actual_text_spans(
                    object_text_page.text_spans, *actual_text_spans);
            }
            bool use_object_spans = should_prefer_text_object_spans(
                parsed_page.text_spans, object_text_page.text_spans);
            if (!use_object_spans && actual_text_spans != nullptr &&
                missing_actual_text_span(parsed_page.text_spans,
                                         *actual_text_spans) &&
                !missing_actual_text_span(object_text_page.text_spans,
                                          *actual_text_spans)) {
                use_object_spans = true;
            }
            if (use_object_spans) {
                parsed_page.text_spans =
                    std::move(object_text_page.text_spans);
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
