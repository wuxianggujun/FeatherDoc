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
        argc > 1 ? fs::path(argv[1]) : fs::current_path() / "section_page_setup.docx";

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
    if (!intro.has_next() || !intro.add_run("Portrait contract cover").has_next()) {
        std::cerr << "failed to initialize the first paragraph\n";
        return 1;
    }

    append_body_paragraph(doc,
                          "Section 0 keeps portrait orientation with wider left/right margins.");
    append_body_paragraph(doc,
                          "This is a report-style opening page with generous whitespace.");
    append_body_paragraph(doc, "The next section switches to landscape.");

    featherdoc::section_page_setup first_setup{};
    first_setup.orientation = featherdoc::page_orientation::portrait;
    first_setup.width_twips = 12240U;
    first_setup.height_twips = 15840U;
    first_setup.margins.top_twips = 1440U;
    first_setup.margins.bottom_twips = 1440U;
    first_setup.margins.left_twips = 1800U;
    first_setup.margins.right_twips = 1800U;
    first_setup.margins.header_twips = 720U;
    first_setup.margins.footer_twips = 720U;
    first_setup.page_number_start = 5U;
    if (!doc.set_section_page_setup(0, first_setup)) {
        print_document_error(doc, "set_section_page_setup(section 0)");
        return 1;
    }

    if (!doc.append_section(false)) {
        print_document_error(doc, "append_section");
        return 1;
    }

    append_body_paragraph(doc, "Landscape appendix");
    append_body_paragraph(doc,
                          "Section 1 rotates to landscape for wider tables and review notes.");
    append_body_paragraph(doc, "Margins are tighter so more content fits horizontally.");
    append_body_paragraph(doc, "Visual validation should show a wide second page.");

    featherdoc::section_page_setup second_setup{};
    second_setup.orientation = featherdoc::page_orientation::landscape;
    second_setup.width_twips = 15840U;
    second_setup.height_twips = 12240U;
    second_setup.margins.top_twips = 720U;
    second_setup.margins.bottom_twips = 1080U;
    second_setup.margins.left_twips = 1440U;
    second_setup.margins.right_twips = 1440U;
    second_setup.margins.header_twips = 360U;
    second_setup.margins.footer_twips = 540U;
    if (!doc.set_section_page_setup(1, second_setup)) {
        print_document_error(doc, "set_section_page_setup(section 1)");
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
