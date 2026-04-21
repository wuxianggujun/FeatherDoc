#include <array>
#include <featherdoc.hpp>

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

auto add_text(featherdoc::Paragraph paragraph, std::string_view text,
              featherdoc::formatting_flag formatting =
                  featherdoc::formatting_flag::none) -> bool {
    return paragraph.add_run(std::string{text}, formatting).has_next();
}

auto add_cell_text(featherdoc::TableCell cell, std::string_view text,
                   featherdoc::formatting_flag formatting =
                       featherdoc::formatting_flag::none) -> bool {
    return add_text(cell.paragraphs(), text, formatting);
}

auto append_body_paragraph(
    featherdoc::Document &doc, std::string_view text,
    featherdoc::formatting_flag formatting = featherdoc::formatting_flag::none)
    -> featherdoc::Paragraph {
    auto paragraph = doc.paragraphs();
    while (paragraph.has_next()) {
        paragraph.next();
    }

    return paragraph.insert_paragraph_after(std::string{text}, formatting);
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

auto configure_table(featherdoc::Table table) -> bool {
    return table.set_width_twips(7800U) &&
           table.set_layout_mode(featherdoc::table_layout_mode::fixed) &&
           table.set_alignment(featherdoc::table_alignment::center) &&
           table.set_indent_twips(120U) && table.set_style_id("TableGrid") &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::top, 72U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 96U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::bottom, 72U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 96U) &&
           set_table_column_widths(table,
                                   std::array<std::uint32_t, 2>{2200U, 5600U});
}

auto fill_row(featherdoc::TableRow row, std::string_view fill_color,
              std::string_view left_text, std::string_view right_text,
              bool bold_left = false, bool bold_right = false) -> bool {
    auto cell = row.cells();
    if (!cell.has_next() || !cell.set_fill_color(std::string{fill_color}) ||
        !add_cell_text(cell, left_text,
                       bold_left ? featherdoc::formatting_flag::bold
                                 : featherdoc::formatting_flag::none)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color(std::string{fill_color}) ||
        !add_cell_text(cell, right_text,
                       bold_right ? featherdoc::formatting_flag::bold
                                  : featherdoc::formatting_flag::none)) {
        return false;
    }

    return true;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "table_row_visual.docx";

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
    if (!paragraph.set_text("Table row CLI visual baseline") ||
        !paragraph
             .add_run(" Compare append, insert-before, insert-after, and remove on the same "
                      "body table.")
             .has_next()) {
        print_document_error(doc, "write title");
        return 1;
    }

    auto table = doc.append_table(3U, 2U);
    if (!table.has_next() || !require_step(configure_table(table), "configure table")) {
        return 1;
    }

    auto row = table.rows();
    if (!row.has_next() ||
        !require_step(row.set_height_twips(520U, featherdoc::row_height_rule::exact),
                      "set header row height") ||
        !require_step(fill_row(row, "D9EAF7", "ROW ROLE", "VISUAL CUE", true, true),
                      "fill header row")) {
        return 1;
    }

    row.next();
    if (!row.has_next() ||
        !require_step(row.set_height_twips(1120U, featherdoc::row_height_rule::at_least),
                      "set alpha row height") ||
        !require_step(row.set_cant_split(), "set alpha row cant_split") ||
        !require_step(
            fill_row(row, "FFF2CC", "ALPHA BLOCK",
                     "Insert-before should create a blank amber row above this content.",
                     true, false),
            "fill alpha row")) {
        return 1;
    }

    row.next();
    if (!row.has_next() ||
        !require_step(row.set_height_twips(1120U, featherdoc::row_height_rule::at_least),
                      "set omega row height") ||
        !require_step(
            fill_row(row, "E2F0D9", "OMEGA BLOCK",
                     "Append should create a blank green row below this content.", true,
                     false),
            "fill omega row")) {
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Trailing note: the paragraph below the table should stay fixed while the table "
        "grows, shifts, or loses its header row.");
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
