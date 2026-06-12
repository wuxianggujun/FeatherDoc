#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"
#include "cli_style_test_support.hpp"

namespace {
auto find_numbering_abstract_xml_node(pugi::xml_node numbering_root,
                                      std::string_view definition_name)
    -> pugi::xml_node {
    for (auto abstract_num = numbering_root.child("w:abstractNum");
         abstract_num != pugi::xml_node{};
         abstract_num = abstract_num.next_sibling("w:abstractNum")) {
        const auto name = abstract_num.child("w:name");
        if (name != pugi::xml_node{} &&
            std::string_view{name.attribute("w:val").value()} == definition_name) {
            return abstract_num;
        }
    }

    return {};
}

auto find_numbering_level_xml_node(pugi::xml_node abstract_num, std::string_view level)
    -> pugi::xml_node {
    for (auto numbering_level = abstract_num.child("w:lvl");
         numbering_level != pugi::xml_node{};
         numbering_level = numbering_level.next_sibling("w:lvl")) {
        if (std::string_view{numbering_level.attribute("w:ilvl").value()} == level) {
            return numbering_level;
        }
    }

    return {};
}
} // namespace

TEST_CASE("cli set-paragraph-style-numbering links a custom numbering definition to a style") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_style_numbering_source.docx";
    const fs::path updated = working_directory / "cli_style_numbering_updated.docx";
    const fs::path output = working_directory / "cli_style_numbering_output.json";
    const fs::path inspect_output =
        working_directory / "cli_style_numbering_inspect_output.json";
    const fs::path inspect_text_output =
        working_directory / "cli_style_numbering_inspect_output.txt";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
    remove_if_exists(inspect_text_output);

    create_cli_paragraph_style_fixture(source, "LegalHeading", "Legal Heading",
                                       "Heading1", false);

    CHECK_EQ(run_cli({"set-paragraph-style-numbering",
                      source.string(),
                      "LegalHeading",
                      "--definition-name",
                      "LegalHeadingOutline",
                      "--numbering-level",
                      "0:decimal:1:%1.",
                      "--numbering-level",
                      "1:decimal:1:%1.%2.",
                      "--style-level",
                      "1",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"set-paragraph-style-numbering\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style_id\":\"LegalHeading\",\"definition_name\":\"LegalHeadingOutline\","
            "\"style_level\":1,\"definition_levels\":[{\"level\":0,\"kind\":\"decimal\","
            "\"start\":1,\"text_pattern\":\"%1.\"},{\"level\":1,\"kind\":\"decimal\","
            "\"start\":1,\"text_pattern\":\"%1.%2.\"}]}\n"});

    const auto styles_xml = read_docx_entry(updated, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    REQUIRE(styles_root != pugi::xml_node{});

    const auto style = find_style_xml_node(styles_root, "LegalHeading");
    REQUIRE(style != pugi::xml_node{});
    const auto style_num_pr = style.child("w:pPr").child("w:numPr");
    REQUIRE(style_num_pr != pugi::xml_node{});
    CHECK_EQ(std::string_view{style_num_pr.child("w:ilvl").attribute("w:val").value()},
             "1");
    CHECK_NE(std::string_view{style_num_pr.child("w:numId").attribute("w:val").value()},
             "");

    const auto numbering_xml = read_docx_entry(updated, "word/numbering.xml");
    pugi::xml_document numbering_document;
    REQUIRE(numbering_document.load_string(numbering_xml.c_str()));
    const auto numbering_root = numbering_document.child("w:numbering");
    REQUIRE(numbering_root != pugi::xml_node{});

    const auto abstract_num =
        find_numbering_abstract_xml_node(numbering_root, "LegalHeadingOutline");
    REQUIRE(abstract_num != pugi::xml_node{});

    const auto level_zero = find_numbering_level_xml_node(abstract_num, "0");
    REQUIRE(level_zero != pugi::xml_node{});
    CHECK_EQ(std::string_view{level_zero.child("w:numFmt").attribute("w:val").value()},
             "decimal");
    CHECK_EQ(std::string_view{level_zero.child("w:start").attribute("w:val").value()},
             "1");
    CHECK_EQ(std::string_view{level_zero.child("w:lvlText").attribute("w:val").value()},
             "%1.");

    const auto level_one = find_numbering_level_xml_node(abstract_num, "1");
    REQUIRE(level_one != pugi::xml_node{});
    CHECK_EQ(std::string_view{level_one.child("w:numFmt").attribute("w:val").value()},
             "decimal");
    CHECK_EQ(std::string_view{level_one.child("w:start").attribute("w:val").value()},
             "1");
    CHECK_EQ(std::string_view{level_one.child("w:lvlText").attribute("w:val").value()},
             "%1.%2.");

    CHECK_EQ(run_cli({"inspect-styles", updated.string(), "--style", "LegalHeading", "--json"},
                     inspect_output),
             0);
    const auto style_num_id =
        std::string{style_num_pr.child("w:numId").attribute("w:val").value()};
    const auto definition_id = std::string{abstract_num.attribute("w:abstractNumId").value()};
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"style\":{\"style_id\":\"LegalHeading\",\"name\":\"Legal Heading\","
            "\"based_on\":\"Heading1\",\"kind\":\"paragraph\",\"type\":\"paragraph\","
            "\"numbering\":{\"num_id\":"} +
            style_num_id +
            ",\"level\":1,\"definition_id\":" + definition_id +
            ",\"definition_name\":\"LegalHeadingOutline\",\"instance\":{\"instance_id\":" +
            style_num_id +
            ",\"level_overrides\":[]}},"
            "\"is_default\":false,\"is_custom\":true,\"is_semi_hidden\":false,"
            "\"is_unhide_when_used\":false,\"is_quick_format\":true}}\n");

    CHECK_EQ(run_cli({"inspect-styles", updated.string(), "--style", "LegalHeading"},
                     inspect_text_output),
             0);
    const auto inspect_text = read_text_file(inspect_text_output);
    CHECK_NE(
        inspect_text.find("numbering: (level=1, num_id=" + style_num_id +
                          ", definition_id=" + definition_id +
                          ", definition_name=LegalHeadingOutline, override_count=0)"),
        std::string::npos);
    CHECK_NE(inspect_text.find("numbering_instance_id: " + style_num_id),
             std::string::npos);
    CHECK_NE(inspect_text.find("numbering_override_count: 0"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
    remove_if_exists(inspect_text_output);
}


TEST_CASE("cli inspect-style-numbering lists numbered paragraph styles only") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_style_numbering_source.docx";
    const fs::path updated =
        working_directory / "cli_inspect_style_numbering_updated.docx";
    const fs::path mutate_output =
        working_directory / "cli_inspect_style_numbering_mutate.json";
    const fs::path json_output =
        working_directory / "cli_inspect_style_numbering_output.json";
    const fs::path text_output =
        working_directory / "cli_inspect_style_numbering_output.txt";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(json_output);
    remove_if_exists(text_output);

    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.create_empty());

        auto legal_heading = featherdoc::paragraph_style_definition{};
        legal_heading.name = "Legal Heading";
        legal_heading.based_on = std::string{"Heading1"};
        legal_heading.is_quick_format = true;
        REQUIRE(document.ensure_paragraph_style("LegalHeading", legal_heading));

        auto body_numbered = featherdoc::paragraph_style_definition{};
        body_numbered.name = "Body Numbered";
        body_numbered.based_on = std::string{"Normal"};
        body_numbered.is_quick_format = true;
        REQUIRE(document.ensure_paragraph_style("BodyNumbered", body_numbered));

        auto plain_body = featherdoc::paragraph_style_definition{};
        plain_body.name = "Plain Body";
        plain_body.based_on = std::string{"Normal"};
        plain_body.is_quick_format = true;
        REQUIRE(document.ensure_paragraph_style("PlainBody", plain_body));

        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"ensure-style-linked-numbering",
                      source.string(),
                      "--definition-name",
                      "SharedOutline",
                      "--numbering-level",
                      "0:decimal:1:%1.",
                      "--numbering-level",
                      "1:decimal:1:%1.%2.",
                      "--style-link",
                      "LegalHeading:0",
                      "--style-link",
                      "BodyNumbered:1",
                      "--output",
                      updated.string(),
                      "--json"},
                     mutate_output),
             0);

    CHECK_EQ(run_cli({"inspect-style-numbering", updated.string(), "--json"},
                     json_output),
             0);
    const auto inspect_json = read_text_file(json_output);
    CHECK_NE(inspect_json.find("\"count\":2"), std::string::npos);
    CHECK_NE(inspect_json.find("\"style_id\":\"LegalHeading\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"style_id\":\"BodyNumbered\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"definition_name\":\"SharedOutline\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"level\":0"), std::string::npos);
    CHECK_NE(inspect_json.find("\"level\":1"), std::string::npos);
    CHECK_NE(inspect_json.find("\"instance\":{\"instance_id\":"),
             std::string::npos);
    CHECK_EQ(inspect_json.find("\"style_id\":\"PlainBody\""),
             std::string::npos);
    CHECK_EQ(inspect_json.find("\"numbering\":null"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-style-numbering", updated.string()}, text_output), 0);
    const auto inspect_text = read_text_file(text_output);
    CHECK_NE(inspect_text.find("styles: 2"), std::string::npos);
    CHECK_NE(inspect_text.find("id=LegalHeading"), std::string::npos);
    CHECK_NE(inspect_text.find("id=BodyNumbered"), std::string::npos);
    CHECK_NE(inspect_text.find("definition_name=SharedOutline"),
             std::string::npos);
    CHECK_NE(inspect_text.find("numbering=(level=0,"), std::string::npos);
    CHECK_NE(inspect_text.find("numbering=(level=1,"), std::string::npos);
    CHECK_EQ(inspect_text.find("id=PlainBody"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
}

TEST_CASE("cli inspect-style-numbering reports missing input as json") {
    const fs::path working_directory = fs::current_path();
    const fs::path output =
        working_directory / "cli_inspect_style_numbering_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"inspect-style-numbering", "--json"}, output), 2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"inspect-style-numbering\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"inspect-style-numbering "
            "expects an input path\"}\n"});

    remove_if_exists(output);
}


TEST_CASE("cli audit-style-numbering reports clean numbered styles") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_audit_style_numbering_clean_source.docx";
    const fs::path updated =
        working_directory / "cli_audit_style_numbering_clean_updated.docx";
    const fs::path mutate_output =
        working_directory / "cli_audit_style_numbering_clean_mutate.json";
    const fs::path audit_output =
        working_directory / "cli_audit_style_numbering_clean_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(audit_output);

    create_cli_paragraph_style_fixture(source, "LegalHeading", "Legal Heading",
                                       "Heading1", false);

    CHECK_EQ(run_cli({"set-paragraph-style-numbering",
                      source.string(),
                      "LegalHeading",
                      "--definition-name",
                      "LegalHeadingOutline",
                      "--numbering-level",
                      "0:decimal:1:%1.",
                      "--output",
                      updated.string(),
                      "--json"},
                     mutate_output),
             0);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      updated.string(),
                      "--json",
                      "--fail-on-issue"},
                     audit_output),
             0);
    const auto audit_json = read_text_file(audit_output);
    CHECK_NE(audit_json.find("\"command\":\"audit-style-numbering\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"clean\":true"), std::string::npos);
    CHECK_NE(audit_json.find("\"numbered_style_count\":1"),
             std::string::npos);
    CHECK_NE(audit_json.find("\"issue_count\":0"), std::string::npos);
    CHECK_NE(audit_json.find("\"suggestion_count\":0"),
             std::string::npos);
    CHECK_NE(audit_json.find("\"style_id\":\"LegalHeading\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"definition_name\":\"LegalHeadingOutline\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(audit_output);
}

TEST_CASE("cli audit-style-numbering reports broken style bindings") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_audit_style_numbering_broken_source.docx";
    const fs::path json_output =
        working_directory / "cli_audit_style_numbering_broken_output.json";
    const fs::path text_output =
        working_directory / "cli_audit_style_numbering_broken_output.txt";
    const fs::path parse_output =
        working_directory / "cli_audit_style_numbering_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(parse_output);

    create_cli_style_numbering_audit_broken_fixture(source);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      source.string(),
                      "--json",
                      "--fail-on-issue"},
                     json_output),
             1);
    const auto audit_json = read_text_file(json_output);
    CHECK_NE(audit_json.find("\"clean\":false"), std::string::npos);
    CHECK_NE(audit_json.find("\"paragraph_style_count\":4"),
             std::string::npos);
    CHECK_NE(audit_json.find("\"numbered_style_count\":2"),
             std::string::npos);
    CHECK_NE(audit_json.find("\"issue_count\":2"), std::string::npos);
    CHECK_NE(audit_json.find("\"suggestion_count\":2"),
             std::string::npos);
    CHECK_NE(audit_json.find("\"kind\":\"missing_num_id\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"style_id\":\"MissingNumId\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"kind\":\"orphan_instance\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"style_id\":\"OrphanNumId\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"num_id\":777"), std::string::npos);
    CHECK_NE(audit_json.find("\"action\":\"clear_style_numbering\""),
             std::string::npos);
    CHECK_NE(audit_json.find(
                 "\"command_template\":\"featherdoc_cli clear-paragraph-style-numbering <input.docx> MissingNumId --output <output.docx> --json\""),
             std::string::npos);
    CHECK_NE(audit_json.find(
                 "\"command_template\":\"featherdoc_cli clear-paragraph-style-numbering <input.docx> OrphanNumId --output <output.docx> --json\""),
             std::string::npos);
    CHECK_EQ(audit_json.find("\"style_id\":\"PlainBody\""),
             std::string::npos);

    CHECK_EQ(run_cli({"audit-style-numbering", source.string()}, text_output), 0);
    const auto audit_text = read_text_file(text_output);
    CHECK_NE(audit_text.find("clean: no"), std::string::npos);
    CHECK_NE(audit_text.find("numbered_styles: 2"), std::string::npos);
    CHECK_NE(audit_text.find("issues: 2"), std::string::npos);
    CHECK_NE(audit_text.find("suggestions: 2"), std::string::npos);
    CHECK_NE(audit_text.find("suggestion[0]: action=clear_style_numbering"),
             std::string::npos);
    CHECK_NE(audit_text.find(
                 "command=featherdoc_cli clear-paragraph-style-numbering <input.docx> MissingNumId --output <output.docx> --json"),
             std::string::npos);
    CHECK_NE(audit_text.find("kind=missing_num_id style_id=MissingNumId"),
             std::string::npos);
    CHECK_NE(audit_text.find("kind=orphan_instance style_id=OrphanNumId"),
             std::string::npos);

    CHECK_EQ(run_cli({"audit-style-numbering", "--json"}, parse_output), 2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"audit-style-numbering\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"audit-style-numbering "
            "expects an input path\"}\n"});

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(parse_output);
}



