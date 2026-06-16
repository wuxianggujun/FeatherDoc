#include "document_core_unit_test_support.hpp"

TEST_CASE("paragraph remove deletes a middle body paragraph and keeps the wrapper usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_remove.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto first = doc.paragraphs();
    REQUIRE(first.has_next());
    REQUIRE(first.set_text("first"));

    auto removed = first.insert_paragraph_after("second");
    REQUIRE(removed.has_next());
    auto third = removed.insert_paragraph_after("third");
    REQUIRE(third.has_next());

    CHECK(removed.remove());
    CHECK(removed.has_next());
    CHECK_EQ(removed.runs().get_text(), "third");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "first\nthird\n");

    fs::remove(target);
}

TEST_CASE("paragraph remove rejects removing the last body paragraph") {
    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK_FALSE(paragraph.remove());
    CHECK(paragraph.has_next());
}

TEST_CASE("paragraph remove rejects removing the last paragraph in a table cell") {
    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    auto cell_paragraph = table.rows().cells().paragraphs();
    REQUIRE(cell_paragraph.has_next());
    CHECK_FALSE(cell_paragraph.remove());
    CHECK(cell_paragraph.has_next());
}

TEST_CASE("save_as rejects an empty output path") {
    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());

    CHECK_EQ(doc.save_as({}), featherdoc::document_errc::empty_path);
    CHECK_EQ(doc.last_error().code, featherdoc::document_errc::empty_path);
    CHECK_FALSE(doc.last_error().detail.empty());
}

TEST_CASE("open and save work with an absolute path outside the build directory") {
    namespace fs = std::filesystem;

    const fs::path temp_root =
        fs::temp_directory_path() / "FeatherDoc absolute path regression";
    const fs::path nested_dir = temp_root / "nested docs";
    const fs::path target = nested_dir / "absolute-open-test.docx";

    fs::create_directories(nested_dir);
    fs::copy_file("my_test.docx", target, fs::copy_options::overwrite_existing);

    featherdoc::Document doc(fs::absolute(target));
    CHECK_FALSE(doc.open());
    CHECK(doc.is_open());

    auto paragraph = doc.paragraphs();
    CHECK(paragraph.has_next());
    paragraph.add_run(" [absolute path edit]");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(fs::absolute(target));
    CHECK_FALSE(reopened.open());
    CHECK_NE(collect_document_text(reopened).find("[absolute path edit]"),
             std::string::npos);

    fs::remove_all(temp_root);
}

#if defined(_WIN32)
TEST_CASE("open succeeds while another process keeps the docx writable but shareable") {
    namespace fs = std::filesystem;

    const fs::path temp_root =
        fs::temp_directory_path() / "FeatherDoc share mode regression";
    const fs::path target = temp_root / "opened-by-word.docx";

    std::error_code filesystem_error;
    fs::remove_all(temp_root, filesystem_error);
    filesystem_error.clear();
    fs::create_directories(temp_root, filesystem_error);
    REQUIRE_FALSE(filesystem_error);
    fs::copy_file("my_test.docx", target, fs::copy_options::overwrite_existing,
                  filesystem_error);
    REQUIRE_FALSE(filesystem_error);

    const HANDLE writer_handle =
        CreateFileW(target.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(writer_handle != INVALID_HANDLE_VALUE);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK(doc.is_open());

    CHECK(CloseHandle(writer_handle) != 0);
    fs::remove_all(temp_root, filesystem_error);
}
#endif

TEST_CASE("set_text preserves xml:space when leading or trailing spaces exist") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "preserve_spaces.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    doc.paragraphs().add_run("marker");
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto paragraph = reopened.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.runs();
    REQUIRE(run.has_next());
    CHECK(run.set_text("  padded text  "));
    CHECK_FALSE(reopened.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto text_node =
        xml_document.child("w:document").child("w:body").child("w:p").child("w:r").child("w:t");
    REQUIRE(text_node != pugi::xml_node{});
    CHECK_EQ(std::string{text_node.attribute("xml:space").value()}, "preserve");
    CHECK_EQ(std::string{text_node.text().get()}, "  padded text  ");

    CHECK(run.set_text("plain text"));
    CHECK_FALSE(reopened.save());

    const auto plain_xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document plain_xml_document;
    REQUIRE(plain_xml_document.load_string(plain_xml_text.c_str()));

    const auto plain_text_node = plain_xml_document.child("w:document")
                                     .child("w:body")
                                     .child("w:p")
                                     .child("w:r")
                                     .child("w:t");
    REQUIRE(plain_text_node != pugi::xml_node{});
    CHECK_FALSE(plain_text_node.attribute("xml:space"));
    CHECK_EQ(std::string{plain_text_node.text().get()}, "plain text");

    CHECK(run.set_text("first line\nsecond line"));
    CHECK_FALSE(reopened.save());

    const auto multiline_xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document multiline_xml_document;
    REQUIRE(multiline_xml_document.load_string(multiline_xml_text.c_str()));

    const auto multiline_run =
        multiline_xml_document.child("w:document").child("w:body").child("w:p").child("w:r");
    REQUIRE(multiline_run != pugi::xml_node{});
    CHECK_EQ(std::string{multiline_run.child("w:t").text().get()}, "first line");
    const auto line_break = multiline_run.child("w:br");
    REQUIRE(line_break != pugi::xml_node{});
    const auto trailing_text = line_break.next_sibling();
    REQUIRE(trailing_text != pugi::xml_node{});
    CHECK_EQ(std::string_view{trailing_text.name()}, "w:t");
    CHECK_EQ(std::string{trailing_text.text().get()}, "second line");

    featherdoc::Document multiline_reopened(target);
    CHECK_FALSE(multiline_reopened.open());
    auto multiline_run_handle = multiline_reopened.paragraphs().runs();
    REQUIRE(multiline_run_handle.has_next());
    CHECK_EQ(multiline_run_handle.get_text(), "first line\nsecond line");

    fs::remove(target);
}

TEST_CASE("add_run creates a top-level paragraph when the document body has none") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_only_body.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tr>
        <w:tc>
          <w:p>
            <w:r><w:t>cell text</w:t></w:r>
          </w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto paragraph = doc.paragraphs();
    CHECK_FALSE(paragraph.has_next());
    auto run = paragraph.add_run("appended after table");
    CHECK(run.has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "appended after table\n");

    fs::remove(target);
}

