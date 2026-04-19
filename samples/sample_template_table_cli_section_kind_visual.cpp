#include <featherdoc.hpp>

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

bool set_row_values(featherdoc::TableRow row, std::string_view left,
                    std::string_view right) {
    auto cell = row.cells();
    if (!cell.has_next()) {
        return false;
    }
    if (!cell.set_text(std::string{left})) {
        return false;
    }

    cell.next();
    if (!cell.has_next()) {
        return false;
    }

    return cell.set_text(std::string{right});
}

bool set_row_fill(featherdoc::TableRow row, std::string_view fill_color) {
    for (auto cell = row.cells(); cell.has_next(); cell.next()) {
        if (!cell.set_fill_color(fill_color)) {
            return false;
        }
    }

    return true;
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

bool configure_two_column_table(featherdoc::Table table,
                                std::string_view header_left,
                                std::string_view header_right,
                                std::string_view header_fill,
                                std::string_view body_left,
                                std::string_view body_right) {
    if (!table.has_next()) {
        return false;
    }

    if (!table.set_width_twips(6800U) || !table.set_style_id("TableGrid") ||
        !table.set_style_look({true, false, true, false, true, false}) ||
        !table.set_layout_mode(featherdoc::table_layout_mode::fixed) ||
        !table.set_alignment(featherdoc::table_alignment::center) ||
        !table.set_column_width_twips(0U, 2600U) ||
        !table.set_column_width_twips(1U, 4200U) ||
        !table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 96U) ||
        !table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 96U) ||
        !set_standard_borders(table)) {
        return false;
    }

    auto row = table.rows();
    if (!row.has_next()) {
        return false;
    }
    if (!set_row_fill(row, header_fill) ||
        !set_row_values(row, header_left, header_right)) {
        return false;
    }

    row.next();
    if (!row.has_next()) {
        return false;
    }

    return set_row_values(row, body_left, body_right);
}

bool seed_default_header(featherdoc::Document &doc) {
    auto &header_paragraph = doc.ensure_section_header_paragraphs(0U);
    if (!header_paragraph.has_next() ||
        !header_paragraph.set_text(
            "Default header baseline. Page 3 should render these tables.")) {
        return false;
    }

    auto header_template = doc.section_header_template(0U);
    if (!header_template) {
        std::cerr << "failed to access default section header template\n";
        return false;
    }

    if (!require_step(configure_two_column_table(
                          header_template.append_table(2U, 2U),
                          "Default Header", "Control", "D9EAF7",
                          "Retained default header table",
                          "Should remain unchanged."),
                      "configure retained default header table")) {
        return false;
    }

    return require_step(configure_two_column_table(
                            header_template.append_table(2U, 2U),
                            "Default Header Final", "Value", "EADCF8",
                            "Pending default header update", "Seed"),
                        "configure target default header table");
}

bool seed_first_header(featherdoc::Document &doc) {
    auto &header_paragraph = doc.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    if (!header_paragraph.has_next() ||
        !header_paragraph.set_text(
            "First-page header control. Page 1 must not use the default or "
            "even header tables.")) {
        return false;
    }

    return true;
}

bool seed_even_header(featherdoc::Document &doc) {
    auto &header_paragraph = doc.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!header_paragraph.has_next() ||
        !header_paragraph.set_text(
            "Even header baseline. Page 2 should render these tables.")) {
        return false;
    }

    auto header_template = doc.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    if (!header_template) {
        std::cerr << "failed to access even section header template\n";
        return false;
    }

    if (!require_step(configure_two_column_table(
                          header_template.append_table(2U, 2U),
                          "Even Header", "Control", "D9EAF7",
                          "Retained even header table",
                          "Should remain unchanged."),
                      "configure retained even header table")) {
        return false;
    }

    return require_step(configure_two_column_table(
                            header_template.append_table(2U, 2U),
                            "Even Header Final", "Value", "FCE5CD",
                            "Pending even header update", "Seed"),
                        "configure target even header table");
}

bool seed_default_footer(featherdoc::Document &doc) {
    auto &footer_paragraph = doc.ensure_section_footer_paragraphs(0U);
    if (!footer_paragraph.has_next() ||
        !footer_paragraph.set_text(
            "Default footer baseline. Page 3 should render these tables.")) {
        return false;
    }

    auto footer_template = doc.section_footer_template(0U);
    if (!footer_template) {
        std::cerr << "failed to access default section footer template\n";
        return false;
    }

    if (!require_step(configure_two_column_table(
                          footer_template.append_table(2U, 2U),
                          "Default Footer", "Control", "D9EAF7",
                          "Retained default footer table",
                          "Should remain unchanged."),
                      "configure retained default footer table")) {
        return false;
    }

    return require_step(configure_two_column_table(
                            footer_template.append_table(2U, 2U),
                            "Default Footer Final", "Value", "E2F0D9",
                            "Pending default footer update", "Seed"),
                        "configure target default footer table");
}

bool seed_first_footer(featherdoc::Document &doc) {
    auto &footer_paragraph = doc.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    if (!footer_paragraph.has_next() ||
        !footer_paragraph.set_text(
            "First-page footer baseline. Page 1 should render these tables.")) {
        return false;
    }

    auto footer_template = doc.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    if (!footer_template) {
        std::cerr << "failed to access first-page section footer template\n";
        return false;
    }

    if (!require_step(configure_two_column_table(
                          footer_template.append_table(2U, 2U),
                          "First Footer", "Control", "D9EAF7",
                          "Retained first footer table",
                          "Should remain unchanged."),
                      "configure retained first footer table")) {
        return false;
    }

    return require_step(configure_two_column_table(
                            footer_template.append_table(2U, 2U),
                            "First Footer Final", "Value", "FCE4D6",
                            "Pending first footer update", "Seed"),
                        "configure target first footer table");
}

bool seed_body(featherdoc::Document &doc) {
    auto body_template = doc.body_template();
    if (!body_template) {
        std::cerr << "failed to access body template\n";
        return false;
    }

    auto intro = body_template.paragraphs();
    if (!intro.has_next() ||
        !intro.set_text(
            "Section kind visual regression sample. Page 1 uses first-page "
            "footer, page 2 uses even header, and page 3 uses default "
            "header/footer.")) {
        return false;
    }

    if (!body_template.append_paragraph(
             "The regression captures selected pages instead of only page 1 "
             "so the kind-specific tables remain visible in Word.")
             .has_next()) {
        return false;
    }

    for (std::size_t line_index = 0U; line_index < 150U; ++line_index) {
        const auto line_number = std::to_string(line_index + 1U);
        auto paragraph = body_template.append_paragraph(
            "Section kind filler line " + line_number +
            " keeps the document long enough for page 1, page 2, and page 3 "
            "visual checks.");
        if (!paragraph.has_next()) {
            return false;
        }
    }

    if (!body_template.append_paragraph(
             "Review page 3 to confirm the default header/footer tables grow "
             "without colliding with the body flow.")
             .has_next()) {
        return false;
    }

    return true;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() /
                       "template_table_cli_section_kind_visual.docx";

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

    if (!seed_body(doc) || !seed_default_header(doc) || !seed_first_header(doc) ||
        !seed_even_header(doc) || !seed_default_footer(doc) ||
        !seed_first_footer(doc)) {
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