TEST_CASE("cli repair-style-numbering plans and applies safe clear repairs") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_repair_style_numbering_source.docx";
    const fs::path repaired =
        working_directory / "cli_repair_style_numbering_repaired.docx";
    const fs::path plan_output =
        working_directory / "cli_repair_style_numbering_plan.json";
    const fs::path apply_output =
        working_directory / "cli_repair_style_numbering_apply.json";
    const fs::path audit_output =
        working_directory / "cli_repair_style_numbering_audit.json";
    const fs::path output_without_apply =
        working_directory / "cli_repair_style_numbering_output_without_apply.json";
    const fs::path apply_without_output =
        working_directory / "cli_repair_style_numbering_apply_without_output.json";

    remove_if_exists(source);
    remove_if_exists(repaired);
    remove_if_exists(plan_output);
    remove_if_exists(apply_output);
    remove_if_exists(audit_output);
    remove_if_exists(output_without_apply);
    remove_if_exists(apply_without_output);

    create_cli_style_numbering_audit_broken_fixture(source);

    CHECK_EQ(run_cli({"repair-style-numbering", source.string(), "--json"},
                     plan_output),
             0);
    const auto plan_json = read_text_file(plan_output);
    CHECK_NE(plan_json.find("\"command\":\"repair-style-numbering\""),
             std::string::npos);
    CHECK_NE(plan_json.find("\"mode\":\"plan\""), std::string::npos);
    CHECK_NE(plan_json.find("\"before_clean\":false"), std::string::npos);
    CHECK_NE(plan_json.find("\"before_issue_count\":2"), std::string::npos);
    CHECK_NE(plan_json.find("\"after_issue_count\":null"), std::string::npos);
    CHECK_NE(plan_json.find("\"suggestion_count\":2"), std::string::npos);
    CHECK_NE(plan_json.find("\"applyable_count\":2"), std::string::npos);
    CHECK_NE(plan_json.find("\"applied_count\":0"), std::string::npos);
    CHECK_NE(plan_json.find("\"applyable\":true"), std::string::npos);
    CHECK_NE(plan_json.find("\"output_path\":null"), std::string::npos);
    CHECK_NE(plan_json.find(
                 "\"command_template\":\"featherdoc_cli clear-paragraph-style-numbering <input.docx> MissingNumId --output <output.docx> --json\""),
             std::string::npos);

    CHECK_EQ(run_cli({"repair-style-numbering",
                      source.string(),
                      "--apply",
                      "--output",
                      repaired.string(),
                      "--json"},
                     apply_output),
             0);
    const auto apply_json = read_text_file(apply_output);
    CHECK_NE(apply_json.find("\"mode\":\"apply\""), std::string::npos);
    CHECK_NE(apply_json.find("\"before_issue_count\":2"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_clean\":true"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_issue_count\":0"), std::string::npos);
    CHECK_NE(apply_json.find("\"applied_count\":2"), std::string::npos);
    CHECK_NE(apply_json.find("\"output_path\":" + json_quote(repaired.string())),
             std::string::npos);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      repaired.string(),
                      "--json",
                      "--fail-on-issue"},
                     audit_output),
             0);
    const auto repaired_audit_json = read_text_file(audit_output);
    CHECK_NE(repaired_audit_json.find("\"clean\":true"), std::string::npos);
    CHECK_NE(repaired_audit_json.find("\"numbered_style_count\":0"),
             std::string::npos);
    CHECK_NE(repaired_audit_json.find("\"issue_count\":0"), std::string::npos);

    const auto source_styles_xml = read_docx_entry(source, "word/styles.xml");
    CHECK_NE(source_styles_xml.find("w:styleId=\"OrphanNumId\""),
             std::string::npos);
    CHECK_NE(source_styles_xml.find("<w:numId w:val=\"777\"/>"),
             std::string::npos);
    const auto repaired_styles_xml = read_docx_entry(repaired, "word/styles.xml");
    CHECK_EQ(repaired_styles_xml.find("<w:numPr>"), std::string::npos);

    CHECK_EQ(run_cli({"repair-style-numbering",
                      source.string(),
                      "--output",
                      repaired.string(),
                      "--json"},
                     output_without_apply),
             2);
    CHECK_EQ(
        read_text_file(output_without_apply),
        std::string{
            "{\"command\":\"repair-style-numbering\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"--output requires --apply\"}\n"});

    CHECK_EQ(run_cli({"repair-style-numbering",
                      source.string(),
                      "--apply",
                      "--json"},
                     apply_without_output),
             2);
    CHECK_EQ(
        read_text_file(apply_without_output),
        std::string{
            "{\"command\":\"repair-style-numbering\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"repair-style-numbering "
            "--apply requires --output <path>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(repaired);
    remove_if_exists(plan_output);
    remove_if_exists(apply_output);
    remove_if_exists(audit_output);
    remove_if_exists(output_without_apply);
    remove_if_exists(apply_without_output);
}


TEST_CASE("cli repair-style-numbering applies based-on alignment repairs") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_repair_style_numbering_based_on_source.docx";
    const fs::path repaired =
        working_directory / "cli_repair_style_numbering_based_on_repaired.docx";
    const fs::path audit_output =
        working_directory / "cli_repair_style_numbering_based_on_audit.json";
    const fs::path apply_output =
        working_directory / "cli_repair_style_numbering_based_on_apply.json";
    const fs::path repaired_audit_output =
        working_directory / "cli_repair_style_numbering_based_on_repaired_audit.json";

    remove_if_exists(source);
    remove_if_exists(repaired);
    remove_if_exists(audit_output);
    remove_if_exists(apply_output);
    remove_if_exists(repaired_audit_output);

    create_cli_style_numbering_based_on_mismatch_fixture(source);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      source.string(),
                      "--json",
                      "--fail-on-issue"},
                     audit_output),
             1);
    const auto audit_json = read_text_file(audit_output);
    CHECK_NE(audit_json.find("\"clean\":false"), std::string::npos);
    CHECK_NE(audit_json.find("\"issue_count\":1"), std::string::npos);
    CHECK_NE(audit_json.find("\"kind\":\"based_on_definition_mismatch\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"style_id\":\"ChildNumbered\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"action\":\"align_with_based_on_numbering\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"target_definition_name\":\"BaseOutline\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"target_level\":0"), std::string::npos);
    CHECK_NE(audit_json.find("\"applyable\":true"), std::string::npos);

    CHECK_EQ(run_cli({"repair-style-numbering",
                      source.string(),
                      "--apply",
                      "--output",
                      repaired.string(),
                      "--json"},
                     apply_output),
             0);
    const auto apply_json = read_text_file(apply_output);
    CHECK_NE(apply_json.find("\"mode\":\"apply\""), std::string::npos);
    CHECK_NE(apply_json.find("\"before_issue_count\":1"),
             std::string::npos);
    CHECK_NE(apply_json.find("\"applyable_count\":1"), std::string::npos);
    CHECK_NE(apply_json.find("\"applied_count\":1"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_clean\":true"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_issue_count\":0"), std::string::npos);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      repaired.string(),
                      "--json",
                      "--fail-on-issue"},
                     repaired_audit_output),
             0);
    const auto repaired_audit_json = read_text_file(repaired_audit_output);
    CHECK_NE(repaired_audit_json.find("\"clean\":true"), std::string::npos);
    CHECK_NE(repaired_audit_json.find("\"issue_count\":0"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(repaired);
    remove_if_exists(audit_output);
    remove_if_exists(apply_output);
    remove_if_exists(repaired_audit_output);
}


TEST_CASE("cli repair-style-numbering relinks missing levels to unique same-name definitions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_repair_style_numbering_relink_source.docx";
    const fs::path repaired =
        working_directory / "cli_repair_style_numbering_relink_repaired.docx";
    const fs::path audit_output =
        working_directory / "cli_repair_style_numbering_relink_audit.json";
    const fs::path apply_output =
        working_directory / "cli_repair_style_numbering_relink_apply.json";
    const fs::path repaired_audit_output =
        working_directory / "cli_repair_style_numbering_relink_repaired_audit.json";

    remove_if_exists(source);
    remove_if_exists(repaired);
    remove_if_exists(audit_output);
    remove_if_exists(apply_output);
    remove_if_exists(repaired_audit_output);

    create_cli_style_numbering_missing_level_relink_fixture(source);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      source.string(),
                      "--json",
                      "--fail-on-issue"},
                     audit_output),
             1);
    const auto audit_json = read_text_file(audit_output);
    CHECK_NE(audit_json.find("\"clean\":false"), std::string::npos);
    CHECK_NE(audit_json.find("\"issue_count\":2"), std::string::npos);
    CHECK_NE(audit_json.find("\"kind\":\"missing_level_definition\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"kind\":\"duplicate_definition_name\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"action\":\"relink_style_numbering\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"target_definition_id\":2"),
             std::string::npos);
    CHECK_NE(audit_json.find("\"target_definition_name\":\"SharedOutline\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"target_level\":1"), std::string::npos);
    CHECK_NE(audit_json.find("\"applyable\":true"), std::string::npos);

    CHECK_EQ(run_cli({"repair-style-numbering",
                      source.string(),
                      "--apply",
                      "--output",
                      repaired.string(),
                      "--json"},
                     apply_output),
             0);
    const auto apply_json = read_text_file(apply_output);
    CHECK_NE(apply_json.find("\"before_issue_count\":2"),
             std::string::npos);
    CHECK_NE(apply_json.find("\"applyable_count\":1"), std::string::npos);
    CHECK_NE(apply_json.find("\"applied_count\":1"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_clean\":false"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_issue_count\":1"),
             std::string::npos);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      repaired.string(),
                      "--json",
                      "--fail-on-issue"},
                     repaired_audit_output),
             1);
    const auto repaired_audit_json = read_text_file(repaired_audit_output);
    CHECK_EQ(repaired_audit_json.find("\"kind\":\"missing_level_definition\""),
             std::string::npos);
    CHECK_NE(repaired_audit_json.find("\"kind\":\"duplicate_definition_name\""),
             std::string::npos);

    const auto repaired_styles_xml = read_docx_entry(repaired, "word/styles.xml");
    pugi::xml_document repaired_styles_document;
    REQUIRE(repaired_styles_document.load_string(repaired_styles_xml.c_str()));
    const auto relinked_style = find_style_xml_node(
        repaired_styles_document.child("w:styles"), "NeedsRelink");
    REQUIRE(relinked_style != pugi::xml_node{});
    CHECK_EQ(std::string_view{relinked_style.child("w:pPr")
                                  .child("w:numPr")
                                  .child("w:numId")
                                  .attribute("w:val")
                                  .value()},
             "8");

    remove_if_exists(source);
    remove_if_exists(repaired);
    remove_if_exists(audit_output);
    remove_if_exists(apply_output);
    remove_if_exists(repaired_audit_output);
}


