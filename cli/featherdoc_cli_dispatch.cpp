#include "featherdoc_cli_dispatch.hpp"

#include "featherdoc_cli_bookmark_commands.hpp"
#include "featherdoc_cli_content_control_commands.hpp"
#include "featherdoc_cli_document_content_commands.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_field_commands.hpp"
#include "featherdoc_cli_formatting_commands.hpp"
#include "featherdoc_cli_image_commands.hpp"
#include "featherdoc_cli_numbering_catalog_commands.hpp"
#include "featherdoc_cli_numbering_inspect_commands.hpp"
#include "featherdoc_cli_page_setup_commands.hpp"
#include "featherdoc_cli_paragraph_inspect_commands.hpp"
#include "featherdoc_cli_pdf_commands.hpp"
#include "featherdoc_cli_review_commands.hpp"
#include "featherdoc_cli_run_recipe_commands.hpp"
#include "featherdoc_cli_section_part_commands.hpp"
#include "featherdoc_cli_semantic_diff.hpp"
#include "featherdoc_cli_style_inspect_commands.hpp"
#include "featherdoc_cli_style_numbering_commands.hpp"
#include "featherdoc_cli_style_refactor_commands.hpp"
#include "featherdoc_cli_table_commands.hpp"
#include "featherdoc_cli_template_schema_commands.hpp"
#include "featherdoc_cli_template_table_commands.hpp"

#include <featherdoc.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_featherdoc_cli_command(
    const std::vector<std::string_view> &arguments) -> int {
    const auto command = arguments.front();
    featherdoc::Document doc;

    if (command == "run-recipe") {
        return run_recipe_command(command, arguments);
    }

    if (command == "export-pdf") {
        return run_export_pdf_command(command, arguments, doc);
    }

#if defined(FEATHERDOC_CLI_ENABLE_PDF_IMPORT)
    if (command == "import-pdf") {
        return run_import_pdf_command(command, arguments, doc);
    }
#endif

    if (is_section_part_command(command)) {
        return run_section_part_command(command, arguments, doc);
    }

    if (is_style_inspect_command(command)) {
        return run_style_inspect_command(command, arguments, doc);
    }

    if (is_style_numbering_command(command)) {
        return run_style_numbering_command(command, arguments, doc);
    }

    if (is_table_command(command)) {
        return run_table_command(command, arguments, doc);
    }

    if (is_style_refactor_command(command)) {
        return run_style_refactor_command(command, arguments, doc);
    }

    if (is_numbering_catalog_command(command)) {
        return run_numbering_catalog_command(command, arguments, doc);
    }

    if (is_numbering_inspect_command(command)) {
        return run_numbering_inspect_command(command, arguments, doc);
    }

    if (is_paragraph_inspect_command(command)) {
        return run_paragraph_inspect_command(command, arguments);
    }

    if (is_template_table_command(command)) {
        return run_template_table_command(command, arguments);
    }

    if (is_formatting_command(command)) {
        return run_formatting_command(command, arguments);
    }

    if (is_page_setup_command(command)) {
        return run_page_setup_command(command, arguments, doc);
    }

    if (is_field_command(command)) {
        return run_field_command(command, arguments, doc);
    }

    if (is_bookmark_command(command)) {
        return run_bookmark_command(command, arguments, doc);
    }

    if (is_content_control_command(command)) {
        return run_content_control_command(command, arguments, doc);
    }

    if (is_document_content_command(command)) {
        return run_document_content_command(command, arguments, doc);
    }

    if (is_review_command(command)) {
        return run_review_command(command, arguments, doc);
    }

    if (command == "semantic-diff") {
        return run_semantic_diff_command(command, arguments, doc);
    }

    if (is_image_command(command)) {
        return run_image_command(command, arguments, doc);
    }

    if (is_template_schema_command(command)) {
        return run_template_schema_command(command, arguments, doc);
    }

    print_parse_error("unknown command: " + std::string(command));
    return 2;
}

} // namespace featherdoc_cli
