#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>
#include <string_view>

namespace fs = std::filesystem;

namespace {

void print_document_error(const featherdoc::Document &doc, std::string_view operation) {
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

auto append_body_paragraph(featherdoc::Document &doc, std::string_view text)
    -> featherdoc::Paragraph {
    auto paragraph = doc.paragraphs();
    while (paragraph.has_next()) {
        paragraph.next();
    }

    return paragraph.insert_paragraph_after(std::string{text});
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1]) : fs::current_path() / "page_number_fields.docx";

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

    auto intro = doc.paragraphs();
    if (!intro.has_next() || !intro.add_run("Template part field sample").has_next()) {
        std::cerr << "failed to initialize the first paragraph\n";
        return 1;
    }

    append_body_paragraph(doc, "This sample keeps the body simple and focuses on header/footer fields.");
    append_body_paragraph(doc, "Open the generated DOCX in Microsoft Word and update fields if needed.");

    auto &header = doc.ensure_section_header_paragraphs(0);
    auto &footer = doc.ensure_section_footer_paragraphs(0);
    if (!header.has_next() || !footer.has_next()) {
        std::cerr << "failed to materialize section header/footer parts\n";
        return 1;
    }

    if (!header.set_text("Current page")) {
        std::cerr << "failed to initialize the header text\n";
        return 1;
    }
    if (!footer.set_text("Total pages")) {
        std::cerr << "failed to initialize the footer text\n";
        return 1;
    }

    auto header_template = doc.section_header_template(0);
    auto footer_template = doc.section_footer_template(0);
    if (!header_template || !footer_template) {
        std::cerr << "failed to access the section header/footer templates\n";
        return 1;
    }

    if (!header_template.append_page_number_field()) {
        print_document_error(doc, "append_page_number_field");
        return 1;
    }
    if (!footer_template.append_total_pages_field()) {
        print_document_error(doc, "append_total_pages_field");
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
