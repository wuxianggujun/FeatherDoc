#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "allocation_failure_test_case.hpp"
#include "document_core_unit_test_support.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <initializer_list>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY && defined(_WIN32)
#error "Bookmark batch allocation-failure tests are Linux-only"
#endif

namespace {

constexpr auto document_relationships_entry =
    "word/_rels/document.xml.rels";
constexpr auto nested_visibility_header_entry = "word/header1.xml";
constexpr auto image_entry = "word/media/%E6%B7%B7%E5%90%88_%F0%9F%98%80.png";

const auto image_payload = std::string{"bookmark-sidecar-image-payload"};

const auto bookmark_content_types_xml = std::string{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Default Extension="png" ContentType="image/png"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)"};

const auto bookmark_document_relationships_xml = std::string{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rIdImage"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image"
                Target="media/%E6%B7%B7%E5%90%88_%F0%9F%98%80.png"/>
</Relationships>
)"};

auto inline_image_run_xml() -> std::string {
    return R"(<w:r><w:drawing><wp:inline>
      <wp:extent cx="9525" cy="9525"/>
      <wp:docPr id="1" name="混合图片 😀"/>
      <a:graphic><a:graphicData><pic:pic><pic:blipFill>
        <a:blip r:embed="rIdImage"/>
      </pic:blipFill></pic:pic></a:graphicData></a:graphic>
    </wp:inline></w:drawing></w:r>)";
}

auto floating_image_run_xml() -> std::string {
    return R"(<w:r><w:drawing><wp:anchor simplePos="0" relativeHeight="1"
      behindDoc="0" locked="0" layoutInCell="1" allowOverlap="1">
      <wp:simplePos x="0" y="0"/>
      <wp:positionH relativeFrom="column"><wp:posOffset>0</wp:posOffset></wp:positionH>
      <wp:positionV relativeFrom="paragraph"><wp:posOffset>0</wp:posOffset></wp:positionV>
      <wp:extent cx="9525" cy="9525"/>
      <wp:docPr id="2" name="浮动图片 日本語 🪶"/>
      <wp:wrapNone/>
      <a:graphic><a:graphicData><pic:pic><pic:blipFill>
        <a:blip r:embed="rIdImage"/>
      </pic:blipFill></pic:pic></a:graphicData></a:graphic>
    </wp:anchor></w:drawing></w:r>)";
}

auto text_batch_document_xml(bool malformed) -> std::string {
    auto xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"
            xmlns:wp="http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing"
            xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main"
            xmlns:pic="http://schemas.openxmlformats.org/drawingml/2006/picture">
  <w:body>
    <w:p>
      <w:r><w:t>前缀</w:t></w:r>
      <w:bookmarkStart w:id="1" w:name="客户_😀"/>
      <w:r><w:t>旧客户一</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
      <w:r><w:t>后缀</w:t></w:r>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="2" w:name="图像_日本語"/>
)"};
    xml += inline_image_run_xml();
    xml += floating_image_run_xml();
    xml += R"(
      <w:bookmarkEnd w:id="2"/>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="3" w:name="客户_😀"/>
      <w:r><w:t>旧客户二</w:t></w:r>
      <w:bookmarkEnd w:id="3"/>
    </w:p>
)";
    if (malformed) {
        xml += R"(
    <w:p>
      <w:bookmarkStart w:id="4" w:name="损坏_🚫"/>
      <w:r><w:t>缺少结束标记</w:t></w:r>
    </w:p>
)";
    }
    xml += R"(
  </w:body>
</w:document>
)";
    return xml;
}

auto visibility_batch_document_xml(bool malformed) -> std::string {
    auto xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"
            xmlns:wp="http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing"
            xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main"
            xmlns:pic="http://schemas.openxmlformats.org/drawingml/2006/picture">
  <w:body>
    <w:p><w:bookmarkStart w:id="10" w:name="保留块_😀"/></w:p>
    <w:p><w:r><w:t>保留段落</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="10"/></w:p>
    <w:p><w:bookmarkStart w:id="11" w:name="隐藏块_日本語"/></w:p>
    <w:tbl>
      <w:tr><w:tc><w:p><w:r><w:t>隐藏表格</w:t></w:r></w:p></w:tc></w:tr>
    </w:tbl>
    <w:p>)"};
    xml += inline_image_run_xml();
    xml += floating_image_run_xml();
    xml += R"(</w:p>
    <w:p><w:bookmarkEnd w:id="11"/></w:p>
)";
    if (malformed) {
        xml += R"(
    <w:p><w:bookmarkStart w:id="12" w:name="损坏块_🚫"/></w:p>
    <w:p><w:r><w:t>缺少结束标记</w:t></w:r></w:p>
)";
    }
    xml += R"(
  </w:body>
</w:document>
)";
    return xml;
}

auto nested_visibility_blocks_xml(bool crossing) -> std::string {
    if (crossing) {
        return R"(
    <w:p><w:bookmarkStart w:id="30" w:name="隐藏交叉块_😀"/></w:p>
    <w:p><w:r><w:t>隐藏区间前文 中文</w:t></w:r></w:p>
    <w:p><w:bookmarkStart w:id="31" w:name="保留交叉块_日本語_🪶"/></w:p>
    <w:p><w:r><w:t>交叉内容 中文 😀</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="30"/></w:p>
    <w:p><w:r><w:t>保留区间后文 日本語</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="31"/></w:p>
    <w:p><w:r><w:t>交叉尾部保持 🪶</w:t></w:r></w:p>
)";
    }

    return R"(
    <w:p><w:bookmarkStart w:id="20" w:name="外层块_😀"/></w:p>
    <w:p><w:r><w:t>外层前文 中文</w:t></w:r></w:p>
    <w:p><w:bookmarkStart w:id="21" w:name="内层块_日本語_🪶"/></w:p>
    <w:p><w:r><w:t>内层内容 中文 😀</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="21"/></w:p>
    <w:p><w:r><w:t>外层后文 日本語</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="20"/></w:p>
    <w:p><w:r><w:t>嵌套尾部保持 🪶</w:t></w:r></w:p>
)";
}

auto nested_visibility_document_xml(bool crossing) -> std::string {
    auto xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
)"};
    xml += nested_visibility_blocks_xml(crossing);
    xml += R"(
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rIdHeader"/>
    </w:sectPr>
  </w:body>
