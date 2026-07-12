#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"

#include <featherdoc/detail/path.hpp>

namespace {

constexpr auto repairable_document_xml =
    R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"/>)";

void create_package_fixture(
    const fs::path &path,
    const std::vector<std::pair<std::string, std::string>> &entries) {
    remove_if_exists(path);
    int zip_error = 0;
    const auto archive_path = featherdoc::detail::path_to_utf8(path);
    zip_t *archive = zip_openwitherror(archive_path.c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    for (const auto &[entry_name, content] : entries) {
        REQUIRE(write_archive_entry(archive, entry_name.c_str(), content));
    }
    zip_close(archive);
}

} // namespace

TEST_CASE("CLI inspects and repairs deterministic package defects") {
    const auto input = fs::current_path() / u8"CLI-修复输入-日本語-🙂.docx";
    const auto repaired = fs::current_path() / u8"CLI-修复输出-日本語-🙂.docx";
    const auto inspect_output = fs::current_path() / u8"CLI-诊断结果-🙂.json";
    const auto repair_output = fs::current_path() / u8"CLI-修复结果-🙂.json";
    create_package_fixture(input,
                           {{"word/document.xml", repairable_document_xml},
                            {"customXml/item1.xml", "<custom>keep</custom>"}});

    REQUIRE_EQ(run_cli({"inspect-package",
                        featherdoc::detail::path_to_utf8(input), "--json"},
                       inspect_output),
               0);
    const auto inspection = read_text_file(inspect_output);
    CHECK(inspection.find("\"strict_valid\":false") != std::string::npos);
    CHECK(inspection.find("missing_document_body") != std::string::npos);
    CHECK(inspection.find("missing_root_relationships") != std::string::npos);
    CHECK(inspection.find("missing_content_types") != std::string::npos);

    REQUIRE_EQ(run_cli({"repair-package",
                        featherdoc::detail::path_to_utf8(input), "--output",
                        featherdoc::detail::path_to_utf8(repaired), "--json"},
                       repair_output),
               0);
    const auto repair_result = read_text_file(repair_output);
    CHECK(repair_result.find("\"changed\":true") != std::string::npos);
    CHECK(repair_result.find("\"actions\":[") != std::string::npos);

    featherdoc::Document strict_document(repaired);
    CHECK_FALSE(strict_document.open());
    CHECK(strict_document.is_open());

    remove_if_exists(input);
    remove_if_exists(repaired);
    remove_if_exists(inspect_output);
    remove_if_exists(repair_output);
}

TEST_CASE("CLI refuses unsafe package repair without creating output") {
    const auto input = fs::current_path() / "cli_package_unsafe_input.docx";
    const auto repaired = fs::current_path() / "cli_package_unsafe_output.docx";
    const auto command_output = fs::current_path() / "cli_package_unsafe.json";
    create_package_fixture(
        input, {{"[Content_Types].xml", "<Types>"},
                {"_rels/.rels", "<Relationships>"},
                {"word/document.xml", repairable_document_xml}});
    remove_if_exists(repaired);

    CHECK_EQ(run_cli({"repair-package",
                      featherdoc::detail::path_to_utf8(input), "--output",
                      featherdoc::detail::path_to_utf8(repaired), "--json"},
                     command_output),
             1);
    CHECK_FALSE(fs::exists(repaired));
    const auto result = read_text_file(command_output);
    CHECK(result.find("\"stage\":\"repair\"") != std::string::npos);

    remove_if_exists(input);
    remove_if_exists(repaired);
    remove_if_exists(command_output);
}

TEST_CASE("CLI repair never overwrites its input") {
    const auto input = fs::current_path() / "cli_package_no_overwrite.docx";
    const auto command_output =
        fs::current_path() / "cli_package_no_overwrite.json";
    create_package_fixture(input,
                           {{"word/document.xml", repairable_document_xml}});
    const auto original_bytes = read_binary_file(input);

    CHECK_EQ(run_cli({"repair-package",
                      featherdoc::detail::path_to_utf8(input), "--output",
                      featherdoc::detail::path_to_utf8(input), "--json"},
                     command_output),
             2);
    CHECK_EQ(read_binary_file(input), original_bytes);
    const auto result = read_text_file(command_output);
    CHECK(result.find("output must differ from its input") != std::string::npos);

    remove_if_exists(input);
    remove_if_exists(command_output);
}
