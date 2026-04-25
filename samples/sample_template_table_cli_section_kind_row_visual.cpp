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

    for (std::size_t column_index = 0U; column_index < column_widths.size();
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
    for (std::size_t cell_index = 0U; cell_index < cells.size(); ++cell_index) {
        if (!cell.has_next()) {
            return false;
        }

        const auto &seeded = cells[cell_index];
        if (!seeded.fill_color.empty() &&
            !cell.set_fill_color(std::string{seeded.fill_color})) {
            return false;
        }
        if (!cell.set_vertical_alignment(
                featherdoc::cell_vertical_alignment::center) ||
            !cell.set_text(std::string{seeded.text})) {
            return false;
        }

        if (cell_index + 1U < cells.size()) {
            cell.next();
        }
    }

    return true;
}

bool append_seeded_table(featherdoc::TemplatePart part,
                         const std::vector<std::uint32_t> &column_widths,
                         const std::vector<std::vector<seeded_cell>> &rows) {
    if (rows.empty()) {
        return false;
    }

    auto table = part.append_table(static_cast<std::uint32_t>(rows.size()),
                                   static_cast<std::uint32_t>(
                                       column_widths.size()));
    if (!configure_table(table, column_widths)) {
        return false;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < rows.size(); ++row_index) {
        if (!set_row_cells(row, rows[row_index])) {
            return false;
        }
        if (row_index + 1U < rows.size()) {
            row.next();
        }
    }

    return true;
}

bool append_retained_table(featherdoc::TemplatePart part,
                           std::string_view header_left,
                           std::string_view header_right,
                           std::string_view body_left,
                           std::string_view body_right) {
    return append_seeded_table(
        part, {2200U, 4200U},
        {
            {
                {header_left, "D9EAF7"},
                {header_right, "D9EAF7"},
            },
            {
                {body_left, ""},
                {body_right, ""},
            },
        });
}

bool append_target_table(featherdoc::TemplatePart part,
                         const std::vector<std::vector<seeded_cell>> &rows) {
    return append_seeded_table(part, {2200U, 4200U}, rows);
}

bool append_body_filler(featherdoc::Document &doc, std::string_view intro,
                        std::string_view page_hint) {
    auto body_template = doc.body_template();
    if (!body_template) {
        std::cerr << "failed to access body template\n";
        return false;
    }

    auto paragraph = body_template.paragraphs();
    if (!paragraph.has_next() || !paragraph.set_text(std::string{intro})) {
        return false;
    }

    if (!body_template.append_paragraph(std::string{page_hint}).has_next()) {
        return false;
    }

    for (std::size_t line_index = 0U; line_index < 150U; ++line_index) {
        auto filler = body_template.append_paragraph(
            "Section kind row filler line " +
            std::to_string(line_index + 1U) +
            " keeps the document long enough for page 1, page 2, and page 3 "
            "visual checks.");
        if (!filler.has_next()) {
            return false;
        }
    }

    return body_template.append_paragraph(
               "Review the selected pages in the regression output to confirm "
               "only the targeted section-kind row chain changed while the "
               "control page kept its original layout.")
        .has_next();
}

bool seed_section_header_default_insert_row_before(featherdoc::Document &doc) {
    auto &first_header = doc.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    if (!first_header.has_next() ||
        !first_header.set_text(
            "First-page header control. Page 1 must not render the default "
            "header row insertion target.")) {
        return false;
    }

    auto &default_header = doc.ensure_section_header_paragraphs(0U);
    if (!default_header.has_next() ||
        !default_header.set_text(
            "Default header baseline. Page 3 should show the retained blue "
            "table plus the purple row target table.")) {
        return false;
    }

    auto default_header_template = doc.section_header_template(0U);
    if (!default_header_template) {
        std::cerr << "failed to access default section header template\n";
        return false;
    }

    if (!require_step(append_retained_table(default_header_template,
                                            "Default Header", "Control",
                                            "Retained default header table",
                                            "Should remain unchanged."),
                      "append retained default header table")) {
        return false;
    }

    if (!require_step(append_target_table(
                          default_header_template,
                          {
                              {
                                  {"Default Header Final", "EADCF8"},
                                  {"Value", "EADCF8"},
                              },
                              {
                                  {"Keep default header seed", ""},
                                  {"Seed detail", ""},
                              },
                          }),
                      "append default header row target table")) {
        return false;
    }

    return append_body_filler(
        doc,
        "Section-header default insert-row-before baseline. Page 1 is the "
        "control page and page 3 is the default-header target page.",
        "After the mutation, page 3 should gain a new middle row inside the "
        "purple default-header table.");
}

