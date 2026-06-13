#pragma once

#include "featherdoc_cli_command_support.hpp"

#if defined(FEATHERDOC_CLI_ENABLE_PDF_IMPORT)
#include <featherdoc/pdf/pdf_document_importer.hpp>
#endif

#include <iosfwd>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

#if defined(FEATHERDOC_CLI_ENABLE_PDF_IMPORT)
[[nodiscard]] auto pdf_import_failure_kind_name(
    featherdoc::pdf::PdfDocumentImportFailureKind failure_kind) -> std::string_view;

void write_pdf_table_continuation_diagnostics_json(
    std::ostream &stream,
    const std::vector<featherdoc::pdf::PdfTableContinuationDiagnostic>
        &diagnostics);

void print_pdf_import_failure(
    std::string_view command, const path_type &input_path,
    const path_type &output_path,
    const featherdoc::pdf::PdfDocumentImportResult &result, bool json_output);
#endif

} // namespace featherdoc_cli
