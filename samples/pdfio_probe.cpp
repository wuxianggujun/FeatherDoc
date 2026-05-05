#include <featherdoc/pdf/pdf_writer.hpp>

#include <filesystem>
#include <iostream>

int main(int argc, char **argv) {
    const std::filesystem::path output =
        argc > 1 ? std::filesystem::path(argv[1])
                 : std::filesystem::path("featherdoc-pdfio-probe.pdf");

    const featherdoc::pdf::PdfWriterOptions options;
    const auto layout = featherdoc::pdf::make_pdfio_probe_layout(options);
    featherdoc::pdf::PdfioGenerator generator;
    featherdoc::pdf::IPdfGenerator &pdf_generator = generator;
    const auto result = pdf_generator.write(layout, output, options);
    if (!result) {
        std::cerr << result.error_message << '\n';
        return 1;
    }

    std::cout << "wrote " << output.string() << " (" << result.bytes_written
              << " bytes)\n";
    return 0;
}
