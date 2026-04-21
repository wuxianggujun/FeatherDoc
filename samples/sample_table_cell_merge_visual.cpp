#include <array>
#include <featherdoc.hpp>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

namespace {

auto require_step(bool ok, std::string_view step) -> bool {
    if (!ok) {
        std::cerr << "step failed: " << step << '\n';
    }
    return ok;
}

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

auto append_body_paragraph(featherdoc::Document &doc, std::string_view text)
    -> featherdoc::Paragraph {
    auto paragraph = doc.paragraphs();
    while (paragraph.has_next()) {
        paragraph.next();
    }

    return paragraph.insert_paragraph_after(std::string{text});
}

template <std::size_t N>
auto set_table_column_widths(featherdoc::Table table,
                             const std::array<std::uint32_t, N> &widths) -> bool {
    for (std::size_t index = 0; index < widths.size(); ++index) {
        if (!table.set_column_width_twips(index, widths[index])) {
            return false;
        }
    }
    return true;
}

template <std::size_t N>
auto configure_table(featherdoc::Table table,
                     const std::array<std::uint32_t, N> &widths) -> bool {
    std::uint32_t total_width = 0U;
    for (const auto width : widths) {
        total_width += width;
    }

    return table.set_width_twips(total_width) &&
           table.set_layout_mode(featherdoc::table_layout_mode::fixed) &&
           table.set_alignment(featherdoc::table_alignment::center) &&
           table.set_style_id("TableGrid") &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::top, 72U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 96U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::bottom, 72U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 96U) &&
           set_table_column_widths(table, widths);
}

auto set_visual_cell(featherdoc::TableCell cell, std::string_view fill_color,
                     std::string_view text) -> bool {
    return cell.set_fill_color(std::string{fill_color}) &&
           cell.set_text(std::string{text});
}

auto fill_two_cell_row(featherdoc::TableRow row, std::string_view left_fill,
                       std::string_view left_text, std::string_view right_fill,
                       std::string_view right_text) -> bool {
    auto cell = row.cells();
    if (!cell.has_next() ||
        !set_visual_cell(cell, left_fill, left_text)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() ||
        !set_visual_cell(cell, right_fill, right_text)) {
        return false;
    }

    return true;
}

auto fill_three_cell_row(featherdoc::TableRow row, std::string_view first_fill,
                         std::string_view first_text,
                         std::string_view second_fill,
                         std::string_view second_text,
                         std::string_view third_fill,
                         std::string_view third_text) -> bool {
    auto cell = row.cells();
    if (!cell.has_next() ||
        !set_visual_cell(cell, first_fill, first_text)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() ||
        !set_visual_cell(cell, second_fill, second_text)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() ||
        !set_visual_cell(cell, third_fill, third_text)) {
        return false;
    }

    return true;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "table_cell_merge_visual.docx";

    if (output_path.has_parent_path()) {
        std::error_code directory_error;
        fs::create_directories(output_path.parent_path(), directory_error);
        if (directory_error) {
            std::cerr << "failed to create output directory: "
                      << output_path.parent_path().string() << '\n';
            return 1;
        }
    }

    featherdoc::Document doc(output_path);
    if (const auto error = doc.create_empty()) {
        print_document_error(doc, "create_empty");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    auto paragraph = doc.paragraphs();
    if (!paragraph.set_text("Body table cell CLI visual baseline") ||
        !paragraph
             .add_run(" Compare set-table-cell-text, merge-table-cells, and "
                      "unmerge-table-cells on one page.")
             .has_next()) {
        print_document_error(doc, "write title");
        return 1;
    }

    paragraph = append_body_paragraph(
        doc, "Table 0: set-table-cell-text should only rewrite the bottom-right target cell.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append table 0 intro paragraph\n";
        return 1;
    }

    auto set_text_table = doc.append_table(2U, 2U);
    if (!set_text_table.has_next() ||
        !require_step(configure_table(
                          set_text_table,
                          std::array<std::uint32_t, 2>{2400U, 4200U}),
                      "configure set-text table")) {
        return 1;
    }

    auto row = set_text_table.rows();
    if (!row.has_next() ||
        !require_step(fill_two_cell_row(row, "D9EAF7", "AREA", "D9EAF7",
                                        "VISUAL CUE"),
                      "fill set-text header row")) {
        return 1;
    }

    row.next();
    if (!row.has_next() ||
        !require_step(fill_two_cell_row(row, "FFF2CC", "SET-TEXT TARGET",
                                        "FFF2CC", "ORIGINAL BODY TARGET"),
                      "fill set-text target row")) {
        return 1;
    }

    paragraph = append_body_paragraph(
        doc, "Table 1: merge-table-cells should swallow the middle amber cell into the left anchor.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append table 1 intro paragraph\n";
        return 1;
    }

    auto merge_table = doc.append_table(2U, 3U);
    if (!merge_table.has_next() ||
        !require_step(configure_table(
                          merge_table,
                          std::array<std::uint32_t, 3>{2000U, 2000U, 2200U}),
                      "configure merge table")) {
        return 1;
    }

    row = merge_table.rows();
    if (!row.has_next() ||
        !require_step(fill_three_cell_row(row, "FCE4D6", "MERGE ANCHOR",
                                          "FCE4D6", "SOURCE CELL",
                                          "E2F0D9", "TAIL CELL"),
                      "fill merge header row")) {
        return 1;
    }

    row.next();
    if (!row.has_next() ||
        !require_step(fill_three_cell_row(row, "FFF2CC", "ROW 2 LEFT",
                                          "FFF2CC", "ROW 2 MIDDLE",
                                          "FFF2CC", "ROW 2 RIGHT"),
                      "fill merge body row")) {
        return 1;
    }

    paragraph = append_body_paragraph(
        doc, "Table 2: unmerge-table-cells should split the violet banner back into three visible columns.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append table 2 intro paragraph\n";
        return 1;
    }

    auto unmerge_table = doc.append_table(1U, 3U);
    if (!unmerge_table.has_next() ||
        !require_step(configure_table(
                          unmerge_table,
                          std::array<std::uint32_t, 3>{2000U, 2000U, 2200U}),
                      "configure unmerge table")) {
        return 1;
    }

    row = unmerge_table.rows();
    if (!row.has_next() ||
        !require_step(fill_three_cell_row(row, "EADCF8", "UNMERGE ANCHOR",
                                          "EADCF8", "SPLIT CELL",
                                          "DDEBF7", "TAIL CELL"),
                      "fill unmerge row")) {
        return 1;
    }

    auto merged_anchor = row.cells();
    if (!merged_anchor.has_next() ||
        !require_step(merged_anchor.merge_right(1U), "pre-merge unmerge anchor")) {
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Trailing note: the paragraph below all target tables should stay fixed while only the selected table mutates.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append trailing note paragraph\n";
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
