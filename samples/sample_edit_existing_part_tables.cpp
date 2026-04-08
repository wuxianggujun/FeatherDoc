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

bool set_row_values(featherdoc::TableRow row, std::string_view left,
                    std::string_view right) {
    auto cell = row.cells();
    if (!cell.has_next()) {
        return false;
    }
    if (!cell.set_text(std::string{left})) {
        return false;
    }

    cell.next();
    if (!cell.has_next()) {
        return false;
    }

    return cell.set_text(std::string{right});
}

bool set_row_fill(featherdoc::TableRow row, std::string_view fill_color) {
    for (auto cell = row.cells(); cell.has_next(); cell.next()) {
        if (!cell.set_fill_color(fill_color)) {
            return false;
        }
    }

    return true;
}

bool configure_two_column_table(featherdoc::Table table,
                                std::string_view header_left,
                                std::string_view header_right,
                                std::string_view header_fill,
                                std::string_view body_left,
                                std::string_view body_right) {
    if (!table.has_next()) {
        return false;
    }

    if (!table.set_width_twips(6800U) || !table.set_style_id("TableGrid") ||
        !table.set_layout_mode(featherdoc::table_layout_mode::fixed)) {
        return false;
    }

    auto row = table.rows();
    if (!row.has_next()) {
        return false;
    }
    if (!set_row_fill(row, header_fill) ||
        !set_row_values(row, header_left, header_right)) {
        return false;
    }

    row.next();
    if (!row.has_next()) {
        return false;
    }

    return set_row_values(row, body_left, body_right);
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path = argc > 1 ? fs::path(argv[1])
                                          : fs::current_path() /
                                                "edited_existing_part_tables.docx";

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
    if (!body_paragraph.set_text(
            "Template-part table editing sample. Review the header tables in Word.")) {
        print_document_error(seed, "set body text");
        return 1;
    }

    auto header_paragraph = seed.ensure_section_header_paragraphs(0);
    if (!header_paragraph.has_next() ||
        !header_paragraph.set_text("Header template-part table editing sample")) {
        std::cerr << "failed to seed header paragraph\n";
        return 1;
    }

    auto header_template = seed.section_header_template(0);
    if (!header_template) {
        std::cerr << "failed to access header template\n";
        return 1;
    }

    if (!require_step(configure_two_column_table(
                          header_template.append_table(2, 2), "Section", "Status",
                          "D9EAF7", "Retained header table",
                          "This table should remain in the final header."),
                      "configure retained header table")) {
        return 1;
    }

    if (!require_step(configure_two_column_table(
                          header_template.append_table(2, 2), "Temporary", "State",
                          "FCE4D6", "Middle table",
                          "This highlighted table should be removed."),
                      "configure removable header table")) {
        return 1;
    }

    if (!require_step(configure_two_column_table(
                          header_template.append_table(2, 2), "Final", "Result",
                          "EADCF8", "Pending update",
                          "This header table will be updated after removal."),
                      "configure final header table")) {
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

    header_template = doc.section_header_template(0);
    if (!header_template) {
        std::cerr << "failed to reopen header template\n";
        return 1;
    }

    auto selected_table = header_template.tables();
    selected_table.next();
    if (!selected_table.has_next()) {
        std::cerr << "missing removable middle header table\n";
        return 1;
    }

    if (!selected_table.remove()) {
        std::cerr << "remove header table failed\n";
        return 1;
    }
    if (!selected_table.has_next()) {
        std::cerr << "table wrapper did not move to the surviving header table\n";
        return 1;
    }

    auto final_row = selected_table.rows();
    final_row.next();
    if (!final_row.has_next()) {
        std::cerr << "missing final header table body row after removal\n";
        return 1;
    }
    if (!require_step(set_row_values(
                          final_row, "Final header table",
                          "Reached through the same wrapper after middle-table removal."),
                      "update final header table body row")) {
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
