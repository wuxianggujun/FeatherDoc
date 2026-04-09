#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>
#include <unordered_set>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <featherdoc.hpp>

namespace {
constexpr auto test_document_xml_entry = "word/document.xml";
constexpr auto test_relationships_xml_entry = "_rels/.rels";
constexpr auto test_content_types_xml_entry = "[Content_Types].xml";
constexpr auto test_relationships_xml =
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
constexpr auto test_content_types_xml =
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)";

std::unordered_set<void *> tracked_pugi_allocations;
bool saw_unexpected_pugi_deallocation = false;

auto tracked_pugi_allocate(std::size_t size) -> void * {
    void *ptr = std::malloc(size);
    if (ptr != nullptr) {
        tracked_pugi_allocations.insert(ptr);
    }
    return ptr;
}

auto tracked_pugi_deallocate(void *ptr) -> void {
    if (ptr == nullptr) {
        return;
    }

    if (tracked_pugi_allocations.erase(ptr) == 0U) {
        saw_unexpected_pugi_deallocation = true;
    }

    std::free(ptr);
}

struct pugi_memory_management_guard final {
    pugi::allocation_function allocation{};
    pugi::deallocation_function deallocation{};

    pugi_memory_management_guard()
        : allocation(pugi::get_memory_allocation_function()),
          deallocation(pugi::get_memory_deallocation_function()) {}

    ~pugi_memory_management_guard() {
        pugi::set_memory_management_functions(this->allocation, this->deallocation);
        tracked_pugi_allocations.clear();
        saw_unexpected_pugi_deallocation = false;
    }
};

auto collect_document_text(featherdoc::Document &doc) -> std::string {
    std::ostringstream stream;
    for (featherdoc::Paragraph paragraph = doc.paragraphs(); paragraph.has_next();
         paragraph.next()) {
        for (featherdoc::Run run = paragraph.runs(); run.has_next(); run.next()) {
            stream << run.get_text();
        }
        stream << '\n';
    }
    return stream.str();
}

auto utf8_from_u8(std::u8string_view text) -> std::string {
    return {reinterpret_cast<const char *>(text.data()), text.size()};
}

auto collect_table_text(featherdoc::Document &doc) -> std::string {
    std::ostringstream stream;
    for (featherdoc::Table table = doc.tables(); table.has_next(); table.next()) {
        for (featherdoc::TableRow row = table.rows(); row.has_next(); row.next()) {
            for (featherdoc::TableCell cell = row.cells(); cell.has_next(); cell.next()) {
                for (featherdoc::Paragraph paragraph = cell.paragraphs();
                     paragraph.has_next(); paragraph.next()) {
                    for (featherdoc::Run run = paragraph.runs(); run.has_next(); run.next()) {
                        stream << run.get_text();
                    }
                    stream << '\n';
                }
            }
        }
    }
    return stream.str();
}

auto collect_template_part_text(featherdoc::TemplatePart part) -> std::string {
    std::ostringstream stream;
    for (featherdoc::Paragraph paragraph = part.paragraphs(); paragraph.has_next();
         paragraph.next()) {
        for (featherdoc::Run run = paragraph.runs(); run.has_next(); run.next()) {
            stream << run.get_text();
        }
        stream << '\n';
    }
    return stream.str();
}

auto collect_template_part_table_text(featherdoc::TemplatePart part) -> std::string {
    std::ostringstream stream;
    for (featherdoc::Table table = part.tables(); table.has_next(); table.next()) {
        for (featherdoc::TableRow row = table.rows(); row.has_next(); row.next()) {
            for (featherdoc::TableCell cell = row.cells(); cell.has_next(); cell.next()) {
                for (featherdoc::Paragraph paragraph = cell.paragraphs();
                     paragraph.has_next(); paragraph.next()) {
                    for (featherdoc::Run run = paragraph.runs(); run.has_next(); run.next()) {
                        stream << run.get_text();
                    }
                    stream << '\n';
                }
            }
        }
    }
    return stream.str();
}

auto set_two_cell_row_text(featherdoc::TableRow row, std::string_view left,
                           std::string_view right) -> bool {
    auto cell = row.cells();
    if (!cell.has_next()) {
        return false;
    }
    if (!cell.set_text(std::string{left})) {
        return false;
    }

    cell.next();
    if (!cell.has_next()) {
        return false;
    }

    return cell.set_text(std::string{right});
}

auto configure_clone_template_table(featherdoc::Table table, std::string_view prefix) -> bool {
    if (!table.has_next()) {
        return false;
    }

    if (!table.set_width_twips(7200U) || !table.set_style_id("TableGrid") ||
        !table.set_layout_mode(featherdoc::table_layout_mode::fixed) ||
        !table.set_alignment(featherdoc::table_alignment::center) ||
        !table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 120U) ||
        !table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 120U)) {
        return false;
    }

    auto header_row = table.rows();
    if (!header_row.has_next() ||
        !header_row.set_height_twips(360U, featherdoc::row_height_rule::exact) ||
        !header_row.set_repeats_header()) {
        return false;
    }

    auto header_cell = header_row.cells();
    if (!header_cell.has_next() ||
        !header_cell.set_fill_color("D9EAF7") ||
        !header_cell.set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center) ||
        !header_cell.set_text(std::string{prefix} + "-h1")) {
        return false;
    }

    header_cell.next();
    if (!header_cell.has_next() ||
        !header_cell.set_fill_color("D9EAF7") ||
        !header_cell.set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center) ||
        !header_cell.set_text(std::string{prefix} + "-h2")) {
        return false;
    }

    header_row.next();
    if (!header_row.has_next() ||
        !header_row.set_height_twips(420U, featherdoc::row_height_rule::at_least)) {
        return false;
    }

    auto body_cell = header_row.cells();
    if (!body_cell.has_next() ||
        !body_cell.set_fill_color("FCE4D6") ||
        !body_cell.set_margin_twips(featherdoc::cell_margin_edge::left, 160U) ||
        !body_cell.set_text(std::string{prefix} + "-b1")) {
        return false;
    }

    body_cell.next();
    if (!body_cell.has_next() ||
        !body_cell.set_fill_color("FFF2CC") ||
        !body_cell.set_margin_twips(featherdoc::cell_margin_edge::right, 160U) ||
        !body_cell.set_text(std::string{prefix} + "-b2")) {
        return false;
    }

    return true;
}

auto count_named_children(pugi::xml_node parent, const char *child_name) -> std::size_t {
    std::size_t count = 0;
    for (auto child = parent.child(child_name); child != pugi::xml_node{};
         child = child.next_sibling(child_name)) {
        ++count;
    }
    return count;
}

auto count_named_descendants(pugi::xml_node parent, const char *child_name) -> std::size_t {
    if (parent == pugi::xml_node{}) {
        return 0U;
    }

    std::size_t count = 0U;
    for (auto child = parent.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::strcmp(child.name(), child_name) == 0) {
            ++count;
        }
        count += count_named_descendants(child, child_name);
    }
    return count;
}

auto count_substring_occurrences(std::string_view text, std::string_view needle)
    -> std::size_t {
    if (needle.empty()) {
        return 0U;
    }

    std::size_t count = 0U;
    std::size_t position = 0U;
    while ((position = text.find(needle, position)) != std::string_view::npos) {
        ++count;
        position += needle.size();
    }
    return count;
}

auto write_binary_file(const std::filesystem::path &path, const std::string &data) -> void {
    std::ofstream stream(path, std::ios::binary);
    REQUIRE(stream.good());
    stream.write(data.data(), static_cast<std::streamsize>(data.size()));
    REQUIRE(stream.good());
}

auto tiny_png_data() -> std::string {
    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    return {reinterpret_cast<const char *>(tiny_png_bytes), sizeof(tiny_png_bytes)};
}

auto read_binary_file(const std::filesystem::path &path) -> std::vector<unsigned char> {
    std::ifstream stream(path, std::ios::binary);
    REQUIRE(stream.good());

    return {std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
}

auto read_le16(const std::vector<unsigned char> &data, std::size_t offset) -> std::uint16_t {
    REQUIRE_LE(offset + 2U, data.size());
    return static_cast<std::uint16_t>(data[offset]) |
           (static_cast<std::uint16_t>(data[offset + 1U]) << 8U);
}

auto read_le32(const std::vector<unsigned char> &data, std::size_t offset) -> std::uint32_t {
    REQUIRE_LE(offset + 4U, data.size());
    return static_cast<std::uint32_t>(data[offset]) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 8U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 3U]) << 24U);
}

auto extra_contains_empty_zip64_field(const std::vector<unsigned char> &data, std::size_t offset,
                                      std::size_t size) -> bool {
    const std::size_t end = offset + size;
    if (end > data.size()) {
        return false;
    }

    std::size_t cursor = offset;
    while (cursor + 4U <= end) {
        const auto header_id = read_le16(data, cursor);
        const auto field_size = read_le16(data, cursor + 2U);
        cursor += 4U;

        if (cursor + field_size > end) {
            return false;
        }

        if (header_id == 0x0001U && field_size == 0U) {
            return true;
        }

        cursor += field_size;
    }

    return false;
}

auto collect_empty_zip64_extra_locations(const std::filesystem::path &path)
    -> std::vector<std::string> {
    constexpr std::uint32_t local_file_header_signature = 0x04034B50U;
    constexpr std::uint32_t central_directory_header_signature = 0x02014B50U;
    constexpr std::uint32_t end_of_central_directory_signature = 0x06054B50U;

    const auto data = read_binary_file(path);
    std::vector<std::string> locations;

    std::size_t offset = 0U;
    while (offset + 4U <= data.size()) {
        const auto signature = read_le32(data, offset);
        if (signature != local_file_header_signature) {
            break;
        }

        const auto compressed_size = static_cast<std::size_t>(read_le32(data, offset + 18U));
        const auto filename_size = static_cast<std::size_t>(read_le16(data, offset + 26U));
        const auto extra_size = static_cast<std::size_t>(read_le16(data, offset + 28U));
        const auto header_size = 30U + filename_size + extra_size;
        REQUIRE_LE(offset + header_size, data.size());

        const std::string entry_name{
            reinterpret_cast<const char *>(data.data() + offset + 30U), filename_size};
        const auto extra_offset = offset + 30U + filename_size;
        if (extra_contains_empty_zip64_field(data, extra_offset, extra_size)) {
            locations.push_back("local:" + entry_name);
        }

        offset += header_size + compressed_size;
    }

    while (offset + 4U <= data.size()) {
        const auto signature = read_le32(data, offset);
        if (signature == end_of_central_directory_signature) {
            break;
        }

        REQUIRE_EQ(signature, central_directory_header_signature);

        const auto filename_size = static_cast<std::size_t>(read_le16(data, offset + 28U));
        const auto extra_size = static_cast<std::size_t>(read_le16(data, offset + 30U));
        const auto comment_size = static_cast<std::size_t>(read_le16(data, offset + 32U));
        const auto header_size = 46U + filename_size + extra_size + comment_size;
        REQUIRE_LE(offset + header_size, data.size());

        const std::string entry_name{
            reinterpret_cast<const char *>(data.data() + offset + 46U), filename_size};
        const auto extra_offset = offset + 46U + filename_size;
        if (extra_contains_empty_zip64_field(data, extra_offset, extra_size)) {
            locations.push_back("central:" + entry_name);
        }

        offset += header_size;
    }

    return locations;
}

auto collect_zero_version_needed_locations(const std::filesystem::path &path)
    -> std::vector<std::string> {
    constexpr std::uint32_t local_file_header_signature = 0x04034B50U;
    constexpr std::uint32_t central_directory_header_signature = 0x02014B50U;
    constexpr std::uint32_t end_of_central_directory_signature = 0x06054B50U;

    const auto data = read_binary_file(path);
    std::vector<std::string> locations;

    std::size_t offset = 0U;
    while (offset + 4U <= data.size()) {
        const auto signature = read_le32(data, offset);
        if (signature != local_file_header_signature) {
            break;
        }

        const auto version_needed = read_le16(data, offset + 4U);
        const auto compressed_size = static_cast<std::size_t>(read_le32(data, offset + 18U));
        const auto filename_size = static_cast<std::size_t>(read_le16(data, offset + 26U));
        const auto extra_size = static_cast<std::size_t>(read_le16(data, offset + 28U));
        const auto header_size = 30U + filename_size + extra_size;
        REQUIRE_LE(offset + header_size, data.size());

        const std::string entry_name{
            reinterpret_cast<const char *>(data.data() + offset + 30U), filename_size};
        if (version_needed == 0U) {
            locations.push_back("local:" + entry_name);
        }

        offset += header_size + compressed_size;
    }

    while (offset + 4U <= data.size()) {
        const auto signature = read_le32(data, offset);
        if (signature == end_of_central_directory_signature) {
            break;
        }

        REQUIRE_EQ(signature, central_directory_header_signature);

        const auto version_needed = read_le16(data, offset + 6U);
        const auto filename_size = static_cast<std::size_t>(read_le16(data, offset + 28U));
        const auto extra_size = static_cast<std::size_t>(read_le16(data, offset + 30U));
        const auto comment_size = static_cast<std::size_t>(read_le16(data, offset + 32U));
        const auto header_size = 46U + filename_size + extra_size + comment_size;
        REQUIRE_LE(offset + header_size, data.size());

        const std::string entry_name{
            reinterpret_cast<const char *>(data.data() + offset + 46U), filename_size};
        if (version_needed == 0U) {
            locations.push_back("central:" + entry_name);
        }

        offset += header_size;
    }

    return locations;
}

auto write_test_docx(const std::filesystem::path &path, const std::string &document_xml)
    -> void {
    struct archive_entry {
        std::string name;
        std::string content;
    };

    const std::vector<archive_entry> entries{
        {test_content_types_xml_entry, test_content_types_xml},
        {test_relationships_xml_entry, test_relationships_xml},
        {test_document_xml_entry, document_xml},
    };

    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                   &zip_error);
    REQUIRE(zip != nullptr);

    for (const auto &entry : entries) {
        REQUIRE_EQ(zip_entry_open(zip, entry.name.c_str()), 0);
        REQUIRE_GE(zip_entry_write(zip, entry.content.data(), entry.content.size()), 0);
        REQUIRE_EQ(zip_entry_close(zip), 0);
    }

    zip_close(zip);
}

auto write_test_archive_entries(
    const std::filesystem::path &path,
    const std::vector<std::pair<std::string, std::string>> &entries) -> void {
    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                   &zip_error);
    REQUIRE(zip != nullptr);

    for (const auto &[name, content] : entries) {
        REQUIRE_EQ(zip_entry_open(zip, name.c_str()), 0);
        REQUIRE_GE(zip_entry_write(zip, content.data(), content.size()), 0);
        REQUIRE_EQ(zip_entry_close(zip), 0);
    }

    zip_close(zip);
}

auto read_test_docx_entry(const std::filesystem::path &path, const char *entry_name)
    -> std::string {
    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                   &zip_error);
    CHECK(zip != nullptr);
    if (zip == nullptr) {
        return {};
    }
    CHECK_EQ(zip_entry_open(zip, entry_name), 0);

    void *buffer = nullptr;
    size_t buffer_size = 0;
    CHECK_GE(zip_entry_read(zip, &buffer, &buffer_size), 0);
    if (buffer == nullptr) {
        zip_entry_close(zip);
        zip_close(zip);
        return {};
    }

    std::string content(static_cast<const char *>(buffer), buffer_size);
    std::free(buffer);

    CHECK_EQ(zip_entry_close(zip), 0);
    zip_close(zip);
    return content;
}

auto read_test_archive_entries(const std::filesystem::path &path)
    -> std::vector<std::pair<std::string, std::string>> {
    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                   &zip_error);
    REQUIRE(zip != nullptr);

    std::vector<std::pair<std::string, std::string>> entries;
    const auto total_entries = zip_entries_total(zip);
    REQUIRE_GE(total_entries, 0);
    entries.reserve(static_cast<std::size_t>(total_entries));

    for (ssize_t index = 0; index < total_entries; ++index) {
        REQUIRE_EQ(zip_entry_openbyindex(zip, static_cast<std::size_t>(index)), 0);

        const auto *current_entry_name = zip_entry_name(zip);
        REQUIRE(current_entry_name != nullptr);

        void *buffer = nullptr;
        size_t buffer_size = 0U;
        REQUIRE_GE(zip_entry_read(zip, &buffer, &buffer_size), 0);
        REQUIRE(buffer != nullptr);

        entries.emplace_back(
            current_entry_name,
            std::string(static_cast<const char *>(buffer), buffer_size));
        std::free(buffer);
        REQUIRE_EQ(zip_entry_close(zip), 0);
    }

    zip_close(zip);
    return entries;
}

auto rewrite_test_docx_entry(const std::filesystem::path &path, const char *entry_name,
                             std::string content) -> void {
    auto entries = read_test_archive_entries(path);

    bool replaced = false;
    for (auto &[current_entry_name, current_content] : entries) {
        if (current_entry_name != entry_name) {
            continue;
        }

        current_content = std::move(content);
        replaced = true;
        break;
    }

    REQUIRE(replaced);
    write_test_archive_entries(path, entries);
}

auto test_docx_entry_exists(const std::filesystem::path &path, const char *entry_name)
    -> bool {
    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                   &zip_error);
    CHECK(zip != nullptr);
    if (zip == nullptr) {
        return false;
    }

    const bool exists = zip_entry_open(zip, entry_name) == 0;
    if (exists) {
        CHECK_EQ(zip_entry_close(zip), 0);
    }
    zip_close(zip);
    return exists;
}

auto convert_nth_inline_drawing_to_anchor(std::string xml_text, std::size_t inline_index)
    -> std::string {
    constexpr auto inline_open_tag = "<wp:inline";
    constexpr auto inline_close_tag = "</wp:inline>";
    constexpr auto anchor_open_attributes =
        R"( distT="0" distB="0" distL="0" distR="0" simplePos="0" relativeHeight="0" behindDoc="0" locked="0" layoutInCell="1" allowOverlap="1")";
    constexpr auto anchor_positioning_xml =
        R"(<wp:simplePos x="0" y="0"/><wp:positionH relativeFrom="column"><wp:posOffset>0</wp:posOffset></wp:positionH><wp:positionV relativeFrom="paragraph"><wp:posOffset>0</wp:posOffset></wp:positionV><wp:wrapNone/>)";

    std::size_t open_search_from = 0U;
    std::size_t open_tag_position = std::string::npos;
    for (std::size_t current_index = 0U; current_index <= inline_index; ++current_index) {
        open_tag_position = xml_text.find(inline_open_tag, open_search_from);
        REQUIRE_NE(open_tag_position, std::string::npos);
        open_search_from = open_tag_position + std::strlen(inline_open_tag);
    }

    const auto open_tag_end = xml_text.find('>', open_tag_position);
    REQUIRE_NE(open_tag_end, std::string::npos);
    xml_text.replace(open_tag_position, std::strlen(inline_open_tag), "<wp:anchor");
    xml_text.insert(open_tag_end, anchor_open_attributes);

    const auto extent_position = xml_text.find("<wp:extent", open_tag_end);
    REQUIRE_NE(extent_position, std::string::npos);
    xml_text.insert(extent_position, anchor_positioning_xml);

    const auto close_tag_position = xml_text.find(inline_close_tag, open_tag_end);
    REQUIRE_NE(close_tag_position, std::string::npos);
    xml_text.replace(close_tag_position, std::strlen(inline_close_tag), "</wp:anchor>");

    return xml_text;
}
} // namespace

TEST_CASE("checks contents of my_test.docx") {
    featherdoc::Document doc("my_test.docx");
    const auto open_error = doc.open();
    CHECK_FALSE(open_error);
    CHECK(doc.is_open());

    std::ostringstream ss;

    for (featherdoc::Paragraph p = doc.paragraphs(); p.has_next(); p.next()) {
        for (featherdoc::Run r = p.runs(); r.has_next(); r.next()) {
            ss << r.get_text() << std::endl;
        }
    }

    CHECK_EQ("This is a test\nokay?\n", ss.str());
}

TEST_CASE("open fails for missing file") {
    featherdoc::Document doc;
    doc.set_path("missing.docx");

    CHECK_EQ(doc.path().filename().string(), "missing.docx");
    const auto error = doc.open();
    CHECK_EQ(error, std::make_error_code(std::errc::no_such_file_or_directory));
    CHECK_EQ(doc.last_error().code, error);
    CHECK_FALSE(doc.last_error().detail.empty());
    CHECK_FALSE(doc.is_open());
}

TEST_CASE("open reports explicit errors for empty path and invalid archive") {
    namespace fs = std::filesystem;

    featherdoc::Document empty_doc;
    CHECK_EQ(empty_doc.open(), featherdoc::document_errc::empty_path);
    CHECK_EQ(empty_doc.last_error().code, featherdoc::document_errc::empty_path);
    CHECK_FALSE(empty_doc.last_error().detail.empty());

    const fs::path invalid_path = fs::current_path() / "invalid.docx";
    {
        std::ofstream invalid_file(invalid_path, std::ios::binary);
        invalid_file << "not a docx archive";
    }

    featherdoc::Document invalid_doc(invalid_path);
    CHECK_EQ(invalid_doc.open(), featherdoc::document_errc::archive_open_failed);
    CHECK_EQ(invalid_doc.last_error().code,
             featherdoc::document_errc::archive_open_failed);
    CHECK_FALSE(invalid_doc.last_error().detail.empty());
    CHECK_FALSE(invalid_doc.is_open());

    fs::remove(invalid_path);
}

TEST_CASE("open keeps zip buffers out of pugixml-owned deallocation paths") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "allocator_boundary_regression.docx";
    fs::remove(target);

    write_test_docx(
        target,
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>allocator safety</w:t></w:r></w:p>
  </w:body>
</w:document>
)");

    pugi_memory_management_guard guard;
    tracked_pugi_allocations.clear();
    saw_unexpected_pugi_deallocation = false;
    pugi::set_memory_management_functions(tracked_pugi_allocate, tracked_pugi_deallocate);

    {
        featherdoc::Document doc(target);
        CHECK_FALSE(doc.open());
        CHECK(doc.is_open());
        CHECK_EQ(collect_document_text(doc), "allocator safety\n");
    }

    CHECK_FALSE(saw_unexpected_pugi_deallocation);
    CHECK(tracked_pugi_allocations.empty());

    fs::remove(target);
}

TEST_CASE("open reports encrypted docx files as unsupported") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "encrypted.docx";
    fs::remove(target);

    zip_t *zip = zip_open_with_password(target.string().c_str(),
                                        ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                        "secret");
    REQUIRE(zip != nullptr);

    REQUIRE_EQ(zip_entry_open(zip, test_content_types_xml_entry), 0);
    REQUIRE_GE(zip_entry_write(zip, test_content_types_xml,
                               std::strlen(test_content_types_xml)),
               0);
    REQUIRE_EQ(zip_entry_close(zip), 0);

    REQUIRE_EQ(zip_entry_open(zip, test_relationships_xml_entry), 0);
    REQUIRE_GE(zip_entry_write(zip, test_relationships_xml,
                               std::strlen(test_relationships_xml)),
               0);
    REQUIRE_EQ(zip_entry_close(zip), 0);

    constexpr auto encrypted_document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>secret</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    REQUIRE_EQ(zip_entry_open(zip, test_document_xml_entry), 0);
    REQUIRE_GE(zip_entry_write(zip, encrypted_document_xml,
                               std::strlen(encrypted_document_xml)),
               0);
    REQUIRE_EQ(zip_entry_close(zip), 0);
    zip_close(zip);

    featherdoc::Document doc(target);
    CHECK_EQ(doc.open(), featherdoc::document_errc::encrypted_document_unsupported);
    CHECK_EQ(doc.last_error().code,
             featherdoc::document_errc::encrypted_document_unsupported);
    CHECK_EQ(doc.last_error().entry_name, test_document_xml_entry);
    CHECK(doc.last_error().detail.find("encrypted") != std::string::npos);
    CHECK_FALSE(doc.is_open());

    fs::remove(target);
}

TEST_CASE("open reports a missing document XML entry for non-docx zip archives") {
    namespace fs = std::filesystem;

    const fs::path archive_path = fs::current_path() / "not_a_word_document.docx";
    fs::remove(archive_path);

    int zip_error = 0;
    zip_t *zip = zip_openwitherror(archive_path.string().c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                   &zip_error);
    REQUIRE(zip != nullptr);

    constexpr auto placeholder_entry = "payload.txt";
    constexpr auto placeholder_text = "plain zip content";
    REQUIRE_EQ(zip_entry_open(zip, placeholder_entry), 0);
    REQUIRE_GE(zip_entry_write(zip, placeholder_text,
                               std::strlen(placeholder_text)),
               0);
    REQUIRE_EQ(zip_entry_close(zip), 0);
    zip_close(zip);

    featherdoc::Document doc(archive_path);
    CHECK_EQ(doc.open(), featherdoc::document_errc::document_xml_open_failed);
    CHECK_EQ(doc.last_error().code,
             featherdoc::document_errc::document_xml_open_failed);
    CHECK_EQ(doc.last_error().entry_name, "word/document.xml");
    CHECK_FALSE(doc.last_error().detail.empty());
    CHECK_FALSE(doc.is_open());

    fs::remove(archive_path);
}

