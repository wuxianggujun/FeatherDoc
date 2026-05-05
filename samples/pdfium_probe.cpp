#include <featherdoc/pdf/pdf_parser.hpp>

#include <filesystem>
#include <iostream>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "usage: featherdoc_pdfium_probe <input.pdf>\n";
        return 2;
    }

    const std::filesystem::path input = argv[1];
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
    return 0;
}
