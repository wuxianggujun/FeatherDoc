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
    std::optional<featherdoc::cell_vertical_alignment> vertical_alignment) -> bool {
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

    if (vertical_alignment.has_value() &&
        !cell.set_vertical_alignment(*vertical_alignment)) {
        return false;
    }

    return add_cell_lines(cell, lines);
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "table_cell_vertical_alignment_visual.docx";

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
    if (!paragraph.set_text("Body table cell vertical-alignment CLI visual baseline") ||
        !paragraph
             .add_run(
                 " Compare set-table-cell-vertical-alignment and clear-table-cell-vertical-alignment on one page.")
             .has_next()) {
        print_document_error(doc, "write title");
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Table 0: set-table-cell-vertical-alignment should move the target cell text from the default top position down to the bottom.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append table 0 intro paragraph\n";
        return 1;
    }

    auto set_alignment_table = doc.append_table(1U, 1U);
    if (!set_alignment_table.has_next() ||
        !require_step(configure_table(set_alignment_table,
                                      std::array<std::uint32_t, 1>{2600U}),
                      "configure set-alignment table") ||
        !require_step(
            seed_single_cell_table(
                set_alignment_table, 4200U, "DDEBF7",
                std::array<std::string_view, 1>{"SET TO BOTTOM"},
                std::nullopt),
            "seed set-alignment table")) {
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Table 1: clear-table-cell-vertical-alignment should remove the pre-seeded bottom alignment and restore the target cell to the default top position.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append table 1 intro paragraph\n";
        return 1;
    }

    auto clear_alignment_table = doc.append_table(1U, 1U);
    if (!clear_alignment_table.has_next() ||
        !require_step(configure_table(clear_alignment_table,
                                      std::array<std::uint32_t, 1>{2600U}),
                      "configure clear-alignment table") ||
        !require_step(
            seed_single_cell_table(
                clear_alignment_table, 4200U, "FFF2CC",
                std::array<std::string_view, 1>{"CLEAR TO TOP"},
                featherdoc::cell_vertical_alignment::bottom),
            "seed clear-alignment table")) {
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Trailing note: the paragraph below both target tables should stay fixed while only the selected cell vertical position changes.");
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
