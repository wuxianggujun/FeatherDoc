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

bool seed_headers_and_footers(featherdoc::Document &doc) {
    auto &default_header = doc.ensure_section_header_paragraphs(0U);
    auto &even_header = doc.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto &default_footer = doc.ensure_section_footer_paragraphs(0U);
    auto &first_footer = doc.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);

    if (!default_header.has_next() ||
        !default_header.set_text(
            "Default header control. Page 1 and page 3 should keep this line.") ||
        !even_header.has_next() ||
        !even_header.set_text(
            "Pending even header update") ||
        !default_footer.has_next() ||
        !default_footer.set_text(
            "Default footer control. Page 2 and page 3 should keep this line.") ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            "Pending first footer update")) {
        return false;
    }

    return true;
}

bool seed_body(featherdoc::Document &doc) {
    auto body_template = doc.body_template();
    if (!body_template) {
        return false;
    }

    auto intro = body_template.paragraphs();
    if (!intro.has_next() ||
        !intro.set_text(
            "Section text multiline visual regression sample. Review page 1 for "
            "the first-page footer case and page 2 for the even-header case.")) {
        return false;
    }

    if (!body_template.append_paragraph(
             "The default header and footer lines act as controls and must stay "
             "single-line in the mutated documents.")
             .has_next()) {
        return false;
    }

    for (std::size_t line_index = 0U; line_index < 150U; ++line_index) {
        auto paragraph = body_template.append_paragraph(
            "Section text filler line " + std::to_string(line_index + 1U) +
            " keeps the document long enough to expose page 1, page 2, and page 3.");
        if (!paragraph.has_next()) {
            return false;
        }
    }

    if (!body_template.append_paragraph(
             "Page 3 should still show the default header and default footer "
             "controls without overlap or clipping.")
             .has_next()) {
        return false;
    }

    return true;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "section_text_multiline_visual.docx";

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

    if (!seed_headers_and_footers(doc) || !seed_body(doc)) {
        print_document_error(doc, "seed document");
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
