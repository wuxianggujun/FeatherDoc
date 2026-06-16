#include "featherdoc_cli_pdf_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_pdf_import_output.hpp"
#include "featherdoc_cli_pdf_parse.hpp"

#if defined(FEATHERDOC_CLI_ENABLE_PDF_IMPORT)
#include <featherdoc/pdf/pdf_document_importer.hpp>
#endif

#include <iostream>
#include <ostream>
#include <string>
#include <vector>

namespace featherdoc_cli {

#if defined(FEATHERDOC_CLI_ENABLE_PDF_IMPORT)
auto run_import_pdf_command(std::string_view command,
                            const std::vector<std::string_view> &arguments,
                            featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "import-pdf expects an input path",
                          json_output);
        return 2;
    }

    import_pdf_options options;
    std::string error_message;
    if (!parse_import_pdf_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::pdf::PdfDocumentImportOptions import_options;
    import_options.import_table_candidates_as_tables =
        options.import_table_candidates_as_tables;
    if (options.min_table_continuation_confidence.has_value()) {
        import_options.min_table_continuation_confidence =
            *options.min_table_continuation_confidence;
    }

    const auto input_path = path_type(std::string(arguments[1]));
    const auto import_result = featherdoc::pdf::import_pdf_text_document(
        input_path, doc, import_options);
    if (!import_result) {
        print_pdf_import_failure(command, input_path, *options.output_path,
                                 import_result, options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command,
                       options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&input_path, &options, &import_result](std::ostream &stream) {
                stream << ",\"input\":";
                write_json_string(stream, input_path.string());
                stream << ",\"output\":";
                write_json_string(stream, options.output_path->string());
                stream << ",\"paragraphs_imported\":"
                       << import_result.paragraphs_imported;
                stream << ",\"tables_imported\":"
                       << import_result.tables_imported;
                stream << ",\"table_continuation_diagnostics_count\":"
                       << import_result.table_continuation_diagnostics.size();
                stream << ",\"table_continuation_diagnostics\":";
                write_pdf_table_continuation_diagnostics_json(
                    stream, import_result.table_continuation_diagnostics);
                stream << ",\"import_table_candidates_as_tables\":"
                       << json_bool(options.import_table_candidates_as_tables);
                if (options.min_table_continuation_confidence.has_value()) {
                    stream << ",\"min_table_continuation_confidence\":"
                           << *options.min_table_continuation_confidence;
                }
            });
    } else {
        std::cout << "imported " << input_path.string() << " -> "
                  << options.output_path->string() << " ("
                  << import_result.paragraphs_imported << " paragraphs, "
                  << import_result.tables_imported << " tables)\n";
    }

    return 0;
}
#endif

} // namespace featherdoc_cli
