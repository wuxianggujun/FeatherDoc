#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "basic_document_xml_test_support.hpp"
#include "basic_docx_archive_test_support.hpp"
#include "allocation_failure_test_case.hpp"
#include "doctest.h"

#include <featherdoc.hpp>

namespace {

pugi::allocation_function delegated_style_restore_allocate = nullptr;
std::size_t style_restore_allocation_calls = 0U;
std::size_t style_restore_failure_call = 0U;

auto controlled_style_restore_allocate(std::size_t size) -> void * {
    ++style_restore_allocation_calls;
    if (style_restore_failure_call != 0U &&
        style_restore_allocation_calls == style_restore_failure_call) {
        return nullptr;
    }
    return delegated_style_restore_allocate != nullptr
               ? delegated_style_restore_allocate(size)
               : nullptr;
}

struct style_restore_allocator_guard final {
    pugi::allocation_function previous_allocate{
        pugi::get_memory_allocation_function()};
    pugi::deallocation_function previous_deallocate{
        pugi::get_memory_deallocation_function()};

    style_restore_allocator_guard() {
        delegated_style_restore_allocate = this->previous_allocate;
        style_restore_allocation_calls = 0U;
        style_restore_failure_call = 0U;
        pugi::set_memory_management_functions(controlled_style_restore_allocate,
                                              this->previous_deallocate);
    }

    ~style_restore_allocator_guard() {
        pugi::set_memory_management_functions(this->previous_allocate,
                                              this->previous_deallocate);
        delegated_style_restore_allocate = nullptr;
        style_restore_allocation_calls = 0U;
        style_restore_failure_call = 0U;
    }
};

auto make_large_style_restore_rollback(std::string source_style_id)
    -> featherdoc::style_refactor_rollback_entry {
    auto rollback = featherdoc::style_refactor_rollback_entry{};
    rollback.action = featherdoc::style_refactor_action::merge;
    rollback.source_style_id = std::move(source_style_id);
    rollback.target_style_id = "TargetStyle";
    rollback.restorable = true;
    rollback.source_style_xml =
        "<w:style xmlns:w=\"http://schemas.openxmlformats.org/"
        "wordprocessingml/2006/main\" w:type=\"paragraph\" w:styleId=\"" +
        rollback.source_style_id +
        "\"><w:name w:val=\"Restored source\"/></w:style>";

    auto usage = featherdoc::style_usage_summary{};
    usage.style_id = rollback.source_style_id;
    usage.paragraph_count = 2U;
    usage.body.paragraph_count = 2U;
    for (std::size_t ordinal = 1U; ordinal <= 2U; ++ordinal) {
        auto hit = featherdoc::style_usage_hit{};
        hit.part = featherdoc::style_usage_part_kind::body;
        hit.kind = featherdoc::style_usage_hit_kind::paragraph;
        hit.entry_name = "word/document.xml";
        hit.ordinal = ordinal;
        hit.node_ordinal = ordinal;
        usage.hits.push_back(std::move(hit));
    }
    rollback.source_usage = std::move(usage);
    return rollback;
}

void write_style_identity_transaction_fixture(
    const std::filesystem::path &path, std::string_view target_style_id) {
    const auto content_types_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
  <Override PartName="/word/header1.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)"};
    const auto document_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:pPr><w:pStyle w:val="SourceStyle"/></w:pPr><w:r><w:t>body</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId3"/>
      <w:footerReference w:type="default" r:id="rId4"/>
    </w:sectPr>
  </w:body>
</w:document>
)"};
    const auto document_relationships_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
  <Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header" Target="header1.xml"/>
  <Relationship Id="rId4" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer" Target="footer1.xml"/>
</Relationships>
)"};
    const auto styles_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/></w:style>
  <w:style w:type="paragraph" w:styleId="SourceStyle"><w:name w:val="Source Style"/></w:style>
  <w:style w:type="paragraph" w:styleId=")"} +
        std::string{target_style_id} +
        R"("><w:name w:val="Target Style"/></w:style>
  <w:style w:type="paragraph" w:styleId="DependentStyle"><w:name w:val="Dependent Style"/><w:basedOn w:val="SourceStyle"/></w:style>
</w:styles>
)";
    const auto header_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:pPr><w:pStyle w:val="SourceStyle"/></w:pPr><w:r><w:t>header</w:t></w:r></w:p>
</w:hdr>
)"};
    const auto footer_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:pPr><w:pStyle w:val="SourceStyle"/></w:pPr><w:r><w:t>footer</w:t></w:r></w:p>
</w:ftr>
)"};

    write_test_archive_entries(
        path, {
                  {test_content_types_xml_entry, content_types_xml},
                  {test_relationships_xml_entry, test_relationships_xml},
                  {test_document_xml_entry, document_xml},
                  {"word/_rels/document.xml.rels", document_relationships_xml},
                  {"word/styles.xml", styles_xml},
                  {"word/header1.xml", header_xml},
                  {"word/footer1.xml", footer_xml},
              });
}

void write_style_refactor_batch_transaction_fixture(
    const std::filesystem::path &path) {
    const auto content_types_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
</Types>
)"};
    const auto root_relationships_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
</Relationships>
)"};
    // document.xml is intentionally in FeatherDoc's compact save form so a
    // failure followed by save() can be compared byte-for-byte.
    const auto document_xml = std::string{
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:pStyle w:val="SourceA"/></w:pPr><w:r><w:t>first</w:t></w:r></w:p><w:p><w:pPr><w:pStyle w:val="SourceB"/></w:pPr><w:r><w:t>second</w:t></w:r></w:p><w:sectPr/></w:body></w:document>)"};
    const auto document_relationships_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
</Relationships>
)"};
    const auto styles_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/></w:style>
  <w:style w:type="paragraph" w:styleId="SourceA"><w:name w:val="Source A"/></w:style>
  <w:style w:type="paragraph" w:styleId="SourceB"><w:name w:val="Source B"/></w:style>
  <w:style w:type="paragraph" w:styleId="DependentA"><w:name w:val="Dependent A"/><w:basedOn w:val="SourceA"/></w:style>
  <w:style w:type="paragraph" w:styleId="DependentB"><w:name w:val="Dependent B"/><w:basedOn w:val="SourceB"/></w:style>
</w:styles>
)"};

    write_test_archive_entries(
        path, {
                  {test_content_types_xml_entry, content_types_xml},
                  {test_relationships_xml_entry, root_relationships_xml},
                  {test_document_xml_entry, document_xml},
                  {"word/_rels/document.xml.rels", document_relationships_xml},
                  {"word/styles.xml", styles_xml},
              });
}

void write_style_attachment_transaction_fixture(
    const std::filesystem::path &path) {
    const auto content_types_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)"};
    const auto root_relationships_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
