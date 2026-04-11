#include <featherdoc.hpp>

#include <array>
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

bool require_table_column_width(featherdoc::Table table, std::size_t column_index,
                                std::uint32_t expected_width,
                                std::string_view operation) {
    const auto width = table.column_width_twips(column_index);
    if (width.has_value() && *width == expected_width) {
        return true;
    }

    std::cerr << operation << " failed";
    if (!width.has_value()) {
        std::cerr << ": width is missing";
    } else {
        std::cerr << ": expected " << expected_width << ", got " << *width;
    }
    std::cerr << '\n';
    return false;
}

bool require_cell_width(featherdoc::TableCell cell, std::uint32_t expected_width,
                        std::string_view operation) {
    const auto width = cell.width_twips();
    if (width.has_value() && *width == expected_width) {
        return true;
    }

    std::cerr << operation << " failed";
    if (!width.has_value()) {
        std::cerr << ": width is missing";
    } else {
        std::cerr << ": expected " << expected_width << ", got " << *width;
    }
    std::cerr << '\n';
    return false;
}

bool require_row_height(featherdoc::TableRow row, std::uint32_t expected_height,
                        std::string_view operation) {
    const auto height = row.height_twips();
    if (height.has_value() && *height == expected_height) {
        return true;
    }

    std::cerr << operation << " failed";
    if (!height.has_value()) {
        std::cerr << ": height is missing";
    } else {
        std::cerr << ": expected " << expected_height << ", got " << *height;
    }
    std::cerr << '\n';
    return false;
}

bool require_empty_text(featherdoc::TableCell cell, std::string_view operation) {
    if (cell.get_text().empty()) {
        return true;
    }

    std::cerr << operation << " failed: expected cleared continuation cell\n";
    return false;
}

bool configure_seed_table(featherdoc::Table table) {
    return table.has_next() && table.set_width_twips(7000U) &&
           table.set_layout_mode(featherdoc::table_layout_mode::fixed) &&
           table.set_alignment(featherdoc::table_alignment::center) &&
           table.set_style_id("TableGrid") && table.set_column_width_twips(0U, 1000U) &&
           table.set_column_width_twips(1U, 1900U) &&
           table.set_column_width_twips(2U, 4100U);
}

bool set_row_values(featherdoc::TableRow row,
                    const std::array<std::string_view, 3> &texts,
                    const std::array<std::string_view, 3> &fills,
                    std::uint32_t row_height) {
    if (!row.has_next() ||
        !row.set_height_twips(row_height, featherdoc::row_height_rule::at_least)) {
        return false;
    }

    auto cell = row.cells();
    for (std::size_t index = 0; index < texts.size(); ++index) {
        if (!cell.has_next() ||
            !cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center) ||
            !cell.set_fill_color(std::string{fills[index]}) ||
            !cell.set_text(std::string{texts[index]})) {
            return false;
        }
        cell.next();
    }

    return true;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1]) : fs::current_path() / "merge_down_fixed_grid.docx";

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
        !paragraph.set_text("Fixed-grid merge_down sample")) {
        std::cerr << "failed to seed intro paragraph\n";
        return 1;
    }
    if (!paragraph
             .add_run(
                 " Seed a two-row fixed-layout table, reopen it, merge the first-column "
                 "anchor downward, and verify that the merged orange pillar stays locked to "
                 "the same 1000-twip grid column while the 1900/4100 neighbor columns remain "
                 "unchanged.")
             .has_next()) {
        std::cerr << "failed to extend intro paragraph\n";
        return 1;
    }

    auto table = seed.append_table(2, 3);
    if (!require_step(configure_seed_table(table), "configure seed table")) {
        return 1;
    }

    if (!require_step(
            set_row_values(table.rows(),
                           {"Anchor 1000", "Middle 1900", "Tail 4100"},
                           {"FCE4D6", "FFF2CC", "E2F0D9"}, 760U),
            "fill seed top row")) {
        return 1;
    }

    auto lower_row = table.rows();
    lower_row.next();
    if (!require_step(
            set_row_values(lower_row,
                           {"Continuation 1000", "1900 stays aligned",
                            "4100 stays widest"},
                           {"FCE4D6", "FFF2CC", "E2F0D9"}, 1040U),
            "fill seed lower row")) {
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
             .add_run(
                 " Final visual target: the merged orange pillar should remain the narrow "
                 "1000-twip first column across both rows, clearly slimmer than the yellow "
                 "1900-twip column and the green 4100-twip tail column.")
             .has_next()) {
        std::cerr << "failed to update intro paragraph after reopen\n";
        return 1;
    }

    table = doc.tables();
    if (!table.has_next() ||
        !require_table_column_width(table, 0U, 1000U, "verify reopened first grid width") ||
        !require_table_column_width(table, 1U, 1900U, "verify reopened second grid width") ||
        !require_table_column_width(table, 2U, 4100U, "verify reopened third grid width")) {
        return 1;
    }

    auto top_row = table.rows();
    if (!top_row.has_next() ||
        !require_row_height(top_row, 760U, "verify reopened top row height")) {
        return 1;
    }

    auto merged_anchor = top_row.cells();
    if (!merged_anchor.has_next() ||
        !require_cell_width(merged_anchor, 1000U, "verify reopened anchor width") ||
        !require_step(merged_anchor.merge_down(1U), "merge_down after reopen") ||
        !require_cell_width(merged_anchor, 1000U, "verify merged anchor width") ||
        !require_step(merged_anchor.set_fill_color("FCE4D6"), "recolor merged anchor") ||
        !require_step(merged_anchor.set_text("Merged orange pillar = 1000"),
                      "set merged anchor text")) {
        return 1;
    }

    auto top_middle = merged_anchor;
    top_middle.next();
    if (!top_middle.has_next() ||
        !require_cell_width(top_middle, 1900U, "verify top middle width") ||
        !require_step(top_middle.set_fill_color("FFF2CC"), "recolor top middle") ||
        !require_step(top_middle.set_text("Middle grid column = 1900"),
                      "set top middle text")) {
        return 1;
    }

    auto top_tail = top_middle;
    top_tail.next();
    if (!top_tail.has_next() ||
        !require_cell_width(top_tail, 4100U, "verify top tail width") ||
        !require_step(top_tail.set_fill_color("E2F0D9"), "recolor top tail") ||
        !require_step(top_tail.set_text("Tail grid column = 4100"),
                      "set top tail text")) {
        return 1;
    }

    lower_row = table.rows();
    lower_row.next();
    if (!lower_row.has_next() ||
        !require_row_height(lower_row, 1040U, "verify reopened lower row height")) {
        return 1;
    }

    auto continuation = lower_row.cells();
    if (!continuation.has_next() ||
        !require_cell_width(continuation, 1000U, "verify continuation width after merge") ||
        !require_empty_text(continuation, "verify continuation text cleared")) {
        return 1;
    }

    auto lower_middle = continuation;
    lower_middle.next();
    if (!lower_middle.has_next() ||
        !require_cell_width(lower_middle, 1900U, "verify lower middle width")) {
        return 1;
    }

    auto lower_tail = lower_middle;
    lower_tail.next();
    if (!lower_tail.has_next() ||
        !require_cell_width(lower_tail, 4100U, "verify lower tail width")) {
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
