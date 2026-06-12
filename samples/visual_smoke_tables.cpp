#include <array>
#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {
auto utf8_from_u8(std::u8string_view text) -> std::string {
    return {reinterpret_cast<const char *>(text.data()), text.size()};
}

auto require_step(bool ok, std::string_view step) -> bool {
    if (!ok) {
        std::cerr << "step failed: " << step << '\n';
    }
    return ok;
}

void print_document_error(const featherdoc::Document &doc, std::string_view operation) {
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
              featherdoc::formatting_flag formatting = featherdoc::formatting_flag::none)
    -> bool {
    return paragraph.add_run(std::string{text}, formatting).has_next();
}

auto configure_cjk_font_defaults(featherdoc::Document &doc) -> bool {
    return doc.set_default_run_font_family("Segoe UI") &&
           doc.set_default_run_east_asia_font_family("Microsoft YaHei") &&
           doc.set_style_run_font_family("Strong", "Segoe UI") &&
           doc.set_style_run_east_asia_font_family("Strong", "Microsoft YaHei");
}

auto configure_cjk_language_defaults(featherdoc::Document &doc) -> bool {
    return doc.set_default_run_language("en-US") &&
           doc.set_default_run_east_asia_language("zh-CN") &&
           doc.set_default_run_bidi_language("ar-SA") &&
           doc.set_style_run_language("Strong", "en-US") &&
           doc.set_style_run_east_asia_language("Strong", "zh-CN") &&
           doc.set_style_run_bidi_language("Strong", "ar-SA");
}

auto configure_direction_styles(featherdoc::Document &doc) -> bool {
    return doc.set_style_run_rtl("Emphasis") && doc.set_style_paragraph_bidi("Quote");
}

auto add_cell_text(featherdoc::TableCell cell, std::string_view text,
                   featherdoc::formatting_flag formatting = featherdoc::formatting_flag::none)
    -> bool {
    return add_text(cell.paragraphs(), text, formatting);
}

