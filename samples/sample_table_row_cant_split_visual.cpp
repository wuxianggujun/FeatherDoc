#include <array>
#include <featherdoc.hpp>

#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr std::size_t kTargetRowIndex = 20U;

enum class sample_mode {
    set_case,
    clear_case,
};

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

auto add_cell_lines(featherdoc::TableCell cell,
                    const std::vector<std::string> &lines) -> bool {
    if (lines.empty()) {
        return true;
    }

    auto paragraph = cell.paragraphs();
    if (!add_text(paragraph, lines.front())) {
        return false;
    }

    for (std::size_t index = 1; index < lines.size(); ++index) {
        paragraph = paragraph.insert_paragraph_after(lines[index]);
        if (!paragraph.has_next()) {
            return false;
        }
    }

    return true;
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
auto set_row_widths(featherdoc::TableRow row,
                    const std::array<std::uint32_t, N> &widths) -> bool {
    auto cell = row.cells();
    for (std::size_t index = 0; index < widths.size(); ++index) {
        if (!cell.has_next() || !cell.set_width_twips(widths[index])) {
            return false;
        }
        if (index + 1U < widths.size()) {
            cell.next();
        }
    }
    return true;
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

auto configure_table(featherdoc::Table table, std::uint32_t width_twips) -> bool {
    return table.set_width_twips(width_twips) &&
           table.set_layout_mode(featherdoc::table_layout_mode::fixed) &&
           table.set_alignment(featherdoc::table_alignment::center) &&
           table.set_indent_twips(120U) && table.set_style_id("TableGrid") &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::top, 72U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 96U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::bottom, 72U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 96U) &&
           table.set_border(featherdoc::table_border_edge::top,
                            {featherdoc::border_style::single, 12, "1F1F1F", 0}) &&
           table.set_border(featherdoc::table_border_edge::left,
                            {featherdoc::border_style::single, 8, "808080", 0}) &&
           table.set_border(featherdoc::table_border_edge::bottom,
                            {featherdoc::border_style::single, 12, "1F1F1F", 0}) &&
           table.set_border(featherdoc::table_border_edge::right,
                            {featherdoc::border_style::single, 8, "808080", 0}) &&
           table.set_border(featherdoc::table_border_edge::inside_horizontal,
                            {featherdoc::border_style::single, 6, "B7B7B7", 0}) &&
           table.set_border(featherdoc::table_border_edge::inside_vertical,
                            {featherdoc::border_style::single, 6, "B7B7B7", 0});
}

auto parse_mode(int argc, char **argv) -> sample_mode {
    if (argc <= 2) {
        return sample_mode::set_case;
    }

    const std::string_view mode = argv[2];
    if (mode == "set") {
        return sample_mode::set_case;
    }
    if (mode == "clear") {
        return sample_mode::clear_case;
    }

    std::cerr << "unsupported mode: " << mode << '\n';
    std::exit(1);
}

auto create_multipage_target_table(featherdoc::Document &doc,
                                   bool seed_cant_split) -> bool {
    auto table = doc.append_table(1U, 4U);
    if (!require_step(configure_table(table, 9800U),
                      "multipage target table configure")) {
        return false;
    }
    if (!require_step(set_table_column_widths(
                          table, std::array<std::uint32_t, 4>{900U, 2500U, 3600U, 2800U}),
                      "multipage target column widths")) {
        return false;
    }

    auto header = table.rows();
    if (!require_step(header.has_next(), "multipage header exists") ||
        !require_step(header.set_height_twips(480U, featherdoc::row_height_rule::exact),
                      "multipage header height") ||
        !require_step(header.set_repeats_header(), "multipage header repeat") ||
        !require_step(header.set_cant_split(), "multipage header cantSplit") ||
        !require_step(set_row_widths(
                          header, std::array<std::uint32_t, 4>{900U, 2500U, 3600U, 2800U}),
                      "multipage header widths")) {
        return false;
    }

    auto cell = header.cells();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Case", featherdoc::formatting_flag::bold)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Expected rendering",
                       featherdoc::formatting_flag::bold)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Stress content",
                       featherdoc::formatting_flag::bold)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Notes", featherdoc::formatting_flag::bold)) {
        return false;
    }

    for (std::size_t row_index = 1; row_index <= 24U; ++row_index) {
        auto row = table.append_row(4U);
        if (!require_step(row.has_next(), "multipage row exists") ||
            !require_step(set_row_widths(
                              row, std::array<std::uint32_t, 4>{900U, 2500U, 3600U, 2800U}),
                          "multipage row widths")) {
            return false;
        }

        if (row_index == kTargetRowIndex && seed_cant_split &&
            !require_step(row.set_cant_split(),
                          "multipage highlighted row cantSplit")) {
            return false;
        }

        cell = row.cells();
        if (!cell.has_next()) {
            return false;
        }

        const auto case_label = std::string("R") + std::to_string(row_index);
        if (row_index == kTargetRowIndex && !cell.set_fill_color("FCE4D6")) {
            return false;
        }
        if (!add_cell_text(cell, case_label, featherdoc::formatting_flag::bold)) {
            return false;
        }

        cell.next();
        if (!cell.has_next()) {
            return false;
        }
        if (row_index % 2U == 0U && !cell.set_fill_color("F9F9F9")) {
            return false;
        }
        if (!add_cell_lines(
                cell,
                { "Keep borders continuous",
                  row_index == kTargetRowIndex
                      ? "Highlighted row should move as a whole"
                      : "Header must repeat on later pages" })) {
            return false;
        }

        cell.next();
        if (!cell.has_next()) {
            return false;
        }
        std::vector<std::string> stress_lines;
        if (row_index == kTargetRowIndex) {
            stress_lines = {
                "Large cantSplit row:",
                "line 1 - this block should not break across pages.",
                "line 2 - Word should push the whole row forward when needed.",
                "line 3 - borders should remain intact around the tall content.",
                "line 4 - repeated header should still appear above the row.",
                "line 5 - no clipped fill or missing inner borders.",
                "line 6 - text should remain readable.",
                "line 7 - margin spacing should be preserved.",
                "line 8 - background shading should stay visible.",
                "line 9 - check that the row is fully on one page.",
                "line 10 - this is the row to inspect first.",
                "line 11 - extra height makes the page break behavior obvious.",
                "line 12 - the splittable baseline should continue onto page 3.",
                "line 13 - the locked baseline should jump forward as one unit.",
                "line 14 - keep watching the orange fill and inside borders.",
                "line 15 - page 2 and page 3 should now diverge clearly.",
                "line 16 - this final block is intentionally verbose.",
            };
            if (!cell.set_fill_color("FFF2CC")) {
                return false;
            }
        } else {
            stress_lines = {
                "Baseline content for row " + std::to_string(row_index),
                "Two-line cell keeps the table tall enough to span pages.",
            };
        }
        if (!add_cell_lines(cell, stress_lines)) {
            return false;
        }

        cell.next();
        if (!cell.has_next()) {
            return false;
        }
        if (row_index % 3U == 0U &&
            !cell.set_vertical_alignment(
                featherdoc::cell_vertical_alignment::center)) {
            return false;
        }
        if (!add_cell_lines(
                cell,
                { row_index == kTargetRowIndex
                      ? "Inspect this row in the PNG review."
                      : "Alternating rows help expose spacing drift.",
                  "No unexpected overlap or border gaps." })) {
            return false;
        }
    }

    return true;
}

