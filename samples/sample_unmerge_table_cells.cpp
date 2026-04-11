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

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1]) : fs::current_path() / "unmerge_table_cells.docx";

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
    if (!paragraph.set_text("Table cell unmerge sample")) {
        print_document_error(seed, "set_text");
        return 1;
    }
    if (!paragraph
             .add_run(
                 " Reopen the document, split one horizontal merge and one vertical merge, "
                 "then continue editing the restored cells.")
             .has_next()) {
        std::cerr << "add_run failed\n";
        return 1;
    }

    auto table = seed.append_table(4, 3);
    if (!table.has_next() || !table.set_width_twips(9000U) ||
        !table.set_style_id("TableGrid")) {
        std::cerr << "failed to configure seed table\n";
        return 1;
    }

    auto row = table.rows();
    if (!row.has_next()) {
        std::cerr << "missing header row\n";
        return 1;
    }

    auto cell = row.cells();
    if (!require_step(cell.set_fill_color("D9EAF7"), "set header fill 1") ||
        !require_step(cell.set_text("Case"), "set header text 1")) {
        return 1;
    }
    cell.next();
    if (!require_step(cell.set_fill_color("D9EAF7"), "set header fill 2") ||
        !require_step(cell.set_text("Expected result"), "set header text 2")) {
        return 1;
    }
    cell.next();
    if (!require_step(cell.set_fill_color("D9EAF7"), "set header fill 3") ||
        !require_step(cell.set_text("Notes"), "set header text 3")) {
        return 1;
    }

    row.next();
    if (!row.has_next()) {
        std::cerr << "missing horizontal merge row\n";
        return 1;
    }
    cell = row.cells();
    if (!require_step(cell.set_fill_color("DDEBF7"), "set horizontal merge fill") ||
        !require_step(cell.set_text("Horizontal merge anchor"), "set horizontal merge text") ||
        !require_step(cell.merge_right(1U), "merge right in seed")) {
        return 1;
    }
    auto tail_cell = cell;
    tail_cell.next();
    if (!tail_cell.has_next() ||
        !require_step(tail_cell.set_fill_color("E2F0D9"), "set horizontal tail fill") ||
        !require_step(tail_cell.set_text("Tail stays on the right"), "set horizontal tail text")) {
        return 1;
    }

    row.next();
    if (!row.has_next()) {
        std::cerr << "missing vertical merge anchor row\n";
        return 1;
    }
    cell = row.cells();
    if (!require_step(cell.set_fill_color("FFF2CC"), "set vertical merge fill") ||
        !require_step(cell.set_text("Vertical merge anchor"), "set vertical merge text") ||
        !require_step(cell.merge_down(1U), "merge down in seed")) {
        return 1;
    }
    cell.next();
    if (!cell.has_next() || !require_step(cell.set_text("Top row stays visible"),
                                          "set vertical top detail")) {
        return 1;
    }
    cell.next();
    if (!cell.has_next() ||
        !require_step(cell.set_text("Restore the lower-left cell after reopen"),
                      "set vertical top note")) {
        return 1;
    }

    row.next();
    if (!row.has_next()) {
        std::cerr << "missing vertical merge continuation row\n";
        return 1;
    }
    cell = row.cells();
    cell.next();
    if (!cell.has_next() ||
        !require_step(cell.set_text("Bottom row keeps its middle content"),
                      "set vertical bottom detail")) {
        return 1;
    }
    cell.next();
    if (!cell.has_next() ||
        !require_step(cell.set_text("The left cell should become editable again"),
                      "set vertical bottom note")) {
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
             .add_run(" Final review target: the orange and green cells should appear as "
                      "restored standalone cells with clean borders.")
             .has_next()) {
        std::cerr << "failed to update intro paragraph\n";
        return 1;
    }

    table = doc.tables();
    if (!table.has_next()) {
        std::cerr << "missing target table after reopen\n";
        return 1;
    }

    auto horizontal_row = table.rows();
    horizontal_row.next();
    if (!horizontal_row.has_next()) {
        std::cerr << "missing horizontal row after reopen\n";
        return 1;
    }

    auto horizontal_cell = horizontal_row.cells();
    if (!horizontal_cell.has_next() ||
        !require_step(horizontal_cell.unmerge_right(), "unmerge_right after reopen")) {
        return 1;
    }

    auto restored_horizontal = horizontal_row.cells();
    restored_horizontal.next();
    if (!restored_horizontal.has_next() ||
        !require_step(restored_horizontal.set_fill_color("FCE4D6"),
                      "set restored horizontal fill") ||
        !require_step(restored_horizontal.set_text("Restored right cell"),
                      "set restored horizontal text")) {
        return 1;
    }

    auto vertical_row = table.rows();
    vertical_row.next();
    vertical_row.next();
    vertical_row.next();
    if (!vertical_row.has_next()) {
        std::cerr << "missing vertical continuation row after reopen\n";
        return 1;
    }

    auto restored_vertical = vertical_row.cells();
    if (!restored_vertical.has_next() ||
        !require_step(restored_vertical.unmerge_down(), "unmerge_down from continuation") ||
        !require_step(restored_vertical.set_fill_color("E2F0D9"),
                      "set restored vertical fill") ||
        !require_step(restored_vertical.set_text("Restored lower cell"),
                      "set restored vertical text")) {
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