auto add_cell_lines(featherdoc::TableCell cell, const std::vector<std::string> &lines)
    -> bool {
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

auto append_body_paragraph(featherdoc::Document &doc, std::string_view text,
                           featherdoc::formatting_flag formatting =
                               featherdoc::formatting_flag::none) -> featherdoc::Paragraph {
    auto paragraph = doc.paragraphs();
    while (paragraph.has_next()) {
        paragraph.next();
    }

    return paragraph.insert_paragraph_after(std::string{text}, formatting);
}

template <std::size_t N>
auto set_row_widths(featherdoc::TableRow row, const std::array<std::uint32_t, N> &widths)
    -> bool {
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

#include "visual_smoke_tables_overview.inc"
#include "visual_smoke_tables_merge.inc"
#include "visual_smoke_tables_layout.inc"

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1]) : fs::current_path() / "table_visual_smoke.docx";

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

    if (!configure_cjk_font_defaults(doc) || !configure_cjk_language_defaults(doc) ||
        !configure_direction_styles(doc)) {
        print_document_error(doc, "configure CJK/direction defaults");
        return 1;
    }

    auto paragraph = doc.paragraphs();
    if (!paragraph.has_next() ||
        !add_text(paragraph, "FeatherDoc Word Visual Smoke Check",
                  featherdoc::formatting_flag::bold)) {
        std::cerr << "failed to write title paragraph\n";
        return 1;
    }

    paragraph = paragraph.insert_paragraph_after(
        "This generated document is meant to be opened in Microsoft Word and then rendered to "
        "PDF and PNG for visual inspection.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append description paragraph\n";
        return 1;
    }

    paragraph = paragraph.insert_paragraph_after(
        "Focus points: repeated table headers, cantSplit row pagination, horizontal and "
        "vertical merges, cell text direction, borders, fills, margins, alignment, and "
        "stable mixed-direction wrapping.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append checklist paragraph\n";
        return 1;
    }

    paragraph =
        paragraph.insert_paragraph_after("Page review target: no clipped text, unreadable vertical "
                                         "cells, broken borders, missing shading, or split "
                                         "highlighted rows.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append review target paragraph\n";
        return 1;
    }

    paragraph = paragraph.insert_paragraph_after(
        utf8_from_u8(
            u8"\u4E2D\u6587/CJK \u9ED8\u8BA4\u5B57\u4F53\u68C0\u67E5\uff1A"
            u8"\u8FD9\u884C\u4F9D\u8D56 docDefaults \u7EE7\u627F eastAsia "
            u8"\u5B57\u4F53\u4E0E w:lang \u8BED\u8A00\u6807\u8BB0\uff0c"
            u8"\u9700\u8981\u89C2\u5BDF\u4E2D\u82F1\u6DF7\u6392\u662F\u5426\u7A33\u5B9A\u3002"));
    if (!paragraph.has_next()) {
        std::cerr << "failed to append default CJK review paragraph\n";
        return 1;
    }

    paragraph = paragraph.insert_paragraph_after(
        utf8_from_u8(
            u8"\u4E2D\u6587/CJK \u6837\u5F0F\u7EE7\u627F\u68C0\u67E5\uff1A"
            u8"\u8FD9\u884C\u4F7F\u7528 Strong \u6837\u5F0F\uff0C"
            u8"\u4E0D\u76F4\u63A5\u5BF9 run \u5199\u5B57\u4F53\u6216\u8BED\u8A00\u6807\u8BB0\u3002"));
    if (!paragraph.has_next() || !doc.set_run_style(paragraph.runs(), "Strong")) {
        print_document_error(doc, "append style-based CJK review paragraph");
        return 1;
    }

    paragraph = paragraph.insert_paragraph_after(
        utf8_from_u8(
            u8"RTL/bidi \u9ED8\u8BA4\u7EE7\u627F\u68C0\u67E5\uff1A"
            u8"\u8FD9\u884C\u4F9D\u8D56 docDefaults \u7EE7\u627F bidi \u8BED\u8A00\u6807\u8BB0\uff0C "
            u8"\u0645\u0631\u062D\u0628\u0627 \u0628\u0627\u0644\u0639\u0631\u0628\u064A\u0629 123"));
    if (!paragraph.has_next()) {
        std::cerr << "failed to append default bidi review paragraph\n";
        return 1;
    }

    paragraph = paragraph.insert_paragraph_after(
        utf8_from_u8(
            u8"RTL/bidi \u6837\u5F0F\u7EE7\u627F\u68C0\u67E5\uff1A"
            u8"\u8FD9\u884C\u4F7F\u7528 Strong \u6837\u5F0F\uff0C "
            u8"\u0627\u0644\u0639\u0631\u0628\u064A\u0629 \u0645\u0639 Strong style"));
    if (!paragraph.has_next() || !doc.set_run_style(paragraph.runs(), "Strong")) {
        print_document_error(doc, "append style-based bidi review paragraph");
        return 1;
    }

    paragraph = paragraph.insert_paragraph_after(
        utf8_from_u8(
            u8"RTL \u76F4\u63A5\u5C5E\u6027\u68C0\u67E5\uFF1A"
            u8"\u8FD9\u884C\u901A\u8FC7 Paragraph::set_bidi(true) \u4E0E "
            u8"Run::set_rtl(true) "
            u8"\u5F3A\u5236\u53F3\u5411\u6392\u7248\uFF0C"
            u8"\u0645\u0631\u062D\u0628\u0627 123 ABC"));
    if (!paragraph.has_next()) {
        std::cerr << "failed to append direct RTL review paragraph\n";
        return 1;
    }
    {
        auto rtl_run = paragraph.runs();
        if (!rtl_run.has_next() || !paragraph.set_bidi() || !rtl_run.set_rtl()) {
            print_document_error(doc, "apply direct RTL review properties");
            return 1;
        }
    }

    paragraph = paragraph.insert_paragraph_after(
        utf8_from_u8(
            u8"RTL \u6837\u5F0F\u7EE7\u627F\u68C0\u67E5\uFF1A"
            u8"\u8FD9\u884C\u4F7F\u7528 Quote \u6BB5\u843D\u6837\u5F0F\u4E0E "
            u8"Emphasis \u5B57\u7B26\u6837\u5F0F\uFF0C"
            u8"\u5E94\u4FDD\u6301\u963F\u62C9\u4F2F\u8BED\u4ECE\u53F3\u5411\u5DE6\u663E\u793A\uFF0C"
            u8"\u0627\u0644\u0639\u0631\u0628\u064A\u0629 "
            u8"\u0645\u0639 Quote/Emphasis"));
    if (!paragraph.has_next() || !doc.set_paragraph_style(paragraph, "Quote") ||
        !doc.set_run_style(paragraph.runs(), "Emphasis")) {
        print_document_error(doc, "append style-based RTL review paragraph");
        return 1;
    }

    if (!create_overview_table(doc)) {
        std::cerr << "failed to build the overview smoke table\n";
        return 1;
    }

    paragraph = append_body_paragraph(doc, "");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append spacer after overview table\n";
        return 1;
    }

    if (!create_multipage_table(doc)) {
        std::cerr << "failed to build the multipage smoke table\n";
        return 1;
    }

    paragraph = append_body_paragraph(doc, "");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append spacer after multipage table\n";
        return 1;
    }

    if (!create_merge_table(doc)) {
        std::cerr << "failed to build the merge smoke table\n";
        return 1;
    }

    if (!create_fixed_grid_merge_right_table(doc)) {
        std::cerr << "failed to build the fixed-grid merge-right smoke table\n";
        return 1;
    }

    paragraph = append_body_paragraph(
        doc, "Unmerge target: inspect the orange and green restored cells after splitting one "
             "horizontal merge and one vertical merge. They should look like clean standalone "
             "cells with no leftover merge artifacts.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append unmerge paragraph\n";
        return 1;
    }

    if (!create_unmerge_table(doc)) {
        std::cerr << "failed to build the unmerge smoke table\n";
        return 1;
    }

    paragraph = append_body_paragraph(
        doc, "Column insertion target: inspect inserted yellow and orange columns after both "
             "insert_cell_after()/insert_cell_before() calls, plus the merged-boundary insert "
             "that should place a yellow cell between the blue merged block and the green tail.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append column insertion paragraph\n";
        return 1;
    }

    if (!create_column_insertion_tables(doc)) {
        std::cerr << "failed to build the column insertion smoke tables\n";
        return 1;
    }

    paragraph = append_body_paragraph(
        doc, "Column-width target: inspect the blue/yellow/green three-column table and confirm "
             "the left key column stays narrow, the middle column stays medium, and the right "
             "evidence column stays obviously widest after tblGrid width edits.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append column-width paragraph\n";
        return 1;
    }

    if (!create_column_width_table(doc)) {
        std::cerr << "failed to build the column-width smoke table\n";
        return 1;
    }

    paragraph = append_body_paragraph(
        doc, "Direction stress target: inspect vertical table-cell text and narrow mixed "
             "RTL/LTR/CJK wrapping next to rotated cells.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append direction stress paragraph\n";
        return 1;
    }

    if (!create_direction_stress_table(doc)) {
        std::cerr << "failed to build the direction stress table\n";
        return 1;
    }

    paragraph = append_body_paragraph(
        doc, "End of smoke sample. Review the generated PNG pages rather than trusting XML-only "
             "tests.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append closing paragraph\n";
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << output_path.string() << '\n';
    return 0;
}
