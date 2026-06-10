#include "featherdoc_cli_template_table_json_batch_operation_parse_detail.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_template_table_json_patch_parse_detail.hpp"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace featherdoc_cli::detail {

namespace {

auto resolve_header_cells(
    template_table_json_batch_operation_parse_state &&state,
    std::vector<std::string> &header_cells, std::string &error_message)
    -> bool {
    if (state.saw_header_cells && state.saw_header_cell_texts &&
        state.header_cells_value != state.header_cell_texts_value) {
        error_message = "JSON patch members 'header_cells' and "
                        "'header_cell_texts' must match when both are present";
        return false;
    }
    if (state.saw_header_cells) {
        header_cells = std::move(state.header_cells_value);
    } else if (state.saw_header_cell_texts) {
        header_cells = std::move(state.header_cell_texts_value);
    }
    return true;
}

} // namespace

auto finalize_template_table_json_batch_operation(
    template_table_json_batch_operation_parse_state &&state,
    template_table_json_batch_operation &operation, std::string &error_message)
    -> bool {
    std::optional<std::string> resolved_bookmark_name;
    if (!resolve_json_patch_string_member(state.bookmark_value,
                                          state.bookmark_name_value,
                                          "bookmark", "bookmark_name",
                                          resolved_bookmark_name,
                                          error_message)) {
        return false;
    }

    std::optional<std::string> resolved_after_text;
    if (!resolve_json_patch_string_member(state.after_text_value,
                                          state.after_paragraph_text_value,
                                          "after_text",
                                          "after_paragraph_text",
                                          resolved_after_text,
                                          error_message)) {
        return false;
    }

    std::vector<std::string> resolved_header_cells;
    if (!resolve_header_cells(std::move(state), resolved_header_cells,
                              error_message)) {
        return false;
    }

    std::optional<std::size_t> resolved_header_row_index;
    if (!resolve_json_patch_index_member(state.header_row_value,
                                         state.header_row_index_value,
                                         "header_row", "header_row_index",
                                         resolved_header_row_index,
                                         error_message)) {
        return false;
    }

    featherdoc::template_table_selector selector{};
    selector.table_index = state.table_index_value;
    selector.bookmark_name = std::move(resolved_bookmark_name);
    selector.after_paragraph_text = std::move(resolved_after_text);
    selector.header_cell_texts = std::move(resolved_header_cells);
    if (resolved_header_row_index.has_value()) {
        selector.header_row_index = *resolved_header_row_index;
    }
    if (state.occurrence_value.has_value()) {
        selector.occurrence = *state.occurrence_value;
    }

    if (!validate_template_table_selector(
            selector, false, resolved_header_row_index.has_value(),
            state.occurrence_value.has_value(), error_message)) {
        return false;
    }

    std::optional<std::size_t> resolved_part_index;
    if (!resolve_json_patch_index_member(state.index_value,
                                         state.part_index_value, "index",
                                         "part_index", resolved_part_index,
                                         error_message)) {
        return false;
    }

    std::optional<std::size_t> resolved_section_index;
    if (!resolve_json_patch_index_member(state.section_value,
                                         state.section_index_value, "section",
                                         "section_index",
                                         resolved_section_index,
                                         error_message)) {
        return false;
    }

    template_table_json_patch patch;
    if (!finalize_template_table_json_patch(
            state.mode_value, state.start_row_value, state.start_row_index_value,
            state.start_cell_value, state.start_cell_index_value,
            std::move(state.rows), state.saw_rows, patch, error_message)) {
        return false;
    }

    operation.selector = std::move(selector);
    operation.part = state.part_value;
    operation.part_index = resolved_part_index;
    operation.section_index = resolved_section_index;
    operation.reference_kind = state.reference_kind_value;
    operation.patch = std::move(patch);
    return true;
}

} // namespace featherdoc_cli::detail
