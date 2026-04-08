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

bool append_text_tail(featherdoc::Paragraph paragraph, std::string_view text) {
    if (!paragraph.has_next()) {
        return false;
    }
    return paragraph.add_run(std::string{text}).has_next();
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1]) : fs::current_path() / "insert_paragraph_before.docx";

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
    if (!body_paragraph.has_next() || !body_paragraph.set_text("Body anchor paragraph")) {
        std::cerr << "failed to seed body paragraph\n";
        return 1;
    }

    auto &header_paragraph = seed.ensure_section_header_paragraphs(0);
    if (!header_paragraph.has_next() || !header_paragraph.set_text("Header anchor paragraph")) {
        std::cerr << "failed to seed header paragraph\n";
        return 1;
    }

    auto &footer_paragraph = seed.ensure_section_footer_paragraphs(0);
    if (!footer_paragraph.has_next() || !footer_paragraph.set_text("Footer anchor paragraph")) {
        std::cerr << "failed to seed footer paragraph\n";
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

    auto body_anchor = doc.paragraphs();
    if (!body_anchor.has_next()) {
        std::cerr << "missing body anchor paragraph\n";
        return 1;
    }
    auto inserted_body = body_anchor.insert_paragraph_before(
        "Body paragraph inserted before", featherdoc::formatting_flag::bold);
    if (!append_text_tail(inserted_body, " the original body anchor.")) {
        std::cerr << "failed to insert body paragraph before anchor\n";
        return 1;
    }

    auto header_anchor = doc.section_header_paragraphs(0);
    if (!header_anchor.has_next()) {
        std::cerr << "missing header anchor paragraph\n";
        return 1;
    }
    auto inserted_header = header_anchor.insert_paragraph_before("Header paragraph inserted first");
    if (!append_text_tail(inserted_header, " before the original header anchor.")) {
        std::cerr << "failed to insert header paragraph before anchor\n";
        return 1;
    }

    auto footer_anchor = doc.section_footer_paragraphs(0);
    if (!footer_anchor.has_next()) {
        std::cerr << "missing footer anchor paragraph\n";
        return 1;
    }
    auto inserted_footer = footer_anchor.insert_paragraph_before("Footer paragraph inserted first");
    if (!append_text_tail(inserted_footer, " before the original footer anchor.")) {
        std::cerr << "failed to insert footer paragraph before anchor\n";
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
