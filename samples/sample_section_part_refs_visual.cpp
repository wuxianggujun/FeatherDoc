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

bool append_filler_lines(featherdoc::Document &doc,
                         std::string_view prefix,
                         std::size_t count) {
    for (std::size_t index = 0; index < count; ++index) {
        if (!append_body_paragraph(
                doc,
                std::string(prefix) + " line " + std::to_string(index + 1U) +
                    " keeps section 2 long enough to expose page 3 and page 4.")) {
            return false;
        }
    }

    return true;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "section_part_refs_visual.docx";

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
    if (!intro.has_next() ||
        !intro.set_text("SECTION 0 CONTROL PAGE. This page owns the shared header and footer parts.")) {
        print_document_error(doc, "seed section 0 body");
        return 1;
    }

    if (!append_body_paragraph(
            doc,
            "Page 1 should show ALPHA SHARED HEADER at the top and ALPHA SHARED FOOTER at the bottom.") ||
        !append_body_paragraph(
            doc,
            "Later CLI cases reassign these shared parts into section 2 and remove section-specific parts from section 1.")) {
        print_document_error(doc, "append section 0 body");
        return 1;
    }

    auto &section0_header = doc.ensure_section_header_paragraphs(0U);
    auto &section0_footer = doc.ensure_section_footer_paragraphs(0U);
    if (!section0_header.has_next() ||
        !section0_header.set_text("ALPHA SHARED HEADER") ||
        !section0_footer.has_next() ||
        !section0_footer.set_text("ALPHA SHARED FOOTER")) {
        print_document_error(doc, "seed section 0 header/footer");
        return 1;
    }

    if (!doc.append_section(false)) {
        print_document_error(doc, "append section 1");
        return 1;
    }

    if (!append_body_paragraph(
            doc,
            "SECTION 1 REMOVABLE PART PAGE. This page starts with its own header and first-page footer.") ||
        !append_body_paragraph(
            doc,
            "Page 2 should show BETA REMOVABLE HEADER at the top and BETA FIRST FOOTER at the bottom before part-removal cases run.") ||
        !append_body_paragraph(
            doc,
            "After remove-header-part / remove-footer-part, the page 2 header or footer should disappear without moving the body copy.")) {
        print_document_error(doc, "append section 1 body");
        return 1;
    }

    auto &section1_header = doc.ensure_section_header_paragraphs(1U);
    auto &section1_first_footer = doc.ensure_section_footer_paragraphs(
        1U, featherdoc::section_reference_kind::first_page);
    if (!section1_header.has_next() ||
        !section1_header.set_text("BETA REMOVABLE HEADER") ||
        !section1_first_footer.has_next() ||
        !section1_first_footer.set_text("BETA FIRST FOOTER")) {
        print_document_error(doc, "seed section 1 header/footer");
        return 1;
    }

    if (!doc.append_section(false)) {
        print_document_error(doc, "append section 2");
        return 1;
    }

    if (!append_body_paragraph(
            doc,
            "SECTION 2 TARGET PAGES. This section starts with no header or footer parts of its own.") ||
        !append_body_paragraph(
            doc,
            "Page 3 should stay headerless and footerless in the baseline, then pick up ALPHA SHARED FOOTER after assign-section-footer.") ||
        !append_body_paragraph(
            doc,
            "Page 4 should stay headerless in the baseline, then pick up ALPHA SHARED HEADER after assign-section-header --kind even.")) {
        print_document_error(doc, "append section 2 intro");
        return 1;
    }

    if (!append_filler_lines(doc, "SECTION 2 FILLER", 120U)) {
        print_document_error(doc, "append section 2 filler");
        return 1;
    }

    if (!append_body_paragraph(
            doc,
            "SECTION 2 CLOSING NOTE. The final page should still keep the section body readable after header/footer references change.")) {
        print_document_error(doc, "append section 2 filler");
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