TEST_CASE("paragraph iteration skips non-paragraph siblings and appends before sectPr") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_iteration_regression.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>first</w:t></w:r></w:p>
    <w:tbl>
      <w:tr>
        <w:tc>
          <w:p><w:r><w:t>table cell</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>second</w:t></w:r></w:p>
    <w:sectPr/>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    std::size_t paragraph_count = 0;
    for (auto paragraph = doc.paragraphs(); paragraph.has_next(); paragraph.next()) {
        ++paragraph_count;
    }
    CHECK_EQ(paragraph_count, 2);

    auto paragraph = doc.paragraphs();
    while (paragraph.has_next()) {
        paragraph.next();
    }
    paragraph.insert_paragraph_after("third");
    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    std::ostringstream child_order;
    const auto body = xml_document.child("w:document").child("w:body");
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        child_order << child.name() << '\n';
    }
    CHECK_EQ(child_order.str(), "w:p\nw:tbl\nw:p\nw:p\nw:sectPr\n");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "first\nsecond\nthird\n");

    fs::remove(target);
}

TEST_CASE("insert_paragraph_after saves cleanly from the document paragraph cursor") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_paragraph_after_regression.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto &paragraphs = doc.paragraphs();
    CHECK(paragraphs.has_next());

    const auto inserted =
        paragraphs.insert_paragraph_after("inserted after initial paragraph");
    CHECK(inserted.has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened),
             "\ninserted after initial paragraph\n");

    fs::remove(target);
}

TEST_CASE("insert_paragraph_before prepends a body paragraph and keeps the anchor usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_paragraph_before_body.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto anchor = doc.paragraphs();
    REQUIRE(anchor.has_next());
    CHECK(anchor.set_text("anchor"));

    auto inserted = anchor.insert_paragraph_before("before");
    REQUIRE(inserted.has_next());
    CHECK(inserted.add_run(" intro").has_next());
    CHECK(anchor.has_next());
    CHECK_EQ(anchor.runs().get_text(), "anchor");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before intro\nanchor\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:p"), 2U);
    CHECK_EQ(std::string_view{body_node.first_child().name()}, "w:p");

    fs::remove(target);
}

TEST_CASE("insert_paragraph_before works inside a table cell") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_paragraph_before_cell.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    REQUIRE(table.has_next());

    auto anchor = table.rows().cells().paragraphs();
    REQUIRE(anchor.has_next());
    CHECK(anchor.set_text("cell anchor"));

    auto inserted = anchor.insert_paragraph_before("cell before");
    REQUIRE(inserted.has_next());
    CHECK(inserted.add_run(" intro").has_next());
    CHECK(anchor.has_next());
    CHECK_EQ(anchor.runs().get_text(), "cell anchor");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "cell before intro\ncell anchor\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto cell_node = xml_document.child("w:document")
                               .child("w:body")
                               .child("w:tbl")
                               .child("w:tr")
                               .child("w:tc");
    REQUIRE(cell_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(cell_node, "w:p"), 2U);

    fs::remove(target);
}

