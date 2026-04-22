#include <featherdoc.hpp>

#include <array>
#include <cstdint>
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
auto configure_table(featherdoc::Table table, std::string_view style_id,
                     const std::array<std::uint32_t, N> &widths) -> bool {
    std::uint32_t total_width = 0U;
    for (const auto width : widths) {
        total_width += width;
    }

    return table.set_width_twips(total_width) &&
           table.set_layout_mode(featherdoc::table_layout_mode::fixed) &&
           table.set_alignment(featherdoc::table_alignment::center) &&
           table.set_style_id(style_id) && set_table_column_widths(table, widths);
}

auto fill_two_cell_row(featherdoc::TableRow row, std::string_view left_text,
                       std::string_view right_text) -> bool {
    auto cell = row.cells();
    if (!cell.has_next() || !cell.set_text(std::string{left_text})) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_text(std::string{right_text})) {
        return false;
    }

    return true;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1]) : fs::current_path() / "ensure_style_visual.docx";

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

    auto paragraph_style = featherdoc::paragraph_style_definition{};
    paragraph_style.name = "Review Paragraph";
    paragraph_style.based_on = std::string{"Normal"};
    paragraph_style.next_style = std::string{"ReviewPara"};
    paragraph_style.is_custom = true;
    paragraph_style.is_semi_hidden = false;
    paragraph_style.is_unhide_when_used = false;
    paragraph_style.is_quick_format = true;
    paragraph_style.run_font_family = std::string{"Courier New"};
    if (!doc.ensure_paragraph_style("ReviewPara", paragraph_style)) {
        print_document_error(doc, "ensure_paragraph_style(ReviewPara)");
        return 1;
    }

    auto inherited_paragraph_style = featherdoc::paragraph_style_definition{};
    inherited_paragraph_style.name = "Review Paragraph Child";
    inherited_paragraph_style.based_on = std::string{"ReviewPara"};
    inherited_paragraph_style.next_style = std::string{"ReviewParaChild"};
    inherited_paragraph_style.is_custom = true;
    inherited_paragraph_style.is_semi_hidden = false;
    inherited_paragraph_style.is_unhide_when_used = false;
    inherited_paragraph_style.is_quick_format = true;
    if (!doc.ensure_paragraph_style("ReviewParaChild", inherited_paragraph_style)) {
        print_document_error(doc, "ensure_paragraph_style(ReviewParaChild)");
        return 1;
    }

    auto character_style = featherdoc::character_style_definition{};
    character_style.name = "Accent Marker";
    character_style.based_on = std::string{"DefaultParagraphFont"};
    character_style.is_custom = true;
    character_style.is_semi_hidden = false;
    character_style.is_unhide_when_used = false;
    character_style.is_quick_format = true;
    character_style.run_font_family = std::string{"Courier New"};
    character_style.run_rtl = false;
    if (!doc.ensure_character_style("AccentMarker", character_style)) {
        print_document_error(doc, "ensure_character_style(AccentMarker)");
        return 1;
    }

    auto table_style = featherdoc::table_style_definition{};
    table_style.name = "Review Table";
    table_style.based_on = std::string{"TableGrid"};
    table_style.is_custom = true;
    table_style.is_semi_hidden = false;
    table_style.is_unhide_when_used = false;
    table_style.is_quick_format = true;
    if (!doc.ensure_table_style("ReviewTable", table_style)) {
        print_document_error(doc, "ensure_table_style(ReviewTable)");
        return 1;
    }

    auto paragraph = doc.paragraphs();
    if (!paragraph.set_text("Ensure style CLI visual baseline") ||
        !paragraph
             .add_run(" Compare paragraph-style definition rewrites against character-style rewrites on one page.")
             .has_next()) {
        print_document_error(doc, "write title");
        return 1;
    }

    auto paragraph_style_target = append_body_paragraph(
        doc,
        "Paragraph style target: this paragraph already uses ReviewPara, so ensure-paragraph-style should rewrite the style definition and restyle this whole line without rebinding the paragraph.");
    if (!paragraph_style_target.has_next() ||
        !doc.set_paragraph_style(paragraph_style_target, "ReviewPara")) {
        print_document_error(doc, "configure paragraph style target");
        return 1;
    }

    auto inherited_paragraph_target = append_body_paragraph(
        doc,
        "Inherited style target: this paragraph uses ReviewParaChild, so rewriting ReviewPara should flow through the basedOn chain and restyle this line without rebinding the child paragraph.");
    if (!inherited_paragraph_target.has_next() ||
        !doc.set_paragraph_style(inherited_paragraph_target, "ReviewParaChild")) {
        print_document_error(doc, "configure inherited paragraph style target");
        return 1;
    }

    auto character_style_target = append_body_paragraph(doc, "Character style target: ");
    if (!character_style_target.has_next()) {
        std::cerr << "failed to append character style target paragraph\n";
        return 1;
    }
    auto styled_run = character_style_target.add_run(
        "ALPHA 123 hello world");
    if (!styled_run.has_next() || !doc.set_run_style(styled_run, "AccentMarker") ||
        !character_style_target
             .add_run(
                 " while the prefix and suffix stay in the document default formatting.")
             .has_next()) {
        print_document_error(doc, "configure character style target");
        return 1;
    }

    auto table_intro = append_body_paragraph(
        doc,
        "Table style target: this table already uses ReviewTable, so ensure-table-style should switch its inherited look between bordered TableGrid and borderless TableNormal without rebinding the table.");
    if (!table_intro.has_next()) {
        std::cerr << "failed to append table intro paragraph\n";
        return 1;
    }

    auto table = doc.append_table(3U, 2U);
    if (!table.has_next() ||
        !configure_table(table, "ReviewTable",
                         std::array<std::uint32_t, 2>{2600U, 3000U})) {
        print_document_error(doc, "configure ReviewTable target");
        return 1;
    }

    auto row = table.rows();
    if (!row.has_next() ||
        !fill_two_cell_row(row, "CASE", "EXPECTED LOOK")) {
        print_document_error(doc, "fill ReviewTable header row");
        return 1;
    }

    row.next();
    if (!row.has_next() ||
        !fill_two_cell_row(row, "baseline", "ReviewTable inherits TableGrid")) {
        print_document_error(doc, "fill ReviewTable baseline row");
        return 1;
    }

    row.next();
    if (!row.has_next() ||
        !fill_two_cell_row(row, "mutation", "ReviewTable inherits TableNormal")) {
        print_document_error(doc, "fill ReviewTable mutation row");
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