TEST_CASE("open exposes malformed XML context") {
    namespace fs = std::filesystem;

    const fs::path malformed_path = fs::current_path() / "malformed.docx";
    const std::string malformed_xml = "<w:document><w:body>";

    int zip_error = 0;
    zip_t *zip = zip_openwitherror(malformed_path.string().c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                   &zip_error);
    REQUIRE(zip != nullptr);
    REQUIRE_EQ(zip_entry_open(zip, "word/document.xml"), 0);
    REQUIRE_GE(zip_entry_write(zip, malformed_xml.data(), malformed_xml.size()), 0);
    REQUIRE_EQ(zip_entry_close(zip), 0);
    zip_close(zip);

    featherdoc::Document doc(malformed_path);
    CHECK_EQ(doc.open(), featherdoc::document_errc::document_xml_parse_failed);
    CHECK_EQ(doc.last_error().code,
             featherdoc::document_errc::document_xml_parse_failed);
    CHECK_EQ(doc.last_error().entry_name, "word/document.xml");
    CHECK(doc.last_error().xml_offset.has_value());
    CHECK_FALSE(doc.last_error().detail.empty());

    fs::remove(malformed_path);
}

TEST_CASE("can save changes to a copied document") {
    namespace fs = std::filesystem;

    const fs::path source = "my_test.docx";
    const fs::path target = fs::current_path() / "my_test_copy.docx";

    fs::copy_file(source, target, fs::copy_options::overwrite_existing);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto paragraph = doc.paragraphs();
    paragraph.add_run("!", featherdoc::formatting_flag::bold);

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    CHECK_NE(collect_document_text(reopened).find('!'), std::string::npos);

    fs::remove(target);
}

TEST_CASE("save reports unopened document") {
    featherdoc::Document doc("my_test.docx");
    CHECK_EQ(doc.save(), featherdoc::document_errc::document_not_open);
    CHECK_EQ(doc.last_error().code, featherdoc::document_errc::document_not_open);
    CHECK_FALSE(doc.last_error().detail.empty());
}

TEST_CASE("save_as writes to a new path without mutating the source path") {
    namespace fs = std::filesystem;

    const fs::path source_seed = "my_test.docx";
    const fs::path source = fs::current_path() / "my_test_save_as_source.docx";
    const fs::path target = fs::current_path() / "my_test_save_as_target.docx";

    fs::copy_file(source_seed, source, fs::copy_options::overwrite_existing);
    fs::remove(target);

    featherdoc::Document doc(source);
    CHECK_FALSE(doc.open());

    auto paragraph = doc.paragraphs();
    paragraph.add_run("!", featherdoc::formatting_flag::bold);

    CHECK_FALSE(doc.save_as(target));
    CHECK_EQ(doc.path(), source);
    CHECK_FALSE(doc.last_error());

    featherdoc::Document reopened_source(source);
    CHECK_FALSE(reopened_source.open());

    featherdoc::Document reopened_target(target);
    CHECK_FALSE(reopened_target.open());

    const auto source_text = collect_document_text(reopened_source);
    const auto target_text = collect_document_text(reopened_target);

    CHECK_EQ(source_text.find('!'), std::string::npos);
    CHECK_NE(target_text.find('!'), std::string::npos);

    fs::remove(source);
    fs::remove(target);
}

TEST_CASE("create_empty builds an editable document without a source archive") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "generated_empty.docx";
    fs::remove(target);

    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.is_open());
    CHECK_FALSE(doc.last_error());

    auto paragraph = doc.paragraphs();
    CHECK(paragraph.has_next());
    paragraph.add_run("Generated from scratch",
                      featherdoc::formatting_flag::bold |
                          featherdoc::formatting_flag::italic);

    CHECK_FALSE(doc.save_as(target));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "Generated from scratch\n");

    fs::remove(target);
}

TEST_CASE("create_empty supports save through the configured path") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "generated_empty_save.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    paragraph.add_run("Saved with save()");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "Saved with save()\n");

    fs::remove(target);
}

TEST_CASE("paragraph set_text replaces all runs in place") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_set_text.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    auto first_run = paragraph.add_run("first");
    REQUIRE(first_run.has_next());
    REQUIRE(paragraph.add_run(" second").has_next());
    REQUIRE(paragraph.add_run(" third").has_next());

    CHECK(paragraph.set_text("replaced paragraph"));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "replaced paragraph\n");

    auto reopened_paragraph = reopened.paragraphs();
    REQUIRE(reopened_paragraph.has_next());

    std::size_t run_count = 0U;
    for (auto run = reopened_paragraph.runs(); run.has_next(); run.next()) {
        ++run_count;
    }
    CHECK_EQ(run_count, 1U);

    fs::remove(target);
}

TEST_CASE("run remove deletes the targeted run from a paragraph") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "run_remove.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.set_text("seed"));

    auto first_run = paragraph.runs();
    REQUIRE(first_run.has_next());
    REQUIRE(first_run.set_text("left"));
    auto removed_run = paragraph.add_run(" middle");
    REQUIRE(removed_run.has_next());
    REQUIRE(paragraph.add_run(" right").has_next());

    CHECK(removed_run.remove());
    CHECK(removed_run.has_next());
    CHECK_EQ(removed_run.get_text(), " right");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "left right\n");

    fs::remove(target);
}

TEST_CASE("run insertions keep the anchor usable and preserve run order") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "run_insert_around_anchor.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("anchor"));

    auto anchor = paragraph.runs();
    REQUIRE(anchor.has_next());

    auto inserted_before =
        anchor.insert_run_before("left ", featherdoc::formatting_flag::bold);
    REQUIRE(inserted_before.has_next());
    auto inserted_after = anchor.insert_run_after(" right");
    REQUIRE(inserted_after.has_next());

    CHECK(anchor.has_next());
    CHECK_EQ(anchor.get_text(), "anchor");
    CHECK_EQ(inserted_before.get_text(), "left ");
    CHECK_EQ(inserted_after.get_text(), " right");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "left anchor right\n");

    auto reopened_runs = reopened.paragraphs().runs();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), "left ");
    reopened_runs.next();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), "anchor");
    reopened_runs.next();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), " right");

    fs::remove(target);
}

TEST_CASE("insert_run_before saves cleanly from an empty paragraph run cursor") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_run_before_empty_cursor.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto runs = doc.paragraphs().runs();
    CHECK_FALSE(runs.has_next());

    auto inserted = runs.insert_run_before("created from empty run cursor");
    REQUIRE(inserted.has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "created from empty run cursor\n");

    fs::remove(target);
}

TEST_CASE("insert_run_after appends cleanly from an exhausted run cursor") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_run_after_exhausted_cursor.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("seed"));

    auto runs = paragraph.runs();
    while (runs.has_next()) {
        runs.next();
    }

    auto inserted = runs.insert_run_after(" tail");
    REQUIRE(inserted.has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "seed tail\n");

    fs::remove(target);
}

TEST_CASE("insert_run_like_before clones run properties and keeps the anchor usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_run_like_before_body.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());

    auto anchor = paragraph.add_run("anchor");
    REQUIRE(anchor.has_next());
    CHECK(doc.set_run_style(anchor, "Strong"));
    CHECK(anchor.set_font_family("Segoe UI"));
    CHECK(anchor.set_east_asia_font_family("Microsoft YaHei"));
    CHECK(anchor.set_language("en-US"));
    CHECK(anchor.set_east_asia_language("zh-CN"));
    CHECK(anchor.set_bidi_language("ar-SA"));
    CHECK(anchor.set_rtl());

    auto inserted = anchor.insert_run_like_before();
    REQUIRE(inserted.has_next());
    CHECK_EQ(inserted.get_text(), "");
    CHECK(inserted.set_text("before "));

    const auto inserted_font_family = inserted.font_family();
    REQUIRE(inserted_font_family.has_value());
    CHECK_EQ(*inserted_font_family, "Segoe UI");

    const auto inserted_east_asia_font = inserted.east_asia_font_family();
    REQUIRE(inserted_east_asia_font.has_value());
    CHECK_EQ(*inserted_east_asia_font, "Microsoft YaHei");

    const auto inserted_language = inserted.language();
    REQUIRE(inserted_language.has_value());
    CHECK_EQ(*inserted_language, "en-US");

    const auto inserted_east_asia_language = inserted.east_asia_language();
    REQUIRE(inserted_east_asia_language.has_value());
    CHECK_EQ(*inserted_east_asia_language, "zh-CN");

    const auto inserted_bidi_language = inserted.bidi_language();
    REQUIRE(inserted_bidi_language.has_value());
    CHECK_EQ(*inserted_bidi_language, "ar-SA");

    const auto inserted_rtl = inserted.rtl();
    REQUIRE(inserted_rtl.has_value());
    CHECK(*inserted_rtl);

    CHECK(anchor.has_next());
    CHECK_EQ(anchor.get_text(), "anchor");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml,
                                         "<w:rStyle w:val=\"Strong\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:ascii=\"Segoe UI\""), 2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml,
                                         "w:eastAsia=\"Microsoft YaHei\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:val=\"en-US\""), 2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:eastAsia=\"zh-CN\""), 2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:bidi=\"ar-SA\""), 2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:rtl w:val=\"1\""), 2U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before anchor\n");

    auto reopened_runs = reopened.paragraphs().runs();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), "before ");
    const auto reopened_inserted_language = reopened_runs.language();
    REQUIRE(reopened_inserted_language.has_value());
    CHECK_EQ(*reopened_inserted_language, "en-US");
    const auto reopened_inserted_rtl = reopened_runs.rtl();
    REQUIRE(reopened_inserted_rtl.has_value());
    CHECK(*reopened_inserted_rtl);

    reopened_runs.next();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), "anchor");
    const auto reopened_anchor_language = reopened_runs.language();
    REQUIRE(reopened_anchor_language.has_value());
    CHECK_EQ(*reopened_anchor_language, "en-US");
    const auto reopened_anchor_rtl = reopened_runs.rtl();
    REQUIRE(reopened_anchor_rtl.has_value());
    CHECK(*reopened_anchor_rtl);

    fs::remove(target);
}

TEST_CASE("insert_run_like_after clones run properties without copying anchor text") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_run_like_after_body.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());

    auto anchor = paragraph.add_run("anchor");
    REQUIRE(anchor.has_next());
    CHECK(doc.set_run_style(anchor, "Emphasis"));
    CHECK(anchor.set_language("en-US"));
    CHECK(anchor.set_rtl());

    auto inserted = anchor.insert_run_like_after();
    REQUIRE(inserted.has_next());
    CHECK_EQ(inserted.get_text(), "");
    CHECK(inserted.set_text(" after"));
    CHECK_EQ(anchor.get_text(), "anchor");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml,
                                         "<w:rStyle w:val=\"Emphasis\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:t>anchor</w:t>"), 1U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "anchor after\n");

    auto reopened_runs = reopened.paragraphs().runs();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), "anchor");
    const auto reopened_anchor_language = reopened_runs.language();
    REQUIRE(reopened_anchor_language.has_value());
    CHECK_EQ(*reopened_anchor_language, "en-US");

    reopened_runs.next();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), " after");
    const auto reopened_inserted_language = reopened_runs.language();
    REQUIRE(reopened_inserted_language.has_value());
    CHECK_EQ(*reopened_inserted_language, "en-US");
    const auto reopened_inserted_rtl = reopened_runs.rtl();
    REQUIRE(reopened_inserted_rtl.has_value());
    CHECK(*reopened_inserted_rtl);

    fs::remove(target);
}

TEST_CASE("insert_run_like requires a live anchor run") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_run_like_invalid_cursor.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto empty_runs = doc.paragraphs().runs();
    CHECK_FALSE(empty_runs.has_next());

    auto inserted_before = empty_runs.insert_run_like_before();
    CHECK_FALSE(inserted_before.has_next());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("seed"));

    auto exhausted_runs = paragraph.runs();
    while (exhausted_runs.has_next()) {
        exhausted_runs.next();
    }

    auto inserted_after = exhausted_runs.insert_run_like_after();
    CHECK_FALSE(inserted_after.has_next());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "seed\n");

    fs::remove(target);
}

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

    CHECK_EQ(doc.replace_bookmark_text("bookmark", " updated value "), 1);
    CHECK_EQ(collect_document_text(doc), "prefix updated value suffix\n");

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

TEST_CASE("body template part can append and edit tables") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "body_template_tables.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto paragraph = body_template.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body intro"));

    auto first_table = body_template.append_table(1, 2);
    REQUIRE(first_table.has_next());
    auto first_cell = first_table.rows().cells();
    REQUIRE(first_cell.has_next());
    CHECK(first_cell.set_text("body-a"));
    first_cell.next();
    REQUIRE(first_cell.has_next());
    CHECK(first_cell.set_text("body-b"));

    auto removable_table = body_template.append_table(1, 1);
    REQUIRE(removable_table.has_next());
    CHECK(removable_table.rows().cells().set_text("temporary table"));

    CHECK(removable_table.remove());
    CHECK(removable_table.has_next());
    CHECK_EQ(removable_table.rows().cells().get_text(), "body-a");
    CHECK(removable_table.rows().cells().set_text("body-a-updated"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_body = reopened.body_template();
    REQUIRE(static_cast<bool>(reopened_body));
    CHECK_EQ(collect_template_part_text(reopened_body), "body intro\n");
    CHECK_EQ(collect_template_part_table_text(reopened_body), "body-a-updated\nbody-b\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:tbl"), 1U);

    fs::remove(target);
}

TEST_CASE("body template part can append paragraphs before section properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "body_template_append_paragraph.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto &header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.set_text("header"));

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));

    auto paragraph = body_template.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body intro"));

    auto appended = body_template.append_paragraph("body appended");
    REQUIRE(appended.has_next());
    CHECK(appended.add_run(" tail").has_next());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_body = reopened.body_template();
    REQUIRE(static_cast<bool>(reopened_body));
    CHECK_EQ(collect_template_part_text(reopened_body), "body intro\nbody appended tail\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:p"), 2U);
    CHECK_EQ(std::string_view{body_node.last_child().name()}, "w:sectPr");

    fs::remove(target);
}

TEST_CASE("header template part can append paragraphs and keep the returned paragraph editable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_template_append_paragraph.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto &header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.set_text("Header intro"));

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));

    auto appended = header_template.append_paragraph("Header appended");
    REQUIRE(appended.has_next());
    CHECK(appended.add_run(" tail").has_next());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(collect_template_part_text(header_template), "Header intro\nHeader appended tail\n");

    const auto header_xml = read_test_docx_entry(target, "word/header1.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(header_xml.c_str()));
    const auto header_node = xml_document.child("w:hdr");
    REQUIRE(header_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(header_node, "w:p"), 2U);

    fs::remove(target);
}

TEST_CASE("footer template part can append an empty paragraph and populate it later") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "footer_template_append_paragraph.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto &footer = doc.ensure_section_footer_paragraphs(0);
    REQUIRE(footer.has_next());
    CHECK(footer.set_text("Footer intro"));

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));

    auto appended = footer_template.append_paragraph();
    REQUIRE(appended.has_next());
    CHECK(appended.add_run("Footer appended").has_next());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    footer_template = reopened.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(collect_template_part_text(footer_template), "Footer intro\nFooter appended\n");

    const auto footer_xml = read_test_docx_entry(target, "word/footer1.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(footer_xml.c_str()));
    const auto footer_node = xml_document.child("w:ftr");
    REQUIRE(footer_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(footer_node, "w:p"), 2U);

    fs::remove(target);
}

TEST_CASE("header template part tables can remove a middle table and keep the wrapper usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_template_table_remove.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto header_paragraph = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header_paragraph.has_next());
    CHECK(header_paragraph.set_text("Header intro"));

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));

    auto first_table = header_template.append_table(2, 2);
    REQUIRE(first_table.has_next());
    auto row = first_table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Section"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Status"));
    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Retained"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Header table"));

    auto removable_table = header_template.append_table(1, 1);
    REQUIRE(removable_table.has_next());
    CHECK(removable_table.rows().cells().set_text("temporary middle table"));

    auto final_table = header_template.append_table(2, 2);
    REQUIRE(final_table.has_next());
    row = final_table.rows();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Final"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("State"));
    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Pending"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Will be updated"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    auto selected_table = header_template.tables();
    REQUIRE(selected_table.has_next());
    selected_table.next();
    REQUIRE(selected_table.has_next());
    CHECK(selected_table.remove());
    CHECK(selected_table.has_next());

    row = selected_table.rows();
    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Final"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Reached after middle-table removal"));

    CHECK_FALSE(reopened.save());

    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    pugi::xml_document header_document;
    REQUIRE(header_document.load_string(saved_header.c_str()));
    const auto header_root = header_document.child("w:hdr");
    REQUIRE(header_root != pugi::xml_node{});
    CHECK_EQ(count_named_children(header_root, "w:tbl"), 2U);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());

    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(collect_template_part_text(header_template), "Header intro\n");
    CHECK_EQ(collect_template_part_table_text(header_template),
             "Section\nStatus\nRetained\nHeader table\nFinal\nState\nFinal\nReached after middle-table removal\n");

    fs::remove(target);
}

TEST_CASE("header and footer template parts reuse bookmark template APIs") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_footer_template_parts.docx";
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
  <w:tbl>
    <w:tblGrid>
      <w:gridCol w:w="2400"/>
      <w:gridCol w:w="2400"/>
    </w:tblGrid>
    <w:tr>
      <w:tc><w:p><w:r><w:t>Name</w:t></w:r></w:p></w:tc>
      <w:tc><w:p><w:r><w:t>Qty</w:t></w:r></w:p></w:tc>
    </w:tr>
    <w:tr>
      <w:tc>
        <w:p>
          <w:bookmarkStart w:id="0" w:name="item_row"/>
          <w:r><w:t>template item</w:t></w:r>
          <w:bookmarkEnd w:id="0"/>
        </w:p>
      </w:tc>
      <w:tc><w:p><w:r><w:t>0</w:t></w:r></w:p></w:tc>
    </w:tr>
  </w:tbl>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t>Company: </w:t></w:r>
    <w:bookmarkStart w:id="1" w:name="company_name"/>
    <w:r><w:t>old company</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="2" w:name="footer_lines"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="2"/>
  </w:p>
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
    CHECK_EQ(doc.body_template().entry_name(), test_document_xml_entry);
    CHECK_EQ(doc.header_template().entry_name(), "word/header1.xml");
    CHECK_EQ(doc.footer_template().entry_name(), "word/footer1.xml");
    CHECK_FALSE(static_cast<bool>(doc.section_header_template(1)));

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_table_rows(
                 "item_row", {{"Apple", "2"}, {"Pear", "5"}}),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    const auto footer_fill_result =
        footer_template.fill_bookmarks({{"company_name", "Acme Corp"}});
    CHECK_EQ(footer_fill_result.requested, 1);
    CHECK_EQ(footer_fill_result.matched, 1);
    CHECK_EQ(footer_fill_result.replaced, 1);
    CHECK(footer_fill_result);
    CHECK_EQ(footer_template.replace_bookmark_with_paragraphs(
                 "footer_lines", {"First line", "Second line"}),
             1);

    CHECK_FALSE(doc.save());

    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(saved_header.find("template item"), std::string::npos);
    CHECK_EQ(saved_header.find("w:name=\"item_row\""), std::string::npos);
    CHECK_NE(saved_header.find("Apple"), std::string::npos);
    CHECK_NE(saved_header.find("Pear"), std::string::npos);

    pugi::xml_document header_document;
    REQUIRE(header_document.load_string(saved_header.c_str()));
    const auto header_table = header_document.child("w:hdr").child("w:tbl");
    REQUIRE(header_table != pugi::xml_node{});
    CHECK_EQ(count_named_children(header_table, "w:tr"), 3);

    const auto saved_footer = read_test_docx_entry(target, "word/footer1.xml");
    CHECK_NE(saved_footer.find("Company: "), std::string::npos);
    CHECK_NE(saved_footer.find("Acme Corp"), std::string::npos);
    CHECK_NE(saved_footer.find("First line"), std::string::npos);
    CHECK_NE(saved_footer.find("Second line"), std::string::npos);
    CHECK_EQ(saved_footer.find("w:name=\"footer_lines\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    std::ostringstream footer_lines;
    for (auto paragraph = reopened.section_footer_paragraphs(0); paragraph.has_next();
         paragraph.next()) {
        for (auto run = paragraph.runs(); run.has_next(); run.next()) {
            footer_lines << run.get_text();
        }
        footer_lines << '\n';
    }
    CHECK_EQ(footer_lines.str(), "Company: Acme Corp\nFirst line\nSecond line\n");

    fs::remove(target);
}

TEST_CASE("header and footer template parts can remove standalone bookmark blocks") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_remove_bookmark_block.docx";
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
  <w:p><w:r><w:t>Header keep</w:t></w:r></w:p>
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_block"/>
    <w:r><w:t>Header delete</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>Footer keep</w:t></w:r></w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="footer_block"/>
    <w:r><w:t>Footer delete</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
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

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.remove_bookmark_block("header_block"), 1);
    CHECK_FALSE(doc.last_error());

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.remove_bookmark_block("footer_block"), 1);
    CHECK_FALSE(doc.last_error());

    CHECK_FALSE(doc.save());

    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    CHECK_NE(saved_header.find("Header keep"), std::string::npos);
    CHECK_EQ(saved_header.find("Header delete"), std::string::npos);
    CHECK_EQ(saved_header.find("w:name=\"header_block\""), std::string::npos);

    const auto saved_footer = read_test_docx_entry(target, "word/footer1.xml");
    CHECK_NE(saved_footer.find("Footer keep"), std::string::npos);
    CHECK_EQ(saved_footer.find("Footer delete"), std::string::npos);
    CHECK_EQ(saved_footer.find("w:name=\"footer_block\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    fs::remove(target);
}

TEST_CASE("header and footer template parts can replace bookmark placeholders with images") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "header_footer_template_images.docx";
    const fs::path image_path = fs::current_path() / "header_footer_template_images.png";
    fs::remove(target);
    fs::remove(image_path);

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
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:bookmarkStart w:id="1" w:name="footer_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
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

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo", image_path, 20U, 10U),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.replace_bookmark_with_image("footer_logo", image_path, 30U, 15U),
             1);

    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);
    CHECK_EQ(read_test_docx_entry(target, "word/media/image2.png"), image_data);

    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    CHECK_NE(saved_header.find("<w:drawing"), std::string::npos);
    CHECK_NE(saved_header.find("cx=\"190500\""), std::string::npos);
    CHECK_NE(saved_header.find("cy=\"95250\""), std::string::npos);
    CHECK_EQ(saved_header.find("w:name=\"header_logo\""), std::string::npos);

    const auto saved_footer = read_test_docx_entry(target, "word/footer1.xml");
    CHECK_NE(saved_footer.find("<w:drawing"), std::string::npos);
    CHECK_NE(saved_footer.find("cx=\"285750\""), std::string::npos);
    CHECK_NE(saved_footer.find("cy=\"142875\""), std::string::npos);
    CHECK_EQ(saved_footer.find("w:name=\"footer_logo\""), std::string::npos);

    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_NE(saved_header_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);

    const auto saved_footer_relationships =
        read_test_docx_entry(target, "word/_rels/footer1.xml.rels");
    CHECK_NE(saved_footer_relationships.find("Target=\"media/image2.png\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/_rels/header1.xml.rels"));
    CHECK(test_docx_entry_exists(target, "word/_rels/footer1.xml.rels"));
    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("template parts list existing inline images and can extract them") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "template_part_inline_images.docx";
    const fs::path image_path = fs::current_path() / "template_part_inline_images.png";
    const fs::path extracted_path =
        fs::current_path() / "template_part_inline_images_extracted.png";
    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);

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
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo_one"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_logo_two"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:bookmarkStart w:id="2" w:name="footer_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="2"/>
  </w:p>
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

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    const std::vector<unsigned char> expected_image_data(image_data.begin(), image_data.end());
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_one", image_path, 20U, 10U),
             1);
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_two", image_path, 30U, 15U),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.replace_bookmark_with_image("footer_logo", image_path, 40U, 20U),
             1);

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto header_images = header_template.inline_images();
    REQUIRE_EQ(header_images.size(), 2U);
    CHECK_EQ(header_images[0].entry_name, "word/media/image1.png");
    CHECK_EQ(header_images[0].content_type, "image/png");
    CHECK_EQ(header_images[0].width_px, 20U);
    CHECK_EQ(header_images[0].height_px, 10U);
    CHECK_EQ(header_images[1].entry_name, "word/media/image2.png");
    CHECK_EQ(header_images[1].content_type, "image/png");
    CHECK_EQ(header_images[1].width_px, 30U);
    CHECK_EQ(header_images[1].height_px, 15U);
    CHECK(header_template.extract_inline_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_image_data);
    CHECK_FALSE(header_template.extract_inline_image(2U, extracted_path));
    CHECK_EQ(reopened.last_error().code,
             std::make_error_code(std::errc::result_out_of_range));

    footer_template = reopened.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    const auto footer_images = footer_template.inline_images();
    REQUIRE_EQ(footer_images.size(), 1U);
    CHECK_EQ(footer_images[0].entry_name, "word/media/image3.png");
    CHECK_EQ(footer_images[0].content_type, "image/png");
    CHECK_EQ(footer_images[0].width_px, 40U);
    CHECK_EQ(footer_images[0].height_px, 20U);

    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);
}

