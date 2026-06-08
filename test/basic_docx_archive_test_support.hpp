#pragma once

#include "doctest.h"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <zip.h>

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

bool write_basic_archive_entry(zip_t *archive, const char *entry_name,
                               std::string_view content) {
    if (zip_entry_open(archive, entry_name) != 0) {
        return false;
    }
    if (zip_entry_write(archive, content.data(), content.size()) < 0) {
        zip_entry_close(archive);
        return false;
    }
    return zip_entry_close(archive) == 0;
}

auto write_test_archive_entries(
    const std::filesystem::path &path,
    const std::vector<std::pair<std::string, std::string>> &entries) -> void;

auto write_test_docx(const std::filesystem::path &path,
                     const std::string &document_xml) -> void {
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

auto write_test_docx_with_styles(const std::filesystem::path &path,
                                 const std::string &document_xml,
                                 const std::string &styles_xml) -> void {
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
    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles"
                Target="styles.xml"/>
</Relationships>
)";

    write_test_archive_entries(
        path,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/styles.xml", styles_xml},
        });
}

auto write_test_docx_with_header_footer(
    const std::filesystem::path &path, std::string_view body_text,
    std::string_view header_text, std::string_view footer_text) -> void {
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

    auto document_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>)"};
    document_xml += body_text;
    document_xml += R"(</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId2"/>
      <w:footerReference w:type="default" r:id="rId3"/>
    </w:sectPr>
  </w:body>
</w:document>
)";

    auto header_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>)"};
    header_xml += header_text;
    header_xml += R"(</w:t></w:r></w:p>
</w:hdr>
)";

    auto footer_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>)"};
    footer_xml += footer_text;
    footer_xml += R"(</w:t></w:r></w:p>
</w:ftr>
)";

    write_test_archive_entries(
        path,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header_xml},
            {"word/footer1.xml", footer_xml},
        });
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

auto read_test_docx_entry(const std::filesystem::path &path,
                          const char *entry_name) -> std::string {
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

auto rewrite_test_docx_entry(const std::filesystem::path &path,
                             const char *entry_name,
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

auto test_docx_entry_exists(const std::filesystem::path &path,
                            const char *entry_name) -> bool {
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

} // namespace
