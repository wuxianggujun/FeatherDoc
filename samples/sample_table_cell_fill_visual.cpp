#include <array>
#include <featherdoc.hpp>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
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
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::top, 96U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 120U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::bottom, 96U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 120U) &&
           set_table_column_widths(table, widths);
}

auto set_visual_cell(featherdoc::TableCell cell,
                     std::optional<std::string_view> fill_color,
                     std::string_view text) -> bool {
    if (fill_color.has_value() &&
        !cell.set_fill_color(std::string{*fill_color})) {
        return false;
    }

    return cell.set_text(std::string{text});
}

auto fill_two_cell_row(featherdoc::TableRow row, std::uint32_t height_twips,
                       std::optional<std::string_view> left_fill,
                       std::string_view left_text,
                       std::optional<std::string_view> right_fill,
                       std::string_view right_text) -> bool {
    if (!row.set_height_twips(height_twips,
                              featherdoc::row_height_rule::at_least)) {
        return false;
    }

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

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "table_cell_fill_visual.docx";

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
    if (!paragraph.set_text("Body table cell fill CLI visual baseline") ||
        !paragraph
             .add_run(" Compare set-table-cell-fill and clear-table-cell-fill "
                      "on one page.")
             .has_next()) {
        print_document_error(doc, "write title");
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Table 0: set-table-cell-fill should color only the clear bottom-right target cell.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append table 0 intro paragraph\n";
        return 1;
    }

    auto set_fill_table = doc.append_table(2U, 2U);
    if (!set_fill_table.has_next() ||
        !require_step(configure_table(
                          set_fill_table,
                          std::array<std::uint32_t, 2>{2500U, 4100U}),
                      "configure set-fill table")) {
        return 1;
    }

    auto row = set_fill_table.rows();
    if (!row.has_next() ||
        !require_step(fill_two_cell_row(row, 560U, "D9EAF7", "CASE",
                                        "D9EAF7", "EXPECTED MUTATION"),
                      "fill set-fill header row")) {
        return 1;
    }

    row.next();
    if (!row.has_next() ||
        !require_step(fill_two_cell_row(row, 1220U, "FFF2CC",
                                        "set-table-cell-fill", std::nullopt,
                                        "TARGET STARTS CLEAR"),
                      "fill set-fill target row")) {
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Table 1: clear-table-cell-fill should remove the orange fill from the lower-left target cell.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append table 1 intro paragraph\n";
        return 1;
    }

    auto clear_fill_table = doc.append_table(2U, 2U);
    if (!clear_fill_table.has_next() ||
        !require_step(configure_table(
                          clear_fill_table,
                          std::array<std::uint32_t, 2>{2500U, 4100U}),
                      "configure clear-fill table")) {
        return 1;
    }

    row = clear_fill_table.rows();
    if (!row.has_next() ||
        !require_step(fill_two_cell_row(row, 560U, "D9EAF7", "TARGET STATUS",
                                        "D9EAF7", "COMMAND"),
                      "fill clear-fill header row")) {
        return 1;
    }

    row.next();
    if (!row.has_next() ||
        !require_step(fill_two_cell_row(row, 1220U, "F4B183",
                                        "TARGET STARTS FILLED", "E2F0D9",
                                        "clear-table-cell-fill"),
                      "fill clear-fill target row")) {
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Trailing note: the paragraph below both target tables should stay fixed while only the selected cell shading changes.");
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