TEST_CASE("template part replace_inline_image updates only the selected header image") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };
    constexpr unsigned char tiny_gif_bytes[] = {
        0x47U, 0x49U, 0x46U, 0x38U, 0x39U, 0x61U, 0x01U, 0x00U, 0x01U, 0x00U, 0x80U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0x21U, 0xF9U, 0x04U,
        0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x2CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x01U, 0x00U, 0x00U, 0x02U, 0x02U, 0x44U, 0x01U, 0x00U, 0x3BU,
    };

    const fs::path target = fs::current_path() / "template_part_replace_inline_image.docx";
    const fs::path source_image_path =
        fs::current_path() / "template_part_replace_inline_image_source.png";
    const fs::path replacement_image_path =
        fs::current_path() / "template_part_replace_inline_image_replacement.gif";
    const fs::path extracted_path =
        fs::current_path() / "template_part_replace_inline_image_extracted.gif";
    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);

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
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo_one"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_logo_two"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:bookmarkStart w:id="2" w:name="footer_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="2"/>
  </w:p>
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

    const std::string source_image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                        sizeof(tiny_png_bytes));
    const std::string replacement_image_data(reinterpret_cast<const char *>(tiny_gif_bytes),
                                             sizeof(tiny_gif_bytes));
    const std::vector<unsigned char> expected_replacement_data(
        replacement_image_data.begin(), replacement_image_data.end());
    write_binary_file(source_image_path, source_image_data);
    write_binary_file(replacement_image_path, replacement_image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_one",
                                                         source_image_path, 20U, 10U),
             1);
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_two",
                                                         source_image_path, 30U, 15U),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.replace_bookmark_with_image("footer_logo", source_image_path,
                                                         40U, 20U),
             1);
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK(header_template.replace_inline_image(1U, replacement_image_path));

    auto header_images = header_template.inline_images();
    REQUIRE_EQ(header_images.size(), 2U);
    CHECK_EQ(header_images[0].entry_name, "word/media/image1.png");
    CHECK_EQ(header_images[0].content_type, "image/png");
    CHECK_EQ(header_images[1].width_px, 30U);
    CHECK_EQ(header_images[1].height_px, 15U);
    CHECK_EQ(header_images[1].content_type, "image/gif");
    CHECK(header_images[1].entry_name.ends_with(".gif"));
    CHECK_NE(header_images[1].entry_name, "word/media/image2.png");

    footer_template = reopened.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    auto footer_images = footer_template.inline_images();
    REQUIRE_EQ(footer_images.size(), 1U);
    CHECK_EQ(footer_images[0].entry_name, "word/media/image3.png");
    CHECK_EQ(footer_images[0].content_type, "image/png");

    const auto replacement_entry_name = header_images[1].entry_name;
    const auto replacement_target =
        (fs::path{"media"} / fs::path{replacement_entry_name}.filename()).generic_string();

    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image3.png"));
    CHECK(test_docx_entry_exists(target, replacement_entry_name.c_str()));
    CHECK_EQ(read_test_docx_entry(target, replacement_entry_name.c_str()),
             replacement_image_data);

    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_NE(saved_header_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_EQ(saved_header_relationships.find("Target=\"media/image2.png\""),
             std::string::npos);
    CHECK_NE(saved_header_relationships.find("Target=\"" + replacement_target + "\""),
             std::string::npos);

    const auto saved_footer_relationships =
        read_test_docx_entry(target, "word/_rels/footer1.xml.rels");
    CHECK_NE(saved_footer_relationships.find("Target=\"media/image3.png\""),
             std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("Extension=\"gif\""), std::string::npos);
    CHECK_NE(saved_content_types.find("ContentType=\"image/gif\""), std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    header_images = header_template.inline_images();
    REQUIRE_EQ(header_images.size(), 2U);
    CHECK_EQ(header_images[1].content_type, "image/gif");
    CHECK_EQ(header_images[1].width_px, 30U);
    CHECK_EQ(header_images[1].height_px, 15U);
    CHECK_EQ(header_images[1].entry_name, replacement_entry_name);
    CHECK(header_template.extract_inline_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_replacement_data);

    footer_template = reopened_again.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    footer_images = footer_template.inline_images();
    REQUIRE_EQ(footer_images.size(), 1U);
    CHECK_EQ(footer_images[0].entry_name, "word/media/image3.png");
    CHECK_EQ(footer_images[0].content_type, "image/png");

    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);
}

TEST_CASE("template part drawing_images includes anchored header images") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };
    constexpr unsigned char tiny_gif_bytes[] = {
        0x47U, 0x49U, 0x46U, 0x38U, 0x39U, 0x61U, 0x01U, 0x00U, 0x01U, 0x00U, 0x80U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0x21U, 0xF9U, 0x04U,
        0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x2CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x01U, 0x00U, 0x00U, 0x02U, 0x02U, 0x44U, 0x01U, 0x00U, 0x3BU,
    };

    const fs::path target = fs::current_path() / "template_part_drawing_images_anchor.docx";
    const fs::path source_image_path =
        fs::current_path() / "template_part_drawing_images_anchor_source.png";
    const fs::path replacement_image_path =
        fs::current_path() / "template_part_drawing_images_anchor_replacement.gif";
    const fs::path extracted_path =
        fs::current_path() / "template_part_drawing_images_anchor_extracted.gif";
    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);

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
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo_one"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_logo_two"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:bookmarkStart w:id="2" w:name="footer_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="2"/>
  </w:p>
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

    const std::string source_image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                        sizeof(tiny_png_bytes));
    const std::string replacement_image_data(reinterpret_cast<const char *>(tiny_gif_bytes),
                                             sizeof(tiny_gif_bytes));
    const std::vector<unsigned char> expected_replacement_data(
        replacement_image_data.begin(), replacement_image_data.end());
    write_binary_file(source_image_path, source_image_data);
    write_binary_file(replacement_image_path, replacement_image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_one",
                                                         source_image_path, 20U, 10U),
             1);
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_two",
                                                         source_image_path, 30U, 15U),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.replace_bookmark_with_image("footer_logo", source_image_path,
                                                         40U, 20U),
             1);
    CHECK_FALSE(doc.save());

    auto anchored_header_xml = read_test_docx_entry(target, "word/header1.xml");
    anchored_header_xml = convert_nth_inline_drawing_to_anchor(anchored_header_xml, 1U);
    CHECK_EQ(count_substring_occurrences(anchored_header_xml, "<wp:anchor"), 1U);
    rewrite_test_docx_entry(target, "word/header1.xml", std::move(anchored_header_xml));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));

    const auto header_drawing_images = header_template.drawing_images();
    REQUIRE_EQ(header_drawing_images.size(), 2U);
    CHECK_EQ(header_drawing_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(header_drawing_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(header_drawing_images[1].width_px, 30U);
    CHECK_EQ(header_drawing_images[1].height_px, 15U);

    const auto header_inline_images = header_template.inline_images();
    REQUIRE_EQ(header_inline_images.size(), 1U);
    CHECK_EQ(header_inline_images[0].entry_name, "word/media/image1.png");

    CHECK(header_template.extract_drawing_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path),
             std::vector<unsigned char>(source_image_data.begin(), source_image_data.end()));

    CHECK(header_template.replace_drawing_image(1U, replacement_image_path));

    auto updated_header_images = header_template.drawing_images();
    REQUIRE_EQ(updated_header_images.size(), 2U);
    CHECK_EQ(updated_header_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(updated_header_images[1].content_type, "image/gif");
    CHECK(updated_header_images[1].entry_name.ends_with(".gif"));
    const auto replacement_entry_name = updated_header_images[1].entry_name;
    const auto replacement_target =
        (fs::path{"media"} / fs::path{replacement_entry_name}.filename()).generic_string();

    CHECK_FALSE(reopened.save());

    const auto saved_header_xml = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:inline"), 1U);
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:anchor"), 1U);

    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_NE(saved_header_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_EQ(saved_header_relationships.find("Target=\"media/image2.png\""), std::string::npos);
    CHECK_NE(saved_header_relationships.find("Target=\"" + replacement_target + "\""),
             std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    updated_header_images = header_template.drawing_images();
    REQUIRE_EQ(updated_header_images.size(), 2U);
    CHECK_EQ(updated_header_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(updated_header_images[1].content_type, "image/gif");
    CHECK_EQ(updated_header_images[1].entry_name, replacement_entry_name);
    CHECK(header_template.extract_drawing_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_replacement_data);

    footer_template = reopened_again.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    const auto footer_images = footer_template.inline_images();
    REQUIRE_EQ(footer_images.size(), 1U);
    CHECK_EQ(footer_images[0].entry_name, "word/media/image3.png");
    CHECK_EQ(footer_images[0].content_type, "image/png");

    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);
}

TEST_CASE("template part remove_drawing_image and remove_inline_image prune header media") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "template_part_remove_images.docx";
    const fs::path image_path = fs::current_path() / "template_part_remove_images_source.png";
    fs::remove(target);
    fs::remove(image_path);

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
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo_one"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_logo_two"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:bookmarkStart w:id="2" w:name="footer_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="2"/>
  </w:p>
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

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_one", image_path,
                                                         20U, 10U),
             1);
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_two", image_path,
                                                         30U, 15U),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.replace_bookmark_with_image("footer_logo", image_path, 40U, 20U),
             1);
    CHECK_FALSE(doc.save());

    auto anchored_header_xml = read_test_docx_entry(target, "word/header1.xml");
    anchored_header_xml = convert_nth_inline_drawing_to_anchor(anchored_header_xml, 1U);
    CHECK_EQ(count_substring_occurrences(anchored_header_xml, "<wp:anchor"), 1U);
    rewrite_test_docx_entry(target, "word/header1.xml", std::move(anchored_header_xml));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));

    auto header_drawing_images = header_template.drawing_images();
    REQUIRE_EQ(header_drawing_images.size(), 2U);
    CHECK_EQ(header_drawing_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);

    CHECK(header_template.remove_drawing_image(1U));
    header_drawing_images = header_template.drawing_images();
    REQUIRE_EQ(header_drawing_images.size(), 1U);
    CHECK_EQ(header_drawing_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(header_template.inline_images().size(), 1U);

    CHECK(header_template.remove_inline_image(0U));
    CHECK(header_template.drawing_images().empty());
    CHECK(header_template.inline_images().empty());
    CHECK_FALSE(header_template.remove_drawing_image(0U));
    CHECK_EQ(reopened.last_error().code,
             std::make_error_code(std::errc::result_out_of_range));

    footer_template = reopened.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    const auto footer_images = footer_template.inline_images();
    REQUIRE_EQ(footer_images.size(), 1U);
    CHECK_EQ(footer_images[0].entry_name, "word/media/image3.png");

    CHECK_FALSE(reopened.save());

    const auto saved_header_xml = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:inline"), 0U);
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:anchor"), 0U);
    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_EQ(saved_header_relationships.find("relationships/image"), std::string::npos);

    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image3.png"));

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK(header_template.drawing_images().empty());
    CHECK(header_template.inline_images().empty());

    footer_template = reopened_again.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.inline_images().size(), 1U);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("template part replace_bookmark_with_floating_image writes anchored header drawings") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target =
        fs::current_path() / "template_part_replace_bookmark_floating_image.docx";
    const fs::path image_path =
        fs::current_path() / "template_part_replace_bookmark_floating_image.png";
    const fs::path extracted_path =
        fs::current_path() /
        "template_part_replace_bookmark_floating_image_extracted.png";
    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);

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
</Relationships>
)";

    const std::string header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>before</w:t></w:r></w:p>
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p><w:r><w:t>after</w:t></w:r></w:p>
</w:hdr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header_xml},
        });

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    const std::vector<unsigned char> expected_image_data(image_data.begin(), image_data.end());
    write_binary_file(image_path, image_data);

    featherdoc::floating_image_options options;
    options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    options.horizontal_offset_px = 40;
    options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    options.vertical_offset_px = 12;

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_floating_image("header_logo",
                                                                  image_path, 30U, 15U,
                                                                  options),
             1);
    CHECK_FALSE(doc.save());

    const auto saved_header_xml = read_test_docx_entry(target, "word/header1.xml");
    CHECK_NE(saved_header_xml.find("<wp:anchor"), std::string::npos);
    CHECK_NE(saved_header_xml.find("relativeFrom=\"page\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("relativeFrom=\"margin\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("<wp:posOffset>381000</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_header_xml.find("<wp:posOffset>114300</wp:posOffset>"),
             std::string::npos);
    CHECK_EQ(saved_header_xml.find("w:name=\"header_logo\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto drawing_images = header_template.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(drawing_images[0].width_px, 30U);
    CHECK_EQ(drawing_images[0].height_px, 15U);
    CHECK(header_template.extract_drawing_image(0U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_image_data);

    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);
}

TEST_CASE("body template part can append inline images and preserve them across reopen save") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "body_template_append_image_roundtrip.docx";
    const fs::path image_path =
        fs::current_path() / "body_template_append_image_roundtrip.png";
    fs::remove(target);
    fs::remove(image_path);

    const auto image_data = tiny_png_data();
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK(body_template.append_image(image_path));
    CHECK(body_template.append_image(image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);
    CHECK_EQ(read_test_docx_entry(target, "word/media/image2.png"), image_data);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_relationships,
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\""),
             2U);
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"media/image2.png\""), std::string::npos);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 2U);
    CHECK_NE(saved_document_xml.find("cx=\"9525\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"9525\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cx=\"190500\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"95250\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto body_images = body_template.inline_images();
    REQUIRE_EQ(body_images.size(), 2U);
    CHECK_EQ(body_images[0].content_type, "image/png");
    CHECK_EQ(body_images[0].width_px, 1U);
    CHECK_EQ(body_images[0].height_px, 1U);
    CHECK_EQ(body_images[1].content_type, "image/png");
    CHECK_EQ(body_images[1].width_px, 20U);
    CHECK_EQ(body_images[1].height_px, 10U);

    auto paragraph = body_template.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run("reopened edit").has_next());
    CHECK_FALSE(reopened.save());

    const auto relationships_after_resave =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(relationships_after_resave.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_NE(relationships_after_resave.find("Target=\"media/image2.png\""),
             std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("body template part can append floating images and preserve them across reopen save") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "body_template_append_floating_image_roundtrip.docx";
    const fs::path image_path =
        fs::current_path() / "body_template_append_floating_image_roundtrip.png";
    fs::remove(target);
    fs::remove(image_path);

    const auto image_data = tiny_png_data();
    write_binary_file(image_path, image_data);

    featherdoc::floating_image_options options;
    options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    options.horizontal_offset_px = 24;
    options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    options.vertical_offset_px = -8;
    options.behind_text = true;
    options.allow_overlap = false;

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK(body_template.append_floating_image(image_path, 20U, 10U, options));
    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_relationships,
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\""),
             1U);
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""), std::string::npos);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:anchor"), 1U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 0U);
    CHECK_NE(saved_document_xml.find("relativeFrom=\"page\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("relativeFrom=\"margin\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:posOffset>228600</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:posOffset>-76200</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("behindDoc=\"1\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("allowOverlap=\"0\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto drawing_images = body_template.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(drawing_images[0].content_type, "image/png");
    CHECK_EQ(drawing_images[0].width_px, 20U);
    CHECK_EQ(drawing_images[0].height_px, 10U);
    CHECK_EQ(body_template.inline_images().size(), 0U);

    auto paragraph = body_template.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run("reopened edit").has_next());
    CHECK_FALSE(reopened.save());

    const auto relationships_after_resave =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(relationships_after_resave.find("Target=\"media/image1.png\""),
             std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("header template part can append inline images") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_template_append_images.docx";
    const fs::path image_path =
        fs::current_path() / "header_template_append_images.png";
    fs::remove(target);
    fs::remove(image_path);

    const auto image_data = tiny_png_data();
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    auto &header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.add_run("header intro").has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK(header_template.append_image(image_path));
    CHECK(header_template.append_image(image_path, 30U, 15U));
    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));

    const auto saved_header_xml = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:inline"), 2U);
    CHECK_NE(saved_header_xml.find("cx=\"9525\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("cy=\"9525\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("cx=\"285750\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("cy=\"142875\""), std::string::npos);

    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_header_relationships,
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\""),
             2U);
    CHECK_NE(saved_header_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_NE(saved_header_relationships.find("Target=\"media/image2.png\""),
             std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("Extension=\"png\""), std::string::npos);
    CHECK_NE(saved_content_types.find("ContentType=\"image/png\""), std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());

    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto header_images = header_template.inline_images();
    REQUIRE_EQ(header_images.size(), 2U);
    CHECK_EQ(header_images[0].content_type, "image/png");
    CHECK_EQ(header_images[0].width_px, 1U);
    CHECK_EQ(header_images[0].height_px, 1U);
    CHECK_EQ(header_images[1].content_type, "image/png");
    CHECK_EQ(header_images[1].width_px, 30U);
    CHECK_EQ(header_images[1].height_px, 15U);

    CHECK(reopened_again.header_paragraphs().add_run(" reopened edit").has_next());
    CHECK_FALSE(reopened_again.save());

    const auto relationships_after_resave =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_NE(relationships_after_resave.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_NE(relationships_after_resave.find("Target=\"media/image2.png\""),
             std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("header template part can append floating images") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "header_template_append_floating_images.docx";
    const fs::path image_path =
        fs::current_path() / "header_template_append_floating_images.png";
    fs::remove(target);
    fs::remove(image_path);

    const auto image_data = tiny_png_data();
    write_binary_file(image_path, image_data);

    featherdoc::floating_image_options options;
    options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    options.horizontal_offset_px = 40;
    options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    options.vertical_offset_px = 12;

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    auto &header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.add_run("header intro").has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK(header_template.append_floating_image(image_path, 30U, 15U, options));
    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);

    const auto saved_header_xml = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:anchor"), 1U);
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:inline"), 0U);
    CHECK_NE(saved_header_xml.find("relativeFrom=\"page\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("relativeFrom=\"margin\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("<wp:posOffset>381000</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_header_xml.find("<wp:posOffset>114300</wp:posOffset>"),
             std::string::npos);

    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_header_relationships,
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\""),
             1U);
    CHECK_NE(saved_header_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());

    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto drawing_images = header_template.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(drawing_images[0].content_type, "image/png");
    CHECK_EQ(drawing_images[0].width_px, 30U);
    CHECK_EQ(drawing_images[0].height_px, 15U);
    CHECK_EQ(header_template.inline_images().size(), 0U);

    CHECK(reopened_again.header_paragraphs().add_run(" reopened edit").has_next());
    CHECK_FALSE(reopened_again.save());

    const auto relationships_after_resave =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_NE(relationships_after_resave.find("Target=\"media/image1.png\""),
             std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("apply_bookmark_block_visibility keeps and removes body template blocks") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_block_visibility.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p><w:bookmarkStart w:id="0" w:name="keep_block"/></w:p>
    <w:p><w:r><w:t>Keep me</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="0"/></w:p>
    <w:p><w:r><w:t>middle</w:t></w:r></w:p>
    <w:p><w:bookmarkStart w:id="1" w:name="hide_block"/></w:p>
    <w:tbl>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Secret Cell</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>Hide me</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="1"/></w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.apply_bookmark_block_visibility({
        {"keep_block", true},
        {"hide_block", false},
    });
    CHECK_EQ(result.requested, 2);
    CHECK_EQ(result.matched, 2);
    CHECK_EQ(result.kept, 1);
    CHECK_EQ(result.removed, 1);
    CHECK(result);
    CHECK_FALSE(doc.last_error());

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("keep_block"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("hide_block"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Keep me"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("Hide me"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("Secret Cell"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nKeep me\nmiddle\nafter\n");

    fs::remove(target);
}

TEST_CASE("header template parts can change bookmark block visibility") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_template_block_visibility.docx";
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
</Relationships>
)";

    const std::string header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:bookmarkStart w:id="0" w:name="offer_block"/></w:p>
  <w:p><w:r><w:t>Offer line 1</w:t></w:r></w:p>
  <w:p><w:r><w:t>Offer line 2</w:t></w:r></w:p>
  <w:p><w:bookmarkEnd w:id="0"/></w:p>
  <w:p><w:r><w:t>Always here</w:t></w:r></w:p>
</w:hdr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.set_bookmark_block_visibility("offer_block", false), 1);

    CHECK_FALSE(doc.save());

    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(saved_header.find("offer_block"), std::string::npos);
    CHECK_EQ(saved_header.find("Offer line 1"), std::string::npos);
    CHECK_EQ(saved_header.find("Offer line 2"), std::string::npos);
    CHECK_NE(saved_header.find("Always here"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_paragraphs().runs().get_text(), "Always here");

    fs::remove(target);
}

TEST_CASE("apply_bookmark_block_visibility rejects duplicate or empty binding names") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_block_visibility_validation.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>body</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document unopened(target);
    const auto unopened_result =
        unopened.apply_bookmark_block_visibility({{"promo_block", true}});
    CHECK_EQ(unopened_result.requested, 1);
    CHECK_EQ(unopened.last_error().code, featherdoc::document_errc::document_not_open);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto duplicate_result = doc.apply_bookmark_block_visibility(
        {{"promo_block", true}, {"promo_block", false}});
    CHECK_EQ(duplicate_result.requested, 2);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    const auto empty_result = doc.apply_bookmark_block_visibility({{"", true}});
    CHECK_EQ(empty_result.requested, 1);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE("ensure_header_paragraphs and ensure_footer_paragraphs work for create_empty") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "ensure_header_footer_empty.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK_EQ(doc.section_count(), 1);

    auto header = doc.ensure_header_paragraphs();
    REQUIRE(header.has_next());
    CHECK(header.add_run("generated header").has_next());

    auto footer = doc.ensure_footer_paragraphs();
    REQUIRE(footer.has_next());
    CHECK(footer.add_run("generated footer").has_next());

    CHECK_EQ(doc.header_count(), 1);
    CHECK_EQ(doc.footer_count(), 1);
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("xmlns:r="), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:headerReference"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:footerReference"), std::string::npos);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.header_paragraphs().runs().get_text(), "generated header");
    CHECK_EQ(reopened.footer_paragraphs().runs().get_text(), "generated footer");

    fs::remove(target);
}

TEST_CASE("section header and footer access resolves references per section") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "section_header_footer_access.docx";
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
  <Override PartName="/word/header2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/header3.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/footer2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId2"/>
          <w:footerReference w:type="default" r:id="rId3"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId4"/>
      <w:headerReference w:type="even" r:id="rId5"/>
      <w:footerReference w:type="first" r:id="rId6"/>
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
  <Relationship Id="rId4"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header2.xml"/>
  <Relationship Id="rId5"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header3.xml"/>
  <Relationship Id="rId6"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer2.xml"/>
</Relationships>
)";

    const std::string header1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string header2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 default header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string header3_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 even header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string footer1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string footer2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 first footer</w:t></w:r></w:p>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header1_xml},
            {"word/header2.xml", header2_xml},
            {"word/header3.xml", header3_xml},
            {"word/footer1.xml", footer1_xml},
            {"word/footer2.xml", footer2_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.section_count(), 2);
    CHECK_EQ(doc.header_count(), 3);
    CHECK_EQ(doc.footer_count(), 2);

    auto section0_header = doc.section_header_paragraphs(0);
    REQUIRE(section0_header.has_next());
    CHECK_EQ(section0_header.runs().get_text(), "section 1 header");

    auto section0_footer = doc.section_footer_paragraphs(0);
    REQUIRE(section0_footer.has_next());
    CHECK_EQ(section0_footer.runs().get_text(), "section 1 footer");

    auto section1_default_header = doc.section_header_paragraphs(1);
    REQUIRE(section1_default_header.has_next());
    CHECK_EQ(section1_default_header.runs().get_text(), "section 2 default header");

    auto section1_even_header = doc.section_header_paragraphs(
        1, featherdoc::section_reference_kind::even_page);
    REQUIRE(section1_even_header.has_next());
    CHECK_EQ(section1_even_header.runs().get_text(), "section 2 even header");

    auto section1_first_footer = doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK_EQ(section1_first_footer.runs().get_text(), "section 2 first footer");

    CHECK_FALSE(doc.section_header_paragraphs(
                    0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(1).has_next());
    CHECK_FALSE(doc.section_header_paragraphs(2).has_next());

    CHECK(section1_default_header.runs().set_text("updated section 2 header"));
    CHECK(section1_even_header.runs().set_text("updated section 2 even"));
    CHECK(section1_first_footer.runs().set_text("updated section 2 first footer"));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 2);
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(),
             "updated section 2 header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "updated section 2 even");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "updated section 2 first footer");

    fs::remove(target);
}

TEST_CASE("ensure section header and footer paragraphs create references for a single section") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "ensure_section_header_footer_single.docx";
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
    CHECK_EQ(doc.footer_count(), 0);

    auto default_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(default_header.has_next());
    CHECK(default_header.add_run("default header").has_next());

    auto even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    CHECK(even_header.add_run("even header").has_next());

    auto first_footer = doc.ensure_section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_footer.has_next());
    CHECK(first_footer.add_run("first footer").has_next());

    auto same_even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(same_even_header.has_next());
    CHECK_EQ(doc.header_count(), 2);
    CHECK_EQ(doc.footer_count(), 1);
    CHECK_EQ(same_even_header.runs().get_text(), "even header");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto saved_section_properties =
        saved_document.child("w:document").child("w:body").child("w:sectPr");
    REQUIRE(saved_section_properties != pugi::xml_node{});
    CHECK_NE(saved_document_xml.find("<w:sectPr"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:headerReference"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"default\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"even\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:footerReference"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"first\""), std::string::npos);
    CHECK(saved_section_properties.child("w:titlePg") != pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("relationships/settings"), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"settings.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/header2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/settings.xml"), std::string::npos);

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 1);
    CHECK_EQ(reopened.header_count(), 2);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "default header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "even header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 0, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "first footer");

    fs::remove(target);
}

