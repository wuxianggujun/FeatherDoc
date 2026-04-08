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

bool set_row_values(featherdoc::TableRow row, std::string_view item,
                    std::string_view description, std::string_view amount) {
    auto cell = row.cells();
    if (!cell.has_next()) {
        return false;
    }
    if (!cell.set_text(std::string{item})) {
        return false;
    }
    cell.next();
    if (!cell.has_next()) {
        return false;
    }
    if (!cell.set_text(std::string{description})) {
        return false;
    }
    cell.next();
    if (!cell.has_next()) {
        return false;
    }
    return cell.set_text(std::string{amount});
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1]) : fs::current_path() / "insert_table_row.docx";

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
    if (!paragraph.set_text("Table row insertion sample")) {
        print_document_error(seed, "set_text");
        return 1;
    }
    if (!paragraph.add_run(
            " Reopen the document, clone the first item row, and insert it in place.")
             .has_next()) {
        std::cerr << "add_run failed\n";
        return 1;
    }

    auto table = seed.append_table(3, 3);
    if (!require_step(table.set_width_twips(9000U), "set table width") ||
        !require_step(table.set_style_id("TableGrid"), "set table style")) {
        return 1;
    }

    auto row = table.rows();
    if (!row.has_next()) {
        std::cerr << "failed to create header row\n";
        return 1;
    }

    auto cell = row.cells();
    if (!require_step(cell.set_fill_color("D9EAF7"), "set header fill 1") ||
        !require_step(cell.set_text("Item"), "set header text 1")) {
        return 1;
    }
    cell.next();
    if (!require_step(cell.set_fill_color("D9EAF7"), "set header fill 2") ||
        !require_step(cell.set_text("Description"), "set header text 2")) {
        return 1;
    }
    cell.next();
    if (!require_step(cell.set_fill_color("D9EAF7"), "set header fill 3") ||
        !require_step(cell.set_text("Amount"), "set header text 3")) {
        return 1;
    }

    row.next();
    if (!row.has_next()) {
        std::cerr << "failed to create template data row\n";
        return 1;
    }
    if (!require_step(row.set_cant_split(), "set data row cant_split") ||
        !require_step(set_row_values(
                          row, "Template row",
                          "This row will be cloned into the middle.", "1,800.00"),
                      "set template row values")) {
        return 1;
    }
    cell = row.cells();
    if (!require_step(cell.set_fill_color("FFF2CC"), "set data fill 1")) {
        return 1;
    }
    cell.next();
    if (!require_step(cell.set_fill_color("FFF2CC"), "set data fill 2")) {
        return 1;
    }
    cell.next();
    if (!require_step(cell.set_fill_color("FFF2CC"), "set data fill 3")) {
        return 1;
    }

    row.next();
    if (!row.has_next()) {
        std::cerr << "failed to create total row\n";
        return 1;
    }
    if (!require_step(set_row_values(row, "Total", "", "1,800.00"),
                      "set total row values")) {
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

    auto existing_table = doc.tables();
    if (!existing_table.has_next()) {
        std::cerr << "failed to load sample table\n";
        return 1;
    }

    auto source_row = existing_table.rows();
    source_row.next();
    if (!source_row.has_next()) {
        std::cerr << "missing source row for insertion\n";
        return 1;
    }

    auto inserted_row = source_row.insert_row_after();
    if (!inserted_row.has_next()) {
        std::cerr << "insert_row_after failed\n";
        return 1;
    }

    if (!require_step(set_row_values(
                          inserted_row, "Inserted row",
                          "Inserted after reopening while preserving row/cell layout.",
                          "600.00"),
                      "set inserted row values")) {
        return 1;
    }

    auto total_row = inserted_row;
    total_row.next();
    if (!total_row.has_next()) {
        std::cerr << "missing total row after insertion\n";
        return 1;
    }
    if (!require_step(set_row_values(total_row, "Total", "", "2,400.00"),
                      "update total row values")) {
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
