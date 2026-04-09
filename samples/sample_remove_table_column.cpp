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

bool set_row_values(featherdoc::TableRow row, std::string_view first,
                    std::string_view second, std::string_view third) {
    auto cell = row.cells();
    if (!cell.has_next()) {
        return false;
    }
    if (!cell.set_text(std::string{first})) {
        return false;
    }

    cell.next();
    if (!cell.has_next()) {
        return false;
    }
    if (!cell.set_text(std::string{second})) {
        return false;
    }

    cell.next();
    if (!cell.has_next()) {
        return false;
    }
    return cell.set_text(std::string{third});
}

bool set_header_colors(featherdoc::TableRow row) {
    auto cell = row.cells();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7")) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("FCE4D6")) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("E2F0D9")) {
        return false;
    }

    return true;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1]) : fs::current_path() / "remove_table_column.docx";

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

    auto paragraph = seed.paragraphs();
    if (!paragraph.set_text("Table column removal sample")) {
        print_document_error(seed, "set_text");
        return 1;
    }
    if (!paragraph
             .add_run(
                 " Reopen the document, remove the temporary middle column, and "
                 "continue editing the surviving result column through the same wrapper.")
             .has_next()) {
        std::cerr << "add_run failed\n";
        return 1;
    }

    auto table = seed.append_table(2, 3);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_style_id("TableGrid") ||
        !table.set_layout_mode(featherdoc::table_layout_mode::fixed)) {
        std::cerr << "failed to configure seed table\n";
        return 1;
    }

    auto header_row = table.rows();
    if (!header_row.has_next() || !require_step(set_header_colors(header_row), "set header colors") ||
        !require_step(set_row_values(header_row, "Item", "Temporary", "Result"),
                      "set header values")) {
        return 1;
    }

    auto body_row = header_row;
    body_row.next();
    if (!body_row.has_next() ||
        !require_step(set_row_values(body_row, "Keep this column",
                                     "Remove this temporary column",
                                     "Update this cell after removal"),
                      "set body values")) {
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

    paragraph = doc.paragraphs();
    if (!paragraph.has_next() ||
        !paragraph.add_run(" The final table should visibly keep only the blue and green columns.")
              .has_next()) {
        std::cerr << "failed to update intro paragraph\n";
        return 1;
    }

    table = doc.tables();
    if (!table.has_next()) {
        std::cerr << "missing target table\n";
        return 1;
    }

    auto removable_column = table.rows().cells();
    if (!removable_column.has_next()) {
        std::cerr << "missing first header cell\n";
        return 1;
    }
    removable_column.next();
    if (!removable_column.has_next()) {
        std::cerr << "missing removable middle header cell\n";
        return 1;
    }

    if (!removable_column.remove()) {
        std::cerr << "remove column failed\n";
        return 1;
    }
    if (!removable_column.has_next()) {
        std::cerr << "cell wrapper did not move to the surviving result column\n";
        return 1;
    }
    if (!require_step(removable_column.set_text("Result (survived)"),
                      "update surviving header cell")) {
        return 1;
    }

    auto updated_body_row = table.rows();
    updated_body_row.next();
    if (!updated_body_row.has_next()) {
        std::cerr << "missing body row after column removal\n";
        return 1;
    }

    auto updated_body_cell = updated_body_row.cells();
    if (!updated_body_cell.has_next()) {
        std::cerr << "missing body first cell after column removal\n";
        return 1;
    }
    updated_body_cell.next();
    if (!updated_body_cell.has_next()) {
        std::cerr << "missing surviving body result cell after column removal\n";
        return 1;
    }
    if (!require_step(updated_body_cell.set_text(
                          "Reached through the updated two-column table after removing the middle column."),
                      "update surviving body cell")) {
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
