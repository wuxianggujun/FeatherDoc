#include <featherdoc.hpp>

#include <array>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace fs = std::filesystem;

namespace {

constexpr auto sample_style_id = "TableGrid";

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

bool fill_seed_table(featherdoc::Table table) {
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_style_id(sample_style_id) ||
        !table.set_layout_mode(featherdoc::table_layout_mode::fixed) ||
        !table.set_alignment(featherdoc::table_alignment::center)) {
        return false;
    }

    const std::array<std::array<const char *, 3>, 4> rows = {{
        {"Stage", "Owner", "Status"},
        {"Plan", "Avery", "Queued"},
        {"Build", "Noah", "Running"},
        {"Ship", "Mila", "Blocked"},
    }};

    auto row = table.rows();
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index == 0U ? 520U : 420U,
                                  featherdoc::row_height_rule::at_least)) {
            return false;
        }

        auto cell = row.cells();
        for (const auto *text : rows[row_index]) {
            if (!cell.has_next() || !cell.set_text(text)) {
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
                                                "edit_existing_table_style_look.docx";

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

    auto intro = seed.paragraphs();
    if (!intro.has_next() ||
        !intro.set_text("Seed a styled table, reopen it, then retune tblLook flags while"
                        " keeping the existing style reference in place.")) {
        std::cerr << "failed to seed intro paragraph\n";
        return 1;
    }

    if (!fill_seed_table(seed.append_table(4, 3))) {
        std::cerr << "failed to seed styled table\n";
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

    intro = doc.paragraphs();
    if (!intro.has_next() ||
        !intro.add_run(
                  " The reopened table now writes a different tblLook combination for the same"
                  " style, so XML-level style routing can be updated without rebuilding the"
                  " table.")
              .has_next()) {
        std::cerr << "failed to update intro paragraph\n";
        return 1;
    }

    auto table = doc.tables();
    if (!table.has_next()) {
        std::cerr << "missing target table\n";
        return 1;
    }

    featherdoc::table_style_look style_look{};
    style_look.first_row = true;
    style_look.last_row = true;
    style_look.first_column = false;
    style_look.last_column = false;
    style_look.banded_rows = false;
    style_look.banded_columns = true;
    if (!table.set_style_look(style_look)) {
        std::cerr << "failed to update table style look\n";
        return 1;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0; row_index < 3U; ++row_index) {
        if (!row.has_next()) {
            std::cerr << "table row traversal failed\n";
            return 1;
        }
        row.next();
    }
    if (!row.has_next()) {
        std::cerr << "missing last row\n";
        return 1;
    }

    auto last_cell = row.cells();
    if (!last_cell.has_next() || !last_cell.set_text("Ship total")) {
        std::cerr << "failed to update last-row cell 1\n";
        return 1;
    }
    last_cell.next();
    if (!last_cell.has_next() || !last_cell.set_text("Team")) {
        std::cerr << "failed to update last-row cell 2\n";
        return 1;
    }
    last_cell.next();
    if (!last_cell.has_next() || !last_cell.set_text("Ready after review")) {
        std::cerr << "failed to update last-row cell 3\n";
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