TEST_CASE("replace section header and footer text rewrites parts cleanly") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "replace_section_header_footer_text.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK(doc.replace_section_header_text(0, "old header\nold second line"));
    CHECK(doc.replace_section_header_text(0, "new header"));
    CHECK(doc.replace_section_header_text(
        0, "even header", featherdoc::section_reference_kind::even_page));
    CHECK(doc.replace_section_footer_text(
        0, " first footer ", featherdoc::section_reference_kind::first_page));
    CHECK_FALSE(doc.save());

    auto collect_section_part_lines =
        [](featherdoc::Paragraph paragraph) -> std::vector<std::string> {
        std::vector<std::string> lines;
        for (; paragraph.has_next(); paragraph.next()) {
            std::string text;
            for (auto run = paragraph.runs(); run.has_next(); run.next()) {
                text += run.get_text();
            }
            lines.push_back(std::move(text));
        }
        return lines;
    };

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 1);
    CHECK_EQ(reopened.header_count(), 2);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(collect_section_part_lines(reopened.section_header_paragraphs(0)),
             std::vector<std::string>{"new header"});
    CHECK_EQ(collect_section_part_lines(reopened.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)),
             std::vector<std::string>{"even header"});
    CHECK_EQ(collect_section_part_lines(reopened.section_footer_paragraphs(
                 0, featherdoc::section_reference_kind::first_page)),
             std::vector<std::string>{" first footer "});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    CHECK_NE(saved_settings_xml.find("w:evenAndOddHeaders"), std::string::npos);

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.replace_section_header_text(0, "missing"));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("ensure section header and footer paragraphs create references per section") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "ensure_section_header_footer_multi.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr/>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr/>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.section_count(), 2);

    auto section0_footer = doc.ensure_section_footer_paragraphs(0);
    REQUIRE(section0_footer.has_next());
    CHECK(section0_footer.add_run("section 1 footer").has_next());

    auto section0_even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(section0_even_header.has_next());
    CHECK(section0_even_header.add_run("section 1 even header").has_next());

    auto section1_first_footer = doc.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 2 first footer").has_next());

    auto section1_default_header = doc.ensure_section_header_paragraphs(1);
    REQUIRE(section1_default_header.has_next());
    CHECK(section1_default_header.add_run("section 2 default header").has_next());

    auto same_section1_default_header = doc.ensure_section_header_paragraphs(1);
    REQUIRE(same_section1_default_header.has_next());
    CHECK_EQ(same_section1_default_header.runs().get_text(), "section 2 default header");
    CHECK_EQ(doc.header_count(), 2);
    CHECK_EQ(doc.footer_count(), 2);

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    const auto section0_properties = body.child("w:p").child("w:pPr").child("w:sectPr");
    const auto section1_properties = body.child("w:sectPr");
    REQUIRE(section0_properties != pugi::xml_node{});
    REQUIRE(section1_properties != pugi::xml_node{});
    CHECK_NE(saved_document_xml.find("w:type=\"even\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"first\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"default\""), std::string::npos);
    CHECK(section0_properties.child("w:titlePg") == pugi::xml_node{});
    CHECK(section1_properties.child("w:titlePg") != pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"settings.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/header2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/settings.xml"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 2);
    CHECK_EQ(reopened.header_count(), 2);
    CHECK_EQ(reopened.footer_count(), 2);
    CHECK_EQ(reopened.section_footer_paragraphs(0).runs().get_text(), "section 1 footer");
    CHECK_EQ(reopened.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 1 even header");
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(),
             "section 2 default header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 2 first footer");

    fs::remove(target);
}

TEST_CASE("ensure even-page section headers preserve existing settings.xml content") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "ensure_section_even_settings_reuse.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/settings.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>body text</w:t></w:r></w:p>
  </w:body>
</w:document>
)";

    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings"
                Target="settings.xml"/>
</Relationships>
)";

    const std::string settings_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:zoom w:percent="125"/>
</w:settings>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/settings.xml", settings_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    CHECK(even_header.add_run("even header").has_next());

    CHECK_FALSE(doc.save());

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    const auto settings_root = saved_settings.child("w:settings");
    REQUIRE(settings_root != pugi::xml_node{});
    CHECK(settings_root.child("w:zoom") != pugi::xml_node{});
    CHECK(settings_root.child("w:evenAndOddHeaders") != pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"settings.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "even header");

    fs::remove(target);
}

TEST_CASE("assign section header and footer paragraphs reuse existing parts") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "assign_section_header_footer_existing_parts.docx";
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
  <Override PartName="/word/header2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/footer2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId2"/>
          <w:footerReference w:type="default" r:id="rId3"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId4"/>
      <w:footerReference w:type="default" r:id="rId5"/>
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
  <Relationship Id="rId4"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header2.xml"/>
  <Relationship Id="rId5"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer2.xml"/>
</Relationships>
)";

    const std::string header1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string header2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string footer1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string footer2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 footer</w:t></w:r></w:p>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header1_xml},
            {"word/header2.xml", header2_xml},
            {"word/footer1.xml", footer1_xml},
            {"word/footer2.xml", footer2_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.header_count(), 2);
    CHECK_EQ(doc.footer_count(), 2);

    auto shared_default_header = doc.assign_section_header_paragraphs(1, 0);
    REQUIRE(shared_default_header.has_next());
    CHECK(shared_default_header.runs().set_text("shared header"));

    auto shared_even_header = doc.assign_section_header_paragraphs(
        1, 0, featherdoc::section_reference_kind::even_page);
    REQUIRE(shared_even_header.has_next());
    CHECK_EQ(shared_even_header.runs().get_text(), "shared header");

    auto shared_default_footer = doc.assign_section_footer_paragraphs(1, 0);
    REQUIRE(shared_default_footer.has_next());
    CHECK(shared_default_footer.runs().set_text("shared footer"));

    auto shared_first_footer = doc.assign_section_footer_paragraphs(
        1, 0, featherdoc::section_reference_kind::first_page);
    REQUIRE(shared_first_footer.has_next());
    CHECK_EQ(shared_first_footer.runs().get_text(), "shared footer");

    CHECK_EQ(doc.header_count(), 2);
    CHECK_EQ(doc.footer_count(), 2);
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    const auto section0_properties = body.child("w:p").child("w:pPr").child("w:sectPr");
    const auto section1_properties = body.child("w:sectPr");
    REQUIRE(section0_properties != pugi::xml_node{});
    REQUIRE(section1_properties != pugi::xml_node{});

    const auto find_reference_id =
        [](pugi::xml_node section_properties, const char *reference_name,
           const char *reference_type) -> std::string {
        for (auto reference = section_properties.child(reference_name);
             reference != pugi::xml_node{};
             reference = reference.next_sibling(reference_name)) {
            if (std::string_view{reference.attribute("w:type").value()} == reference_type) {
                return reference.attribute("r:id").value();
            }
        }
        return {};
    };

    CHECK_EQ(find_reference_id(section0_properties, "w:headerReference", "default"), "rId2");
    CHECK_EQ(find_reference_id(section1_properties, "w:headerReference", "default"), "rId2");
    CHECK_EQ(find_reference_id(section1_properties, "w:headerReference", "even"), "rId2");
    CHECK_EQ(find_reference_id(section0_properties, "w:footerReference", "default"), "rId3");
    CHECK_EQ(find_reference_id(section1_properties, "w:footerReference", "default"), "rId3");
    CHECK_EQ(find_reference_id(section1_properties, "w:footerReference", "first"), "rId3");
    CHECK(section1_properties.child("w:titlePg") != pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"footer2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"settings.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/header2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/footer2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/settings.xml"), std::string::npos);

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK(test_docx_entry_exists(target, "word/footer1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer2.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "shared header");
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "shared header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "shared header");
    CHECK_EQ(reopened.section_footer_paragraphs(0).runs().get_text(), "shared footer");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "shared footer");

    fs::remove(target);
}

TEST_CASE("remove section header and footer references prunes orphaned parts on save") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "remove_section_header_footer_references.docx";
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
  <Override PartName="/word/header2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/footer2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId2"/>
          <w:footerReference w:type="default" r:id="rId3"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId4"/>
      <w:footerReference w:type="default" r:id="rId5"/>
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
  <Relationship Id="rId4"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header2.xml"/>
  <Relationship Id="rId5"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer2.xml"/>
</Relationships>
)";

    const std::string header1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string header2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string footer1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string footer2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 footer</w:t></w:r></w:p>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header1_xml},
            {"word/header2.xml", header2_xml},
            {"word/footer1.xml", footer1_xml},
            {"word/footer2.xml", footer2_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK(doc.remove_section_header_reference(1));
    CHECK(doc.remove_section_footer_reference(1));
    CHECK_FALSE(doc.remove_section_header_reference(
        1, featherdoc::section_reference_kind::even_page));
    CHECK_FALSE(doc.last_error());
    CHECK_FALSE(doc.section_header_paragraphs(1).has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(1).has_next());
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    const auto section1_properties = body.child("w:sectPr");
    REQUIRE(section1_properties != pugi::xml_node{});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:headerReference", "w:type",
                                                         "default"),
             pugi::xml_node{});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:footerReference", "w:type",
                                                         "default"),
             pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"footer2.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/header2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/footer2.xml"), std::string::npos);

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK(test_docx_entry_exists(target, "word/footer1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer2.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(0).runs().get_text(), "section 1 footer");
    CHECK_FALSE(reopened.section_header_paragraphs(1).has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(1).has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.remove_section_header_reference(0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("removing first and even references cleans title page and settings flags") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "remove_section_first_even_reference_cleanup.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    CHECK(even_header.add_run("even header").has_next());

    auto first_footer = doc.ensure_section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_footer.has_next());
    CHECK(first_footer.add_run("first footer").has_next());

    CHECK(doc.remove_section_header_reference(
        0, featherdoc::section_reference_kind::even_page));
    CHECK(doc.remove_section_footer_reference(
        0, featherdoc::section_reference_kind::first_page));
    CHECK_FALSE(doc.section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto section_properties =
        saved_document.child("w:document").child("w:body").child("w:sectPr");
    REQUIRE(section_properties != pugi::xml_node{});
    CHECK(section_properties.child("w:titlePg") == pugi::xml_node{});
    CHECK(section_properties.find_child_by_attribute("w:headerReference", "w:type", "even") ==
          pugi::xml_node{});
    CHECK(section_properties.find_child_by_attribute("w:footerReference", "w:type", "first") ==
          pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") ==
          pugi::xml_node{});

    CHECK_FALSE(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer1.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 0);
    CHECK_EQ(reopened.footer_count(), 0);
    CHECK_FALSE(reopened.section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page)
                    .has_next());

    fs::remove(target);
}

TEST_CASE("remove header and footer parts updates counts and prunes archive output") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "remove_header_footer_parts.docx";
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
  <Override PartName="/word/header2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/footer2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId2"/>
          <w:footerReference w:type="default" r:id="rId3"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId4"/>
      <w:footerReference w:type="default" r:id="rId5"/>
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
  <Relationship Id="rId4"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header2.xml"/>
  <Relationship Id="rId5"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer2.xml"/>
</Relationships>
)";

    const std::string header1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string header2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string footer1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string footer2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 footer</w:t></w:r></w:p>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header1_xml},
            {"word/header2.xml", header2_xml},
            {"word/footer1.xml", footer1_xml},
            {"word/footer2.xml", footer2_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.header_count(), 2);
    CHECK_EQ(doc.footer_count(), 2);
    CHECK(doc.remove_header_part(1));
    CHECK(doc.remove_footer_part(1));
    CHECK_EQ(doc.header_count(), 1);
    CHECK_EQ(doc.footer_count(), 1);
    CHECK_FALSE(doc.section_header_paragraphs(1).has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(1).has_next());
    CHECK_FALSE(doc.save());

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"footer2.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/header2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/footer2.xml"), std::string::npos);

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK(test_docx_entry_exists(target, "word/footer1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer2.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(0).runs().get_text(), "section 1 footer");
    CHECK_FALSE(reopened.section_header_paragraphs(1).has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(1).has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.remove_header_part(0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("copying section header and footer references replaces target layout") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "copy_section_header_footer_refs.docx";
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
  <Override PartName="/word/header2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/footer2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/settings.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId2"/>
          <w:footerReference w:type="default" r:id="rId3"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId4"/>
      <w:headerReference w:type="even" r:id="rId4"/>
      <w:footerReference w:type="default" r:id="rId5"/>
      <w:footerReference w:type="first" r:id="rId5"/>
      <w:titlePg w:val="1"/>
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
  <Relationship Id="rId4"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header2.xml"/>
  <Relationship Id="rId5"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer2.xml"/>
  <Relationship Id="rId6"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings"
                Target="settings.xml"/>
</Relationships>
)";

    const std::string header1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string header2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string footer1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string footer2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string settings_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:evenAndOddHeaders w:val="1"/>
</w:settings>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header1_xml},
            {"word/header2.xml", header2_xml},
            {"word/footer1.xml", footer1_xml},
            {"word/footer2.xml", footer2_xml},
            {"word/settings.xml", settings_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK(doc.copy_section_header_references(0, 1));
    CHECK(doc.copy_section_footer_references(0, 1));
    CHECK_FALSE(doc.section_header_paragraphs(
        1, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_footer_paragraphs(1).runs().get_text(), "section 1 footer");
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto section1_properties =
        saved_document.child("w:document").child("w:body").child("w:sectPr");
    REQUIRE(section1_properties != pugi::xml_node{});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:headerReference", "w:type",
                                                         "default")
                 .attribute("r:id")
                 .value(),
             std::string{"rId2"});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:footerReference", "w:type",
                                                         "default")
                 .attribute("r:id")
                 .value(),
             std::string{"rId3"});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:headerReference", "w:type", "even"),
             pugi::xml_node{});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:footerReference", "w:type",
                                                         "first"),
             pugi::xml_node{});
    CHECK(section1_properties.child("w:titlePg") == pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") ==
          pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"footer2.xml\""), std::string::npos);

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK(test_docx_entry_exists(target, "word/footer1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer2.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(1).runs().get_text(), "section 1 footer");
    CHECK_FALSE(reopened.section_header_paragraphs(
        1, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.copy_section_header_references(0, 1));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("append section can inherit or reset header and footer references") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "append_section_header_footer.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto default_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(default_header.has_next());
    CHECK(default_header.add_run("default header").has_next());

    auto even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    CHECK(even_header.add_run("even header").has_next());

    auto first_footer = doc.ensure_section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_footer.has_next());
    CHECK(first_footer.add_run("first footer").has_next());

    CHECK(doc.append_section());
    CHECK_EQ(doc.section_count(), 2);
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "default header");
    CHECK_EQ(doc.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "even header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "first footer");

    CHECK(doc.append_section(false));
    CHECK_EQ(doc.section_count(), 3);
    CHECK_FALSE(doc.section_header_paragraphs(2).has_next());
    CHECK_FALSE(doc.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});

    std::vector<pugi::xml_node> section_nodes;
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        if (const auto section_properties = paragraph.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            section_nodes.push_back(section_properties);
        }
    }
    section_nodes.push_back(body.child("w:sectPr"));
    REQUIRE(section_nodes.size() == 3);

    CHECK(section_nodes[1].child("w:titlePg") != pugi::xml_node{});
    CHECK(section_nodes[2].child("w:titlePg") == pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:headerReference", "w:type",
                                                      "default"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:headerReference", "w:type", "even"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:footerReference", "w:type",
                                                      "first"),
             pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 3);
    CHECK_EQ(reopened.header_count(), 2);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "default header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "even header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "first footer");
    CHECK_FALSE(reopened.section_header_paragraphs(2).has_next());
    CHECK_FALSE(reopened.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.append_section());
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("insert section can split layout inheritance or reset references") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_section_header_footer.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto section0_default_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(section0_default_header.has_next());
    CHECK(section0_default_header.add_run("section 0 header").has_next());

    auto section0_even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(section0_even_header.has_next());
    CHECK(section0_even_header.add_run("section 0 even header").has_next());

    CHECK(doc.append_section(false));
    CHECK_EQ(doc.section_count(), 2);

    auto section1_default_header = doc.ensure_section_header_paragraphs(1);
    REQUIRE(section1_default_header.has_next());
    CHECK(section1_default_header.add_run("section 1 header").has_next());

    auto section1_first_footer = doc.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 1 first footer").has_next());

    CHECK(doc.insert_section(0));
    CHECK_EQ(doc.section_count(), 3);
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(doc.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 0 even header");
    CHECK_FALSE(doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_EQ(doc.section_header_paragraphs(2).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    CHECK(doc.insert_section(1, false));
    CHECK_EQ(doc.section_count(), 4);
    CHECK_FALSE(doc.section_header_paragraphs(2).has_next());
    CHECK_FALSE(doc.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_EQ(doc.section_header_paragraphs(3).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 3, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    CHECK(doc.insert_section(3));
    CHECK_EQ(doc.section_count(), 5);
    CHECK_EQ(doc.section_header_paragraphs(4).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 4, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});

    std::vector<pugi::xml_node> section_nodes;
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        if (const auto section_properties = paragraph.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            section_nodes.push_back(section_properties);
        }
    }
    section_nodes.push_back(body.child("w:sectPr"));
    REQUIRE(section_nodes.size() == 5);

    CHECK(section_nodes[2].child("w:titlePg") == pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:headerReference", "w:type",
                                                      "default"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:headerReference", "w:type", "even"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:footerReference", "w:type",
                                                      "first"),
             pugi::xml_node{});
    CHECK(section_nodes[3].child("w:titlePg") != pugi::xml_node{});
    CHECK(section_nodes[4].child("w:titlePg") != pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 5);
    CHECK_EQ(reopened.header_count(), 3);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 0 even header");
    CHECK_FALSE(reopened.section_header_paragraphs(2).has_next());
    CHECK_FALSE(reopened.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_EQ(reopened.section_header_paragraphs(3).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 3, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");
    CHECK_EQ(reopened.section_header_paragraphs(4).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 4, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.insert_section(0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("remove section merges boundaries and prunes orphaned header footer parts on save") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "remove_section_header_footer.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto section0_default_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(section0_default_header.has_next());
    CHECK(section0_default_header.add_run("section 0 header").has_next());

    CHECK(doc.append_section(false));
    auto section1_default_header = doc.ensure_section_header_paragraphs(1);
    REQUIRE(section1_default_header.has_next());
    CHECK(section1_default_header.add_run("section 1 header").has_next());

    auto section1_first_footer = doc.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 1 first footer").has_next());

    CHECK(doc.append_section(false));
    auto section2_default_header = doc.ensure_section_header_paragraphs(2);
    REQUIRE(section2_default_header.has_next());
    CHECK(section2_default_header.add_run("section 2 header").has_next());

    auto section2_even_header = doc.ensure_section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page);
    REQUIRE(section2_even_header.has_next());
    CHECK(section2_even_header.add_run("section 2 even header").has_next());

    CHECK(doc.remove_section(1));
    CHECK_EQ(doc.section_count(), 2);
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 2 header");
    CHECK_EQ(doc.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 2 even header");
    CHECK_FALSE(doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());

    CHECK(doc.remove_section(1));
    CHECK_EQ(doc.section_count(), 1);
    CHECK_EQ(doc.section_header_paragraphs(0).runs().get_text(), "section 0 header");
    CHECK_FALSE(doc.section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_FALSE(doc.remove_section(0));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});

    std::vector<pugi::xml_node> section_nodes;
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        if (const auto section_properties = paragraph.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            section_nodes.push_back(section_properties);
        }
    }
    section_nodes.push_back(body.child("w:sectPr"));
    REQUIRE(section_nodes.size() == 1);
    CHECK(section_nodes[0].child("w:titlePg") == pugi::xml_node{});
    CHECK_EQ(section_nodes[0].find_child_by_attribute("w:headerReference", "w:type", "even"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[0].find_child_by_attribute("w:footerReference", "w:type", "first"),
             pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") ==
          pugi::xml_node{});

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header3.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer1.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 1);
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 0);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "section 0 header");
    CHECK_FALSE(reopened.section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page)
                    .has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.remove_section(0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("move section reorders body content and keeps header footer layouts attached") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "move_section_header_footer.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK(doc.paragraphs().add_run("section 0 body").has_next());
    auto section0_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(section0_header.has_next());
    CHECK(section0_header.add_run("section 0 header").has_next());

    CHECK(doc.append_section(false));

    auto append_body_paragraph = [](featherdoc::Document &document, const char *text) {
        auto paragraph = document.paragraphs();
        while (paragraph.has_next()) {
            paragraph.next();
        }

        const auto inserted = paragraph.insert_paragraph_after(text);
        REQUIRE(inserted.has_next());
    };

    append_body_paragraph(doc, "section 1 body");
    auto section1_first_footer = doc.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 1 first footer").has_next());

    CHECK(doc.append_section(false));
    append_body_paragraph(doc, "section 2 body");

    auto section2_header = doc.ensure_section_header_paragraphs(2);
    REQUIRE(section2_header.has_next());
    CHECK(section2_header.add_run("section 2 header").has_next());

    auto section2_even_header = doc.ensure_section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page);
    REQUIRE(section2_even_header.has_next());
    CHECK(section2_even_header.add_run("section 2 even header").has_next());

    CHECK(doc.move_section(2, 0));
    CHECK_EQ(doc.section_count(), 3);
    CHECK_EQ(doc.section_header_paragraphs(0).runs().get_text(), "section 2 header");
    CHECK_EQ(doc.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 2 even header");
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");
    CHECK_FALSE(doc.move_section(3, 0));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto collect_non_empty_document_text = [](featherdoc::Document &document) {
        std::ostringstream stream;
        for (auto paragraph = document.paragraphs(); paragraph.has_next(); paragraph.next()) {
            std::string text;
            for (auto run = paragraph.runs(); run.has_next(); run.next()) {
                text += run.get_text();
            }

            if (!text.empty()) {
                stream << text << '\n';
            }
        }
        return stream.str();
    };

    CHECK_EQ(collect_non_empty_document_text(reopened),
             "section 2 body\nsection 0 body\nsection 1 body\n");
    CHECK_EQ(reopened.section_count(), 3);
    CHECK_EQ(reopened.header_count(), 3);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "section 2 header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 2 even header");
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.move_section(0, 0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("table traversal exposes text stored inside table cells") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_text_access.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>outside</w:t></w:r></w:p>
    <w:tbl>
      <w:tr>
        <w:tc>
          <w:p><w:r><w:t>cell one</w:t></w:r></w:p>
          <w:p><w:r><w:t>cell two</w:t></w:r></w:p>
        </w:tc>
        <w:tc>
          <w:p><w:r><w:t>cell three</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    std::ostringstream text;
    for (auto table = doc.tables(); table.has_next(); table.next()) {
        for (auto row = table.rows(); row.has_next(); row.next()) {
            for (auto cell = row.cells(); cell.has_next(); cell.next()) {
                for (auto paragraph = cell.paragraphs(); paragraph.has_next();
                     paragraph.next()) {
                    for (auto run = paragraph.runs(); run.has_next(); run.next()) {
                        text << run.get_text();
                    }
                    text << '\n';
                }
            }
        }
    }

    CHECK_EQ(text.str(), "cell one\ncell two\ncell three\n");

    fs::remove(target);
}

TEST_CASE("fill_bookmarks replaces multiple bookmark bindings and reports misses") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_fill_batch.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>Customer: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>old customer</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:r><w:t>Invoice: </w:t></w:r>
      <w:bookmarkStart w:id="1" w:name="invoice"/>
      <w:r><w:t>old invoice</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.fill_bookmarks(
        {
            {"customer", "Acme Corp"},
            {"invoice", "INV-2026-0001"},
            {"missing", "ignored"},
        });

    CHECK_FALSE(doc.last_error());
    CHECK_EQ(result.requested, 3);
    CHECK_EQ(result.matched, 2);
    CHECK_EQ(result.replaced, 2);
    REQUIRE(result.missing_bookmarks.size() == 1);
    CHECK_EQ(result.missing_bookmarks.front(), "missing");
    CHECK_FALSE(static_cast<bool>(result));
    CHECK_EQ(collect_document_text(doc), "Customer: Acme Corp\nInvoice: INV-2026-0001\n");

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(xml_text.find("Acme Corp"), std::string::npos);
    CHECK_NE(xml_text.find("INV-2026-0001"), std::string::npos);
    CHECK_EQ(xml_text.find("old customer"), std::string::npos);
    CHECK_EQ(xml_text.find("old invoice"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened),
             "Customer: Acme Corp\nInvoice: INV-2026-0001\n");

    fs::remove(target);
}

TEST_CASE("fill_bookmarks rejects duplicate or empty binding names") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_fill_validation.docx";
    fs::remove(target);

    featherdoc::Document unopened(target);
    const auto unopened_result = unopened.fill_bookmarks({{"customer", "Acme Corp"}});
    CHECK_EQ(unopened_result.requested, 1);
    CHECK_EQ(unopened.last_error().code, featherdoc::document_errc::document_not_open);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    const auto duplicate_result = doc.fill_bookmarks(
        {
            {"customer", "Acme Corp"},
            {"customer", "Other"},
        });
    CHECK_EQ(duplicate_result.requested, 2);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("duplicate"), std::string::npos);

    const auto empty_result = doc.fill_bookmarks({{"", "blank"}});
    CHECK_EQ(empty_result.requested, 1);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("must not be empty"), std::string::npos);

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_table swaps a standalone bookmark paragraph for a table") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_table.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="items"/>
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

    const std::vector<std::vector<std::string>> rows{
        {"Name", "Qty"},
        {"Apple", "2"},
        {"Pear", "5"},
    };
    CHECK_EQ(doc.replace_bookmark_with_table("items", rows), 1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_document_text(doc), "before\nafter\n");
    CHECK_EQ(collect_table_text(doc), "Name\nQty\nApple\n2\nPear\n5\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:tbl>"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"items\""), std::string::npos);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    std::ostringstream child_order;
    const auto body = xml_document.child("w:document").child("w:body");
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        child_order << child.name() << '\n';
    }
    CHECK_EQ(child_order.str(), "w:p\nw:tbl\nw:p\n");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nafter\n");
    CHECK_EQ(collect_table_text(reopened), "Name\nQty\nApple\n2\nPear\n5\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_paragraphs expands a standalone bookmark paragraph") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_paragraphs.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="items"/>
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

    CHECK_EQ(doc.replace_bookmark_with_paragraphs("items", {"Apple", "Pear"}), 1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_document_text(doc), "before\nApple\nPear\nafter\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:t>Apple</w:t>"), std::string::npos);
    CHECK_NE(saved_document_xml.find("<w:t>Pear</w:t>"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"items\""), std::string::npos);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    std::ostringstream child_order;
    const auto body = xml_document.child("w:document").child("w:body");
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        child_order << child.name() << '\n';
    }
    CHECK_EQ(child_order.str(), "w:p\nw:p\nw:p\nw:p\n");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nApple\nPear\nafter\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_paragraphs accepts an empty replacement list") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_paragraphs_empty.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="items"/>
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

    CHECK_EQ(doc.replace_bookmark_with_paragraphs("items", std::vector<std::string>{}), 1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_document_text(doc), "before\nafter\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("placeholder"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"items\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nafter\n");

    fs::remove(target);
}

