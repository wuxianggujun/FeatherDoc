#include <featherdoc/detail/path.hpp>
#include <featherdoc/pdf/pdf_writer.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

namespace {

int run_pdf_install_smoke(const std::filesystem::path &output_path) {
    namespace fs = std::filesystem;

    std::error_code filesystem_error;
    if (output_path.has_parent_path()) {
        fs::create_directories(output_path.parent_path(), filesystem_error);
        if (filesystem_error) {
            std::cerr << "failed to create output directory: "
                      << filesystem_error.message() << '\n';
            return 1;
        }
    }

    fs::remove(output_path, filesystem_error);
    filesystem_error.clear();

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "Installed FeatherDoc Pdf component smoke";

    featherdoc::pdf::PdfPageLayout page;
    featherdoc::pdf::PdfTextRun text;
    text.baseline_origin = {72.0, 720.0};
    text.text = "Installed FeatherDoc::Pdf component smoke";
    text.font_family = "Helvetica";
    text.font_size_points = 14.0;
    page.text_runs.push_back(std::move(text));
    layout.pages.push_back(std::move(page));

    featherdoc::pdf::PdfWriterOptions options;
    options.title = layout.metadata.title;

    featherdoc::pdf::PdfioGenerator generator;
    const auto result = generator.write(layout, output_path, options);
    if (!result) {
        std::cerr << "PDF generation failed: " << result.error_message << '\n';
        return 1;
    }
    if (result.bytes_written == 0U) {
        std::cerr << "PDF generation reported zero bytes\n";
        return 1;
    }

    std::ifstream input(output_path, std::ios::binary);
    std::array<char, 5> signature{};
    if (!input.read(signature.data(), static_cast<std::streamsize>(signature.size())) ||
        std::string(signature.data(), signature.size()) != "%PDF-") {
        std::cerr << "generated file does not have a PDF signature\n";
        return 1;
    }

    std::cout << "Generated install PDF smoke document at "
              << featherdoc::detail::path_to_utf8(output_path) << '\n';
    return 0;
}

} // namespace

#if defined(_WIN32)
int wmain(int argc, wchar_t **argv) {
    const std::filesystem::path output_path =
        argc > 1 ? std::filesystem::path(argv[1])
                 : std::filesystem::path(L"featherdoc-install-pdf-smoke.pdf");
    return run_pdf_install_smoke(output_path);
}
#else
int main(int argc, char **argv) {
    const std::filesystem::path output_path =
        argc > 1 ? std::filesystem::path(argv[1])
                 : std::filesystem::path("featherdoc-install-pdf-smoke.pdf");
    return run_pdf_install_smoke(output_path);
}
#endif
