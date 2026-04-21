#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>
#include <optional>
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

auto append_target_paragraph(
    featherdoc::Document &doc, std::string_view prefix,
    std::string_view target_text, std::string_view suffix,
    std::optional<std::string_view> font_family = std::nullopt,
    std::optional<std::string_view> language = std::nullopt) -> bool {
    auto paragraph = append_body_paragraph(doc, prefix);
    if (!paragraph.has_next()) {
        return false;
    }

    auto target_run = paragraph.add_run(std::string{target_text});
    if (!target_run.has_next()) {
        return false;
    }

    if (font_family.has_value() && !target_run.set_font_family(*font_family)) {
        return false;
    }
    if (language.has_value() && !target_run.set_language(*language)) {
        return false;
    }

    return paragraph.add_run(std::string{suffix}).has_next();
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "run_font_language_visual.docx";

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
    if (!paragraph.set_text("Run font/language CLI visual baseline") ||
        !paragraph
             .add_run(" Compare direct font-family changes against language-tag metadata changes on a single page.")
             .has_next()) {
        print_document_error(doc, "write title");
        return 1;
    }

    if (!append_target_paragraph(
            doc, "Run font-family set target: ",
            "MONO SHIFT 0123456789 ABCD",
            " should switch from proportional body text to Courier New when the CLI applies a direct run font family.")) {
        std::cerr << "failed to append run font-family set target\n";
        return 1;
    }

    if (!append_target_paragraph(
            doc, "Run font-family clear target: ",
            "MONO RESET 0123456789 ABCD",
            " should fall back from Courier New to the default proportional body font when the CLI clears the direct font family.",
            "Courier New")) {
        std::cerr << "failed to append run font-family clear target\n";
        return 1;
    }

    if (!append_target_paragraph(
            doc, "Run language set target: ",
            "LANG TAG INSERT 2026 alpha beta",
            " should keep the same visible layout while the CLI adds a ja-JP language tag.")) {
        std::cerr << "failed to append run language set target\n";
        return 1;
    }

    if (!append_target_paragraph(
            doc, "Run language clear target: ",
            "LANG TAG REMOVE 2026 alpha beta",
            " should keep the same visible layout while the CLI removes the existing ja-JP language tag.",
            std::nullopt, "ja-JP")) {
        std::cerr << "failed to append run language clear target\n";
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Trailing anchor: this note should stay below the four run targets so the before/after screenshots expose any unexpected layout drift.");
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