TEST_CASE("remove_bookmark_block deletes a standalone bookmark paragraph") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_remove_block.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="items"/>
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

    CHECK_EQ(doc.remove_bookmark_block("missing"), 0);
    CHECK_FALSE(doc.last_error());

    CHECK_EQ(doc.remove_bookmark_block("items"), 1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_document_text(doc), "before\nafter\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("placeholder"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"items\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nafter\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_table_rows expands a template row inside an existing table") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_table_rows.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Name</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Qty</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:trPr><w:cantSplit/></w:trPr>
        <w:tc>
          <w:tcPr><w:shd w:fill="AAAAAA"/></w:tcPr>
          <w:p>
            <w:pPr><w:jc w:val="center"/></w:pPr>
            <w:bookmarkStart w:id="0" w:name="item_row"/>
            <w:r><w:rPr><w:b/></w:rPr><w:t>template name</w:t></w:r>
            <w:bookmarkEnd w:id="0"/>
          </w:p>
        </w:tc>
        <w:tc>
          <w:tcPr><w:shd w:fill="BBBBBB"/></w:tcPr>
          <w:p>
            <w:pPr><w:jc w:val="right"/></w:pPr>
            <w:r><w:rPr><w:i/></w:rPr><w:t>template qty</w:t></w:r>
          </w:p>
        </w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Total</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>7</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_with_table_rows(
                 "item_row", {{"Apple", "2"}, {"Pear", "5"}}),
             1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_table_text(doc), "Name\nQty\nApple\n2\nPear\n5\nTotal\n7\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("template name"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("template qty"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"item_row\""), std::string::npos);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:fill=\"AAAAAA\""), 2);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:fill=\"BBBBBB\""), 2);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    const auto table = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table != pugi::xml_node{});
    CHECK_EQ(count_named_children(table, "w:tr"), 4);
    CHECK_EQ(count_named_descendants(table, "w:cantSplit"), 2);
    CHECK_EQ(count_named_descendants(table, "w:b"), 2);
    CHECK_EQ(count_named_descendants(table, "w:i"), 2);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "Name\nQty\nApple\n2\nPear\n5\nTotal\n7\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_table_rows accepts an empty replacement list") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_table_rows_empty.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Name</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Qty</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc>
          <w:p>
            <w:bookmarkStart w:id="0" w:name="item_row"/>
            <w:r><w:t>template name</w:t></w:r>
            <w:bookmarkEnd w:id="0"/>
          </w:p>
        </w:tc>
        <w:tc><w:p><w:r><w:t>template qty</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Total</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>7</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_with_table_rows("item_row", {}), 1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_table_text(doc), "Name\nQty\nTotal\n7\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("template name"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("template qty"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"item_row\""), std::string::npos);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    const auto table = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table != pugi::xml_node{});
    CHECK_EQ(count_named_children(table, "w:tr"), 2);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "Name\nQty\nTotal\n7\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_table_rows rejects mismatched row widths") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_table_rows_validation.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:p>
            <w:bookmarkStart w:id="0" w:name="item_row"/>
            <w:r><w:t>template name</w:t></w:r>
            <w:bookmarkEnd w:id="0"/>
          </w:p>
        </w:tc>
        <w:tc><w:p><w:r><w:t>template qty</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_with_table_rows("item_row", {{"Apple"}}), 0);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("cell count"), std::string::npos);
    CHECK_EQ(collect_table_text(doc), "template name\ntemplate qty\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_image swaps a standalone bookmark paragraph for an inline image") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "bookmark_replace_image.docx";
    const fs::path image_path = fs::current_path() / "bookmark_replace_image.png";
    fs::remove(target);
    fs::remove(image_path);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="logo"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.replace_bookmark_with_image("logo", image_path, 20U, 10U), 1);
    CHECK_FALSE(doc.last_error());

    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:drawing"), std::string::npos);
    CHECK_NE(saved_document_xml.find("cx=\"190500\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"95250\""), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"logo\""), std::string::npos);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\n\nafter\n");

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("replace_bookmark_with_floating_image swaps a standalone bookmark paragraph for an anchored image") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "bookmark_replace_floating_image.docx";
    const fs::path image_path = fs::current_path() / "bookmark_replace_floating_image.png";
    fs::remove(target);
    fs::remove(image_path);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="logo"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::floating_image_options options;
    options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    options.horizontal_offset_px = 24;
    options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    options.vertical_offset_px = -8;
    options.behind_text = true;
    options.allow_overlap = false;

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.replace_bookmark_with_floating_image("logo", image_path, 20U, 10U, options),
             1);
    CHECK_FALSE(doc.last_error());

    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<wp:anchor"), std::string::npos);
    CHECK_NE(saved_document_xml.find("relativeFrom=\"page\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("relativeFrom=\"margin\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:posOffset>228600</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:posOffset>-76200</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("behindDoc=\"1\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("allowOverlap=\"0\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:wrapNone"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"logo\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\n\nafter\n");
    const auto drawing_images = reopened.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(drawing_images[0].width_px, 20U);
    CHECK_EQ(drawing_images[0].height_px, 10U);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("block bookmark replacements reject bookmarks that do not occupy their own paragraph") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "bookmark_replace_block_validation.docx";
    const fs::path image_path = fs::current_path() / "bookmark_replace_block_validation.png";
    fs::remove(target);
    fs::remove(image_path);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>prefix </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="block"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
      <w:r><w:t> suffix</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.remove_bookmark_block("block"), 0);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("occupy its own paragraph"), std::string::npos);

    CHECK_EQ(doc.replace_bookmark_with_paragraphs("block", {"A", "B"}), 0);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_EQ(doc.replace_bookmark_with_table_rows("block", {{"A"}}), 0);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_EQ(doc.replace_bookmark_with_table("block", {{"A"}}), 0);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_EQ(doc.replace_bookmark_with_image("block", image_path), 0);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("append_table creates a new editable table in an empty document") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "append_table_create_empty.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 2);
    auto row = table.rows();
    REQUIRE(row.has_next());

    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.paragraphs().add_run("r0c0").has_next());

    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.paragraphs().add_run("r0c1").has_next());

    row.next();
    REQUIRE(row.has_next());

    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.paragraphs().add_run("r1c0").has_next());

    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.paragraphs().add_run("r1c1").has_next());

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
    CHECK_EQ(child_order.str(), "w:p\nw:tbl\n");

    const auto table_node = body.child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK(table_node.child("w:tblPr") != pugi::xml_node{});
    const auto table_grid = table_node.child("w:tblGrid");
    REQUIRE(table_grid != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_grid, "w:gridCol"), 2);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "r0c0\nr0c1\nr1c0\nr1c1\n");

    fs::remove(target);
}

TEST_CASE("table append_row and append_cell extend existing tables before sectPr") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "append_table_row_and_cell.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>outside</w:t></w:r></w:p>
    <w:tbl>
      <w:tr>
        <w:tc>
          <w:p><w:r><w:t>seed</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
    <w:sectPr/>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto table = doc.tables();
    REQUIRE(table.has_next());

    auto appended_row = table.append_row();
    REQUIRE(appended_row.has_next());

    auto first_cell = appended_row.cells();
    REQUIRE(first_cell.has_next());
    CHECK(first_cell.paragraphs().add_run("appended one").has_next());

    auto second_cell = appended_row.append_cell();
    REQUIRE(second_cell.has_next());
    CHECK(second_cell.paragraphs().add_run("appended two").has_next());

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
    CHECK_EQ(child_order.str(), "w:p\nw:tbl\nw:sectPr\n");

    const auto table_node = body.child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK(table_node.child("w:tblPr") != pugi::xml_node{});
    const auto table_grid = table_node.child("w:tblGrid");
    REQUIRE(table_grid != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_grid, "w:gridCol"), 2);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "seed\nappended one\nappended two\n");

    fs::remove(target);
}

TEST_CASE("table cells can set and clear explicit widths") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_widths.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 2);
    auto row = table.rows();
    REQUIRE(row.has_next());

    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK_FALSE(cell.width_twips().has_value());
    CHECK(cell.set_width_twips(2400U));
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 2400U);
    CHECK(cell.paragraphs().add_run("left").has_next());

    cell.next();
    REQUIRE(cell.has_next());
    CHECK_FALSE(cell.width_twips().has_value());
    CHECK(cell.set_width_twips(1800U));
    REQUIRE(cell.width_twips().has_value());
    CHECK_EQ(*cell.width_twips(), 1800U);
    CHECK(cell.clear_width());
    CHECK_FALSE(cell.width_twips().has_value());
    CHECK(cell.paragraphs().add_run("right").has_next());

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});

    const auto row_node = table_node.child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});

    const auto first_cell = row_node.child("w:tc");
    REQUIRE(first_cell != pugi::xml_node{});
    const auto first_width = first_cell.child("w:tcPr").child("w:tcW");
    REQUIRE(first_width != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_width.attribute("w:type").value()}, "dxa");
    CHECK_EQ(std::string_view{first_width.attribute("w:w").value()}, "2400");

    const auto second_cell = first_cell.next_sibling("w:tc");
    REQUIRE(second_cell != pugi::xml_node{});
    CHECK_EQ(second_cell.child("w:tcPr").child("w:tcW"), pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 2400U);

    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    CHECK_FALSE(reopened_cell.width_twips().has_value());

    fs::remove(target);
}

TEST_CASE("table cells can replace text while preserving cell properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_set_text.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto cell = doc.append_table(1, 1).rows().cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_fill_color("D9EAF7"));
    CHECK(cell.set_margin_twips(featherdoc::cell_margin_edge::left, 120U));

    auto paragraph = cell.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("before"));
    auto extra_paragraph = paragraph.insert_paragraph_after("after");
    REQUIRE(extra_paragraph.has_next());
    CHECK_EQ(cell.get_text(), "before\nafter");

    CHECK(cell.set_text("updated"));
    CHECK_EQ(cell.get_text(), "updated");

    REQUIRE(cell.fill_color().has_value());
    CHECK_EQ(*cell.fill_color(), "D9EAF7");
    REQUIRE(cell.margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_EQ(*cell.margin_twips(featherdoc::cell_margin_edge::left), 120U);

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto cell_node = xml_document.child("w:document")
                               .child("w:body")
                               .child("w:tbl")
                               .child("w:tr")
                               .child("w:tc");
    REQUIRE(cell_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(cell_node, "w:p"), 1);
    CHECK_EQ(std::string_view{
                 cell_node.child("w:tcPr").child("w:shd").attribute("w:fill").value()},
             "D9EAF7");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    CHECK_EQ(reopened_cell.get_text(), "updated");
    REQUIRE(reopened_cell.fill_color().has_value());
    CHECK_EQ(*reopened_cell.fill_color(), "D9EAF7");
    REQUIRE(reopened_cell.margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_EQ(*reopened_cell.margin_twips(featherdoc::cell_margin_edge::left), 120U);

    fs::remove(target);
}

TEST_CASE("table rows can remove a middle row and keep the wrapper usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_remove.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(3, 1);
    auto row = table.rows();
    REQUIRE(row.has_next());
    CHECK(row.cells().set_text("row-1"));

    row.next();
    REQUIRE(row.has_next());
    CHECK(row.cells().set_text("row-2"));
    auto removed_row = row;

    row.next();
    REQUIRE(row.has_next());
    CHECK(row.cells().set_text("row-3"));

    CHECK(removed_row.remove());
    CHECK(removed_row.has_next());
    CHECK_EQ(removed_row.cells().get_text(), "row-3");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "row-1\nrow-3\n");

    fs::remove(target);
}

TEST_CASE("table rows can insert a formatted row after the current row") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_insert_after.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 2);
    auto first_row = table.rows();
    REQUIRE(first_row.has_next());
    CHECK(first_row.set_cant_split());

    auto first_cell = first_row.cells();
    REQUIRE(first_cell.has_next());
    CHECK(first_cell.set_width_twips(2400U));
    CHECK(first_cell.set_fill_color("D9EAF7"));
    CHECK(first_cell.set_text("row-1a"));

    auto second_cell = first_cell;
    second_cell.next();
    REQUIRE(second_cell.has_next());
    CHECK(second_cell.set_text("row-1b"));

    auto last_row = first_row;
    last_row.next();
    REQUIRE(last_row.has_next());
    CHECK(last_row.cells().set_text("row-2a"));
    auto last_row_second_cell = last_row.cells();
    last_row_second_cell.next();
    REQUIRE(last_row_second_cell.has_next());
    CHECK(last_row_second_cell.set_text("row-2b"));

    auto inserted_row = table.rows().insert_row_after();
    REQUIRE(inserted_row.has_next());
    CHECK(table.rows().has_next());
    CHECK_EQ(table.rows().cells().get_text(), "row-1a");

    auto inserted_cell = inserted_row.cells();
    REQUIRE(inserted_cell.has_next());
    CHECK_EQ(inserted_cell.get_text(), "");
    CHECK(inserted_cell.set_text("inserted-a"));

    auto inserted_second_cell = inserted_cell;
    inserted_second_cell.next();
    REQUIRE(inserted_second_cell.has_next());
    CHECK_EQ(inserted_second_cell.get_text(), "");
    CHECK(inserted_second_cell.set_text("inserted-b"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened),
             "row-1a\nrow-1b\ninserted-a\ninserted-b\nrow-2a\nrow-2b\n");

    auto reopened_row = reopened.tables().rows();
    REQUIRE(reopened_row.has_next());
    reopened_row.next();
    REQUIRE(reopened_row.has_next());
    CHECK(reopened_row.cant_split());

    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.fill_color().has_value());
    CHECK_EQ(*reopened_cell.fill_color(), "D9EAF7");
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 2400U);

    fs::remove(target);
}

TEST_CASE("table rows can insert a formatted row before the current row") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_insert_before.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(3, 2);
    auto row = table.rows();
    REQUIRE(row.has_next());
    CHECK(row.cells().set_text("row-1a"));
    auto row1_second_cell = row.cells();
    row1_second_cell.next();
    REQUIRE(row1_second_cell.has_next());
    CHECK(row1_second_cell.set_text("row-1b"));

    row.next();
    REQUIRE(row.has_next());
    CHECK(row.set_cant_split());
    auto source_cell = row.cells();
    REQUIRE(source_cell.has_next());
    CHECK(source_cell.set_width_twips(2400U));
    CHECK(source_cell.set_fill_color("FFF2CC"));
    CHECK(source_cell.set_text("source-a"));
    auto source_second_cell = source_cell;
    source_second_cell.next();
    REQUIRE(source_second_cell.has_next());
    CHECK(source_second_cell.set_fill_color("FFF2CC"));
    CHECK(source_second_cell.set_text("source-b"));

    row.next();
    REQUIRE(row.has_next());
    CHECK(row.cells().set_text("row-3a"));
    auto row3_second_cell = row.cells();
    row3_second_cell.next();
    REQUIRE(row3_second_cell.has_next());
    CHECK(row3_second_cell.set_text("row-3b"));

    auto source_row = table.rows();
    source_row.next();
    REQUIRE(source_row.has_next());
    auto inserted_row = source_row.insert_row_before();
    REQUIRE(inserted_row.has_next());
    CHECK(source_row.has_next());
    CHECK_EQ(source_row.cells().get_text(), "");

    auto inserted_cell = inserted_row.cells();
    REQUIRE(inserted_cell.has_next());
    CHECK_EQ(inserted_cell.get_text(), "");
    CHECK(inserted_cell.set_text("inserted-a"));
    auto inserted_second_cell = inserted_cell;
    inserted_second_cell.next();
    REQUIRE(inserted_second_cell.has_next());
    CHECK_EQ(inserted_second_cell.get_text(), "");
    CHECK(inserted_second_cell.set_text("inserted-b"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened),
             "row-1a\nrow-1b\ninserted-a\ninserted-b\nsource-a\nsource-b\nrow-3a\nrow-3b\n");

    auto reopened_row = reopened.tables().rows();
    REQUIRE(reopened_row.has_next());
    reopened_row.next();
    REQUIRE(reopened_row.has_next());
    CHECK(reopened_row.cant_split());

    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.fill_color().has_value());
    CHECK_EQ(*reopened_cell.fill_color(), "FFF2CC");
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 2400U);
    CHECK_EQ(reopened_cell.get_text(), "inserted-a");

    fs::remove(target);
}

TEST_CASE("tables can remove a middle table and keep the wrapper usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_remove.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto first_table = doc.append_table(1, 1);
    REQUIRE(first_table.has_next());
    CHECK(first_table.rows().cells().set_text("table-1"));

    auto second_table = doc.append_table(1, 1);
    REQUIRE(second_table.has_next());
    CHECK(second_table.rows().cells().set_text("table-2"));
    auto removed_table = second_table;

    auto third_table = doc.append_table(1, 1);
    REQUIRE(third_table.has_next());
    CHECK(third_table.rows().cells().set_text("table-3"));

    CHECK(removed_table.remove());
    CHECK(removed_table.has_next());
    CHECK_EQ(removed_table.rows().cells().get_text(), "table-3");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "table-1\ntable-3\n");

    fs::remove(target);
}

TEST_CASE("tables can insert a new table before the selected body table and keep the wrapper usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_insert_before.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body paragraph"));

    auto first_table = doc.append_table(1, 1);
    REQUIRE(first_table.has_next());
    CHECK(first_table.rows().cells().set_text("table-1"));

    auto last_table = doc.append_table(1, 1);
    REQUIRE(last_table.has_next());
    CHECK(last_table.rows().cells().set_text("table-3"));

    auto inserted_table = last_table.insert_table_before(1, 1);
    REQUIRE(inserted_table.has_next());
    CHECK(last_table.has_next());
    CHECK(last_table.rows().cells().set_text("table-2"));
    CHECK_EQ(inserted_table.rows().cells().get_text(), "table-2");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "body paragraph\n");
    CHECK_EQ(collect_table_text(reopened), "table-1\ntable-2\ntable-3\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:tbl"), 3U);

    fs::remove(target);
}

TEST_CASE("tables can insert a new table after the last body table before section properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_insert_after_last.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body paragraph"));

    auto &header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.set_text("header"));

    auto last_table = doc.append_table(1, 1);
    REQUIRE(last_table.has_next());
    CHECK(last_table.rows().cells().set_text("table-1"));

    auto inserted_table = last_table.insert_table_after(1, 1);
    REQUIRE(inserted_table.has_next());
    CHECK(last_table.has_next());
    CHECK(last_table.rows().cells().set_text("table-2"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "body paragraph\n");
    CHECK_EQ(collect_table_text(reopened), "table-1\ntable-2\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:tbl"), 2U);
    CHECK_EQ(std::string_view{body_node.last_child().name()}, "w:sectPr");

    fs::remove(target);
}

TEST_CASE("tables can insert a styled table clone before the selected body table") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_insert_like_before.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body paragraph"));

    auto first_table = doc.append_table(2, 2);
    REQUIRE(first_table.has_next());
    REQUIRE(configure_clone_template_table(first_table, "seed"));

    auto anchor_table = doc.append_table(2, 2);
    REQUIRE(anchor_table.has_next());
    REQUIRE(configure_clone_template_table(anchor_table, "anchor"));

    auto inserted_table = anchor_table.insert_table_like_before();
    REQUIRE(inserted_table.has_next());
    CHECK(anchor_table.has_next());

    const auto inserted_width = inserted_table.width_twips();
    REQUIRE(inserted_width.has_value());
    CHECK_EQ(*inserted_width, 7200U);

    const auto inserted_style = inserted_table.style_id();
    REQUIRE(inserted_style.has_value());
    CHECK_EQ(*inserted_style, "TableGrid");

    const auto inserted_layout = inserted_table.layout_mode();
    REQUIRE(inserted_layout.has_value());
    CHECK_EQ(*inserted_layout, featherdoc::table_layout_mode::fixed);

    auto inserted_row = inserted_table.rows();
    REQUIRE(inserted_row.has_next());
    CHECK(inserted_row.repeats_header());
    CHECK_EQ(inserted_row.height_twips().value_or(0U), 360U);

    auto inserted_cell = inserted_row.cells();
    REQUIRE(inserted_cell.has_next());
    CHECK_EQ(inserted_cell.fill_color().value_or(""), "D9EAF7");
    CHECK_EQ(inserted_cell.get_text(), "");

    inserted_cell.next();
    REQUIRE(inserted_cell.has_next());
    CHECK_EQ(inserted_cell.fill_color().value_or(""), "D9EAF7");
    CHECK_EQ(inserted_cell.get_text(), "");

    inserted_row.next();
    REQUIRE(inserted_row.has_next());
    CHECK_EQ(inserted_row.height_twips().value_or(0U), 420U);
    inserted_cell = inserted_row.cells();
    REQUIRE(inserted_cell.has_next());
    CHECK_EQ(inserted_cell.fill_color().value_or(""), "FCE4D6");
    CHECK_EQ(inserted_cell.margin_twips(featherdoc::cell_margin_edge::left).value_or(0U),
             160U);
    CHECK_EQ(inserted_cell.get_text(), "");

    inserted_cell.next();
    REQUIRE(inserted_cell.has_next());
    CHECK_EQ(inserted_cell.fill_color().value_or(""), "FFF2CC");
    CHECK_EQ(inserted_cell.margin_twips(featherdoc::cell_margin_edge::right).value_or(0U),
             160U);
    CHECK_EQ(inserted_cell.get_text(), "");

    inserted_row = inserted_table.rows();
    REQUIRE(inserted_row.has_next());
    CHECK(set_two_cell_row_text(inserted_row, "clone-h1", "clone-h2"));
    inserted_row.next();
    REQUIRE(inserted_row.has_next());
    CHECK(set_two_cell_row_text(inserted_row, "clone-b1", "clone-b2"));

    anchor_table.next();
    REQUIRE(anchor_table.has_next());
    CHECK_EQ(anchor_table.rows().cells().get_text(), "anchor-h1");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "body paragraph\n");
    CHECK_EQ(collect_table_text(reopened),
             "seed-h1\nseed-h2\nseed-b1\nseed-b2\n"
             "clone-h1\nclone-h2\nclone-b1\nclone-b2\n"
             "anchor-h1\nanchor-h2\nanchor-b1\nanchor-b2\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:tbl"), 3U);

    fs::remove(target);
}

TEST_CASE("tables can insert a styled table clone after the last body table before section properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_insert_like_after_last.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body paragraph"));

    auto &header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.set_text("header"));

    auto anchor_table = doc.append_table(2, 2);
    REQUIRE(anchor_table.has_next());
    REQUIRE(configure_clone_template_table(anchor_table, "anchor"));

    auto inserted_table = anchor_table.insert_table_like_after();
    REQUIRE(inserted_table.has_next());
    CHECK(anchor_table.has_next());

    auto inserted_row = inserted_table.rows();
    REQUIRE(inserted_row.has_next());
    CHECK(inserted_row.repeats_header());
    CHECK_EQ(inserted_row.cells().fill_color().value_or(""), "D9EAF7");
    CHECK_EQ(inserted_row.cells().get_text(), "");

    inserted_row = inserted_table.rows();
    REQUIRE(inserted_row.has_next());
    CHECK(set_two_cell_row_text(inserted_row, "clone-h1", "clone-h2"));
    inserted_row.next();
    REQUIRE(inserted_row.has_next());
    CHECK(set_two_cell_row_text(inserted_row, "clone-b1", "clone-b2"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "body paragraph\n");
    CHECK_EQ(collect_table_text(reopened),
             "anchor-h1\nanchor-h2\nanchor-b1\nanchor-b2\n"
             "clone-h1\nclone-h2\nclone-b1\nclone-b2\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:tbl"), 2U);
    CHECK_EQ(std::string_view{body_node.last_child().name()}, "w:sectPr");

    fs::remove(target);
}

TEST_CASE("header template part tables can insert a styled table clone after the selected header table") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_template_table_insert_like_after.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_paragraph = doc.paragraphs();
    REQUIRE(body_paragraph.has_next());
    CHECK(body_paragraph.set_text("body paragraph"));

    auto &header_paragraph = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header_paragraph.has_next());
    CHECK(header_paragraph.set_text("Header intro"));

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));

    auto anchor_table = header_template.append_table(2, 2);
    REQUIRE(anchor_table.has_next());
    REQUIRE(configure_clone_template_table(anchor_table, "anchor"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    auto selected_table = header_template.tables();
    REQUIRE(selected_table.has_next());

    auto inserted_table = selected_table.insert_table_like_after();
    REQUIRE(inserted_table.has_next());
    CHECK(selected_table.has_next());

    auto inserted_row = inserted_table.rows();
    REQUIRE(inserted_row.has_next());
    CHECK(inserted_row.repeats_header());
    CHECK_EQ(inserted_row.cells().fill_color().value_or(""), "D9EAF7");
    CHECK_EQ(inserted_row.cells().get_text(), "");

    inserted_row = inserted_table.rows();
    REQUIRE(inserted_row.has_next());
    CHECK(set_two_cell_row_text(inserted_row, "clone-h1", "clone-h2"));
    inserted_row.next();
    REQUIRE(inserted_row.has_next());
    CHECK(set_two_cell_row_text(inserted_row, "clone-b1", "clone-b2"));

    CHECK_FALSE(reopened.save());

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(collect_template_part_text(header_template), "Header intro\n");
    CHECK_EQ(collect_template_part_table_text(header_template),
             "anchor-h1\nanchor-h2\nanchor-b1\nanchor-b2\n"
             "clone-h1\nclone-h2\nclone-b1\nclone-b2\n");

    const auto header_xml = read_test_docx_entry(target, "word/header1.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(header_xml.c_str()));
    const auto header_node = xml_document.child("w:hdr");
    REQUIRE(header_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(header_node, "w:tbl"), 2U);

    fs::remove(target);
}

TEST_CASE("table remove rejects removing the last block item in the document body") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_remove_last_block.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tr>
        <w:tc>
          <w:p>
            <w:r><w:t>only table</w:t></w:r>
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

    auto table = doc.tables();
    REQUIRE(table.has_next());
    CHECK_FALSE(table.remove());
    CHECK(table.has_next());

    fs::remove(target);
}

TEST_CASE("tables can remove the only table when body paragraphs remain") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_remove_keep_body_paragraph.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body paragraph"));

    auto table = doc.append_table(1, 1);
    REQUIRE(table.has_next());
    CHECK(table.rows().cells().set_text("table-1"));

    CHECK(table.remove());
    CHECK_FALSE(table.has_next());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "body paragraph\n");
    CHECK_EQ(collect_table_text(reopened), "");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:tbl"), 0U);
    CHECK_EQ(count_named_children(body_node, "w:p"), 1U);

    fs::remove(target);
}