bool seed_section_header_even_append_row(featherdoc::Document &doc) {
    auto &default_header = doc.ensure_section_header_paragraphs(0U);
    if (!default_header.has_next() ||
        !default_header.set_text(
            "Default header control. Page 3 should not inherit the even "
            "header row append target.")) {
        return false;
    }

    auto &even_header = doc.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!even_header.has_next() ||
        !even_header.set_text(
            "Even header baseline. Page 2 should show the retained blue table "
            "plus the peach row target table.")) {
        return false;
    }

    auto even_header_template = doc.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    if (!even_header_template) {
        std::cerr << "failed to access even section header template\n";
        return false;
    }

    if (!require_step(append_retained_table(even_header_template, "Even Header",
                                            "Control",
                                            "Retained even header table",
                                            "Should remain unchanged."),
                      "append retained even header table")) {
        return false;
    }

    if (!require_step(append_target_table(
                          even_header_template,
                          {
                              {
                                  {"Even Header Final", "FCE5CD"},
                                  {"Value", "FCE5CD"},
                              },
                              {
                                  {"Keep even header seed", ""},
                                  {"Seed detail", ""},
                              },
                          }),
                      "append even header row target table")) {
        return false;
    }

    return append_body_filler(
        doc,
        "Section-header even append-row baseline. Page 2 is the even-header "
        "target page and page 3 is the default-header control page.",
        "After the mutation, the peach even-header table on page 2 should "
        "gain a new trailing row.");
}

bool seed_section_footer_first_remove_row(featherdoc::Document &doc) {
    auto &default_footer = doc.ensure_section_footer_paragraphs(0U);
    if (!default_footer.has_next() ||
        !default_footer.set_text(
            "Default footer control. Page 3 must not render the first-page "
            "footer row removal target.")) {
        return false;
    }

    auto &first_footer = doc.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    if (!first_footer.has_next() ||
        !first_footer.set_text(
            "First-page footer baseline. Page 1 should show the retained blue "
            "table plus the orange three-row target table.")) {
        return false;
    }

    auto first_footer_template = doc.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    if (!first_footer_template) {
        std::cerr << "failed to access first section footer template\n";
        return false;
    }

    if (!require_step(append_retained_table(first_footer_template,
                                            "First Footer", "Control",
                                            "Retained first footer table",
                                            "Should remain unchanged."),
                      "append retained first footer table")) {
        return false;
    }

    if (!require_step(append_target_table(
                          first_footer_template,
                          {
                              {
                                  {"First Footer Final", "FCE4D6"},
                                  {"Value", "FCE4D6"},
                              },
                              {
                                  {"Remove this row", "FFF2CC"},
                                  {"Tighten spacing", "FFF2CC"},
                              },
                              {
                                  {"Keep footer tail", ""},
                                  {"Seed detail", ""},
                              },
                          }),
                      "append first footer row target table")) {
        return false;
    }

    return append_body_filler(
        doc,
        "Section-footer first remove-row baseline. Page 1 is the first-page "
        "footer target and page 3 is the default-footer control page.",
        "After the mutation, the orange first-page footer table should drop "
        "its middle row without colliding with the body.");
}

bool seed_section_footer_default_insert_row_after(featherdoc::Document &doc) {
    auto &first_footer = doc.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    if (!first_footer.has_next() ||
        !first_footer.set_text(
            "First-page footer control. Page 1 should stay unchanged while the "
            "default footer inserts a new row on page 3.")) {
        return false;
    }

    auto &default_footer = doc.ensure_section_footer_paragraphs(0U);
    if (!default_footer.has_next() ||
        !default_footer.set_text(
            "Default footer baseline. Page 3 should show the retained blue "
            "table plus the green row target table.")) {
        return false;
    }

    auto default_footer_template = doc.section_footer_template(0U);
    if (!default_footer_template) {
        std::cerr << "failed to access default section footer template\n";
        return false;
    }

    if (!require_step(append_retained_table(default_footer_template,
                                            "Default Footer", "Control",
                                            "Retained default footer table",
                                            "Should remain unchanged."),
                      "append retained default footer table")) {
        return false;
    }

    if (!require_step(append_target_table(
                          default_footer_template,
                          {
                              {
                                  {"Default Footer Final", "E2F0D9"},
                                  {"Value", "E2F0D9"},
                              },
                              {
                                  {"Keep default footer seed", ""},
                                  {"Seed detail", ""},
                              },
                          }),
                      "append default footer row target table")) {
        return false;
    }

    return append_body_filler(
        doc,
        "Section-footer default insert-row-after baseline. Page 1 is the "
        "first-page footer control and page 3 is the default-footer target.",
        "After the mutation, the green default-footer table on page 3 should "
        "gain a new middle row.");
}

bool seed_case(featherdoc::Document &doc, std::string_view case_id) {
    if (case_id == "section-header-default-insert-before-row") {
        return seed_section_header_default_insert_row_before(doc);
    }
    if (case_id == "section-header-even-append-row") {
        return seed_section_header_even_append_row(doc);
    }
    if (case_id == "section-footer-first-remove-row") {
        return seed_section_footer_first_remove_row(doc);
    }
    if (case_id == "section-footer-default-insert-after-row") {
        return seed_section_footer_default_insert_row_after(doc);
    }

    std::cerr << "unknown case id: " << case_id << '\n';
    return false;
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "usage: sample_template_table_cli_section_kind_row_visual "
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