TEST_CASE("cli repair-style-numbering imports catalog repairs before style fixes") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_repair_style_numbering_catalog_source.docx";
    const fs::path catalog =
        working_directory / "cli_repair_style_numbering_catalog.json";
    const fs::path repaired =
        working_directory / "cli_repair_style_numbering_catalog_repaired.docx";
    const fs::path plan_output =
        working_directory / "cli_repair_style_numbering_catalog_plan.json";
    const fs::path apply_output =
        working_directory / "cli_repair_style_numbering_catalog_apply.json";
    const fs::path audit_output =
        working_directory / "cli_repair_style_numbering_catalog_audit.json";

    remove_if_exists(source);
    remove_if_exists(catalog);
    remove_if_exists(repaired);
    remove_if_exists(plan_output);
    remove_if_exists(apply_output);
    remove_if_exists(audit_output);

    create_cli_style_numbering_catalog_repair_fixture(source);
    write_binary_file(
        catalog,
        "{\"definition_count\":1,\"instance_count\":0,\"definitions\":["
        "{\"name\":\"CatalogOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":1,\"text_pattern\":\"%1.\"},"
        "{\"level\":1,\"kind\":\"decimal\",\"start\":1,\"text_pattern\":\"%1.%2.\"}"
        "],\"instances\":[]}]}");

    CHECK_EQ(run_cli({"repair-style-numbering", source.string(), "--json"},
                     plan_output),
             0);
    const auto plain_plan_json = read_text_file(plan_output);
    CHECK_NE(plain_plan_json.find("\"action\":\"add_numbering_level\""),
             std::string::npos);
    CHECK_NE(plain_plan_json.find(
                 "patch-numbering-catalog numbering-catalog.json --patch-file "
                 "<patch-with-upsert_levels.json>"),
             std::string::npos);

    CHECK_EQ(run_cli({"repair-style-numbering",
                      source.string(),
                      "--catalog-file",
                      catalog.string(),
                      "--json"},
                     plan_output),
             2);
    CHECK_NE(read_text_file(plan_output).find("--catalog-file requires --apply"),
             std::string::npos);

    CHECK_EQ(run_cli({"repair-style-numbering",
                      source.string(),
                      "--catalog-file",
                      catalog.string(),
                      "--apply",
                      "--output",
                      repaired.string(),
                      "--json"},
                     apply_output),
             0);
    const auto apply_json = read_text_file(apply_output);
    CHECK_NE(apply_json.find("\"before_issue_count\":1"),
             std::string::npos);
    CHECK_NE(apply_json.find("\"catalog_file\":" + json_quote(catalog.string())),
             std::string::npos);
    CHECK_NE(apply_json.find("\"catalog_import\":{"), std::string::npos);
    CHECK_NE(apply_json.find("\"input_definition_count\":1"),
             std::string::npos);
    CHECK_NE(apply_json.find("\"imported_definition_count\":1"),
             std::string::npos);
    CHECK_NE(apply_json.find("\"imported_instance_count\":0"),
             std::string::npos);
    CHECK_NE(apply_json.find("\"applyable_count\":0"), std::string::npos);
    CHECK_NE(apply_json.find("\"applied_count\":0"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_clean\":true"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_issue_count\":0"), std::string::npos);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      repaired.string(),
                      "--json",
                      "--fail-on-issue"},
                     audit_output),
             0);
    const auto audit_json = read_text_file(audit_output);
    CHECK_NE(audit_json.find("\"clean\":true"), std::string::npos);
    CHECK_NE(audit_json.find("\"issue_count\":0"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(catalog);
    remove_if_exists(repaired);
    remove_if_exists(plan_output);
    remove_if_exists(apply_output);
    remove_if_exists(audit_output);
}


TEST_CASE("cli ensure-style-linked-numbering links multiple paragraph styles to one shared instance") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_style_linked_numbering_source.docx";
    const fs::path updated =
        working_directory / "cli_style_linked_numbering_updated.docx";
    const fs::path output =
        working_directory / "cli_style_linked_numbering_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.create_empty());

        auto heading_style = featherdoc::paragraph_style_definition{};
        heading_style.name = "Legal Heading";
        heading_style.based_on = std::string{"Heading1"};
        heading_style.paragraph_bidi = false;
        REQUIRE(document.ensure_paragraph_style("LegalHeading", heading_style));

        auto subheading_style = featherdoc::paragraph_style_definition{};
        subheading_style.name = "Legal Subheading";
        subheading_style.based_on = std::string{"Heading2"};
        subheading_style.paragraph_bidi = false;
        REQUIRE(document.ensure_paragraph_style("LegalSubheading", subheading_style));

        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"ensure-style-linked-numbering",
                      source.string(),
                      "--definition-name",
                      "LegalOutlineShared",
                      "--numbering-level",
                      "0:decimal:7:(%1)",
                      "--numbering-level",
                      "1:decimal:1:(%1.%2)",
                      "--style-link",
                      "LegalHeading:0",
                      "--style-link",
                      "LegalSubheading:1",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    featherdoc::Document updated_document(updated);
    REQUIRE_FALSE(updated_document.open());
    const auto definitions = updated_document.list_numbering_definitions();
    REQUIRE_FALSE(updated_document.last_error());
    const auto definition = std::find_if(definitions.begin(), definitions.end(),
                                         [](const auto &summary) {
                                             return summary.name == "LegalOutlineShared";
                                         });
    REQUIRE(definition != definitions.end());
    REQUIRE_EQ(definition->instance_ids.size(), 1U);
    const auto definition_id = std::to_string(definition->definition_id);

    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"ensure-style-linked-numbering\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"definition_id\":"} +
            definition_id +
            ",\"definition_name\":\"LegalOutlineShared\",\"definition_levels\":["
            "{\"level\":0,\"kind\":\"decimal\",\"start\":7,\"text_pattern\":\"(%1)\"},"
            "{\"level\":1,\"kind\":\"decimal\",\"start\":1,\"text_pattern\":\"(%1.%2)\"}],"
            "\"style_links\":[{\"style_id\":\"LegalHeading\",\"level\":0},"
            "{\"style_id\":\"LegalSubheading\",\"level\":1}]}\n");

    const auto heading_style = updated_document.find_style("LegalHeading");
    const auto subheading_style = updated_document.find_style("LegalSubheading");
    REQUIRE(heading_style.has_value());
    REQUIRE(subheading_style.has_value());
    REQUIRE(heading_style->numbering.has_value());
    REQUIRE(subheading_style->numbering.has_value());
    REQUIRE(heading_style->numbering->num_id.has_value());
    REQUIRE(subheading_style->numbering->num_id.has_value());
    CHECK_EQ(*heading_style->numbering->num_id, *subheading_style->numbering->num_id);
    REQUIRE(heading_style->numbering->level.has_value());
    REQUIRE(subheading_style->numbering->level.has_value());
    CHECK_EQ(*heading_style->numbering->level, 0U);
    CHECK_EQ(*subheading_style->numbering->level, 1U);

    const auto styles_xml = read_docx_entry(updated, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    REQUIRE(styles_root != pugi::xml_node{});
    const auto heading_style_xml = find_style_xml_node(styles_root, "LegalHeading");
    const auto subheading_style_xml = find_style_xml_node(styles_root, "LegalSubheading");
    REQUIRE(heading_style_xml != pugi::xml_node{});
    REQUIRE(subheading_style_xml != pugi::xml_node{});
    CHECK_EQ(std::string_view{
                 heading_style_xml.child("w:pPr").child("w:numPr").child("w:ilvl").attribute("w:val").value()},
             "0");
    CHECK_EQ(std::string_view{
                 subheading_style_xml.child("w:pPr").child("w:numPr").child("w:ilvl").attribute("w:val").value()},
             "1");

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}