</Relationships>
)"};
    const auto document_xml = std::string{
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:t>unstyled</w:t></w:r></w:p><w:sectPr/></w:body></w:document>)"};
    const auto document_relationships_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
</Relationships>
)"};

    write_test_archive_entries(
        path, {
                  {test_content_types_xml_entry, content_types_xml},
                  {test_relationships_xml_entry, root_relationships_xml},
                  {test_document_xml_entry, document_xml},
                  {"word/_rels/document.xml.rels", document_relationships_xml},
              });
}

void check_style_identity_transaction_allocation_failures(
    const std::filesystem::path &path, std::string_view destination_style_id,
    bool merge) {
    const auto original_styles_xml =
        read_test_docx_entry(path, "word/styles.xml");
    const auto original_relationships_xml =
        read_test_docx_entry(path, "word/_rels/document.xml.rels");
    const auto original_content_types_xml =
        read_test_docx_entry(path, test_content_types_xml_entry);
    const auto apply_mutation = [&](featherdoc::Document &document) {
        return merge
                   ? document.merge_style("SourceStyle", destination_style_id)
                   : document.rename_style("SourceStyle", destination_style_id);
    };

    auto allocator_guard = style_restore_allocator_guard{};
    std::size_t successful_allocation_count = 0U;
    {
        featherdoc::Document document(path);
        REQUIRE_FALSE(document.open());
        REQUIRE(document.find_style("SourceStyle").has_value());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());
        style_restore_allocation_calls = 0U;
        style_restore_failure_call = 0U;
        REQUIRE(apply_mutation(document));
        successful_allocation_count = style_restore_allocation_calls;
        CHECK_FALSE(paragraph.has_next());
        CHECK_FALSE(document.find_style("SourceStyle").has_value());
        REQUIRE(document.find_style(destination_style_id).has_value());
        const auto usage = document.find_style_usage(destination_style_id);
        REQUIRE(usage.has_value());
        CHECK_EQ(usage->body.paragraph_count, 1U);
        CHECK_EQ(usage->header.paragraph_count, 1U);
        CHECK_EQ(usage->footer.paragraph_count, 1U);
        const auto dependent = document.find_style("DependentStyle");
        REQUIRE(dependent.has_value());
        CHECK_EQ(dependent->based_on, destination_style_id);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        style_restore_failure_call = 0U;
        featherdoc::Document document(path);
        REQUIRE_FALSE(document.open());
        REQUIRE(document.find_style("SourceStyle").has_value());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());

        style_restore_allocation_calls = 0U;
        style_restore_failure_call = failure_call;
        const auto mutated = apply_mutation(document);
        style_restore_failure_call = 0U;

        CAPTURE(merge);
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        CAPTURE(style_restore_allocation_calls);
        CAPTURE(document.last_error().detail);
        CAPTURE(document.last_error().entry_name);
        CHECK_FALSE(mutated);
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK_FALSE(document.last_error().entry_name.empty());
        CHECK(document.find_style("SourceStyle").has_value());
        if (!merge) {
            CHECK_FALSE(document.find_style(destination_style_id).has_value());
        } else {
            CHECK(document.find_style(destination_style_id).has_value());
        }
        const auto source_usage = document.find_style_usage("SourceStyle");
        REQUIRE(source_usage.has_value());
        CHECK_EQ(source_usage->body.paragraph_count, 1U);
        CHECK_EQ(source_usage->header.paragraph_count, 1U);
        CHECK_EQ(source_usage->footer.paragraph_count, 1U);
        const auto dependent = document.find_style("DependentStyle");
        REQUIRE(dependent.has_value());
        CHECK_EQ(dependent->based_on, "SourceStyle");
        CHECK(paragraph.has_next());

        const auto save_error = document.save();
        INFO(save_error.message());
        INFO(document.last_error().detail);
        CHECK_FALSE(save_error);
        CHECK_EQ(read_test_docx_entry(path, "word/styles.xml"),
                 original_styles_xml);
        CHECK_EQ(read_test_docx_entry(path, "word/_rels/document.xml.rels"),
                 original_relationships_xml);
        CHECK_EQ(read_test_docx_entry(path, test_content_types_xml_entry),
                 original_content_types_xml);
    }
}

void check_applied_style_transaction_allocation_failures(
    const std::filesystem::path &path, bool run_style) {
    write_style_attachment_transaction_fixture(path);
    const auto original_document_xml =
        read_test_docx_entry(path, test_document_xml_entry);
    const auto original_relationships_xml =
        read_test_docx_entry(path, "word/_rels/document.xml.rels");
    const auto original_content_types_xml =
        read_test_docx_entry(path, test_content_types_xml_entry);

    const auto apply_style = [run_style](featherdoc::Document &document,
                                         featherdoc::Paragraph paragraph,
                                         featherdoc::Run run) {
        return run_style ? document.set_run_style(run, "Strong")
                         : document.set_paragraph_style(paragraph, "Normal");
    };

    auto allocator_guard = style_restore_allocator_guard{};
    std::size_t successful_allocation_count = 0U;
    {
        featherdoc::Document document(path);
        REQUIRE_FALSE(document.open());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());
        auto run = paragraph.runs();
        REQUIRE(run.has_next());

        style_restore_allocation_calls = 0U;
        style_restore_failure_call = 0U;
        REQUIRE(apply_style(document, paragraph, run));
        successful_allocation_count = style_restore_allocation_calls;
        CHECK(paragraph.has_next());
        CHECK(run.has_next());
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        style_restore_failure_call = 0U;
        write_style_attachment_transaction_fixture(path);
        featherdoc::Document document(path);
        REQUIRE_FALSE(document.open());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());
        auto run = paragraph.runs();
        REQUIRE(run.has_next());

        style_restore_allocation_calls = 0U;
        style_restore_failure_call = failure_call;
        const auto styled = apply_style(document, paragraph, run);
        style_restore_failure_call = 0U;

        CAPTURE(run_style);
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        CAPTURE(style_restore_allocation_calls);
        CAPTURE(document.last_error().detail);
        CAPTURE(document.last_error().entry_name);
        CHECK_FALSE(styled);
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK_FALSE(document.last_error().entry_name.empty());
        CHECK(paragraph.has_next());
        CHECK(run.has_next());
        CHECK_EQ(run.get_text(), "unstyled");
        CHECK_FALSE(run.style_id().has_value());
        const auto paragraph_summary = document.inspect_paragraph(0U);
        REQUIRE(paragraph_summary.has_value());
        CHECK_FALSE(paragraph_summary->style_id.has_value());

        const auto save_error = document.save();
        INFO(save_error.message());
        INFO(document.last_error().detail);
        REQUIRE_FALSE(save_error);
        CHECK_FALSE(test_docx_entry_exists(path, "word/styles.xml"));
        CHECK_EQ(read_test_docx_entry(path, test_document_xml_entry),
                 original_document_xml);
        CHECK_EQ(read_test_docx_entry(path, "word/_rels/document.xml.rels"),
                 original_relationships_xml);
        CHECK_EQ(read_test_docx_entry(path, test_content_types_xml_entry),
                 original_content_types_xml);
        CHECK(paragraph.has_next());
        CHECK(run.has_next());

        style_restore_allocation_calls = 0U;
        REQUIRE(apply_style(document, paragraph, run));
        const auto retry_save_error = document.save();
        INFO(retry_save_error.message());
        INFO(document.last_error().detail);
        REQUIRE_FALSE(retry_save_error);
        CHECK(test_docx_entry_exists(path, "word/styles.xml"));
        const auto updated_document_xml =
            read_test_docx_entry(path, test_document_xml_entry);
        if (run_style) {
            CHECK_NE(updated_document_xml.find(R"(w:rStyle w:val="Strong")"),
                     std::string::npos);
        } else {
            CHECK_NE(updated_document_xml.find(R"(w:pStyle w:val="Normal")"),
                     std::string::npos);
        }
    }

    style_restore_failure_call = 0U;
    std::filesystem::remove(path);
}

