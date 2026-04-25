#include <array>
#include <featherdoc.hpp>

#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

namespace {

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
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::top, 90U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 120U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::bottom, 90U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 120U) &&
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

auto fill_body_row(featherdoc::TableRow row, sample_mode mode,
                   std::uint32_t row_index) -> bool {
    auto cell = row.cells();
    if (!cell.has_next()) {
        return false;
    }

    const auto row_label = "ROW " + std::to_string(row_index);
    if (row_index % 2U == 0U && !cell.set_fill_color("F7F7F7")) {
        return false;
    }
    if (!cell.paragraphs().set_text(row_label)) {
        return false;
    }

    cell.next();
    if (!cell.has_next()) {
        return false;
    }
    if (row_index % 2U == 1U && !cell.set_fill_color("FCFCFC")) {
        return false;
    }

    const std::string mode_text =
        mode == sample_mode::set_case ? "Baseline page 2 starts with body rows."
                                      : "Baseline page 2 repeats the yellow header.";
    if (!cell.paragraphs().set_text(mode_text)) {
        return false;
    }

    cell.next();
    if (!cell.has_next()) {
        return false;
    }

    const std::string detail_text =
        "Continuation detail for row " + std::to_string(row_index) +
        ". Use page 2 for visual review.";
    return cell.paragraphs().set_text(detail_text);
}

auto seed_repeat_header_document(featherdoc::Document &doc, sample_mode mode) -> bool {
    auto paragraph = doc.paragraphs();
    const auto title_text =
        mode == sample_mode::set_case
            ? std::string_view{"Body table row repeat-header CLI visual baseline (set case)"}
            : std::string_view{"Body table row repeat-header CLI visual baseline (clear case)"};
    const auto subtitle_text =
        mode == sample_mode::set_case
            ? std::string_view{
                  " Page 2 should not repeat the blue header until the CLI mutation runs."}
            : std::string_view{
                  " Page 2 should repeat the yellow header before the CLI mutation runs."};
    if (!paragraph.set_text(std::string{title_text}) ||
        !paragraph.add_run(std::string{subtitle_text}).has_next()) {
        return false;
    }

    const auto intro_text =
        mode == sample_mode::set_case
            ? std::string_view{
                  "Target table: set-table-row-repeat-header should make the first blue row reappear at the top of page 2."}
            : std::string_view{
                  "Target table: clear-table-row-repeat-header should stop the first yellow row from reappearing at the top of page 2."};
    paragraph = append_body_paragraph(doc, intro_text);
    if (!paragraph.has_next()) {
        return false;
    }

    auto table = doc.append_table(1U, 3U);
    if (!table.has_next() ||
        !configure_table(table, std::array<std::uint32_t, 3>{1500U, 3000U, 3900U})) {
        return false;
    }

    auto header = table.rows();
    if (!header.has_next()) {
        return false;
    }
    if (!header.set_height_twips(780U, featherdoc::row_height_rule::exact)) {
        return false;
    }
    if (mode == sample_mode::clear_case && !header.set_repeats_header()) {
        return false;
    }

    auto cell = header.cells();
    const auto header_fill =
        mode == sample_mode::set_case ? std::string_view{"DDEBF7"}
                                      : std::string_view{"FFF2CC"};
    const auto header_label =
        mode == sample_mode::set_case ? std::string_view{"SET HEADER"}
                                      : std::string_view{"CLEAR HEADER"};
    const auto header_cue =
        mode == sample_mode::set_case
            ? std::string_view{"PAGE 2 GAINS THIS BAND"}
            : std::string_view{"PAGE 2 LOSES THIS BAND"};

    if (!cell.has_next() || !cell.set_fill_color(std::string{header_fill}) ||
        !cell.paragraphs().set_text(std::string{header_label})) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color(std::string{header_fill}) ||
        !cell.paragraphs().set_text(std::string{header_cue})) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color(std::string{header_fill}) ||
        !cell.paragraphs().set_text("ROW 0 TARGET")) {
        return false;
    }

    for (std::uint32_t row_index = 1; row_index <= 18U; ++row_index) {
        auto row = table.append_row(3U);
        if (!row.has_next() ||
            !row.set_height_twips(780U, featherdoc::row_height_rule::exact) ||
            !fill_body_row(row, mode, row_index)) {
            return false;
        }
    }

    paragraph = append_body_paragraph(
        doc,
        "Trailing note: the table content stays fixed; only the header-repeat behavior on page 2 should change.");
    return paragraph.has_next();
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "table_row_repeat_header_visual.docx";
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

    if (!seed_repeat_header_document(doc, mode)) {
        print_document_error(doc, "seed_repeat_header_document");
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