</w:document>
)";
    return xml;
}

auto nested_visibility_header_xml(bool crossing) -> std::string {
    auto xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
)"};
    xml += nested_visibility_blocks_xml(crossing);
    xml += R"(
</w:hdr>
)";
    return xml;
}

auto write_nested_visibility_fixture(const std::filesystem::path &path,
                                     bool crossing) -> void {
    const auto content_types_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
</Types>
)"};
    const auto relationships_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rIdHeader"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
</Relationships>
)"};
    write_test_archive_entries(
        path,
        {{test_content_types_xml_entry, content_types_xml},
         {test_relationships_xml_entry, test_relationships_xml},
         {test_document_xml_entry,
          nested_visibility_document_xml(crossing)},
         {document_relationships_entry, relationships_xml},
         {nested_visibility_header_entry,
          nested_visibility_header_xml(crossing)}});
}

auto write_bookmark_fixture(const std::filesystem::path &path,
                            std::string document_xml) -> void {
    write_test_archive_entries(
        path, {{test_content_types_xml_entry, bookmark_content_types_xml},
               {test_relationships_xml_entry, test_relationships_xml},
               {test_document_xml_entry, std::move(document_xml)},
               {document_relationships_entry,
                bookmark_document_relationships_xml},
               {image_entry, image_payload}});
}

auto normalize_bookmark_fixture(const std::filesystem::path &path) -> void {
    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open());
    REQUIRE_FALSE(document.save());
}

struct package_snapshot final {
    std::string document_xml;
    std::string content_types_xml;
    std::string package_relationships_xml;
    std::string document_relationships_xml;
    std::string image;
};

auto snapshot_package(const std::filesystem::path &path) -> package_snapshot {
    return {read_test_docx_entry(path, test_document_xml_entry),
            read_test_docx_entry(path, test_content_types_xml_entry),
            read_test_docx_entry(path, test_relationships_xml_entry),
            read_test_docx_entry(path, document_relationships_entry),
            read_test_docx_entry(path, image_entry)};
}

auto check_package_equals(const std::filesystem::path &path,
                          const package_snapshot &expected) -> void {
    CHECK_EQ(read_test_docx_entry(path, test_document_xml_entry),
             expected.document_xml);
    CHECK_EQ(read_test_docx_entry(path, test_content_types_xml_entry),
             expected.content_types_xml);
    CHECK_EQ(read_test_docx_entry(path, test_relationships_xml_entry),
             expected.package_relationships_xml);
    CHECK_EQ(read_test_docx_entry(path, document_relationships_entry),
             expected.document_relationships_xml);
    CHECK_EQ(read_test_docx_entry(path, image_entry), expected.image);
}

using archive_entry_snapshot = std::pair<std::string, std::string>;

auto snapshot_archive_entries(
    const std::filesystem::path &path,
    std::initializer_list<std::string_view> entry_names)
    -> std::vector<archive_entry_snapshot> {
    auto snapshot = std::vector<archive_entry_snapshot>{};
    snapshot.reserve(entry_names.size());
    for (const auto entry_name : entry_names) {
        snapshot.emplace_back(
            std::string{entry_name},
            read_test_docx_entry(path, std::string{entry_name}.c_str()));
    }
    return snapshot;
}

auto check_archive_entries_equal(
    const std::filesystem::path &path,
    const std::vector<archive_entry_snapshot> &expected) -> void {
    for (const auto &[entry_name, content] : expected) {
        CHECK_EQ(read_test_docx_entry(path, entry_name.c_str()), content);
    }
}

struct part_text_handles final {
    std::vector<featherdoc::Paragraph> paragraphs;
    std::vector<featherdoc::Run> runs;
};

template <typename Part>
auto capture_part_text_handles(Part &part) -> part_text_handles {
    auto handles = part_text_handles{};
    for (auto paragraph = part.paragraphs(); paragraph.has_next();
         paragraph.next()) {
        handles.paragraphs.push_back(paragraph);
        for (auto run = paragraph.runs(); run.has_next(); run.next()) {
            handles.runs.push_back(run);
        }
    }
    REQUIRE_FALSE(handles.paragraphs.empty());
    REQUIRE_FALSE(handles.runs.empty());
    return handles;
}

auto check_part_text_handles_valid(const part_text_handles &handles) -> void {
    for (const auto &paragraph : handles.paragraphs) {
        CHECK(paragraph.valid());
    }
    for (const auto &run : handles.runs) {
        CHECK(run.valid());
    }
}

auto conflicting_visibility_bindings(bool crossing, bool reverse)
    -> std::vector<featherdoc::bookmark_block_visibility_binding> {
    auto bindings = crossing
                        ? std::vector<
                              featherdoc::bookmark_block_visibility_binding>{
                              {"隐藏交叉块_😀", false},
                              {"保留交叉块_日本語_🪶", true}}
                        : std::vector<
                              featherdoc::bookmark_block_visibility_binding>{
                              {"外层块_😀", false},
                              {"内层块_日本語_🪶", true}};
    if (reverse) {
        std::reverse(bindings.begin(), bindings.end());
    }
    return bindings;
}

auto safe_nested_visibility_bindings(bool reverse)
    -> std::vector<featherdoc::bookmark_block_visibility_binding> {
    auto bindings =
        std::vector<featherdoc::bookmark_block_visibility_binding>{
            {"外层块_😀", true}, {"内层块_日本語_🪶", false}};
    if (reverse) {
        std::reverse(bindings.begin(), bindings.end());
    }
    return bindings;
}

constexpr std::size_t large_bookmark_row_column_count = 24U;