auto wrap_style_xml_deeply(std::string leaf, std::size_t depth) -> std::string {
    std::string xml;
    xml.reserve(leaf.size() + depth * 7U);
    for (std::size_t index = 0U; index < depth; ++index) {
        xml += "<n>";
    }
    xml += leaf;
    for (std::size_t index = 0U; index < depth; ++index) {
        xml += "</n>";
    }
    return xml;
}

} // namespace

TEST_CASE("style traversal handles deeply nested XML iteratively") {
    namespace fs = std::filesystem;

    constexpr std::size_t nesting_depth = 25'000U;
    const auto target = fs::current_path() / "styles_deep_xml.docx";
    fs::remove(target);

    const auto styled_paragraph =
        std::string{"<w:p><w:pPr><w:pStyle w:val=\"SourceStyle\"/>"
                    "</w:pPr><w:r><w:t>deep style</w:t></w:r></w:p>"};
    const auto document_xml =
        std::string{"<w:document xmlns:w=\"http://schemas.openxmlformats.org/"
                    "wordprocessingml/2006/main\"><w:body>"} +
        wrap_style_xml_deeply(styled_paragraph, nesting_depth) +
        "</w:body></w:document>";
    const auto source_definition =
        wrap_style_xml_deeply("<w:rPr><w:b/></w:rPr>", nesting_depth);
    const auto styles_xml =
        std::string{"<w:styles xmlns:w=\"http://schemas.openxmlformats.org/"
                    "wordprocessingml/2006/main\">"
                    "<w:style w:type=\"paragraph\" w:styleId=\"SourceStyle\">"
                    "<w:name w:val=\"Source Style\"/>"} +
        source_definition +
        "</w:style>"
        "<w:style w:type=\"paragraph\" w:styleId=\"TargetStyle\">"
        "<w:name w:val=\"Target Style\"/><w:rPr><w:i/></w:rPr>"
        "</w:style></w:styles>";
    write_test_docx_with_styles(target, document_xml, styles_xml);

    featherdoc::document_open_options options;
    options.limits.max_compression_ratio = 100'000U;
    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open(options));

    const auto usage = document.find_style_usage("SourceStyle");
    REQUIRE(usage.has_value());
    CHECK_EQ(usage->paragraph_count, 1U);
    const auto plan =
        document.plan_style_refactor({{featherdoc::style_refactor_action::merge,
                                       "SourceStyle", "TargetStyle"}});
    REQUIRE(plan.has_value());
    REQUIRE_EQ(plan->operations.size(), 1U);
    CHECK(document.rename_style("SourceStyle", "RenamedStyle"));
    const auto renamed_usage = document.find_style_usage("RenamedStyle");
    REQUIRE(renamed_usage.has_value());
    CHECK_EQ(renamed_usage->paragraph_count, 1U);
    const auto save_error = document.save();
    INFO(save_error.message());
    INFO(document.last_error().detail);
    CHECK_FALSE(save_error);

    const auto saved_document_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    CHECK_LT(saved_document_xml.size(), 1024U * 1024U);
    CHECK_NE(saved_document_xml.find("w:val=\"RenamedStyle\""),
             std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:val=\"SourceStyle\""),
             std::string::npos);

    fs::remove(target);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "rename_style is atomic across every pugi allocation failure") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "style_rename_allocation_failure.docx";
    fs::remove(target);
    write_style_identity_transaction_fixture(target, "ExistingTarget");
    check_style_identity_transaction_allocation_failures(
        target, std::string(40'000U, 'R'), false);
    fs::remove(target);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "merge_style is atomic across every pugi allocation failure") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "style_merge_allocation_failure.docx";
    fs::remove(target);
    const auto target_style_id = std::string(40'000U, 'M');
    write_style_identity_transaction_fixture(target, target_style_id);
    check_style_identity_transaction_allocation_failures(target,
                                                         target_style_id, true);
    fs::remove(target);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "apply_style_refactor publishes a multi-operation batch atomically") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "style_refactor_batch_allocation_failure.docx";
    fs::remove(target);
    write_style_refactor_batch_transaction_fixture(target);

    const auto original_document_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    const auto original_styles_xml =
        read_test_docx_entry(target, "word/styles.xml");
    const auto original_relationships_xml =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    const auto original_content_types_xml =
        read_test_docx_entry(target, test_content_types_xml_entry);
    const auto renamed_a = std::string{"RenamedA-"} + std::string(40'000U, 'A');
    const auto renamed_b = std::string{"RenamedB-"} + std::string(40'000U, 'B');
    const auto first_operation =
        std::vector<featherdoc::style_refactor_request>{
            {featherdoc::style_refactor_action::rename, "SourceA", renamed_a},
        };
    const auto full_batch = std::vector<featherdoc::style_refactor_request>{
        {featherdoc::style_refactor_action::rename, "SourceA", renamed_a},
        {featherdoc::style_refactor_action::rename, "SourceB", renamed_b},
    };

    auto allocator_guard = style_restore_allocator_guard{};
    std::size_t first_operation_allocation_count = 0U;
    {
        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        REQUIRE(document.find_style("SourceA").has_value());
        style_restore_allocation_calls = 0U;
        style_restore_failure_call = 0U;
        const auto applied = document.apply_style_refactor(first_operation);
        REQUIRE(applied.has_value());
        REQUIRE(applied->applied());
        first_operation_allocation_count = style_restore_allocation_calls;
    }

    std::size_t full_batch_allocation_count = 0U;
    {
        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        REQUIRE(document.find_style("SourceA").has_value());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());
        style_restore_allocation_calls = 0U;
        style_restore_failure_call = 0U;
        const auto applied = document.apply_style_refactor(full_batch);
        REQUIRE(applied.has_value());
        REQUIRE(applied->applied());
        REQUIRE_EQ(applied->applied_count, 2U);
        REQUIRE_EQ(applied->rollback_entries.size(), 2U);
        full_batch_allocation_count = style_restore_allocation_calls;
        CHECK_FALSE(paragraph.has_next());
        CHECK_FALSE(document.find_style("SourceA").has_value());
        CHECK_FALSE(document.find_style("SourceB").has_value());
        CHECK(document.find_style(renamed_a).has_value());
        CHECK(document.find_style(renamed_b).has_value());
    }
    REQUIRE_GT(full_batch_allocation_count, first_operation_allocation_count);

    // Every allocation unique to the second operation must fail without
    // publishing the already-completed first operation from the work DOM.
    for (std::size_t failure_call = first_operation_allocation_count + 1U;
         failure_call <= full_batch_allocation_count; ++failure_call) {
        style_restore_failure_call = 0U;
        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        REQUIRE(document.find_style("SourceA").has_value());
        REQUIRE(document.find_style("SourceB").has_value());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());

        style_restore_allocation_calls = 0U;
        style_restore_failure_call = failure_call;
        const auto applied = document.apply_style_refactor(full_batch);
        style_restore_failure_call = 0U;

        CAPTURE(failure_call);
        CAPTURE(first_operation_allocation_count);
        CAPTURE(full_batch_allocation_count);
        CAPTURE(style_restore_allocation_calls);
        CAPTURE(document.last_error().detail);
        CAPTURE(document.last_error().entry_name);
        CHECK_FALSE(applied.has_value());
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK_FALSE(document.last_error().entry_name.empty());
        CHECK(document.find_style("SourceA").has_value());
        CHECK(document.find_style("SourceB").has_value());
        CHECK_FALSE(document.find_style(renamed_a).has_value());
        CHECK_FALSE(document.find_style(renamed_b).has_value());
        const auto dependent_a = document.find_style("DependentA");
        REQUIRE(dependent_a.has_value());
        CHECK_EQ(dependent_a->based_on, "SourceA");
        const auto dependent_b = document.find_style("DependentB");
        REQUIRE(dependent_b.has_value());
        CHECK_EQ(dependent_b->based_on, "SourceB");
        const auto source_a_usage = document.find_style_usage("SourceA");
        REQUIRE(source_a_usage.has_value());
        CHECK_EQ(source_a_usage->body.paragraph_count, 1U);
        const auto source_b_usage = document.find_style_usage("SourceB");
        REQUIRE(source_b_usage.has_value());
        CHECK_EQ(source_b_usage->body.paragraph_count, 1U);
        CHECK(paragraph.has_next());

        const auto save_error = document.save();
        INFO(save_error.message());
        INFO(document.last_error().detail);
        CHECK_FALSE(save_error);
        CHECK_EQ(read_test_docx_entry(target, test_document_xml_entry),
                 original_document_xml);
        CHECK_EQ(read_test_docx_entry(target, "word/styles.xml"),
                 original_styles_xml);
        CHECK_EQ(read_test_docx_entry(target, "word/_rels/document.xml.rels"),
                 original_relationships_xml);
        CHECK_EQ(read_test_docx_entry(target, test_content_types_xml_entry),
                 original_content_types_xml);
    }

    fs::remove(target);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "applied paragraph and run style transactions preserve the package at "
    "every pugixml allocation failure") {
    namespace fs = std::filesystem;

    check_applied_style_transaction_allocation_failures(
        fs::current_path() /
            "paragraph_style_transaction_allocation_failure.docx",
        false);
    check_applied_style_transaction_allocation_failures(
        fs::current_path() / "run_style_transaction_allocation_failure.docx",
        true);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "rename_style atomically attaches a missing styles part") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "style_rename_attach_allocation_failure.docx";
    fs::remove(target);
    const auto renamed_style_id = std::string(40'000U, 'A');
    auto allocator_guard = style_restore_allocator_guard{};
    std::size_t successful_allocation_count = 0U;
    {
        featherdoc::Document document(target);
        REQUIRE_FALSE(document.create_empty());
        REQUIRE(document.find_style("Heading1").has_value());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());
        style_restore_allocation_calls = 0U;
        style_restore_failure_call = 0U;
        REQUIRE(document.rename_style("Heading1", renamed_style_id));
        successful_allocation_count = style_restore_allocation_calls;
        CHECK_FALSE(paragraph.has_next());
        const auto save_error = document.save();
        INFO(save_error.message());
        INFO(document.last_error().detail);
        REQUIRE_FALSE(save_error);
        CHECK(test_docx_entry_exists(target, "word/styles.xml"));
        CHECK_NE(read_test_docx_entry(target, "word/_rels/document.xml.rels")
                     .find("relationships/styles"),
                 std::string::npos);
        CHECK_NE(read_test_docx_entry(target, test_content_types_xml_entry)
                     .find("wordprocessingml.styles+xml"),
                 std::string::npos);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        style_restore_failure_call = 0U;
        featherdoc::Document document(target);
        REQUIRE_FALSE(document.create_empty());
        REQUIRE(document.find_style("Heading1").has_value());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());

        style_restore_allocation_calls = 0U;
        style_restore_failure_call = failure_call;
        const auto renamed =
            document.rename_style("Heading1", renamed_style_id);
        style_restore_failure_call = 0U;

        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        CAPTURE(style_restore_allocation_calls);
        CAPTURE(document.last_error().detail);
        CAPTURE(document.last_error().entry_name);
        CHECK_FALSE(renamed);
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK(document.find_style("Heading1").has_value());
        CHECK_FALSE(document.find_style(renamed_style_id).has_value());
        CHECK(paragraph.has_next());

        const auto save_error = document.save();
        INFO(save_error.message());
        INFO(document.last_error().detail);
        CHECK_FALSE(save_error);
        CHECK_FALSE(test_docx_entry_exists(target, "word/styles.xml"));
        CHECK_FALSE(
            test_docx_entry_exists(target, "word/_rels/document.xml.rels"));
        CHECK_EQ(read_test_docx_entry(target, test_content_types_xml_entry)
                     .find("wordprocessingml.styles+xml"),
                 std::string::npos);
    }

    fs::remove(target);
}