TEST_CASE("tables can remove the last table and keep the wrapper usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_remove_last_table.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body paragraph"));

    auto first_table = doc.append_table(1, 1);
    REQUIRE(first_table.has_next());
    CHECK(first_table.rows().cells().set_text("table-1"));

    auto last_table = doc.append_table(1, 1);
    REQUIRE(last_table.has_next());
    CHECK(last_table.rows().cells().set_text("table-2"));

    CHECK(last_table.remove());
    CHECK(last_table.has_next());
    CHECK_EQ(last_table.rows().cells().get_text(), "table-1");
    CHECK(last_table.rows().cells().set_text("table-1-updated"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "body paragraph\n");
    CHECK_EQ(collect_table_text(reopened), "table-1-updated\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:tbl"), 1U);

    fs::remove(target);
}

TEST_CASE("header template part tables can insert a new table after the selected header table") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_template_table_insert_after.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_paragraph = doc.paragraphs();
    REQUIRE(body_paragraph.has_next());
    CHECK(body_paragraph.set_text("body paragraph"));

    auto &header_paragraph = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header_paragraph.has_next());
    CHECK(header_paragraph.set_text("Header intro"));

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));

    auto first_table = header_template.append_table(1, 1);
    REQUIRE(first_table.has_next());
    CHECK(first_table.rows().cells().set_text("header-1"));

    auto last_table = header_template.append_table(1, 1);
    REQUIRE(last_table.has_next());
    CHECK(last_table.rows().cells().set_text("header-3"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    auto selected_table = header_template.tables();
    REQUIRE(selected_table.has_next());

    auto inserted_table = selected_table.insert_table_after(1, 1);
    REQUIRE(inserted_table.has_next());
    CHECK(selected_table.has_next());
    CHECK(selected_table.rows().cells().set_text("header-2"));

    CHECK_FALSE(reopened.save());

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(collect_template_part_text(header_template), "Header intro\n");
    CHECK_EQ(collect_template_part_table_text(header_template),
             "header-1\nheader-2\nheader-3\n");

    const auto header_xml = read_test_docx_entry(target, "word/header1.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(header_xml.c_str()));
    const auto header_node = xml_document.child("w:hdr");
    REQUIRE(header_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(header_node, "w:tbl"), 3U);

    fs::remove(target);
}

TEST_CASE("table row remove rejects removing the last table row") {
    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());

    auto row = doc.append_table(1, 1).rows();
    REQUIRE(row.has_next());
    CHECK_FALSE(row.remove());
    CHECK(row.has_next());
}

TEST_CASE("table row insert after rejects vertical-merge rows") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_insert_after_vertical_merge.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto row = doc.append_table(2, 1).rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("merged"));
    CHECK(cell.merge_down(1U));

    auto inserted = row.insert_row_after();
    CHECK_FALSE(inserted.has_next());
    CHECK(row.has_next());
    CHECK_EQ(row.cells().get_text(), "merged");

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_node, "w:tr"), 2);

    fs::remove(target);
}

TEST_CASE("table row insert before rejects vertical-merge rows") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_insert_before_vertical_merge.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto row = doc.append_table(2, 1).rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("merged"));
    CHECK(cell.merge_down(1U));

    auto inserted = row.insert_row_before();
    CHECK_FALSE(inserted.has_next());
    CHECK(row.has_next());
    CHECK_EQ(row.cells().get_text(), "merged");

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_node, "w:tr"), 2);

    fs::remove(target);
}

TEST_CASE("table row remove promotes the next vertical-merge continuation row") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_remove_vertical_merge.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(3, 1);
    auto first_row = table.rows();
    REQUIRE(first_row.has_next());

    auto merged_cell = first_row.cells();
    REQUIRE(merged_cell.has_next());
    CHECK(merged_cell.set_text("merged"));
    CHECK(merged_cell.merge_down(2U));

    CHECK(first_row.remove());
    CHECK(first_row.has_next());
    CHECK_EQ(first_row.cells().get_text(), "merged");

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_node, "w:tr"), 2);

    const auto promoted_cell = table_node.child("w:tr").child("w:tc");
    REQUIRE(promoted_cell != pugi::xml_node{});
    CHECK_EQ(std::string_view{
                 promoted_cell.child("w:tcPr").child("w:vMerge").attribute("w:val").value()},
             "restart");
    CHECK_EQ(std::string_view{
                 promoted_cell.child("w:p").child("w:r").child("w:t").text().get()},
             "merged");

    const auto trailing_cell = table_node.child("w:tr").next_sibling("w:tr").child("w:tc");
    REQUIRE(trailing_cell != pugi::xml_node{});
    CHECK_EQ(std::string_view{
                 trailing_cell.child("w:tcPr").child("w:vMerge").attribute("w:val").value()},
             "continue");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "merged\n\n");

    fs::remove(target);
}

TEST_CASE("table cells can merge right across following cells") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_merge_right.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 3);
    auto row = table.rows();
    REQUIRE(row.has_next());

    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.paragraphs().add_run("A").has_next());

    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.paragraphs().add_run("B").has_next());

    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.paragraphs().add_run("C").has_next());

    auto merged = row.cells();
    REQUIRE(merged.has_next());
    CHECK_EQ(merged.column_span(), 1U);
    CHECK(merged.merge_right(1U));
    CHECK_EQ(merged.column_span(), 2U);
    CHECK_FALSE(merged.merge_right(5U));

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto table_grid = table_node.child("w:tblGrid");
    REQUIRE(table_grid != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_grid, "w:gridCol"), 3);

    const auto row_node = table_node.child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(row_node, "w:tc"), 2);

    const auto first_cell = row_node.child("w:tc");
    REQUIRE(first_cell != pugi::xml_node{});
    const auto grid_span = first_cell.child("w:tcPr").child("w:gridSpan");
    REQUIRE(grid_span != pugi::xml_node{});
    CHECK_EQ(std::string_view{grid_span.attribute("w:val").value()}, "2");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "A\nC\n");

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    CHECK_EQ(reopened_cell.column_span(), 2U);

    reopened_cell.next();
    REQUIRE(reopened_cell.has_next());
    CHECK_EQ(reopened_cell.column_span(), 1U);
    reopened_cell.next();
    CHECK_FALSE(reopened_cell.has_next());

    fs::remove(target);
}

TEST_CASE("table cells can merge down across following rows") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_merge_down.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(3, 2);
    auto row = table.rows();
    REQUIRE(row.has_next());

    auto first_row_cell = row.cells();
    REQUIRE(first_row_cell.has_next());
    CHECK(first_row_cell.paragraphs().add_run("A").has_next());
    first_row_cell.next();
    REQUIRE(first_row_cell.has_next());
    CHECK(first_row_cell.paragraphs().add_run("B").has_next());

    row.next();
    REQUIRE(row.has_next());
    auto second_row_cell = row.cells();
    REQUIRE(second_row_cell.has_next());
    CHECK(second_row_cell.paragraphs().add_run("C").has_next());
    second_row_cell.next();
    REQUIRE(second_row_cell.has_next());
    CHECK(second_row_cell.paragraphs().add_run("D").has_next());

    row.next();
    REQUIRE(row.has_next());
    auto third_row_cell = row.cells();
    REQUIRE(third_row_cell.has_next());
    CHECK(third_row_cell.paragraphs().add_run("E").has_next());
    third_row_cell.next();
    REQUIRE(third_row_cell.has_next());
    CHECK(third_row_cell.paragraphs().add_run("F").has_next());

    auto merged_row = table.rows();
    REQUIRE(merged_row.has_next());
    auto merged_cell = merged_row.cells();
    REQUIRE(merged_cell.has_next());
    CHECK(merged_cell.merge_down(2U));
    CHECK_FALSE(merged_cell.merge_down(1U));

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(table_node, "w:tr"), 3);

    auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});
    auto third_row = second_row.next_sibling("w:tr");
    REQUIRE(third_row != pugi::xml_node{});

    const auto first_cell = first_row.child("w:tc");
    const auto second_cell = second_row.child("w:tc");
    const auto third_cell = third_row.child("w:tc");
    REQUIRE(first_cell != pugi::xml_node{});
    REQUIRE(second_cell != pugi::xml_node{});
    REQUIRE(third_cell != pugi::xml_node{});

    CHECK_EQ(std::string_view{
                 first_cell.child("w:tcPr").child("w:vMerge").attribute("w:val").value()},
             "restart");
    CHECK_EQ(std::string_view{
                 second_cell.child("w:tcPr").child("w:vMerge").attribute("w:val").value()},
             "continue");
    CHECK_EQ(std::string_view{
                 third_cell.child("w:tcPr").child("w:vMerge").attribute("w:val").value()},
             "continue");
    CHECK_EQ(count_named_descendants(second_cell, "w:t"), 0U);
    CHECK_EQ(count_named_descendants(third_cell, "w:t"), 0U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "A\nB\n\nD\n\nF\n");

    fs::remove(target);
}

TEST_CASE("tables can set widths style ids and borders") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_width_style_borders.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 2);
    CHECK_FALSE(table.width_twips().has_value());
    CHECK_FALSE(table.style_id().has_value());

    CHECK(table.set_width_twips(7200U));
    REQUIRE(table.width_twips().has_value());
    CHECK_EQ(*table.width_twips(), 7200U);

    CHECK(table.set_style_id("TableGrid"));
    REQUIRE(table.style_id().has_value());
    CHECK_EQ(*table.style_id(), "TableGrid");

    CHECK(table.set_border(featherdoc::table_border_edge::top,
                           {featherdoc::border_style::single, 12U, "FF0000", 0U}));
    CHECK(table.set_border(featherdoc::table_border_edge::inside_vertical,
                           {featherdoc::border_style::dashed, 8U, "808080", 0U}));

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_border(featherdoc::cell_border_edge::bottom,
                          {featherdoc::border_style::thick, 16U, "00AA00", 0U}));
    CHECK(cell.paragraphs().add_run("left").has_next());

    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_border(featherdoc::cell_border_edge::right,
                          {featherdoc::border_style::dotted, 6U, "0000FF", 1U}));
    CHECK(cell.paragraphs().add_run("right").has_next());

    CHECK_FALSE(doc.save());
    CHECK(test_docx_entry_exists(target, "word/styles.xml"));
    CHECK(collect_empty_zip64_extra_locations(target).empty());
    CHECK(collect_zero_version_needed_locations(target).empty());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});

    const auto table_width = table_node.child("w:tblPr").child("w:tblW");
    REQUIRE(table_width != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_width.attribute("w:type").value()}, "dxa");
    CHECK_EQ(std::string_view{table_width.attribute("w:w").value()}, "7200");

    const auto table_style = table_node.child("w:tblPr").child("w:tblStyle");
    REQUIRE(table_style != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_style.attribute("w:val").value()}, "TableGrid");

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find(
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"styles.xml\""), std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("PartName=\"/word/styles.xml\""), std::string::npos);
    CHECK_NE(saved_content_types.find(
                 "application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"),
             std::string::npos);

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("w:styleId=\"TableGrid\""), std::string::npos);

    const auto table_borders = table_node.child("w:tblPr").child("w:tblBorders");
    REQUIRE(table_borders != pugi::xml_node{});
    const auto top_border = table_borders.child("w:top");
    REQUIRE(top_border != pugi::xml_node{});
    CHECK_EQ(std::string_view{top_border.attribute("w:val").value()}, "single");
    CHECK_EQ(std::string_view{top_border.attribute("w:sz").value()}, "12");
    CHECK_EQ(std::string_view{top_border.attribute("w:color").value()}, "FF0000");

    const auto inside_vertical = table_borders.child("w:insideV");
    REQUIRE(inside_vertical != pugi::xml_node{});
    CHECK_EQ(std::string_view{inside_vertical.attribute("w:val").value()}, "dashed");
    CHECK_EQ(std::string_view{inside_vertical.attribute("w:sz").value()}, "8");
    CHECK_EQ(std::string_view{inside_vertical.attribute("w:color").value()}, "808080");

    const auto first_cell = table_node.child("w:tr").child("w:tc");
    REQUIRE(first_cell != pugi::xml_node{});
    const auto first_cell_bottom = first_cell.child("w:tcPr").child("w:tcBorders").child("w:bottom");
    REQUIRE(first_cell_bottom != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_cell_bottom.attribute("w:val").value()}, "thick");
    CHECK_EQ(std::string_view{first_cell_bottom.attribute("w:sz").value()}, "16");
    CHECK_EQ(std::string_view{first_cell_bottom.attribute("w:color").value()}, "00AA00");

    const auto second_cell = first_cell.next_sibling("w:tc");
    REQUIRE(second_cell != pugi::xml_node{});
    const auto second_cell_right = second_cell.child("w:tcPr").child("w:tcBorders").child("w:right");
    REQUIRE(second_cell_right != pugi::xml_node{});
    CHECK_EQ(std::string_view{second_cell_right.attribute("w:val").value()}, "dotted");
    CHECK_EQ(std::string_view{second_cell_right.attribute("w:sz").value()}, "6");
    CHECK_EQ(std::string_view{second_cell_right.attribute("w:space").value()}, "1");
    CHECK_EQ(std::string_view{second_cell_right.attribute("w:color").value()}, "0000FF");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.width_twips().has_value());
    CHECK_EQ(*reopened_table.width_twips(), 7200U);
    REQUIRE(reopened_table.style_id().has_value());
    CHECK_EQ(*reopened_table.style_id(), "TableGrid");

    fs::remove(target);
}

TEST_CASE("table widths style ids and borders can be cleared") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_width_style_clear.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblPr>
        <w:tblStyle w:val="TableGrid"/>
        <w:tblW w:w="7200" w:type="dxa"/>
        <w:tblBorders>
          <w:top w:val="single" w:sz="8" w:space="0" w:color="000000"/>
        </w:tblBorders>
      </w:tblPr>
      <w:tblGrid>
        <w:gridCol w:w="0"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="0" w:type="auto"/>
            <w:tcBorders>
              <w:bottom w:val="single" w:sz="8" w:space="0" w:color="000000"/>
            </w:tcBorders>
          </w:tcPr>
          <w:p><w:r><w:t>seed</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto table = doc.tables();
    REQUIRE(table.has_next());
    REQUIRE(table.width_twips().has_value());
    CHECK_EQ(*table.width_twips(), 7200U);
    REQUIRE(table.style_id().has_value());
    CHECK_EQ(*table.style_id(), "TableGrid");

    CHECK(table.clear_width());
    CHECK(table.clear_style_id());
    CHECK(table.clear_border(featherdoc::table_border_edge::top));

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.clear_border(featherdoc::cell_border_edge::bottom));

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto table_properties = table_node.child("w:tblPr");
    REQUIRE(table_properties != pugi::xml_node{});
    CHECK_EQ(table_properties.child("w:tblW"), pugi::xml_node{});
    CHECK_EQ(table_properties.child("w:tblStyle"), pugi::xml_node{});
    CHECK_EQ(table_properties.child("w:tblBorders"), pugi::xml_node{});

    const auto cell_properties = table_node.child("w:tr").child("w:tc").child("w:tcPr");
    REQUIRE(cell_properties != pugi::xml_node{});
    CHECK_EQ(cell_properties.child("w:tcBorders"), pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    CHECK_FALSE(reopened_table.width_twips().has_value());
    CHECK_FALSE(reopened_table.style_id().has_value());

    fs::remove(target);
}

TEST_CASE("table-level properties survive append_row") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_append_row_property_regression.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 2);
    CHECK(table.set_width_twips(7200U));
    CHECK(table.set_style_id("TableGrid"));
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));
    CHECK(table.set_alignment(featherdoc::table_alignment::center));
    CHECK(table.set_indent_twips(360U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::top, 120U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 240U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::bottom, 360U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 480U));

    auto appended_row = table.append_row(2);
    REQUIRE(appended_row.has_next());
    auto appended_cell = appended_row.cells();
    REQUIRE(appended_cell.has_next());
    CHECK(appended_cell.paragraphs().add_run("left").has_next());
    appended_cell.next();
    REQUIRE(appended_cell.has_next());
    CHECK(appended_cell.paragraphs().add_run("right").has_next());

    REQUIRE(table.width_twips().has_value());
    CHECK_EQ(*table.width_twips(), 7200U);
    REQUIRE(table.style_id().has_value());
    CHECK_EQ(*table.style_id(), "TableGrid");
    REQUIRE(table.layout_mode().has_value());
    CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
    REQUIRE(table.alignment().has_value());
    CHECK_EQ(*table.alignment(), featherdoc::table_alignment::center);
    REQUIRE(table.indent_twips().has_value());
    CHECK_EQ(*table.indent_twips(), 360U);
    REQUIRE(table.cell_margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::top), 120U);
    REQUIRE(table.cell_margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::left), 240U);
    REQUIRE(table.cell_margin_twips(featherdoc::cell_margin_edge::bottom).has_value());
    CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::bottom), 360U);
    REQUIRE(table.cell_margin_twips(featherdoc::cell_margin_edge::right).has_value());
    CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::right), 480U);

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto table_properties = table_node.child("w:tblPr");
    REQUIRE(table_properties != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_properties.child("w:tblW").attribute("w:w").value()}, "7200");
    CHECK_EQ(std::string_view{table_properties.child("w:tblW").attribute("w:type").value()},
             "dxa");
    CHECK_EQ(std::string_view{table_properties.child("w:tblStyle").attribute("w:val").value()},
             "TableGrid");
    CHECK_EQ(std::string_view{table_properties.child("w:tblLayout").attribute("w:type").value()},
             "fixed");
    CHECK_EQ(std::string_view{table_properties.child("w:jc").attribute("w:val").value()},
             "center");
    CHECK_EQ(std::string_view{table_properties.child("w:tblInd").attribute("w:w").value()},
             "360");
    CHECK_EQ(std::string_view{table_properties.child("w:tblCellMar")
                                  .child("w:top")
                                  .attribute("w:w")
                                  .value()},
             "120");
    CHECK_EQ(std::string_view{table_properties.child("w:tblCellMar")
                                  .child("w:left")
                                  .attribute("w:w")
                                  .value()},
             "240");
    CHECK_EQ(std::string_view{table_properties.child("w:tblCellMar")
                                  .child("w:bottom")
                                  .attribute("w:w")
                                  .value()},
             "360");
    CHECK_EQ(std::string_view{table_properties.child("w:tblCellMar")
                                  .child("w:right")
                                  .attribute("w:w")
                                  .value()},
             "480");
    CHECK_EQ(count_named_children(table_node, "w:tr"), 2U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.width_twips().has_value());
    CHECK_EQ(*reopened_table.width_twips(), 7200U);
    REQUIRE(reopened_table.style_id().has_value());
    CHECK_EQ(*reopened_table.style_id(), "TableGrid");
    REQUIRE(reopened_table.layout_mode().has_value());
    CHECK_EQ(*reopened_table.layout_mode(), featherdoc::table_layout_mode::fixed);
    REQUIRE(reopened_table.alignment().has_value());
    CHECK_EQ(*reopened_table.alignment(), featherdoc::table_alignment::center);
    REQUIRE(reopened_table.indent_twips().has_value());
    CHECK_EQ(*reopened_table.indent_twips(), 360U);
    REQUIRE(reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_EQ(*reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::top), 120U);
    REQUIRE(reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_EQ(*reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::left), 240U);
    REQUIRE(reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::bottom).has_value());
    CHECK_EQ(*reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::bottom), 360U);
    REQUIRE(reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::right).has_value());
    CHECK_EQ(*reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::right), 480U);

    fs::remove(target);
}

TEST_CASE("table layout mode can be set and cleared") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_layout_mode.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    CHECK_FALSE(table.layout_mode().has_value());
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));
    REQUIRE(table.layout_mode().has_value());
    CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    auto layout_node = table_node.child("w:tblPr").child("w:tblLayout");
    REQUIRE(layout_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{layout_node.attribute("w:type").value()}, "fixed");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.layout_mode().has_value());
    CHECK_EQ(*reopened_table.layout_mode(), featherdoc::table_layout_mode::fixed);
    CHECK(reopened_table.clear_layout_mode());
    CHECK_FALSE(reopened_table.layout_mode().has_value());
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(table_node.child("w:tblPr").child("w:tblLayout"), pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("tables can set and clear alignment and indent") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_alignment_indent.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    CHECK_FALSE(table.alignment().has_value());
    CHECK_FALSE(table.indent_twips().has_value());
    CHECK(table.set_alignment(featherdoc::table_alignment::center));
    CHECK(table.set_indent_twips(360U));
    REQUIRE(table.alignment().has_value());
    CHECK_EQ(*table.alignment(), featherdoc::table_alignment::center);
    REQUIRE(table.indent_twips().has_value());
    CHECK_EQ(*table.indent_twips(), 360U);
    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    auto alignment_node = table_node.child("w:tblPr").child("w:jc");
    REQUIRE(alignment_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{alignment_node.attribute("w:val").value()}, "center");
    auto indent_node = table_node.child("w:tblPr").child("w:tblInd");
    REQUIRE(indent_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{indent_node.attribute("w:w").value()}, "360");
    CHECK_EQ(std::string_view{indent_node.attribute("w:type").value()}, "dxa");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.alignment().has_value());
    CHECK_EQ(*reopened_table.alignment(), featherdoc::table_alignment::center);
    REQUIRE(reopened_table.indent_twips().has_value());
    CHECK_EQ(*reopened_table.indent_twips(), 360U);
    CHECK(reopened_table.clear_alignment());
    CHECK(reopened_table.clear_indent());
    CHECK_FALSE(reopened_table.alignment().has_value());
    CHECK_FALSE(reopened_table.indent_twips().has_value());
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    CHECK_EQ(table_node.child("w:tblPr").child("w:jc"), pugi::xml_node{});
    CHECK_EQ(table_node.child("w:tblPr").child("w:tblInd"), pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("tables can set default cell margins") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_default_cell_margins.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    CHECK_FALSE(table.cell_margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::top, 120U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 240U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::bottom, 360U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 480U));
    REQUIRE(table.cell_margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::top), 120U);
    REQUIRE(table.cell_margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::left), 240U);
    REQUIRE(table.cell_margin_twips(featherdoc::cell_margin_edge::bottom).has_value());
    CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::bottom), 360U);
    REQUIRE(table.cell_margin_twips(featherdoc::cell_margin_edge::right).has_value());
    CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::right), 480U);
    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto margins = table_node.child("w:tblPr").child("w:tblCellMar");
    REQUIRE(margins != pugi::xml_node{});
    CHECK_EQ(std::string_view{margins.child("w:top").attribute("w:w").value()}, "120");
    CHECK_EQ(std::string_view{margins.child("w:left").attribute("w:w").value()}, "240");
    CHECK_EQ(std::string_view{margins.child("w:bottom").attribute("w:w").value()}, "360");
    CHECK_EQ(std::string_view{margins.child("w:right").attribute("w:w").value()}, "480");
    CHECK_EQ(std::string_view{margins.child("w:right").attribute("w:type").value()}, "dxa");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    REQUIRE(reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_EQ(*reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::top), 120U);
    REQUIRE(reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_EQ(*reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::left), 240U);
    REQUIRE(reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::bottom).has_value());
    CHECK_EQ(*reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::bottom), 360U);
    REQUIRE(reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::right).has_value());
    CHECK_EQ(*reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::right), 480U);

    fs::remove(target);
}

