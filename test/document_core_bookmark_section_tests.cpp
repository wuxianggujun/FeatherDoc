#include "document_core_unit_test_support.hpp"
#include "allocation_failure_test_case.hpp"
#include "basic_image_fixture_test_support.hpp"

TEST_CASE("replace_bookmark_text rewrites bookmarked content and preserves markers") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>prefix</w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="bookmark"/>
      <w:r><w:t>old</w:t></w:r>
      <w:proofErr w:type="spellStart"/>
      <w:r><w:t>content</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
      <w:r><w:t>suffix</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto paragraph_handle = doc.paragraphs();
    REQUIRE(paragraph_handle.valid());
    auto prefix_run = paragraph_handle.runs();
    REQUIRE(prefix_run.valid());
    auto removed_old_run = prefix_run;
    removed_old_run.next();
    REQUIRE(removed_old_run.valid());
    auto removed_content_run = removed_old_run;
    removed_content_run.next();
    REQUIRE(removed_content_run.valid());
    auto suffix_run = removed_content_run;
    suffix_run.next();
    REQUIRE(suffix_run.valid());

    CHECK_EQ(doc.replace_bookmark_text("bookmark", " updated value "), 1);
    CHECK_EQ(collect_document_text(doc), "prefix updated value suffix\n");
    CHECK(paragraph_handle.valid());
    CHECK(prefix_run.valid());
    CHECK_FALSE(removed_old_run.valid());
    CHECK_FALSE(removed_content_run.valid());
    CHECK(suffix_run.valid());

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(xml_text.find("w:bookmarkStart"), std::string::npos);
    CHECK_NE(xml_text.find("w:bookmarkEnd"), std::string::npos);
    CHECK_NE(xml_text.find("updated value"), std::string::npos);
    CHECK_NE(xml_text.find("xml:space=\"preserve\""), std::string::npos);
    CHECK_EQ(xml_text.find("old"), std::string::npos);
    CHECK_EQ(xml_text.find("content"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "prefix updated value suffix\n");

    fs::remove(target);
}

TEST_CASE("bookmark text replacement validates every duplicate before publishing") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "bookmark_text_duplicate_validation_atomic.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>before</w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="duplicate_text"/>
      <w:r><w:t>first placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="1" w:name="duplicate_text"/>
      <w:r><w:t>second placeholder</w:t></w:r>
    </w:p>
    <w:p>
      <w:bookmarkEnd w:id="1"/>
      <w:r><w:t>after</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    REQUIRE_FALSE(doc.open());

    auto first_paragraph = doc.paragraphs();
    auto first_prefix_run = first_paragraph.runs();
    auto first_placeholder_run = first_prefix_run;
    first_placeholder_run.next();
    auto second_paragraph = first_paragraph;
    second_paragraph.next();
    auto second_placeholder_run = second_paragraph.runs();
    auto third_paragraph = second_paragraph;
    third_paragraph.next();
    auto third_run = third_paragraph.runs();
    REQUIRE(first_paragraph.valid());
    REQUIRE(first_prefix_run.valid());
    REQUIRE(first_placeholder_run.valid());
    REQUIRE(second_paragraph.valid());
    REQUIRE(second_placeholder_run.valid());
    REQUIRE(third_paragraph.valid());
    REQUIRE(third_run.valid());

    const auto text_before = collect_document_text(doc);
    CHECK_EQ(doc.replace_bookmark_text("duplicate_text", "replacement"), 0U);
    CHECK(doc.last_error());
    CHECK_EQ(collect_document_text(doc), text_before);
    CHECK(first_paragraph.valid());
    CHECK(first_prefix_run.valid());
    CHECK(first_placeholder_run.valid());
    CHECK(second_paragraph.valid());
    CHECK(second_placeholder_run.valid());
    CHECK(third_paragraph.valid());
    CHECK(third_run.valid());

    fs::remove(target);
}

TEST_CASE("bookmark table replacement retires only the placeholder paragraph") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "bookmark_table_handle_retirement.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="table_slot"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto before_paragraph = doc.paragraphs();
    auto removed_placeholder = before_paragraph;
    removed_placeholder.next();
    auto removed_run = removed_placeholder.runs();
    auto after_paragraph = removed_placeholder;
    after_paragraph.next();
    REQUIRE(before_paragraph.valid());
    REQUIRE(removed_placeholder.valid());
    REQUIRE(removed_run.valid());
    REQUIRE(after_paragraph.valid());

    CHECK_EQ(doc.replace_bookmark_with_table(
                 "table_slot", {{"Name", "Qty"}, {"Apple", "2"}}),
             1U);
    CHECK_FALSE(doc.last_error());
    CHECK(before_paragraph.valid());
    CHECK_FALSE(removed_placeholder.valid());
    CHECK_FALSE(removed_run.valid());
    CHECK(after_paragraph.valid());

    auto replacement_table = doc.tables();
    REQUIRE(replacement_table.valid());
    CHECK_EQ(replacement_table.rows().cells().get_text(), "Name");

    fs::remove(target);
}

