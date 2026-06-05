#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"
#include "cli_style_test_support.hpp"
TEST_CASE("cli inspect-style-inheritance resolves effective properties across basedOn chain") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_style_inheritance_source.docx";
    const fs::path rooted =
        working_directory / "cli_style_inheritance_rooted.docx";
    const fs::path based =
        working_directory / "cli_style_inheritance_based.docx";
    const fs::path updated =
        working_directory / "cli_style_inheritance_updated.docx";
    const fs::path root_output =
        working_directory / "cli_style_inheritance_root_output.json";
    const fs::path base_output =
        working_directory / "cli_style_inheritance_base_output.json";
    const fs::path child_output =
        working_directory / "cli_style_inheritance_child_output.json";
    const fs::path inspect_output =
        working_directory / "cli_style_inheritance_inspect_output.json";

    remove_if_exists(source);
    remove_if_exists(rooted);
    remove_if_exists(based);
    remove_if_exists(updated);
    remove_if_exists(root_output);
    remove_if_exists(base_output);
    remove_if_exists(child_output);
    remove_if_exists(inspect_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"set-style-run-properties",
                      source.string(),
                      "Normal",
                      "--bidi-language",
                      "ar-SA",
                      "--output",
                      rooted.string(),
                      "--json"},
                     root_output),
             0);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      rooted.string(),
                      "BaseStyle",
                      "--name",
                      "Base Style",
                      "--based-on",
                      "Normal",
                      "--run-font-family",
                      "Segoe UI",
                      "--run-east-asia-language",
                      "zh-CN",
                      "--run-rtl",
                      "true",
                      "--output",
                      based.string(),
                      "--json"},
                     base_output),
             0);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      based.string(),
                      "ChildStyle",
                      "--name",
                      "Child Style",
                      "--based-on",
                      "BaseStyle",
                      "--run-language",
                      "en-US",
                      "--paragraph-bidi",
                      "true",
                      "--output",
                      updated.string(),
                      "--json"},
                     child_output),
             0);

    CHECK_EQ(run_cli({"inspect-style-inheritance",
                      updated.string(),
                      "ChildStyle",
                      "--json"},
                     inspect_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"style_id\":\"ChildStyle\",\"type\":\"paragraph\","
            "\"kind\":\"paragraph\",\"based_on\":\"BaseStyle\","
            "\"inheritance_chain\":[\"ChildStyle\",\"BaseStyle\",\"Normal\"],"
            "\"resolved_properties\":{\"font_family\":{\"value\":\"Segoe UI\","
            "\"source_style_id\":\"BaseStyle\"},"
            "\"east_asia_font_family\":{\"value\":null,\"source_style_id\":null},"
            "\"language\":{\"value\":\"en-US\",\"source_style_id\":\"ChildStyle\"},"
            "\"east_asia_language\":{\"value\":\"zh-CN\","
            "\"source_style_id\":\"BaseStyle\"},"
            "\"bidi_language\":{\"value\":\"ar-SA\","
            "\"source_style_id\":\"Normal\"},"
            "\"rtl\":{\"value\":true,\"source_style_id\":\"BaseStyle\"},"
            "\"paragraph_bidi\":{\"value\":true,"
            "\"source_style_id\":\"ChildStyle\"}}}\n"});

    remove_if_exists(source);
    remove_if_exists(rooted);
    remove_if_exists(based);
    remove_if_exists(updated);
    remove_if_exists(root_output);
    remove_if_exists(base_output);
    remove_if_exists(child_output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli inspect-style-inheritance reports parse and inspect errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_style_inheritance_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_style_inheritance_parse_output.json";
    const fs::path inspect_output =
        working_directory / "cli_style_inheritance_inspect_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(inspect_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"inspect-style-inheritance",
                      source.string(),
                      "Normal",
                      "--bogus",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-style-inheritance\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"unknown option: --bogus\"}\n"});

    CHECK_EQ(run_cli({"inspect-style-inheritance",
                      source.string(),
                      "MissingStyle",
                      "--json"},
                     inspect_output),
             1);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find("\"command\":\"inspect-style-inheritance\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(inspect_json.find(
                 "\"detail\":\"style id 'MissingStyle' was not found in "
                 "word/styles.xml\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"entry\":\"word/styles.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli materialize-style-run-properties freezes inherited values on the child style") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_materialize_style_source.docx";
    const fs::path rooted =
        working_directory / "cli_materialize_style_rooted.docx";
    const fs::path based =
        working_directory / "cli_materialize_style_based.docx";
    const fs::path child =
        working_directory / "cli_materialize_style_child.docx";
    const fs::path materialized =
        working_directory / "cli_materialize_style_materialized.docx";
    const fs::path base_mutated =
        working_directory / "cli_materialize_style_base_mutated.docx";
    const fs::path updated =
        working_directory / "cli_materialize_style_updated.docx";
    const fs::path root_output =
        working_directory / "cli_materialize_style_root_output.json";
    const fs::path base_output =
        working_directory / "cli_materialize_style_base_output.json";
    const fs::path child_output =
        working_directory / "cli_materialize_style_child_output.json";
    const fs::path materialize_output =
        working_directory / "cli_materialize_style_materialize_output.json";
    const fs::path base_mutate_output =
        working_directory / "cli_materialize_style_base_mutate_output.json";
    const fs::path normal_mutate_output =
        working_directory / "cli_materialize_style_normal_mutate_output.json";
    const fs::path inspect_output =
        working_directory / "cli_materialize_style_inspect_output.json";

    remove_if_exists(source);
    remove_if_exists(rooted);
    remove_if_exists(based);
    remove_if_exists(child);
    remove_if_exists(materialized);
    remove_if_exists(base_mutated);
    remove_if_exists(updated);
    remove_if_exists(root_output);
    remove_if_exists(base_output);
    remove_if_exists(child_output);
    remove_if_exists(materialize_output);
    remove_if_exists(base_mutate_output);
    remove_if_exists(normal_mutate_output);
    remove_if_exists(inspect_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"set-style-run-properties",
                      source.string(),
                      "Normal",
                      "--bidi-language",
                      "ar-SA",
                      "--output",
                      rooted.string(),
                      "--json"},
                     root_output),
             0);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      rooted.string(),
                      "BaseStyle",
                      "--name",
                      "Base Style",
                      "--based-on",
                      "Normal",
                      "--run-font-family",
                      "Segoe UI",
                      "--run-east-asia-language",
                      "zh-CN",
                      "--run-rtl",
                      "true",
                      "--paragraph-bidi",
                      "true",
                      "--output",
                      based.string(),
                      "--json"},
                     base_output),
             0);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      based.string(),
                      "ChildStyle",
                      "--name",
                      "Child Style",
                      "--based-on",
                      "BaseStyle",
                      "--output",
                      child.string(),
                      "--json"},
                     child_output),
             0);

    CHECK_EQ(run_cli({"materialize-style-run-properties",
                      child.string(),
                      "ChildStyle",
                      "--output",
                      materialized.string(),
                      "--json"},
                     materialize_output),
             0);
    CHECK_EQ(
        read_text_file(materialize_output),
        std::string{
            "{\"command\":\"materialize-style-run-properties\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style_id\":\"ChildStyle\",\"materialized\":["
            "{\"field\":\"font_family\",\"source_style_id\":\"BaseStyle\"},"
            "{\"field\":\"east_asia_language\",\"source_style_id\":\"BaseStyle\"},"
            "{\"field\":\"bidi_language\",\"source_style_id\":\"Normal\"},"
            "{\"field\":\"rtl\",\"source_style_id\":\"BaseStyle\"},"
            "{\"field\":\"paragraph_bidi\",\"source_style_id\":\"BaseStyle\"}]}\n"});

    CHECK_EQ(run_cli({"set-style-run-properties",
                      materialized.string(),
                      "BaseStyle",
                      "--font-family",
                      "Arial",
                      "--east-asia-language",
                      "ja-JP",
                      "--rtl",
                      "false",
                      "--paragraph-bidi",
                      "false",
                      "--output",
                      base_mutated.string(),
                      "--json"},
                     base_mutate_output),
             0);

    CHECK_EQ(run_cli({"set-style-run-properties",
                      base_mutated.string(),
                      "Normal",
                      "--bidi-language",
                      "he-IL",
                      "--output",
                      updated.string(),
                      "--json"},
                     normal_mutate_output),
             0);

    CHECK_EQ(run_cli({"inspect-style-inheritance",
                      updated.string(),
                      "ChildStyle",
                      "--json"},
                     inspect_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"style_id\":\"ChildStyle\",\"type\":\"paragraph\","
            "\"kind\":\"paragraph\",\"based_on\":\"BaseStyle\","
            "\"inheritance_chain\":[\"ChildStyle\",\"BaseStyle\",\"Normal\"],"
            "\"resolved_properties\":{\"font_family\":{\"value\":\"Segoe UI\","
            "\"source_style_id\":\"ChildStyle\"},"
            "\"east_asia_font_family\":{\"value\":null,\"source_style_id\":null},"
            "\"language\":{\"value\":null,\"source_style_id\":null},"
            "\"east_asia_language\":{\"value\":\"zh-CN\","
            "\"source_style_id\":\"ChildStyle\"},"
            "\"bidi_language\":{\"value\":\"ar-SA\","
            "\"source_style_id\":\"ChildStyle\"},"
            "\"rtl\":{\"value\":true,\"source_style_id\":\"ChildStyle\"},"
            "\"paragraph_bidi\":{\"value\":true,"
            "\"source_style_id\":\"ChildStyle\"}}}\n"});

    remove_if_exists(source);
    remove_if_exists(rooted);
    remove_if_exists(based);
    remove_if_exists(child);
    remove_if_exists(materialized);
    remove_if_exists(base_mutated);
    remove_if_exists(updated);
    remove_if_exists(root_output);
    remove_if_exists(base_output);
    remove_if_exists(child_output);
    remove_if_exists(materialize_output);
    remove_if_exists(base_mutate_output);
    remove_if_exists(normal_mutate_output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli materialize-style-run-properties reports parse and inspect errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_materialize_style_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_materialize_style_parse_output.json";
    const fs::path inspect_output =
        working_directory / "cli_materialize_style_inspect_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(inspect_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"materialize-style-run-properties",
                      source.string(),
                      "Normal",
                      "--bogus",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"materialize-style-run-properties\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"unknown option: --bogus\"}\n"});

    CHECK_EQ(run_cli({"materialize-style-run-properties",
                      source.string(),
                      "MissingStyle",
                      "--json"},
                     inspect_output),
             1);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find("\"command\":\"materialize-style-run-properties\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(inspect_json.find(
                 "\"detail\":\"style id 'MissingStyle' was not found in "
                 "word/styles.xml\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"entry\":\"word/styles.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli rebase-paragraph-style-based-on preserves inherited values while switching parent") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_rebase_paragraph_style_source.docx";
    const fs::path base_a_doc =
        working_directory / "cli_rebase_paragraph_style_base_a.docx";
    const fs::path base_b_doc =
        working_directory / "cli_rebase_paragraph_style_base_b.docx";
    const fs::path child_doc =
        working_directory / "cli_rebase_paragraph_style_child.docx";
    const fs::path rebased_doc =
        working_directory / "cli_rebase_paragraph_style_rebased.docx";
    const fs::path base_a_output =
        working_directory / "cli_rebase_paragraph_style_base_a.json";
    const fs::path base_b_output =
        working_directory / "cli_rebase_paragraph_style_base_b.json";
    const fs::path child_output =
        working_directory / "cli_rebase_paragraph_style_child.json";
    const fs::path rebase_output =
        working_directory / "cli_rebase_paragraph_style_rebase.json";
    const fs::path inspect_output =
        working_directory / "cli_rebase_paragraph_style_inspect.json";
    const fs::path properties_output =
        working_directory / "cli_rebase_paragraph_style_properties.json";

    remove_if_exists(source);
    remove_if_exists(base_a_doc);
    remove_if_exists(base_b_doc);
    remove_if_exists(child_doc);
    remove_if_exists(rebased_doc);
    remove_if_exists(base_a_output);
    remove_if_exists(base_b_output);
    remove_if_exists(child_output);
    remove_if_exists(rebase_output);
    remove_if_exists(inspect_output);
    remove_if_exists(properties_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      source.string(),
                      "BaseA",
                      "--name",
                      "Base A",
                      "--based-on",
                      "Normal",
                      "--run-font-family",
                      "Segoe UI",
                      "--run-east-asia-language",
                      "zh-CN",
                      "--run-rtl",
                      "true",
                      "--output",
                      base_a_doc.string(),
                      "--json"},
                     base_a_output),
             0);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      base_a_doc.string(),
                      "BaseB",
                      "--name",
                      "Base B",
                      "--based-on",
                      "Normal",
                      "--run-font-family",
                      "Arial",
                      "--run-east-asia-language",
                      "ja-JP",
                      "--run-rtl",
                      "false",
                      "--output",
                      base_b_doc.string(),
                      "--json"},
                     base_b_output),
             0);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      base_b_doc.string(),
                      "ChildStyle",
                      "--name",
                      "Child Style",
                      "--based-on",
                      "BaseA",
                      "--run-language",
                      "en-US",
                      "--output",
                      child_doc.string(),
                      "--json"},
                     child_output),
             0);

    CHECK_EQ(run_cli({"rebase-paragraph-style-based-on",
                      child_doc.string(),
                      "ChildStyle",
                      "BaseB",
                      "--output",
                      rebased_doc.string(),
                      "--json"},
                     rebase_output),
             0);
    CHECK_EQ(
        read_text_file(rebase_output),
        std::string{
            "{\"command\":\"rebase-paragraph-style-based-on\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style_id\":\"ChildStyle\",\"based_on\":\"BaseB\",\"preserved\":["
            "{\"field\":\"font_family\",\"source_style_id\":\"BaseA\"},"
            "{\"field\":\"east_asia_language\",\"source_style_id\":\"BaseA\"},"
            "{\"field\":\"rtl\",\"source_style_id\":\"BaseA\"}]}\n"});

    CHECK_EQ(run_cli({"inspect-style-inheritance",
                      rebased_doc.string(),
                      "ChildStyle",
                      "--json"},
                     inspect_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"style_id\":\"ChildStyle\",\"type\":\"paragraph\","
            "\"kind\":\"paragraph\",\"based_on\":\"BaseB\","
            "\"inheritance_chain\":[\"ChildStyle\",\"BaseB\",\"Normal\"],"
            "\"resolved_properties\":{\"font_family\":{\"value\":\"Segoe UI\","
            "\"source_style_id\":\"ChildStyle\"},"
            "\"east_asia_font_family\":{\"value\":null,\"source_style_id\":null},"
            "\"language\":{\"value\":\"en-US\",\"source_style_id\":\"ChildStyle\"},"
            "\"east_asia_language\":{\"value\":\"zh-CN\","
            "\"source_style_id\":\"ChildStyle\"},"
            "\"bidi_language\":{\"value\":null,\"source_style_id\":null},"
            "\"rtl\":{\"value\":true,\"source_style_id\":\"ChildStyle\"},"
            "\"paragraph_bidi\":{\"value\":null,"
            "\"source_style_id\":null}}}\n"});

    CHECK_EQ(run_cli({"inspect-paragraph-style-properties",
                      rebased_doc.string(),
                      "ChildStyle",
                      "--json"},
                     properties_output),
             0);
    CHECK_EQ(
        read_text_file(properties_output),
        std::string{
            "{\"style_id\":\"ChildStyle\",\"paragraph_style_properties\":{"
            "\"based_on\":\"BaseB\",\"next_style\":null,"
            "\"outline_level\":null}}\n"});

    remove_if_exists(source);
    remove_if_exists(base_a_doc);
    remove_if_exists(base_b_doc);
    remove_if_exists(child_doc);
    remove_if_exists(rebased_doc);
    remove_if_exists(base_a_output);
    remove_if_exists(base_b_output);
    remove_if_exists(child_output);
    remove_if_exists(rebase_output);
    remove_if_exists(inspect_output);
    remove_if_exists(properties_output);
}

