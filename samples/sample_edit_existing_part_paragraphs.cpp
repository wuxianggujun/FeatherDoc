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

bool require_step(bool ok, std::string_view operation) {
    if (!ok) {
        std::cerr << operation << " failed\n";
    }
    return ok;
}

bool append_text_tail(featherdoc::Paragraph paragraph, std::string_view text) {
    if (!paragraph.has_next()) {
        return false;
    }
    return paragraph.add_run(std::string{text}).has_next();
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "edit_existing_part_paragraphs.docx";

    if (output_path.has_parent_path()) {
        std::error_code directory_error;
        fs::create_directories(output_path.parent_path(), directory_error);
        if (directory_error) {
            std::cerr << "failed to create output directory: "
                      << output_path.parent_path().string() << '\n';
            return 1;
        }
    }

    featherdoc::Document seed(output_path);
    if (const auto error = seed.create_empty()) {
        print_document_error(seed, "create_empty");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    auto body_paragraph = seed.paragraphs();
    if (!body_paragraph.set_text("TemplatePart paragraph append sample")) {
        print_document_error(seed, "set body intro");
        return 1;
    }
    if (!body_paragraph
             .add_run(" Reopen the document and append body/header/footer paragraphs "
                      "through TemplatePart handles.")
             .has_next()) {
        std::cerr << "add body intro tail failed\n";
        return 1;
    }

    auto &header_paragraph = seed.ensure_section_header_paragraphs(0);
    if (!header_paragraph.set_text("Header seed")) {
        print_document_error(seed, "set header seed");
        return 1;
    }

    auto &footer_paragraph = seed.ensure_section_footer_paragraphs(0);
    if (!footer_paragraph.set_text("Footer seed")) {
        print_document_error(seed, "set footer seed");
        return 1;
    }

    if (const auto error = seed.save()) {
        print_document_error(seed, "save seed");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    featherdoc::Document doc(output_path);
    if (const auto error = doc.open()) {
        print_document_error(doc, "open");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    auto body_template = doc.body_template();
    if (!body_template) {
        std::cerr << "body_template unavailable\n";
        return 1;
    }
    auto appended_body = body_template.append_paragraph(
        "Body appended through TemplatePart",
        featherdoc::formatting_flag::bold);
    if (!require_step(append_text_tail(appended_body, " with a follow-up run."),
                      "append body paragraph tail")) {
        return 1;
    }

    auto header_template = doc.section_header_template(0);
    if (!header_template) {
        std::cerr << "header_template unavailable\n";
        return 1;
    }
    auto appended_header = header_template.append_paragraph("Header appended");
    if (!require_step(append_text_tail(appended_header, " after reopening."),
                      "append header paragraph tail")) {
        return 1;
    }

    auto footer_template = doc.section_footer_template(0);
    if (!footer_template) {
        std::cerr << "footer_template unavailable\n";
        return 1;
    }
    auto appended_footer = footer_template.append_paragraph();
    if (!require_step(append_text_tail(appended_footer, "Footer appended after reopening."),
                      "append footer paragraph tail")) {
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