TEST_CASE("tables can clear default cell margins") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_default_cell_margins_clear.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblPr>
        <w:tblCellMar>
          <w:top w:w="120" w:type="dxa"/>
          <w:left w:w="240" w:type="dxa"/>
          <w:bottom w:w="360" w:type="dxa"/>
          <w:right w:w="480" w:type="dxa"/>
        </w:tblCellMar>
      </w:tblPr>
      <w:tblGrid>
        <w:gridCol w:w="0"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="0" w:type="auto"/>
          </w:tcPr>
          <w:p><w:r><w:t>seed</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto table = doc.tables();
    REQUIRE(table.has_next());
    REQUIRE(table.cell_margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::top), 120U);
    CHECK(table.clear_cell_margin(featherdoc::cell_margin_edge::top));
    CHECK(table.clear_cell_margin(featherdoc::cell_margin_edge::left));
    CHECK(table.clear_cell_margin(featherdoc::cell_margin_edge::bottom));
    CHECK(table.clear_cell_margin(featherdoc::cell_margin_edge::right));
    CHECK_FALSE(table.cell_margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_FALSE(table.cell_margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_FALSE(table.cell_margin_twips(featherdoc::cell_margin_edge::bottom).has_value());
    CHECK_FALSE(table.cell_margin_twips(featherdoc::cell_margin_edge::right).has_value());

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto table_properties =
        xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tblPr");
    REQUIRE(table_properties != pugi::xml_node{});
    CHECK_EQ(table_properties.child("w:tblCellMar"), pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    CHECK_FALSE(reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_FALSE(reopened_table.cell_margin_twips(featherdoc::cell_margin_edge::left).has_value());

    fs::remove(target);
}

TEST_CASE("table rows can set and clear heights") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_height.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    auto row = table.rows();
    REQUIRE(row.has_next());

    CHECK_FALSE(row.height_twips().has_value());
    CHECK_FALSE(row.height_rule().has_value());
    CHECK(row.set_height_twips(360U, featherdoc::row_height_rule::exact));
    REQUIRE(row.height_twips().has_value());
    CHECK_EQ(*row.height_twips(), 360U);
    REQUIRE(row.height_rule().has_value());
    CHECK_EQ(*row.height_rule(), featherdoc::row_height_rule::exact);
    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto row_node = xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});
    auto row_height = row_node.child("w:trPr").child("w:trHeight");
    REQUIRE(row_height != pugi::xml_node{});
    CHECK_EQ(std::string_view{row_height.attribute("w:val").value()}, "360");
    CHECK_EQ(std::string_view{row_height.attribute("w:hRule").value()}, "exact");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    REQUIRE(reopened_row.height_twips().has_value());
    CHECK_EQ(*reopened_row.height_twips(), 360U);
    REQUIRE(reopened_row.height_rule().has_value());
    CHECK_EQ(*reopened_row.height_rule(), featherdoc::row_height_rule::exact);
    CHECK(reopened_row.clear_height());
    CHECK_FALSE(reopened_row.height_twips().has_value());
    CHECK_FALSE(reopened_row.height_rule().has_value());
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    row_node = xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});
    CHECK_EQ(row_node.child("w:trPr"), pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("table rows can set and clear repeating headers") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_repeat_header.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 1);
    auto row = table.rows();
    REQUIRE(row.has_next());

    CHECK_FALSE(row.repeats_header());
    CHECK(row.set_repeats_header());
    CHECK(row.repeats_header());
    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto row_node = xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});
    auto table_header = row_node.child("w:trPr").child("w:tblHeader");
    REQUIRE(table_header != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_header.attribute("w:val").value()}, "1");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    CHECK(reopened_row.repeats_header());
    CHECK(reopened_row.clear_repeats_header());
    CHECK_FALSE(reopened_row.repeats_header());
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    row_node = xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});
    CHECK_EQ(row_node.child("w:trPr"), pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("table rows can set and clear cant-split") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_row_cant_split.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(2, 1);
    auto row = table.rows();
    REQUIRE(row.has_next());

    CHECK_FALSE(row.cant_split());
    CHECK(row.set_cant_split());
    CHECK(row.cant_split());
    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto row_node = xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});
    auto cant_split = row_node.child("w:trPr").child("w:cantSplit");
    REQUIRE(cant_split != pugi::xml_node{});
    CHECK_EQ(std::string_view{cant_split.attribute("w:val").value()}, "1");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    CHECK(reopened_row.cant_split());
    CHECK(reopened_row.clear_cant_split());
    CHECK_FALSE(reopened_row.cant_split());
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    row_node = xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});
    CHECK_EQ(row_node.child("w:trPr"), pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("table cells can set and clear vertical alignment") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_vertical_alignment.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());

    CHECK_FALSE(cell.vertical_alignment().has_value());
    CHECK(cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center));
    REQUIRE(cell.vertical_alignment().has_value());
    CHECK_EQ(*cell.vertical_alignment(), featherdoc::cell_vertical_alignment::center);
    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto cell_node =
        xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cell_node != pugi::xml_node{});
    auto vertical_alignment = cell_node.child("w:tcPr").child("w:vAlign");
    REQUIRE(vertical_alignment != pugi::xml_node{});
    CHECK_EQ(std::string_view{vertical_alignment.attribute("w:val").value()}, "center");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.vertical_alignment().has_value());
    CHECK_EQ(*reopened_cell.vertical_alignment(),
             featherdoc::cell_vertical_alignment::center);
    CHECK(reopened_cell.clear_vertical_alignment());
    CHECK_FALSE(reopened_cell.vertical_alignment().has_value());
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    cell_node =
        xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cell_node != pugi::xml_node{});
    CHECK_EQ(cell_node.child("w:tcPr").child("w:vAlign"), pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("table cells can set and clear text direction") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_text_direction.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());

    CHECK_FALSE(cell.text_direction().has_value());
    CHECK(cell.set_text_direction(
        featherdoc::cell_text_direction::top_to_bottom_right_to_left));
    REQUIRE(cell.text_direction().has_value());
    CHECK_EQ(*cell.text_direction(),
             featherdoc::cell_text_direction::top_to_bottom_right_to_left);
    CHECK_FALSE(doc.save());

    auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    auto cell_node =
        xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cell_node != pugi::xml_node{});
    auto text_direction = cell_node.child("w:tcPr").child("w:textDirection");
    REQUIRE(text_direction != pugi::xml_node{});
    CHECK_EQ(std::string_view{text_direction.attribute("w:val").value()}, "tbRl");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.text_direction().has_value());
    CHECK_EQ(*reopened_cell.text_direction(),
             featherdoc::cell_text_direction::top_to_bottom_right_to_left);
    CHECK(reopened_cell.clear_text_direction());
    CHECK_FALSE(reopened_cell.text_direction().has_value());
    CHECK_FALSE(reopened.save());

    xml_text = read_test_docx_entry(target, test_document_xml_entry);
    xml_document.reset();
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    cell_node =
        xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cell_node != pugi::xml_node{});
    CHECK_EQ(cell_node.child("w:tcPr").child("w:textDirection"), pugi::xml_node{});

    fs::remove(target);
}

TEST_CASE("table cells can set fill colors and margins") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_fill_margins.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());

    CHECK_FALSE(cell.fill_color().has_value());
    CHECK_FALSE(cell.margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK(cell.set_fill_color("A1B2C3"));
    REQUIRE(cell.fill_color().has_value());
    CHECK_EQ(*cell.fill_color(), "A1B2C3");
    CHECK(cell.set_margin_twips(featherdoc::cell_margin_edge::top, 120U));
    CHECK(cell.set_margin_twips(featherdoc::cell_margin_edge::left, 240U));
    CHECK(cell.set_margin_twips(featherdoc::cell_margin_edge::bottom, 360U));
    CHECK(cell.set_margin_twips(featherdoc::cell_margin_edge::right, 480U));
    CHECK(cell.paragraphs().add_run("seed").has_next());

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto cell_node =
        xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cell_node != pugi::xml_node{});

    const auto shading = cell_node.child("w:tcPr").child("w:shd");
    REQUIRE(shading != pugi::xml_node{});
    CHECK_EQ(std::string_view{shading.attribute("w:val").value()}, "clear");
    CHECK_EQ(std::string_view{shading.attribute("w:color").value()}, "auto");
    CHECK_EQ(std::string_view{shading.attribute("w:fill").value()}, "A1B2C3");

    const auto margins = cell_node.child("w:tcPr").child("w:tcMar");
    REQUIRE(margins != pugi::xml_node{});
    CHECK_EQ(std::string_view{margins.child("w:top").attribute("w:w").value()}, "120");
    CHECK_EQ(std::string_view{margins.child("w:left").attribute("w:w").value()}, "240");
    CHECK_EQ(std::string_view{margins.child("w:bottom").attribute("w:w").value()}, "360");
    CHECK_EQ(std::string_view{margins.child("w:right").attribute("w:w").value()}, "480");
    CHECK_EQ(std::string_view{margins.child("w:right").attribute("w:type").value()}, "dxa");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.fill_color().has_value());
    CHECK_EQ(*reopened_cell.fill_color(), "A1B2C3");
    REQUIRE(reopened_cell.margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_EQ(*reopened_cell.margin_twips(featherdoc::cell_margin_edge::top), 120U);
    REQUIRE(reopened_cell.margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_EQ(*reopened_cell.margin_twips(featherdoc::cell_margin_edge::left), 240U);
    REQUIRE(reopened_cell.margin_twips(featherdoc::cell_margin_edge::bottom).has_value());
    CHECK_EQ(*reopened_cell.margin_twips(featherdoc::cell_margin_edge::bottom), 360U);
    REQUIRE(reopened_cell.margin_twips(featherdoc::cell_margin_edge::right).has_value());
    CHECK_EQ(*reopened_cell.margin_twips(featherdoc::cell_margin_edge::right), 480U);

    fs::remove(target);
}

TEST_CASE("table cells can clear fill colors and margins") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_cell_fill_margins_clear.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="0"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="0" w:type="auto"/>
            <w:shd w:val="clear" w:color="auto" w:fill="ABCDEF"/>
            <w:tcMar>
              <w:top w:w="120" w:type="dxa"/>
              <w:left w:w="240" w:type="dxa"/>
              <w:bottom w:w="360" w:type="dxa"/>
              <w:right w:w="480" w:type="dxa"/>
            </w:tcMar>
          </w:tcPr>
          <w:p><w:r><w:t>seed</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto table = doc.tables();
    REQUIRE(table.has_next());
    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());

    REQUIRE(cell.fill_color().has_value());
    CHECK_EQ(*cell.fill_color(), "ABCDEF");
    CHECK(cell.clear_fill_color());
    CHECK_FALSE(cell.fill_color().has_value());
    CHECK(cell.clear_margin(featherdoc::cell_margin_edge::top));
    CHECK(cell.clear_margin(featherdoc::cell_margin_edge::left));
    CHECK(cell.clear_margin(featherdoc::cell_margin_edge::bottom));
    CHECK(cell.clear_margin(featherdoc::cell_margin_edge::right));
    CHECK_FALSE(cell.margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_FALSE(cell.margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_FALSE(cell.margin_twips(featherdoc::cell_margin_edge::bottom).has_value());
    CHECK_FALSE(cell.margin_twips(featherdoc::cell_margin_edge::right).has_value());

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto cell_properties =
        xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc").child("w:tcPr");
    REQUIRE(cell_properties != pugi::xml_node{});
    CHECK_EQ(cell_properties.child("w:shd"), pugi::xml_node{});
    CHECK_EQ(cell_properties.child("w:tcMar"), pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_table = reopened.tables();
    REQUIRE(reopened_table.has_next());
    auto reopened_row = reopened_table.rows();
    REQUIRE(reopened_row.has_next());
    auto reopened_cell = reopened_row.cells();
    REQUIRE(reopened_cell.has_next());
    CHECK_FALSE(reopened_cell.fill_color().has_value());
    CHECK_FALSE(reopened_cell.margin_twips(featherdoc::cell_margin_edge::top).has_value());

    fs::remove(target);
}

TEST_CASE("append_image writes inline media parts and preserves them across reopen save") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "append_image_roundtrip.docx";
    const fs::path image_path = fs::current_path() / "tiny_image.png";
    fs::remove(target);
    fs::remove(image_path);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(image_path));
    CHECK(doc.append_image(image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);
    CHECK_EQ(read_test_docx_entry(target, "word/media/image2.png"), image_data);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_relationships,
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\""),
             2);
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"media/image2.png\""), std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("Extension=\"png\""), std::string::npos);
    CHECK_NE(saved_content_types.find("ContentType=\"image/png\""), std::string::npos);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 2);
    CHECK_NE(saved_document_xml.find("cx=\"9525\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"9525\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cx=\"190500\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"95250\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.paragraphs().add_run("reopened edit").has_next());
    CHECK_FALSE(reopened.save());
    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));

    const auto relationships_after_resave =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(relationships_after_resave.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_NE(relationships_after_resave.find("Target=\"media/image2.png\""),
             std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("append_floating_image writes anchored media parts and preserves them across reopen save") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "append_floating_image_roundtrip.docx";
    const fs::path image_path = fs::current_path() / "floating_tiny_image.png";
    fs::remove(target);
    fs::remove(image_path);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::floating_image_options options;
    options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    options.horizontal_offset_px = 24;
    options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    options.vertical_offset_px = -8;
    options.behind_text = true;
    options.allow_overlap = false;

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_floating_image(image_path, 20U, 10U, options));
    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_relationships,
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\""),
             1);
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""), std::string::npos);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:anchor"), 1U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 0U);
    CHECK_NE(saved_document_xml.find("relativeFrom=\"page\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("relativeFrom=\"margin\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:posOffset>228600</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:posOffset>-76200</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("behindDoc=\"1\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("allowOverlap=\"0\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cx=\"190500\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"95250\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto drawing_images = reopened.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(drawing_images[0].content_type, "image/png");
    CHECK_EQ(drawing_images[0].width_px, 20U);
    CHECK_EQ(drawing_images[0].height_px, 10U);
    CHECK_EQ(reopened.inline_images().size(), 0U);
    CHECK(reopened.paragraphs().add_run("reopened edit").has_next());
    CHECK_FALSE(reopened.save());

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("inline_images lists existing body images and can extract them") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "inline_images_roundtrip.docx";
    const fs::path image_path = fs::current_path() / "inline_images_roundtrip.png";
    const fs::path extracted_path = fs::current_path() / "inline_images_extracted.png";
    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    const std::vector<unsigned char> expected_image_data(image_data.begin(), image_data.end());
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(image_path));
    CHECK(doc.append_image(image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto images = reopened.inline_images();
    REQUIRE_EQ(images.size(), 2U);

    CHECK_EQ(images[0].index, 0U);
    CHECK_EQ(images[0].entry_name, "word/media/image1.png");
    CHECK_EQ(images[0].display_name, image_path.filename().string());
    CHECK_EQ(images[0].content_type, "image/png");
    CHECK_EQ(images[0].width_px, 1U);
    CHECK_EQ(images[0].height_px, 1U);

    CHECK_EQ(images[1].index, 1U);
    CHECK_EQ(images[1].entry_name, "word/media/image2.png");
    CHECK_EQ(images[1].display_name, image_path.filename().string());
    CHECK_EQ(images[1].content_type, "image/png");
    CHECK_EQ(images[1].width_px, 20U);
    CHECK_EQ(images[1].height_px, 10U);

    CHECK(reopened.extract_inline_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_image_data);

    CHECK_FALSE(reopened.extract_inline_image(2U, extracted_path));
    CHECK_EQ(reopened.last_error().code,
             std::make_error_code(std::errc::result_out_of_range));

    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);
}

TEST_CASE("replace_inline_image updates only the selected body image and preserves layout") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };
    constexpr unsigned char tiny_gif_bytes[] = {
        0x47U, 0x49U, 0x46U, 0x38U, 0x39U, 0x61U, 0x01U, 0x00U, 0x01U, 0x00U, 0x80U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0x21U, 0xF9U, 0x04U,
        0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x2CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x01U, 0x00U, 0x00U, 0x02U, 0x02U, 0x44U, 0x01U, 0x00U, 0x3BU,
    };

    const fs::path target = fs::current_path() / "replace_inline_image.docx";
    const fs::path source_image_path = fs::current_path() / "replace_inline_image_source.png";
    const fs::path replacement_image_path =
        fs::current_path() / "replace_inline_image_replacement.gif";
    const fs::path extracted_path =
        fs::current_path() / "replace_inline_image_extracted.gif";
    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);

    const std::string source_image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                        sizeof(tiny_png_bytes));
    const std::string replacement_image_data(reinterpret_cast<const char *>(tiny_gif_bytes),
                                             sizeof(tiny_gif_bytes));
    const std::vector<unsigned char> expected_replacement_data(
        replacement_image_data.begin(), replacement_image_data.end());
    write_binary_file(source_image_path, source_image_data);
    write_binary_file(replacement_image_path, replacement_image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(source_image_path));
    CHECK(doc.append_image(source_image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.replace_inline_image(1U, replacement_image_path));

    auto images = reopened.inline_images();
    REQUIRE_EQ(images.size(), 2U);
    CHECK_EQ(images[0].entry_name, "word/media/image1.png");
    CHECK_EQ(images[0].content_type, "image/png");
    CHECK_EQ(images[1].width_px, 20U);
    CHECK_EQ(images[1].height_px, 10U);
    CHECK_EQ(images[1].content_type, "image/gif");
    CHECK(images[1].entry_name.ends_with(".gif"));
    CHECK_NE(images[1].entry_name, "word/media/image2.png");

    const auto replacement_entry_name = images[1].entry_name;
    const auto replacement_target =
        (fs::path{"media"} / fs::path{replacement_entry_name}.filename()).generic_string();

    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK(test_docx_entry_exists(target, replacement_entry_name.c_str()));
    CHECK_EQ(read_test_docx_entry(target, replacement_entry_name.c_str()),
             replacement_image_data);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"media/image2.png\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"" + replacement_target + "\""),
             std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("Extension=\"gif\""), std::string::npos);
    CHECK_NE(saved_content_types.find("ContentType=\"image/gif\""), std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    images = reopened_again.inline_images();
    REQUIRE_EQ(images.size(), 2U);
    CHECK_EQ(images[1].content_type, "image/gif");
    CHECK_EQ(images[1].width_px, 20U);
    CHECK_EQ(images[1].height_px, 10U);
    CHECK_EQ(images[1].entry_name, replacement_entry_name);
    CHECK(reopened_again.extract_inline_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_replacement_data);

    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);
}

TEST_CASE("drawing_images includes anchored body images and replace_drawing_image preserves them") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };
    constexpr unsigned char tiny_gif_bytes[] = {
        0x47U, 0x49U, 0x46U, 0x38U, 0x39U, 0x61U, 0x01U, 0x00U, 0x01U, 0x00U, 0x80U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0x21U, 0xF9U, 0x04U,
        0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x2CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x01U, 0x00U, 0x00U, 0x02U, 0x02U, 0x44U, 0x01U, 0x00U, 0x3BU,
    };

    const fs::path target = fs::current_path() / "drawing_images_anchor.docx";
    const fs::path source_image_path = fs::current_path() / "drawing_images_anchor_source.png";
    const fs::path replacement_image_path =
        fs::current_path() / "drawing_images_anchor_replacement.gif";
    const fs::path extracted_path =
        fs::current_path() / "drawing_images_anchor_extracted.gif";
    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);

    const std::string source_image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                        sizeof(tiny_png_bytes));
    const std::string replacement_image_data(reinterpret_cast<const char *>(tiny_gif_bytes),
                                             sizeof(tiny_gif_bytes));
    const std::vector<unsigned char> expected_replacement_data(
        replacement_image_data.begin(), replacement_image_data.end());
    write_binary_file(source_image_path, source_image_data);
    write_binary_file(replacement_image_path, replacement_image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(source_image_path));
    CHECK(doc.append_image(source_image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    auto anchored_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    anchored_document_xml = convert_nth_inline_drawing_to_anchor(anchored_document_xml, 1U);
    CHECK_EQ(count_substring_occurrences(anchored_document_xml, "<wp:anchor"), 1U);
    rewrite_test_docx_entry(target, test_document_xml_entry, std::move(anchored_document_xml));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto drawing_images = reopened.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 2U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(drawing_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(drawing_images[0].content_type, "image/png");
    CHECK_EQ(drawing_images[1].content_type, "image/png");
    CHECK_EQ(drawing_images[1].width_px, 20U);
    CHECK_EQ(drawing_images[1].height_px, 10U);

    const auto inline_images = reopened.inline_images();
    REQUIRE_EQ(inline_images.size(), 1U);
    CHECK_EQ(inline_images[0].entry_name, "word/media/image1.png");

    CHECK(reopened.extract_drawing_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path),
             std::vector<unsigned char>(source_image_data.begin(), source_image_data.end()));
    CHECK_FALSE(reopened.extract_inline_image(1U, extracted_path));
    CHECK_EQ(reopened.last_error().code,
             std::make_error_code(std::errc::result_out_of_range));

    CHECK(reopened.replace_drawing_image(1U, replacement_image_path));

    auto updated_images = reopened.drawing_images();
    REQUIRE_EQ(updated_images.size(), 2U);
    CHECK_EQ(updated_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(updated_images[1].width_px, 20U);
    CHECK_EQ(updated_images[1].height_px, 10U);
    CHECK_EQ(updated_images[1].content_type, "image/gif");
    CHECK(updated_images[1].entry_name.ends_with(".gif"));
    const auto replacement_entry_name = updated_images[1].entry_name;
    const auto replacement_target =
        (fs::path{"media"} / fs::path{replacement_entry_name}.filename()).generic_string();

    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK(test_docx_entry_exists(target, replacement_entry_name.c_str()));
    CHECK_EQ(read_test_docx_entry(target, replacement_entry_name.c_str()),
             replacement_image_data);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 1U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:anchor"), 1U);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"media/image2.png\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"" + replacement_target + "\""),
             std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("Extension=\"gif\""), std::string::npos);
    CHECK_NE(saved_content_types.find("ContentType=\"image/gif\""), std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    updated_images = reopened_again.drawing_images();
    REQUIRE_EQ(updated_images.size(), 2U);
    CHECK_EQ(updated_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(updated_images[1].content_type, "image/gif");
    CHECK_EQ(updated_images[1].entry_name, replacement_entry_name);
    CHECK(reopened_again.extract_drawing_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_replacement_data);

    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);
}

TEST_CASE("remove_drawing_image and remove_inline_image prune body media parts") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "drawing_images_remove.docx";
    const fs::path image_path = fs::current_path() / "drawing_images_remove_source.png";
    fs::remove(target);
    fs::remove(image_path);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(image_path));
    CHECK(doc.append_image(image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    auto anchored_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    anchored_document_xml = convert_nth_inline_drawing_to_anchor(anchored_document_xml, 1U);
    CHECK_EQ(count_substring_occurrences(anchored_document_xml, "<wp:anchor"), 1U);
    rewrite_test_docx_entry(target, test_document_xml_entry, std::move(anchored_document_xml));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto drawing_images = reopened.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 2U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(drawing_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);

    CHECK(reopened.remove_drawing_image(1U));
    drawing_images = reopened.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(reopened.inline_images().size(), 1U);

    CHECK(reopened.remove_inline_image(0U));
    CHECK(reopened.drawing_images().empty());
    CHECK(reopened.inline_images().empty());
    CHECK_FALSE(reopened.remove_inline_image(0U));
    CHECK_EQ(reopened.last_error().code,
             std::make_error_code(std::errc::result_out_of_range));

    CHECK_FALSE(reopened.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 0U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:anchor"), 0U);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(saved_relationships.find("relationships/image"), std::string::npos);

    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image2.png"));

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    CHECK(reopened_again.drawing_images().empty());
    CHECK(reopened_again.inline_images().empty());

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("append_image reports unsupported image extensions explicitly") {
    namespace fs = std::filesystem;

    const fs::path image_path = fs::current_path() / "unsupported_image.txt";
    fs::remove(image_path);
    write_binary_file(image_path, "not an image");

    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());
    CHECK_FALSE(doc.append_image(image_path));
    CHECK_EQ(doc.last_error().code, featherdoc::document_errc::image_format_unsupported);
    CHECK_FALSE(doc.last_error().detail.empty());

    fs::remove(image_path);
}

TEST_CASE("set_paragraph_list creates numbering parts and preserves them across reopen save") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_list_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto first_item = doc.paragraphs();
    CHECK(doc.set_paragraph_list(first_item, featherdoc::list_kind::bullet));
    CHECK(first_item.add_run("bullet 0").has_next());

    auto nested_item = first_item.insert_paragraph_after("");
    CHECK(doc.set_paragraph_list(nested_item, featherdoc::list_kind::bullet, 1U));
    CHECK(nested_item.add_run("bullet 1").has_next());

    auto decimal_item = nested_item.insert_paragraph_after("");
    CHECK(doc.set_paragraph_list(decimal_item, featherdoc::list_kind::decimal));
    CHECK(decimal_item.add_run("decimal 0").has_next());

    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/numbering.xml"));

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find(
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"numbering.xml\""), std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("PartName=\"/word/numbering.xml\""), std::string::npos);
    CHECK_NE(saved_content_types.find(
                 "application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"),
             std::string::npos);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:numPr>"), 3);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    const auto body = xml_document.child("w:document").child("w:body");
    auto first_paragraph = body.child("w:p");
    REQUIRE(first_paragraph != pugi::xml_node{});
    auto second_paragraph = first_paragraph.next_sibling("w:p");
    REQUIRE(second_paragraph != pugi::xml_node{});
    auto third_paragraph = second_paragraph.next_sibling("w:p");
    REQUIRE(third_paragraph != pugi::xml_node{});

    const auto first_num_pr = first_paragraph.child("w:pPr").child("w:numPr");
    const auto second_num_pr = second_paragraph.child("w:pPr").child("w:numPr");
    const auto third_num_pr = third_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(first_num_pr != pugi::xml_node{});
    REQUIRE(second_num_pr != pugi::xml_node{});
    REQUIRE(third_num_pr != pugi::xml_node{});

    CHECK_EQ(std::string{first_num_pr.child("w:ilvl").attribute("w:val").value()}, "0");
    CHECK_EQ(std::string{second_num_pr.child("w:ilvl").attribute("w:val").value()}, "1");
    CHECK_EQ(std::string{third_num_pr.child("w:ilvl").attribute("w:val").value()}, "0");
    CHECK_EQ(std::string{first_num_pr.child("w:numId").attribute("w:val").value()},
             std::string{second_num_pr.child("w:numId").attribute("w:val").value()});
    CHECK_NE(std::string{first_num_pr.child("w:numId").attribute("w:val").value()},
             std::string{third_num_pr.child("w:numId").attribute("w:val").value()});

    const auto numbering_xml = read_test_docx_entry(target, "word/numbering.xml");
    CHECK_NE(numbering_xml.find("FeatherDocBulletList"), std::string::npos);
    CHECK_NE(numbering_xml.find("FeatherDocDecimalList"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.paragraphs().add_run("tail").has_next());
    CHECK_FALSE(reopened.save());
    CHECK(test_docx_entry_exists(target, "word/numbering.xml"));

    fs::remove(target);
}

TEST_CASE("clear_paragraph_list removes numbering markup and invalid level is rejected") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_list_clear.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>seed</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto paragraph = doc.paragraphs();
    CHECK_FALSE(doc.set_paragraph_list(paragraph, featherdoc::list_kind::bullet, 9U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK(doc.set_paragraph_list(paragraph, featherdoc::list_kind::bullet));
    CHECK(doc.clear_paragraph_list(paragraph));
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:numPr>"), 0);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:pPr>"), 0);

    fs::remove(target);
}

