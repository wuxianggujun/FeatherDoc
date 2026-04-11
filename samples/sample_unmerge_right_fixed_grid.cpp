#include <featherdoc.hpp>

#include <array>
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

bool require_table_column_width(featherdoc::Table table, std::size_t column_index,
                                std::uint32_t expected_width,
                                std::string_view operation) {
    const auto width = table.column_width_twips(column_index);
    if (width.has_value() && *width == expected_width) {
        return true;
    }

    std::cerr << operation << " failed";
    if (!width.has_value()) {
        std::cerr << ": width is missing";
    } else {
        std::cerr << ": expected " << expected_width << ", got " << *width;
    }
    std::cerr << '\n';
    return false;
}

bool require_cell_width(featherdoc::TableCell cell, std::uint32_t expected_width,
                        std::string_view operation) {
    const auto width = cell.width_twips();
    if (width.has_value() && *width == expected_width) {
        return true;
    }

    std::cerr << operation << " failed";
    if (!width.has_value()) {
        std::cerr << ": width is missing";
    } else {
        std::cerr << ": expected " << expected_width << ", got " << *width;
    }
    std::cerr << '\n';
    return false;
}

bool require_column_span(featherdoc::TableCell cell, std::size_t expected_span,
                         std::string_view operation) {
    const auto span = cell.column_span();
    if (span == expected_span) {
        return true;
    }

    std::cerr << operation << " failed: expected " << expected_span << ", got " << span
              << '\n';
    return false;
}

bool require_empty_text(featherdoc::TableCell cell, std::string_view operation) {
    if (cell.get_text().empty()) {
        return true;
    }

    std::cerr << operation << " failed: expected empty text\n";
    return false;
}

bool configure_seed_table(featherdoc::Table table) {
    return table.has_next() && table.set_width_twips(7000U) &&
           table.set_layout_mode(featherdoc::table_layout_mode::fixed) &&
           table.set_alignment(featherdoc::table_alignment::center) &&
           table.set_style_id("TableGrid") && table.set_column_width_twips(0U, 1000U) &&
           table.set_column_width_twips(1U, 1900U) &&
           table.set_column_width_twips(2U, 4100U);
}