auto large_bookmark_table_row_document_xml() -> std::string {
    auto xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
)"};
    for (std::size_t occurrence = 0U; occurrence < 2U; ++occurrence) {
        xml += "    <w:tbl><w:tblGrid>\n";
        for (std::size_t column = 0U;
             column < large_bookmark_row_column_count; ++column) {
            xml += "      <w:gridCol w:w=\"1200\"/>\n";
        }
        xml += "    </w:tblGrid><w:tr><w:trPr><w:cantSplit/>"
               "</w:trPr>\n";
        for (std::size_t column = 0U;
             column < large_bookmark_row_column_count; ++column) {
            xml +=
                "      <w:tc><w:tcPr><w:tcW w:w=\"1200\" "
                "w:type=\"dxa\"/><w:shd w:fill=\"AABBCC\"/></w:tcPr>"
                "<w:p><w:pPr><w:jc w:val=\"center\"/></w:pPr>";
            if (column == 0U) {
                xml += "<w:bookmarkStart w:id=\"" +
                       std::to_string(100U + occurrence) +
                       "\" w:name=\"大行模板_中文_😀\"/>";
            }
            xml +=
                "<w:r><w:rPr><w:b/><w:color w:val=\"336699\"/>"
                "</w:rPr><w:t>";
            xml.append(2048U,
                       static_cast<char>('A' + (column % 26U)));
            xml += " 中文_日本語_🪶_" + std::to_string(occurrence) + "_" +
                   std::to_string(column) + "</w:t></w:r>";
            if (column == 0U) {
                xml += "<w:bookmarkEnd w:id=\"" +
                       std::to_string(100U + occurrence) + "\"/>";
            }
            xml += "</w:p></w:tc>\n";
        }
        xml += "    </w:tr></w:tbl>\n";
    }
    xml += R"(  </w:body>
</w:document>
)";
    return xml;
}

auto large_bookmark_row_replacements()
    -> std::vector<std::vector<std::string>> {
    auto rows = std::vector<std::vector<std::string>>(2U);
    for (std::size_t row = 0U; row < rows.size(); ++row) {
        rows[row].reserve(large_bookmark_row_column_count);
        for (std::size_t column = 0U;
             column < large_bookmark_row_column_count; ++column) {
            rows[row].push_back("新行" + std::to_string(row) + "_列" +
                                std::to_string(column) +
                                " 中文 😀\n日本語 🪶");
        }
    }
    return rows;
}

struct table_tree_handles final {
    std::vector<featherdoc::Table> tables;
    std::vector<featherdoc::TableRow> rows;
    std::vector<featherdoc::TableCell> cells;
    std::vector<featherdoc::Paragraph> paragraphs;
    std::vector<featherdoc::Run> runs;
};

template <typename Part>
auto capture_table_tree_handles(Part &part) -> table_tree_handles {
    auto handles = table_tree_handles{};
    for (auto table = part.tables(); table.has_next(); table.next()) {
        handles.tables.push_back(table);
        for (auto row = table.rows(); row.has_next(); row.next()) {
            handles.rows.push_back(row);
            for (auto cell = row.cells(); cell.has_next(); cell.next()) {
                handles.cells.push_back(cell);
                for (auto paragraph = cell.paragraphs(); paragraph.has_next();
                     paragraph.next()) {
                    handles.paragraphs.push_back(paragraph);
                    for (auto run = paragraph.runs(); run.has_next();
                         run.next()) {
                        handles.runs.push_back(run);
                    }
                }
            }
        }
    }
    REQUIRE_EQ(handles.tables.size(), 2U);
    REQUIRE_EQ(handles.rows.size(), 2U);
    REQUIRE_EQ(handles.cells.size(),
               2U * large_bookmark_row_column_count);
    REQUIRE_EQ(handles.paragraphs.size(), handles.cells.size());
    REQUIRE_EQ(handles.runs.size(), handles.cells.size());
    return handles;
}

auto check_table_tree_handles_valid(const table_tree_handles &handles) -> void {
    for (const auto &table : handles.tables) {
        CHECK(table.valid());
    }
    for (const auto &row : handles.rows) {
        CHECK(row.valid());
    }
    for (const auto &cell : handles.cells) {
        CHECK(cell.valid());
    }
    for (const auto &paragraph : handles.paragraphs) {
        CHECK(paragraph.valid());
    }
    for (const auto &run : handles.runs) {
        CHECK(run.valid());
    }
}

struct text_batch_handles final {
    featherdoc::Paragraph first_paragraph;
    featherdoc::Run prefix_run;
    featherdoc::Run first_replaced_run;
    featherdoc::Run suffix_run;
    featherdoc::Paragraph image_paragraph;
    featherdoc::Run image_run;
    featherdoc::Run floating_image_run;
    featherdoc::Paragraph duplicate_paragraph;
    featherdoc::Run duplicate_replaced_run;
};

auto capture_text_batch_handles(featherdoc::Document &document)
    -> text_batch_handles {
    auto first_paragraph = document.paragraphs();
    auto prefix_run = first_paragraph.runs();
    auto first_replaced_run = prefix_run;
    first_replaced_run.next();
    auto suffix_run = first_replaced_run;
    suffix_run.next();
    auto image_paragraph = first_paragraph;
    image_paragraph.next();
    auto image_run = image_paragraph.runs();
    auto floating_image_run = image_run;
    floating_image_run.next();
    auto duplicate_paragraph = image_paragraph;
    duplicate_paragraph.next();
    auto duplicate_replaced_run = duplicate_paragraph.runs();

    REQUIRE(first_paragraph.valid());
    REQUIRE(prefix_run.valid());
    REQUIRE(first_replaced_run.valid());
    REQUIRE(suffix_run.valid());
    REQUIRE(image_paragraph.valid());
    REQUIRE(image_run.valid());
    REQUIRE(floating_image_run.valid());
    REQUIRE(duplicate_paragraph.valid());
    REQUIRE(duplicate_replaced_run.valid());
    return {first_paragraph,          prefix_run,
            first_replaced_run,      suffix_run,
            image_paragraph,         image_run,
            floating_image_run,
            duplicate_paragraph,     duplicate_replaced_run};
}

auto check_text_batch_handles_valid(const text_batch_handles &handles) -> void {
    CHECK(handles.first_paragraph.valid());
    CHECK(handles.prefix_run.valid());
    CHECK(handles.first_replaced_run.valid());
    CHECK(handles.suffix_run.valid());
    CHECK(handles.image_paragraph.valid());
    CHECK(handles.image_run.valid());
    CHECK(handles.floating_image_run.valid());
    CHECK(handles.duplicate_paragraph.valid());
    CHECK(handles.duplicate_replaced_run.valid());
}

auto check_text_batch_success(const text_batch_handles &handles) -> void {
    CHECK(handles.first_paragraph.valid());
    CHECK(handles.prefix_run.valid());
    CHECK_FALSE(handles.first_replaced_run.valid());
    CHECK(handles.suffix_run.valid());
    CHECK(handles.image_paragraph.valid());
    CHECK_FALSE(handles.image_run.valid());
    CHECK_FALSE(handles.floating_image_run.valid());
    CHECK(handles.duplicate_paragraph.valid());
    CHECK_FALSE(handles.duplicate_replaced_run.valid());
}

