#include <featherdoc.hpp>
#include <iostream>
#include <string>

namespace {
void print_paragraph_text(featherdoc::Paragraph paragraph) {
    std::string text;
    for (auto run = paragraph.runs(); run.has_next(); run.next()) {
        text += run.get_text();
    }
    std::cout << text << '\n';
}
} // namespace

int main() {
    featherdoc::Document doc("my_test.docx");
    if (const auto error = doc.open()) {
        const auto &error_info = doc.last_error();
        std::cerr << error.message();
        if (!error_info.detail.empty()) {
            std::cerr << ": " << error_info.detail;
        }
        if (!error_info.entry_name.empty()) {
            std::cerr << " [entry=" << error_info.entry_name << ']';
        }
        if (error_info.xml_offset.has_value()) {
            std::cerr << " [xml_offset=" << *error_info.xml_offset << ']';
        }
        std::cerr << '\n';
        return 1;
    }

    for (auto p = doc.paragraphs(); p.has_next(); p.next()) {
        print_paragraph_text(p);
    }

    for (auto table = doc.tables(); table.has_next(); table.next()) {
        for (auto row = table.rows(); row.has_next(); row.next()) {
            for (auto cell = row.cells(); cell.has_next(); cell.next()) {
                for (auto paragraph = cell.paragraphs(); paragraph.has_next();
                     paragraph.next()) {
                    print_paragraph_text(paragraph);
                }
            }
        }
    }

    return 0;
}
