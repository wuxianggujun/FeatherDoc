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

auto create_overview_table(featherdoc::Document &doc) -> bool {
    auto table = doc.append_table(1, 3);
    if (!require_step(configure_table(table, 9000U), "overview table configure")) {
        return false;
    }

    auto first_row = table.rows();
    if (!require_step(first_row.has_next(), "overview first row exists") ||
        !require_step(first_row.set_height_twips(520U, featherdoc::row_height_rule::exact),
                      "overview first row height")) {
        return false;
    }
    if (!require_step(set_row_widths(first_row, std::array<std::uint32_t, 3>{3000U, 3000U, 3000U}),
                      "overview first row widths")) {
        return false;
    }

    auto cell = first_row.cells();
    if (!require_step(cell.has_next(), "overview first row first cell exists") ||
        !require_step(cell.set_fill_color("D9EAF7"), "overview banner fill") ||
        !require_step(cell.set_border(featherdoc::cell_border_edge::bottom,
                                      {featherdoc::border_style::thick, 12, "1F1F1F", 0}),
                      "overview banner border") ||
        !require_step(add_cell_text(cell,
                                    "Overview banner: fixed-width table, merged cells, borders, and margins",
                                    featherdoc::formatting_flag::bold),
                      "overview banner text") ||
        !require_step(cell.merge_right(2U), "overview banner merge right")) {
        return false;
    }

    auto second_row = table.append_row(3);
    if (!require_step(second_row.has_next(), "overview second row exists") ||
        !require_step(set_row_widths(second_row, std::array<std::uint32_t, 3>{3000U, 3000U, 3000U}),
                      "overview second row widths")) {
        return false;
    }

    auto third_row = table.append_row(3);
    if (!third_row.has_next() ||
        !set_row_widths(third_row, std::array<std::uint32_t, 3>{3000U, 3000U, 3000U})) {
        return false;
    }

    cell = second_row.cells();
    if (!require_step(cell.has_next(), "overview second row first cell exists") ||
        !require_step(cell.set_fill_color("FFF2CC"), "overview vertical merge fill") ||
        !require_step(cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center),
                      "overview vertical merge valign") ||
        !require_step(add_cell_text(cell, "Vertical merge sample", featherdoc::formatting_flag::bold),
                      "overview vertical merge text") ||
        !require_step(cell.merge_down(1U), "overview vertical merge down")) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("E2F0D9") ||
        !add_cell_lines(cell,
                        {"Cell fill + per-edge border",
                         "Expected: green background and thick dark bottom edge"}) ||
        !cell.set_border(featherdoc::cell_border_edge::bottom,
                         {featherdoc::border_style::thick, 12, "2F5597", 0})) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("FCE4D6") ||
        !cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center) ||
        !cell.set_margin_twips(featherdoc::cell_margin_edge::left, 180U) ||
        !cell.set_margin_twips(featherdoc::cell_margin_edge::right, 180U)) {
        return false;
    }

    auto cjk_paragraph = cell.paragraphs();
    auto cjk_run = cjk_paragraph.add_run(
        utf8_from_u8(u8"\u4E2D\u6587/CJK \u6DF7\u6392\u68C0\u67E5"),
        featherdoc::formatting_flag::bold);
    if (!cjk_run.has_next()) {
        return false;
    }

    cjk_paragraph = cjk_paragraph.insert_paragraph_after(
        utf8_from_u8(
            u8"\u9884\u671F\uff1A\u8868\u683C\u5BBD\u5EA6\u4E0D\u88AB\u4E2D\u6587\u6491\u7206\uFF0C"
            u8"Invoice INV-2026-0001 \u6362\u884C\u4ECD\u7136\u7A33\u5B9A"));
    if (!cjk_paragraph.has_next()) {
        return false;
    }

    cell = third_row.cells();
    if (!cell.has_next()) {
        return false;
    }

    cell.next();
    if (!cell.has_next() ||
        !add_cell_lines(cell,
                        {"Horizontal merge preserved in first row",
                         "Vertical merge from left column should continue here"})) {
        return false;
    }

    cell.next();
    if (!cell.has_next() ||
        !add_cell_lines(cell,
                        {"Inside borders should stay continuous",
                         "No clipped shading or unexpected spacing"})) {
        return false;
    }

    return true;
}

