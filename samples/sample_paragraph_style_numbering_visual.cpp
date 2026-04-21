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

bool configure_paragraph(featherdoc::Document &doc, featherdoc::Paragraph paragraph,
                         std::string_view style_id, std::string_view text) {
    return paragraph.has_next() && doc.set_paragraph_style(paragraph, style_id) &&
           paragraph.set_text(std::string{text});
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "paragraph_style_numbering_visual.docx";

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

    auto legal_heading_style = featherdoc::paragraph_style_definition{};
    legal_heading_style.name = "Legal Heading";
    legal_heading_style.based_on = std::string{"Heading1"};
    legal_heading_style.is_quick_format = true;
    legal_heading_style.paragraph_bidi = false;
    if (!doc.ensure_paragraph_style("LegalHeading", legal_heading_style)) {
        print_document_error(doc, "ensure_paragraph_style(LegalHeading)");
        return 1;
    }

    auto legal_subheading_style = featherdoc::paragraph_style_definition{};
    legal_subheading_style.name = "Legal Subheading";
    legal_subheading_style.based_on = std::string{"Heading2"};
    legal_subheading_style.is_quick_format = true;
    legal_subheading_style.paragraph_bidi = false;
    if (!doc.ensure_paragraph_style("LegalSubheading", legal_subheading_style)) {
        print_document_error(doc, "ensure_paragraph_style(LegalSubheading)");
        return 1;
    }

    auto body_numbered_style = featherdoc::paragraph_style_definition{};
    body_numbered_style.name = "Body Numbered";
    body_numbered_style.based_on = std::string{"Normal"};
    body_numbered_style.is_quick_format = true;
    body_numbered_style.paragraph_bidi = true;
    if (!doc.ensure_paragraph_style("BodyNumbered", body_numbered_style)) {
        print_document_error(doc, "ensure_paragraph_style(BodyNumbered)");
        return 1;
    }

    auto baseline_definition = featherdoc::numbering_definition{};
    baseline_definition.name = "BodyStyleOutlineBaseline";
    baseline_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 4U, 0U, "[%1]"},
    };

    const auto baseline_definition_id =
        doc.ensure_numbering_definition(baseline_definition);
    if (!baseline_definition_id.has_value()) {
        print_document_error(doc, "ensure_numbering_definition");
        return 1;
    }

    if (!doc.set_paragraph_style_numbering("BodyNumbered", *baseline_definition_id,
                                           0U)) {
        print_document_error(doc, "set_paragraph_style_numbering(BodyNumbered)");
        return 1;
    }

    auto paragraph = doc.paragraphs();
    if (!paragraph.set_text("Paragraph style numbering CLI visual baseline") ||
        !paragraph
             .add_run(
                 " Compare style-linked numbering assignment, nested heading numbering, and style-numbering removal on one page.")
             .has_next()) {
        print_document_error(doc, "write title");
        return 1;
    }

    auto legal_heading = paragraph.insert_paragraph_after("");
    if (!configure_paragraph(
            doc, legal_heading, "LegalHeading",
            "Legal heading target: this paragraph already uses the LegalHeading style, so style numbering should appear only after the CLI links a numbering definition to that style.")) {
        std::cerr << "failed to configure LegalHeading paragraph\n";
        return 1;
    }

    auto legal_subheading = legal_heading.insert_paragraph_after("");
    if (!configure_paragraph(
            doc, legal_subheading, "LegalSubheading",
            "Legal subheading target: this paragraph already uses the LegalSubheading style and should become a nested style-numbered child only in the dual-style case.")) {
        std::cerr << "failed to configure LegalSubheading paragraph\n";
        return 1;
    }

    auto body_numbered = legal_subheading.insert_paragraph_after("");
    if (!configure_paragraph(
            doc, body_numbered, "BodyNumbered",
            "Body-numbered clear target: this paragraph starts with baseline style numbering and paragraph bidi enabled, so clearing the style numbering should remove only the visible marker.")) {
        std::cerr << "failed to configure BodyNumbered paragraph\n";
        return 1;
    }

    auto trailing_anchor = body_numbered.insert_paragraph_after(
        "Trailing anchor: every mutation should remain on a single page so the before-and-after screenshots isolate style-numbering changes without pagination drift.");
    if (!trailing_anchor.has_next()) {
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
