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

    if (!table.set_width_twips(7200U) || !table.set_style_id("TableGrid") ||
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
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "insert_table_around_existing.docx";

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
    if (!paragraph.set_text("Table insert-before/after sample")) {
        print_document_error(seed, "set_text");
        return 1;
    }
    if (!paragraph
             .add_run(" Reopen the document, insert new tables around the "
                      "existing anchor table, and save the updated order.")
             .has_next()) {
        std::cerr << "add_run failed\n";
        return 1;
    }

    if (!require_step(configure_two_column_table(
                          seed.append_table(2, 2), "Start", "Status", "D9EAF7",
                          "Original first table",
                          "This one remains at the top of the stack."),
                      "configure first table")) {
        return 1;
    }

    if (!require_step(configure_two_column_table(
                          seed.append_table(2, 2), "Anchor", "State", "EADCF8",
                          "Original anchor table",
                          "New tables will be inserted around this one."),
                      "configure anchor table")) {
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

    auto anchor_table = doc.tables();
    anchor_table.next();
    if (!anchor_table.has_next()) {
        std::cerr << "missing anchor table\n";
        return 1;
    }

    auto inserted_before = anchor_table.insert_table_before(2, 2);
    if (!require_step(configure_two_column_table(
                          inserted_before, "Inserted before", "Result", "FCE4D6",
                          "New middle table",
                          "Created directly above the original anchor table."),
                      "configure inserted-before table")) {
        return 1;
    }

    auto inserted_after = inserted_before.insert_table_after(2, 2);
    if (!require_step(configure_two_column_table(
                          inserted_after, "Inserted after", "Result", "FFF2CC",
                          "New trailing table",
                          "Created directly below the inserted middle table."),
                      "configure inserted-after table")) {
        return 1;
    }

    auto anchor_body_row = anchor_table.rows();
    anchor_body_row.next();
    if (!anchor_body_row.has_next() ||
        !require_step(set_row_values(
                          anchor_body_row, "Original anchor table",
                          "Still editable after neighboring table insertion."),
                      "update anchor table body row")) {
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
