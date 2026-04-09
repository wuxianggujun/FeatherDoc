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

bool fill_table(featherdoc::Table table) {
    if (!table.has_next() || !table.set_width_twips(6400U) ||
        !table.set_layout_mode(featherdoc::table_layout_mode::fixed) ||
        !table.set_alignment(featherdoc::table_alignment::center)) {
        return false;
    }

    const std::array<std::array<const char *, 2>, 2> texts = {{
        {"Backlog", "In review"},
        {"Scheduled", "Released"},
    }};
    const std::array<std::array<const char *, 2>, 2> fills = {{
        {"D9EAF7", "E2F0D9"},
        {"FCE4D6", "FFF2CC"},
    }};

    auto row = table.rows();
    for (std::size_t row_index = 0; row_index < texts.size(); ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(900U, featherdoc::row_height_rule::exact)) {
            return false;
        }

        auto cell = row.cells();
        for (std::size_t cell_index = 0; cell_index < texts[row_index].size(); ++cell_index) {
            if (!cell.has_next() || !cell.set_fill_color(fills[row_index][cell_index]) ||
                !cell.set_width_twips(3200U) ||
                !cell.set_vertical_alignment(
                    featherdoc::cell_vertical_alignment::center) ||
                !cell.set_text(texts[row_index][cell_index])) {
                return false;
            }

            cell.next();
        }

        row.next();
    }

    return true;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path = argc > 1 ? fs::path(argv[1])
                                          : fs::current_path() /
                                                "edit_existing_table_spacing.docx";

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
        !paragraph.set_text("Seed a compact table, reopen it, then add visible cell spacing.")) {
        std::cerr << "failed to seed intro paragraph\n";
        return 1;
    }

    if (!fill_table(seed.append_table(2, 2))) {
        std::cerr << "failed to seed table\n";
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

    auto intro = doc.paragraphs();
    if (!intro.has_next() ||
        !intro.add_run(" The reopened table now uses tblCellSpacing for visual gutters.")
              .has_next()) {
        std::cerr << "failed to update intro paragraph\n";
        return 1;
    }

    auto table = doc.tables();
    if (!table.has_next() || !table.set_cell_spacing_twips(160U)) {
        std::cerr << "failed to set table cell spacing\n";
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