TEST_CASE("bookmark paragraph replacement validates every duplicate before publishing") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "bookmark_duplicate_validation_atomic.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="duplicate_slot"/>
      <w:r><w:t>first placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:r><w:t>prefix</w:t></w:r>
      <w:bookmarkStart w:id="1" w:name="duplicate_slot"/>
      <w:r><w:t>second placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto first_paragraph = doc.paragraphs();
    auto first_run = first_paragraph.runs();
    auto second_paragraph = first_paragraph;
    second_paragraph.next();
    auto second_prefix_run = second_paragraph.runs();
    auto second_placeholder_run = second_prefix_run;
    second_placeholder_run.next();
    REQUIRE(first_paragraph.valid());
    REQUIRE(first_run.valid());
    REQUIRE(second_paragraph.valid());
    REQUIRE(second_prefix_run.valid());
    REQUIRE(second_placeholder_run.valid());

    const auto text_before = collect_document_text(doc);
    CHECK_EQ(doc.replace_bookmark_with_paragraphs("duplicate_slot", {"new"}),
             0U);
    CHECK(doc.last_error());
    CHECK_EQ(collect_document_text(doc), text_before);
    CHECK(first_paragraph.valid());
    CHECK(first_run.valid());
    CHECK(second_paragraph.valid());
    CHECK(second_prefix_run.valid());
    CHECK(second_placeholder_run.valid());

    fs::remove(target);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "bookmark inline and floating image replacement roll back every package allocation failure") {
    namespace fs = std::filesystem;

    const auto unicode_directory =
        fs::current_path() / fs::path{u8"书签图片_😀_事务"};
    const auto image_path = unicode_directory / fs::path{u8"样例_🪶.png"};
    fs::remove_all(unicode_directory);
    fs::create_directories(unicode_directory);
    write_binary_file(image_path, tiny_png_data());

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="logo"/>
      <w:r><w:t>first placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="1" w:name="logo"/>
      <w:r><w:t>second placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
  </w:body>
</w:document>
)";

    pugi_memory_management_guard allocation_guard;
    delegated_pugi_allocate = allocation_guard.allocation;
    pugi::set_memory_management_functions(controlled_pugi_allocate,
                                          allocation_guard.deallocation);

    featherdoc::floating_image_options floating_options;
    floating_options.horizontal_offset_px = -7;
    floating_options.vertical_offset_px = 11;
    floating_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::square;
    floating_options.crop = featherdoc::floating_image_crop{10U, 20U, 30U,
                                                            40U};

    const auto replace_image = [&](featherdoc::Document &document,
                                   bool floating) {
        return floating
                   ? document.replace_bookmark_with_floating_image(
                         "logo", image_path, 20U, 10U, floating_options)
                   : document.replace_bookmark_with_image("logo", image_path,
                                                          20U, 10U);
    };

    for (const bool floating : {false, true}) {
        CAPTURE(floating);
        const auto mode_name = floating ? "floating" : "inline";
        const auto baseline_path =
            unicode_directory / (std::string{mode_name} + "_baseline.docx");
        fs::remove(baseline_path);
        write_test_docx(baseline_path, document_xml);
        {
            featherdoc::Document baseline(baseline_path);
            controlled_pugi_failure_call = 0U;
            REQUIRE_FALSE(baseline.open());
            REQUIRE_FALSE(baseline.save());
        }
        const auto baseline_document_xml =
            read_test_docx_entry(baseline_path, test_document_xml_entry);
        const auto baseline_content_types =
            read_test_docx_entry(baseline_path, test_content_types_xml_entry);
        const auto baseline_package_relationships =
            read_test_docx_entry(baseline_path, test_relationships_xml_entry);

        std::size_t successful_allocation_count = 0U;
        const auto successful_path =
            unicode_directory / (std::string{mode_name} + "_success.docx");
        fs::remove(successful_path);
        write_test_docx(successful_path, document_xml);
        {
            featherdoc::Document successful(successful_path);
            controlled_pugi_failure_call = 0U;
            REQUIRE_FALSE(successful.open());
            controlled_pugi_allocation_calls = 0U;
            REQUIRE_EQ(replace_image(successful, floating), 2U);
            successful_allocation_count = controlled_pugi_allocation_calls;
            REQUIRE_GT(successful_allocation_count, 0U);
        }

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            const auto target = unicode_directory /
                                (std::string{mode_name} + "_failure_" +
                                 std::to_string(failure_call) + ".docx");
            fs::remove(target);
            write_test_docx(target, document_xml);

            featherdoc::Document document(target);
            controlled_pugi_failure_call = 0U;
            REQUIRE_FALSE(document.open());
            auto placeholder_paragraph = document.paragraphs();
            auto placeholder_run = placeholder_paragraph.runs();
            auto second_placeholder_paragraph = placeholder_paragraph;
            second_placeholder_paragraph.next();
            auto second_placeholder_run =
                second_placeholder_paragraph.runs();
            REQUIRE(placeholder_paragraph.valid());
            REQUIRE(placeholder_run.valid());
            REQUIRE(second_placeholder_paragraph.valid());
            REQUIRE(second_placeholder_run.valid());

            controlled_pugi_allocation_calls = 0U;
            controlled_pugi_failure_call = failure_call;
            REQUIRE_EQ(replace_image(document, floating), 0U);
            const auto mutation_error = document.last_error();
            controlled_pugi_failure_call = 0U;
            CHECK_EQ(mutation_error.code,
                     std::make_error_code(std::errc::not_enough_memory));
            CHECK(placeholder_paragraph.valid());
            CHECK(placeholder_run.valid());
            CHECK(second_placeholder_paragraph.valid());
            CHECK(second_placeholder_run.valid());

            REQUIRE_FALSE(document.save());
            CHECK_EQ(read_test_docx_entry(target, test_document_xml_entry),
                     baseline_document_xml);
            CHECK_EQ(read_test_docx_entry(target,
                                          test_content_types_xml_entry),
                     baseline_content_types);
            CHECK_EQ(read_test_docx_entry(target,
                                          test_relationships_xml_entry),
                     baseline_package_relationships);
            CHECK_FALSE(test_docx_entry_exists(
                target, "word/_rels/document.xml.rels"));
            CHECK_FALSE(
                test_docx_entry_exists(target, "word/media/image1.png"));
            CHECK_FALSE(
                test_docx_entry_exists(target, "word/media/image2.png"));

            controlled_pugi_allocation_calls = 0U;
            REQUIRE_EQ(replace_image(document, floating), 2U);
            CHECK_FALSE(placeholder_paragraph.valid());
            CHECK_FALSE(placeholder_run.valid());
            CHECK_FALSE(second_placeholder_paragraph.valid());
            CHECK_FALSE(second_placeholder_run.valid());
            REQUIRE_FALSE(document.save());
            CHECK(test_docx_entry_exists(target,
                                         "word/_rels/document.xml.rels"));
            CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
            CHECK(test_docx_entry_exists(target, "word/media/image2.png"));

            featherdoc::Document reopened(target);
            REQUIRE_FALSE(reopened.open());
            const auto images = reopened.drawing_images();
            REQUIRE_EQ(images.size(), 2U);
            for (const auto &image : images) {
                CHECK_EQ(image.display_name,
                         utf8_from_u8(u8"样例_🪶.png"));
            }

            fs::remove(target);
        }

        fs::remove(baseline_path);
        fs::remove(successful_path);
    }

    fs::remove_all(unicode_directory);
}

