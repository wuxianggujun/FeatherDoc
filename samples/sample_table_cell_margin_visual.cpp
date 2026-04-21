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
auto seed_single_cell_table(featherdoc::Table table, std::uint32_t row_height_twips,
                            std::string_view fill_color,
                            const std::array<std::string_view, N> &lines,
                            std::optional<std::uint32_t> left_margin_twips) -> bool {
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

    if (left_margin_twips.has_value()) {
        if (!cell.set_margin_twips(featherdoc::cell_margin_edge::left,
                                   *left_margin_twips)) {
            return false;
        }
    } else {
        if (!cell.clear_margin(featherdoc::cell_margin_edge::left)) {
            return false;
        }
    }

    return add_cell_lines(cell, lines);
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "table_cell_margin_visual.docx";

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
    if (!paragraph.set_text("Body table cell margin CLI visual baseline") ||
        !paragraph
             .add_run(
                 " Compare set-table-cell-margin and clear-table-cell-margin on one page.")
             .has_next()) {
        print_document_error(doc, "write title");
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Table 0: set-table-cell-margin should add a visible left inset while keeping the cell width and row height fixed.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append table 0 intro paragraph\n";
        return 1;
    }

    auto set_margin_table = doc.append_table(1U, 1U);
    if (!set_margin_table.has_next() ||
        !require_step(configure_table(set_margin_table,
                                      std::array<std::uint32_t, 1>{3200U}),
                      "configure set-margin table") ||
        !require_step(
            seed_single_cell_table(
                set_margin_table, 2200U, "DDEBF7",
                std::array<std::string_view, 2>{"LEFT EDGE", "SHIFT ME"},
                std::nullopt),
            "seed set-margin table")) {
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Table 1: clear-table-cell-margin should remove the pre-seeded left inset and bring the text back toward the left border.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append table 1 intro paragraph\n";
        return 1;
    }

    auto clear_margin_table = doc.append_table(1U, 1U);
    if (!clear_margin_table.has_next() ||
        !require_step(configure_table(clear_margin_table,
                                      std::array<std::uint32_t, 1>{3200U}),
                      "configure clear-margin table") ||
        !require_step(
            seed_single_cell_table(
                clear_margin_table, 2200U, "FFF2CC",
                std::array<std::string_view, 2>{"CLEAR LEFT", "BACK TO EDGE"},
                720U),
            "seed clear-margin table")) {
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Trailing note: the paragraph below both target tables should stay fixed while only the selected cell left margin changes.");
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