TEST_CASE("insert_paragraph_like_before clones paragraph properties and keeps the anchor usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_paragraph_like_before_body.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto anchor = doc.paragraphs();
    REQUIRE(anchor.has_next());
    CHECK(doc.set_paragraph_style(anchor, "Heading1"));
    CHECK(anchor.set_bidi());
    CHECK(doc.set_paragraph_list(anchor, featherdoc::list_kind::bullet));
    CHECK(anchor.set_text("anchor"));

    auto inserted = anchor.insert_paragraph_like_before();
    REQUIRE(inserted.has_next());
    CHECK(inserted.set_text("before"));
    REQUIRE(inserted.bidi().has_value());
    CHECK(*inserted.bidi());
    CHECK(anchor.has_next());
    CHECK_EQ(anchor.runs().get_text(), "anchor");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml,
                                         "<w:pStyle w:val=\"Heading1\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:bidi"), 2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:numPr>"), 2U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nanchor\n");

    auto reopened_inserted = reopened.paragraphs();
    REQUIRE(reopened_inserted.has_next());
    REQUIRE(reopened_inserted.bidi().has_value());
    CHECK(*reopened_inserted.bidi());
    reopened_inserted.next();
    REQUIRE(reopened_inserted.has_next());
    REQUIRE(reopened_inserted.bidi().has_value());
    CHECK(*reopened_inserted.bidi());

    fs::remove(target);
}

TEST_CASE("insert_paragraph_like_after clones cell paragraph properties without copying body text") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_paragraph_like_after_cell.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto anchor = doc.append_table(1, 1).rows().cells().paragraphs();
    REQUIRE(anchor.has_next());
    CHECK(doc.set_paragraph_style(anchor, "Heading2"));
    CHECK(anchor.set_bidi());
    CHECK(anchor.set_text("cell anchor"));

    auto inserted = anchor.insert_paragraph_like_after();
    REQUIRE(inserted.has_next());
    CHECK(inserted.set_text("cell like"));
    REQUIRE(inserted.bidi().has_value());
    CHECK(*inserted.bidi());

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml,
                                         "<w:pStyle w:val=\"Heading2\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:bidi"), 2U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "cell anchor\ncell like\n");

    fs::remove(target);
}

TEST_CASE("insert_paragraph_like strips section breaks from cloned paragraph properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_paragraph_like_sectpr.docx";
    fs::remove(target);

    const auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:pgSz w:w="12240" w:h="15840"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>anchor</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t>tail</w:t></w:r>
    </w:p>
    <w:sectPr/>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto anchor = doc.paragraphs();
    REQUIRE(anchor.has_next());
    CHECK_EQ(anchor.runs().get_text(), "anchor");

    auto inserted = anchor.insert_paragraph_like_before();
    REQUIRE(inserted.has_next());
    CHECK(inserted.set_text("before"));
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:sectPr"), 2U);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    const auto body = xml_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});

    const auto inserted_paragraph = body.child("w:p");
    REQUIRE(inserted_paragraph != pugi::xml_node{});
    CHECK_EQ(inserted_paragraph.child("w:pPr").child("w:sectPr"), pugi::xml_node{});

    const auto anchor_paragraph = inserted_paragraph.next_sibling("w:p");
    REQUIRE(anchor_paragraph != pugi::xml_node{});
    CHECK_NE(anchor_paragraph.child("w:pPr").child("w:sectPr"), pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nanchor\ntail\n");

    fs::remove(target);
}

TEST_CASE("run iteration skips non-run siblings inside a paragraph") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "run_iteration_regression.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>alpha</w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="bookmark"/>
      <w:r><w:t>beta</w:t></w:r>
      <w:proofErr w:type="spellStart"/>
      <w:r><w:t>gamma</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    std::size_t run_count = 0;
    std::ostringstream text;
    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    for (auto run = paragraph.runs(); run.has_next(); run.next()) {
        ++run_count;
        text << run.get_text();
    }

    CHECK_EQ(run_count, 3);
    CHECK_EQ(text.str(), "alphabetagamma");

    fs::remove(target);
}
