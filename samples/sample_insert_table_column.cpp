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

bool set_seed_column_widths(featherdoc::Table table) {
    return table.has_next() && table.set_column_width_twips(0U, 2000U) &&
           table.set_column_width_twips(1U, 2000U);
}

bool set_two_cell_values(featherdoc::TableRow row, std::string_view first,
                         std::string_view second) {
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
    return cell.set_text(std::string{second});
}

bool set_seed_header_styles(featherdoc::TableRow row) {
    auto cell = row.cells();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7")) {
        return false;
    }

    cell.next();
    return cell.has_next() && cell.set_fill_color("E2F0D9");
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1]) : fs::current_path() / "insert_table_column.docx";

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
    if (!paragraph.set_text("Table column insertion sample")) {
        print_document_error(seed, "set_text");
        return 1;
    }
    if (!paragraph
             .add_run(
                 " Reopen the document, insert one cloned column after the base column, "
                 "insert another before the result column, and keep editing the new cells "
                 "while the inserted columns inherit the source grid widths.")
             .has_next()) {
        std::cerr << "add_run failed\n";
        return 1;
    }

    auto table = seed.append_table(2, 2);
    if (!table.has_next() || !table.set_width_twips(8000U) ||
        !table.set_style_id("TableGrid") ||
        !table.set_layout_mode(featherdoc::table_layout_mode::fixed) ||
        !require_step(set_seed_column_widths(table), "set seed column widths")) {
        std::cerr << "failed to configure seed table\n";
        return 1;
    }

    auto header_row = table.rows();
    if (!header_row.has_next() ||
        !require_step(set_seed_header_styles(header_row), "set header styles") ||
        !require_step(set_two_cell_values(header_row, "Base", "Result"),
                      "set header values")) {
        return 1;
    }

    auto body_row = header_row;
    body_row.next();
    if (!body_row.has_next() ||
        !require_step(set_two_cell_values(body_row, "Owner", "Completed"),
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
        !paragraph
             .add_run(" The final table should end with four columns: Base, Priority, "
                      "Status, and Result, with the inserted columns inheriting the base "
                      "and result grid widths.")
             .has_next()) {
        std::cerr << "failed to update intro paragraph\n";
        return 1;
    }

    table = doc.tables();
    if (!table.has_next()) {
        std::cerr << "missing target table\n";
        return 1;
    }

    auto header_left = table.rows().cells();
    if (!header_left.has_next()) {
        std::cerr << "missing base header cell\n";
        return 1;
    }

    auto inserted_after = header_left.insert_cell_after();
    if (!inserted_after.has_next() ||
        !require_step(inserted_after.set_fill_color("FFF2CC"),
                      "color inserted-after header") ||
        !require_step(inserted_after.set_text("Priority"),
                      "set inserted-after header")) {
        return 1;
    }

    auto updated_body_row = table.rows();
    updated_body_row.next();
    if (!updated_body_row.has_next()) {
        std::cerr << "missing body row after first insertion\n";
        return 1;
    }

    auto inserted_after_body = updated_body_row.cells();
    inserted_after_body.next();
    if (!inserted_after_body.has_next() ||
        !require_step(inserted_after_body.set_fill_color("FFF2CC"),
                      "color inserted-after body cell") ||
        !require_step(inserted_after_body.set_text("High"),
                      "set inserted-after body cell")) {
        return 1;
    }

    auto header_result = table.rows().cells();
    header_result.next();
    header_result.next();
    if (!header_result.has_next()) {
        std::cerr << "missing result header cell after first insertion\n";
        return 1;
    }

    auto inserted_before = header_result.insert_cell_before();
    if (!inserted_before.has_next() ||
        !require_step(inserted_before.set_fill_color("FCE4D6"),
                      "color inserted-before header") ||
        !require_step(inserted_before.set_text("Status"),
                      "set inserted-before header")) {
        return 1;
    }

    updated_body_row = table.rows();
    updated_body_row.next();
    if (!updated_body_row.has_next()) {
        std::cerr << "missing body row after second insertion\n";
        return 1;
    }

    auto inserted_before_body = updated_body_row.cells();
    inserted_before_body.next();
    inserted_before_body.next();
    if (!inserted_before_body.has_next() ||
        !require_step(inserted_before_body.set_fill_color("FCE4D6"),
                      "color inserted-before body cell") ||
        !require_step(inserted_before_body.set_text("Ready"),
                      "set inserted-before body cell")) {
        return 1;
    }

    auto result_body = updated_body_row.cells();
    result_body.next();
    result_body.next();
    result_body.next();
    if (!result_body.has_next() ||
        !require_step(result_body.set_text(
                          "Reached through insert_cell_before/after after reopening the document."),
                      "set final result body cell")) {
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
