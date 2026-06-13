#include "featherdoc_cli_command_support.hpp"

#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_section_options_parse.hpp"
#include "featherdoc_cli_table_cell_options_parse.hpp"

#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>

namespace featherdoc_cli {
namespace {

auto read_text_source_value(const std::optional<std::string> &inline_text,
                            const std::optional<path_type> &text_file,
                            std::string_view expected_message,
                            std::string &text,
                            std::string &error_message) -> bool {
    if (inline_text.has_value()) {
        text = *inline_text;
        return true;
    }

    if (!text_file.has_value()) {
        error_message = std::string(expected_message);
        return false;
    }

    std::ifstream stream(*text_file, std::ios::binary);
    if (!stream.good()) {
        error_message = "failed to read text file: " + text_file->string();
        return false;
    }

    text.assign(std::istreambuf_iterator<char>(stream),
                std::istreambuf_iterator<char>());
    text = strip_utf8_bom(std::move(text));
    return true;
}

} // namespace

auto save_document(featherdoc::Document &doc,
                   const std::optional<path_type> &output_path,
                   std::string_view command, bool json_output) -> bool {
    const auto error =
        output_path.has_value() ? doc.save_as(*output_path) : doc.save();
    if (error) {
        return report_document_error(command, "save", doc.last_error(), json_output);
    }

    return true;
}

auto open_document(const path_type &input_path, featherdoc::Document &doc,
                   std::string_view command, bool json_output) -> bool {
    doc.set_path(input_path);
    const auto error = doc.open();
    if (error) {
        return report_document_error(command, "open", doc.last_error(), json_output);
    }

    return true;
}

auto resolve_body_table(featherdoc::Document &doc, std::size_t table_index,
                        featherdoc::Table &table, std::string_view command,
                        bool json_output, std::string_view stage) -> bool {
    table = doc.tables();
    for (std::size_t current_index = 0;
         current_index < table_index && table.has_next(); ++current_index) {
        table.next();
    }

    if (!table.has_next()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail =
            "table index '" + std::to_string(table_index) + "' is out of range";
        error_info.entry_name = "word/document.xml";
        return report_operation_failure(command, stage,
                                        "table index is out of range", error_info,
                                        json_output);
    }

    return true;
}

auto resolve_body_table_row(featherdoc::Document &doc, std::size_t table_index,
                            std::size_t row_index, featherdoc::TableRow &row,
                            std::string_view command, bool json_output,
                            std::string_view stage) -> bool {
    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command, json_output,
                            stage)) {
        return false;
    }

    row = table.rows();
    for (std::size_t current_index = 0;
         current_index < row_index && row.has_next(); ++current_index) {
        row.next();
    }

    if (!row.has_next()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail =
            "row index '" + std::to_string(row_index) +
            "' is out of range for table index '" + std::to_string(table_index) +
            "'";
        error_info.entry_name = "word/document.xml";
        return report_operation_failure(command, stage,
                                        "row index is out of range", error_info,
                                        json_output);
    }

    return true;
}

auto resolve_body_table_cell(featherdoc::Document &doc, std::size_t table_index,
                             std::size_t row_index, std::size_t cell_index,
                             featherdoc::TableCell &cell,
                             std::string_view command, bool json_output)
    -> bool {
    featherdoc::TableRow row;
    if (!resolve_body_table_row(doc, table_index, row_index, row, command,
                                json_output)) {
        return false;
    }

    cell = row.cells();
    for (std::size_t current_index = 0;
         current_index < cell_index && cell.has_next(); ++current_index) {
        cell.next();
    }

    if (cell.has_next()) {
        return true;
    }

    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail =
        "cell index '" + std::to_string(cell_index) +
        "' is out of range for row index '" + std::to_string(row_index) +
        "' in table index '" + std::to_string(table_index) + "'";
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate",
                                    "cell index is out of range", error_info,
                                    json_output);
}

auto read_text_source(const cli_text_source_options &options, std::string &text,
                      std::string &error_message) -> bool {
    return read_text_source_value(options.text, options.text_file,
                                  "expected text input", text, error_message);
}

auto read_text_source(const section_text_options &options, std::string &text,
                      std::string &error_message) -> bool {
    return read_text_source_value(options.text, options.text_file,
                                  "expected --text <text> or --text-file <path>",
                                  text, error_message);
}

auto read_text_source(const table_cell_text_options &options, std::string &text,
                      std::string &error_message) -> bool {
    return read_text_source_value(options.text, options.text_file,
                                  "expected --text <text> or --text-file <path>",
                                  text, error_message);
}

void write_json_mutation_result(std::string_view command, featherdoc::Document &doc,
                                const std::optional<path_type> &output_path) {
    write_json_mutation_result(command, doc, output_path,
                               [](std::ostream &) {});
}

} // namespace featherdoc_cli
