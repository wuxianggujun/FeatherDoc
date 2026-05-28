#include <featherdoc/pdf/pdf_parser.hpp>

#include <fpdf_text.h>
#include <fpdfview.h>

#include <iomanip>
#include <filesystem>
#include <iostream>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "usage: featherdoc_pdfium_probe <input.pdf>\n";
        return 2;
    }

    const std::filesystem::path input = argv[1];
    const bool print_details = argc > 2 && std::string_view(argv[2]) == "--details";
    featherdoc::pdf::PdfiumParser parser;
    featherdoc::pdf::IPdfParser &pdf_parser = parser;

    const auto result = pdf_parser.parse(input, {});
    if (!result) {
        std::cerr << result.error_message << '\n';
        return 1;
    }

    std::size_t text_spans = 0;
    for (const auto &page : result.document.pages) {
        text_spans += page.text_spans.size();
    }

    std::cout << "parsed " << input.string() << " ("
              << result.document.pages.size() << " pages, " << text_spans
              << " text spans)\n";

    if (print_details) {
        std::cout << std::fixed << std::setprecision(1);
        const auto utf8_input = input.string();
        FPDF_DOCUMENT raw_document = FPDF_LoadDocument(utf8_input.c_str(), nullptr);
        for (std::size_t page_index = 0; page_index < result.document.pages.size();
             ++page_index) {
            const auto &page = result.document.pages[page_index];
            std::cout << "page " << page_index << " spans="
                      << page.text_spans.size() << " lines="
                      << page.text_lines.size() << " tables="
                      << page.table_candidates.size() << '\n';
            if (raw_document != nullptr) {
                FPDF_PAGE raw_page =
                    FPDF_LoadPage(raw_document, static_cast<int>(page_index));
                if (raw_page != nullptr) {
                    FPDF_TEXTPAGE raw_text_page = FPDFText_LoadPage(raw_page);
                    if (raw_text_page != nullptr) {
                        const int raw_char_count =
                            FPDFText_CountChars(raw_text_page);
                        std::cout << "  raw_char_count=" << raw_char_count
                                  << " raw_unicode_codes=";
                        for (int char_index = 0;
                             char_index < raw_char_count && char_index < 24;
                             ++char_index) {
                            const auto code = FPDFText_GetUnicode(
                                raw_text_page, char_index);
                            std::cout << ' '
                                      << static_cast<unsigned long>(code);
                        }
                        std::cout << '\n';
                        FPDFText_ClosePage(raw_text_page);
                    } else {
                        std::cout << "  raw_text_page=unavailable\n";
                    }
                    FPDF_ClosePage(raw_page);
                } else {
                    std::cout << "  raw_page=unavailable\n";
                }
            } else {
                std::cout << "  raw_document=unavailable\n";
            }
            for (std::size_t span_index = 0; span_index < page.text_spans.size();
                 ++span_index) {
                const auto &span = page.text_spans[span_index];
                std::cout << "  span " << span_index << " text=\"" << span.text
                          << "\" bounds=(" << span.bounds.x_points << ", "
                          << span.bounds.y_points << ", "
                          << span.bounds.width_points << ", "
                          << span.bounds.height_points << ")\n";
            }
            for (std::size_t line_index = 0; line_index < page.text_lines.size();
                 ++line_index) {
                const auto &line = page.text_lines[line_index];
                std::cout << "  line " << line_index << " text=\"" << line.text
                          << "\" spans=" << line.text_spans.size()
                          << " bounds=(" << line.bounds.x_points << ", "
                          << line.bounds.y_points << ", "
                          << line.bounds.width_points << ", "
                          << line.bounds.height_points << ")\n";
            }
            for (std::size_t table_index = 0;
                 table_index < page.table_candidates.size(); ++table_index) {
                const auto &table = page.table_candidates[table_index];
                std::cout << "  table " << table_index << " anchors=";
                for (const auto anchor : table.column_anchor_x_points) {
                    std::cout << ' ' << anchor;
                }
                std::cout << " rows=" << table.rows.size() << '\n';
                for (std::size_t row_index = 0; row_index < table.rows.size();
                     ++row_index) {
                    const auto &row = table.rows[row_index];
                    std::cout << "    row " << row_index << ':';
                    for (const auto &cell : row.cells) {
                        std::cout << " [";
                        if (cell.has_text) {
                            std::cout << cell.text;
                        }
                        std::cout << "]";
                    }
                    std::cout << '\n';
                }
            }
        }
        if (raw_document != nullptr) {
            FPDF_CloseDocument(raw_document);
        }
    }

    return 0;
}