TEST_CASE(
    "merge_style rewrites references removes source style and round-trips") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_merge_round_trip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    featherdoc::paragraph_style_definition source_style;
    source_style.name = "Source Style";
    source_style.based_on = "Normal";
    source_style.next_style = "SourceStyle";
    source_style.is_quick_format = true;
    CHECK(doc.ensure_paragraph_style("SourceStyle", source_style));

    featherdoc::paragraph_style_definition target_style;
    target_style.name = "Target Style";
    target_style.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("TargetStyle", target_style));

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("merged style paragraph").has_next());
    CHECK(doc.set_paragraph_style(paragraph, "SourceStyle"));

    CHECK(doc.merge_style("SourceStyle", "TargetStyle"));
    CHECK_FALSE(doc.find_style("SourceStyle").has_value());
    const auto merged_style = doc.find_style("TargetStyle");
    REQUIRE(merged_style.has_value());
    CHECK_EQ(merged_style->style_id, "TargetStyle");
    CHECK_EQ(merged_style->name, "Target Style");

    const auto usage = doc.find_style_usage("TargetStyle");
    REQUIRE(usage.has_value());
    CHECK_EQ(usage->paragraph_count, 1U);
    CHECK_EQ(usage->total_count(), 1U);

    CHECK_FALSE(doc.merge_style("TargetStyle", "TargetStyle"));
    CHECK_EQ(doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_FALSE(reopened.find_style("SourceStyle").has_value());
    const auto reopened_usage = reopened.find_style_usage("TargetStyle");
    REQUIRE(reopened_usage.has_value());
    CHECK_EQ(reopened_usage->paragraph_count, 1U);
    CHECK_EQ(reopened_usage->total_count(), 1U);

    const auto styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(styles_xml.find(R"(w:styleId="SourceStyle")"), std::string::npos);
    CHECK_NE(styles_xml.find(R"(w:styleId="TargetStyle")"), std::string::npos);
    const auto document_xml = read_test_docx_entry(target, "word/document.xml");
    CHECK_NE(document_xml.find(R"(w:pStyle w:val="TargetStyle")"),
             std::string::npos);
    CHECK_EQ(document_xml.find(R"(w:pStyle w:val="SourceStyle")"),
             std::string::npos);

    fs::remove(target);
}