TEST_CASE("existing header and footer paragraphs can be edited and saved") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_footer_access.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>body</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId2"/>
      <w:footerReference w:type="default" r:id="rId3"/>
    </w:sectPr>
  </w:body>
</w:document>
)";

    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
</Relationships>
)";

    const std::string header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>old header</w:t></w:r></w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>old footer</w:t></w:r></w:p>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header_xml},
            {"word/footer1.xml", footer_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.header_count(), 1);
    CHECK_EQ(doc.footer_count(), 1);

    auto header = doc.header_paragraphs();
    REQUIRE(header.has_next());
    auto header_run = header.runs();
    REQUIRE(header_run.has_next());
    CHECK(header_run.set_text("updated header"));

    auto footer = doc.footer_paragraphs();
    REQUIRE(footer.has_next());
    auto footer_run = footer.runs();
    REQUIRE(footer_run.has_next());
    CHECK(footer_run.set_text(" updated footer "));

    CHECK_FALSE(doc.save());

    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    CHECK_NE(saved_header.find("updated header"), std::string::npos);

    const auto saved_footer = read_test_docx_entry(target, "word/footer1.xml");
    CHECK_NE(saved_footer.find("updated footer"), std::string::npos);
    CHECK_NE(saved_footer.find("xml:space=\"preserve\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);

    auto reopened_header = reopened.header_paragraphs();
    REQUIRE(reopened_header.has_next());
    CHECK_EQ(reopened_header.runs().get_text(), "updated header");

    auto reopened_footer = reopened.footer_paragraphs();
    REQUIRE(reopened_footer.has_next());
    CHECK_EQ(reopened_footer.runs().get_text(), " updated footer ");

    CHECK_FALSE(reopened.header_paragraphs(1).has_next());
    CHECK_FALSE(reopened.footer_paragraphs(1).has_next());

    fs::remove(target);
}

TEST_CASE("ensure_header_paragraphs creates a default header for existing documents") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "ensure_header_existing.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>body text</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.section_count(), 1);
    CHECK_EQ(doc.header_count(), 0);

    auto header = doc.ensure_header_paragraphs();
    REQUIRE(header.has_next());
    CHECK(header.add_run("generated header").has_next());
    CHECK_EQ(doc.header_count(), 1);

    auto same_header = doc.ensure_header_paragraphs();
    REQUIRE(same_header.has_next());
    CHECK_EQ(doc.header_count(), 1);

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("xmlns:r="), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:headerReference"), std::string::npos);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("relationships/header"), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "body text\n");
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.header_paragraphs().runs().get_text(), "generated header");

    fs::remove(target);
}
