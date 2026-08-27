#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"

namespace {

#if !defined(FEATHERDOC_CLI_ENABLE_PDF)
TEST_CASE("cli does not expose export-pdf when pdf support is disabled") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_export_pdf_disabled_source.docx";
    const fs::path output =
        working_directory / "cli_export_pdf_disabled_output.json";
    const fs::path pdf_output =
        working_directory / "cli_export_pdf_disabled.pdf";

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(pdf_output);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"export-pdf", source.string(), "--output",
                      pdf_output.string(), "--json"},
                     output),
             2);

    const auto error_text = read_text_file(output);
    CHECK_NE(error_text.find("unknown command: export-pdf"),
             std::string::npos);
    CHECK_EQ(error_text.find("FEATHERDOC_BUILD_PDF"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(pdf_output);
}
#endif

} // namespace