TEST_CASE("cli rebase-paragraph-style-based-on reports parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_rebase_paragraph_style_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_rebase_paragraph_style_parse.json";
    const fs::path mutate_output =
        working_directory / "cli_rebase_paragraph_style_mutate.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"rebase-paragraph-style-based-on",
                      source.string(),
                      "Normal",
                      "Heading1",
                      "--bogus",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"rebase-paragraph-style-based-on\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"unknown option: --bogus\"}\n"});

    CHECK_EQ(run_cli({"rebase-paragraph-style-based-on",
                      source.string(),
                      "MissingStyle",
                      "Heading1",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"rebase-paragraph-style-based-on\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(mutate_json.find(
                 "\"detail\":\"style id 'MissingStyle' was not found in "
                 "word/styles.xml\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/styles.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli rebase-character-style-based-on preserves inherited values while switching parent") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_rebase_character_style_source.docx";
    const fs::path base_a_doc =
        working_directory / "cli_rebase_character_style_base_a.docx";
    const fs::path base_b_doc =
        working_directory / "cli_rebase_character_style_base_b.docx";
    const fs::path child_doc =
        working_directory / "cli_rebase_character_style_child.docx";
    const fs::path rebased_doc =
        working_directory / "cli_rebase_character_style_rebased.docx";
    const fs::path base_a_output =
        working_directory / "cli_rebase_character_style_base_a.json";
    const fs::path base_b_output =
        working_directory / "cli_rebase_character_style_base_b.json";
    const fs::path child_output =
        working_directory / "cli_rebase_character_style_child.json";
    const fs::path rebase_output =
        working_directory / "cli_rebase_character_style_rebase.json";
    const fs::path inspect_output =
        working_directory / "cli_rebase_character_style_inspect.json";
    const fs::path style_output =
        working_directory / "cli_rebase_character_style_style.json";

    remove_if_exists(source);
    remove_if_exists(base_a_doc);
    remove_if_exists(base_b_doc);
    remove_if_exists(child_doc);
    remove_if_exists(rebased_doc);
    remove_if_exists(base_a_output);
    remove_if_exists(base_b_output);
    remove_if_exists(child_output);
    remove_if_exists(rebase_output);
    remove_if_exists(inspect_output);
    remove_if_exists(style_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"ensure-character-style",
                      source.string(),
                      "BaseA",
                      "--name",
                      "Base A",
                      "--based-on",
                      "DefaultParagraphFont",
                      "--run-font-family",
                      "Segoe UI",
                      "--run-east-asia-language",
                      "zh-CN",
                      "--run-rtl",
                      "true",
                      "--output",
                      base_a_doc.string(),
                      "--json"},
                     base_a_output),
             0);

    CHECK_EQ(run_cli({"ensure-character-style",
                      base_a_doc.string(),
                      "BaseB",
                      "--name",
                      "Base B",
                      "--based-on",
                      "DefaultParagraphFont",
                      "--run-font-family",
                      "Arial",
                      "--run-east-asia-language",
                      "ja-JP",
                      "--run-rtl",
                      "false",
                      "--output",
                      base_b_doc.string(),
                      "--json"},
                     base_b_output),
             0);

    CHECK_EQ(run_cli({"ensure-character-style",
                      base_b_doc.string(),
                      "ChildStyle",
                      "--name",
                      "Child Style",
                      "--based-on",
                      "BaseA",
                      "--run-language",
                      "en-US",
                      "--output",
                      child_doc.string(),
                      "--json"},
                     child_output),
             0);

    CHECK_EQ(run_cli({"rebase-character-style-based-on",
                      child_doc.string(),
                      "ChildStyle",
                      "BaseB",
                      "--output",
                      rebased_doc.string(),
                      "--json"},
                     rebase_output),
             0);
    CHECK_EQ(
        read_text_file(rebase_output),
        std::string{
            "{\"command\":\"rebase-character-style-based-on\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style_id\":\"ChildStyle\",\"based_on\":\"BaseB\",\"preserved\":["
            "{\"field\":\"font_family\",\"source_style_id\":\"BaseA\"},"
            "{\"field\":\"east_asia_language\",\"source_style_id\":\"BaseA\"},"
            "{\"field\":\"rtl\",\"source_style_id\":\"BaseA\"}]}\n"});

    CHECK_EQ(run_cli({"inspect-style-inheritance",
                      rebased_doc.string(),
                      "ChildStyle",
                      "--json"},
                     inspect_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"style_id\":\"ChildStyle\",\"type\":\"character\","
            "\"kind\":\"character\",\"based_on\":\"BaseB\","
            "\"inheritance_chain\":[\"ChildStyle\",\"BaseB\",\"DefaultParagraphFont\"],"
            "\"resolved_properties\":{\"font_family\":{\"value\":\"Segoe UI\","
            "\"source_style_id\":\"ChildStyle\"},"
            "\"east_asia_font_family\":{\"value\":null,\"source_style_id\":null},"
            "\"language\":{\"value\":\"en-US\",\"source_style_id\":\"ChildStyle\"},"
            "\"east_asia_language\":{\"value\":\"zh-CN\","
            "\"source_style_id\":\"ChildStyle\"},"
            "\"bidi_language\":{\"value\":null,\"source_style_id\":null},"
            "\"rtl\":{\"value\":true,\"source_style_id\":\"ChildStyle\"},"
            "\"paragraph_bidi\":{\"value\":null,"
            "\"source_style_id\":null}}}\n"});

    CHECK_EQ(run_cli({"inspect-styles",
                      rebased_doc.string(),
                      "--style",
                      "ChildStyle",
                      "--json"},
                     style_output),
             0);
    const auto style_json = read_text_file(style_output);
    CHECK_NE(style_json.find("\"style_id\":\"ChildStyle\""), std::string::npos);
    CHECK_NE(style_json.find("\"type\":\"character\""), std::string::npos);
    CHECK_NE(style_json.find("\"based_on\":\"BaseB\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(base_a_doc);
    remove_if_exists(base_b_doc);
    remove_if_exists(child_doc);
    remove_if_exists(rebased_doc);
    remove_if_exists(base_a_output);
    remove_if_exists(base_b_output);
    remove_if_exists(child_output);
    remove_if_exists(rebase_output);
    remove_if_exists(inspect_output);
    remove_if_exists(style_output);
}

TEST_CASE("cli rebase-character-style-based-on reports parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_rebase_character_style_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_rebase_character_style_parse.json";
    const fs::path mutate_output =
        working_directory / "cli_rebase_character_style_mutate.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"rebase-character-style-based-on",
                      source.string(),
                      "DefaultParagraphFont",
                      "Strong",
                      "--bogus",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"rebase-character-style-based-on\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"unknown option: --bogus\"}\n"});

    CHECK_EQ(run_cli({"rebase-character-style-based-on",
                      source.string(),
                      "MissingStyle",
                      "Strong",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"rebase-character-style-based-on\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(mutate_json.find(
                 "\"detail\":\"style id 'MissingStyle' was not found in "
                 "word/styles.xml\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/styles.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}