TEST_CASE("plan_style_refactor validates rename and merge operations without "
          "mutating") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_refactor_plan.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    featherdoc::paragraph_style_definition source_style;
    source_style.name = "Source Style";
    source_style.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("SourceStyle", source_style));

    featherdoc::paragraph_style_definition target_style;
    target_style.name = "Target Style";
    target_style.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("TargetStyle", target_style));

    featherdoc::character_style_definition character_target;
    character_target.name = "Character Target";
    CHECK(doc.ensure_character_style("CharacterTarget", character_target));

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("refactor plan paragraph").has_next());
    CHECK(doc.set_paragraph_style(paragraph, "SourceStyle"));

    const auto plan = doc.plan_style_refactor({
        {featherdoc::style_refactor_action::rename, "SourceStyle",
         "RenamedStyle"},
        {featherdoc::style_refactor_action::merge, "SourceStyle",
         "TargetStyle"},
        {featherdoc::style_refactor_action::merge, "SourceStyle",
         "CharacterTarget"},
        {featherdoc::style_refactor_action::rename, "MissingStyle", "NewStyle"},
        {featherdoc::style_refactor_action::rename, "SourceStyle",
         "TargetStyle"},
    });
    REQUIRE(plan.has_value());
    CHECK_FALSE(plan->clean());
    CHECK_EQ(plan->operation_count, 5U);
    CHECK_EQ(plan->applyable_count, 2U);
    CHECK_EQ(plan->issue_count, 3U);
    REQUIRE_EQ(plan->operations.size(), 5U);

    CHECK_EQ(plan->operations[0].action,
             featherdoc::style_refactor_action::rename);
    CHECK(plan->operations[0].applyable);
    REQUIRE(plan->operations[0].source_usage.has_value());
    CHECK_EQ(plan->operations[0].source_usage->total_count(), 1U);
    CHECK_FALSE(plan->operations[0].target_style.has_value());

    CHECK_EQ(plan->operations[1].action,
             featherdoc::style_refactor_action::merge);
    CHECK(plan->operations[1].applyable);
    REQUIRE(plan->operations[1].target_style.has_value());
    CHECK_EQ(plan->operations[1].target_style->kind,
             featherdoc::style_kind::paragraph);

    REQUIRE_EQ(plan->operations[2].issues.size(), 1U);
    CHECK_EQ(plan->operations[2].issues[0].code, "style_type_mismatch");
    CHECK_FALSE(plan->operations[2].applyable);

    REQUIRE_EQ(plan->operations[3].issues.size(), 1U);
    CHECK_EQ(plan->operations[3].issues[0].code, "missing_source_style");
    CHECK_FALSE(plan->operations[3].applyable);

    REQUIRE_EQ(plan->operations[4].issues.size(), 1U);
    CHECK_EQ(plan->operations[4].issues[0].code, "target_style_exists");
    CHECK_FALSE(plan->operations[4].applyable);

    CHECK(doc.find_style("SourceStyle").has_value());
    CHECK(doc.find_style("TargetStyle").has_value());
    CHECK_FALSE(doc.find_style("RenamedStyle").has_value());

    fs::remove(target);
}