auto create_multipage_table(featherdoc::Document &doc) -> bool {
    auto table = doc.append_table(1, 4);
    if (!require_step(configure_table(table, 9800U), "multipage table configure")) {
        return false;
    }

    auto header = table.rows();
    if (!require_step(header.has_next(), "multipage header exists") ||
        !require_step(header.set_height_twips(480U, featherdoc::row_height_rule::exact),
                      "multipage header height") ||
        !require_step(header.set_repeats_header(), "multipage header repeat") ||
        !require_step(header.set_cant_split(), "multipage header cantSplit") ||
        !require_step(set_row_widths(header, std::array<std::uint32_t, 4>{900U, 2500U, 3600U, 2800U}),
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
        !add_cell_text(cell, "Expected rendering", featherdoc::formatting_flag::bold)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Stress content", featherdoc::formatting_flag::bold)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Notes", featherdoc::formatting_flag::bold)) {
        return false;
    }

    for (std::size_t row_index = 1; row_index <= 24U; ++row_index) {
        auto row = table.append_row(4);
        if (!require_step(row.has_next(), "multipage row exists") ||
            !require_step(set_row_widths(row, std::array<std::uint32_t, 4>{900U, 2500U, 3600U, 2800U}),
                          "multipage row widths")) {
            return false;
        }

        if (row_index == 16U &&
            !require_step(row.set_cant_split(), "multipage highlighted row cantSplit")) {
            return false;
        }

        cell = row.cells();
        if (!cell.has_next()) {
            return false;
        }

        const auto case_label = std::string("R") + std::to_string(row_index);
        if (row_index == 16U && !cell.set_fill_color("FCE4D6")) {
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
        if (!add_cell_lines(cell,
                            {"Keep borders continuous",
                             row_index == 16U ? "Highlighted row should move as a whole"
                                              : "Header must repeat on later pages"})) {
            return false;
        }

        cell.next();
        if (!cell.has_next()) {
            return false;
        }
        std::vector<std::string> stress_lines;
        if (row_index == 16U) {
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
            !cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center)) {
            return false;
        }
        if (!add_cell_lines(cell,
                            {row_index == 16U ? "Inspect this row in the PNG review."
                                              : "Alternating rows help expose spacing drift.",
                             "No unexpected overlap or border gaps."})) {
            return false;
        }
    }

    return true;
}

auto create_merge_table(featherdoc::Document &doc) -> bool {
    auto table = doc.append_table(1, 4);
    if (!require_step(configure_table(table, 9000U), "merge table configure")) {
        return false;
    }

    auto row = table.rows();
    if (!require_step(row.has_next(), "merge first row exists") ||
        !require_step(set_row_widths(row, std::array<std::uint32_t, 4>{2250U, 2250U, 2250U, 2250U}),
                      "merge first row widths")) {
        return false;
    }

    auto cell = row.cells();
    if (!require_step(cell.has_next(), "merge first row first cell exists") ||
        !require_step(cell.set_fill_color("D9EAF7"), "merge horizontal fill") ||
        !require_step(add_cell_text(cell, "Horizontal merge", featherdoc::formatting_flag::bold),
                      "merge horizontal text") ||
        !require_step(cell.merge_right(1U), "merge horizontal right")) {
        return false;
    }

    cell = row.cells();
    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("E2F0D9") ||
        !add_cell_text(cell, "Reference")) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("FCE4D6") ||
        !add_cell_text(cell, "Borders")) {
        return false;
    }

    row = table.append_row(4);
    if (!require_step(row.has_next(), "merge second row exists") ||
        !require_step(set_row_widths(row, std::array<std::uint32_t, 4>{2250U, 2250U, 2250U, 2250U}),
                      "merge second row widths")) {
        return false;
    }

    auto third_row = table.append_row(4);
    if (!third_row.has_next() ||
        !set_row_widths(third_row, std::array<std::uint32_t, 4>{2250U, 2250U, 2250U, 2250U})) {
        return false;
    }

    cell = row.cells();
    if (!require_step(cell.has_next(), "merge second row first cell exists") ||
        !require_step(cell.set_fill_color("FFF2CC"), "merge vertical fill") ||
        !require_step(add_cell_text(cell, "Vertical merge", featherdoc::formatting_flag::bold),
                      "merge vertical text") ||
        !require_step(cell.merge_down(1U), "merge vertical down")) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center) ||
        !add_cell_lines(cell, {"Center aligned", "Cell-level margins active"}) ||
        !cell.set_margin_twips(featherdoc::cell_margin_edge::top, 180U) ||
        !cell.set_margin_twips(featherdoc::cell_margin_edge::bottom, 180U)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_border(featherdoc::cell_border_edge::right,
                                             {featherdoc::border_style::dashed, 10, "C00000", 0}) ||
        !add_cell_lines(cell, {"Dashed right border", "Should align after export"})) {
        return false;
    }

    cell.next();
    if (!cell.has_next() ||
        !add_cell_lines(cell, {"Baseline cell", "Used to compare row spacing"})) {
        return false;
    }

    cell = third_row.cells();
    if (!cell.has_next()) {
        return false;
    }
    cell.next();
    if (!cell.has_next() ||
        !add_cell_lines(cell, {"Continuation under vertical merge",
                               "The left yellow block should span both rows"})) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("EDEDED") ||
        !add_cell_text(cell, "Shaded")) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !add_cell_text(cell, "Stable")) {
        return false;
    }

    row = table.append_row(4);
    if (!row.has_next() ||
        !set_row_widths(row, std::array<std::uint32_t, 4>{2250U, 2250U, 2250U, 2250U})) {
        return false;
    }

    cell = row.cells();
    if (!cell.has_next() || !add_cell_text(cell, "Footer")) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !add_cell_text(cell, "No clipped borders")) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !add_cell_text(cell, "No missing fills")) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !add_cell_text(cell, "No overlap")) {
        return false;
    }

    return true;
}
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

    if (!configure_cjk_font_defaults(doc) || !configure_cjk_language_defaults(doc)) {
        print_document_error(doc, "configure CJK defaults");
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
        "vertical merges, borders, fills, margins, alignment, and stable spacing.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append checklist paragraph\n";
        return 1;
    }

    paragraph =
        paragraph.insert_paragraph_after("Page review target: no clipped text, broken borders, "
                                         "missing shading, or split highlighted rows.");
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