bool set_row_values(featherdoc::TableRow row,
                    const std::array<std::string_view, 3> &texts,
                    const std::array<std::string_view, 3> *fills = nullptr) {
    if (!row.has_next() ||
        !row.set_height_twips(880U, featherdoc::row_height_rule::at_least)) {
        return false;
    }

    auto cell = row.cells();
    for (std::size_t index = 0; index < texts.size(); ++index) {
        if (!cell.has_next() ||
            !cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center)) {
            return false;
        }
        if (fills != nullptr && !cell.set_fill_color(std::string{(*fills)[index]})) {
            return false;
        }
        if (!cell.set_text(std::string{texts[index]})) {
            return false;
        }
        cell.next();
    }

    return true;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1]) : fs::current_path() / "unmerge_right_fixed_grid.docx";

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
    if (!paragraph.has_next() ||
        !paragraph.set_text("Fixed-grid unmerge_right sample")) {
        std::cerr << "failed to seed intro paragraph\n";
        return 1;
    }
    if (!paragraph
             .add_run(
                 " Seed a fixed-layout table with the first two cells already merged, "
                 "reopen it, unmerge the horizontal span, and verify that the original "
                 "1000/1900/4100 tblGrid widths are restored immediately.")
             .has_next()) {
        std::cerr << "failed to extend intro paragraph\n";
        return 1;
    }

    auto table = seed.append_table(2, 3);
    if (!require_step(configure_seed_table(table), "configure seed table")) {
        return 1;
    }

    const std::array<std::string_view, 3> seed_fills = {
        "D9EAF7",
        "FFF2CC",
        "E2F0D9",
    };
    auto row = table.rows();
    if (!require_step(set_row_values(row, {"Base 1000", "Base 1900", "Tail 4100"},
                                     &seed_fills),
                      "fill seed merged row")) {
        return 1;
    }

    auto merged_anchor = row.cells();
    if (!merged_anchor.has_next() ||
        !require_step(merged_anchor.merge_right(1U), "merge_right in seed") ||
        !require_step(merged_anchor.set_fill_color("D9EAF7"), "recolor merged seed anchor") ||
        !require_step(merged_anchor.set_text("Seed merged = 1000 + 1900"),
                      "set merged seed text")) {
        return 1;
    }

    auto seed_tail = merged_anchor;
    seed_tail.next();
    if (!seed_tail.has_next() ||
        !require_step(seed_tail.set_fill_color("E2F0D9"), "recolor seed tail") ||
        !require_step(seed_tail.set_text("Seed tail = 4100"), "set seed tail text")) {
        return 1;
    }

    row.next();
    if (!require_step(set_row_values(row,
                                     {"1000 twips base", "1900 twips base",
                                      "4100 twips tail remains widest"}),
                      "fill seed detail row")) {
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
             .add_run(
                 " Final visual target: the restored blue and yellow cells should return to "
                 "distinct 1000-twip and 1900-twip widths, while the green 4100-twip tail "
                 "column remains visibly widest.")
             .has_next()) {
        std::cerr << "failed to update intro paragraph after reopen\n";
        return 1;
    }

    table = doc.tables();
    if (!table.has_next() ||
        !require_table_column_width(table, 0U, 1000U, "verify reopened first grid width") ||
        !require_table_column_width(table, 1U, 1900U, "verify reopened second grid width") ||
        !require_table_column_width(table, 2U, 4100U, "verify reopened third grid width")) {
        return 1;
    }

    auto restored_anchor = table.rows().cells();
    if (!restored_anchor.has_next() ||
        !require_cell_width(restored_anchor, 2900U, "verify merged width before unmerge") ||
        !require_column_span(restored_anchor, 2U, "verify merged span before unmerge") ||
        !require_step(restored_anchor.unmerge_right(), "unmerge_right after reopen") ||
        !require_cell_width(restored_anchor, 1000U, "verify restored first width") ||
        !require_column_span(restored_anchor, 1U, "verify restored first span") ||
        !require_step(restored_anchor.set_fill_color("D9EAF7"), "recolor restored first cell") ||
        !require_step(restored_anchor.set_text("Restored first cell = 1000"),
                      "set restored first text")) {
        return 1;
    }

    auto restored_second = restored_anchor;
    restored_second.next();
    if (!restored_second.has_next() ||
        !require_cell_width(restored_second, 1900U, "verify restored second width") ||
        !require_column_span(restored_second, 1U, "verify restored second span") ||
        !require_empty_text(restored_second, "verify restored second text cleared") ||
        !require_step(restored_second.set_fill_color("FFF2CC"), "recolor restored second cell") ||
        !require_step(restored_second.set_text("Restored second cell = 1900"),
                      "set restored second text")) {
        return 1;
    }

    auto restored_tail = restored_second;
    restored_tail.next();
    if (!restored_tail.has_next() ||
        !require_cell_width(restored_tail, 4100U, "verify restored tail width") ||
        !require_step(restored_tail.set_fill_color("E2F0D9"), "recolor restored tail") ||
        !require_step(restored_tail.set_text("Tail grid column = 4100"),
                      "set restored tail text")) {
        return 1;
    }

    auto detail_row = table.rows();
    detail_row.next();
    if (!detail_row.has_next()) {
        std::cerr << "missing detail row after reopen\n";
        return 1;
    }

    auto detail_cell = detail_row.cells();
    if (!detail_cell.has_next() ||
        !require_cell_width(detail_cell, 1000U, "verify detail first width")) {
        return 1;
    }
    detail_cell.next();
    if (!detail_cell.has_next() ||
        !require_cell_width(detail_cell, 1900U, "verify detail second width")) {
        return 1;
    }
    detail_cell.next();
    if (!detail_cell.has_next() ||
        !require_cell_width(detail_cell, 4100U, "verify detail third width")) {
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