TEST_CASE("suggest_style_merges recommends duplicate custom styles by usage") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_merge_suggestions.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    featherdoc::paragraph_style_definition duplicate_a;
    duplicate_a.name = "Duplicate Body A";
    duplicate_a.based_on = "Normal";
    duplicate_a.run_font_family = "Aptos";
    CHECK(doc.ensure_paragraph_style("DuplicateBodyA", duplicate_a));

    featherdoc::paragraph_style_definition duplicate_b;
    duplicate_b.name = "Duplicate Body B";
    duplicate_b.based_on = "Normal";
    duplicate_b.run_font_family = "Aptos";
    CHECK(doc.ensure_paragraph_style("DuplicateBodyB", duplicate_b));

    featherdoc::paragraph_style_definition duplicate_c;
    duplicate_c.name = "Duplicate Body C";
    duplicate_c.based_on = "Normal";
    duplicate_c.next_style = "Normal";
    duplicate_c.run_font_family = "Aptos";
    CHECK(doc.ensure_paragraph_style("DuplicateBodyC", duplicate_c));

    auto first_paragraph = doc.paragraphs();
    REQUIRE(first_paragraph.has_next());
    REQUIRE(
        first_paragraph.add_run("first duplicate style reference").has_next());
    CHECK(doc.set_paragraph_style(first_paragraph, "DuplicateBodyA"));
    auto second_paragraph = first_paragraph.insert_paragraph_after("second");
    CHECK(doc.set_paragraph_style(second_paragraph, "DuplicateBodyA"));
    auto third_paragraph = second_paragraph.insert_paragraph_after("third");
    CHECK(doc.set_paragraph_style(third_paragraph, "DuplicateBodyB"));
    auto fourth_paragraph = third_paragraph.insert_paragraph_after("fourth");
    CHECK(doc.set_paragraph_style(fourth_paragraph, "DuplicateBodyC"));

    const auto plan = doc.suggest_style_merges();
    REQUIRE(plan.has_value());
    CHECK(plan->clean());
    CHECK_EQ(plan->operation_count, 2U);
    CHECK_EQ(plan->applyable_count, 2U);
    CHECK_EQ(plan->issue_count, 0U);
    REQUIRE_EQ(plan->operations.size(), 2U);

    const auto confidence_summary = plan->suggestion_confidence_summary();
    CHECK_EQ(confidence_summary.suggestion_count, 2U);
    CHECK_EQ(confidence_summary.exact_xml_match_count, 1U);
    CHECK_EQ(confidence_summary.xml_difference_count, 1U);
    REQUIRE(confidence_summary.min_confidence.has_value());
    CHECK_EQ(*confidence_summary.min_confidence, 80U);
    REQUIRE(confidence_summary.max_confidence.has_value());
    CHECK_EQ(*confidence_summary.max_confidence, 95U);
    REQUIRE(confidence_summary.recommended_min_confidence.has_value());
    CHECK_EQ(*confidence_summary.recommended_min_confidence, 95U);
    CHECK_NE(confidence_summary.recommendation.find("review lower-confidence"),
             std::string::npos);

    const auto &exact_operation = plan->operations[0];
    CHECK_EQ(exact_operation.action, featherdoc::style_refactor_action::merge);
    CHECK_EQ(exact_operation.source_style_id, "DuplicateBodyB");
    CHECK_EQ(exact_operation.target_style_id, "DuplicateBodyA");
    REQUIRE(exact_operation.source_usage.has_value());
    CHECK_EQ(exact_operation.source_usage->paragraph_count, 1U);
    REQUIRE(exact_operation.suggestion.has_value());
    CHECK_EQ(exact_operation.suggestion->reason_code,
             "matching_style_signature_and_xml");
    CHECK_EQ(exact_operation.suggestion->confidence, 95U);
    CHECK(exact_operation.suggestion->differences.empty());
    CHECK_NE(std::find(exact_operation.suggestion->evidence.begin(),
                       exact_operation.suggestion->evidence.end(),
                       "style_definition_xml_matches"),
             exact_operation.suggestion->evidence.end());
    REQUIRE(exact_operation.target_style.has_value());
    CHECK_EQ(exact_operation.target_style->kind,
             featherdoc::style_kind::paragraph);

    const auto &diff_operation = plan->operations[1];
    CHECK_EQ(diff_operation.action, featherdoc::style_refactor_action::merge);
    CHECK_EQ(diff_operation.source_style_id, "DuplicateBodyC");
    CHECK_EQ(diff_operation.target_style_id, "DuplicateBodyA");
    REQUIRE(diff_operation.suggestion.has_value());
    CHECK_EQ(diff_operation.suggestion->reason_code,
             "matching_resolved_style_signature");
    CHECK_EQ(diff_operation.suggestion->confidence, 80U);
    CHECK_NE(std::find(diff_operation.suggestion->evidence.begin(),
                       diff_operation.suggestion->evidence.end(),
                       "style_definition_xml_differs"),
             diff_operation.suggestion->evidence.end());
    CHECK_NE(std::find(diff_operation.suggestion->differences.begin(),
                       diff_operation.suggestion->differences.end(), "w:next"),
             diff_operation.suggestion->differences.end());

    CHECK(doc.find_style("DuplicateBodyA").has_value());
    CHECK(doc.find_style("DuplicateBodyB").has_value());
    CHECK(doc.find_style("DuplicateBodyC").has_value());

    fs::remove(target);
}