auto make_text_bindings()
    -> std::vector<featherdoc::bookmark_text_binding> {
    auto long_replacement = std::string(40U * 1024U, 'X');
    long_replacement += " 中文 😀";
    return {{"客户_😀", std::move(long_replacement)},
            {"图像_日本語", "图片已替换 🪶"}};
}

struct visibility_batch_handles final {
    featherdoc::Paragraph kept_start;
    featherdoc::Paragraph kept_content;
    featherdoc::Paragraph kept_end;
    featherdoc::Paragraph hidden_start;
    featherdoc::Table hidden_table;
    featherdoc::Paragraph hidden_image;
    featherdoc::Run hidden_image_run;
    featherdoc::Run hidden_floating_image_run;
    featherdoc::Paragraph hidden_end;
};

auto capture_visibility_batch_handles(featherdoc::Document &document)
    -> visibility_batch_handles {
    auto kept_start = document.paragraphs();
    auto kept_content = kept_start;
    kept_content.next();
    auto kept_end = kept_content;
    kept_end.next();
    auto hidden_start = kept_end;
    hidden_start.next();
    auto hidden_image = hidden_start;
    hidden_image.next();
    auto hidden_image_run = hidden_image.runs();
    auto hidden_floating_image_run = hidden_image_run;
    hidden_floating_image_run.next();
    auto hidden_end = hidden_image;
    hidden_end.next();
    auto hidden_table = document.tables();

    REQUIRE(kept_start.valid());
    REQUIRE(kept_content.valid());
    REQUIRE(kept_end.valid());
    REQUIRE(hidden_start.valid());
    REQUIRE(hidden_table.valid());
    REQUIRE(hidden_image.valid());
    REQUIRE(hidden_image_run.valid());
    REQUIRE(hidden_floating_image_run.valid());
    REQUIRE(hidden_end.valid());
    return {kept_start,       kept_content, kept_end,      hidden_start,
            hidden_table, hidden_image, hidden_image_run,
            hidden_floating_image_run, hidden_end};
}

auto check_visibility_handles_valid(
    const visibility_batch_handles &handles) -> void {
    CHECK(handles.kept_start.valid());
    CHECK(handles.kept_content.valid());
    CHECK(handles.kept_end.valid());
    CHECK(handles.hidden_start.valid());
    CHECK(handles.hidden_table.valid());
    CHECK(handles.hidden_image.valid());
    CHECK(handles.hidden_image_run.valid());
    CHECK(handles.hidden_floating_image_run.valid());
    CHECK(handles.hidden_end.valid());
}

auto check_visibility_success(const visibility_batch_handles &handles) -> void {
    CHECK_FALSE(handles.kept_start.valid());
    CHECK(handles.kept_content.valid());
    CHECK_FALSE(handles.kept_end.valid());
    CHECK_FALSE(handles.hidden_start.valid());
    CHECK_FALSE(handles.hidden_table.valid());
    CHECK_FALSE(handles.hidden_image.valid());
    CHECK_FALSE(handles.hidden_image_run.valid());
    CHECK_FALSE(handles.hidden_floating_image_run.valid());
    CHECK_FALSE(handles.hidden_end.valid());
}

auto visibility_bindings()
    -> std::vector<featherdoc::bookmark_block_visibility_binding> {
    return {{"保留块_😀", true}, {"隐藏块_日本語", false}};
}

std::atomic_bool global_allocation_window_enabled{false};
std::atomic_size_t global_allocation_calls{0U};
std::atomic_size_t global_failure_call{0U};

auto should_fail_global_allocation() noexcept -> bool {
    if (!global_allocation_window_enabled.load(std::memory_order_relaxed)) {
        return false;
    }
    const auto call =
        global_allocation_calls.fetch_add(1U, std::memory_order_relaxed) + 1U;
    const auto failure = global_failure_call.load(std::memory_order_relaxed);
    return failure != 0U && call == failure;
}

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY
auto allocate_test_memory(std::size_t size) -> void * {
    if (should_fail_global_allocation()) {
        throw std::bad_alloc{};
    }
    if (auto *memory = std::malloc(size == 0U ? 1U : size)) {
        return memory;
    }
    throw std::bad_alloc{};
}

auto allocate_aligned_test_memory(std::size_t size, std::size_t alignment)
    -> void * {
    if (should_fail_global_allocation()) {
        throw std::bad_alloc{};
    }
    void *memory = nullptr;
    if (posix_memalign(&memory, alignment, size == 0U ? 1U : size) == 0) {
        return memory;
    }
    throw std::bad_alloc{};
}
#endif

class global_allocation_window final {
public:
    explicit global_allocation_window(std::size_t failure_call) noexcept {
        global_allocation_calls.store(0U, std::memory_order_relaxed);
        global_failure_call.store(failure_call, std::memory_order_relaxed);
        global_allocation_window_enabled.store(true, std::memory_order_release);
    }

    global_allocation_window(const global_allocation_window &) = delete;
    auto operator=(const global_allocation_window &)
        -> global_allocation_window & = delete;

    ~global_allocation_window() {
        global_allocation_window_enabled.store(false,
                                                std::memory_order_release);
    }
};

} // namespace

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY
void *operator new(std::size_t size) { return allocate_test_memory(size); }
void *operator new[](std::size_t size) { return allocate_test_memory(size); }
void *operator new(std::size_t size, std::align_val_t alignment) {
    return allocate_aligned_test_memory(size,
                                        static_cast<std::size_t>(alignment));
}
void *operator new[](std::size_t size, std::align_val_t alignment) {
    return allocate_aligned_test_memory(size,
                                        static_cast<std::size_t>(alignment));
}
void operator delete(void *memory) noexcept { std::free(memory); }
void operator delete[](void *memory) noexcept { std::free(memory); }
void operator delete(void *memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void *memory, std::size_t) noexcept {
    std::free(memory);
}
void operator delete(void *memory, std::align_val_t) noexcept {
    std::free(memory);
}
void operator delete[](void *memory, std::align_val_t) noexcept {
    std::free(memory);
}
void operator delete(void *memory, std::size_t, std::align_val_t) noexcept {
    std::free(memory);
}
void operator delete[](void *memory, std::size_t,
                       std::align_val_t) noexcept {
    std::free(memory);
}
#endif

