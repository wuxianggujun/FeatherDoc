#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"

namespace {

#if !defined(FEATHERDOC_CLI_ENABLE_PDF)
TEST_CASE("cli export-pdf reports disabled pdf support") {
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
             1);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"export-pdf\""), std::string::npos);
    CHECK_NE(json.find("\"ok\":false"), std::string::npos);
    CHECK_NE(json.find("\"stage\":\"export\""), std::string::npos);
    CHECK_NE(json.find("\"message\":\"Operation not supported\""),
             std::string::npos);
    CHECK_NE(json.find(
                 "PDF export requires configuring with -DFEATHERDOC_BUILD_PDF=ON"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(pdf_output);
}
#endif

} // namespace
