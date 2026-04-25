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
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::top, 120U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 144U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::bottom, 120U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 144U) &&
           set_table_column_widths(table, widths);
}

template <std::size_t N>
auto add_cell_lines(featherdoc::TableCell cell,
                    const std::array<std::string_view, N> &lines) -> bool {
    if constexpr (N == 0U) {
        return true;
    }

    auto paragraph = cell.paragraphs();
    if (!paragraph.set_text(std::string{lines[0]})) {
        return false;
    }

    for (std::size_t index = 1; index < lines.size(); ++index) {
        auto inserted = paragraph.insert_paragraph_after(std::string{lines[index]});
        if (!inserted.has_next()) {
            return false;
        }
        paragraph = inserted;
    }

    return true;
}

template <std::size_t N>
auto seed_single_cell_table(
    featherdoc::Table table, std::uint32_t row_height_twips,
    std::string_view fill_color, const std::array<std::string_view, N> &lines,
    std::optional<featherdoc::cell_text_direction> text_direction) -> bool {
    auto row = table.rows();
    if (!row.has_next() ||
        !row.set_height_twips(row_height_twips,
                              featherdoc::row_height_rule::at_least)) {
        return false;
    }

    auto cell = row.cells();
    if (!cell.has_next() || !cell.set_fill_color(std::string{fill_color})) {
        return false;
    }

    if (text_direction.has_value() &&
        !cell.set_text_direction(*text_direction)) {
        return false;
    }

    return add_cell_lines(cell, lines);
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "table_cell_text_direction_visual.docx";

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
    if (!paragraph.set_text("Body table cell text-direction CLI visual baseline") ||
        !paragraph
             .add_run(
                 " Compare set-table-cell-text-direction and clear-table-cell-text-direction on one page.")
             .has_next()) {
        print_document_error(doc, "write title");
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Table 0: set-table-cell-text-direction should turn the target cell into top-to-bottom right-to-left text.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append table 0 intro paragraph\n";
        return 1;
    }

    auto set_direction_table = doc.append_table(1U, 1U);
    if (!set_direction_table.has_next() ||
        !require_step(configure_table(set_direction_table,
                                      std::array<std::uint32_t, 1>{2100U}),
                      "configure set-direction table") ||
        !require_step(
            seed_single_cell_table(
                set_direction_table, 3000U, "DDEBF7",
                std::array<std::string_view, 2>{"SET TARGET", "TBRL 2026"},
                std::nullopt),
            "seed set-direction table")) {
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Table 1: clear-table-cell-text-direction should restore the pre-seeded vertical cell back to normal horizontal flow.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append table 1 intro paragraph\n";
        return 1;
    }

    auto clear_direction_table = doc.append_table(1U, 1U);
    if (!clear_direction_table.has_next() ||
        !require_step(configure_table(clear_direction_table,
                                      std::array<std::uint32_t, 1>{2100U}),
                      "configure clear-direction table") ||
        !require_step(
            seed_single_cell_table(
                clear_direction_table, 3000U, "FFF2CC",
                std::array<std::string_view, 2>{"CLEAR TARGET", "BACK TO LRTB"},
                featherdoc::cell_text_direction::top_to_bottom_right_to_left),
            "seed clear-direction table")) {
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Trailing note: the paragraph below both target tables should stay fixed while only the selected cell text direction changes.");
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
