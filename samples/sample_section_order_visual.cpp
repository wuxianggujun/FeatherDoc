#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>
#include <string_view>

namespace fs = std::filesystem;

namespace {

void print_document_error(const featherdoc::Document &doc,
                          std::string_view operation) {
    const auto &error_info = doc.last_error();
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

bool append_body_paragraph(featherdoc::Document &doc, std::string_view text) {
    auto paragraph = doc.paragraphs();
    while (paragraph.has_next()) {
        paragraph.next();
    }

    return paragraph.insert_paragraph_after(std::string{text}).has_next();
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "section_order_visual.docx";

    if (output_path.has_parent_path()) {
        std::error_code directory_error;
        fs::create_directories(output_path.parent_path(), directory_error);
        if (directory_error) {
            std::cerr << "failed to create output directory: "
                      << output_path.parent_path().string() << '\n';
            return 1;
        }
    }

    featherdoc::Document doc(output_path);
    if (const auto error = doc.create_empty()) {
        print_document_error(doc, "create_empty");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    auto section0 = doc.paragraphs();
    if (!section0.has_next() ||
        !section0.set_text("SECTION 0 BODY PAGE") ||
        !append_body_paragraph(
            doc,
            "Page 1 should show ALPHA HEADER and no footer in the baseline.")) {
        print_document_error(doc, "seed section 0 body");
        return 1;
    }

    auto &section0_header = doc.ensure_section_header_paragraphs(0U);
    if (!section0_header.has_next() ||
        !section0_header.set_text("ALPHA HEADER")) {
        print_document_error(doc, "seed section 0 header");
        return 1;
    }

    if (!doc.append_section(false)) {
        print_document_error(doc, "append section 1");
        return 1;
    }

    if (!append_body_paragraph(doc, "SECTION 1 BODY PAGE") ||
        !append_body_paragraph(
            doc,
            "Page 2 should show BETA HEADER and BETA FIRST FOOTER in the baseline.")) {
        print_document_error(doc, "seed section 1 body");
        return 1;
    }

    auto &section1_header = doc.ensure_section_header_paragraphs(1U);
    auto &section1_first_footer = doc.ensure_section_footer_paragraphs(
        1U, featherdoc::section_reference_kind::first_page);
    if (!section1_header.has_next() ||
        !section1_header.set_text("BETA HEADER") ||
        !section1_first_footer.has_next() ||
        !section1_first_footer.set_text("BETA FIRST FOOTER")) {
        print_document_error(doc, "seed section 1 header/footer");
        return 1;
    }

    if (!doc.append_section(false)) {
        print_document_error(doc, "append section 2");
        return 1;
    }

    if (!append_body_paragraph(doc, "SECTION 2 BODY PAGE") ||
        !append_body_paragraph(
            doc,
            "Page 3 should show no header and no footer in the baseline.")) {
        print_document_error(doc, "seed section 2 body");
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
