#include <sstream>
#include <string>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_usage.hpp"

TEST_CASE("cli print_usage includes core command families") {
    std::ostringstream stream;
    featherdoc_cli::print_usage(stream);

    const auto text = stream.str();
    CHECK_NE(text.find("Usage:\n"), std::string::npos);
    CHECK_NE(text.find("featherdoc_cli run-recipe --recipe <recipe.json>"),
             std::string::npos);
    CHECK_NE(text.find("featherdoc_cli export-pdf <input.docx>"),
             std::string::npos);
    CHECK_NE(text.find("--render-headers-and-footers"), std::string::npos);
    CHECK_NE(text.find("--render-inline-images"), std::string::npos);
    CHECK_NE(text.find("--font-file <path>"), std::string::npos);
    CHECK_NE(text.find("--cjk-font-file <path>"), std::string::npos);
    CHECK_NE(text.find("--font-map <family>=<path>]..."), std::string::npos);
    CHECK_NE(text.find("--no-font-subset"), std::string::npos);
    CHECK_NE(text.find("--no-system-font-fallbacks"), std::string::npos);
    CHECK_NE(text.find("--summary-json <path>"), std::string::npos);
#if defined(FEATHERDOC_CLI_ENABLE_PDF_IMPORT)
    CHECK_NE(text.find("featherdoc_cli import-pdf <input.pdf> --output "
                       "<output.docx>"),
             std::string::npos);
    CHECK_NE(text.find("--import-table-candidates-as-tables"),
             std::string::npos);
    CHECK_NE(text.find("--min-table-continuation-confidence <score>"),
             std::string::npos);
    CHECK_EQ(text.find("--min-table-continuation-confidence <count>"),
             std::string::npos);
    CHECK_NE(text.find("table_continuation_diagnostics"),
             std::string::npos);
    CHECK_NE(text.find("min_table_continuation_confidence"),
             std::string::npos);
#endif
    CHECK_NE(text.find("featherdoc_cli inspect-styles <input.docx>"),
             std::string::npos);
    CHECK_NE(text.find("featherdoc_cli export-numbering-catalog <input.docx>"),
             std::string::npos);
    CHECK_NE(text.find("featherdoc_cli plan-table-position-presets <input.docx>"),
             std::string::npos);
    CHECK_NE(text.find("featherdoc_cli check-template-schema <input.docx>"),
             std::string::npos);
}
