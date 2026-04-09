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

bool add_decimal_item(featherdoc::Document &doc, featherdoc::Paragraph paragraph,
                      std::string_view text, bool restart = false) {
    const bool ok = restart ? doc.restart_paragraph_list(paragraph, featherdoc::list_kind::decimal)
                            : doc.set_paragraph_list(paragraph, featherdoc::list_kind::decimal);
    return ok && paragraph.add_run(std::string{text}).has_next();
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path = argc > 1 ? fs::path(argv[1])
                                          : fs::current_path() / "restart_paragraph_list.docx";

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

    auto intro = seed.paragraphs();
    if (!intro.has_next() ||
        !intro.set_text(
            "Seed one decimal list, reopen the document, then start a second list from 1.")) {
        std::cerr << "failed to seed intro paragraph\n";
        return 1;
    }

    auto first_item = intro.insert_paragraph_after("");
    if (!first_item.has_next() ||
        !add_decimal_item(seed, first_item, "Draft release notes")) {
        std::cerr << "failed to seed first list item\n";
        return 1;
    }

    auto second_item = first_item.insert_paragraph_after("");
    if (!second_item.has_next() ||
        !add_decimal_item(seed, second_item, "Verify smoke outputs")) {
        std::cerr << "failed to seed second list item\n";
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

    intro = doc.paragraphs();
    if (!intro.has_next() ||
        !intro.add_run(" The reopened document adds a second decimal list that restarts at 1.")
              .has_next()) {
        std::cerr << "failed to update intro paragraph\n";
        return 1;
    }

    auto existing_list_item = intro;
    existing_list_item.next();
    if (!existing_list_item.has_next()) {
        std::cerr << "missing first seeded list item\n";
        return 1;
    }

    auto existing_second_item = existing_list_item;
    existing_second_item.next();
    if (!existing_second_item.has_next()) {
        std::cerr << "missing second seeded list item\n";
        return 1;
    }

    auto divider = existing_second_item.insert_paragraph_after("Restarted checklist");
    if (!divider.has_next()) {
        std::cerr << "failed to add divider paragraph\n";
        return 1;
    }

    auto restarted_first = divider.insert_paragraph_after("");
    if (!restarted_first.has_next() ||
        !add_decimal_item(doc, restarted_first, "Publish release tag", true)) {
        std::cerr << "failed to add restarted first item\n";
        return 1;
    }

    auto restarted_second = restarted_first.insert_paragraph_after("");
    if (!restarted_second.has_next() ||
        !add_decimal_item(doc, restarted_second, "Announce release")) {
        std::cerr << "failed to add restarted second item\n";
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
