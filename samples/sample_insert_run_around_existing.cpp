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

bool rewrite_run_ring(featherdoc::Run anchor, std::string_view before_text,
                      featherdoc::formatting_flag before_formatting,
                      std::string_view after_text) {
    if (!anchor.has_next()) {
        return false;
    }

    auto before = anchor.insert_run_before(std::string{before_text}, before_formatting);
    if (!before.has_next()) {
        return false;
    }

    auto after = anchor.insert_run_after(std::string{after_text});
    return after.has_next();
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path = argc > 1 ? fs::path(argv[1])
                                          : fs::current_path() / "insert_run_around_existing.docx";

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
    if (!body_paragraph.has_next() || !body_paragraph.set_text("body anchor run")) {
        std::cerr << "failed to seed body paragraph\n";
        return 1;
    }

    auto &header_paragraph = seed.ensure_section_header_paragraphs(0);
    if (!header_paragraph.has_next() || !header_paragraph.set_text("header anchor run")) {
        std::cerr << "failed to seed header paragraph\n";
        return 1;
    }

    auto &footer_paragraph = seed.ensure_section_footer_paragraphs(0);
    if (!footer_paragraph.has_next() || !footer_paragraph.set_text("footer anchor run")) {
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

    auto body_runs = doc.paragraphs().runs();
    if (!rewrite_run_ring(body_runs, "Body run inserted before ",
                          featherdoc::formatting_flag::bold, " after the original body anchor.")) {
        std::cerr << "failed to insert body runs around anchor\n";
        return 1;
    }

    auto header_runs = doc.section_header_paragraphs(0).runs();
    if (!rewrite_run_ring(header_runs, "Header run inserted before ",
                          featherdoc::formatting_flag::none,
                          " after the original header anchor.")) {
        std::cerr << "failed to insert header runs around anchor\n";
        return 1;
    }

    auto footer_runs = doc.section_footer_paragraphs(0).runs();
    if (!rewrite_run_ring(footer_runs, "Footer run inserted before ",
                          featherdoc::formatting_flag::none,
                          " after the original footer anchor.")) {
        std::cerr << "failed to insert footer runs around anchor\n";
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