TEST_CASE("set_paragraph_style and set_run_style create styles parts and round-trip") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_run_style_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(doc.set_paragraph_style(paragraph, "Heading1"));

    auto run = paragraph.add_run("Styled text");
    REQUIRE(run.has_next());
    CHECK(doc.set_run_style(run, "Strong"));

    CHECK_FALSE(doc.save());
    CHECK(test_docx_entry_exists(target, "word/styles.xml"));

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find(
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"styles.xml\""), std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("PartName=\"/word/styles.xml\""), std::string::npos);
    CHECK_NE(saved_content_types.find(
                 "application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"),
             std::string::npos);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:pStyle w:val=\"Heading1\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("<w:rStyle w:val=\"Strong\""), std::string::npos);

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("w:styleId=\"Heading1\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:styleId=\"Strong\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.paragraphs().add_run(" tail").has_next());
    CHECK_FALSE(reopened.save());
    CHECK(test_docx_entry_exists(target, "word/styles.xml"));

    fs::remove(target);
}

TEST_CASE("clear_paragraph_style and clear_run_style remove markup and reject empty ids") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_run_style_clear.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
</Types>
)";
    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:pPr><w:pStyle w:val="Heading1"/></w:pPr>
      <w:r>
        <w:rPr><w:rStyle w:val="Strong"/></w:rPr>
        <w:t>seed</w:t>
      </w:r>
    </w:p>
  </w:body>
</w:document>
)";
    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles"
                Target="styles.xml"/>
</Relationships>
)";
    const std::string styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="Heading1">
    <w:name w:val="heading 1"/>
    <w:basedOn w:val="Normal"/>
  </w:style>
  <w:style w:type="character" w:default="1" w:styleId="DefaultParagraphFont">
    <w:name w:val="Default Paragraph Font"/>
  </w:style>
  <w:style w:type="character" w:styleId="Strong">
    <w:name w:val="Strong"/>
    <w:basedOn w:val="DefaultParagraphFont"/>
  </w:style>
</w:styles>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/styles.xml", styles_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK_FALSE(doc.set_paragraph_style(paragraph, ""));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    auto run = paragraph.runs();
    REQUIRE(run.has_next());
    CHECK_FALSE(doc.set_run_style(run, ""));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK(doc.clear_paragraph_style(paragraph));
    CHECK(doc.clear_run_style(run));
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:pStyle"), 0);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:rStyle"), 0);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:pPr>"), 0);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:rPr>"), 0);

    CHECK(test_docx_entry_exists(target, "word/styles.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "seed\n");

    fs::remove(target);
}

TEST_CASE("run font family APIs write rFonts and clear removes empty run properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "run_font_family_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"\u4E2D\u6587 CJK smoke");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());

    auto run = paragraph.add_run(run_text);
    REQUIRE(run.has_next());

    CHECK_FALSE(run.set_font_family(""));
    CHECK_FALSE(run.set_east_asia_font_family(""));
    CHECK(run.set_font_family("Segoe UI"));
    CHECK(run.set_east_asia_font_family("Microsoft YaHei"));

    const auto font_family = run.font_family();
    REQUIRE(font_family.has_value());
    CHECK_EQ(*font_family, "Segoe UI");

    const auto east_asia_font = run.east_asia_font_family();
    REQUIRE(east_asia_font.has_value());
    CHECK_EQ(*east_asia_font, "Microsoft YaHei");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("w:rFonts"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:ascii=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:hAnsi=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:cs=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:eastAsia=\"Microsoft YaHei\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_run = reopened.paragraphs().runs();
    REQUIRE(reopened_run.has_next());
    CHECK_EQ(reopened_run.get_text(), run_text);

    const auto reopened_font_family = reopened_run.font_family();
    REQUIRE(reopened_font_family.has_value());
    CHECK_EQ(*reopened_font_family, "Segoe UI");

    const auto reopened_east_asia_font = reopened_run.east_asia_font_family();
    REQUIRE(reopened_east_asia_font.has_value());
    CHECK_EQ(*reopened_east_asia_font, "Microsoft YaHei");

    CHECK(reopened_run.clear_font_family());
    CHECK_FALSE(reopened.save());

    const auto cleared_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(cleared_document_xml, "<w:rFonts"), 0);
    CHECK_EQ(count_substring_occurrences(cleared_document_xml, "<w:rPr>"), 0);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    auto cleared_run = cleared.paragraphs().runs();
    REQUIRE(cleared_run.has_next());
    CHECK_FALSE(cleared_run.font_family().has_value());
    CHECK_FALSE(cleared_run.east_asia_font_family().has_value());

    fs::remove(target);
}

TEST_CASE("default run font APIs edit docDefaults and round-trip through styles.xml") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "default_run_font_family_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"\u9ED8\u8BA4\u5B57\u4F53\u5199\u5165");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_FALSE(doc.set_default_run_font_family(""));
    CHECK_FALSE(doc.set_default_run_east_asia_font_family(""));
    CHECK_FALSE(doc.default_run_font_family().has_value());
    CHECK_FALSE(doc.default_run_east_asia_font_family().has_value());

    CHECK(doc.set_default_run_font_family("Segoe UI"));
    CHECK(doc.set_default_run_east_asia_font_family("Microsoft YaHei"));

    const auto default_font_family = doc.default_run_font_family();
    REQUIRE(default_font_family.has_value());
    CHECK_EQ(*default_font_family, "Segoe UI");

    const auto default_east_asia_font = doc.default_run_east_asia_font_family();
    REQUIRE(default_east_asia_font.has_value());
    CHECK_EQ(*default_east_asia_font, "Microsoft YaHei");

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run(run_text).has_next());

    CHECK_FALSE(doc.save());
    CHECK(test_docx_entry_exists(target, "word/styles.xml"));

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("<w:docDefaults>"), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:ascii=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:hAnsi=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:cs=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:eastAsia=\"Microsoft YaHei\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_default_font_family = reopened.default_run_font_family();
    REQUIRE(reopened_default_font_family.has_value());
    CHECK_EQ(*reopened_default_font_family, "Segoe UI");

    const auto reopened_default_east_asia_font = reopened.default_run_east_asia_font_family();
    REQUIRE(reopened_default_east_asia_font.has_value());
    CHECK_EQ(*reopened_default_east_asia_font, "Microsoft YaHei");

    CHECK_EQ(collect_document_text(reopened), run_text + "\n");
    CHECK(reopened.clear_default_run_font_family());
    CHECK_FALSE(reopened.save());

    const auto cleared_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(count_substring_occurrences(cleared_styles_xml, "w:eastAsia=\"Microsoft YaHei\""), 0);
    CHECK_EQ(count_substring_occurrences(cleared_styles_xml, "w:ascii=\"Segoe UI\""), 0);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_FALSE(cleared.default_run_font_family().has_value());
    CHECK_FALSE(cleared.default_run_east_asia_font_family().has_value());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}

TEST_CASE("style run font APIs edit styles.xml and preserve unrelated style markup") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_run_font_family_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"\u6837\u5F0F\u5B57\u4F53\u5199\u5165");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_FALSE(doc.set_style_run_font_family("", "Segoe UI"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_style_run_font_family("MissingStyle", "Segoe UI"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.style_run_font_family("MissingStyle").has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK(doc.set_style_run_font_family("Strong", "Segoe UI"));
    CHECK(doc.set_style_run_east_asia_font_family("Strong", "Microsoft YaHei"));

    const auto style_font_family = doc.style_run_font_family("Strong");
    REQUIRE(style_font_family.has_value());
    CHECK_EQ(*style_font_family, "Segoe UI");

    const auto style_east_asia_font = doc.style_run_east_asia_font_family("Strong");
    REQUIRE(style_east_asia_font.has_value());
    CHECK_EQ(*style_east_asia_font, "Microsoft YaHei");

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run(run_text);
    REQUIRE(run.has_next());
    CHECK(doc.set_run_style(run, "Strong"));

    CHECK_FALSE(doc.save());

    auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("w:styleId=\"Strong\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:ascii=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:eastAsia=\"Microsoft YaHei\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:b"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), run_text + "\n");

    const auto reopened_style_font_family = reopened.style_run_font_family("Strong");
    REQUIRE(reopened_style_font_family.has_value());
    CHECK_EQ(*reopened_style_font_family, "Segoe UI");

    const auto reopened_style_east_asia_font = reopened.style_run_east_asia_font_family("Strong");
    REQUIRE(reopened_style_east_asia_font.has_value());
    CHECK_EQ(*reopened_style_east_asia_font, "Microsoft YaHei");

    CHECK(reopened.clear_style_run_font_family("Strong"));
    CHECK_FALSE(reopened.save());

    saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:eastAsia=\"Microsoft YaHei\""), 0);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:ascii=\"Segoe UI\""), 0);
    CHECK_NE(saved_styles_xml.find("w:b"), std::string::npos);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_FALSE(cleared.style_run_font_family("Strong").has_value());
    CHECK_FALSE(cleared.style_run_east_asia_font_family("Strong").has_value());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}

TEST_CASE("run language APIs write w:lang and clear removes empty run properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "run_language_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"中文 language smoke");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());

    auto run = paragraph.add_run(run_text);
    REQUIRE(run.has_next());

    CHECK_FALSE(run.set_language(""));
    CHECK_FALSE(run.set_east_asia_language(""));
    CHECK_FALSE(run.set_bidi_language(""));
    CHECK(run.set_language("en-US"));
    CHECK(run.set_east_asia_language("zh-CN"));
    CHECK(run.set_bidi_language("ar-SA"));

    const auto language = run.language();
    REQUIRE(language.has_value());
    CHECK_EQ(*language, "en-US");

    const auto east_asia_language = run.east_asia_language();
    REQUIRE(east_asia_language.has_value());
    CHECK_EQ(*east_asia_language, "zh-CN");

    const auto bidi_language = run.bidi_language();
    REQUIRE(bidi_language.has_value());
    CHECK_EQ(*bidi_language, "ar-SA");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:lang"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:val=\"en-US\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:eastAsia=\"zh-CN\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:bidi=\"ar-SA\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_run = reopened.paragraphs().runs();
    REQUIRE(reopened_run.has_next());
    CHECK_EQ(reopened_run.get_text(), run_text);

    const auto reopened_language = reopened_run.language();
    REQUIRE(reopened_language.has_value());
    CHECK_EQ(*reopened_language, "en-US");

    const auto reopened_east_asia_language = reopened_run.east_asia_language();
    REQUIRE(reopened_east_asia_language.has_value());
    CHECK_EQ(*reopened_east_asia_language, "zh-CN");

    const auto reopened_bidi_language = reopened_run.bidi_language();
    REQUIRE(reopened_bidi_language.has_value());
    CHECK_EQ(*reopened_bidi_language, "ar-SA");

    CHECK(reopened_run.clear_language());
    CHECK_FALSE(reopened.save());

    const auto cleared_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(cleared_document_xml, "<w:lang"), 0);
    CHECK_EQ(count_substring_occurrences(cleared_document_xml, "<w:rPr>"), 0);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    auto cleared_run = cleared.paragraphs().runs();
    REQUIRE(cleared_run.has_next());
    CHECK_FALSE(cleared_run.language().has_value());
    CHECK_FALSE(cleared_run.east_asia_language().has_value());
    CHECK_FALSE(cleared_run.bidi_language().has_value());

    fs::remove(target);
}

TEST_CASE("default run language APIs edit docDefaults and round-trip through styles.xml") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "default_run_language_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"默认语言写入");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_FALSE(doc.set_default_run_language(""));
    CHECK_FALSE(doc.set_default_run_east_asia_language(""));
    CHECK_FALSE(doc.set_default_run_bidi_language(""));
    CHECK_FALSE(doc.default_run_language().has_value());
    CHECK_FALSE(doc.default_run_east_asia_language().has_value());
    CHECK_FALSE(doc.default_run_bidi_language().has_value());

    CHECK(doc.set_default_run_language("en-US"));
    CHECK(doc.set_default_run_east_asia_language("zh-CN"));
    CHECK(doc.set_default_run_bidi_language("ar-SA"));

    const auto default_language = doc.default_run_language();
    REQUIRE(default_language.has_value());
    CHECK_EQ(*default_language, "en-US");

    const auto default_east_asia_language = doc.default_run_east_asia_language();
    REQUIRE(default_east_asia_language.has_value());
    CHECK_EQ(*default_east_asia_language, "zh-CN");

    const auto default_bidi_language = doc.default_run_bidi_language();
    REQUIRE(default_bidi_language.has_value());
    CHECK_EQ(*default_bidi_language, "ar-SA");

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run(run_text).has_next());

    CHECK_FALSE(doc.save());
    CHECK(test_docx_entry_exists(target, "word/styles.xml"));

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("<w:docDefaults>"), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:lang"), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:val=\"en-US\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:eastAsia=\"zh-CN\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:bidi=\"ar-SA\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_default_language = reopened.default_run_language();
    REQUIRE(reopened_default_language.has_value());
    CHECK_EQ(*reopened_default_language, "en-US");

    const auto reopened_default_east_asia_language =
        reopened.default_run_east_asia_language();
    REQUIRE(reopened_default_east_asia_language.has_value());
    CHECK_EQ(*reopened_default_east_asia_language, "zh-CN");

    const auto reopened_default_bidi_language = reopened.default_run_bidi_language();
    REQUIRE(reopened_default_bidi_language.has_value());
    CHECK_EQ(*reopened_default_bidi_language, "ar-SA");

    CHECK_EQ(collect_document_text(reopened), run_text + "\n");
    CHECK(reopened.clear_default_run_language());
    CHECK_FALSE(reopened.save());

    const auto cleared_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(count_substring_occurrences(cleared_styles_xml, "w:eastAsia=\"zh-CN\""), 0);
    CHECK_EQ(count_substring_occurrences(cleared_styles_xml, "w:val=\"en-US\""), 0);
    CHECK_EQ(count_substring_occurrences(cleared_styles_xml, "w:bidi=\"ar-SA\""), 0);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_FALSE(cleared.default_run_language().has_value());
    CHECK_FALSE(cleared.default_run_east_asia_language().has_value());
    CHECK_FALSE(cleared.default_run_bidi_language().has_value());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}

TEST_CASE("style run language APIs edit styles.xml and preserve unrelated style markup") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_run_language_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"样式语言写入");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_FALSE(doc.set_style_run_language("", "en-US"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_style_run_language("MissingStyle", "en-US"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.style_run_language("MissingStyle").has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK(doc.set_style_run_language("Strong", "en-US"));
    CHECK(doc.set_style_run_east_asia_language("Strong", "zh-CN"));
    CHECK(doc.set_style_run_bidi_language("Strong", "ar-SA"));

    const auto style_language = doc.style_run_language("Strong");
    REQUIRE(style_language.has_value());
    CHECK_EQ(*style_language, "en-US");

    const auto style_east_asia_language = doc.style_run_east_asia_language("Strong");
    REQUIRE(style_east_asia_language.has_value());
    CHECK_EQ(*style_east_asia_language, "zh-CN");

    const auto style_bidi_language = doc.style_run_bidi_language("Strong");
    REQUIRE(style_bidi_language.has_value());
    CHECK_EQ(*style_bidi_language, "ar-SA");

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run(run_text);
    REQUIRE(run.has_next());
    CHECK(doc.set_run_style(run, "Strong"));

    CHECK_FALSE(doc.save());

    auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("w:styleId=\"Strong\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:lang"), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:val=\"en-US\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:eastAsia=\"zh-CN\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:bidi=\"ar-SA\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:b"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), run_text + "\n");

    const auto reopened_style_language = reopened.style_run_language("Strong");
    REQUIRE(reopened_style_language.has_value());
    CHECK_EQ(*reopened_style_language, "en-US");

    const auto reopened_style_east_asia_language =
        reopened.style_run_east_asia_language("Strong");
    REQUIRE(reopened_style_east_asia_language.has_value());
    CHECK_EQ(*reopened_style_east_asia_language, "zh-CN");

    const auto reopened_style_bidi_language = reopened.style_run_bidi_language("Strong");
    REQUIRE(reopened_style_bidi_language.has_value());
    CHECK_EQ(*reopened_style_bidi_language, "ar-SA");

    CHECK(reopened.clear_style_run_language("Strong"));
    CHECK_FALSE(reopened.save());

    saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:eastAsia=\"zh-CN\""), 0);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:val=\"en-US\""), 0);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:bidi=\"ar-SA\""), 0);
    CHECK_NE(saved_styles_xml.find("w:b"), std::string::npos);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_FALSE(cleared.style_run_language("Strong").has_value());
    CHECK_FALSE(cleared.style_run_east_asia_language("Strong").has_value());
    CHECK_FALSE(cleared.style_run_bidi_language("Strong").has_value());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}

TEST_CASE("run rtl APIs write w:rtl and clear removes empty run properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "run_rtl_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"مرحبا RTL 123");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());

    auto run = paragraph.add_run(run_text);
    REQUIRE(run.has_next());

    CHECK_FALSE(run.rtl().has_value());
    CHECK(run.set_rtl());

    const auto rtl = run.rtl();
    REQUIRE(rtl.has_value());
    CHECK(*rtl);

    CHECK_FALSE(doc.save());

    auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:rtl"), std::string::npos);
    CHECK_NE(saved_document_xml.find("<w:rtl w:val=\"1\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_run = reopened.paragraphs().runs();
    REQUIRE(reopened_run.has_next());
    CHECK_EQ(reopened_run.get_text(), run_text);

    const auto reopened_rtl = reopened_run.rtl();
    REQUIRE(reopened_rtl.has_value());
    CHECK(*reopened_rtl);

    CHECK(reopened_run.clear_rtl());
    CHECK_FALSE(reopened.save());

    saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:rtl"), 0);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:rPr>"), 0);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    auto cleared_run = cleared.paragraphs().runs();
    REQUIRE(cleared_run.has_next());
    CHECK_FALSE(cleared_run.rtl().has_value());

    fs::remove(target);
}

TEST_CASE("paragraph bidi APIs write w:bidi and clear removes empty paragraph properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_bidi_roundtrip.docx";
    const auto paragraph_text = utf8_from_u8(u8"فقرة عربية مع 123");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run(paragraph_text).has_next());

    CHECK_FALSE(paragraph.bidi().has_value());
    CHECK(paragraph.set_bidi());

    auto bidi = paragraph.bidi();
    REQUIRE(bidi.has_value());
    CHECK(*bidi);

    CHECK_FALSE(doc.save());

    auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:bidi"), std::string::npos);
    CHECK_NE(saved_document_xml.find("<w:bidi w:val=\"1\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_paragraph = reopened.paragraphs();
    REQUIRE(reopened_paragraph.has_next());
    CHECK_EQ(collect_document_text(reopened), paragraph_text + "\n");

    bidi = reopened_paragraph.bidi();
    REQUIRE(bidi.has_value());
    CHECK(*bidi);

    CHECK(reopened_paragraph.set_bidi(false));
    CHECK_FALSE(reopened.save());

    saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:bidi w:val=\"0\""), std::string::npos);

    featherdoc::Document reopened_false(target);
    CHECK_FALSE(reopened_false.open());
    auto false_paragraph = reopened_false.paragraphs();
    REQUIRE(false_paragraph.has_next());
    bidi = false_paragraph.bidi();
    REQUIRE(bidi.has_value());
    CHECK_FALSE(*bidi);

    CHECK(false_paragraph.clear_bidi());
    CHECK_FALSE(reopened_false.save());

    saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:bidi"), 0);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:pPr>"), 0);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_EQ(collect_document_text(cleared), paragraph_text + "\n");
    CHECK_FALSE(cleared.paragraphs().bidi().has_value());

    fs::remove(target);
}

TEST_CASE("default direction APIs edit docDefaults and round-trip through styles.xml") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "default_direction_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"默认方向写入");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_FALSE(doc.default_run_rtl().has_value());
    CHECK_FALSE(doc.default_paragraph_bidi().has_value());
    CHECK(doc.set_default_run_rtl());
    CHECK(doc.set_default_paragraph_bidi());

    auto default_run_rtl = doc.default_run_rtl();
    REQUIRE(default_run_rtl.has_value());
    CHECK(*default_run_rtl);

    auto default_paragraph_bidi = doc.default_paragraph_bidi();
    REQUIRE(default_paragraph_bidi.has_value());
    CHECK(*default_paragraph_bidi);

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run(run_text).has_next());

    CHECK_FALSE(doc.save());

    auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("<w:rtl w:val=\"1\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:bidi w:val=\"1\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    default_run_rtl = reopened.default_run_rtl();
    REQUIRE(default_run_rtl.has_value());
    CHECK(*default_run_rtl);

    default_paragraph_bidi = reopened.default_paragraph_bidi();
    REQUIRE(default_paragraph_bidi.has_value());
    CHECK(*default_paragraph_bidi);

    CHECK_EQ(collect_document_text(reopened), run_text + "\n");
    CHECK(reopened.set_default_run_rtl(false));
    CHECK(reopened.set_default_paragraph_bidi(false));
    CHECK_FALSE(reopened.save());

    saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("<w:rtl w:val=\"0\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:bidi w:val=\"0\""), std::string::npos);

    featherdoc::Document reopened_false(target);
    CHECK_FALSE(reopened_false.open());
    default_run_rtl = reopened_false.default_run_rtl();
    REQUIRE(default_run_rtl.has_value());
    CHECK_FALSE(*default_run_rtl);
    default_paragraph_bidi = reopened_false.default_paragraph_bidi();
    REQUIRE(default_paragraph_bidi.has_value());
    CHECK_FALSE(*default_paragraph_bidi);

    CHECK(reopened_false.clear_default_run_rtl());
    CHECK(reopened_false.clear_default_paragraph_bidi());
    CHECK_FALSE(reopened_false.save());

    saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "<w:rtl"), 0);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "<w:bidi"), 0);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_FALSE(cleared.default_run_rtl().has_value());
    CHECK_FALSE(cleared.default_paragraph_bidi().has_value());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}

TEST_CASE("style direction APIs edit styles.xml and preserve unrelated style markup") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_direction_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"样式方向写入");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_FALSE(doc.set_style_run_rtl(""));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_style_run_rtl("MissingStyle"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.style_run_rtl("MissingStyle").has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.set_style_paragraph_bidi(""));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_style_paragraph_bidi("MissingStyle"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.style_paragraph_bidi("MissingStyle").has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK(doc.set_style_run_rtl("Emphasis"));
    CHECK(doc.set_style_paragraph_bidi("Heading1"));

    auto style_run_rtl = doc.style_run_rtl("Emphasis");
    REQUIRE(style_run_rtl.has_value());
    CHECK(*style_run_rtl);

    auto style_paragraph_bidi = doc.style_paragraph_bidi("Heading1");
    REQUIRE(style_paragraph_bidi.has_value());
    CHECK(*style_paragraph_bidi);

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run(run_text);
    REQUIRE(run.has_next());
    CHECK(doc.set_paragraph_style(paragraph, "Heading1"));
    CHECK(doc.set_run_style(run, "Emphasis"));

    CHECK_FALSE(doc.save());

    auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("w:styleId=\"Emphasis\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:styleId=\"Heading1\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:rtl w:val=\"1\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:bidi w:val=\"1\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:i"), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:outlineLvl"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), run_text + "\n");

    style_run_rtl = reopened.style_run_rtl("Emphasis");
    REQUIRE(style_run_rtl.has_value());
    CHECK(*style_run_rtl);

    style_paragraph_bidi = reopened.style_paragraph_bidi("Heading1");
    REQUIRE(style_paragraph_bidi.has_value());
    CHECK(*style_paragraph_bidi);

    CHECK(reopened.set_style_run_rtl("Emphasis", false));
    CHECK(reopened.set_style_paragraph_bidi("Heading1", false));
    CHECK_FALSE(reopened.save());

    saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("<w:rtl w:val=\"0\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:bidi w:val=\"0\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:i"), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:outlineLvl"), std::string::npos);

    featherdoc::Document reopened_false(target);
    CHECK_FALSE(reopened_false.open());
    style_run_rtl = reopened_false.style_run_rtl("Emphasis");
    REQUIRE(style_run_rtl.has_value());
    CHECK_FALSE(*style_run_rtl);
    style_paragraph_bidi = reopened_false.style_paragraph_bidi("Heading1");
    REQUIRE(style_paragraph_bidi.has_value());
    CHECK_FALSE(*style_paragraph_bidi);

    CHECK(reopened_false.clear_style_run_rtl("Emphasis"));
    CHECK(reopened_false.clear_style_paragraph_bidi("Heading1"));
    CHECK_FALSE(reopened_false.save());

    saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "<w:rtl"), 0);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "<w:bidi w:val=\"0\""), 0);
    CHECK_NE(saved_styles_xml.find("<w:i"), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:outlineLvl"), std::string::npos);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_FALSE(cleared.style_run_rtl("Emphasis").has_value());
    CHECK_FALSE(cleared.style_paragraph_bidi("Heading1").has_value());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}
