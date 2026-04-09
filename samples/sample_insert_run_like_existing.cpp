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

bool seed_styled_anchor(featherdoc::Document &doc, featherdoc::Paragraph paragraph,
                        std::string_view text, std::string_view style_id,
                        std::string_view language, bool rtl = false) {
    if (!paragraph.has_next() || !paragraph.set_text(std::string{text})) {
        return false;
    }

    auto anchor = paragraph.runs();
    if (!anchor.has_next() || !doc.set_run_style(anchor, style_id) ||
        !anchor.set_language(language)) {
        return false;
    }

    if (rtl && !anchor.set_rtl()) {
        return false;
    }

    return true;
}

bool insert_like_before(featherdoc::Run anchor, std::string_view text) {
    if (!anchor.has_next()) {
        return false;
    }

    auto inserted = anchor.insert_run_like_before();
    return inserted.has_next() && inserted.set_text(std::string{text});
}

bool insert_like_after(featherdoc::Run anchor, std::string_view text) {
    if (!anchor.has_next()) {
        return false;
    }

    auto inserted = anchor.insert_run_like_after();
    return inserted.has_next() && inserted.set_text(std::string{text});
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path = argc > 1 ? fs::path(argv[1])
                                          : fs::current_path() /
                                                "insert_run_like_existing.docx";

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

    if (!seed_styled_anchor(seed, seed.paragraphs(), "Body anchor run", "Strong", "en-US")) {
        std::cerr << "failed to seed body anchor run\n";
        return 1;
    }

    auto &header_paragraph = seed.ensure_section_header_paragraphs(0);
    if (!seed_styled_anchor(seed, header_paragraph, "Header anchor run", "Emphasis",
                            "en-US")) {
        std::cerr << "failed to seed header anchor run\n";
        return 1;
    }

    auto &footer_paragraph = seed.ensure_section_footer_paragraphs(0);
    if (!seed_styled_anchor(seed, footer_paragraph, "نص مرجعي", "Emphasis", "ar-SA",
                            true)) {
        std::cerr << "failed to seed footer anchor run\n";
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

    auto body_anchor = doc.paragraphs().runs();
    if (!body_anchor.has_next() ||
        !insert_like_before(body_anchor, "Body cloned before ") ||
        !insert_like_after(body_anchor, " and after the anchor")) {
        std::cerr << "failed to clone body run formatting around anchor\n";
        return 1;
    }

    auto header_anchor = doc.section_header_paragraphs(0).runs();
    if (!header_anchor.has_next() ||
        !insert_like_before(header_anchor, "Header cloned before ")) {
        std::cerr << "failed to clone header run formatting before anchor\n";
        return 1;
    }

    auto footer_anchor = doc.section_footer_paragraphs(0).runs();
    if (!footer_anchor.has_next() ||
        !insert_like_after(footer_anchor, " لاحقة منسوخة")) {
        std::cerr << "failed to clone footer run formatting after anchor\n";
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
