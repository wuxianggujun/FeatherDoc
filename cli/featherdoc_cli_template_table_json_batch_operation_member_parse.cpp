#include "featherdoc_cli_template_table_json_batch_operation_parse_detail.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_table_json_patch_parse_detail.hpp"

#include <string>
#include <string_view>

namespace featherdoc_cli::detail {

namespace {

auto parse_unique_string_member(std::string_view content, std::size_t &index,
                                std::optional<std::string> &value,
                                std::string_view member_name,
                                std::string &error_message) -> bool {
    if (value.has_value()) {
        error_message = "JSON patch member '" + std::string(member_name) +
                        "' must not be duplicated";
        return false;
    }
    skip_json_patch_whitespace(content, index);
    value.emplace();
    return parse_json_patch_string(content, index, *value, error_message);
}

auto parse_unique_index_member(std::string_view content, std::size_t &index,
                               std::optional<std::size_t> &value,
                               std::string_view member_name,
                               std::string &error_message) -> bool {
    if (value.has_value()) {
        error_message = "JSON patch member '" + std::string(member_name) +
                        "' must not be duplicated";
        return false;
    }
    std::size_t parsed_index = 0U;
    if (!parse_json_patch_index_value(content, index, parsed_index,
                                      member_name, error_message)) {
        return false;
    }
    value = parsed_index;
    return true;
}

} // namespace

auto parse_template_table_json_batch_operation_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    template_table_json_batch_operation_parse_state &state,
    std::string &error_message) -> bool {
    if (member_name == "bookmark") {
        return parse_unique_string_member(content, index, state.bookmark_value,
                                          member_name, error_message);
    }
    if (member_name == "bookmark_name") {
        return parse_unique_string_member(
            content, index, state.bookmark_name_value, member_name,
            error_message);
    }
    if (member_name == "table_index") {
        return parse_unique_index_member(content, index, state.table_index_value,
                                         member_name, error_message);
    }
    if (member_name == "after_text") {
        return parse_unique_string_member(content, index, state.after_text_value,
                                          member_name, error_message);
    }
    if (member_name == "after_paragraph_text") {
        return parse_unique_string_member(
            content, index, state.after_paragraph_text_value, member_name,
            error_message);
    }
    if (member_name == "header_cells") {
        if (state.saw_header_cells) {
            error_message =
                "JSON patch member 'header_cells' must not be duplicated";
            return false;
        }
        if (!parse_json_patch_text_array(content, index,
                                         state.header_cells_value,
                                         "header_cells", error_message)) {
            return false;
        }
        state.saw_header_cells = true;
        return true;
    }
    if (member_name == "header_cell_texts") {
        if (state.saw_header_cell_texts) {
            error_message =
                "JSON patch member 'header_cell_texts' must not be duplicated";
            return false;
        }
        if (!parse_json_patch_text_array(content, index,
                                         state.header_cell_texts_value,
                                         "header_cell_texts", error_message)) {
            return false;
        }
        state.saw_header_cell_texts = true;
        return true;
    }
    if (member_name == "header_row") {
        return parse_unique_index_member(content, index, state.header_row_value,
                                         member_name, error_message);
    }
    if (member_name == "header_row_index") {
        return parse_unique_index_member(
            content, index, state.header_row_index_value, member_name,
            error_message);
    }
    if (member_name == "occurrence") {
        return parse_unique_index_member(content, index, state.occurrence_value,
                                         member_name, error_message);
    }
    if (member_name == "part") {
        if (state.part_value.has_value()) {
            error_message = "JSON patch member 'part' must not be duplicated";
            return false;
        }
        validation_part_family parsed_part = validation_part_family::body;
        if (!parse_json_patch_part_value(content, index, parsed_part,
                                         error_message)) {
            return false;
        }
        state.part_value = parsed_part;
        return true;
    }
    if (member_name == "index") {
        return parse_unique_index_member(content, index, state.index_value,
                                         member_name, error_message);
    }
    if (member_name == "part_index") {
        return parse_unique_index_member(content, index, state.part_index_value,
                                         member_name, error_message);
    }
    if (member_name == "section") {
        return parse_unique_index_member(content, index, state.section_value,
                                         member_name, error_message);
    }
    if (member_name == "section_index") {
        return parse_unique_index_member(
            content, index, state.section_index_value, member_name,
            error_message);
    }
    if (member_name == "kind") {
        if (state.reference_kind_value.has_value()) {
            error_message = "JSON patch member 'kind' must not be duplicated";
            return false;
        }
        featherdoc::section_reference_kind parsed_reference_kind =
            featherdoc::section_reference_kind::default_reference;
        if (!parse_json_patch_reference_kind_value(
                content, index, parsed_reference_kind, error_message)) {
            return false;
        }
        state.reference_kind_value = parsed_reference_kind;
        return true;
    }
    if (member_name == "mode") {
        return parse_unique_string_member(content, index, state.mode_value,
                                          member_name, error_message);
    }
    if (member_name == "start_row") {
        return parse_unique_index_member(content, index, state.start_row_value,
                                         member_name, error_message);
    }
    if (member_name == "start_row_index") {
        return parse_unique_index_member(
            content, index, state.start_row_index_value, member_name,
            error_message);
    }
    if (member_name == "start_cell") {
        return parse_unique_index_member(content, index, state.start_cell_value,
                                         member_name, error_message);
    }
    if (member_name == "start_cell_index") {
        return parse_unique_index_member(
            content, index, state.start_cell_index_value, member_name,
            error_message);
    }
    if (member_name == "rows") {
        if (state.saw_rows) {
            error_message = "JSON patch member 'rows' must not be duplicated";
            return false;
        }
        if (!parse_json_patch_rows_matrix(content, index, state.rows,
                                          error_message)) {
            return false;
        }
        state.saw_rows = true;
        return true;
    }
    return skip_json_patch_value(content, index, error_message);
}

} // namespace featherdoc_cli::detail
