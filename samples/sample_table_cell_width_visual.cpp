#include <featherdoc.hpp>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
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

auto append_body_paragraph(featherdoc::Document &doc, std::string_view text)
    -> featherdoc::Paragraph {
    auto paragraph = doc.paragraphs();
    while (paragraph.has_next()) {
        paragraph.next();
    }

    return paragraph.insert_paragraph_after(std::string{text});
}

auto configure_table(featherdoc::Table table) -> bool {
    return table.set_alignment(featherdoc::table_alignment::center) &&
           table.set_style_id("TableGrid") &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::top, 120U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 120U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::bottom, 120U) &&
           table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 120U);
}

auto seed_single_cell_table(featherdoc::Table table, std::string_view fill_color,
                            std::string_view text,
                            std::optional<std::uint32_t> width_twips) -> bool {
    auto row = table.rows();
    if (!row.has_next()) {
        return false;
    }

    auto cell = row.cells();
    if (!cell.has_next() || !cell.set_fill_color(std::string{fill_color})) {
        return false;
    }

    if (width_twips.has_value()) {
        if (!cell.set_width_twips(*width_twips)) {
            return false;
        }
    } else {
        if (!cell.clear_width()) {
            return false;
        }
    }

    auto paragraph = cell.paragraphs();
    return paragraph.set_text(std::string{text});
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "table_cell_width_visual.docx";

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

    constexpr std::string_view set_text =
        "SET WIDTH SHOULD WRAP THIS SENTENCE INTO A NARROW COLUMN";
    constexpr std::string_view clear_text =
        "CLEAR WIDTH SHOULD RETURN THIS SENTENCE TO A WIDE AUTO TABLE";

    auto paragraph = doc.paragraphs();
    if (!paragraph.set_text("Body table cell width CLI visual baseline") ||
        !paragraph
             .add_run(
                 " Compare set-table-cell-width and clear-table-cell-width on one page.")
             .has_next()) {
        print_document_error(doc, "write title");
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Table 0: set-table-cell-width should force the auto-width target cell into a visibly narrower single-cell column.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append table 0 intro paragraph\n";
        return 1;
    }

    auto set_width_table = doc.append_table(1U, 1U);
    if (!set_width_table.has_next() ||
        !require_step(configure_table(set_width_table),
                      "configure set-width table") ||
        !require_step(seed_single_cell_table(set_width_table, "DDEBF7", set_text,
                                             std::nullopt),
                      "seed set-width table")) {
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Table 1: clear-table-cell-width should remove the pre-seeded narrow width and let the target cell expand back to auto width.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append table 1 intro paragraph\n";
        return 1;
    }

    auto clear_width_table = doc.append_table(1U, 1U);
    if (!clear_width_table.has_next() ||
        !require_step(configure_table(clear_width_table),
                      "configure clear-width table") ||
        !require_step(seed_single_cell_table(clear_width_table, "FFF2CC", clear_text,
                                             2222U),
                      "seed clear-width table")) {
        return 1;
    }

    paragraph = append_body_paragraph(
        doc,
        "Trailing note: the paragraph below both target tables should stay fixed while only the selected cell width changes.");
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
