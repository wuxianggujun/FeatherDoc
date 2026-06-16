#include "featherdoc_cli_template_table_output.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"

#include <iostream>
#include <ostream>

namespace featherdoc_cli {

namespace {

void write_json_row_texts_matrix(
    std::ostream &stream, const std::vector<std::vector<std::string>> &rows) {
    stream << '[';
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        if (row_index != 0U) {
            stream << ',';
        }
        stream << '[';
        for (std::size_t cell_index = 0; cell_index < rows[row_index].size();
             ++cell_index) {
            if (cell_index != 0U) {
                stream << ',';
            }
            write_json_string(stream, rows[row_index][cell_index]);
        }
        stream << ']';
    }
    stream << ']';
}

} // namespace

void write_json_template_table_row_texts_result(
    std::ostream &stream, const selected_template_part &selected,
    std::size_t table_index, std::size_t start_row_index,
    const std::vector<std::vector<std::string>> &rows,
    const std::optional<std::string> &bookmark_name) {
    stream << ',';
    write_json_selected_template_part(stream, selected);
    if (bookmark_name.has_value()) {
        stream << ",\"bookmark_name\":";
        write_json_string(stream, *bookmark_name);
    }
    stream << ",\"table_index\":" << table_index
           << ",\"start_row_index\":" << start_row_index
           << ",\"row_count\":" << rows.size() << ",\"rows\":";
    write_json_row_texts_matrix(stream, rows);
}

void print_template_table_row_texts_result(
    const selected_template_part &selected, std::size_t table_index,
    std::size_t start_row_index, const std::vector<std::vector<std::string>> &rows,
    const std::optional<std::string> &bookmark_name,
    const std::optional<path_type> &output_path) {
    print_selected_template_part(std::cout, selected);
    if (bookmark_name.has_value()) {
        std::cout << "bookmark_name: " << *bookmark_name << '\n';
    }
    std::cout << "table_index: " << table_index << '\n'
              << "start_row_index: " << start_row_index << '\n'
              << "row_count: " << rows.size() << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    for (std::size_t offset = 0U; offset < rows.size(); ++offset) {
        std::cout << "row[" << (start_row_index + offset) << "]: ";
        for (std::size_t cell_index = 0; cell_index < rows[offset].size();
             ++cell_index) {
            if (cell_index != 0U) {
                std::cout << '\t';
            }
            std::cout << rows[offset][cell_index];
        }
        std::cout << '\n';
    }
}

void write_json_template_table_cell_block_texts_result(
    std::ostream &stream, const selected_template_part &selected,
    std::size_t table_index, std::size_t start_row_index,
    std::size_t start_cell_index, const std::vector<std::vector<std::string>> &rows,
    const std::optional<std::string> &bookmark_name) {
    stream << ',';
    write_json_selected_template_part(stream, selected);
    if (bookmark_name.has_value()) {
        stream << ",\"bookmark_name\":";
        write_json_string(stream, *bookmark_name);
    }
    stream << ",\"table_index\":" << table_index
           << ",\"start_row_index\":" << start_row_index
           << ",\"start_cell_index\":" << start_cell_index
           << ",\"row_count\":" << rows.size() << ",\"rows\":";
    write_json_row_texts_matrix(stream, rows);
}

void print_template_table_cell_block_texts_result(
    const selected_template_part &selected, std::size_t table_index,
    std::size_t start_row_index, std::size_t start_cell_index,
    const std::vector<std::vector<std::string>> &rows,
    const std::optional<std::string> &bookmark_name,
    const std::optional<path_type> &output_path) {
    print_selected_template_part(std::cout, selected);
    if (bookmark_name.has_value()) {
        std::cout << "bookmark_name: " << *bookmark_name << '\n';
    }
    std::cout << "table_index: " << table_index << '\n'
              << "start_row_index: " << start_row_index << '\n'
              << "start_cell_index: " << start_cell_index << '\n'
              << "row_count: " << rows.size() << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    for (std::size_t offset = 0U; offset < rows.size(); ++offset) {
        std::cout << "row[" << (start_row_index + offset)
                  << "] from cell[" << start_cell_index << "]: ";
        for (std::size_t cell_offset = 0U; cell_offset < rows[offset].size();
             ++cell_offset) {
            if (cell_offset != 0U) {
                std::cout << '\t';
            }
            std::cout << rows[offset][cell_offset];
        }
        std::cout << '\n';
    }
}

auto template_table_json_patch_mode_name(template_table_json_patch_mode mode)
    -> std::string_view {
    switch (mode) {
    case template_table_json_patch_mode::rows:
        return "rows";
    case template_table_json_patch_mode::block:
        return "block";
    }

    return "rows";
}

