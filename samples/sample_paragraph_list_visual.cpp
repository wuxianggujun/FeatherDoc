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

auto append_list_paragraph(featherdoc::Document &doc, std::string_view text,
                           featherdoc::list_kind kind) -> bool {
    auto paragraph = append_body_paragraph(doc, text);
    return paragraph.has_next() && doc.set_paragraph_list(paragraph, kind);
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "paragraph_list_visual.docx";

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
    if (!paragraph.set_text("Paragraph list CLI visual baseline") ||
        !paragraph
             .add_run(" Compare set-list, restart-list, and clear-list mutations on one page.")
             .has_next()) {
        print_document_error(doc, "write title");
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Set target: this plain paragraph should gain a bullet marker when the CLI applies set-paragraph-list.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append set target paragraph\n";
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Restart witness: the next two paragraphs already belong to one decimal list, so the restart target should render as a fresh item 1.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append restart witness paragraph\n";
        return 1;
    }

    if (!append_list_paragraph(
            doc, "Existing decimal item A stays in the original list instance.",
            featherdoc::list_kind::decimal)) {
        std::cerr << "failed to append first decimal list item\n";
        return 1;
    }

    if (!append_list_paragraph(
            doc, "Existing decimal item B should remain item 2 in the same list.",
            featherdoc::list_kind::decimal)) {
        std::cerr << "failed to append second decimal list item\n";
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Restart target: this plain paragraph should restart as item 1 when the CLI applies restart-paragraph-list.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append restart target paragraph\n";
        return 1;
    }

    if (!append_list_paragraph(
            doc,
            "Clear target: this paragraph starts as a bullet item and should fall back to normal body text when the CLI clears the list.",
            featherdoc::list_kind::bullet)) {
        std::cerr << "failed to append clear target paragraph\n";
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Trailing anchor: every mutation should remain on a single page so the before/after screenshots expose numbering drift without pagination noise.");
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
