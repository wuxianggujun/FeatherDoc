#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct seeded_cell final {
    std::string_view text;
    std::string_view fill_color;
};

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

bool set_standard_borders(featherdoc::Table table) {
    const auto border = featherdoc::border_definition{
        featherdoc::border_style::single, 8U, "666666", 0U};

    return table.set_border(featherdoc::table_border_edge::top, border) &&
           table.set_border(featherdoc::table_border_edge::left, border) &&
           table.set_border(featherdoc::table_border_edge::bottom, border) &&
           table.set_border(featherdoc::table_border_edge::right, border) &&
           table.set_border(featherdoc::table_border_edge::inside_horizontal,
                            border) &&
           table.set_border(featherdoc::table_border_edge::inside_vertical,
                            border);
}

bool configure_table(featherdoc::Table table,
                     const std::vector<std::uint32_t> &column_widths) {
    if (!table.has_next() || column_widths.empty()) {
        return false;
    }

    std::uint32_t total_width = 0U;
    for (const auto width : column_widths) {
        total_width += width;
    }

    if (!table.set_width_twips(total_width) ||
        !table.set_style_id("TableGrid") ||
        !table.set_style_look({true, false, true, false, true, false}) ||
        !table.set_layout_mode(featherdoc::table_layout_mode::fixed) ||
        !table.set_alignment(featherdoc::table_alignment::center) ||
        !table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 96U) ||
        !table.set_cell_margin_twips(featherdoc::cell_margin_edge::right,
                                     96U) ||
        !set_standard_borders(table)) {
        return false;
    }

    for (std::size_t column_index = 0; column_index < column_widths.size();
         ++column_index) {
        if (!table.set_column_width_twips(column_index,
                                          column_widths[column_index])) {
            return false;
        }
    }

    return true;
}

bool set_row_cells(featherdoc::TableRow row,
                   const std::vector<seeded_cell> &cells) {
    if (!row.has_next() ||
        !row.set_height_twips(820U, featherdoc::row_height_rule::at_least)) {
        return false;
    }

    auto cell = row.cells();
    for (const auto &seeded : cells) {
        if (!cell.has_next()) {
            return false;
        }
        if (!seeded.fill_color.empty() &&
            !cell.set_fill_color(std::string{seeded.fill_color})) {
            return false;
        }
        if (!cell.set_vertical_alignment(
                featherdoc::cell_vertical_alignment::center) ||
            !cell.set_text(std::string{seeded.text})) {
            return false;
        }
        cell.next();
    }

    return true;
}

bool append_retained_table(featherdoc::TemplatePart part,
                           std::string_view header_left,
                           std::string_view header_right,
                           std::string_view body_left,
                           std::string_view body_right) {
    auto table = part.append_table(2U, 2U);
    if (!configure_table(table, {2200U, 4200U})) {
        return false;
    }

    auto row = table.rows();
    if (!set_row_cells(row,
                       {
                           {header_left, "D9EAF7"},
                           {header_right, "D9EAF7"},
                       })) {
        return false;
    }

    row.next();
    return set_row_cells(row,
                         {
                             {body_left, ""},
                             {body_right, ""},
                         });
}

bool append_body_context(featherdoc::Document &doc, std::string_view intro,
                         std::string_view tail) {
    auto body_template = doc.body_template();
    if (!body_template) {
        std::cerr << "failed to access body template\n";
        return false;
    }

    auto paragraph = body_template.paragraphs();
    if (!paragraph.has_next() || !paragraph.set_text(std::string{intro})) {
        return false;
    }

    return body_template.append_paragraph(std::string{tail}).has_next();
}

bool seed_header_merge_right(featherdoc::Document &doc) {
    auto &header_paragraph = doc.ensure_header_paragraphs();
    if (!header_paragraph.has_next() ||
        !header_paragraph.set_text(
            "Template table CLI direct header merge-right baseline")) {
        return false;
    }

    auto header_template = doc.header_template(0U);
    if (!header_template) {
        std::cerr << "failed to access header template\n";
        return false;
    }

    if (!require_step(append_retained_table(header_template, "Retained", "State",
                                            "Blue direct header table",
                                            "Should remain unchanged."),
                      "append retained direct header table")) {
        return false;
    }

    auto target_table = header_template.append_table(2U, 3U);
    if (!require_step(configure_table(target_table, {1400U, 2100U, 3300U}),
                      "configure direct header merge-right target")) {
        return false;
    }

    auto row = target_table.rows();
    if (!set_row_cells(row,
                       {
                           {"Purple span anchor", "EADCF8"},
                           {"Merge source", "EADCF8"},
                           {"Green tail", "E2F0D9"},
                       })) {
        return false;
    }

    row.next();
    if (!set_row_cells(row,
                       {
                           {"1400 cue", ""},
                           {"2100 cue", ""},
                           {"3300 cue", ""},
                       })) {
        return false;
    }

    return append_body_context(
        doc,
        "The direct-header merge-right sample keeps body text below the header "
        "area.",
        "After the purple direct header cell expands to the right, the retained "
        "blue table and the body paragraphs should stay stable.");
}