void write_json_template_table_selector_fields(
    std::ostream &stream, const featherdoc::template_table_selector &selector) {
    if (selector.bookmark_name.has_value()) {
        stream << ",\"bookmark_name\":";
        write_json_string(stream, *selector.bookmark_name);
    }
    if (selector.after_paragraph_text.has_value()) {
        stream << ",\"after_text\":";
        write_json_string(stream, *selector.after_paragraph_text);
    }
    if (!selector.header_cell_texts.empty()) {
        stream << ",\"header_cells\":";
        write_json_strings(stream, selector.header_cell_texts);
        stream << ",\"header_row_index\":" << selector.header_row_index;
    }
    if (template_table_selector_uses_text_matching(selector)) {
        stream << ",\"occurrence\":" << selector.occurrence;
    }
}

void print_template_table_selector_fields(
    std::ostream &stream, const featherdoc::template_table_selector &selector) {
    if (selector.bookmark_name.has_value()) {
        stream << "bookmark_name: " << *selector.bookmark_name << '\n';
    }
    if (selector.after_paragraph_text.has_value()) {
        stream << "after_text: " << *selector.after_paragraph_text << '\n';
    }
    if (!selector.header_cell_texts.empty()) {
        stream << "header_row_index: " << selector.header_row_index << '\n'
               << "header_cells: ";
        for (std::size_t index = 0U; index < selector.header_cell_texts.size();
             ++index) {
            if (index != 0U) {
                stream << '\t';
            }
            stream << selector.header_cell_texts[index];
        }
        stream << '\n';
    }
    if (template_table_selector_uses_text_matching(selector)) {
        stream << "occurrence: " << selector.occurrence << '\n';
    }
}

void write_json_template_table_from_json_result(
    std::ostream &stream, const selected_template_part &selected,
    std::size_t table_index, const template_table_json_patch &patch,
    const featherdoc::template_table_selector &selector) {
    stream << ',';
    write_json_selected_template_part(stream, selected);
    write_json_template_table_selector_fields(stream, selector);
    stream << ",\"mode\":";
    write_json_string(
        stream, std::string(template_table_json_patch_mode_name(patch.mode)));
    stream << ",\"table_index\":" << table_index
           << ",\"start_row_index\":" << patch.start_row_index;
    if (patch.mode == template_table_json_patch_mode::block) {
        stream << ",\"start_cell_index\":" << patch.start_cell_index;
    }
    stream << ",\"row_count\":" << patch.rows.size() << ",\"rows\":";
    write_json_row_texts_matrix(stream, patch.rows);
}

void print_template_table_from_json_result(
    const selected_template_part &selected, std::size_t table_index,
    const template_table_json_patch &patch,
    const featherdoc::template_table_selector &selector,
    const std::optional<path_type> &output_path) {
    print_selected_template_part(std::cout, selected);
    print_template_table_selector_fields(std::cout, selector);
    std::cout << "mode: " << template_table_json_patch_mode_name(patch.mode)
              << '\n'
              << "table_index: " << table_index << '\n'
              << "start_row_index: " << patch.start_row_index << '\n';
    if (patch.mode == template_table_json_patch_mode::block) {
        std::cout << "start_cell_index: " << patch.start_cell_index << '\n';
    }
    std::cout << "row_count: " << patch.rows.size() << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }

    for (std::size_t offset = 0U; offset < patch.rows.size(); ++offset) {
        if (patch.mode == template_table_json_patch_mode::block) {
            std::cout << "row[" << (patch.start_row_index + offset)
                      << "] from cell[" << patch.start_cell_index << "]: ";
        } else {
            std::cout << "row[" << (patch.start_row_index + offset) << "]: ";
        }
        for (std::size_t cell_index = 0U; cell_index < patch.rows[offset].size();
             ++cell_index) {
            if (cell_index != 0U) {
                std::cout << '\t';
            }
            std::cout << patch.rows[offset][cell_index];
        }
        std::cout << '\n';
    }
}

void write_json_template_tables_from_json_result(
    std::ostream &stream,
    const std::vector<applied_template_table_json_batch_operation> &operations) {
    stream << ",\"operation_count\":" << operations.size() << ",\"operations\":[";
    for (std::size_t index = 0U; index < operations.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }

        const auto &operation = operations[index];
        stream << "{\"operation_index\":" << operation.operation_index;
        write_json_template_table_from_json_result(
            stream, operation.selected, operation.table_index, operation.patch,
            operation.selector);
        stream << '}';
    }
    stream << ']';
}

void print_template_tables_from_json_result(
    const std::vector<applied_template_table_json_batch_operation> &operations,
    const std::optional<path_type> &output_path) {
    std::cout << "operation_count: " << operations.size() << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }

    for (std::size_t index = 0U; index < operations.size(); ++index) {
        if (index != 0U) {
            std::cout << '\n';
        }

        std::cout << "operation_index: " << operations[index].operation_index
                  << '\n';
        print_template_table_from_json_result(
            operations[index].selected, operations[index].table_index,
            operations[index].patch, operations[index].selector,
            std::nullopt);
    }
}

} // namespace featherdoc_cli
