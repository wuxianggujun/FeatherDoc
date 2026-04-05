#include <featherdoc.hpp>
#include <iostream>

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
        for (auto r = p.runs(); r.has_next(); r.next()) {
            std::cout << r.get_text() << std::endl;
        }
    }

    return 0;
}
