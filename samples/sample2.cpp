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

    featherdoc::Paragraph p =
        doc.paragraphs().insert_paragraph_after("You can insert text in ");
    p.add_run("italic, ", featherdoc::formatting_flag::none);
    p.add_run("bold, ", featherdoc::formatting_flag::bold);
    p.add_run("underline, ", featherdoc::formatting_flag::underline);
    p.add_run("superscript", featherdoc::formatting_flag::superscript);
    p.add_run(" or ");
    p.add_run("subscript, ", featherdoc::formatting_flag::subscript);
    p.add_run("small caps, ", featherdoc::formatting_flag::smallcaps);
    p.add_run("and shadows, ", featherdoc::formatting_flag::shadow);
    p.add_run("and of course ");
    p.add_run("combine them.",
              featherdoc::formatting_flag::bold |
                  featherdoc::formatting_flag::italic |
                  featherdoc::formatting_flag::underline |
                  featherdoc::formatting_flag::smallcaps);

    if (const auto error = doc.save()) {
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

    return 0;
}
