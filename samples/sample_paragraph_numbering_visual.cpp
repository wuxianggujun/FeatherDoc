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
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "paragraph_numbering_visual.docx";

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

    auto paragraph = doc.paragraphs();
    if (!paragraph.set_text("Paragraph numbering CLI visual baseline") ||
        !paragraph
             .add_run(" Compare custom decimal and bullet numbering definitions on one page.")
             .has_next()) {
        print_document_error(doc, "write title");
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Root target: this plain paragraph should become custom item (3) when the CLI applies a definition that starts at 3.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append root target paragraph\n";
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Nested parent target: this paragraph should become custom item (3) before a nested child paragraph renders as 3.1.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append nested parent target paragraph\n";
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Nested child target: this paragraph should become custom item (3.1) beneath the nested parent when the CLI applies level 1 numbering.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append nested child target paragraph\n";
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Custom bullet target: this plain paragraph should gain a custom >> bullet marker when the CLI applies a bullet numbering definition.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append custom bullet target paragraph\n";
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Trailing anchor: every mutation should remain on a single page so the before/after screenshots isolate numbering changes without pagination drift.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append trailing anchor paragraph\n";
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
