#include "cli_semantic_diff_test_support.hpp"

TEST_CASE("cli semantic-diff can isolate section page setup changes") {
    const fs::path working_directory = fs::current_path();
    const fs::path left = working_directory / "cli_semantic_diff_section_left.docx";
    const fs::path right = working_directory / "cli_semantic_diff_section_right.docx";
    const fs::path output = working_directory / "cli_semantic_diff_section.json";
    const fs::path disabled_output =
        working_directory / "cli_semantic_diff_section_disabled.json";

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(output);
    remove_if_exists(disabled_output);

    create_cli_page_setup_fixture(left);
    create_cli_page_setup_fixture(right);

    featherdoc::Document right_doc(right);
    REQUIRE_FALSE(right_doc.open());
    auto setup = featherdoc::section_page_setup{};
    setup.orientation = featherdoc::page_orientation::landscape;
    setup.width_twips = 15840U;
    setup.height_twips = 12240U;
    setup.margins.top_twips = 720U;
    setup.margins.right_twips = 1800U;
    setup.margins.bottom_twips = 1080U;
    setup.margins.left_twips = 1800U;
    setup.margins.header_twips = 360U;
    setup.margins.footer_twips = 540U;
    setup.page_number_start = 5U;
    REQUIRE(right_doc.set_section_page_setup(0, setup));
    REQUIRE_FALSE(right_doc.save());

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls"},
                     output),
             0);
    const auto json = read_text_file(output);
    CHECK_NE(json.find(R"("sections":{"left_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("section_changes")"), std::string::npos);
    CHECK_NE(json.find("orientation=portrait"), std::string::npos);
    CHECK_NE(json.find("orientation=landscape"), std::string::npos);
    CHECK_NE(json.find("page_number_start=5"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"page_setup.orientation")"),
             std::string::npos);
    CHECK_NE(json.find(R"("field_path":"page_setup.width")"),
             std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-sections"},
                     disabled_output),
             0);
    const auto disabled_json = read_text_file(disabled_output);
    CHECK_NE(disabled_json.find(R"("change_count":0)"), std::string::npos);
    CHECK_NE(disabled_json.find(R"("sections":{"left_count":0)"),
             std::string::npos);

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(output);
    remove_if_exists(disabled_output);
}

TEST_CASE("cli semantic-diff reports header and footer template part changes") {
    const fs::path working_directory = fs::current_path();
    const fs::path left = working_directory / "cli_semantic_diff_part_left.docx";
    const fs::path right = working_directory / "cli_semantic_diff_part_right.docx";
    const fs::path json_output = working_directory / "cli_semantic_diff_part.json";
    const fs::path disabled_output =
        working_directory / "cli_semantic_diff_part_disabled.json";
    const fs::path text_output = working_directory / "cli_semantic_diff_part.txt";

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(disabled_output);
    remove_if_exists(text_output);

    create_cli_semantic_diff_part_fixture(left, "Body stable", "Header draft",
                                          "Footer draft");
    create_cli_semantic_diff_part_fixture(right, "Body stable", "Header approved",
                                          "Footer approved");

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("change_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("template_parts":{"left_count":2)"),
             std::string::npos);
    CHECK_NE(json.find(R"("changed_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("template_part_results")"), std::string::npos);
    CHECK_NE(json.find(R"("part":"header")"), std::string::npos);
    CHECK_NE(json.find(R"("part":"section-header")"), std::string::npos);
    CHECK_NE(json.find(R"("part":"section-footer")"), std::string::npos);
    CHECK_NE(json.find(R"("part_index":0)"), std::string::npos);
    CHECK_NE(json.find(R"("section_index":0)"), std::string::npos);
    CHECK_NE(json.find(R"("reference_kind":"default")"), std::string::npos);
    CHECK_NE(json.find(R"("left_resolved_from_section_index":0)"),
             std::string::npos);
    CHECK_NE(json.find(R"("right_resolved_from_section_index":0)"),
             std::string::npos);
    CHECK_NE(json.find(R"("entry_name":"word/header1.xml")"),
             std::string::npos);
    CHECK_NE(json.find(R"("entry_name":"word/footer1.xml")"),
             std::string::npos);
    CHECK_NE(json.find("Header draft"), std::string::npos);
    CHECK_NE(json.find("Header approved"), std::string::npos);
    CHECK_NE(json.find("Footer approved"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"text")"), std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-template-parts"},
                     disabled_output),
             0);
    const auto disabled_json = read_text_file(disabled_output);
    CHECK_NE(disabled_json.find(R"("change_count":0)"), std::string::npos);
    CHECK_NE(disabled_json.find(R"("template_parts":{"left_count":0)"),
             std::string::npos);

    const fs::path physical_only_output =
        working_directory / "cli_semantic_diff_part_physical_only.json";
    remove_if_exists(physical_only_output);
    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-resolved-section-template-parts"},
                     physical_only_output),
             0);
    const auto physical_only_json = read_text_file(physical_only_output);
    CHECK_NE(physical_only_json.find(R"("change_count":2)"), std::string::npos);
    CHECK_EQ(physical_only_json.find(R"("part":"section-header")"),
             std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string()}, text_output),
             0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("template_part[1]: part=header index=0"), std::string::npos);
    CHECK_NE(text.find("template_part[2]: part=footer index=0"), std::string::npos);
    CHECK_NE(text.find("template_part[3]: part=section-header section=0 kind=default"),
             std::string::npos);
    CHECK_NE(text.find("template_part[4]: part=section-footer section=0 kind=default"),
             std::string::npos);
    CHECK_NE(text.find("template_part_paragraph_change[0].field[0]: text"),
             std::string::npos);

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(disabled_output);
    remove_if_exists(physical_only_output);
    remove_if_exists(text_output);
}

TEST_CASE("cli semantic-diff aligns inserted paragraphs by content") {
    const fs::path working_directory = fs::current_path();
    const fs::path left = working_directory / "cli_semantic_diff_align_left.docx";
    const fs::path right = working_directory / "cli_semantic_diff_align_right.docx";
    const fs::path aligned_output = working_directory / "cli_semantic_diff_align.json";
    const fs::path index_output = working_directory / "cli_semantic_diff_index.json";

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(aligned_output);
    remove_if_exists(index_output);

    create_cli_semantic_diff_paragraph_fixture(left, {"Intro", "Scope", "Total"});
    create_cli_semantic_diff_paragraph_fixture(
        right, {"Intro", "Inserted approval note", "Scope", "Total"});

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json"},
                     aligned_output),
             0);
    const auto aligned_json = read_text_file(aligned_output);
    CHECK_NE(aligned_json.find(R"("align_sequences_by_content":true)"),
             std::string::npos);
    CHECK_NE(aligned_json.find(R"("added_count":1)"), std::string::npos);
    CHECK_NE(aligned_json.find(R"("changed_count":0)"), std::string::npos);
    CHECK_NE(aligned_json.find(R"("unchanged_count":3)"), std::string::npos);
    CHECK_NE(aligned_json.find("Inserted approval note"), std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(),
                      "--index-alignment", "--json"},
                     index_output),
             0);
    const auto index_json = read_text_file(index_output);
    CHECK_NE(index_json.find(R"("align_sequences_by_content":false)"),
             std::string::npos);
    CHECK_NE(index_json.find(R"("changed_count":2)"), std::string::npos);

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(aligned_output);
    remove_if_exists(index_output);
}
