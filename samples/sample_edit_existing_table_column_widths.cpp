#include <featherdoc.hpp>

#include <array>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace fs = std::filesystem;

namespace {

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

bool require_step(bool ok, std::string_view step) {
    if (!ok) {
        std::cerr << "step failed: " << step << '\n';
    }
    return ok;
}

bool configure_table(featherdoc::Table table) {
    return table.has_next() && table.set_width_twips(7800U) &&
           table.set_layout_mode(featherdoc::table_layout_mode::fixed) &&
           table.set_alignment(featherdoc::table_alignment::center) &&
           table.set_style_id("TableGrid");
}

bool set_row_values(featherdoc::TableRow row,
                    const std::array<const char *, 3> &texts,
                    const std::array<const char *, 3> *fills = nullptr) {
    if (!row.has_next() ||
        !row.set_height_twips(900U, featherdoc::row_height_rule::at_least)) {
        return false;
    }

    auto cell = row.cells();
    for (std::size_t index = 0; index < texts.size(); ++index) {
        if (!cell.has_next() ||
            !cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center)) {
            return false;
        }
        if (fills != nullptr && !cell.set_fill_color((*fills)[index])) {
            return false;
        }
        if (!cell.set_text(texts[index])) {
            return false;
        }
        cell.next();
    }

    return true;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1]) : fs::current_path() / "edit_existing_table_column_widths.docx";

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
        !paragraph.set_text("Seed a three-column table, reopen it, and edit tblGrid column widths "
                            "without touching raw XML.")) {
        std::cerr << "failed to seed intro paragraph\n";
        return 1;
    }

    auto table = seed.append_table(3, 3);
    if (!require_step(configure_table(table), "configure seed table")) {
        return 1;
    }

    const std::array<const char *, 3> header_fills = {
        "D9EAF7",
        "FFF2CC",
        "E2F0D9",
    };
    auto row = table.rows();
    if (!require_step(set_row_values(row,
                                     {"Key", "Review state", "Evidence and implementation note"},
                                     &header_fills),
                      "fill header row")) {
        return 1;
    }

    row.next();
    if (!require_step(set_row_values(
                          row,
                          {"A-7", "Waiting",
                           "This column will become the widest one after reopening the document."}),
                      "fill second row")) {
        return 1;
    }

    row.next();
    if (!require_step(set_row_values(
                          row,
                          {"B-2", "Ready",
                           "Use Table::set_column_width_twips(...) to widen this detail column "
                           "while keeping the table layout fixed."}),
                      "fill third row")) {
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
             .add_run(" The reopened table now uses column widths 1200 / 2200 / 4400 twips "
                      "through Table::set_column_width_twips(...), then re-applies fixed layout "
                      "to normalize any stale cell tcW values back to tblGrid.")
             .has_next()) {
        std::cerr << "failed to update intro paragraph\n";
        return 1;
    }

    table = doc.tables();
    if (!table.has_next() ||
        !require_step(table.set_column_width_twips(0U, 1200U), "set first column width") ||
        !require_step(table.set_column_width_twips(1U, 2200U), "set second column width") ||
        !require_step(table.set_column_width_twips(2U, 4400U), "set third column width") ||
        !require_step(table.clear_column_width(1U), "clear second column width once") ||
        !require_step(table.set_column_width_twips(1U, 2200U),
                      "restore second column width") ||
        !require_step(table.set_layout_mode(featherdoc::table_layout_mode::fixed),
                      "reapply fixed layout to normalize cell widths")) {
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
