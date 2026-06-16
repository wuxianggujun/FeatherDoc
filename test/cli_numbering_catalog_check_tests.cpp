#include "cli_numbering_catalog_test_support.hpp"

TEST_CASE("cli check-numbering-catalog gates docx catalog against baseline") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_check_numbering_catalog_source.docx";
    const fs::path baseline_catalog =
        working_directory / "cli_check_numbering_catalog_baseline.json";
    const fs::path drift_catalog =
        working_directory / "cli_check_numbering_catalog_drift.json";
    const fs::path dirty_catalog =
        working_directory / "cli_check_numbering_catalog_dirty.json";
    const fs::path generated_catalog =
        working_directory / "cli_check_numbering_catalog_generated.json";
    const fs::path match_output =
        working_directory / "cli_check_numbering_catalog_match_output.json";
    const fs::path drift_output =
        working_directory / "cli_check_numbering_catalog_drift_output.json";
    const fs::path dirty_output =
        working_directory / "cli_check_numbering_catalog_dirty_output.json";
    const fs::path parse_output =
        working_directory / "cli_check_numbering_catalog_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(baseline_catalog);
    remove_if_exists(drift_catalog);
    remove_if_exists(dirty_catalog);
    remove_if_exists(generated_catalog);
    remove_if_exists(match_output);
    remove_if_exists(drift_output);
    remove_if_exists(dirty_output);
    remove_if_exists(parse_output);

    auto document = featherdoc::Document(source);
    REQUIRE_FALSE(document.create_empty());

    auto catalog = featherdoc::numbering_catalog{};
    auto catalog_definition = featherdoc::numbering_catalog_definition{};
    catalog_definition.definition.name = "CheckOutline";
    catalog_definition.definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::bullet, 1U, 1U, "o"},
    };
    catalog_definition.instances.push_back(
        featherdoc::numbering_instance_summary{
            99U,
            {featherdoc::numbering_level_override_summary{0U, 3U,
                                                          std::nullopt}}});
    catalog.definitions.push_back(std::move(catalog_definition));

    const auto import_summary = document.import_numbering_catalog(catalog);
    REQUIRE(static_cast<bool>(import_summary));
    REQUIRE_FALSE(document.save());

    write_binary_file(
        baseline_catalog,
        "{\"definitions\":[{\"name\":\"CheckOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":1,"
        "\"text_pattern\":\"%1.\"},"
        "{\"level\":1,\"kind\":\"bullet\",\"start\":1,"
        "\"text_pattern\":\"o\"}],\"instances\":["
        "{\"instance_id\":99,\"level_overrides\":["
        "{\"level\":0,\"start_override\":3,"
        "\"level_definition\":null}]}]}]}\n");
    write_binary_file(
        drift_catalog,
        "{\"definitions\":[{\"name\":\"CheckOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":7,"
        "\"text_pattern\":\"%1.\"},"
        "{\"level\":1,\"kind\":\"bullet\",\"start\":1,"
        "\"text_pattern\":\"o\"}],\"instances\":["
        "{\"instance_id\":99,\"level_overrides\":["
        "{\"level\":0,\"start_override\":3,"
        "\"level_definition\":null}]}]}]}\n");
    write_binary_file(dirty_catalog,
                      "{\"definitions\":[{\"name\":\"\",\"levels\":[],"
                      "\"instances\":[]}]}\n");

    CHECK_EQ(run_cli({"check-numbering-catalog",
                      source.string(),
                      "--catalog-file",
                      baseline_catalog.string(),
                      "--output",
                      generated_catalog.string(),
                      "--json"},
                     match_output),
             0);
    const auto match_json = read_text_file(match_output);
    CHECK_NE(match_json.find("\"command\":\"check-numbering-catalog\""),
             std::string::npos);
    CHECK_NE(match_json.find("\"matches\":true"), std::string::npos);
    CHECK_NE(match_json.find("\"clean\":true"), std::string::npos);
    CHECK_NE(match_json.find("\"catalog_file\":" +
                             json_quote(baseline_catalog.string())),
             std::string::npos);
    CHECK_NE(match_json.find("\"generated_output_path\":" +
                             json_quote(generated_catalog.string())),
             std::string::npos);
    CHECK_NE(match_json.find("\"baseline_issue_count\":0"),
             std::string::npos);
    CHECK_NE(match_json.find("\"generated_issue_count\":0"),
             std::string::npos);
    CHECK_NE(match_json.find("\"changed_definition_count\":0"),
             std::string::npos);
    CHECK_NE(read_text_file(generated_catalog).find("\"name\":\"CheckOutline\""),
             std::string::npos);

    CHECK_EQ(run_cli({"check-numbering-catalog",
                      source.string(),
                      "--catalog-file",
                      drift_catalog.string(),
                      "--json"},
                     drift_output),
             1);
    const auto drift_json = read_text_file(drift_output);
    CHECK_NE(drift_json.find("\"matches\":false"), std::string::npos);
    CHECK_NE(drift_json.find("\"clean\":true"), std::string::npos);
    CHECK_NE(drift_json.find("\"changed_definition_count\":1"),
             std::string::npos);
    CHECK_NE(drift_json.find("\"changed_level_count\":1"),
             std::string::npos);

    CHECK_EQ(run_cli({"check-numbering-catalog",
                      source.string(),
                      "--catalog-file",
                      dirty_catalog.string(),
                      "--json"},
                     dirty_output),
             1);
    const auto dirty_json = read_text_file(dirty_output);
    CHECK_NE(dirty_json.find("\"matches\":false"), std::string::npos);
    CHECK_NE(dirty_json.find("\"clean\":false"), std::string::npos);
    CHECK_NE(dirty_json.find("\"baseline_issue_count\":2"),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"empty_definition_name\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"empty_levels\""),
             std::string::npos);

    CHECK_EQ(run_cli({"check-numbering-catalog", source.string(), "--json"},
                     parse_output),
             2);
    CHECK_EQ(read_text_file(parse_output),
             std::string{
                 "{\"command\":\"check-numbering-catalog\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"missing "
                 "--catalog-file <catalog.json>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(baseline_catalog);
    remove_if_exists(drift_catalog);
    remove_if_exists(dirty_catalog);
    remove_if_exists(generated_catalog);
    remove_if_exists(match_output);
    remove_if_exists(drift_output);
    remove_if_exists(dirty_output);
    remove_if_exists(parse_output);
}