auto seed_document(featherdoc::Document &doc, sample_mode mode) -> bool {
    auto paragraph = doc.paragraphs();
    const auto title_text =
        mode == sample_mode::set_case
            ? std::string_view{"Body table row cant-split CLI visual baseline (set case)"}
            : std::string_view{"Body table row cant-split CLI visual baseline (clear case)"};
    const auto subtitle_text =
        mode == sample_mode::set_case
            ? std::string_view{
                  " The highlighted orange row starts splittable, and page 1 / page 2 should diverge after the mutation runs."}
            : std::string_view{
                  " The highlighted orange row starts locked together, and page 1 / page 2 should diverge after the mutation runs."};
    if (!paragraph.set_text(std::string{title_text}) ||
        !paragraph.add_run(std::string{subtitle_text}).has_next()) {
        return false;
    }

    const auto intro_text =
        mode == sample_mode::set_case
            ? std::string_view{
                  "Target row: set-table-row-cant-split should push the highlighted row onto page 2 as one intact band instead of letting it split across the page break."}
            : std::string_view{
                  "Target row: clear-table-row-cant-split should let the highlighted row split across the page break instead of forcing the whole row onto page 2."};
    paragraph = append_body_paragraph(doc, intro_text);
    if (!paragraph.has_next()) {
        return false;
    }

    if (!create_multipage_target_table(doc, mode == sample_mode::clear_case)) {
        return false;
    }

    paragraph = append_body_paragraph(
        doc,
        "Trailing note: only the highlighted row's page-break behavior should change; the row text and styling stay fixed.");
    return paragraph.has_next();
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "table_row_cant_split_visual.docx";
    const auto mode = parse_mode(argc, argv);

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

    if (!seed_document(doc, mode)) {
        print_document_error(doc, "seed_document");
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
