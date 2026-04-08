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

bool set_two_cell_row_text(featherdoc::TableRow row, std::string_view left,
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

bool configure_anchor_table(featherdoc::Table table, std::string_view prefix) {
    if (!table.has_next()) {
        return false;
    }

    if (!table.set_width_twips(7200U) || !table.set_style_id("TableGrid") ||
        !table.set_layout_mode(featherdoc::table_layout_mode::fixed) ||
        !table.set_alignment(featherdoc::table_alignment::center) ||
        !table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 120U) ||
        !table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 120U)) {
        return false;
    }

    auto header_row = table.rows();
    if (!header_row.has_next() ||
        !header_row.set_height_twips(360U, featherdoc::row_height_rule::exact) ||
        !header_row.set_repeats_header()) {
        return false;
    }

    auto header_cell = header_row.cells();
    if (!header_cell.has_next() ||
        !header_cell.set_fill_color("D9EAF7") ||
        !header_cell.set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center) ||
        !header_cell.set_text(std::string{prefix} + " title")) {
        return false;
    }

    header_cell.next();
    if (!header_cell.has_next() ||
        !header_cell.set_fill_color("D9EAF7") ||
        !header_cell.set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center) ||
        !header_cell.set_text("Status")) {
        return false;
    }

    header_row.next();
    if (!header_row.has_next() ||
        !header_row.set_height_twips(420U, featherdoc::row_height_rule::at_least)) {
        return false;
    }

    auto body_cell = header_row.cells();
    if (!body_cell.has_next() ||
        !body_cell.set_fill_color("FCE4D6") ||
        !body_cell.set_margin_twips(featherdoc::cell_margin_edge::left, 160U) ||
        !body_cell.set_text(std::string{prefix} + " body")) {
        return false;
    }

    body_cell.next();
    if (!body_cell.has_next() ||
        !body_cell.set_fill_color("FFF2CC") ||
        !body_cell.set_margin_twips(featherdoc::cell_margin_edge::right, 160U) ||
        !body_cell.set_text("Seed table keeps the formatting blueprint.")) {
        return false;
    }

    return true;
}

bool fill_cloned_table(featherdoc::Table table, std::string_view header_left,
                       std::string_view header_right, std::string_view body_left,
                       std::string_view body_right) {
    if (!table.has_next()) {
        return false;
    }

    auto row = table.rows();
    if (!row.has_next() || !set_two_cell_row_text(row, header_left, header_right)) {
        return false;
    }

    row.next();
    if (!row.has_next()) {
        return false;
    }

    return set_two_cell_row_text(row, body_left, body_right);
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "insert_table_like_existing.docx";

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
    if (!paragraph.set_text("Table insert-like sample")) {
        print_document_error(seed, "set_text");
        return 1;
    }
    if (!paragraph
             .add_run(" Reopen the document, clone the current table style, "
                      "fill the new tables, and keep the original anchor in place.")
             .has_next()) {
        std::cerr << "add_run failed\n";
        return 1;
    }

    if (!require_step(configure_anchor_table(seed.append_table(2, 2), "Anchor"),
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
    if (!anchor_table.has_next()) {
        std::cerr << "missing anchor table\n";
        return 1;
    }

    auto inserted_before = anchor_table.insert_table_like_before();
    if (!require_step(fill_cloned_table(inserted_before, "Cloned above", "Status",
                                        "Prepared copy",
                                        "Copied the table structure and formatting."),
                      "fill inserted-before table")) {
        return 1;
    }

    anchor_table.next();
    if (!anchor_table.has_next()) {
        std::cerr << "failed to revisit original anchor table\n";
        return 1;
    }

    auto anchor_body_row = anchor_table.rows();
    anchor_body_row.next();
    if (!anchor_body_row.has_next() ||
        !require_step(
            set_two_cell_row_text(anchor_body_row, "Anchor body",
                                  "Original table remains editable after cloning."),
            "update anchor body row")) {
        return 1;
    }

    auto inserted_after = anchor_table.insert_table_like_after();
    if (!require_step(fill_cloned_table(inserted_after, "Cloned below", "Status",
                                        "Second copy",
                                        "Inserted after the original anchor table."),
                      "fill inserted-after table")) {
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
