#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>
#include <optional>
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
    std::cerr << '\n';
}

void print_bool(std::size_t indent, std::string_view label, bool value) {
    std::cout << std::string(indent, ' ') << label << ": " << (value ? "true" : "false") << '\n';
}

void print_optional_bool(std::size_t indent, std::string_view label,
                         const std::optional<bool> &value) {
    std::cout << std::string(indent, ' ') << label << ": ";
    if (value.has_value()) {
        std::cout << (*value ? "true" : "false");
    } else {
        std::cout << "<none>";
    }
    std::cout << '\n';
}

void print_optional_size_t(std::size_t indent, std::string_view label,
                           const std::optional<std::size_t> &value) {
    std::cout << std::string(indent, ' ') << label << ": ";
    if (value.has_value()) {
        std::cout << *value;
    } else {
        std::cout << "<none>";
    }
    std::cout << '\n';
}

void print_entry(std::size_t indent, std::string_view label,
                 const std::optional<std::string> &value) {
    std::cout << std::string(indent, ' ') << label << ": ";
    if (value.has_value()) {
        std::cout << *value;
    } else {
        std::cout << "<none>";
    }
    std::cout << '\n';
}

void print_part_summary(std::string_view label,
                        const featherdoc::section_part_inspection_summary &summary) {
    std::cout << "  " << label << '\n';
    print_bool(4U, "has_default", summary.has_default);
    print_bool(4U, "has_first", summary.has_first);
    print_bool(4U, "has_even", summary.has_even);
    print_bool(4U, "default_linked_to_previous", summary.default_linked_to_previous);
    print_bool(4U, "first_linked_to_previous", summary.first_linked_to_previous);
    print_bool(4U, "even_linked_to_previous", summary.even_linked_to_previous);
    print_entry(4U, "default_entry_name", summary.default_entry_name);
    print_entry(4U, "first_entry_name", summary.first_entry_name);
    print_entry(4U, "even_entry_name", summary.even_entry_name);
    print_entry(4U, "resolved_default_entry_name", summary.resolved_default_entry_name);
    print_entry(4U, "resolved_first_entry_name", summary.resolved_first_entry_name);
    print_entry(4U, "resolved_even_entry_name", summary.resolved_even_entry_name);
    print_optional_size_t(4U, "resolved_default_section_index",
                          summary.resolved_default_section_index);
    print_optional_size_t(4U, "resolved_first_section_index",
                          summary.resolved_first_section_index);
    print_optional_size_t(4U, "resolved_even_section_index",
                          summary.resolved_even_section_index);
}

void print_section_summary(const featherdoc::section_inspection_summary &section) {
    std::cout << "section[" << section.index << "]\n";
    print_optional_bool(2U, "even_and_odd_headers_enabled",
                        section.even_and_odd_headers_enabled);
    print_bool(2U, "different_first_page_enabled", section.different_first_page_enabled);
    print_part_summary("header", section.header);
    print_part_summary("footer", section.footer);
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path = argc > 1
                                     ? fs::path(argv[1])
                                     : fs::current_path() / "section_inspection_visual.docx";
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
    if (!paragraph.has_next() || !paragraph.set_text("Section 0 body intro")) {
        print_document_error(doc, "initialize first section body");
        return 1;
    }

    auto &section0_default_header = doc.ensure_section_header_paragraphs(0U);
    auto &section0_even_header = doc.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto &section0_first_footer = doc.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    if (!section0_default_header.has_next() ||
        !section0_default_header.set_text("Section 0 default header") ||
        !section0_even_header.has_next() ||
        !section0_even_header.set_text("Section 0 even header") ||
        !section0_first_footer.has_next() ||
        !section0_first_footer.set_text("Section 0 first footer")) {
        print_document_error(doc, "initialize first section header/footer");
        return 1;
    }

    auto body_template = doc.body_template();
    for (std::size_t line_index = 0U; line_index < 80U; ++line_index) {
        auto body_line = body_template.append_paragraph("Section 0 filler line " +
                                                        std::to_string(line_index + 1U));
        if (!body_line.has_next()) {
            print_document_error(doc, "append first section filler");
            return 1;
        }
    }

    if (!doc.append_section(false)) {
        print_document_error(doc, "append second section");
        return 1;
    }

    auto &section1_default_header = doc.ensure_section_header_paragraphs(1U);
    auto &section1_default_footer = doc.ensure_section_footer_paragraphs(1U);
    if (!section1_default_header.has_next() ||
        !section1_default_header.set_text("Section 1 default header") ||
        !section1_default_footer.has_next() ||
        !section1_default_footer.set_text("Section 1 default footer")) {
        print_document_error(doc, "initialize second section header/footer");
        return 1;
    }

    auto section1_body = body_template.append_paragraph("Section 1 body");
    if (!section1_body.has_next()) {
        print_document_error(doc, "append second section body");
        return 1;
    }

    if (!doc.append_section(false)) {
        print_document_error(doc, "append third section");
        return 1;
    }

    auto &section2_default_header = doc.ensure_section_header_paragraphs(2U);
    if (!section2_default_header.has_next() ||
        !section2_default_header.set_text("Section 2 default header")) {
        print_document_error(doc, "initialize third section header");
        return 1;
    }

    auto section2_body = body_template.append_paragraph("Section 2 body without footer");
    if (!section2_body.has_next()) {
        print_document_error(doc, "append third section body");
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    featherdoc::Document reopened(output_path);
    if (const auto error = reopened.open()) {
        print_document_error(reopened, "open");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    const auto sections = reopened.inspect_sections();
    std::cout << "section_count=" << sections.section_count
              << ", header_count=" << sections.header_count
              << ", footer_count=" << sections.footer_count << '\n';
    print_optional_bool(0U, "even_and_odd_headers_enabled",
                        sections.even_and_odd_headers_enabled);
    for (const auto &section : sections.sections) {
        print_section_summary(section);
    }

    if (const auto inspected_section = reopened.inspect_section(2U); inspected_section.has_value()) {
        std::cout << "inspect_section(2)\n";
        print_section_summary(*inspected_section);
    }

    return 0;
}
