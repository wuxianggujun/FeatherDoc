#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>
#include <string>
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

auto add_text(featherdoc::Paragraph paragraph, std::string_view text,
              featherdoc::formatting_flag formatting =
                  featherdoc::formatting_flag::none) -> bool {
    return paragraph.add_run(std::string{text}, formatting).has_next();
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "paragraph_run_style_visual.docx";

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
    if (!paragraph.set_text("Paragraph/run style CLI visual baseline") ||
        !paragraph
             .add_run(" Compare paragraph growth/shrink against run-level emphasis on the same page.")
             .has_next()) {
        print_document_error(doc, "write title");
        return 1;
    }

    auto paragraph_set_target = append_body_paragraph(
        doc,
        "Paragraph set target: this line starts as regular body text and should jump to Heading 2 when the CLI applies a paragraph style.");
    if (!paragraph_set_target.has_next()) {
        std::cerr << "failed to append paragraph set target\n";
        return 1;
    }

    auto paragraph_clear_target = append_body_paragraph(
        doc,
        "Paragraph clear target: this line starts as Heading 2 and should fall back to Normal when the CLI clears the paragraph style.");
    if (!paragraph_clear_target.has_next() ||
        !doc.set_paragraph_style(paragraph_clear_target, "Heading2")) {
        print_document_error(doc, "configure paragraph clear target");
        return 1;
    }

    auto run_set_target = append_body_paragraph(doc, "Run set target: ");
    if (!run_set_target.has_next() || !add_text(run_set_target, "BOLD ME") ||
        !add_text(run_set_target,
                  " while the prefix and suffix remain regular body text.")) {
        std::cerr << "failed to append run set target\n";
        return 1;
    }

    auto run_clear_target = append_body_paragraph(doc, "Run clear target: ");
    if (!run_clear_target.has_next()) {
        std::cerr << "failed to append run clear target\n";
        return 1;
    }
    auto strong_run = run_clear_target.add_run("UNBOLD ME");
    if (!strong_run.has_next() || !doc.set_run_style(strong_run, "Strong") ||
        !add_text(run_clear_target,
                  " while the prefix and suffix remain regular body text.")) {
        print_document_error(doc, "configure run clear target");
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Trailing anchor: this note stays after the four mutation targets so before/after screenshots expose any unexpected layout drift.");
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