TEST_CASE("apply_style_refactor applies clean batches and rejects conflicts") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_refactor_apply.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    featherdoc::paragraph_style_definition source_a;
    source_a.name = "Source A";
    source_a.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("SourceA", source_a));

    featherdoc::paragraph_style_definition source_b;
    source_b.name = "Source B";
    source_b.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("SourceB", source_b));

    featherdoc::paragraph_style_definition target_style;
    target_style.name = "Target Style";
    target_style.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("TargetStyle", target_style));

    auto first_paragraph = doc.paragraphs();
    REQUIRE(first_paragraph.has_next());
    REQUIRE(first_paragraph.add_run("first").has_next());
    CHECK(doc.set_paragraph_style(first_paragraph, "SourceA"));
    auto second_paragraph = first_paragraph.insert_paragraph_after("second");
    CHECK(doc.set_paragraph_style(second_paragraph, "SourceB"));
    auto target_paragraph = second_paragraph.insert_paragraph_after("target");
    CHECK(doc.set_paragraph_style(target_paragraph, "TargetStyle"));

    const auto result = doc.apply_style_refactor({
        {featherdoc::style_refactor_action::rename, "SourceA", "RenamedA"},
        {featherdoc::style_refactor_action::merge, "SourceB", "TargetStyle"},
    });
    REQUIRE(result.has_value());
    CHECK(result->applied());
    CHECK(result->changed);
    CHECK_EQ(result->requested_count, 2U);
    CHECK_EQ(result->applied_count, 2U);
    CHECK_EQ(result->skipped_count(), 0U);
    REQUIRE_EQ(result->rollback_entries.size(), 2U);
    CHECK(result->rollback_entries[0].automatic);
    CHECK_EQ(result->rollback_entries[0].action,
             featherdoc::style_refactor_action::rename);
    CHECK_EQ(result->rollback_entries[0].source_style_id, "RenamedA");
    CHECK_EQ(result->rollback_entries[0].target_style_id, "SourceA");
    CHECK_FALSE(result->rollback_entries[1].automatic);
    CHECK(result->rollback_entries[1].restorable);
    CHECK_EQ(result->rollback_entries[1].action,
             featherdoc::style_refactor_action::merge);
    CHECK_EQ(result->rollback_entries[1].source_style_id, "SourceB");
    CHECK_EQ(result->rollback_entries[1].target_style_id, "TargetStyle");
    CHECK_NE(result->rollback_entries[1].source_style_xml.find(
                 R"(w:styleId="SourceB")"),
             std::string::npos);
    REQUIRE(result->rollback_entries[1].source_usage.has_value());
    CHECK_EQ(result->rollback_entries[1].source_usage->paragraph_count, 1U);
    REQUIRE_EQ(result->rollback_entries[1].source_usage->hits.size(), 1U);
    CHECK_EQ(result->rollback_entries[1].source_usage->hits[0].node_ordinal,
             2U);

    CHECK_FALSE(doc.find_style("SourceA").has_value());
    CHECK_FALSE(doc.find_style("SourceB").has_value());
    CHECK(doc.find_style("RenamedA").has_value());
    CHECK(doc.find_style("TargetStyle").has_value());

    const auto renamed_usage = doc.find_style_usage("RenamedA");
    REQUIRE(renamed_usage.has_value());
    CHECK_EQ(renamed_usage->paragraph_count, 1U);
    const auto target_usage = doc.find_style_usage("TargetStyle");
    REQUIRE(target_usage.has_value());
    CHECK_EQ(target_usage->paragraph_count, 2U);

    const auto restore_plan =
        doc.plan_style_refactor_restore({result->rollback_entries[1]});
    REQUIRE(restore_plan.has_value());
    CHECK(restore_plan->restored());
    CHECK_FALSE(restore_plan->changed);
    CHECK(restore_plan->dry_run);
    CHECK_EQ(restore_plan->requested_count, 1U);
    CHECK_EQ(restore_plan->restored_count, 1U);
    CHECK_EQ(restore_plan->issue_count(), 0U);
    CHECK(restore_plan->issue_summary().empty());
    CHECK_EQ(restore_plan->restored_style_count, 1U);
    CHECK_EQ(restore_plan->restored_reference_count, 1U);
    REQUIRE_EQ(restore_plan->operations.size(), 1U);
    CHECK(restore_plan->operations[0].restored);
    CHECK(restore_plan->operations[0].style_restored);
    CHECK_EQ(restore_plan->operations[0].restored_reference_count, 1U);
    CHECK_FALSE(doc.find_style("SourceB").has_value());
    const auto dry_run_target_usage = doc.find_style_usage("TargetStyle");
    REQUIRE(dry_run_target_usage.has_value());
    CHECK_EQ(dry_run_target_usage->paragraph_count, 2U);

    const auto restore =
        doc.restore_style_refactor({result->rollback_entries[1]});
    REQUIRE(restore.has_value());
    CHECK(restore->restored());
    CHECK(restore->changed);
    CHECK_EQ(restore->requested_count, 1U);
    CHECK_EQ(restore->restored_count, 1U);
    CHECK_EQ(restore->restored_style_count, 1U);
    CHECK_EQ(restore->restored_reference_count, 1U);
    REQUIRE_EQ(restore->operations.size(), 1U);
    CHECK(restore->operations[0].restored);
    CHECK(restore->operations[0].style_restored);
    CHECK_EQ(restore->operations[0].restored_reference_count, 1U);
    CHECK(doc.find_style("SourceB").has_value());
    const auto restored_source_usage = doc.find_style_usage("SourceB");
    REQUIRE(restored_source_usage.has_value());
    CHECK_EQ(restored_source_usage->paragraph_count, 1U);
    const auto restored_target_usage = doc.find_style_usage("TargetStyle");
    REQUIRE(restored_target_usage.has_value());
    CHECK_EQ(restored_target_usage->paragraph_count, 1U);

    const auto restore_conflict_plan =
        doc.plan_style_refactor_restore({result->rollback_entries[1]});
    REQUIRE(restore_conflict_plan.has_value());
    CHECK_FALSE(restore_conflict_plan->restored());
    CHECK(restore_conflict_plan->dry_run);
    CHECK_EQ(restore_conflict_plan->issue_count(), 1U);
    const auto restore_conflict_summary =
        restore_conflict_plan->issue_summary();
    REQUIRE_EQ(restore_conflict_summary.size(), 1U);
    CHECK_EQ(restore_conflict_summary[0].code, "source_style_exists");
    CHECK_EQ(restore_conflict_summary[0].count, 1U);
    CHECK_NE(
        restore_conflict_summary[0].suggestion.find("skip this rollback entry"),
        std::string::npos);
    REQUIRE_EQ(restore_conflict_plan->operations.size(), 1U);
    REQUIRE_EQ(restore_conflict_plan->operations[0].issues.size(), 1U);
    CHECK_EQ(restore_conflict_plan->operations[0].issues[0].code,
             "source_style_exists");
    CHECK_NE(restore_conflict_plan->operations[0].issues[0].suggestion.find(
                 "skip this rollback entry"),
             std::string::npos);

    const auto conflict = doc.apply_style_refactor({
        {featherdoc::style_refactor_action::rename, "TargetStyle",
         "FinalTarget"},
        {featherdoc::style_refactor_action::merge, "TargetStyle", "RenamedA"},
    });
    REQUIRE(conflict.has_value());
    CHECK_FALSE(conflict->applied());
    CHECK_FALSE(conflict->changed);
    CHECK_EQ(conflict->applied_count, 0U);
    CHECK_EQ(conflict->skipped_count(), 2U);
    CHECK_EQ(conflict->plan.issue_count, 2U);
    CHECK_EQ(conflict->plan.operations[0].issues[0].code,
             "duplicate_source_operation");
    CHECK_EQ(conflict->plan.operations[1].issues[0].code,
             "duplicate_source_operation");
    CHECK(doc.find_style("TargetStyle").has_value());
    CHECK_FALSE(doc.find_style("FinalTarget").has_value());

    fs::remove(target);
}

TEST_CASE(
    "style refactor restore resolves canonical header and footer PartNames") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "style_refactor_restore_related_parts.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
  <Override PartName="/word/header1.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";
    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>body</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId3"/>
      <w:footerReference w:type="default" r:id="rId4"/>
    </w:sectPr>
  </w:body>
</w:document>
)";
    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
  <Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header" Target="header1.xml"/>
  <Relationship Id="rId4" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer" Target="footer1.xml"/>
</Relationships>
)";
    const std::string styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/></w:style>
  <w:style w:type="paragraph" w:styleId="SourceStyle"><w:name w:val="Source Style"/></w:style>
  <w:style w:type="paragraph" w:styleId="TargetStyle"><w:name w:val="Target Style"/></w:style>
</w:styles>
)";
    const std::string header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:pPr><w:pStyle w:val="SourceStyle"/></w:pPr><w:r><w:t>header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:pPr><w:pStyle w:val="SourceStyle"/></w:pPr><w:r><w:t>footer</w:t></w:r></w:p>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/styles.xml", styles_xml},
            {"word/header1.xml", header_xml},
            {"word/footer1.xml", footer_xml},
        });

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());

    const auto applied = document.apply_style_refactor(
        {{featherdoc::style_refactor_action::merge, "SourceStyle",
          "TargetStyle"}});
    REQUIRE(applied.has_value());
    REQUIRE(applied->applied());
    REQUIRE_EQ(applied->rollback_entries.size(), 1U);

    auto rollback = applied->rollback_entries[0];
    REQUIRE(rollback.source_usage.has_value());
    REQUIRE_EQ(rollback.source_usage->hits.size(), 2U);
    for (auto &hit : rollback.source_usage->hits) {
        if (hit.part == featherdoc::style_usage_part_kind::header) {
            hit.entry_name = "/WORD/HEADER1.XML";
        } else if (hit.part == featherdoc::style_usage_part_kind::footer) {
            hit.entry_name = "/WORD/FOOTER1.XML";
        }
    }

    auto invalid_rollback = rollback;
    const auto invalid_header = std::find_if(
        invalid_rollback.source_usage->hits.begin(),
        invalid_rollback.source_usage->hits.end(), [](const auto &hit) {
            return hit.part == featherdoc::style_usage_part_kind::header;
        });
    REQUIRE(invalid_header != invalid_rollback.source_usage->hits.end());
    invalid_header->entry_name = "word//header1.xml";

    const auto invalid_plan =
        document.plan_style_refactor_restore({invalid_rollback});
    REQUIRE(invalid_plan.has_value());
    CHECK_FALSE(invalid_plan->restored());
    REQUIRE_EQ(invalid_plan->operations.size(), 1U);
    REQUIRE_EQ(invalid_plan->operations[0].issues.size(), 1U);
    CHECK_EQ(invalid_plan->operations[0].issues[0].code,
             "invalid_usage_part_name");

    const auto restore_plan = document.plan_style_refactor_restore({rollback});
    REQUIRE(restore_plan.has_value());
    CHECK(restore_plan->restored());
    CHECK_EQ(restore_plan->issue_count(), 0U);

    const auto restored = document.restore_style_refactor({rollback});
    REQUIRE(restored.has_value());
    CHECK(restored->restored());
    CHECK_EQ(restored->restored_reference_count, 2U);

    const auto restored_usage = document.find_style_usage("SourceStyle");
    REQUIRE(restored_usage.has_value());
    CHECK_EQ(restored_usage->header.paragraph_count, 1U);
    CHECK_EQ(restored_usage->footer.paragraph_count, 1U);

    fs::remove(target);
}

