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

bool require_step(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
        return false;
    }
    return true;
}

bool apply_style(featherdoc::Document &doc, featherdoc::Paragraph paragraph,
                 std::string_view style_id, std::string_view text) {
    return paragraph.has_next() && doc.set_paragraph_style(paragraph, style_id) &&
           paragraph.set_text(std::string{text});
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path = argc > 1 ? fs::path(argv[1])
                                          : fs::current_path() / "style_linked_numbering.docx";

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

    auto heading_one_style = featherdoc::paragraph_style_definition{};
    heading_one_style.name = "Legal Heading 1";
    heading_one_style.based_on = std::string{"Heading1"};
    heading_one_style.is_quick_format = true;
    heading_one_style.paragraph_bidi = false;
    if (!doc.ensure_paragraph_style("LegalHeading1", heading_one_style)) {
        print_document_error(doc, "ensure_paragraph_style(LegalHeading1)");
        return 1;
    }

    auto heading_two_style = featherdoc::paragraph_style_definition{};
    heading_two_style.name = "Legal Heading 2";
    heading_two_style.based_on = std::string{"Heading2"};
    heading_two_style.is_quick_format = true;
    heading_two_style.paragraph_bidi = false;
    if (!doc.ensure_paragraph_style("LegalHeading2", heading_two_style)) {
        print_document_error(doc, "ensure_paragraph_style(LegalHeading2)");
        return 1;
    }

    auto body_style = featherdoc::paragraph_style_definition{};
    body_style.name = "Legal Body";
    body_style.based_on = std::string{"Normal"};
    body_style.run_font_family = std::string{"Segoe UI"};
    body_style.run_east_asia_font_family = std::string{"Microsoft YaHei"};
    body_style.run_language = std::string{"en-US"};
    body_style.run_east_asia_language = std::string{"zh-CN"};
    body_style.is_quick_format = true;
    if (!doc.ensure_paragraph_style("LegalBody", body_style)) {
        print_document_error(doc, "ensure_paragraph_style(LegalBody)");
        return 1;
    }

    auto numbering_definition = featherdoc::numbering_definition{};
    numbering_definition.name = "LegalOutline";
    numbering_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };

    const auto numbering_id = doc.ensure_style_linked_numbering(
        numbering_definition,
        {featherdoc::paragraph_style_numbering_link{"LegalHeading1", 0U},
         featherdoc::paragraph_style_numbering_link{"LegalHeading2", 1U}});
    if (!numbering_id.has_value()) {
        print_document_error(doc, "ensure_style_linked_numbering");
        return 1;
    }

    auto paragraph = doc.paragraphs();
    if (!require_step(apply_style(
                          doc, paragraph, "LegalBody",
                          "This sample demonstrates paragraph styles inheriting a shared "
                          "custom numbering definition."),
                      "failed to configure intro paragraph")) {
        return 1;
    }

    auto chapter_one = paragraph.insert_paragraph_after("");
    if (!require_step(apply_style(doc, chapter_one, "LegalHeading1", "Scope and Purpose"),
                      "failed to configure chapter one")) {
        return 1;
    }

    auto section_one = chapter_one.insert_paragraph_after("");
    if (!require_step(
            apply_style(doc, section_one, "LegalHeading2", "Objectives and review steps"),
            "failed to configure section one")) {
        return 1;
    }

    auto section_body = section_one.insert_paragraph_after("");
    if (!require_step(apply_style(doc, section_body, "LegalBody",
                                  "The section body keeps a plain paragraph style so the "
                                  "numbering only appears on the heading styles."),
                      "failed to configure section body")) {
        return 1;
    }

    auto section_two = section_body.insert_paragraph_after("");
    if (!require_step(
            apply_style(doc, section_two, "LegalHeading2", "Visual validation checklist"),
            "failed to configure section two")) {
        return 1;
    }

    auto chapter_two = section_two.insert_paragraph_after("");
    if (!require_step(apply_style(doc, chapter_two, "LegalHeading1", "Release Sign-Off"),
                      "failed to configure chapter two")) {
        return 1;
    }

    auto section_three = chapter_two.insert_paragraph_after("");
    if (!require_step(
            apply_style(doc, section_three, "LegalHeading2", "Evidence package readiness"),
            "failed to configure section three")) {
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
