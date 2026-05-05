#include <featherdoc.hpp>
#include <featherdoc/pdf/pdf_document_adapter.hpp>
#include <featherdoc/pdf/pdf_writer.hpp>

#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

void print_document_error(const featherdoc::Document &document,
                          std::string_view operation) {
    const auto &error_info = document.last_error();
    std::cerr << operation << " failed";
    if (error_info.code) {
        std::cerr << ": " << error_info.code.message();
    }
    if (!error_info.detail.empty()) {
        std::cerr << " - " << error_info.detail;
    }
    if (!error_info.entry_name.empty()) {
        std::cerr << " [entry=" << error_info.entry_name << ']';
    }
    if (error_info.xml_offset.has_value()) {
        std::cerr << " [xml_offset=" << *error_info.xml_offset << ']';
    }
    std::cerr << '\n';
}

} // namespace

int main(int argc, char **argv) {
    const std::filesystem::path output =
        argc > 1 ? std::filesystem::path(argv[1])
                 : std::filesystem::path("featherdoc-document-probe.pdf");
    auto source_path = output;
    source_path.replace_extension(".docx");

    featherdoc::Document document(source_path);
    if (const auto error = document.create_empty()) {
        print_document_error(document, "create_empty");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    auto paragraph = document.paragraphs();
    if (!paragraph.has_next() ||
        !paragraph.set_text("FeatherDoc document-to-PDF adapter probe")) {
        std::cerr << "failed to seed title paragraph\n";
        return 1;
    }

    paragraph = paragraph.insert_paragraph_after(
        "This file starts as a FeatherDoc Document, moves through the "
        "backend-neutral PdfDocumentLayout model, and is finally written by "
        "the PDFio generator.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append pipeline paragraph\n";
        return 1;
    }

    paragraph = paragraph.insert_paragraph_after(
        "The current adapter intentionally supports a narrow text-only subset: "
        "paragraph order, basic line wrapping, page breaks, A4 size, margins, "
        "and document metadata.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append scope paragraph\n";
        return 1;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions adapter_options;
    adapter_options.metadata.title = "FeatherDoc document PDF adapter probe";
    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, adapter_options);

    featherdoc::pdf::PdfWriterOptions writer_options;
    writer_options.title = adapter_options.metadata.title;
    writer_options.page_size = adapter_options.page_size;

    featherdoc::pdf::PdfioGenerator generator;
    featherdoc::pdf::IPdfGenerator &pdf_generator = generator;
    const auto result = pdf_generator.write(layout, output, writer_options);
    if (!result) {
        std::cerr << result.error_message << '\n';
        return 1;
    }

    std::cout << "wrote " << output.string() << " (" << result.bytes_written
              << " bytes)\n";
    return 0;
}