TEST_CASE("style refactor restore rejects duplicate hits without publishing a "
          "partial operation") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "style_refactor_restore_duplicate_hit.docx";
    fs::remove(target);
    const auto document_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:pPr><w:pStyle w:val="TargetStyle"/></w:pPr><w:r><w:t>first</w:t></w:r></w:p>
    <w:p><w:pPr><w:pStyle w:val="TargetStyle"/></w:pPr><w:r><w:t>second</w:t></w:r></w:p>
  </w:body>
</w:document>
)"};
    const auto styles_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/></w:style>
  <w:style w:type="paragraph" w:styleId="TargetStyle"><w:name w:val="Target Style"/></w:style>
</w:styles>
)"};
    write_test_docx_with_styles(target, document_xml, styles_xml);
    const auto original_styles_xml =
        read_test_docx_entry(target, "word/styles.xml");

    auto rollback = make_large_style_restore_rollback("SourceStyle");
    REQUIRE(rollback.source_usage.has_value());
    REQUIRE_EQ(rollback.source_usage->hits.size(), 2U);
    rollback.source_usage->hits[1] = rollback.source_usage->hits[0];

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());

    const auto plan = document.plan_style_refactor_restore({rollback});
    REQUIRE(plan.has_value());
    CHECK_FALSE(plan->restored());
    REQUIRE_EQ(plan->operations.size(), 1U);
    REQUIRE_EQ(plan->operations[0].issues.size(), 1U);
    CHECK_EQ(plan->operations[0].issues[0].code, "duplicate_usage_hit");

    const auto restored = document.restore_style_refactor({rollback});
    REQUIRE(restored.has_value());
    CHECK_FALSE(restored->restored());
    CHECK_FALSE(restored->changed);
    REQUIRE_EQ(restored->operations.size(), 1U);
    REQUIRE_EQ(restored->operations[0].issues.size(), 1U);
    CHECK_EQ(restored->operations[0].issues[0].code, "duplicate_usage_hit");
    CHECK_FALSE(document.find_style("SourceStyle").has_value());
    const auto target_usage = document.find_style_usage("TargetStyle");
    REQUIRE(target_usage.has_value());
    CHECK_EQ(target_usage->paragraph_count, 2U);
    CHECK(paragraph.has_next());

    const auto save_error = document.save();
    INFO(save_error.message());
    INFO(document.last_error().detail);
    CHECK_FALSE(save_error);
    CHECK_EQ(read_test_docx_entry(target, "word/styles.xml"),
             original_styles_xml);

    fs::remove(target);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "style refactor restore is atomic across every pugi allocation "
    "failure") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "style_refactor_restore_allocation_failure.docx";
    fs::remove(target);

    const auto document_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:pPr><w:pStyle w:val="TargetStyle"/></w:pPr><w:r><w:t>first</w:t></w:r></w:p>
    <w:p><w:pPr><w:pStyle w:val="TargetStyle"/></w:pPr><w:r><w:t>second</w:t></w:r></w:p>
  </w:body>
</w:document>
)"};
    const auto styles_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/></w:style>
  <w:style w:type="paragraph" w:styleId="TargetStyle"><w:name w:val="Target Style"/></w:style>
</w:styles>
)"};
    write_test_docx_with_styles(target, document_xml, styles_xml);
    const auto original_styles_xml =
        read_test_docx_entry(target, "word/styles.xml");

    const auto rollback =
        make_large_style_restore_rollback(std::string(40'000U, 'S'));
    auto allocator_guard = style_restore_allocator_guard{};
    std::size_t successful_restore_allocation_count = 0U;
    {
        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        REQUIRE(document.find_style("TargetStyle").has_value());
        style_restore_allocation_calls = 0U;
        style_restore_failure_call = 0U;
        const auto restored = document.restore_style_refactor({rollback});
        REQUIRE(restored.has_value());
        REQUIRE(restored->restored());
        successful_restore_allocation_count = style_restore_allocation_calls;
        REQUIRE(document.find_style(rollback.source_style_id).has_value());
        const auto source_usage =
            document.find_style_usage(rollback.source_style_id);
        REQUIRE(source_usage.has_value());
        CHECK_EQ(source_usage->paragraph_count, 2U);
    }
    REQUIRE_GT(successful_restore_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_restore_allocation_count; ++failure_call) {
        style_restore_failure_call = 0U;
        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        REQUIRE(document.find_style("TargetStyle").has_value());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());

        style_restore_allocation_calls = 0U;
        style_restore_failure_call = failure_call;
        const auto restored = document.restore_style_refactor({rollback});
        style_restore_failure_call = 0U;

        CAPTURE(failure_call);
        CAPTURE(successful_restore_allocation_count);
        CAPTURE(style_restore_allocation_calls);
        CAPTURE(document.last_error().detail);
        CAPTURE(document.last_error().entry_name);
        CHECK_FALSE(restored.has_value());
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK_FALSE(document.last_error().entry_name.empty());
        CHECK_FALSE(document.find_style(rollback.source_style_id).has_value());
        const auto target_usage = document.find_style_usage("TargetStyle");
        REQUIRE(target_usage.has_value());
        CHECK_EQ(target_usage->paragraph_count, 2U);
        CHECK(paragraph.has_next());

        const auto save_error = document.save();
        INFO(save_error.message());
        INFO(document.last_error().detail);
        CHECK_FALSE(save_error);
        CHECK_EQ(read_test_docx_entry(target, "word/styles.xml"),
                 original_styles_xml);
    }

    fs::remove(target);
}