bool seed_footer_unmerge_down(featherdoc::Document &doc) {
    auto &footer_paragraph = doc.ensure_footer_paragraphs();
    if (!footer_paragraph.has_next() ||
        !footer_paragraph.set_text(
            "Template table CLI direct footer unmerge-down baseline")) {
        return false;
    }

    auto footer_template = doc.footer_template(0U);
    if (!footer_template) {
        std::cerr << "failed to access footer template\n";
        return false;
    }

    if (!require_step(append_retained_table(footer_template, "Retained", "State",
                                            "Blue direct footer table",
                                            "Should remain unchanged."),
                      "append retained direct footer table")) {
        return false;
    }

    auto target_table = footer_template.append_table(3U, 2U);
    if (!require_step(configure_table(target_table, {1800U, 4200U}),
                      "configure direct footer unmerge-down target")) {
        return false;
    }

    auto row = target_table.rows();
    if (!set_row_cells(row,
                       {
                           {"Merged orange pillar", "FCE4D6"},
                           {"Footer top", ""},
                       })) {
        return false;
    }

    row.next();
    if (!set_row_cells(row,
                       {
                           {"Hidden middle", "DDEBF7"},
                           {"Footer middle", ""},
                       })) {
        return false;
    }

    row.next();
    if (!set_row_cells(row,
                       {
                           {"Hidden bottom", "DDEBF7"},
                           {"Footer bottom", ""},
                       })) {
        return false;
    }

    auto merged = target_table.rows().cells();
    if (!merged.has_next() || !merged.merge_down(2U)) {
        std::cerr << "failed to seed direct footer vertical merge\n";
        return false;
    }

    return append_body_context(
        doc,
        "The direct-footer unmerge-down sample keeps the body paragraphs above "
        "the footer area.",
        "After the CLI split, the orange direct footer pillar should break back "
        "into stacked cells without colliding with the body.");
}

bool seed_body_merge_down(featherdoc::Document &doc) {
    auto body_template = doc.body_template();
    if (!body_template) {
        std::cerr << "failed to access body template\n";
        return false;
    }

    auto paragraph = body_template.paragraphs();
    if (!paragraph.has_next() ||
        !paragraph.set_text(
            "Template table CLI direct-part merge-down body baseline")) {
        return false;
    }

    if (!require_step(append_retained_table(body_template, "Retained", "Status",
                                            "Blue body table",
                                            "Should remain unchanged."),
                      "append retained body table")) {
        return false;
    }

    auto target_table = body_template.append_table(3U, 2U);
    if (!require_step(configure_table(target_table, {1800U, 4200U}),
                      "configure body merge-down target")) {
        return false;
    }

    auto row = target_table.rows();
    if (!set_row_cells(row,
                       {
                           {"Green pillar anchor", "E2F0D9"},
                           {"Body top", ""},
                       })) {
        return false;
    }

    row.next();
    if (!set_row_cells(row,
                       {
                           {"Green middle", "E2F0D9"},
                           {"Body middle", ""},
                       })) {
        return false;
    }

    row.next();
    if (!set_row_cells(row,
                       {
                           {"Green tail", "E2F0D9"},
                           {"Body bottom", ""},
                       })) {
        return false;
    }

    return body_template.append_paragraph(
               "After the CLI merge, the green body table should turn its first "
               "column into one vertical pillar while the retained blue table "
               "and surrounding paragraphs stay stable.")
        .has_next();
}

bool seed_case(featherdoc::Document &doc, std::string_view case_id) {
    if (case_id == "header-merge-right") {
        return seed_header_merge_right(doc);
    }
    if (case_id == "footer-unmerge-down") {
        return seed_footer_unmerge_down(doc);
    }
    if (case_id == "body-merge-down") {
        return seed_body_merge_down(doc);
    }

    std::cerr << "unknown case id: " << case_id << '\n';
    return false;
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr
            << "usage: sample_template_table_cli_direct_merge_unmerge_visual "
               "<case-id> <output.docx>\n";
        return 2;
    }

    const std::string_view case_id = argv[1];
    const fs::path output_path = fs::path(argv[2]);

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

    if (!seed_case(doc, case_id)) {
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