TEST_CASE("bookmark text batch rejects a later malformed binding atomically") {
    namespace fs = std::filesystem;
    const auto directory =
        fs::current_path() / fs::path{u8"书签批处理_😀_结构错误"};
    const auto target = directory / fs::path{u8"文本_中文_日本語.docx"};
    fs::remove_all(directory);
    fs::create_directories(directory);
    write_bookmark_fixture(target, text_batch_document_xml(true));
    normalize_bookmark_fixture(target);
    const auto before = snapshot_package(target);

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    const auto handles = capture_text_batch_handles(document);
    const auto duplicate_result = document.fill_bookmarks(
        {{"客户_😀", "不应提交一"},
         {"图像_日本語", "不应提交二"},
         {"客户_😀", "重复绑定"}});
    CHECK_EQ(duplicate_result.requested, 3U);
    CHECK_EQ(duplicate_result.matched, 0U);
    CHECK_EQ(duplicate_result.replaced, 0U);
    CHECK(duplicate_result.missing_bookmarks.empty());
    CHECK(document.last_error());
    CHECK_EQ(document.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    check_text_batch_handles_valid(handles);

    const auto result = document.fill_bookmarks(
        {{"客户_😀", "不应提交"}, {"损坏_🚫", "结构错误"}});
    CHECK_EQ(result.requested, 2U);
    CHECK_EQ(result.matched, 0U);
    CHECK_EQ(result.replaced, 0U);
    CHECK(document.last_error());
    CHECK_EQ(document.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    check_text_batch_handles_valid(handles);
    REQUIRE_FALSE(document.save());
    check_package_equals(target, before);
    fs::remove_all(directory);
}

TEST_CASE(
    "bookmark visibility batch rejects a later malformed binding atomically") {
    namespace fs = std::filesystem;
    const auto directory =
        fs::current_path() / fs::path{u8"书签批处理_😀_块结构错误"};
    const auto target = directory / fs::path{u8"块_中文_日本語.docx"};
    fs::remove_all(directory);
    fs::create_directories(directory);
    write_bookmark_fixture(target, visibility_batch_document_xml(true));
    normalize_bookmark_fixture(target);
    const auto before = snapshot_package(target);

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    const auto handles = capture_visibility_batch_handles(document);
    const auto duplicate_result = document.apply_bookmark_block_visibility(
        {{"保留块_😀", false},
         {"隐藏块_日本語", false},
         {"保留块_😀", true}});
    CHECK_EQ(duplicate_result.requested, 3U);
    CHECK_EQ(duplicate_result.matched, 0U);
    CHECK_EQ(duplicate_result.kept, 0U);
    CHECK_EQ(duplicate_result.removed, 0U);
    CHECK(duplicate_result.missing_bookmarks.empty());
    CHECK(document.last_error());
    CHECK_EQ(document.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    check_visibility_handles_valid(handles);

    const auto result = document.apply_bookmark_block_visibility(
        {{"保留块_😀", false}, {"损坏块_🚫", false}});
    CHECK_EQ(result.requested, 2U);
    CHECK_EQ(result.matched, 0U);
    CHECK_EQ(result.kept, 0U);
    CHECK_EQ(result.removed, 0U);
    CHECK(document.last_error());
    CHECK_EQ(document.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    check_visibility_handles_valid(handles);
    REQUIRE_FALSE(document.save());
    check_package_equals(target, before);
    fs::remove_all(directory);
}

TEST_CASE(
    "bookmark visibility batch rejects hidden ranges that cover or cross visible ranges atomically") {
    namespace fs = std::filesystem;
    const auto directory =
        fs::current_path() / fs::path{u8"书签嵌套冲突_中文_日本語_😀"};
    fs::remove_all(directory);
    fs::create_directories(directory);

    pugi_memory_management_guard allocation_guard;
    delegated_pugi_allocate = allocation_guard.allocation;
    pugi::set_memory_management_functions(controlled_pugi_allocate,
                                          allocation_guard.deallocation);

    for (const bool crossing : {false, true}) {
        for (const bool reverse : {false, true}) {
            CAPTURE(crossing);
            CAPTURE(reverse);
            const auto target =
                directory /
                (std::string{crossing ? "交叉" : "覆盖"} +
                 (reverse ? "_反序" : "_正序") + "_😀.docx");
            write_nested_visibility_fixture(target, crossing);
            normalize_bookmark_fixture(target);
            const auto before = snapshot_archive_entries(
                target,
                {test_content_types_xml_entry, test_relationships_xml_entry,
                 test_document_xml_entry, document_relationships_entry,
                 nested_visibility_header_entry});

            featherdoc::Document document(target);
            REQUIRE_FALSE(document.open());
            auto header = document.section_header_template(0U);
            REQUIRE(static_cast<bool>(header));
            const auto document_handles =
                capture_part_text_handles(document);
            const auto header_handles = capture_part_text_handles(header);
            const auto bindings =
                conflicting_visibility_bindings(crossing, reverse);

            controlled_pugi_allocation_calls = 0U;
            controlled_pugi_failure_call = 1U;
            const auto document_result =
                document.apply_bookmark_block_visibility(bindings);
            const auto document_pugi_allocations =
                controlled_pugi_allocation_calls;
            controlled_pugi_failure_call = 0U;
            CHECK_EQ(document_pugi_allocations, 0U);
            CHECK_EQ(document_result.requested, 2U);
            CHECK_EQ(document_result.matched, 0U);
            CHECK_EQ(document_result.kept, 0U);
            CHECK_EQ(document_result.removed, 0U);
            CHECK(document_result.missing_bookmarks.empty());
            REQUIRE(document.last_error());
            CHECK_EQ(document.last_error().code,
                     std::make_error_code(std::errc::invalid_argument));
            CHECK_EQ(document.last_error().entry_name,
                     test_document_xml_entry);
            CHECK_NE(document.last_error().detail.find("visibility conflict"),
                     std::string::npos);
            check_part_text_handles_valid(document_handles);
            check_part_text_handles_valid(header_handles);

            controlled_pugi_allocation_calls = 0U;
            controlled_pugi_failure_call = 1U;
            const auto header_result =
                header.apply_bookmark_block_visibility(bindings);
            const auto header_pugi_allocations =
                controlled_pugi_allocation_calls;
            controlled_pugi_failure_call = 0U;
            CHECK_EQ(header_pugi_allocations, 0U);
            CHECK_EQ(header_result.requested, 2U);
            CHECK_EQ(header_result.matched, 0U);
            CHECK_EQ(header_result.kept, 0U);
            CHECK_EQ(header_result.removed, 0U);
            CHECK(header_result.missing_bookmarks.empty());
            REQUIRE(document.last_error());
            CHECK_EQ(document.last_error().code,
                     std::make_error_code(std::errc::invalid_argument));
            CHECK_EQ(document.last_error().entry_name,
                     nested_visibility_header_entry);
            CHECK_NE(document.last_error().detail.find("visibility conflict"),
                     std::string::npos);
            check_part_text_handles_valid(document_handles);
            check_part_text_handles_valid(header_handles);

            REQUIRE_FALSE(document.save());
            check_archive_entries_equal(target, before);
        }
    }

    fs::remove_all(directory);
}

TEST_CASE(
    "bookmark visibility batch allows an outer visible range with an inner hidden range") {
    namespace fs = std::filesystem;
    const auto directory =
        fs::current_path() / fs::path{u8"书签安全嵌套_中文_日本語_😀"};
    fs::remove_all(directory);
    fs::create_directories(directory);

    for (const bool reverse : {false, true}) {
        CAPTURE(reverse);
        const auto target =
            directory /
            (std::string{reverse ? "安全嵌套_反序_" : "安全嵌套_正序_"} +
             "🪶.docx");
        write_nested_visibility_fixture(target, false);
        normalize_bookmark_fixture(target);

        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        auto header = document.section_header_template(0U);
        REQUIRE(static_cast<bool>(header));
        const auto bindings = safe_nested_visibility_bindings(reverse);

        const auto document_result =
            document.apply_bookmark_block_visibility(bindings);
        REQUIRE_EQ(document_result.requested, 2U);
        REQUIRE_EQ(document_result.matched, 2U);
        REQUIRE_EQ(document_result.kept, 1U);
        REQUIRE_EQ(document_result.removed, 1U);
        CHECK(document_result.missing_bookmarks.empty());
        CHECK_FALSE(document.last_error());

        const auto header_result =
            header.apply_bookmark_block_visibility(bindings);
        REQUIRE_EQ(header_result.requested, 2U);
        REQUIRE_EQ(header_result.matched, 2U);
        REQUIRE_EQ(header_result.kept, 1U);
        REQUIRE_EQ(header_result.removed, 1U);
        CHECK(header_result.missing_bookmarks.empty());
        CHECK_FALSE(document.last_error());

        REQUIRE_FALSE(document.save());
        for (const auto entry_name : {test_document_xml_entry,
                                      nested_visibility_header_entry}) {
            const auto saved = read_test_docx_entry(target, entry_name);
            CHECK_NE(saved.find("外层前文 中文"), std::string::npos);
            CHECK_NE(saved.find("外层后文 日本語"), std::string::npos);
            CHECK_NE(saved.find("嵌套尾部保持 🪶"), std::string::npos);
            CHECK_EQ(saved.find("内层内容 中文 😀"), std::string::npos);
            CHECK_EQ(saved.find("外层块_😀"), std::string::npos);
            CHECK_EQ(saved.find("内层块_日本語_🪶"), std::string::npos);
        }

        featherdoc::Document reopened(target);
        REQUIRE_FALSE(reopened.open());
    }

    fs::remove_all(directory);
}

TEST_CASE(
    "bookmark text batch rolls back every pugixml allocation failure") {
    namespace fs = std::filesystem;
    const auto directory =
        fs::current_path() / fs::path{u8"书签批处理_😀_pugixml"};
    const auto baseline = directory / fs::path{u8"基线_日本語.docx"};
    fs::remove_all(directory);
    fs::create_directories(directory);
    write_bookmark_fixture(baseline, text_batch_document_xml(false));
    normalize_bookmark_fixture(baseline);
    const auto before = snapshot_package(baseline);
    const auto bindings = make_text_bindings();

    pugi_memory_management_guard allocation_guard;
    delegated_pugi_allocate = allocation_guard.allocation;
    pugi::set_memory_management_functions(controlled_pugi_allocate,
                                          allocation_guard.deallocation);

    std::size_t successful_allocation_count = 0U;
    {
        const auto success_path =
            directory / fs::path{u8"成功_中文_😀.docx"};
        fs::copy_file(baseline, success_path,
                      fs::copy_options::overwrite_existing);
        featherdoc::Document document(success_path);
        controlled_pugi_failure_call = 0U;
        REQUIRE_FALSE(document.open());
        const auto handles = capture_text_batch_handles(document);
        controlled_pugi_allocation_calls = 0U;
        const auto result = document.fill_bookmarks(bindings);
        successful_allocation_count = controlled_pugi_allocation_calls;
        REQUIRE_EQ(result.requested, 2U);
        REQUIRE_EQ(result.matched, 2U);
        REQUIRE_EQ(result.replaced, 3U);
        REQUIRE_GT(successful_allocation_count, 0U);
        check_text_batch_success(handles);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        const auto target = directory /
                            ("失败_😀_" + std::to_string(failure_call) +
                             ".docx");
        fs::copy_file(baseline, target, fs::copy_options::overwrite_existing);
        featherdoc::Document document(target);
        controlled_pugi_failure_call = 0U;
        REQUIRE_FALSE(document.open());
        const auto handles = capture_text_batch_handles(document);

        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = failure_call;
        const auto result = document.fill_bookmarks(bindings);
        const auto mutation_error = document.last_error();
        controlled_pugi_failure_call = 0U;
        CHECK_EQ(result.requested, 2U);
        CHECK_EQ(result.matched, 0U);
        CHECK_EQ(result.replaced, 0U);
        CHECK_EQ(mutation_error.code,
                 std::make_error_code(std::errc::not_enough_memory));
        check_text_batch_handles_valid(handles);
        REQUIRE_FALSE(document.save());
        check_package_equals(target, before);

        const auto retry = document.fill_bookmarks(bindings);
        REQUIRE_EQ(retry.matched, 2U);
        REQUIRE_EQ(retry.replaced, 3U);
        check_text_batch_success(handles);
    }

    fs::remove_all(directory);
}

TEST_CASE(
    "bookmark table-row replacement rolls back every deep-clone pugixml allocation failure") {
    namespace fs = std::filesystem;
    const auto directory =
        fs::current_path() / fs::path{u8"书签大行深拷贝_中文_日本語_😀"};
    const auto baseline = directory / fs::path{u8"大行基线_🪶.docx"};
    fs::remove_all(directory);
    fs::create_directories(directory);
    write_bookmark_fixture(baseline, large_bookmark_table_row_document_xml());
    normalize_bookmark_fixture(baseline);
    const auto before = snapshot_package(baseline);
    const auto replacements = large_bookmark_row_replacements();

    pugi_memory_management_guard allocation_guard;
    delegated_pugi_allocate = allocation_guard.allocation;
    pugi::set_memory_management_functions(controlled_pugi_allocate,
                                          allocation_guard.deallocation);

    std::size_t successful_allocation_count = 0U;
    {
        const auto target = directory / fs::path{u8"大行成功_中文_😀.docx"};
        fs::copy_file(baseline, target, fs::copy_options::overwrite_existing);
        featherdoc::Document document(target);
        controlled_pugi_failure_call = 0U;
        REQUIRE_FALSE(document.open());
        const auto handles = capture_table_tree_handles(document);
        controlled_pugi_allocation_calls = 0U;
        const auto replaced = document.replace_bookmark_with_table_rows(
            "大行模板_中文_😀", replacements);
        successful_allocation_count = controlled_pugi_allocation_calls;
        REQUIRE_EQ(replaced, 2U);
        REQUIRE_GT(successful_allocation_count, 1U);
        REQUIRE_FALSE(document.last_error());
        for (const auto &row : handles.rows) {
            CHECK_FALSE(row.valid());
        }
        REQUIRE_FALSE(document.save());
        const auto saved =
            read_test_docx_entry(target, test_document_xml_entry);
        CHECK_EQ(saved.find("大行模板_中文_😀"), std::string::npos);
        CHECK_EQ(saved.find(std::string(2048U, 'A')), std::string::npos);
        CHECK_NE(saved.find("新行0_列0 中文 😀"), std::string::npos);
        CHECK_NE(saved.find("新行1_列23 中文 😀"), std::string::npos);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        const auto target = directory /
                            ("大行失败_😀_" +
                             std::to_string(failure_call) + ".docx");
        fs::copy_file(baseline, target, fs::copy_options::overwrite_existing);
        featherdoc::Document document(target);
        controlled_pugi_failure_call = 0U;
        REQUIRE_FALSE(document.open());
        const auto handles = capture_table_tree_handles(document);

        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = failure_call;
        const auto replaced = document.replace_bookmark_with_table_rows(
            "大行模板_中文_😀", replacements);
        const auto mutation_error = document.last_error();
        controlled_pugi_failure_call = 0U;
        CHECK_EQ(replaced, 0U);
        CHECK_EQ(mutation_error.code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK_EQ(mutation_error.entry_name, test_document_xml_entry);
        check_table_tree_handles_valid(handles);
        REQUIRE_FALSE(document.save());
        check_package_equals(target, before);

        const auto retry = document.replace_bookmark_with_table_rows(
            "大行模板_中文_😀", replacements);
        REQUIRE_EQ(retry, 2U);
        REQUIRE_FALSE(document.last_error());
        for (const auto &row : handles.rows) {
            CHECK_FALSE(row.valid());
        }
    }

    fs::remove_all(directory);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "bookmark text and visibility batches roll back every global allocation failure") {
    namespace fs = std::filesystem;
    const auto directory =
        fs::current_path() / fs::path{u8"书签批处理_😀_全局内存"};
    fs::remove_all(directory);
    fs::create_directories(directory);

    const auto run_text_sweep = [&]() {
        const auto baseline = directory / fs::path{u8"文本基线_日本語.docx"};
        write_bookmark_fixture(baseline, text_batch_document_xml(false));
        normalize_bookmark_fixture(baseline);
        const auto before = snapshot_package(baseline);
        const auto bindings = make_text_bindings();

        std::size_t successful_allocation_count = 0U;
        {
            const auto target = directory / fs::path{u8"文本成功_😀.docx"};
            fs::copy_file(baseline, target,
                          fs::copy_options::overwrite_existing);
            featherdoc::Document document(target);
            REQUIRE_FALSE(document.open());
            const auto handles = capture_text_batch_handles(document);
            featherdoc::bookmark_fill_result result;
            {
                global_allocation_window window{0U};
                result = document.fill_bookmarks(bindings);
                successful_allocation_count =
                    global_allocation_calls.load(std::memory_order_relaxed);
            }
            REQUIRE_EQ(result.matched, 2U);
            REQUIRE_EQ(result.replaced, 3U);
            REQUIRE_GT(successful_allocation_count, 0U);
            check_text_batch_success(handles);
        }

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            const auto target = directory /
                                ("文本失败_😀_" +
                                 std::to_string(failure_call) + ".docx");
            fs::copy_file(baseline, target,
                          fs::copy_options::overwrite_existing);
            featherdoc::Document document(target);
            REQUIRE_FALSE(document.open());
            const auto handles = capture_text_batch_handles(document);
            featherdoc::bookmark_fill_result result;
            bool threw = false;
            {
                global_allocation_window window{failure_call};
                try {
                    result = document.fill_bookmarks(bindings);
                } catch (...) {
                    threw = true;
                }
            }
            CHECK_FALSE(threw);
            CHECK_EQ(result.requested, 2U);
            CHECK_EQ(result.matched, 0U);
            CHECK_EQ(result.replaced, 0U);
            CHECK_EQ(document.last_error().code,
                     std::make_error_code(std::errc::not_enough_memory));
            check_text_batch_handles_valid(handles);
            REQUIRE_FALSE(document.save());
            check_package_equals(target, before);
            const auto retry = document.fill_bookmarks(bindings);
            REQUIRE_EQ(retry.matched, 2U);
            REQUIRE_EQ(retry.replaced, 3U);
            check_text_batch_success(handles);
        }
    };

    const auto run_visibility_sweep = [&]() {
        const auto baseline =
            directory / fs::path{u8"可见性基线_日本語.docx"};
        write_bookmark_fixture(baseline,
                               visibility_batch_document_xml(false));
        normalize_bookmark_fixture(baseline);
        const auto before = snapshot_package(baseline);
        const auto bindings = visibility_bindings();

        std::size_t successful_allocation_count = 0U;
        {
            const auto target = directory / fs::path{u8"可见性成功_😀.docx"};
            fs::copy_file(baseline, target,
                          fs::copy_options::overwrite_existing);
            featherdoc::Document document(target);
            REQUIRE_FALSE(document.open());
            const auto handles = capture_visibility_batch_handles(document);
            featherdoc::bookmark_block_visibility_result result;
            {
                global_allocation_window window{0U};
                result = document.apply_bookmark_block_visibility(bindings);
                successful_allocation_count =
                    global_allocation_calls.load(std::memory_order_relaxed);
            }
            REQUIRE_EQ(result.matched, 2U);
            REQUIRE_EQ(result.kept, 1U);
            REQUIRE_EQ(result.removed, 1U);
            REQUIRE_GT(successful_allocation_count, 0U);
            check_visibility_success(handles);
        }

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            const auto target = directory /
                                ("可见性失败_😀_" +
                                 std::to_string(failure_call) + ".docx");
            fs::copy_file(baseline, target,
                          fs::copy_options::overwrite_existing);
            featherdoc::Document document(target);
            REQUIRE_FALSE(document.open());
            const auto handles = capture_visibility_batch_handles(document);
            featherdoc::bookmark_block_visibility_result result;
            bool threw = false;
            {
                global_allocation_window window{failure_call};
                try {
                    result =
                        document.apply_bookmark_block_visibility(bindings);
                } catch (...) {
                    threw = true;
                }
            }
            CHECK_FALSE(threw);
            CHECK_EQ(result.requested, 2U);
            CHECK_EQ(result.matched, 0U);
            CHECK_EQ(result.kept, 0U);
            CHECK_EQ(result.removed, 0U);
            CHECK_EQ(document.last_error().code,
                     std::make_error_code(std::errc::not_enough_memory));
            check_visibility_handles_valid(handles);
            REQUIRE_FALSE(document.save());
            check_package_equals(target, before);
            const auto retry =
                document.apply_bookmark_block_visibility(bindings);
            REQUIRE_EQ(retry.matched, 2U);
            REQUIRE_EQ(retry.kept, 1U);
            REQUIRE_EQ(retry.removed, 1U);
            check_visibility_success(handles);
        }
    };

    const auto run_visibility_conflict_sweep = [&]() {
        const auto baseline =
            directory / fs::path{u8"可见性冲突基线_日本語_😀.docx"};
        write_nested_visibility_fixture(baseline, false);
        normalize_bookmark_fixture(baseline);
        const auto before = snapshot_archive_entries(
            baseline,
            {test_content_types_xml_entry, test_relationships_xml_entry,
             test_document_xml_entry, document_relationships_entry,
             nested_visibility_header_entry});
        const auto bindings = conflicting_visibility_bindings(false, false);

        std::size_t conflict_allocation_count = 0U;
        {
            const auto target =
                directory / fs::path{u8"可见性冲突计数_中文_🪶.docx"};
            fs::copy_file(baseline, target,
                          fs::copy_options::overwrite_existing);
            featherdoc::Document document(target);
            REQUIRE_FALSE(document.open());
            auto header = document.section_header_template(0U);
            REQUIRE(static_cast<bool>(header));
            const auto document_handles =
                capture_part_text_handles(document);
            const auto header_handles = capture_part_text_handles(header);
            featherdoc::bookmark_block_visibility_result result;
            {
                global_allocation_window window{0U};
                result = document.apply_bookmark_block_visibility(bindings);
                conflict_allocation_count =
                    global_allocation_calls.load(std::memory_order_relaxed);
            }
            REQUIRE_GT(conflict_allocation_count, 0U);
            CHECK_EQ(result.requested, 2U);
            CHECK_EQ(result.matched, 0U);
            CHECK_EQ(result.kept, 0U);
            CHECK_EQ(result.removed, 0U);
            CHECK_EQ(document.last_error().code,
                     std::make_error_code(std::errc::invalid_argument));
            CHECK_EQ(document.last_error().entry_name,
                     test_document_xml_entry);
            check_part_text_handles_valid(document_handles);
            check_part_text_handles_valid(header_handles);
        }

        for (std::size_t failure_call = 1U;
             failure_call <= conflict_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(conflict_allocation_count);
            const auto target = directory /
                                ("可见性冲突失败_😀_" +
                                 std::to_string(failure_call) + ".docx");
            fs::copy_file(baseline, target,
                          fs::copy_options::overwrite_existing);
            featherdoc::Document document(target);
            REQUIRE_FALSE(document.open());
            auto header = document.section_header_template(0U);
            REQUIRE(static_cast<bool>(header));
            const auto document_handles =
                capture_part_text_handles(document);
            const auto header_handles = capture_part_text_handles(header);
            featherdoc::bookmark_block_visibility_result result;
            bool threw = false;
            {
                global_allocation_window window{failure_call};
                try {
                    result =
                        document.apply_bookmark_block_visibility(bindings);
                } catch (...) {
                    threw = true;
                }
            }
            CHECK_FALSE(threw);
            CHECK_EQ(result.requested, 2U);
            CHECK_EQ(result.matched, 0U);
            CHECK_EQ(result.kept, 0U);
            CHECK_EQ(result.removed, 0U);
            CHECK_EQ(document.last_error().code,
                     std::make_error_code(std::errc::not_enough_memory));
            check_part_text_handles_valid(document_handles);
            check_part_text_handles_valid(header_handles);
            REQUIRE_FALSE(document.save());
            check_archive_entries_equal(target, before);

            const auto retry =
                document.apply_bookmark_block_visibility(bindings);
            CHECK_EQ(retry.requested, 2U);
            CHECK_EQ(retry.matched, 0U);
            CHECK_EQ(document.last_error().code,
                     std::make_error_code(std::errc::invalid_argument));
            check_part_text_handles_valid(document_handles);
            check_part_text_handles_valid(header_handles);
        }
    };

    run_text_sweep();
    run_visibility_sweep();
    run_visibility_conflict_sweep();
    fs::remove_all(directory);
}
