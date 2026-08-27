#include "basic_image_fixture_test_support.hpp"
#include "allocation_failure_test_case.hpp"
#include "document_core_unit_test_support.hpp"

#include <algorithm>
#include <atomic>
#include <barrier>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "document_save_failure_test_support.hpp"
#include "zip_failure_test_support.hpp"

#ifndef MINIZ_HEADER_FILE_ONLY
#define MINIZ_HEADER_FILE_ONLY
#endif
#include <miniz.h>

#include <featherdoc/detail/path.hpp>

extern "C" void document_test_force_next_temp_reservation(
    std::uint64_t timestamp, std::uint64_t sequence_start);

#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {

constexpr auto valid_document_xml =
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p><w:r><w:t>secure</w:t></w:r></w:p></w:body>
</w:document>
)";

constexpr auto wordprocessingml_namespace_uri =
    "http://schemas.openxmlformats.org/wordprocessingml/2006/main";
constexpr auto comments_extended_namespace_uri =
    "http://schemas.microsoft.com/office/word/2012/wordml";
constexpr auto office_document_relationship_type =
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
    "officeDocument";
constexpr auto header_relationship_type =
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
    "header";
constexpr auto footer_relationship_type =
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
    "footer";
constexpr auto settings_relationship_type =
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
    "settings";
constexpr auto numbering_relationship_type =
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
    "numbering";
constexpr auto styles_relationship_type =
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
    "styles";
constexpr auto footnotes_relationship_type =
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
    "footnotes";
constexpr auto endnotes_relationship_type =
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
    "endnotes";
constexpr auto comments_relationship_type =
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
    "comments";
constexpr auto comments_extended_relationship_type =
    "http://schemas.microsoft.com/office/2011/relationships/commentsExtended";
constexpr auto header_content_type =
    "application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml";
constexpr auto footer_content_type =
    "application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml";
constexpr auto settings_content_type =
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.settings+xml";
constexpr auto numbering_content_type =
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml";
constexpr auto styles_content_type =
    "application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml";
constexpr auto footnotes_content_type =
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.footnotes+xml";
constexpr auto endnotes_content_type =
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.endnotes+xml";
constexpr auto comments_content_type =
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.comments+xml";
constexpr auto comments_extended_content_type =
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.commentsExtended+xml";

struct related_xml_part_fixture {
    std::string relationship_id;
    std::string relationship_type;
    std::string target;
    std::string part_name;
    std::string content_type;
    std::string entry_name;
    std::string xml;
    std::string target_mode;
};

auto semantic_xml_limit_test_options() -> featherdoc::document_open_options {
    featherdoc::document_open_options options;
    options.limits.max_xml_part_bytes = 2048U;
    options.limits.max_binary_part_bytes = 8192U;
    options.limits.max_total_uncompressed_bytes = 64U * 1024U;
    options.limits.max_compression_ratio = 10'000U;
    return options;
}

auto oversized_xml_payload(std::string_view prefix, std::string_view suffix)
    -> std::string {
    const auto target_size =
        semantic_xml_limit_test_options().limits.max_xml_part_bytes + 512U;
    std::string xml{prefix};
    xml += "<!--";
    const auto comment_suffix_size = std::string_view{"-->"}.size();
    if (xml.size() + comment_suffix_size + suffix.size() < target_size) {
        const auto filler_size =
            target_size - xml.size() - comment_suffix_size - suffix.size();
        for (std::size_t index = 0U; index < filler_size; ++index) {
            xml.push_back(static_cast<char>('a' + (index % 26U)));
        }
    }
    xml += "-->";
    xml += suffix;
    return xml;
}

auto minimal_xml_payload(std::string_view prefix, std::string_view suffix)
    -> std::string {
    std::string xml{prefix};
    xml += suffix;
    return xml;
}

auto namespaced_word_root_prefix(std::string_view root_name) -> std::string {
    return "<" + std::string{root_name} + " xmlns:w=\"" +
           wordprocessingml_namespace_uri + "\">";
}

auto namespaced_word_root_suffix(std::string_view root_name) -> std::string {
    return "</" + std::string{root_name} + ">";
}

void write_docx_with_related_xml_parts(
    const std::filesystem::path &path,
    const std::vector<related_xml_part_fixture> &parts,
    std::string_view document_xml = valid_document_xml) {
    std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
)";
    for (const auto &part : parts) {
        content_types_xml += "  <Override PartName=\"" + part.part_name +
                             "\" ContentType=\"" + part.content_type + "\"/>\n";
    }
    content_types_xml += "</Types>\n";

    std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
)";
    for (const auto &part : parts) {
        document_relationships_xml +=
            "  <Relationship Id=\"" + part.relationship_id + "\" Type=\"" +
            part.relationship_type + "\" Target=\"" + part.target + "\"";
        if (!part.target_mode.empty()) {
            document_relationships_xml +=
                " TargetMode=\"" + part.target_mode + "\"";
        }
        document_relationships_xml += "/>\n";
    }
    document_relationships_xml += "</Relationships>\n";

    std::vector<std::pair<std::string, std::string>> entries{
        {test_content_types_xml_entry, content_types_xml},
        {test_relationships_xml_entry, test_relationships_xml},
        {test_document_xml_entry, std::string{document_xml}},
        {"word/_rels/document.xml.rels", document_relationships_xml},
    };
    for (const auto &part : parts) {
        entries.emplace_back(part.entry_name, part.xml);
    }
    write_test_archive_entries(path, entries);
}

auto read_file_text(const std::filesystem::path &path) -> std::string {
    std::ifstream stream(path, std::ios::binary);
    return {std::istreambuf_iterator<char>{stream},
            std::istreambuf_iterator<char>{}};
}

void write_file_text(const std::filesystem::path &path, std::string_view text) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    REQUIRE(stream.good());
    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    REQUIRE(stream.good());
}

void append_le16(std::vector<unsigned char> &bytes, std::uint16_t value) {
    bytes.push_back(static_cast<unsigned char>(value & 0xFFU));
    bytes.push_back(static_cast<unsigned char>((value >> 8U) & 0xFFU));
}

void append_le32(std::vector<unsigned char> &bytes, std::uint32_t value) {
    append_le16(bytes, static_cast<std::uint16_t>(value & 0xFFFFU));
    append_le16(bytes, static_cast<std::uint16_t>((value >> 16U) & 0xFFFFU));
}

void append_le64(std::vector<unsigned char> &bytes, std::uint64_t value) {
    append_le32(bytes, static_cast<std::uint32_t>(value & 0xFFFFFFFFULL));
    append_le32(bytes,
                static_cast<std::uint32_t>((value >> 32U) & 0xFFFFFFFFULL));
}

void append_ascii(std::vector<unsigned char> &bytes, std::string_view text) {
    bytes.insert(bytes.end(), text.begin(), text.end());
}

void append_zip64_size_extra(std::vector<unsigned char> &bytes,
                             std::uint64_t uncompressed_size,
                             std::uint64_t compressed_size) {
    append_le16(bytes, 0x0001U);
    append_le16(bytes, 16U);
    append_le64(bytes, uncompressed_size);
    append_le64(bytes, compressed_size);
}

auto zip64_compressed_range_overflow_archive_for_test()
    -> std::vector<unsigned char> {
    constexpr std::string_view file_name = "huge.bin";
    constexpr auto zip64_uncompressed_size = std::uint64_t{1U};
    constexpr auto zip32_sentinel = std::uint32_t{0xFFFFFFFFU};

    std::vector<unsigned char> bytes;
    append_le32(bytes, 0x04034B50U);
    append_le16(bytes, 45U);
    append_le16(bytes, 0U);
    append_le16(bytes, 0U);
    append_le16(bytes, 0U);
    append_le16(bytes, 0U);
    append_le32(bytes, 0U);
    append_le32(bytes, zip32_sentinel);
    append_le32(bytes, zip32_sentinel);
    append_le16(bytes, static_cast<std::uint16_t>(file_name.size()));
    append_le16(bytes, 20U);
    append_ascii(bytes, file_name);

    const auto file_data_offset =
        static_cast<std::uint64_t>(30U + file_name.size() + 20U);
    const auto wrapped_compressed_size =
        std::numeric_limits<std::uint64_t>::max() - file_data_offset + 8U;
    append_zip64_size_extra(bytes, zip64_uncompressed_size,
                            wrapped_compressed_size);

    const auto central_directory_offset =
        static_cast<std::uint32_t>(bytes.size());
    append_le32(bytes, 0x02014B50U);
    append_le16(bytes, 45U);
    append_le16(bytes, 45U);
    append_le16(bytes, 0U);
    append_le16(bytes, 0U);
    append_le16(bytes, 0U);
    append_le16(bytes, 0U);
    append_le32(bytes, 0U);
    append_le32(bytes, zip32_sentinel);
    append_le32(bytes, zip32_sentinel);
    append_le16(bytes, static_cast<std::uint16_t>(file_name.size()));
    append_le16(bytes, 20U);
    append_le16(bytes, 0U);
    append_le16(bytes, 0U);
    append_le16(bytes, 0U);
    append_le32(bytes, 0U);
    append_le32(bytes, 0U);
    append_ascii(bytes, file_name);
    append_zip64_size_extra(bytes, zip64_uncompressed_size,
                            wrapped_compressed_size);

    const auto central_directory_size =
        static_cast<std::uint32_t>(bytes.size() - central_directory_offset);
    append_le32(bytes, 0x06054B50U);
    append_le16(bytes, 0U);
    append_le16(bytes, 0U);
    append_le16(bytes, 1U);
    append_le16(bytes, 1U);
    append_le32(bytes, central_directory_size);
    append_le32(bytes, central_directory_offset);
    append_le16(bytes, 0U);
    return bytes;
}

auto ascii_fold_archive_entry_for_test(std::string_view entry_name)
    -> std::string {
    std::string folded{entry_name};
    for (auto &character : folded) {
        const auto byte = static_cast<unsigned char>(character);
        if (byte >= 'A' && byte <= 'Z') {
            character = static_cast<char>(byte + ('a' - 'A'));
        }
    }
    return folded;
}

auto archive_entries_contain_exact(
    const std::vector<std::pair<std::string, std::string>> &entries,
    std::string_view expected_entry_name) -> bool {
    return std::ranges::any_of(entries, [&](const auto &entry) {
        return entry.first == expected_entry_name;
    });
}

auto archive_entries_count_ascii_folded(
    const std::vector<std::pair<std::string, std::string>> &entries,
    std::string_view expected_folded_entry_name) -> std::size_t {
    return static_cast<std::size_t>(
        std::ranges::count_if(entries, [&](const auto &entry) {
            return ascii_fold_archive_entry_for_test(entry.first) ==
                   expected_folded_entry_name;
        }));
}

void write_tiny_png(const std::filesystem::path &path) {
    constexpr unsigned char png[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U,
        0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x00U, 0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU,
        0x15U, 0xC4U, 0x89U, 0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U,
        0x54U, 0x78U, 0x9CU, 0x63U, 0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U,
        0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U, 0x00U, 0x00U, 0x00U, 0x00U, 0x49U,
        0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U, 0x82U,
    };
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    REQUIRE(stream.good());
    stream.write(reinterpret_cast<const char *>(png), sizeof(png));
    REQUIRE(stream.good());
}

#ifndef _WIN32
auto filesystem_supports_posix_mode_bits(const std::filesystem::path &directory)
    -> bool {
    const auto probe =
        directory / (".featherdoc-mode-probe-" +
                     std::to_string(static_cast<std::uint64_t>(getpid())));
    std::filesystem::remove(probe);
    write_file_text(probe, "mode probe");
    REQUIRE_EQ(::chmod(probe.c_str(), 0640), 0);

    struct stat probe_status{};
    REQUIRE_EQ(::stat(probe.c_str(), &probe_status), 0);
    const auto supports_mode_bits = (probe_status.st_mode & 07777) == 0640;
    std::filesystem::remove(probe);
    return supports_mode_bits;
}
#endif

auto unique_temp_files_for(const std::filesystem::path &target) -> std::size_t {
#ifdef _WIN32
    const auto process_id = static_cast<std::uint64_t>(GetCurrentProcessId());
#else
    const auto process_id = static_cast<std::uint64_t>(getpid());
#endif
    const auto prefix =
        std::filesystem::path{".featherdoc-" + std::to_string(process_id) + "-"}
            .native();
    std::size_t count = 0U;
    for (const auto &entry :
         std::filesystem::directory_iterator(target.parent_path())) {
        if (entry.path().filename().native().starts_with(prefix)) {
            ++count;
        }
    }
    return count;
}

auto forced_temp_collision_path(const std::filesystem::path &directory,
                                std::uint64_t timestamp,
                                std::uint64_t sequence_value)
    -> std::filesystem::path {
#ifdef _WIN32
    const auto process_id = static_cast<std::uint64_t>(GetCurrentProcessId());
#else
    const auto process_id = static_cast<std::uint64_t>(getpid());
#endif
    return directory /
           (".featherdoc-" + std::to_string(process_id) + "-" +
            std::to_string(timestamp) + "-" + std::to_string(sequence_value) +
            ".tmp");
}

auto normalized_xml_document_element(std::string_view xml)
    -> std::optional<std::string> {
    pugi::xml_document document;
    if (!document.load_buffer(xml.data(), xml.size())) {
        return std::nullopt;
    }
    std::ostringstream output;
    document.document_element().print(output, "", pugi::format_raw,
                                      pugi::encoding_utf8);
    return output.str();
}

} // namespace

TEST_CASE("open and save support Unicode document and image paths") {
    namespace fs = std::filesystem;

    const auto directory =
        fs::current_path() / fs::path{u8"Unicode-中文-日本語-🙂-space dir"};
    const auto source = directory / fs::path{u8"源 文档-🙂.docx"};
    const auto image = directory / fs::path{u8"图片-日本語-🙂.png"};
    fs::remove_all(directory);
    REQUIRE(fs::create_directories(directory));

    write_test_docx(source, valid_document_xml);
    write_tiny_png(image);

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.append_image(image));
    REQUIRE(document.paragraphs().add_run(" Unicode edit").has_next());
    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(source);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(reopened.inline_images().size(), 1U);
    CHECK_NE(collect_document_text(reopened).find("Unicode edit"),
             std::string::npos);

    fs::remove_all(directory);
}

TEST_CASE("document I/O rejects embedded NUL paths before OS access") {
    namespace fs = std::filesystem;

    const auto open_prefix = fs::current_path() / "embedded_nul_open.docx";
    const auto save_prefix = fs::current_path() / "embedded_nul_save.docx";
    fs::remove(open_prefix);
    fs::remove(save_prefix);
    write_test_docx(open_prefix, valid_document_xml);
    write_file_text(save_prefix, "original target bytes");

    auto open_path_text = featherdoc::detail::path_to_utf8(open_prefix);
    open_path_text.push_back('\0');
    open_path_text += ".ignored";
    const auto open_path = featherdoc::detail::path_from_utf8(open_path_text);

    featherdoc::Document source_document(open_path);
    const auto open_error = source_document.open();
    CHECK_EQ(open_error, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(source_document.is_open());

    auto save_path_text = featherdoc::detail::path_to_utf8(save_prefix);
    save_path_text.push_back('\0');
    save_path_text += ".ignored";
    const auto save_path = featherdoc::detail::path_from_utf8(save_path_text);

    featherdoc::Document output_document;
    REQUIRE_FALSE(output_document.create_empty());
    const auto save_error = output_document.save_as(save_path);
    CHECK_EQ(save_error, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(read_file_text(save_prefix), "original target bytes");

    fs::remove(open_prefix);
    fs::remove(save_prefix);
}

TEST_CASE("strict open rejects malformed OPC structure and tolerant open is "
          "explicit") {
    namespace fs = std::filesystem;

    const auto check_modes = [](const fs::path &path,
                                featherdoc::document_errc strict_error) {
        featherdoc::Document strict_document(path);
        CHECK_EQ(strict_document.open(), strict_error);
        CHECK_FALSE(strict_document.is_open());

        featherdoc::document_open_options options;
        options.validation = featherdoc::package_validation_mode::tolerant;
        featherdoc::Document tolerant_document(path);
        CHECK_FALSE(tolerant_document.open(options));
        CHECK(tolerant_document.is_open());
    };

    SUBCASE("missing body") {
        const auto path = fs::current_path() / "strict_missing_body.docx";
        write_test_docx(
            path,
            R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"/>)");
        check_modes(path, featherdoc::document_errc::invalid_package_structure);
        fs::remove(path);
    }

    SUBCASE("non-WordprocessingML document root") {
        const auto path = fs::current_path() / "strict_wrong_namespace.docx";
        write_test_docx(
            path,
            R"(<x:document xmlns:x="urn:not-wordprocessingml"><x:body/></x:document>)");
        check_modes(path, featherdoc::document_errc::invalid_package_structure);
        fs::remove(path);
    }

    SUBCASE("reserved w prefix bound to a wrong namespace") {
        const auto path = fs::current_path() / "unsafe_wrong_w_binding.docx";
        write_test_docx(
            path,
            R"(<w:document xmlns:w="urn:not-wordprocessingml"><w:body/></w:document>)");

        featherdoc::Document strict_document(path);
        CHECK_EQ(strict_document.open(),
                 featherdoc::document_errc::invalid_package_structure);
        CHECK_FALSE(strict_document.is_open());
        CHECK_EQ(strict_document.last_error().entry_name,
                 test_document_xml_entry);
        CHECK_FALSE(strict_document.last_error().detail.empty());

        featherdoc::document_open_options options;
        options.validation = featherdoc::package_validation_mode::tolerant;
        featherdoc::Document tolerant_document(path);
        CHECK_EQ(tolerant_document.open(options),
                 featherdoc::document_errc::invalid_package_structure);
        CHECK_FALSE(tolerant_document.is_open());
        CHECK_EQ(tolerant_document.last_error().entry_name,
                 test_document_xml_entry);
        CHECK_FALSE(tolerant_document.last_error().detail.empty());
        fs::remove(path);
    }

    SUBCASE("missing content types") {
        const auto path =
            fs::current_path() / "strict_missing_content_types.docx";
        write_test_archive_entries(
            path, {{test_relationships_xml_entry, test_relationships_xml},
                   {test_document_xml_entry, valid_document_xml}});
        check_modes(path,
                    featherdoc::document_errc::content_types_xml_read_failed);
        fs::remove(path);
    }

    SUBCASE("missing root relationships") {
        const auto path = fs::current_path() / "strict_missing_root_rels.docx";
        write_test_archive_entries(
            path, {{test_content_types_xml_entry, test_content_types_xml},
                   {test_document_xml_entry, valid_document_xml}});
        check_modes(path, featherdoc::document_errc::invalid_package_structure);
        fs::remove(path);
    }

    SUBCASE("wrong main document MIME") {
        const auto path = fs::current_path() / "strict_wrong_mime.docx";
        auto wrong_content_types = std::string{test_content_types_xml};
        const auto expected = std::string{"application/"
                                          "vnd.openxmlformats-officedocument."
                                          "wordprocessingml.document.main+xml"};
        const auto position = wrong_content_types.find(expected);
        REQUIRE_NE(position, std::string::npos);
        wrong_content_types.replace(position, expected.size(),
                                    "application/xml");
        write_test_archive_entries(
            path, {{test_content_types_xml_entry, wrong_content_types},
                   {test_relationships_xml_entry, test_relationships_xml},
                   {test_document_xml_entry, valid_document_xml}});
        check_modes(path, featherdoc::document_errc::invalid_package_structure);
        fs::remove(path);
    }

    SUBCASE("malformed content types XML") {
        const auto path = fs::current_path() / "strict_bad_content_types.docx";
        write_test_archive_entries(
            path, {{test_content_types_xml_entry, "<Types>"},
                   {test_relationships_xml_entry, test_relationships_xml},
                   {test_document_xml_entry, valid_document_xml}});
        check_modes(path,
                    featherdoc::document_errc::content_types_xml_parse_failed);
        fs::remove(path);
    }
}

TEST_CASE("WordprocessingML aliases and default namespaces open save and "
          "reopen canonically") {
    namespace fs = std::filesystem;

    const auto check_roundtrip = [](const fs::path &path,
                                    std::string_view document_xml,
                                    std::string_view expected_text) {
        fs::remove(path);
        write_test_docx(path, std::string{document_xml});

        featherdoc::Document document(path);
        REQUIRE_FALSE(document.open());
        CHECK_NE(collect_document_text(document).find(expected_text),
                 std::string::npos);
        REQUIRE_FALSE(document.save());

        const auto saved_document =
            read_test_docx_entry(path, test_document_xml_entry);
        CHECK_NE(saved_document.find("<w:document"), std::string::npos);
        CHECK_NE(saved_document.find("<w:body"), std::string::npos);
        CHECK_NE(saved_document.find("<w:t"), std::string::npos);

        featherdoc::Document reopened(path);
        REQUIRE_FALSE(reopened.open());
        CHECK_NE(collect_document_text(reopened).find(expected_text),
                 std::string::npos);
        fs::remove(path);
    };

    SUBCASE("alternate prefix") {
        const auto path = fs::current_path() / "wml_alias_main.docx";
        check_roundtrip(
            path,
            std::string{"<q:document xmlns:q=\""} +
                wordprocessingml_namespace_uri +
                "\"><q:body><q:p><q:r><q:t>alias-main</q:t></q:r></q:p>"
                "</q:body></q:document>",
            "alias-main");
    }

    SUBCASE("default namespace") {
        const auto path = fs::current_path() / "wml_default_main.docx";
        check_roundtrip(path,
                        std::string{"<document xmlns=\""} +
                            wordprocessingml_namespace_uri +
                            "\"><body><p><r><t>default-main</t></r></p></body>"
                            "</document>",
                        "default-main");
    }

    SUBCASE("Chinese prefix and package path") {
        const auto path =
            fs::current_path() / fs::path{u8"WML-中文前缀文档.docx"};
        check_roundtrip(
            path,
            std::string{"<文字:document xmlns:文字=\""} +
                wordprocessingml_namespace_uri +
                "\"><文字:body><文字:p><文字:r><文字:t>中文命名空间正文"
                "</文字:t></文字:r></文字:p></文字:body></文字:document>",
            "中文命名空间正文");
    }
}

TEST_CASE("WordprocessingML aliases load across related review and lazy "
          "singleton parts") {
    namespace fs = std::filesystem;

    const auto source = fs::current_path() / "wml_alias_related_source.docx";
    const auto clean_output =
        fs::current_path() / "wml_alias_related_clean.docx";
    const auto dirty_output =
        fs::current_path() / "wml_alias_related_dirty.docx";
    fs::remove(source);
    fs::remove(clean_output);
    fs::remove(dirty_output);

    const auto wml_part = [](std::string_view root,
                             std::string_view body) -> std::string {
        return std::string{"<q:"} + std::string{root} + " xmlns:q=\"" +
               wordprocessingml_namespace_uri + "\">" + std::string{body} +
               "</q:" + std::string{root} + ">";
    };
    const auto main_document_xml =
        std::string{"<w:document xmlns:w=\""} + wordprocessingml_namespace_uri +
        "\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/"
        "relationships\"><w:body><w:p/><w:sectPr>"
        "<w:headerReference w:type=\"default\" r:id=\"rHeader\"/>"
        "<w:footerReference w:type=\"default\" r:id=\"rFooter\"/>"
        "</w:sectPr></w:body></w:document>";

    write_docx_with_related_xml_parts(
        source,
        {{"rHeader",
          header_relationship_type,
          "header1.xml",
          "/word/header1.xml",
          header_content_type,
          "word/header1.xml",
          wml_part("hdr", "<q:p><q:r><q:t>alias-header</q:t></q:r></q:p>"),
          {}},
         {"rFooter",
          footer_relationship_type,
          "footer1.xml",
          "/word/footer1.xml",
          footer_content_type,
          "word/footer1.xml",
          wml_part("ftr", "<q:p><q:r><q:t>alias-footer</q:t></q:r></q:p>"),
          {}},
         {"rSettings",
          settings_relationship_type,
          "settings.xml",
          "/word/settings.xml",
          settings_content_type,
          "word/settings.xml",
          wml_part("settings", "<q:updateFields q:val=\"true\"/>"),
          {}},
         {"rNumbering",
          numbering_relationship_type,
          "numbering.xml",
          "/word/numbering.xml",
          numbering_content_type,
          "word/numbering.xml",
          wml_part("numbering", {}),
          {}},
         {"rStyles",
          styles_relationship_type,
          "styles.xml",
          "/word/styles.xml",
          styles_content_type,
          "word/styles.xml",
          wml_part("styles",
                   "<q:style q:type=\"paragraph\" q:styleId=\"AliasStyle\">"
                   "<q:name q:val=\"Alias Style\"/></q:style>"),
          {}},
         {"rFootnotes",
          footnotes_relationship_type,
          "footnotes.xml",
          "/word/footnotes.xml",
          footnotes_content_type,
          "word/footnotes.xml",
          wml_part("footnotes",
                   "<q:footnote q:id=\"2\"><q:p><q:r><q:t>alias-footnote"
                   "</q:t></q:r></q:p></q:footnote>"),
          {}},
         {"rEndnotes",
          endnotes_relationship_type,
          "endnotes.xml",
          "/word/endnotes.xml",
          endnotes_content_type,
          "word/endnotes.xml",
          wml_part("endnotes",
                   "<q:endnote q:id=\"3\"><q:p><q:r><q:t>alias-endnote"
                   "</q:t></q:r></q:p></q:endnote>"),
          {}},
         {"rComments",
          comments_relationship_type,
          "comments.xml",
          "/word/comments.xml",
          comments_content_type,
          "word/comments.xml",
          wml_part("comments",
                   "<q:comment q:id=\"4\" q:author=\"alias-author\"><q:p>"
                   "<q:r><q:t>alias-comment</q:t></q:r></q:p></q:comment>"),
          {}},
         {"rCommentsExtended",
          comments_extended_relationship_type,
          "commentsExtended.xml",
          "/word/commentsExtended.xml",
          comments_extended_content_type,
          "word/commentsExtended.xml",
          std::string{"<w15:commentsEx xmlns:w15=\""} +
              comments_extended_namespace_uri + "\"/>",
          {}}},
        main_document_xml);

    featherdoc::Document clean_document(source);
    REQUIRE_FALSE(clean_document.open());
    CHECK_EQ(clean_document.update_fields_on_open_enabled(),
             std::optional<bool>{true});
    CHECK_EQ(clean_document.list_numbering_definitions().size(), 0U);
    REQUIRE_EQ(clean_document.list_styles().size(), 1U);
    CHECK_EQ(clean_document.list_styles().front().style_id, "AliasStyle");
    REQUIRE_EQ(clean_document.list_footnotes().size(), 1U);
    REQUIRE_EQ(clean_document.list_endnotes().size(), 1U);
    REQUIRE_EQ(clean_document.list_comments().size(), 1U);
    REQUIRE_FALSE(clean_document.save_as(clean_output));

    CHECK_NE(
        read_test_docx_entry(clean_output, "word/header1.xml").find("<w:hdr"),
        std::string::npos);
    CHECK_NE(
        read_test_docx_entry(clean_output, "word/footer1.xml").find("<w:ftr"),
        std::string::npos);
    CHECK_NE(read_test_docx_entry(clean_output, "word/settings.xml")
                 .find("<q:settings"),
             std::string::npos);
    CHECK_NE(read_test_docx_entry(clean_output, "word/commentsExtended.xml")
                 .find("<w15:commentsEx"),
             std::string::npos);

    featherdoc::Document dirty_document(source);
    REQUIRE_FALSE(dirty_document.open());
    REQUIRE(dirty_document.clear_update_fields_on_open());
    REQUIRE(dirty_document.replace_footnote(0U, "persistent-footnote"));
    REQUIRE(dirty_document.replace_endnote(0U, "persistent-endnote"));
    REQUIRE(dirty_document.replace_comment(0U, "persistent-comment"));
    REQUIRE_FALSE(dirty_document.save_as(dirty_output));
    const auto dirty_settings =
        read_test_docx_entry(dirty_output, "word/settings.xml");
    CHECK_NE(dirty_settings.find("<w:settings"), std::string::npos);
    CHECK_EQ(dirty_settings.find("<q:updateFields"), std::string::npos);
    CHECK_NE(read_test_docx_entry(dirty_output, "word/footnotes.xml")
                 .find("<w:footnotes"),
             std::string::npos);
    CHECK_NE(read_test_docx_entry(dirty_output, "word/endnotes.xml")
                 .find("<w:endnotes"),
             std::string::npos);
    CHECK_NE(read_test_docx_entry(dirty_output, "word/comments.xml")
                 .find("<w:comments"),
             std::string::npos);

    featherdoc::Document reopened(dirty_output);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(reopened.update_fields_on_open_enabled(),
             std::optional<bool>{false});
    const auto reopened_footnotes = reopened.list_footnotes();
    REQUIRE_EQ(reopened_footnotes.size(), 1U);
    CHECK_EQ(reopened_footnotes.front().text, "persistent-footnote");
    const auto reopened_endnotes = reopened.list_endnotes();
    REQUIRE_EQ(reopened_endnotes.size(), 1U);
    CHECK_EQ(reopened_endnotes.front().text, "persistent-endnote");
    const auto reopened_comments = reopened.list_comments();
    REQUIRE_EQ(reopened_comments.size(), 1U);
    CHECK_EQ(reopened_comments.front().text, "persistent-comment");

    fs::remove(source);
    fs::remove(clean_output);
    fs::remove(dirty_output);
}

TEST_CASE("WordprocessingML related parts reject unexpected roots at every "
          "loader boundary") {
    namespace fs = std::filesystem;

    struct eager_part_case final {
        std::string_view file_stem;
        std::string_view relationship_type;
        std::string_view content_type;
        std::string_view target;
        std::string_view part_name;
        std::string_view entry_name;
        std::string_view reference_name;
        std::string_view expected_root;
    };
    const std::vector<eager_part_case> eager_parts{
        {"header", header_relationship_type, header_content_type, "header1.xml",
         "/word/header1.xml", "word/header1.xml", "headerReference", "hdr"},
        {"footer", footer_relationship_type, footer_content_type, "footer1.xml",
         "/word/footer1.xml", "word/footer1.xml", "footerReference", "ftr"},
    };

    for (const auto &part : eager_parts) {
        const auto path =
            fs::current_path() /
            ("wml_wrong_" + std::string{part.file_stem} + "_root.docx");
        const auto main_document =
            std::string{"<w:document xmlns:w=\""} +
            wordprocessingml_namespace_uri +
            "\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/"
            "2006/relationships\"><w:body><w:p/><w:sectPr><w:" +
            std::string{part.reference_name} +
            " w:type=\"default\" r:id=\"rPart\"/></w:sectPr></w:body>"
            "</w:document>";
        write_docx_with_related_xml_parts(
            path,
            {{"rPart",
              std::string{part.relationship_type},
              std::string{part.target},
              std::string{part.part_name},
              std::string{part.content_type},
              std::string{part.entry_name},
              std::string{"<w:unexpected xmlns:w=\""} +
                  wordprocessingml_namespace_uri + "\"/>",
              {}}},
            main_document);

        for (const auto mode :
             {featherdoc::package_validation_mode::strict,
              featherdoc::package_validation_mode::tolerant}) {
            featherdoc::document_open_options options;
            options.validation = mode;
            featherdoc::Document document(path);
            CHECK_EQ(document.open(options),
                     featherdoc::document_errc::invalid_package_structure);
            CHECK_FALSE(document.is_open());
            CHECK_EQ(document.last_error().entry_name, part.entry_name);
            CHECK_NE(document.last_error().detail.find(part.expected_root),
                     std::string::npos);
            CHECK_NE(document.last_error().detail.find("expected_local_name="),
                     std::string::npos);
            CHECK_NE(document.last_error().detail.find("actual_local_name="),
                     std::string::npos);
        }
        fs::remove(path);
    }

    const auto header_document =
        std::string{"<w:document xmlns:w=\""} + wordprocessingml_namespace_uri +
        "\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/"
        "relationships\"><w:body><w:p/><w:sectPr><w:headerReference "
        "w:type=\"default\" r:id=\"rHeader\"/></w:sectPr></w:body>"
        "</w:document>";
    const std::vector<std::pair<std::string_view, std::string>>
        malformed_header_roots{
            {"wrong_namespace",
             "<x:hdr xmlns:x=\"urn:not-wordprocessingml\"/>"},
            {"multiple_roots", std::string{"<w:hdr xmlns:w=\""} +
                                   wordprocessingml_namespace_uri +
                                   "\"/><w:hdr xmlns:w=\"" +
                                   wordprocessingml_namespace_uri + "\"/>"},
        };
    for (const auto &root_shape : malformed_header_roots) {
        const auto path =
            fs::current_path() /
            ("wml_header_" + std::string{root_shape.first} + ".docx");
        write_docx_with_related_xml_parts(path,
                                          {{"rHeader",
                                            header_relationship_type,
                                            "header1.xml",
                                            "/word/header1.xml",
                                            header_content_type,
                                            "word/header1.xml",
                                            root_shape.second,
                                            {}}},
                                          header_document);
        for (const auto mode :
             {featherdoc::package_validation_mode::strict,
              featherdoc::package_validation_mode::tolerant}) {
            featherdoc::document_open_options options;
            options.validation = mode;
            featherdoc::Document document(path);
            CHECK_EQ(document.open(options),
                     featherdoc::document_errc::invalid_package_structure);
            CHECK_FALSE(document.is_open());
            CHECK_EQ(document.last_error().entry_name, "word/header1.xml");
            CHECK_NE(document.last_error().detail.find("hdr"),
                     std::string::npos);
            CHECK_NE(document.last_error().detail.find("expected_local_name="),
                     std::string::npos);
            CHECK_NE(document.last_error().detail.find("actual_local_name="),
                     std::string::npos);
        }
        fs::remove(path);
    }

    enum class lazy_part_action : unsigned char {
        settings,
        numbering,
        styles,
        footnotes,
        endnotes,
        comments,
    };
    struct lazy_part_case final {
        std::string_view file_stem;
        std::string_view relationship_type;
        std::string_view content_type;
        std::string_view target;
        std::string_view part_name;
        std::string_view entry_name;
        std::string_view expected_root;
        lazy_part_action action;
    };
    const std::vector<lazy_part_case> lazy_parts{
        {"settings", settings_relationship_type, settings_content_type,
         "settings.xml", "/word/settings.xml", "word/settings.xml", "settings",
         lazy_part_action::settings},
        {"numbering", numbering_relationship_type, numbering_content_type,
         "numbering.xml", "/word/numbering.xml", "word/numbering.xml",
         "numbering", lazy_part_action::numbering},
        {"styles", styles_relationship_type, styles_content_type, "styles.xml",
         "/word/styles.xml", "word/styles.xml", "styles",
         lazy_part_action::styles},
        {"footnotes", footnotes_relationship_type, footnotes_content_type,
         "footnotes.xml", "/word/footnotes.xml", "word/footnotes.xml",
         "footnotes", lazy_part_action::footnotes},
        {"endnotes", endnotes_relationship_type, endnotes_content_type,
         "endnotes.xml", "/word/endnotes.xml", "word/endnotes.xml", "endnotes",
         lazy_part_action::endnotes},
        {"comments", comments_relationship_type, comments_content_type,
         "comments.xml", "/word/comments.xml", "word/comments.xml", "comments",
         lazy_part_action::comments},
    };

    for (const auto &part : lazy_parts) {
        const auto path =
            fs::current_path() /
            ("wml_wrong_" + std::string{part.file_stem} + "_root.docx");
        write_docx_with_related_xml_parts(
            path, {{"rPart",
                    std::string{part.relationship_type},
                    std::string{part.target},
                    std::string{part.part_name},
                    std::string{part.content_type},
                    std::string{part.entry_name},
                    std::string{"<w:unexpected xmlns:w=\""} +
                        wordprocessingml_namespace_uri + "\"/>",
                    {}}});

        for (const auto mode :
             {featherdoc::package_validation_mode::strict,
              featherdoc::package_validation_mode::tolerant}) {
            featherdoc::document_open_options options;
            options.validation = mode;
            featherdoc::Document document(path);
            REQUIRE_FALSE(document.open(options));
            switch (part.action) {
            case lazy_part_action::settings:
                CHECK_FALSE(
                    document.update_fields_on_open_enabled().has_value());
                break;
            case lazy_part_action::numbering:
                CHECK(document.list_numbering_definitions().empty());
                break;
            case lazy_part_action::styles:
                CHECK(document.list_styles().empty());
                break;
            case lazy_part_action::footnotes:
                CHECK(document.list_footnotes().empty());
                break;
            case lazy_part_action::endnotes:
                CHECK(document.list_endnotes().empty());
                break;
            case lazy_part_action::comments:
                CHECK(document.list_comments().empty());
                break;
            }
            CHECK(document.is_open());
            CHECK_EQ(document.last_error().code,
                     featherdoc::document_errc::invalid_package_structure);
            CHECK_EQ(document.last_error().entry_name, part.entry_name);
            CHECK_NE(document.last_error().detail.find(part.expected_root),
                     std::string::npos);
            CHECK_NE(document.last_error().detail.find("expected_local_name="),
                     std::string::npos);
            CHECK_NE(document.last_error().detail.find("actual_local_name="),
                     std::string::npos);
        }
        fs::remove(path);
    }

    const auto empty_comments_path =
        fs::current_path() / "wml_empty_comments_part.docx";
    write_docx_with_related_xml_parts(empty_comments_path,
                                      {{"rComments",
                                        comments_relationship_type,
                                        "comments.xml",
                                        "/word/comments.xml",
                                        comments_content_type,
                                        "word/comments.xml",
                                        {},
                                        {}}});
    for (const auto mode : {featherdoc::package_validation_mode::strict,
                            featherdoc::package_validation_mode::tolerant}) {
        featherdoc::document_open_options options;
        options.validation = mode;
        featherdoc::Document document(empty_comments_path);
        REQUIRE_FALSE(document.open(options));
        CHECK(document.list_comments().empty());
        CHECK_EQ(document.last_error().code,
                 featherdoc::document_errc::invalid_package_structure);
        CHECK_EQ(document.last_error().entry_name, "word/comments.xml");
    }
    fs::remove(empty_comments_path);
}

TEST_CASE("tolerant open reports repairable and unsafe package diagnostics") {
    namespace fs = std::filesystem;

    SUBCASE("missing deterministic OPC parts are repairable") {
        const auto path =
            fs::current_path() / "tolerant_repairable_diagnostics.docx";
        write_test_archive_entries(
            path,
            {{test_document_xml_entry,
              R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"/>)"}});

        featherdoc::document_open_options options;
        options.validation = featherdoc::package_validation_mode::tolerant;
        featherdoc::Document document(path);
        REQUIRE_FALSE(document.open(options));

        const auto &diagnostics = document.package_diagnostics();
        REQUIRE_EQ(diagnostics.size(), 3U);
        CHECK(std::ranges::all_of(diagnostics, [](const auto &diagnostic) {
            return diagnostic.repairable;
        }));
        CHECK(std::ranges::any_of(diagnostics, [](const auto &diagnostic) {
            return diagnostic.code ==
                   featherdoc::package_diagnostic_code::missing_document_body;
        }));
        CHECK(std::ranges::any_of(diagnostics, [](const auto &diagnostic) {
            return diagnostic.code == featherdoc::package_diagnostic_code::
                                          missing_root_relationships;
        }));
        CHECK(std::ranges::any_of(diagnostics, [](const auto &diagnostic) {
            return diagnostic.code ==
                   featherdoc::package_diagnostic_code::missing_content_types;
        }));
        fs::remove(path);
    }

    SUBCASE("invalid namespace and malformed metadata are unsafe") {
        const auto path =
            fs::current_path() / "tolerant_unsafe_diagnostics.docx";
        write_test_archive_entries(
            path,
            {{test_content_types_xml_entry, "<Types>"},
             {test_relationships_xml_entry, "<Relationships>"},
             {test_document_xml_entry,
              R"(<x:document xmlns:x="urn:not-wordprocessingml"><x:body/></x:document>)"}});

        featherdoc::document_open_options options;
        options.validation = featherdoc::package_validation_mode::tolerant;
        featherdoc::Document document(path);
        REQUIRE_FALSE(document.open(options));

        const auto &diagnostics = document.package_diagnostics();
        REQUIRE_EQ(diagnostics.size(), 3U);
        CHECK(std::ranges::none_of(diagnostics, [](const auto &diagnostic) {
            return diagnostic.repairable;
        }));
        fs::remove(path);
    }
}

TEST_CASE("explicit package repair produces a strict-valid package without "
          "data loss") {
    namespace fs = std::filesystem;
    const auto source = fs::current_path() / "repair_source.docx";
    const auto repaired = fs::current_path() / "repair_output.docx";
    constexpr auto custom_entry = "customXml/item1.xml";
    constexpr auto custom_payload = "<custom>preserve-me</custom>";
    write_test_archive_entries(
        source,
        {{test_document_xml_entry,
          R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"/>)"},
         {custom_entry, custom_payload}});

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open(options));

    const auto report = document.repair_package();
    REQUIRE(report.has_value());
    CHECK_EQ(report->diagnostics_before.size(), 3U);
    CHECK_EQ(report->actions.size(), 3U);
    CHECK(report->changed());
    CHECK(document.package_diagnostics().empty());

    const auto second_report = document.repair_package();
    REQUIRE(second_report.has_value());
    CHECK_FALSE(second_report->changed());

    REQUIRE(document.paragraphs().add_run("repaired").has_next());
    REQUIRE_FALSE(document.save_as(repaired));

    featherdoc::Document strict_document(repaired);
    REQUIRE_FALSE(strict_document.open());
    CHECK_NE(collect_document_text(strict_document).find("repaired"),
             std::string::npos);
    CHECK_EQ(read_test_docx_entry(repaired, custom_entry), custom_payload);

    fs::remove(source);
    fs::remove(repaired);
}

TEST_CASE("package repair preserves prefixed relationship QNames") {
    namespace fs = std::filesystem;
    const auto source =
        fs::current_path() / "repair_prefixed_relationships_source.docx";
    const auto repaired =
        fs::current_path() / "repair_prefixed_relationships_output.docx";
    const auto prefixed_relationships = std::string{
        R"(<opc:Relationships xmlns:opc="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:item="http://schemas.openxmlformats.org/package/2006/relationships"><item:Relationship Id="rCustom" Type="urn:example:custom" Target="customXml/item1.xml"/></opc:Relationships>)"};

    write_test_archive_entries(
        source, {{test_content_types_xml_entry, test_content_types_xml},
                 {test_relationships_xml_entry, prefixed_relationships},
                 {test_document_xml_entry, valid_document_xml},
                 {"customXml/item1.xml", "<custom>preserve</custom>"}});

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open(options));
    REQUIRE_EQ(document.package_diagnostics().size(), 1U);
    CHECK_EQ(document.package_diagnostics().front().code,
             featherdoc::package_diagnostic_code::
                 missing_main_document_relationship);

    const auto report = document.repair_package();
    REQUIRE(report.has_value());
    REQUIRE_EQ(report->actions.size(), 1U);
    REQUIRE_FALSE(document.save_as(repaired));

    const auto saved_relationships =
        read_test_docx_entry(repaired, test_relationships_xml_entry);
    CHECK_NE(saved_relationships.find("<opc:Relationships"), std::string::npos);
    CHECK_NE(saved_relationships.find("<item:Relationship"), std::string::npos);
    CHECK_NE(saved_relationships.find("<opc:Relationship"), std::string::npos);
    CHECK_EQ(saved_relationships.find("<Relationship "), std::string::npos);
    CHECK_NE(saved_relationships.find(office_document_relationship_type),
             std::string::npos);
    CHECK_NE(saved_relationships.find("urn:example:custom"), std::string::npos);

    featherdoc::Document strict_document(repaired);
    REQUIRE_FALSE(strict_document.open());
    CHECK_EQ(read_test_docx_entry(repaired, "customXml/item1.xml"),
             "<custom>preserve</custom>");

    fs::remove(source);
    fs::remove(repaired);
}

TEST_CASE("package repair preserves valid OPC metadata while correcting main "
          "entries") {
    namespace fs = std::filesystem;
    const auto source = fs::current_path() / "repair_metadata_source.docx";
    const auto repaired = fs::current_path() / "repair_metadata_output.docx";
    const auto content_types =
        std::string{R"(<?xml version="1.0" encoding="UTF-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/xml"/>
  <Override PartName="/customXml/item1.xml" ContentType="application/vnd.example.custom+xml"/>
</Types>)"};
    const auto relationships =
        std::string{R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/wrong.xml"/>
  <Relationship Id="rId9" Type="urn:example:custom" Target="customXml/item1.xml"/>
</Relationships>)"};
    write_test_archive_entries(source,
                               {{test_content_types_xml_entry, content_types},
                                {test_relationships_xml_entry, relationships},
                                {test_document_xml_entry, valid_document_xml},
                                {"customXml/item1.xml", "<custom/>"}});

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open(options));
    const auto paragraph_before_repair = document.paragraphs();
    REQUIRE(paragraph_before_repair.valid());
    const auto report = document.repair_package();
    REQUIRE(report.has_value());
    CHECK_EQ(report->actions.size(), 2U);
    CHECK_FALSE(paragraph_before_repair.valid());
    CHECK_FALSE(paragraph_before_repair.set_text("stale repair handle"));
    CHECK(document.paragraphs().valid());
    REQUIRE_FALSE(document.save_as(repaired));

    featherdoc::Document strict_document(repaired);
    REQUIRE_FALSE(strict_document.open());
    const auto repaired_content_types =
        read_test_docx_entry(repaired, test_content_types_xml_entry);
    const auto repaired_relationships =
        read_test_docx_entry(repaired, test_relationships_xml_entry);
    CHECK_NE(repaired_content_types.find("application/vnd.example.custom+xml"),
             std::string::npos);
    CHECK_NE(repaired_relationships.find("urn:example:custom"),
             std::string::npos);

    fs::remove(source);
    fs::remove(repaired);
}

TEST_CASE("unsafe package repair is rejected atomically") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "repair_unsafe_atomic.docx";
    write_test_archive_entries(
        path,
        {{test_content_types_xml_entry, "<Types>"},
         {test_relationships_xml_entry, test_relationships_xml},
         {test_document_xml_entry,
          R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"/>)"}});

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open(options));
    const auto diagnostics_before = document.package_diagnostics();

    const auto report = document.repair_package();
    CHECK_FALSE(report.has_value());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::package_repair_not_possible);
    CHECK_EQ(document.package_diagnostics().size(), diagnostics_before.size());
    CHECK_FALSE(document.paragraphs().add_run("must-not-be-added").has_next());

    fs::remove(path);
}

TEST_CASE("package repair options reject disabled changes atomically") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "repair_options_atomic.docx";
    write_test_archive_entries(path,
                               {{test_document_xml_entry, valid_document_xml}});

    featherdoc::document_open_options open_options;
    open_options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open(open_options));
    REQUIRE_EQ(document.package_diagnostics().size(), 2U);

    featherdoc::document_repair_options repair_options;
    repair_options.repair_root_relationships = false;
    const auto report = document.repair_package(repair_options);
    CHECK_FALSE(report.has_value());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::package_repair_not_possible);
    CHECK_EQ(document.package_diagnostics().size(), 2U);

    fs::remove(path);
}

TEST_CASE("duplicate main OPC declarations are diagnosed as unsafe") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "repair_ambiguous_main_parts.docx";
    const auto duplicate_relationships = std::string{R"(
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/other.xml"/>
</Relationships>)"};
    const auto duplicate_content_types = std::string{R"(
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/document.xml" ContentType="application/xml"/>
</Types>)"};
    write_test_archive_entries(
        path, {{test_content_types_xml_entry, duplicate_content_types},
               {test_relationships_xml_entry, duplicate_relationships},
               {test_document_xml_entry, valid_document_xml}});

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(path);
    REQUIRE(document.open(options));
    CHECK_FALSE(document.is_open());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::invalid_package_structure);
    CHECK_EQ(document.last_error().entry_name, test_content_types_xml_entry);
    CHECK_FALSE(document.repair_package().has_value());

    fs::remove(path);
}

TEST_CASE("logical duplicate content type overrides fail closed") {
    namespace fs = std::filesystem;
    const auto path =
        fs::current_path() / "duplicate_logical_content_type_overrides.docx";
    const auto duplicate_content_types = std::string{R"(
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/customXml/项目.xml" ContentType="application/xml"/>
  <Override PartName="/CUSTOMXML/%e9%a1%b9%e7%9b%ae.XML" ContentType="application/vnd.example.custom+xml"/>
</Types>)"};
    write_test_archive_entries(
        path, {{test_content_types_xml_entry, duplicate_content_types},
               {test_relationships_xml_entry, test_relationships_xml},
               {test_document_xml_entry, valid_document_xml}});

    for (const auto validation_mode :
         {featherdoc::package_validation_mode::strict,
          featherdoc::package_validation_mode::tolerant}) {
        featherdoc::document_open_options options;
        options.validation = validation_mode;
        featherdoc::Document document(path);
        REQUIRE(document.open(options));
        CHECK_FALSE(document.is_open());
        CHECK_EQ(document.last_error().code,
                 featherdoc::document_errc::invalid_package_structure);
        CHECK_EQ(document.last_error().entry_name,
                 test_content_types_xml_entry);
        CHECK_NE(document.last_error().detail.find(
                     "duplicate logical Override PartName"),
                 std::string::npos);
    }

    fs::remove(path);
}

TEST_CASE("invalid content type Override PartName values fail closed") {
    namespace fs = std::filesystem;
    const auto path =
        fs::current_path() / "invalid_content_type_override_part_name.docx";
    const auto invalid_content_types = std::string{R"(
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="customXml/item1.xml" ContentType="application/xml"/>
</Types>)"};
    write_test_archive_entries(
        path, {{test_content_types_xml_entry, invalid_content_types},
               {test_relationships_xml_entry, test_relationships_xml},
               {test_document_xml_entry, valid_document_xml}});

    for (const auto validation_mode :
         {featherdoc::package_validation_mode::strict,
          featherdoc::package_validation_mode::tolerant}) {
        featherdoc::document_open_options options;
        options.validation = validation_mode;
        featherdoc::Document document(path);
        REQUIRE(document.open(options));
        CHECK_FALSE(document.is_open());
        CHECK_EQ(document.last_error().code,
                 featherdoc::document_errc::invalid_package_structure);
        CHECK_NE(document.last_error().detail.find("invalid Override PartName"),
                 std::string::npos);
    }

    fs::remove(path);
}

TEST_CASE("content type Default Extension declarations are validated by OPC "
          "identity") {
    namespace fs = std::filesystem;
    const auto path =
        fs::current_path() / "content_type_default_extension_validation.docx";
    const auto make_content_types = [](std::string_view defaults) {
        return std::string{
                   R"(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
)"} + std::string{defaults} +
               R"(
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>)";
    };
    const auto check_rejected_in_all_modes =
        [&](const std::string &content_types,
            std::string_view expected_detail) {
            write_test_archive_entries(
                path, {{test_content_types_xml_entry, content_types},
                       {test_relationships_xml_entry, test_relationships_xml},
                       {test_document_xml_entry, valid_document_xml}});

            for (const auto validation_mode :
                 {featherdoc::package_validation_mode::strict,
                  featherdoc::package_validation_mode::tolerant}) {
                featherdoc::document_open_options options;
                options.validation = validation_mode;
                featherdoc::Document document(path);
                REQUIRE(document.open(options));
                CHECK_FALSE(document.is_open());
                CHECK_EQ(document.last_error().code,
                         featherdoc::document_errc::invalid_package_structure);
                CHECK_EQ(document.last_error().entry_name,
                         test_content_types_xml_entry);
                CHECK_NE(document.last_error().detail.find(expected_detail),
                         std::string::npos);
            }
        };

    SUBCASE("ASCII case variants are duplicate logical extensions") {
        check_rejected_in_all_modes(
            make_content_types(
                R"(  <Default Extension="PNG" ContentType="image/png"/>
  <Default Extension="png" ContentType="application/octet-stream"/>)"),
            "duplicate logical Default Extension");
    }

    SUBCASE("invalid extension syntax fails closed") {
        std::vector<std::pair<std::string, std::string>> invalid_extensions{
            {"empty", {}},
            {"leading period", ".xml"},
            {"embedded period", "tar.gz"},
            {"slash", "a/b"},
            {"backslash", R"(a\b)"},
            {"control", "a&#x7F;b"},
            {"percent-encoded slash", "a%2Fb"},
            {"percent-encoded backslash", "a%5Cb"},
            {"invalid percent escape", "a%GG"},
            {"character outside ST_Extension", "a;b"},
            {"raw non-ASCII", "项目"},
            {"raw emoji", "🙂"},
        };
        auto invalid_utf8 = std::string{"invalid-"};
        invalid_utf8.push_back(static_cast<char>(0xC3U));
        invalid_utf8.push_back('(');
        invalid_extensions.emplace_back("invalid UTF-8", invalid_utf8);

        for (const auto &invalid_extension : invalid_extensions) {
            INFO(invalid_extension.first);
            const auto &extension = invalid_extension.second;
            check_rejected_in_all_modes(
                make_content_types("  <Default Extension=\"" + extension +
                                   "\" ContentType=\"application/xml\"/>"),
                "invalid Default Extension");
        }
    }

    SUBCASE("valid URI punctuation and Unicode spellings remain accepted") {
        const auto valid_content_types = make_content_types(
            R"(  <Default Extension="a!$&amp;'()*+,:=@-_~" ContentType="application/octet-stream"/>
  <Default Extension="%E9%A1%B9%E7%9B%AE" ContentType="application/vnd.example.chinese+xml"/>
  <Default Extension="%F0%9F%99%82" ContentType="application/vnd.example.emoji+xml"/>)");
        write_test_archive_entries(
            path, {{test_content_types_xml_entry, valid_content_types},
                   {test_relationships_xml_entry, test_relationships_xml},
                   {test_document_xml_entry, valid_document_xml}});

        featherdoc::Document document(path);
        CHECK_FALSE(document.open());
        CHECK(document.is_open());
    }

    fs::remove(path);
}

TEST_CASE("content type media-type declarations are validated before use") {
    namespace fs = std::filesystem;
    const auto path =
        fs::current_path() / "content_type_media_type_validation.docx";
    const auto make_content_types = [](std::string_view declarations) {
        return std::string{
                   R"(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
)"} + std::string{declarations} +
               R"(
</Types>)";
    };
    const auto check_rejected_in_all_modes =
        [&](const std::string &content_types,
            std::string_view expected_detail) {
            write_test_archive_entries(
                path, {{test_content_types_xml_entry, content_types},
                       {test_relationships_xml_entry, test_relationships_xml},
                       {test_document_xml_entry, valid_document_xml}});

            for (const auto validation_mode :
                 {featherdoc::package_validation_mode::strict,
                  featherdoc::package_validation_mode::tolerant}) {
                featherdoc::document_open_options options;
                options.validation = validation_mode;
                featherdoc::Document document(path);
                REQUIRE(document.open(options));
                CHECK_FALSE(document.is_open());
                CHECK_EQ(document.last_error().code,
                         featherdoc::document_errc::invalid_package_structure);
                CHECK_EQ(document.last_error().entry_name,
                         test_content_types_xml_entry);
                CHECK_NE(document.last_error().detail.find(expected_detail),
                         std::string::npos);
            }
        };
    constexpr auto valid_main_override =
        R"(  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>)";

    SUBCASE("missing and malformed Default media types fail closed") {
        const std::vector<std::pair<std::string, std::string>> declarations{
            {"missing", R"(  <Default Extension="xml"/>
)" + std::string{valid_main_override}},
            {"empty", R"(  <Default Extension="xml" ContentType=""/>
)" + std::string{valid_main_override}},
            {"missing subtype",
             R"(  <Default Extension="xml" ContentType="application"/>
)" + std::string{valid_main_override}},
            {"incomplete parameter",
             R"(  <Default Extension="xml" ContentType="application/xml; charset"/>
)" + std::string{valid_main_override}},
            {"non-ASCII token",
             R"(  <Default Extension="xml" ContentType="application/项目"/>
)" + std::string{valid_main_override}},
            {"control character",
             R"(  <Default Extension="xml" ContentType="application/xml; note=&quot;a&#x85;b&quot;"/>
)" + std::string{valid_main_override}},
        };
        for (const auto &declaration_case : declarations) {
            INFO(declaration_case.first);
            const auto &declaration = declaration_case.second;
            check_rejected_in_all_modes(make_content_types(declaration),
                                        "Default");
        }
    }

    SUBCASE("missing and malformed Override media types fail closed") {
        check_rejected_in_all_modes(
            make_content_types(std::string{valid_main_override} +
                               R"(
  <Override PartName="/customXml/item1.xml"/>)"),
            "Override");
        check_rejected_in_all_modes(
            make_content_types(std::string{valid_main_override} +
                               R"(
  <Override PartName="/customXml/item1.xml" ContentType="application/xml; title=&quot;中&quot;"/>)"),
            "Override");
    }

    SUBCASE("ASCII case variants and valid quoted Latin-1 remain accepted") {
        const auto content_types = make_content_types(
            R"(  <Default Extension="xml" ContentType="application/xml; title=&quot;café&quot;"/>
  <Override PartName="/word/document.xml" ContentType="Application/Vnd.OpenXmlFormats-Officedocument.Wordprocessingml.Document.Main+Xml"/>)");
        write_test_archive_entries(
            path, {{test_content_types_xml_entry, content_types},
                   {test_relationships_xml_entry, test_relationships_xml},
                   {test_document_xml_entry, valid_document_xml}});

        featherdoc::Document document(path);
        CHECK_FALSE(document.open());
        CHECK(document.is_open());
    }

    fs::remove(path);
}

TEST_CASE(
    "strict open accepts a normalized absolute root relationship target") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "strict_normalized_root_target.docx";
    const auto relationships =
        std::string{R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="/word/./document.xml"/>
</Relationships>)"};
    write_test_archive_entries(
        path, {{test_content_types_xml_entry, test_content_types_xml},
               {test_relationships_xml_entry, relationships},
               {test_document_xml_entry, valid_document_xml}});

    featherdoc::Document document(path);
    CHECK_FALSE(document.open());
    CHECK(document.is_open());
    fs::remove(path);
}

TEST_CASE("case-varied main part identity opens and saves canonically") {
    namespace fs = std::filesystem;
    const auto source =
        fs::current_path() / "case_varied_main_part_source.docx";
    const auto output =
        fs::current_path() / "case_varied_main_part_output.docx";
    constexpr auto case_varied_document_entry = "WORD/DOCUMENT.XML";
    constexpr auto content_types = R"(
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/WORD/DOCUMENT.XML" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>)";
    constexpr auto relationships = R"(
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="WORD/DOCUMENT.XML"/>
</Relationships>)";
    write_test_archive_entries(
        source, {{test_content_types_xml_entry, content_types},
                 {test_relationships_xml_entry, relationships},
                 {case_varied_document_entry, valid_document_xml}});

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE_FALSE(document.save_as(output));

    const auto saved_entries = read_test_archive_entries(output);
    CHECK(std::ranges::any_of(saved_entries, [](const auto &entry) {
        return entry.first == test_document_xml_entry;
    }));
    CHECK_FALSE(std::ranges::any_of(saved_entries, [&](const auto &entry) {
        return entry.first == case_varied_document_entry;
    }));

    featherdoc::Document reopened(output);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.is_open());

    fs::remove(source);
    fs::remove(output);
}

TEST_CASE(
    "strict open rejects non-canonical or unsafe root relationship targets") {
    namespace fs = std::filesystem;

    struct invalid_target_case {
        std::string_view label;
        std::string_view target;
    };
    constexpr invalid_target_case invalid_targets[]{
        {"URI authority", "//word/document.xml"},
        {"empty path segment", "word//document.xml"},
        {"trailing directory", "word/document.xml/"},
        {"backslash separator", R"(word\document.xml)"},
        {"absolute URI", "https://example.invalid/document.xml"},
        {"query", "word/document.xml?version=1"},
        {"fragment", "word/document.xml#main"},
        {"invalid percent escape", "word/%GGdocument.xml"},
        {"percent-encoded slash", "word%2Fdocument.xml"},
        {"percent-encoded dot-dot", "%2E%2E/word/document.xml"},
    };

    const auto path = fs::current_path() / "strict_unsafe_root_target.docx";
    for (const auto &test_case : invalid_targets) {
        INFO(test_case.label);
        const auto relationships =
            std::string{R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target=")"} +
            std::string{test_case.target} +
            R"("/>
</Relationships>)";
        write_test_archive_entries(
            path, {{test_content_types_xml_entry, test_content_types_xml},
                   {test_relationships_xml_entry, relationships},
                   {test_document_xml_entry, valid_document_xml}});

        featherdoc::Document document(path);
        CHECK_EQ(document.open(),
                 featherdoc::document_errc::invalid_package_structure);
        CHECK_FALSE(document.is_open());
        CHECK_EQ(document.last_error().entry_name,
                 std::string_view{test_relationships_xml_entry});
        fs::remove(path);
    }
}

TEST_CASE(
    "header and footer relationships must be valid internal package targets") {
    namespace fs = std::filesystem;

    SUBCASE("header relationship rejects TargetMode External") {
        const auto path = fs::current_path() / "external_header_target.docx";
        write_docx_with_related_xml_parts(
            path, {{"rIdHeader", header_relationship_type,
                    "https://example.invalid/header.xml", "/word/header1.xml",
                    header_content_type, "word/header1.xml",
                    minimal_xml_payload(namespaced_word_root_prefix("w:hdr"),
                                        namespaced_word_root_suffix("w:hdr")),
                    "External"}});

        featherdoc::Document document(path);
        CHECK_EQ(document.open(),
                 featherdoc::document_errc::invalid_package_structure);
        CHECK_FALSE(document.is_open());
        CHECK_EQ(document.last_error().entry_name,
                 "word/_rels/document.xml.rels");
        fs::remove(path);
    }

    SUBCASE("footer relationship rejects a malformed internal target") {
        const auto path = fs::current_path() / "invalid_footer_target.docx";
        write_docx_with_related_xml_parts(
            path,
            {{"rIdFooter", footer_relationship_type, "footer//footer1.xml",
              "/word/footer1.xml", footer_content_type, "word/footer1.xml",
              minimal_xml_payload(namespaced_word_root_prefix("w:ftr"),
                                  namespaced_word_root_suffix("w:ftr"))}});

        featherdoc::Document document(path);
        CHECK_EQ(document.open(),
                 featherdoc::document_errc::invalid_package_structure);
        CHECK_FALSE(document.is_open());
        CHECK_EQ(document.last_error().entry_name,
                 "word/_rels/document.xml.rels");
        fs::remove(path);
    }
}

TEST_CASE("single case-varied required OPC entry remains accepted") {
    namespace fs = std::filesystem;
    constexpr auto confused_document_entry = "WORD/DOCUMENT.XML";
    const auto path =
        fs::current_path() / "single_case_varied_document_part.docx";

    write_test_archive_entries(
        path, {{test_content_types_xml_entry, test_content_types_xml},
               {test_relationships_xml_entry, test_relationships_xml},
               {confused_document_entry, valid_document_xml}});

    featherdoc::Document strict_document(path);
    CHECK_FALSE(strict_document.open());
    CHECK(strict_document.is_open());

    featherdoc::document_open_options tolerant_options;
    tolerant_options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document tolerant_document(path);
    CHECK_FALSE(tolerant_document.open(tolerant_options));
    CHECK(tolerant_document.is_open());

    fs::remove(path);
}

TEST_CASE(
    "unsafe physical ZIP part names are rejected in every validation mode") {
    namespace fs = std::filesystem;

    const auto check_rejected_in_all_modes =
        [](const fs::path &path, std::string_view expected_entry_name) {
            const auto check_mode =
                [&](featherdoc::package_validation_mode mode) {
                    featherdoc::document_open_options options;
                    options.validation = mode;

                    featherdoc::Document document(path);
                    CHECK_EQ(
                        document.open(options),
                        featherdoc::document_errc::invalid_package_structure);
                    CHECK_FALSE(document.is_open());
                    CHECK_EQ(document.last_error().entry_name,
                             expected_entry_name);
                };

            check_mode(featherdoc::package_validation_mode::strict);
            check_mode(featherdoc::package_validation_mode::tolerant);
        };

    SUBCASE("duplicate main document entry") {
        const auto path = fs::current_path() / "duplicate_document_part.docx";
        write_test_archive_entries(
            path,
            {{test_content_types_xml_entry, test_content_types_xml},
             {test_relationships_xml_entry, test_relationships_xml},
             {test_document_xml_entry, valid_document_xml},
             {test_document_xml_entry,
              R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:t>duplicate</w:t></w:r></w:p></w:body></w:document>)"}});

        check_rejected_in_all_modes(path, test_document_xml_entry);
        fs::remove(path);
    }

    SUBCASE("parent traversal entry") {
        const auto path = fs::current_path() / "traversal_part_name.docx";
        const auto output =
            fs::current_path() / "traversal_part_name_output.docx";
        fs::remove(output);
        write_test_archive_entries(
            path, {{test_content_types_xml_entry, test_content_types_xml},
                   {test_relationships_xml_entry, test_relationships_xml},
                   {test_document_xml_entry, valid_document_xml},
                   {"../escape.bin", "must-not-be-propagated"}});

        check_rejected_in_all_modes(path, "../escape.bin");

        featherdoc::Document document(path);
        CHECK_EQ(document.open(),
                 featherdoc::document_errc::invalid_package_structure);
        CHECK_EQ(document.save_as(output),
                 featherdoc::document_errc::document_not_open);
        CHECK_FALSE(fs::exists(output));

        fs::remove(path);
        fs::remove(output);
    }

    SUBCASE("non-pchar package entry") {
        constexpr auto invalid_entry = "word/name[1].xml";
        const auto path = fs::current_path() / "non_pchar_part_name.docx";
        write_test_archive_entries(
            path, {{test_content_types_xml_entry, test_content_types_xml},
                   {test_relationships_xml_entry, test_relationships_xml},
                   {test_document_xml_entry, valid_document_xml},
                   {invalid_entry, "<invalid/>"}});

        check_rejected_in_all_modes(path, invalid_entry);
        fs::remove(path);
    }

    SUBCASE("dot-terminated package segment") {
        constexpr auto invalid_entry = "word/name./part.xml";
        const auto path = fs::current_path() / "dot_terminated_part_name.docx";
        write_test_archive_entries(
            path, {{test_content_types_xml_entry, test_content_types_xml},
                   {test_relationships_xml_entry, test_relationships_xml},
                   {test_document_xml_entry, valid_document_xml},
                   {invalid_entry, "<invalid/>"}});

        check_rejected_in_all_modes(path, invalid_entry);
        fs::remove(path);
    }

    SUBCASE("dot-only package segment") {
        constexpr auto invalid_entry = "word/.../part.xml";
        const auto path = fs::current_path() / "dot_only_part_name.docx";
        write_test_archive_entries(
            path, {{test_content_types_xml_entry, test_content_types_xml},
                   {test_relationships_xml_entry, test_relationships_xml},
                   {test_document_xml_entry, valid_document_xml},
                   {invalid_entry, "<invalid/>"}});

        check_rejected_in_all_modes(path, invalid_entry);
        fs::remove(path);
    }

    SUBCASE("part name derived by appending a segment") {
        constexpr auto conflicting_entry = "word";
        const auto path = fs::current_path() / "derived_part_name.docx";
        write_test_archive_entries(
            path, {{test_content_types_xml_entry, test_content_types_xml},
                   {test_relationships_xml_entry, test_relationships_xml},
                   {test_document_xml_entry, valid_document_xml},
                   {conflicting_entry, "conflict"}});

        check_rejected_in_all_modes(path, conflicting_entry);
        fs::remove(path);
    }

    SUBCASE("case-confusable related entries") {
        constexpr auto confused_header_entry = "word/Header1.xml";
        const auto path =
            fs::current_path() / "case_confusable_header_part.docx";
        const auto document_relationships = std::string{R"(
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header" Target="header1.xml"/>
</Relationships>)"};
        constexpr auto header_xml = R"(
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>header</w:t></w:r></w:p>
</w:hdr>)";
        write_test_archive_entries(
            path, {{test_content_types_xml_entry, test_content_types_xml},
                   {test_relationships_xml_entry, test_relationships_xml},
                   {test_document_xml_entry, valid_document_xml},
                   {"word/_rels/document.xml.rels", document_relationships},
                   {"word/header1.xml", header_xml},
                   {confused_header_entry, header_xml}});

        check_rejected_in_all_modes(path, confused_header_entry);
        fs::remove(path);
    }
}

TEST_CASE("saving a case-varied related part keeps only the canonical "
          "regenerated entry") {
    namespace fs = std::filesystem;

    const auto source = fs::current_path() / "case_varied_header_source.docx";
    const auto output = fs::current_path() / "case_varied_header_output.docx";
    fs::remove(source);
    fs::remove(output);

    const auto document_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>body</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rIdHeader"/>
    </w:sectPr>
  </w:body>
</w:document>)"};
    const auto content_types_xml =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
</Types>)"};
    const auto document_relationships =
        std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rIdHeader"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
</Relationships>)"};
    constexpr auto header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>header</w:t></w:r></w:p>
</w:hdr>)";

    write_test_archive_entries(
        source, {{test_content_types_xml_entry, content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, document_xml},
                 {"word/_rels/document.xml.rels", document_relationships},
                 {"word/Header1.xml", header_xml}});

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.is_open());
    REQUIRE_FALSE(document.save_as(output));

    const auto entries = read_test_archive_entries(output);
    CHECK_EQ(archive_entries_count_ascii_folded(entries, "word/header1.xml"),
             1U);
    CHECK(archive_entries_contain_exact(entries, "word/header1.xml"));
    CHECK_FALSE(archive_entries_contain_exact(entries, "word/Header1.xml"));

    featherdoc::Document reopened(output);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.is_open());

    fs::remove(source);
    fs::remove(output);
}

TEST_CASE("canonical ZIP directory entries remain accepted") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "canonical_directory_entries.docx";
    write_test_archive_entries(
        path, {{"_rels/", {}},
               {"word/", {}},
               {"word/media/", {}},
               {"customXml/", {}},
               {test_content_types_xml_entry, test_content_types_xml},
               {test_relationships_xml_entry, test_relationships_xml},
               {test_document_xml_entry, valid_document_xml}});

    featherdoc::Document strict_document(path);
    CHECK_FALSE(strict_document.open());
    CHECK(strict_document.is_open());

    featherdoc::document_open_options tolerant_options;
    tolerant_options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document tolerant_document(path);
    CHECK_FALSE(tolerant_document.open(tolerant_options));
    CHECK(tolerant_document.is_open());

    fs::remove(path);
}

TEST_CASE("OPC pchar colon and encoded query delimiters remain accepted") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "valid_opc_pchar_entries.docx";
    write_test_archive_entries(
        path, {{test_content_types_xml_entry, test_content_types_xml},
               {test_relationships_xml_entry, test_relationships_xml},
               {test_document_xml_entry, valid_document_xml},
               {"C:foo", "colon"},
               {"word/name%23query%3F.xml", "encoded"}});

    featherdoc::Document document(path);
    CHECK_FALSE(document.open());
    CHECK(document.is_open());

    fs::remove(path);
}

TEST_CASE("miniz rejects ZIP64 compressed range overflow before extraction") {
    const auto archive = zip64_compressed_range_overflow_archive_for_test();

    mz_zip_archive zip{};
    REQUIRE(mz_zip_reader_init_mem(&zip, archive.data(), archive.size(), 0) ==
            MZ_TRUE);

    mz_zip_archive_file_stat stat{};
    REQUIRE(mz_zip_reader_file_stat(&zip, 0U, &stat) == MZ_TRUE);
    CHECK_EQ(stat.m_uncomp_size, 1U);
    CHECK_GT(stat.m_comp_size, static_cast<mz_uint64>(archive.size()));

    CHECK_FALSE(
        mz_zip_validate_file(&zip, 0U, MZ_ZIP_FLAG_VALIDATE_HEADERS_ONLY));
    CHECK_EQ(mz_zip_get_last_error(&zip), MZ_ZIP_INVALID_HEADER_OR_CORRUPTED);

    mz_zip_clear_last_error(&zip);
    size_t extracted_size = 123U;
    void *extracted =
        mz_zip_reader_extract_to_heap(&zip, 0U, &extracted_size, 0);
    CHECK_EQ(extracted, nullptr);
    CHECK_EQ(extracted_size, 0U);
    CHECK_EQ(mz_zip_get_last_error(&zip), MZ_ZIP_INVALID_HEADER_OR_CORRUPTED);

    mz_zip_clear_last_error(&zip);
    extracted_size = 123U;
    extracted = mz_zip_reader_extract_to_heap(&zip, 0U, &extracted_size,
                                              MZ_ZIP_FLAG_COMPRESSED_DATA);
    CHECK_EQ(extracted, nullptr);
    CHECK_EQ(extracted_size, 0U);
    CHECK_EQ(mz_zip_get_last_error(&zip), MZ_ZIP_INVALID_HEADER_OR_CORRUPTED);

    CHECK(mz_zip_reader_end(&zip) == MZ_TRUE);
}

TEST_CASE(
    "archive metadata limits reject oversized DOCX inputs before extraction") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "archive_limits.docx";
    write_test_archive_entries(
        path, {{test_content_types_xml_entry, test_content_types_xml},
               {test_relationships_xml_entry, test_relationships_xml},
               {test_document_xml_entry, valid_document_xml},
               {"word/media/payload.bin", "12345678"}});

    const auto check_limit = [&](const featherdoc::archive_limits &limits,
                                 std::string_view expected_entry) {
        featherdoc::document_open_options options;
        options.limits = limits;
        featherdoc::Document document(path);
        CHECK_EQ(document.open(options),
                 featherdoc::document_errc::archive_limit_exceeded);
        if (!expected_entry.empty()) {
            CHECK_EQ(document.last_error().entry_name, expected_entry);
        }
        CHECK_FALSE(document.is_open());
    };

    auto limits = featherdoc::archive_limits{};
    limits.max_entries = 3U;
    check_limit(limits, {});

    limits = {};
    limits.max_xml_part_bytes = 1U;
    check_limit(limits, test_content_types_xml_entry);

    limits = {};
    limits.max_binary_part_bytes = 4U;
    check_limit(limits, "word/media/payload.bin");

    limits = {};
    limits.max_total_uncompressed_bytes = 1U;
    check_limit(limits, test_content_types_xml_entry);

    limits = {};
    limits.max_compression_ratio = 0U;
    check_limit(limits, test_content_types_xml_entry);

    fs::remove(path);
}

TEST_CASE("semantic XML parts disguised with binary extensions use XML archive "
          "limits") {
    namespace fs = std::filesystem;

    const auto options = semantic_xml_limit_test_options();
    const auto expect_open_rejects = [&](const fs::path &path,
                                         std::string_view expected_entry_name) {
        featherdoc::Document document(path);
        CHECK_EQ(document.open(options),
                 featherdoc::document_errc::archive_limit_exceeded);
        CHECK_EQ(document.last_error().entry_name, expected_entry_name);
        CHECK_FALSE(document.is_open());
    };

    SUBCASE("header relationship target is rejected during open") {
        const auto path = fs::current_path() / "xml_limit_header_bin.docx";
        write_docx_with_related_xml_parts(
            path,
            {{"rId2", header_relationship_type, "header1.bin",
              "/word/header1.bin", header_content_type, "word/header1.bin",
              oversized_xml_payload(namespaced_word_root_prefix("w:hdr"),
                                    namespaced_word_root_suffix("w:hdr"))}});

        expect_open_rejects(path, "word/header1.bin");
        fs::remove(path);
    }

    SUBCASE("settings relationship target is rejected on lazy load") {
        const auto path = fs::current_path() / "xml_limit_settings_bin.docx";
        write_docx_with_related_xml_parts(
            path,
            {{"rId2", settings_relationship_type, "settings.bin",
              "/word/settings.bin", settings_content_type, "word/settings.bin",
              oversized_xml_payload(
                  namespaced_word_root_prefix("w:settings"),
                  namespaced_word_root_suffix("w:settings"))}});

        featherdoc::Document document(path);
        REQUIRE_FALSE(document.open(options));
        CHECK_FALSE(document.update_fields_on_open_enabled().has_value());
        CHECK_EQ(document.last_error().code,
                 featherdoc::document_errc::archive_limit_exceeded);
        CHECK_EQ(document.last_error().entry_name, "word/settings.bin");
        fs::remove(path);
    }

    SUBCASE("numbering relationship target is rejected on lazy load") {
        const auto path = fs::current_path() / "xml_limit_numbering_bin.docx";
        write_docx_with_related_xml_parts(
            path, {{"rId2", numbering_relationship_type, "numbering.bin",
                    "/word/numbering.bin", numbering_content_type,
                    "word/numbering.bin",
                    oversized_xml_payload(
                        namespaced_word_root_prefix("w:numbering"),
                        namespaced_word_root_suffix("w:numbering"))}});

        featherdoc::Document document(path);
        REQUIRE_FALSE(document.open(options));
        CHECK(document.list_numbering_definitions().empty());
        CHECK_EQ(document.last_error().code,
                 featherdoc::document_errc::archive_limit_exceeded);
        CHECK_EQ(document.last_error().entry_name, "word/numbering.bin");
        fs::remove(path);
    }

    SUBCASE("styles relationship target is rejected on lazy load") {
        const auto path = fs::current_path() / "xml_limit_styles_bin.docx";
        write_docx_with_related_xml_parts(
            path,
            {{"rId2", styles_relationship_type, "styles.bin",
              "/word/styles.bin", styles_content_type, "word/styles.bin",
              oversized_xml_payload(namespaced_word_root_prefix("w:styles"),
                                    namespaced_word_root_suffix("w:styles"))}});

        featherdoc::Document document(path);
        REQUIRE_FALSE(document.open(options));
        CHECK(document.list_styles().empty());
        CHECK_EQ(document.last_error().code,
                 featherdoc::document_errc::archive_limit_exceeded);
        CHECK_EQ(document.last_error().entry_name, "word/styles.bin");
        fs::remove(path);
    }

    SUBCASE("comments relationship target is rejected on lazy load") {
        const auto path = fs::current_path() / "xml_limit_comments_bin.docx";
        write_docx_with_related_xml_parts(
            path,
            {{"rId2", comments_relationship_type, "comments.bin",
              "/word/comments.bin", comments_content_type, "word/comments.bin",
              oversized_xml_payload(
                  namespaced_word_root_prefix("w:comments"),
                  namespaced_word_root_suffix("w:comments"))}});

        featherdoc::Document document(path);
        REQUIRE_FALSE(document.open(options));
        CHECK(document.list_comments().empty());
        CHECK_EQ(document.last_error().code,
                 featherdoc::document_errc::archive_limit_exceeded);
        CHECK_EQ(document.last_error().entry_name, "word/comments.bin");
        fs::remove(path);
    }

    SUBCASE("footnotes relationship target is rejected on lazy load") {
        const auto path = fs::current_path() / "xml_limit_footnotes_bin.docx";
        write_docx_with_related_xml_parts(
            path, {{"rId2", footnotes_relationship_type, "footnotes.bin",
                    "/word/footnotes.bin", footnotes_content_type,
                    "word/footnotes.bin",
                    oversized_xml_payload(
                        namespaced_word_root_prefix("w:footnotes"),
                        namespaced_word_root_suffix("w:footnotes"))}});

        featherdoc::Document document(path);
        REQUIRE_FALSE(document.open(options));
        CHECK(document.list_footnotes().empty());
        CHECK_EQ(document.last_error().code,
                 featherdoc::document_errc::archive_limit_exceeded);
        CHECK_EQ(document.last_error().entry_name, "word/footnotes.bin");
        fs::remove(path);
    }

    SUBCASE("endnotes relationship target is rejected on lazy load") {
        const auto path = fs::current_path() / "xml_limit_endnotes_bin.docx";
        write_docx_with_related_xml_parts(
            path,
            {{"rId2", endnotes_relationship_type, "endnotes.bin",
              "/word/endnotes.bin", endnotes_content_type, "word/endnotes.bin",
              oversized_xml_payload(
                  namespaced_word_root_prefix("w:endnotes"),
                  namespaced_word_root_suffix("w:endnotes"))}});

        featherdoc::Document document(path);
        REQUIRE_FALSE(document.open(options));
        CHECK(document.list_endnotes().empty());
        CHECK_EQ(document.last_error().code,
                 featherdoc::document_errc::archive_limit_exceeded);
        CHECK_EQ(document.last_error().entry_name, "word/endnotes.bin");
        fs::remove(path);
    }

    SUBCASE("commentsExtended relationship target is rejected on lazy load") {
        const auto path =
            fs::current_path() / "xml_limit_comments_extended_bin.docx";
        write_docx_with_related_xml_parts(
            path,
            {{"rId2", comments_relationship_type, "comments.bin",
              "/word/comments.bin", comments_content_type, "word/comments.bin",
              minimal_xml_payload(namespaced_word_root_prefix("w:comments"),
                                  namespaced_word_root_suffix("w:comments"))},
             {"rId3", comments_extended_relationship_type,
              "commentsExtended.bin", "/word/commentsExtended.bin",
              comments_extended_content_type, "word/commentsExtended.bin",
              oversized_xml_payload(
                  std::string{"<w15:commentsEx xmlns:w15=\""} +
                      comments_extended_namespace_uri + "\">",
                  "</w15:commentsEx>")}});

        featherdoc::Document document(path);
        REQUIRE_FALSE(document.open(options));
        CHECK(document.list_comments().empty());
        CHECK_EQ(document.last_error().code,
                 featherdoc::document_errc::archive_limit_exceeded);
        CHECK_EQ(document.last_error().entry_name, "word/commentsExtended.bin");
        fs::remove(path);
    }

    SUBCASE(
        "binary payload over XML limit remains accepted under binary limit") {
        const auto path = fs::current_path() / "xml_limit_binary_payload.docx";
        write_test_archive_entries(
            path,
            {{test_content_types_xml_entry, test_content_types_xml},
             {test_relationships_xml_entry, test_relationships_xml},
             {test_document_xml_entry, valid_document_xml},
             {"word/media/payload.bin",
              std::string(options.limits.max_xml_part_bytes + 512U, 'b')}});

        featherdoc::Document document(path);
        CHECK_FALSE(document.open(options));
        CHECK(document.is_open());
        fs::remove(path);
    }
}

TEST_CASE("lazy XML part reads recheck source archive metadata after open") {
    namespace fs = std::filesystem;

    const auto options = semantic_xml_limit_test_options();
    const auto path =
        fs::current_path() / "xml_limit_replaced_source_settings.docx";
    write_docx_with_related_xml_parts(
        path,
        {{"rId2", settings_relationship_type, "settings.bin",
          "/word/settings.bin", settings_content_type, "word/settings.bin",
          minimal_xml_payload(namespaced_word_root_prefix("w:settings"),
                              namespaced_word_root_suffix("w:settings"))}});

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open(options));

    write_docx_with_related_xml_parts(
        path,
        {{"rId2", settings_relationship_type, "settings.bin",
          "/word/settings.bin", settings_content_type, "word/settings.bin",
          oversized_xml_payload(namespaced_word_root_prefix("w:settings"),
                                namespaced_word_root_suffix("w:settings"))}});

    CHECK_FALSE(document.update_fields_on_open_enabled().has_value());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::source_archive_changed);
    CHECK_EQ(document.last_error().entry_name, "word/settings.bin");
    fs::remove(path);
}

TEST_CASE(
    "lazy XML part reads reject a replaced source with ambiguous entries") {
    namespace fs = std::filesystem;

    const auto options = semantic_xml_limit_test_options();
    const auto path = fs::current_path() /
                      "xml_limit_replaced_source_duplicate_settings.docx";
    write_docx_with_related_xml_parts(
        path,
        {{"rId2", settings_relationship_type, "settings.bin",
          "/word/settings.bin", settings_content_type, "word/settings.bin",
          minimal_xml_payload(namespaced_word_root_prefix("w:settings"),
                              namespaced_word_root_suffix("w:settings"))}});

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open(options));

    const auto settings_xml =
        minimal_xml_payload(namespaced_word_root_prefix("w:settings"),
                            namespaced_word_root_suffix("w:settings"));
    write_test_archive_entries(
        path, {{test_content_types_xml_entry, test_content_types_xml},
               {test_relationships_xml_entry, test_relationships_xml},
               {test_document_xml_entry, valid_document_xml},
               {"word/settings.bin", settings_xml},
               {"WORD/SETTINGS.BIN", settings_xml}});

    CHECK_FALSE(document.update_fields_on_open_enabled().has_value());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::source_archive_changed);
    CHECK_EQ(document.last_error().entry_name, "WORD/SETTINGS.BIN");
    fs::remove(path);
}

#ifndef _WIN32
TEST_CASE(
    "lazy XML part reads report source archive close failures and retry") {
    namespace fs = std::filesystem;

    const auto path =
        fs::current_path() / "lazy_settings_reader_close_failure.docx";
    write_docx_with_related_xml_parts(
        path,
        {{"rId2", settings_relationship_type, "settings.xml",
          "/word/settings.xml", settings_content_type, "word/settings.xml",
          minimal_xml_payload(namespaced_word_root_prefix("w:settings"),
                              namespaced_word_root_suffix("w:settings"))}});

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open());

    featherdoc_test::zip_fail_next_close_stage(
        featherdoc_test::zip_close_failure_reader_end);
    CHECK_FALSE(document.update_fields_on_open_enabled().has_value());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::archive_close_failed);
    CHECK_EQ(document.last_error().entry_name, "word/settings.xml");

    CHECK_FALSE(document.update_fields_on_open_enabled().value_or(false));
    CHECK_FALSE(document.last_error());
    fs::remove(path);
}
#endif

TEST_CASE("settings mutation adopts an orphaned fixed settings part") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "orphan_settings_roundtrip.docx";
    const auto document_relationships =
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"/>)";
    const auto orphan_settings = namespaced_word_root_prefix("w:settings") +
                                 R"(<w:zoom w:percent="137"/>)" +
                                 namespaced_word_root_suffix("w:settings");
    write_test_archive_entries(
        path, {{test_content_types_xml_entry, test_content_types_xml},
               {test_relationships_xml_entry, test_relationships_xml},
               {test_document_xml_entry, valid_document_xml},
               {"word/_rels/document.xml.rels", document_relationships},
               {"WORD/SETTINGS.XML", orphan_settings}});

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open());
    CHECK_FALSE(document.update_fields_on_open_enabled().value_or(true));
    REQUIRE(document.enable_update_fields_on_open());
    REQUIRE_FALSE(document.save());

    const auto saved_settings = read_test_docx_entry(path, "word/settings.xml");
    CHECK(saved_settings.find("w:zoom") != std::string::npos);
    CHECK(saved_settings.find("w:percent=\"137\"") != std::string::npos);
    CHECK(saved_settings.find("w:updateFields") != std::string::npos);
    CHECK(read_test_docx_entry(path, "word/_rels/document.xml.rels")
              .find(settings_relationship_type) != std::string::npos);

    featherdoc::Document reopened(path);
    REQUIRE_FALSE(reopened.open());
    CHECK(reopened.update_fields_on_open_enabled().value_or(false));
    fs::remove(path);
}

TEST_CASE("numbering mutation adopts an orphaned fixed numbering part") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "orphan_numbering_roundtrip.docx";
    const auto document_relationships =
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"/>)";
    const auto orphan_numbering =
        namespaced_word_root_prefix("w:numbering") +
        R"(<w:abstractNum w:abstractNumId="7"><w:name w:val="OrphanNumbering"/><w:lvl w:ilvl="0"><w:start w:val="1"/><w:numFmt w:val="decimal"/><w:lvlText w:val="%1."/></w:lvl></w:abstractNum><w:num w:numId="9"><w:abstractNumId w:val="7"/></w:num>)" +
        namespaced_word_root_suffix("w:numbering");
    write_test_archive_entries(
        path, {{test_content_types_xml_entry, test_content_types_xml},
               {test_relationships_xml_entry, test_relationships_xml},
               {test_document_xml_entry, valid_document_xml},
               {"word/_rels/document.xml.rels", document_relationships},
               {"WORD/NUMBERING.XML", orphan_numbering}});

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open());
    CHECK(std::ranges::any_of(document.list_numbering_definitions(),
                              [](const auto &definition) {
                                  return definition.name == "OrphanNumbering";
                              }));
    auto added_definition = featherdoc::numbering_definition{};
    added_definition.name = "AddedNumbering";
    added_definition.levels = {featherdoc::numbering_level_definition{
        featherdoc::list_kind::decimal, 1U, 0U, "%1."}};
    REQUIRE(document.ensure_numbering_definition(added_definition).has_value());
    REQUIRE_FALSE(document.save());

    const auto saved_numbering =
        read_test_docx_entry(path, "word/numbering.xml");
    CHECK(saved_numbering.find("OrphanNumbering") != std::string::npos);
    CHECK(saved_numbering.find("AddedNumbering") != std::string::npos);
    CHECK(read_test_docx_entry(path, "word/_rels/document.xml.rels")
              .find(numbering_relationship_type) != std::string::npos);

    featherdoc::Document reopened(path);
    REQUIRE_FALSE(reopened.open());
    const auto definitions = reopened.list_numbering_definitions();
    CHECK(std::ranges::any_of(definitions, [](const auto &definition) {
        return definition.name == "OrphanNumbering";
    }));
    CHECK(std::ranges::any_of(definitions, [](const auto &definition) {
        return definition.name == "AddedNumbering";
    }));
    fs::remove(path);
}

TEST_CASE("styles mutation adopts an orphaned fixed styles part") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "orphan_styles_roundtrip.docx";
    const auto document_relationships =
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"/>)";
    const auto orphan_styles =
        namespaced_word_root_prefix("w:styles") +
        R"(<w:style w:type="paragraph" w:styleId="OrphanStyle"><w:name w:val="Orphan Style"/><w:qFormat/></w:style>)" +
        namespaced_word_root_suffix("w:styles");
    write_test_archive_entries(
        path, {{test_content_types_xml_entry, test_content_types_xml},
               {test_relationships_xml_entry, test_relationships_xml},
               {test_document_xml_entry, valid_document_xml},
               {"word/_rels/document.xml.rels", document_relationships},
               {"WORD/STYLES.XML", orphan_styles}});

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open());
    CHECK(std::ranges::any_of(document.list_styles(), [](const auto &style) {
        return style.style_id == "OrphanStyle";
    }));
    auto added_style = featherdoc::paragraph_style_definition{};
    added_style.name = "Added Style";
    added_style.is_quick_format = true;
    REQUIRE(document.ensure_paragraph_style("AddedStyle", added_style));
    REQUIRE_FALSE(document.save());

    const auto saved_styles = read_test_docx_entry(path, "word/styles.xml");
    CHECK(saved_styles.find("OrphanStyle") != std::string::npos);
    CHECK(saved_styles.find("AddedStyle") != std::string::npos);
    CHECK(read_test_docx_entry(path, "word/_rels/document.xml.rels")
              .find(styles_relationship_type) != std::string::npos);

    featherdoc::Document reopened(path);
    REQUIRE_FALSE(reopened.open());
    const auto styles = reopened.list_styles();
    CHECK(std::ranges::any_of(styles, [](const auto &style) {
        return style.style_id == "OrphanStyle";
    }));
    CHECK(std::ranges::any_of(styles, [](const auto &style) {
        return style.style_id == "AddedStyle";
    }));
    fs::remove(path);
}

#ifndef _WIN32
TEST_CASE("open reports source archive close failure and remains retryable") {
    namespace fs = std::filesystem;

    const auto path = fs::current_path() / "open_reader_close_failure.docx";
    write_test_docx(path, valid_document_xml);

    featherdoc::Document document(path);
    featherdoc_test::zip_fail_next_close_stage(
        featherdoc_test::zip_close_failure_reader_end);
    CHECK_EQ(document.open(), featherdoc::document_errc::archive_close_failed);
    CHECK_FALSE(document.is_open());

    CHECK_FALSE(document.open());
    CHECK(document.is_open());
    CHECK_FALSE(document.last_error());
    fs::remove(path);
}
#endif

TEST_CASE(
    "save uses unique temporary files and preserves fixed sibling files") {
    namespace fs = std::filesystem;
    const auto target = fs::current_path() / "save_transaction.docx";
    auto fixed_tmp = target;
    fixed_tmp += ".tmp";
    auto fixed_backup = target;
    fixed_backup += ".bak";
    fs::remove(target);
    write_file_text(fixed_tmp, "user tmp");
    write_file_text(fixed_backup, "user backup");

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.create_empty());
    REQUIRE(document.paragraphs().add_run("transactional save").has_next());
    REQUIRE_FALSE(document.save());

    CHECK_EQ(read_file_text(fixed_tmp), "user tmp");
    CHECK_EQ(read_file_text(fixed_backup), "user backup");
    CHECK_EQ(unique_temp_files_for(target), 0U);

    fs::remove(target);
    fs::remove(fixed_tmp);
    fs::remove(fixed_backup);
}

TEST_CASE("temporary output reservation collision never deletes user files") {
    namespace fs = std::filesystem;

    const auto directory =
        fs::current_path() / "save_transaction_candidate_collisions";
    fs::remove_all(directory);
    REQUIRE(fs::create_directories(directory));

    const auto target = directory / "output.docx";
    constexpr std::uint64_t forced_timestamp = 123456789U;
    constexpr std::uint64_t forced_sequence_start = 987654321U;
    std::vector<std::pair<fs::path, std::string>> collision_files;
    collision_files.reserve(64U);

    for (std::uint64_t offset = 0U; offset < 64U; ++offset) {
        auto collision_path = forced_temp_collision_path(
            directory, forced_timestamp, forced_sequence_start + offset);
        auto collision_content =
            "user collision file " + std::to_string(offset);
        write_file_text(collision_path, collision_content);
        collision_files.emplace_back(std::move(collision_path),
                                     std::move(collision_content));
    }

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    REQUIRE(document.paragraphs().add_run("not written").has_next());

    document_test_force_next_temp_reservation(forced_timestamp,
                                              forced_sequence_start);
    const auto save_error = document.save_as(target);
    CHECK_EQ(save_error, featherdoc::document_errc::output_archive_open_failed);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::output_archive_open_failed);
    CHECK_FALSE(fs::exists(target));
    CHECK_EQ(unique_temp_files_for(target), collision_files.size());

    for (const auto &[path, expected_content] : collision_files) {
        CAPTURE(path);
        CHECK_EQ(read_file_text(path), expected_content);
    }

    fs::remove_all(directory);
}

TEST_CASE("temporary output creation failure is reported before ZIP writing") {
    namespace fs = std::filesystem;
    const auto missing_parent =
        fs::current_path() / "missing_save_transaction_parent";
    const auto target = missing_parent / "output.docx";
    fs::remove_all(missing_parent);

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    const auto save_error = document.save_as(target);
    CHECK_EQ(save_error, featherdoc::document_errc::output_archive_open_failed);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::output_archive_open_failed);
    CHECK_FALSE(fs::exists(missing_parent));
}

TEST_CASE(
    "transactional save keeps temporary names short beside long targets") {
    namespace fs = std::filesystem;

    const auto directory = fs::current_path() / "save_long_target_name";
    fs::remove_all(directory);
    REQUIRE(fs::create_directories(directory));

    constexpr std::size_t desired_target_length = 230U;
    constexpr std::size_t extension_length = 5U;
    const auto directory_length = directory.native().size() + 1U;
    const auto available_stem_length =
        desired_target_length > directory_length + extension_length
            ? desired_target_length - directory_length - extension_length
            : 80U;
    const auto stem_length =
        std::clamp<std::size_t>(available_stem_length, 80U, 180U);
    const auto target = directory / (std::string(stem_length, 'x') + ".docx");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    REQUIRE(document.paragraphs().add_run("long target save").has_next());
    REQUIRE_FALSE(document.save_as(target));
    CHECK_EQ(unique_temp_files_for(target), 0U);

    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "long target save\n");

    fs::remove_all(directory);
}

#ifndef _WIN32
TEST_CASE(
    "ZIP entry write failure preserves the original target and allows retry") {
    namespace fs = std::filesystem;
    const auto target = fs::current_path() / "save_write_failure.docx";
    fs::remove(target);
    write_file_text(target, "original bytes");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    REQUIRE(
        document.paragraphs().add_run("retry after write failure").has_next());

    featherdoc_test::zip_fail_next_write();
    const auto save_error = document.save_as(target);
    CHECK_EQ(save_error,
             featherdoc::document_errc::output_document_xml_write_failed);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::output_document_xml_write_failed);
    CHECK_EQ(read_file_text(target), "original bytes");
    CHECK_EQ(unique_temp_files_for(target), 0U);

    REQUIRE_FALSE(document.save_as(target));
    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "retry after write failure\n");
    CHECK_EQ(unique_temp_files_for(target), 0U);
    fs::remove(target);
}
#endif

#ifndef _WIN32
TEST_CASE("ZIP close failures preserve the original target and allow retry") {
    namespace fs = std::filesystem;

    const auto check_failure = [](int failure_stage,
                                  std::string_view stage_name) {
        auto target = fs::current_path() / "save_close_failure.docx";
        target += "." + std::string{stage_name};
        fs::remove(target);
        write_file_text(target, "original bytes");

        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());
        REQUIRE(document.paragraphs().add_run("retry succeeds").has_next());

        featherdoc_test::zip_fail_next_close_stage(failure_stage);
        const auto save_error = document.save_as(target);
        CHECK_EQ(save_error,
                 featherdoc::document_errc::output_archive_finalize_failed);
        CHECK_EQ(document.last_error().code,
                 featherdoc::document_errc::output_archive_finalize_failed);
        CHECK_EQ(read_file_text(target), "original bytes");
        CHECK_EQ(unique_temp_files_for(target), 0U);

        REQUIRE_FALSE(document.save_as(target));
        featherdoc::Document reopened(target);
        REQUIRE_FALSE(reopened.open());
        CHECK_EQ(collect_document_text(reopened), "retry succeeds\n");
        CHECK_EQ(unique_temp_files_for(target), 0U);
        fs::remove(target);
    };

    SUBCASE("archive finalize") {
        check_failure(featherdoc_test::zip_close_failure_finalize, "finalize");
    }
    SUBCASE("archive truncate") {
        check_failure(featherdoc_test::zip_close_failure_truncate, "truncate");
    }
    SUBCASE("writer end") {
        check_failure(featherdoc_test::zip_close_failure_writer_end,
                      "writer-end");
    }
}
#endif

#ifndef _WIN32
TEST_CASE("source reader close failure prevents target replacement and allows "
          "retry") {
    namespace fs = std::filesystem;

    const auto source = fs::current_path() / "save_reader_close_source.docx";
    const auto target = fs::current_path() / "save_reader_close_target.docx";
    fs::remove(source);
    fs::remove(target);
    write_test_docx(source, valid_document_xml);
    write_file_text(target, "original target bytes");

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.paragraphs().add_run("reader close retry").has_next());

    featherdoc_test::zip_fail_next_close_stage(
        featherdoc_test::zip_close_failure_reader_end);
    const auto save_error = document.save_as(target);
    CHECK_EQ(save_error, featherdoc::document_errc::archive_close_failed);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::archive_close_failed);
    CHECK_EQ(read_file_text(target), "original target bytes");
    CHECK_EQ(unique_temp_files_for(target), 0U);

    REQUIRE_FALSE(document.save_as(target));
    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    CHECK_NE(collect_document_text(reopened).find("reader close retry"),
             std::string::npos);
    CHECK_EQ(unique_temp_files_for(target), 0U);

    fs::remove(source);
    fs::remove(target);
}
#endif

#ifndef _WIN32
TEST_CASE("temporary output sync failure preserves the original target") {
    namespace fs = std::filesystem;
    const auto target = fs::current_path() / "save_file_sync_failure.docx";
    fs::remove(target);
    write_file_text(target, "original bytes");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    REQUIRE(document.paragraphs().add_run("unsynced replacement").has_next());

    featherdoc_test::document_fail_next_sync_stage(
        featherdoc_test::document_sync_failure_file);
    const auto save_error = document.save_as(target);
    CHECK_EQ(save_error, featherdoc::document_errc::output_file_sync_failed);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::output_file_sync_failed);
    CHECK_EQ(read_file_text(target), "original bytes");
    CHECK_EQ(unique_temp_files_for(target), 0U);

    fs::remove(target);
}
#endif

#ifndef _WIN32
TEST_CASE("parent directory sync failure reports that replacement completed") {
    namespace fs = std::filesystem;
    const auto target = fs::current_path() / "save_directory_sync_failure.docx";
    fs::remove(target);
    write_file_text(target, "original bytes");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    REQUIRE(document.paragraphs().add_run("replacement completed").has_next());

    featherdoc_test::document_fail_next_sync_stage(
        featherdoc_test::document_sync_failure_parent_directory);
    const auto save_error = document.save_as(target);
    CHECK_EQ(
        save_error,
        featherdoc::document_errc::output_directory_sync_failed_after_replace);
    CHECK_EQ(
        document.last_error().code,
        featherdoc::document_errc::output_directory_sync_failed_after_replace);
    CHECK_EQ(unique_temp_files_for(target), 0U);

    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "replacement completed\n");

    fs::remove(target);
}
#endif

#ifndef _WIN32
TEST_CASE("POSIX save preserves permissions of an existing target") {
    namespace fs = std::filesystem;
    if (!filesystem_supports_posix_mode_bits(fs::current_path())) {
        return;
    }

    const auto target = fs::current_path() / "save_existing_permissions.docx";
    fs::remove(target);

    featherdoc::Document initial_document;
    REQUIRE_FALSE(initial_document.create_empty());
    REQUIRE_FALSE(initial_document.save_as(target));
    REQUIRE_EQ(::chmod(target.c_str(), 0640), 0);

    featherdoc::Document replacement_document;
    REQUIRE_FALSE(replacement_document.create_empty());
    REQUIRE(replacement_document.paragraphs()
                .add_run("preserve permissions")
                .has_next());
    REQUIRE_FALSE(replacement_document.save_as(target));

    struct stat target_status{};
    REQUIRE_EQ(::stat(target.c_str(), &target_status), 0);
    CHECK_EQ(target_status.st_mode & 07777, 0640);

    fs::remove(target);
}

TEST_CASE("POSIX save applies the process umask to a new target") {
    namespace fs = std::filesystem;
    if (!filesystem_supports_posix_mode_bits(fs::current_path())) {
        return;
    }

    const auto target = fs::current_path() / "save_new_permissions.docx";
    fs::remove(target);

    struct scoped_umask final {
        mode_t previous;
        explicit scoped_umask(mode_t value) : previous(::umask(value)) {}
        ~scoped_umask() { ::umask(this->previous); }
    } mask{0027};

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    REQUIRE_FALSE(document.save_as(target));
    CHECK_EQ(featherdoc_test::document_last_reserved_temp_mode(), 0600U);

    struct stat target_status{};
    REQUIRE_EQ(::stat(target.c_str(), &target_status), 0);
    CHECK_EQ(target_status.st_mode & 07777, 0640);

    fs::remove(target);
}
#endif

TEST_CASE("concurrent saves use independent temporary archives") {
    namespace fs = std::filesystem;
    const auto target = fs::current_path() / "concurrent_save.docx";
    fs::remove(target);

    featherdoc::Document first;
    featherdoc::Document second;
    REQUIRE_FALSE(first.create_empty());
    REQUIRE_FALSE(second.create_empty());
    REQUIRE(first.paragraphs().add_run("first writer").has_next());
    REQUIRE(second.paragraphs().add_run("second writer").has_next());

    std::barrier start{3};
    std::error_code first_error;
    std::error_code second_error;
    std::thread first_thread([&] {
        start.arrive_and_wait();
        first_error = first.save_as(target);
    });
    std::thread second_thread([&] {
        start.arrive_and_wait();
        second_error = second.save_as(target);
    });
    start.arrive_and_wait();
    first_thread.join();
    second_thread.join();

    CHECK_FALSE(first_error);
    CHECK_FALSE(second_error);
    CHECK_EQ(unique_temp_files_for(target), 0U);
    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    const auto text = collect_document_text(reopened);
    CHECK((text.find("first writer") != std::string::npos ||
           text.find("second writer") != std::string::npos));

    fs::remove(target);
}

#if defined(_WIN32) &&                                                     \
    defined(FEATHERDOC_ENABLE_WINDOWS_FAULT_INJECTION_TESTS) &&           \
    FEATHERDOC_ENABLE_WINDOWS_FAULT_INJECTION_TESTS
TEST_CASE("failed Windows replacement preserves the original target") {
    namespace fs = std::filesystem;
    const auto target = fs::current_path() / "replace_failure.docx";
    write_file_text(target, "original bytes");

    const auto locked_file =
        CreateFileW(target.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(locked_file != INVALID_HANDLE_VALUE);

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    const auto save_error = document.save_as(target);
    CHECK_EQ(save_error, featherdoc::document_errc::output_replace_failed);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::output_replace_failed);
    CHECK(CloseHandle(locked_file) != 0);

    CHECK_EQ(read_file_text(target), "original bytes");
    CHECK_EQ(unique_temp_files_for(target), 0U);
    fs::remove(target);
}
#endif

TEST_CASE("reloading a document requires reacquiring XML-backed handles") {
    namespace fs = std::filesystem;
    const auto first_path = fs::current_path() / "handle_generation_first.docx";
    const auto second_path =
        fs::current_path() / "handle_generation_second.docx";
    const auto output_path =
        fs::current_path() / "handle_generation_output.docx";
    write_test_docx(first_path, valid_document_xml);
    write_test_docx(
        second_path,
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:t>second</w:t></w:r></w:p></w:body></w:document>)");

    featherdoc::Document document(first_path);
    REQUIRE_FALSE(document.open());
    auto old_paragraph = document.paragraphs();
    REQUIRE(old_paragraph.has_next());
    auto old_run = old_paragraph.runs();
    REQUIRE(old_run.has_next());
    auto old_template_part = document.body_template();
    REQUIRE(static_cast<bool>(old_template_part));

    document.set_path(second_path);
    CHECK_FALSE(old_paragraph.valid());
    CHECK_FALSE(old_paragraph.has_next());
    CHECK_FALSE(old_paragraph.set_text("stale paragraph"));
    CHECK_FALSE(old_run.valid());
    CHECK_FALSE(old_run.has_next());
    CHECK_FALSE(old_run.set_text("stale run"));
    CHECK_FALSE(static_cast<bool>(old_template_part));
    CHECK_FALSE(old_template_part.append_paragraph("stale template").valid());
    REQUIRE_FALSE(document.open());
    auto current_paragraph = document.paragraphs();
    REQUIRE(current_paragraph.has_next());
    REQUIRE(current_paragraph.add_run(" current handle").has_next());
    REQUIRE_FALSE(document.save_as(output_path));

    featherdoc::Document reopened(output_path);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "second current handle\n");

    fs::remove(first_path);
    fs::remove(second_path);
    fs::remove(output_path);
}

TEST_CASE("all XML-backed handles become inert after Document destruction") {
    featherdoc::Paragraph stale_paragraph;
    featherdoc::Run stale_run;
    featherdoc::Table stale_table;
    featherdoc::TableRow stale_row;
    featherdoc::TableCell stale_cell;
    featherdoc::TemplatePart stale_template_part;

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.valid());
        stale_paragraph = paragraph;
        stale_run = paragraph.add_run("owned text");
        REQUIRE(stale_run.valid());

        stale_table = document.append_table(1U, 1U);
        REQUIRE(stale_table.valid());
        stale_row = stale_table.rows();
        REQUIRE(stale_row.valid());
        stale_cell = stale_row.cells();
        REQUIRE(stale_cell.valid());

        stale_template_part = document.body_template();
        REQUIRE(static_cast<bool>(stale_template_part));
    }

    CHECK_FALSE(stale_paragraph.valid());
    CHECK_FALSE(stale_paragraph.set_text("after destruction"));
    CHECK_FALSE(stale_run.valid());
    CHECK_FALSE(stale_run.set_text("after destruction"));
    CHECK_FALSE(stale_table.valid());
    CHECK_FALSE(stale_table.set_width_twips(1000U));
    CHECK_FALSE(stale_row.valid());
    CHECK_FALSE(stale_row.set_cant_split());
    CHECK_FALSE(stale_cell.valid());
    CHECK_FALSE(stale_cell.set_text("after destruction"));
    CHECK_FALSE(static_cast<bool>(stale_template_part));
    CHECK_FALSE(
        stale_template_part.append_paragraph("after destruction").valid());
    CHECK_FALSE(stale_template_part.append_table(1U, 1U).valid());
}

TEST_CASE("removed XML subtrees invalidate affected handle copies") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    auto removed_run = paragraph.add_run("old run");
    REQUIRE(removed_run.valid());
    REQUIRE(paragraph.set_text("replacement"));
    CHECK_FALSE(removed_run.valid());
    CHECK_FALSE(removed_run.set_text("stale"));
    CHECK(paragraph.valid());
    CHECK_EQ(paragraph.runs().get_text(), "replacement");

    auto removed_paragraph_copy = paragraph;
    REQUIRE(paragraph.insert_paragraph_after("next paragraph").valid());
    REQUIRE(paragraph.remove());
    CHECK_FALSE(removed_paragraph_copy.valid());
    CHECK(paragraph.valid());
    CHECK_EQ(paragraph.runs().get_text(), "next paragraph");

    auto table = document.append_table(1U, 2U);
    REQUIRE(table.valid());
    auto first_cell = table.rows().cells();
    auto removed_cell = first_cell;
    removed_cell.next();
    REQUIRE(removed_cell.valid());
    REQUIRE(first_cell.merge_right());
    CHECK_FALSE(removed_cell.valid());
    CHECK_FALSE(removed_cell.set_text("stale cell"));
    CHECK(first_cell.valid());
    CHECK(table.valid());

    REQUIRE(table.insert_table_after(1U, 1U).valid());
    auto removed_table_copy = table;
    REQUIRE(table.remove());
    CHECK_FALSE(removed_table_copy.valid());
    CHECK(table.valid());
}

TEST_CASE("document relationships validate their root and namespace without "
          "tolerant mutation") {
    namespace fs = std::filesystem;
    const auto source = fs::current_path() / "invalid_document_rels_root.docx";
    const auto output = fs::current_path() / "invalid_document_rels_saved.docx";
    const std::vector<std::string> invalid_relationship_parts{
        R"(<NotRelationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"/>)",
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Unexpected/></Relationships>)",
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="" Type="urn:missing-id" Target="keep.bin"/></Relationships>)",
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rMissingType" Target="keep.bin"/></Relationships>)",
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rMissingTarget" Type="urn:missing-target"/></Relationships>)",
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rMode" Type="urn:mode" Target="keep.bin" TargetMode="Sideways"/></Relationships>)",
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rNested" Type="urn:nested" Target="keep.bin"><Unexpected/></Relationship></Relationships>)",
    };

    for (const auto &relationships_xml : invalid_relationship_parts) {
        write_test_archive_entries(
            source, {{test_content_types_xml_entry, test_content_types_xml},
                     {test_relationships_xml_entry, test_relationships_xml},
                     {test_document_xml_entry, valid_document_xml},
                     {"word/_rels/document.xml.rels", relationships_xml}});

        featherdoc::Document strict_document(source);
        CHECK_EQ(strict_document.open(),
                 featherdoc::document_errc::invalid_package_structure);

        featherdoc::document_open_options options;
        options.validation = featherdoc::package_validation_mode::tolerant;
        featherdoc::Document tolerant_document(source);
        REQUIRE_FALSE(tolerant_document.open(options));
        CHECK(std::ranges::any_of(
            tolerant_document.package_diagnostics(), [](const auto &item) {
                return item.code == featherdoc::package_diagnostic_code::
                                        invalid_relationships_part;
            }));
        REQUIRE_FALSE(tolerant_document.save_as(output));
        CHECK_EQ(read_test_docx_entry(output, "word/_rels/document.xml.rels"),
                 relationships_xml);
    }

    fs::remove(source);
    fs::remove(output);
}

TEST_CASE("prefixed package relationship QNames open and mutate without "
          "namespace loss") {
    namespace fs = std::filesystem;
    const auto source =
        fs::current_path() / "prefixed_relationship_qnames.docx";
    const auto output =
        fs::current_path() / "prefixed_relationship_qnames_saved.docx";
    const auto image_path =
        fs::current_path() / "prefixed_relationship_qnames.png";
    const auto header_xml = std::string{
        R"(<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:p><w:r><w:t>header</w:t></w:r></w:p></w:hdr>)"};

    write_docx_with_related_xml_parts(
        source, {{"rHeader", header_relationship_type, "header1.xml",
                  "/word/header1.xml", header_content_type, "word/header1.xml",
                  header_xml}});
    auto entries = read_test_archive_entries(source);
    for (auto &[entry_name, content] : entries) {
        if (entry_name == test_relationships_xml_entry) {
            content =
                R"(<opc:Relationships xmlns:opc="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:edge="http://schemas.openxmlformats.org/package/2006/relationships"><edge:Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/></opc:Relationships>)";
        } else if (entry_name == test_document_xml_entry) {
            content =
                R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"><w:body><w:p><w:r><w:t>secure</w:t></w:r></w:p><w:sectPr><w:headerReference w:type="default" r:id="rHeader"/></w:sectPr></w:body></w:document>)";
        } else if (entry_name == "word/_rels/document.xml.rels") {
            content =
                std::string{
                    R"(<docrel:Relationships xmlns:docrel="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:item="http://schemas.openxmlformats.org/package/2006/relationships"><item:Relationship Id="rHeader" Type=")"} +
                header_relationship_type +
                R"(" Target="header1.xml"/></docrel:Relationships>)";
        }
    }
    entries.emplace_back(
        "word/_rels/header1.xml.rels",
        R"(<partrel:Relationships xmlns:partrel="http://schemas.openxmlformats.org/package/2006/relationships"/>)");
    write_test_archive_entries(source, entries);
    write_binary_file(image_path, tiny_png_data());

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    CHECK_EQ(document.header_count(), 1U);
    CHECK(document.enable_update_fields_on_open());
    featherdoc::numbering_definition numbering;
    numbering.name = "PrefixedRelationshipNumbering";
    numbering.levels = {featherdoc::numbering_level_definition{
        featherdoc::list_kind::decimal, 1U, 0U, "%1."}};
    CHECK(document.ensure_numbering_definition(numbering).has_value());
    featherdoc::paragraph_style_definition style;
    style.name = "Prefixed Relationship Style";
    style.is_quick_format = true;
    CHECK(document.ensure_paragraph_style("PrefixedRelationshipStyle", style));
    CHECK(document.append_image(image_path));
    CHECK_EQ(document.header_template().append_hyperlink(
                 "prefixed link", "https://example.com/prefixed"),
             1U);
    CHECK(document.ensure_section_footer_paragraphs(0U).valid());
    REQUIRE_FALSE(document.save_as(output));

    const auto saved_document_relationships =
        read_test_docx_entry(output, "word/_rels/document.xml.rels");
    CHECK_NE(saved_document_relationships.find("<item:Relationship"),
             std::string::npos);
    CHECK_NE(saved_document_relationships.find("<docrel:Relationship"),
             std::string::npos);
    CHECK_EQ(saved_document_relationships.find("<Relationship "),
             std::string::npos);
    CHECK_NE(saved_document_relationships.find(settings_relationship_type),
             std::string::npos);
    CHECK_NE(saved_document_relationships.find(numbering_relationship_type),
             std::string::npos);
    CHECK_NE(saved_document_relationships.find(styles_relationship_type),
             std::string::npos);
    CHECK_NE(saved_document_relationships.find(footer_relationship_type),
             std::string::npos);
    CHECK_NE(saved_document_relationships.find("/relationships/image"),
             std::string::npos);

    const auto saved_header_relationships =
        read_test_docx_entry(output, "word/_rels/header1.xml.rels");
    CHECK_NE(saved_header_relationships.find("<partrel:Relationship"),
             std::string::npos);
    CHECK_EQ(saved_header_relationships.find("<Relationship "),
             std::string::npos);
    CHECK_NE(saved_header_relationships.find("https://example.com/prefixed"),
             std::string::npos);

    featherdoc::Document reopened(output);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1U);
    CHECK_EQ(reopened.footer_count(), 1U);
    CHECK(reopened.update_fields_on_open_enabled().value_or(false));
    CHECK(std::ranges::any_of(
        reopened.list_numbering_definitions(), [](const auto &definition) {
            return definition.name == "PrefixedRelationshipNumbering";
        }));
    CHECK(std::ranges::any_of(reopened.list_styles(), [](const auto &summary) {
        return summary.style_id == "PrefixedRelationshipStyle";
    }));

    fs::remove(source);
    fs::remove(output);
    fs::remove(image_path);
}

TEST_CASE("prefixed Content Types QNames open mutate save and reopen without "
          "lexical namespace loss") {
    namespace fs = std::filesystem;
    const auto source =
        fs::current_path() / "prefixed_content_types_qnames.docx";
    const auto output =
        fs::current_path() / "prefixed_content_types_qnames_saved.docx";
    const auto image_path =
        fs::current_path() / "prefixed_content_types_qnames.png";
    constexpr auto content_types = R"(
<ct:Types xmlns:ct="http://schemas.openxmlformats.org/package/2006/content-types"
          xmlns:item="http://schemas.openxmlformats.org/package/2006/content-types">
  <item:Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <item:Default Extension="xml" ContentType="application/xml"/>
  <item:Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</ct:Types>)";
    write_test_archive_entries(
        source, {{test_content_types_xml_entry, content_types},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, valid_document_xml}});
    write_binary_file(image_path, tiny_png_data());

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.enable_update_fields_on_open());

    featherdoc::numbering_definition numbering;
    numbering.name = "PrefixedContentTypesNumbering";
    numbering.levels = {featherdoc::numbering_level_definition{
        featherdoc::list_kind::decimal, 1U, 0U, "%1."}};
    REQUIRE(document.ensure_numbering_definition(numbering).has_value());

    featherdoc::paragraph_style_definition style;
    style.name = "Prefixed Content Types Style";
    style.is_quick_format = true;
    REQUIRE(
        document.ensure_paragraph_style("PrefixedContentTypesStyle", style));
    REQUIRE(document.append_image(image_path));
    REQUIRE_FALSE(document.save_as(output));

    const auto saved_content_types =
        read_test_docx_entry(output, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("<ct:Types"), std::string::npos);
    CHECK_NE(saved_content_types.find("<item:Default"), std::string::npos);
    CHECK_NE(saved_content_types.find("<item:Override"), std::string::npos);
    CHECK_NE(saved_content_types.find("<ct:Default Extension=\"png\""),
             std::string::npos);
    CHECK_NE(saved_content_types.find(
                 "<ct:Override PartName=\"/word/settings.xml\""),
             std::string::npos);
    CHECK_NE(saved_content_types.find(
                 "<ct:Override PartName=\"/word/numbering.xml\""),
             std::string::npos);
    CHECK_NE(
        saved_content_types.find("<ct:Override PartName=\"/word/styles.xml\""),
        std::string::npos);
    CHECK_EQ(saved_content_types.find("<Default "), std::string::npos);
    CHECK_EQ(saved_content_types.find("<Override "), std::string::npos);

    featherdoc::Document reopened(output);
    REQUIRE_FALSE(reopened.open());
    CHECK(reopened.update_fields_on_open_enabled().value_or(false));
    CHECK(std::ranges::any_of(
        reopened.list_numbering_definitions(), [](const auto &definition) {
            return definition.name == "PrefixedContentTypesNumbering";
        }));
    CHECK(std::ranges::any_of(reopened.list_styles(), [](const auto &summary) {
        return summary.style_id == "PrefixedContentTypesStyle";
    }));
    CHECK_EQ(reopened.inline_images().size(), 1U);

    fs::remove(source);
    fs::remove(output);
    fs::remove(image_path);
}

TEST_CASE("strict Content Types validation rejects namespace confusion and "
          "invalid declaration structure while tolerant mode diagnoses it") {
    namespace fs = std::filesystem;
    const auto path =
        fs::current_path() / "content_types_expanded_qname_validation.docx";
    const auto output = fs::current_path() /
                        "content_types_expanded_qname_validation_saved.docx";
    constexpr auto namespace_uri =
        "http://schemas.openxmlformats.org/package/2006/content-types";
    const auto valid_main = std::string{
        R"(<ct:Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>)"};
    const std::vector<std::pair<std::string, std::string>> invalid_documents{
        {"wrong root namespace",
         R"(<ct:Types xmlns:ct="urn:wrong:content-types"><ct:Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/></ct:Types>)"},
        {"duplicate root namespace declaration",
         std::string{"<ct:Types xmlns:ct=\""} + namespace_uri +
             "\" xmlns:ct=\"urn:shadow\">" + valid_main + "</ct:Types>"},
        {"default namespace reset",
         std::string{"<ct:Types xmlns:ct=\""} + namespace_uri +
             "\"><Override xmlns=\"\" PartName=\"/word/fake.xml\" "
             "ContentType=\"application/xml\"/>" +
             valid_main + "</ct:Types>"},
        {"prefix shadow", std::string{"<ct:Types xmlns:ct=\""} + namespace_uri +
                              "\"><ct:Override xmlns:ct=\"urn:shadow\" "
                              "PartName=\"/word/fake.xml\" "
                              "ContentType=\"application/xml\"/>" +
                              valid_main + "</ct:Types>"},
        {"duplicate child namespace declaration",
         std::string{"<ct:Types xmlns:ct=\""} + namespace_uri +
             "\"><ct:Default xmlns:ct=\"" + namespace_uri +
             "\" xmlns:ct=\"urn:shadow\" Extension=\"xml\" "
             "ContentType=\"application/xml\"/>" +
             valid_main + "</ct:Types>"},
        {"unknown child", std::string{"<ct:Types xmlns:ct=\""} + namespace_uri +
                              "\"><ct:Unexpected/>" + valid_main +
                              "</ct:Types>"},
        {"nested declaration",
         std::string{"<ct:Types xmlns:ct=\""} + namespace_uri +
             "\"><ct:Override PartName=\"/word/document.xml\" "
             "ContentType=\"application/vnd.openxmlformats-officedocument."
             "wordprocessingml.document.main+xml\"><ct:Default "
             "Extension=\"xml\" ContentType=\"application/xml\"/>"
             "</ct:Override></ct:Types>"},
        {"non-whitespace text", std::string{"<ct:Types xmlns:ct=\""} +
                                    namespace_uri + "\">unexpected text" +
                                    valid_main + "</ct:Types>"},
        {"invalid child attribute",
         std::string{"<ct:Types xmlns:ct=\""} + namespace_uri +
             "\"><ct:Default Extension=\"xml\" "
             "ContentType=\"application/xml\" Extra=\"invalid\"/>" +
             valid_main + "</ct:Types>"},
        {"invalid root attribute", std::string{"<ct:Types xmlns:ct=\""} +
                                       namespace_uri + "\" Extra=\"invalid\">" +
                                       valid_main + "</ct:Types>"},
    };

    for (const auto &invalid_document : invalid_documents) {
        const auto &case_name = invalid_document.first;
        const auto &content_types = invalid_document.second;
        INFO(case_name);
        write_test_archive_entries(
            path, {{test_content_types_xml_entry, content_types},
                   {test_relationships_xml_entry, test_relationships_xml},
                   {test_document_xml_entry, valid_document_xml}});

        featherdoc::Document strict_document(path);
        CHECK_EQ(strict_document.open(),
                 featherdoc::document_errc::invalid_package_structure);
        CHECK_FALSE(strict_document.is_open());

        featherdoc::document_open_options options;
        options.validation = featherdoc::package_validation_mode::tolerant;
        featherdoc::Document tolerant_document(path);
        REQUIRE_FALSE(tolerant_document.open(options));
        CHECK(tolerant_document.is_open());
        CHECK(std::ranges::any_of(
            tolerant_document.package_diagnostics(), [](const auto &item) {
                return item.code == featherdoc::package_diagnostic_code::
                                        invalid_content_types_root;
            }));
        CHECK_FALSE(tolerant_document.enable_update_fields_on_open());
        CHECK_EQ(tolerant_document.last_error().code,
                 featherdoc::document_errc::invalid_package_structure);
        CHECK_EQ(tolerant_document.last_error().entry_name,
                 test_content_types_xml_entry);
        REQUIRE_FALSE(tolerant_document.save_as(output));
        CHECK_EQ(read_test_docx_entry(output, test_content_types_xml_entry),
                 content_types);
        CHECK_FALSE(test_docx_entry_exists(output, "word/settings.xml"));
    }

    fs::remove(path);
    fs::remove(output);
}

TEST_CASE("logical duplicate prefixed Content Types declarations fail "
          "closed") {
    namespace fs = std::filesystem;
    const auto path =
        fs::current_path() / "duplicate_prefixed_content_types.docx";
    constexpr auto content_types = R"(
<ct:Types xmlns:ct="http://schemas.openxmlformats.org/package/2006/content-types"
          xmlns:other="http://schemas.openxmlformats.org/package/2006/content-types">
  <ct:Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <ct:Override PartName="/customXml/item.xml" ContentType="application/xml"/>
  <other:Override PartName="/CUSTOMXML/ITEM.XML" ContentType="application/vnd.example.custom+xml"/>
</ct:Types>)";
    write_test_archive_entries(
        path, {{test_content_types_xml_entry, content_types},
               {test_relationships_xml_entry, test_relationships_xml},
               {test_document_xml_entry, valid_document_xml}});

    for (const auto mode : {featherdoc::package_validation_mode::strict,
                            featherdoc::package_validation_mode::tolerant}) {
        featherdoc::document_open_options options;
        options.validation = mode;
        featherdoc::Document document(path);
        CHECK_EQ(document.open(options),
                 featherdoc::document_errc::invalid_package_structure);
        CHECK_FALSE(document.is_open());
        CHECK_NE(document.last_error().detail.find(
                     "duplicate logical Override PartName"),
                 std::string::npos);
    }

    fs::remove(path);
}

TEST_CASE("package repair appends a missing main Content Types declaration "
          "with the root prefix") {
    namespace fs = std::filesystem;
    const auto source =
        fs::current_path() / "repair_prefixed_content_types.docx";
    const auto output =
        fs::current_path() / "repair_prefixed_content_types_saved.docx";
    constexpr auto content_types = R"(
<ct:Types xmlns:ct="http://schemas.openxmlformats.org/package/2006/content-types">
  <ct:Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <ct:Default Extension="xml" ContentType="application/xml"/>
</ct:Types>)";
    write_test_archive_entries(
        source, {{test_content_types_xml_entry, content_types},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, valid_document_xml}});

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open(options));
    const auto report = document.repair_package();
    REQUIRE(report.has_value());
    REQUIRE(report->changed());
    REQUIRE_FALSE(document.save_as(output));

    const auto saved_content_types =
        read_test_docx_entry(output, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find(
                 "<ct:Override PartName=\"/word/document.xml\""),
             std::string::npos);
    CHECK_EQ(saved_content_types.find("<Override "), std::string::npos);

    featherdoc::Document reopened(output);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.is_open());

    fs::remove(source);
    fs::remove(output);
}

TEST_CASE("custom review singleton targets survive mutation save and reopen") {
    namespace fs = std::filesystem;
    const auto source =
        fs::current_path() / "custom_review_singleton_targets.docx";
    const auto output =
        fs::current_path() / "custom_review_singleton_targets_saved.docx";
    constexpr auto footnotes_entry = "word/review/custom-footnotes.xml";
    constexpr auto endnotes_entry = "word/review/custom-endnotes.xml";
    constexpr auto comments_entry = "word/review/custom-comments.xml";
    constexpr auto comments_extended_entry =
        "word/review/custom-comments-extended.xml";

    write_docx_with_related_xml_parts(
        source,
        {{"rFootnotes", footnotes_relationship_type,
          "review/custom-footnotes.xml", "/word/review/custom-footnotes.xml",
          footnotes_content_type, footnotes_entry,
          std::string{"<w:footnotes xmlns:w=\""} +
              wordprocessingml_namespace_uri +
              "\"><w:footnote w:id=\"1\"><w:p><w:r><w:t>old "
              "footnote</w:t></w:r></w:p></w:footnote></w:footnotes>"},
         {"rEndnotes", endnotes_relationship_type, "review/custom-endnotes.xml",
          "/word/review/custom-endnotes.xml", endnotes_content_type,
          endnotes_entry,
          std::string{"<w:endnotes xmlns:w=\""} +
              wordprocessingml_namespace_uri +
              "\"><w:endnote w:id=\"1\"><w:p><w:r><w:t>old "
              "endnote</w:t></w:r></w:p></w:endnote></w:endnotes>"},
         {"rComments", comments_relationship_type, "review/custom-comments.xml",
          "/word/review/custom-comments.xml", comments_content_type,
          comments_entry,
          std::string{"<w:comments xmlns:w=\""} +
              wordprocessingml_namespace_uri +
              "\"><w:comment w:id=\"0\" w:author=\"author\"><w:p><w:r>"
              "<w:t>old comment</w:t></w:r></w:p></w:comment></w:comments>"},
         {"rCommentsExtended", comments_extended_relationship_type,
          "review/custom-comments-extended.xml",
          "/word/review/custom-comments-extended.xml",
          comments_extended_content_type, comments_extended_entry,
          std::string{"<w15:commentsEx xmlns:w15=\""} +
              comments_extended_namespace_uri + "\"/>"}});

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE_EQ(document.list_footnotes().size(), 1U);
    REQUIRE_EQ(document.list_endnotes().size(), 1U);
    REQUIRE_EQ(document.list_comments().size(), 1U);
    REQUIRE(document.replace_footnote(0U, "updated footnote 中文"));
    REQUIRE(document.replace_endnote(0U, "updated endnote 日本語"));
    REQUIRE(document.replace_comment(0U, "updated comment 🙂"));
    REQUIRE(document.set_comment_resolved(0U, true));
    REQUIRE_FALSE(document.save_as(output));

    CHECK(test_docx_entry_exists(output, footnotes_entry));
    CHECK(test_docx_entry_exists(output, endnotes_entry));
    CHECK(test_docx_entry_exists(output, comments_entry));
    CHECK(test_docx_entry_exists(output, comments_extended_entry));
    CHECK_FALSE(test_docx_entry_exists(output, "word/footnotes.xml"));
    CHECK_FALSE(test_docx_entry_exists(output, "word/endnotes.xml"));
    CHECK_FALSE(test_docx_entry_exists(output, "word/comments.xml"));
    CHECK_FALSE(test_docx_entry_exists(output, "word/commentsExtended.xml"));
    CHECK_NE(read_test_docx_entry(output, footnotes_entry)
                 .find("updated footnote 中文"),
             std::string::npos);
    CHECK_NE(read_test_docx_entry(output, endnotes_entry)
                 .find("updated endnote 日本語"),
             std::string::npos);
    CHECK_NE(
        read_test_docx_entry(output, comments_entry).find("updated comment 🙂"),
        std::string::npos);
    CHECK_NE(read_test_docx_entry(output, comments_extended_entry)
                 .find("w15:done=\"1\""),
             std::string::npos);

    const auto saved_relationships =
        read_test_docx_entry(output, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("review/custom-footnotes.xml"),
             std::string::npos);
    CHECK_NE(saved_relationships.find("review/custom-endnotes.xml"),
             std::string::npos);
    CHECK_NE(saved_relationships.find("review/custom-comments.xml"),
             std::string::npos);
    CHECK_NE(saved_relationships.find("review/custom-comments-extended.xml"),
             std::string::npos);

    featherdoc::Document reopened(output);
    REQUIRE_FALSE(reopened.open());
    const auto footnotes = reopened.list_footnotes();
    const auto endnotes = reopened.list_endnotes();
    const auto comments = reopened.list_comments();
    REQUIRE_EQ(footnotes.size(), 1U);
    REQUIRE_EQ(endnotes.size(), 1U);
    REQUIRE_EQ(comments.size(), 1U);
    CHECK_EQ(footnotes.front().text, "updated footnote 中文");
    CHECK_EQ(endnotes.front().text, "updated endnote 日本語");
    CHECK_EQ(comments.front().text, "updated comment 🙂");
    CHECK(comments.front().resolved);

    fs::remove(source);
    fs::remove(output);
}

TEST_CASE("singleton part attachment is atomic when relationship metadata is "
          "invalid") {
    namespace fs = std::filesystem;
    const auto source =
        fs::current_path() / "invalid_relationship_metadata_singletons.docx";
    const auto output = fs::current_path() /
                        "invalid_relationship_metadata_singletons_saved.docx";
    const auto invalid_relationships = std::string{
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rBogus" Type="urn:unrelated" Target="keep.bin" TargetMode="Sideways"/></Relationships>)"};
    write_test_archive_entries(
        source, {{test_content_types_xml_entry, test_content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, valid_document_xml},
                 {"word/_rels/document.xml.rels", invalid_relationships}});
    const auto original_content_types =
        read_test_docx_entry(source, test_content_types_xml_entry);

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open(options));

    CHECK_FALSE(document.enable_update_fields_on_open());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::invalid_package_structure);
    CHECK_EQ(document.last_error().entry_name, "word/_rels/document.xml.rels");

    featherdoc::numbering_definition numbering;
    numbering.name = "MustNotAttachNumbering";
    numbering.levels = {featherdoc::numbering_level_definition{
        featherdoc::list_kind::decimal, 1U, 0U, "%1."}};
    CHECK_FALSE(document.ensure_numbering_definition(numbering).has_value());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::invalid_package_structure);
    CHECK_EQ(document.last_error().entry_name, "word/_rels/document.xml.rels");

    featherdoc::paragraph_style_definition style;
    style.name = "Must Not Attach Styles";
    style.is_quick_format = true;
    CHECK_FALSE(document.ensure_paragraph_style("MustNotAttachStyles", style));
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::invalid_package_structure);
    CHECK_EQ(document.last_error().entry_name, "word/_rels/document.xml.rels");

    REQUIRE_FALSE(document.save_as(output));
    CHECK_EQ(read_test_docx_entry(output, "word/_rels/document.xml.rels"),
             invalid_relationships);
    CHECK_EQ(read_test_docx_entry(output, test_content_types_xml_entry),
             original_content_types);
    CHECK_FALSE(test_docx_entry_exists(output, "word/settings.xml"));
    CHECK_FALSE(test_docx_entry_exists(output, "word/numbering.xml"));
    CHECK_FALSE(test_docx_entry_exists(output, "word/styles.xml"));

    fs::remove(source);
    fs::remove(output);
}

TEST_CASE("document relationship identifiers and singleton types fail closed") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "invalid_document_rel_identity.docx";
    const auto check_rejected_in_all_modes =
        [&](std::string_view relationships_xml,
            std::string_view expected_detail,
            std::vector<std::pair<std::string, std::string>> extra_entries =
                {}) {
            std::vector<std::pair<std::string, std::string>> entries{
                {test_content_types_xml_entry, test_content_types_xml},
                {test_relationships_xml_entry, test_relationships_xml},
                {test_document_xml_entry, valid_document_xml},
                {"word/_rels/document.xml.rels",
                 std::string{relationships_xml}},
            };
            entries.insert(entries.end(), extra_entries.begin(),
                           extra_entries.end());
            write_test_archive_entries(path, entries);

            for (const auto mode :
                 {featherdoc::package_validation_mode::strict,
                  featherdoc::package_validation_mode::tolerant}) {
                featherdoc::document_open_options options;
                options.validation = mode;
                featherdoc::Document document(path);
                CHECK_EQ(document.open(options),
                         featherdoc::document_errc::invalid_package_structure);
                CHECK_NE(document.last_error().detail.find(expected_detail),
                         std::string::npos);
            }
        };

    SUBCASE("duplicate Id") {
        check_rejected_in_all_modes(
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rDuplicate" Type="urn:example:first" Target="first.xml"/>
  <Relationship Id="rDuplicate" Type="urn:example:second" Target="second.xml"/>
</Relationships>)",
            "declared more than once");
    }

    SUBCASE("duplicate singleton Type") {
        check_rejected_in_all_modes(
            std::string{
                R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rSettings1" Type=")"} +
                settings_relationship_type + R"(" Target="settings1.xml"/>
  <Relationship Id="rSettings2" Type=")" +
                settings_relationship_type + R"(" Target="settings2.xml"/>
</Relationships>)",
            "singleton document relationship Type",
            {{"word/settings1.xml",
              namespaced_word_root_prefix("w:settings") +
                  namespaced_word_root_suffix("w:settings")},
             {"word/settings2.xml",
              namespaced_word_root_prefix("w:settings") +
                  namespaced_word_root_suffix("w:settings")}});
    }

    fs::remove(path);
}

TEST_CASE("tolerant open diagnoses invalid singleton relationships without "
          "binding or rewriting them") {
    namespace fs = std::filesystem;
    const auto source = fs::current_path() / "invalid_singleton_rel.docx";
    const auto output = fs::current_path() / "invalid_singleton_rel_saved.docx";
    struct invalid_case {
        std::string label;
        std::string relationship_attributes;
        featherdoc::package_diagnostic_code diagnostic;
        bool include_settings_part{false};
    };
    const std::vector<invalid_case> cases{
        {"External", "Target=\"settings.xml\" TargetMode=\"External\"",
         featherdoc::package_diagnostic_code::invalid_document_relationship,
         true},
        {"bad target", "Target=\"settings//settings.xml\"",
         featherdoc::package_diagnostic_code::invalid_document_relationship},
        {"empty target", "Target=\"\"",
         featherdoc::package_diagnostic_code::invalid_document_relationship},
        {"missing target", "",
         featherdoc::package_diagnostic_code::invalid_document_relationship},
        {"dangling target", "Target=\"missing-settings.xml\"",
         featherdoc::package_diagnostic_code::dangling_relationship},
    };

    for (const auto &item : cases) {
        INFO(item.label);
        const auto relationships_xml =
            std::string{
                R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rSettings" Type=")"} +
            settings_relationship_type + "\" " + item.relationship_attributes +
            "/>\n</Relationships>";
        std::vector<std::pair<std::string, std::string>> entries{
            {test_content_types_xml_entry, test_content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, valid_document_xml},
            {"word/_rels/document.xml.rels", relationships_xml},
        };
        if (item.include_settings_part) {
            entries.emplace_back("word/settings.xml",
                                 namespaced_word_root_prefix("w:settings") +
                                     namespaced_word_root_suffix("w:settings"));
        }
        write_test_archive_entries(source, entries);

        featherdoc::Document strict_document(source);
        CHECK_EQ(strict_document.open(),
                 featherdoc::document_errc::invalid_package_structure);

        featherdoc::document_open_options options;
        options.validation = featherdoc::package_validation_mode::tolerant;
        featherdoc::Document tolerant_document(source);
        REQUIRE_FALSE(tolerant_document.open(options));
        CHECK(std::ranges::any_of(
            tolerant_document.package_diagnostics(),
            [&](const auto &diag) { return diag.code == item.diagnostic; }));
        CHECK_FALSE(
            tolerant_document.update_fields_on_open_enabled().value_or(true));
        CHECK_FALSE(tolerant_document.enable_update_fields_on_open());
        CHECK_EQ(tolerant_document.last_error().code,
                 featherdoc::document_errc::invalid_package_structure);
        REQUIRE_FALSE(tolerant_document.save_as(output));
        CHECK_EQ(read_test_docx_entry(output, "word/_rels/document.xml.rels"),
                 relationships_xml);
    }

    fs::remove(source);
    fs::remove(output);
}

TEST_CASE("related part relationships validate their root and namespace") {
    namespace fs = std::filesystem;
    const auto source = fs::current_path() / "invalid_header_rels_root.docx";
    const auto output = fs::current_path() / "invalid_header_rels_saved.docx";
    write_docx_with_related_xml_parts(
        source, {{"rHeader", header_relationship_type, "header1.xml",
                  "/word/header1.xml", header_content_type, "word/header1.xml",
                  namespaced_word_root_prefix("w:hdr") +
                      namespaced_word_root_suffix("w:hdr")}});
    auto entries = read_test_archive_entries(source);
    for (auto &[entry_name, content] : entries) {
        if (entry_name == test_document_xml_entry) {
            content =
                R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
                               xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body><w:p/><w:sectPr><w:headerReference w:type="default" r:id="rHeader"/></w:sectPr></w:body>
</w:document>)";
        }
    }
    const auto invalid_relationships = std::string{
        R"(<BrokenRelationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"/>)"};
    entries.emplace_back("word/_rels/header1.xml.rels", invalid_relationships);
    write_test_archive_entries(source, entries);

    featherdoc::Document strict_document(source);
    CHECK_EQ(strict_document.open(),
             featherdoc::document_errc::invalid_package_structure);
    CHECK_EQ(strict_document.last_error().entry_name,
             "word/_rels/header1.xml.rels");

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document tolerant_document(source);
    REQUIRE_FALSE(tolerant_document.open(options));
    CHECK_EQ(tolerant_document.header_count(), 1U);
    CHECK(std::ranges::any_of(
        tolerant_document.package_diagnostics(), [](const auto &diag) {
            return diag.code == featherdoc::package_diagnostic_code::
                                    invalid_relationships_part;
        }));
    REQUIRE_FALSE(tolerant_document.save_as(output));
    const auto saved_relationships =
        read_test_docx_entry(output, "word/_rels/header1.xml.rels");
    CHECK(saved_relationships.find("BrokenRelationships") != std::string::npos);
    CHECK(saved_relationships.find(
              "http://schemas.openxmlformats.org/package/2006/relationships") !=
          std::string::npos);

    fs::remove(source);
    fs::remove(output);
}

TEST_CASE("related part duplicate relationship identifiers remain read-only "
          "in tolerant mode") {
    namespace fs = std::filesystem;
    const auto source =
        fs::current_path() / "duplicate_header_rels_identity.docx";
    const auto output =
        fs::current_path() / "duplicate_header_rels_identity_saved.docx";
    write_docx_with_related_xml_parts(
        source, {{"rHeader", header_relationship_type, "header1.xml",
                  "/word/header1.xml", header_content_type, "word/header1.xml",
                  namespaced_word_root_prefix("w:hdr") +
                      namespaced_word_root_suffix("w:hdr")}});
    auto entries = read_test_archive_entries(source);
    for (auto &[entry_name, content] : entries) {
        if (entry_name == test_document_xml_entry) {
            content =
                R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"><w:body><w:p/><w:sectPr><w:headerReference w:type="default" r:id="rHeader"/></w:sectPr></w:body></w:document>)";
        }
    }
    const auto duplicate_relationships = std::string{
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="duplicate" Type="urn:first" Target="first.bin"/>
  <Relationship Id="duplicate" Type="urn:second" Target="second.bin"/>
</Relationships>)"};
    entries.emplace_back("word/_rels/header1.xml.rels",
                         duplicate_relationships);
    write_test_archive_entries(source, entries);

    featherdoc::Document strict_document(source);
    CHECK_EQ(strict_document.open(),
             featherdoc::document_errc::invalid_package_structure);

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open(options));
    REQUIRE_EQ(document.header_count(), 1U);
    CHECK_EQ(document.header_template().append_hyperlink("blocked",
                                                         "https://example.com"),
             0U);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::invalid_package_structure);
    CHECK_EQ(document.last_error().entry_name, "word/_rels/header1.xml.rels");

    REQUIRE_FALSE(document.save_as(output));
    const auto saved_relationships =
        read_test_docx_entry(output, "word/_rels/header1.xml.rels");
    const auto first_duplicate = saved_relationships.find("Id=\"duplicate\"");
    REQUIRE_NE(first_duplicate, std::string::npos);
    CHECK_NE(saved_relationships.find("Id=\"duplicate\"", first_duplicate + 1U),
             std::string::npos);
    CHECK_EQ(saved_relationships.find("relationships/hyperlink"),
             std::string::npos);

    fs::remove(source);
    fs::remove(output);
}

TEST_CASE("tolerant open blocks document relationship mutations when the "
          "relationships root is invalid") {
    namespace fs = std::filesystem;
    const auto source =
        fs::current_path() / "invalid_document_rels_mutation.docx";
    const auto output =
        fs::current_path() / "invalid_document_rels_mutation_saved.docx";
    const auto image_path =
        fs::current_path() / "invalid_document_rels_append.png";
    const auto invalid_relationships = std::string{
        R"(<?xml version="1.0" encoding="UTF-8"?>
<BrokenRelationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" marker="preserve-me">
  <Relationship Id="sentinel" Type="urn:sentinel" Target="keep.bin"/>
</BrokenRelationships>
)"};
    write_test_archive_entries(
        source, {{test_content_types_xml_entry, test_content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, valid_document_xml},
                 {"word/_rels/document.xml.rels", invalid_relationships}});
    write_binary_file(image_path, tiny_png_data());

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open(options));
    CHECK(std::ranges::any_of(
        document.package_diagnostics(), [](const auto &diagnostic) {
            return diagnostic.code == featherdoc::package_diagnostic_code::
                                          invalid_relationships_part;
        }));

    CHECK_EQ(document.append_hyperlink("blocked", "https://example.com"), 0U);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::invalid_package_structure);
    CHECK_EQ(document.last_error().entry_name, "word/_rels/document.xml.rels");

    CHECK_FALSE(document.enable_update_fields_on_open());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::invalid_package_structure);
    CHECK_EQ(document.last_error().entry_name, "word/_rels/document.xml.rels");

    CHECK_FALSE(document.append_image(image_path));
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::invalid_package_structure);
    CHECK_EQ(document.last_error().entry_name, "word/_rels/document.xml.rels");

    REQUIRE_FALSE(document.save_as(output));
    CHECK_EQ(read_test_docx_entry(output, "word/_rels/document.xml.rels"),
             invalid_relationships);
    const auto saved_document =
        read_test_docx_entry(output, test_document_xml_entry);
    CHECK_NE(saved_document.find("secure"), std::string::npos);
    CHECK_EQ(saved_document.find("<w:drawing"), std::string::npos);
    CHECK_EQ(read_test_docx_entry(output, test_content_types_xml_entry),
             test_content_types_xml);
    const auto saved_entries = read_test_archive_entries(output);
    CHECK(std::ranges::none_of(saved_entries, [](const auto &entry) {
        return std::string_view{entry.first}.starts_with("word/media/");
    }));

    fs::remove(source);
    fs::remove(output);
    fs::remove(image_path);
}

TEST_CASE("content control image replacement is atomic when document "
          "relationships are invalid") {
    namespace fs = std::filesystem;
    const auto source =
        fs::current_path() / "invalid_document_rels_content_control.docx";
    const auto output =
        fs::current_path() / "invalid_document_rels_content_control_saved.docx";
    const auto image_path =
        fs::current_path() / "invalid_document_rels_content_control.png";
    const auto invalid_relationships = std::string{
        R"(<BrokenRelationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" marker="preserve-me">
  <Relationship Id="sentinel" Type="urn:sentinel" Target="keep.bin"/>
</BrokenRelationships>)"};
    const auto document_xml = std::string{
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:sdt>
      <w:sdtPr><w:tag w:val="protected-image"/></w:sdtPr>
      <w:sdtContent><w:p><w:r><w:t>must remain</w:t></w:r></w:p></w:sdtContent>
    </w:sdt>
  </w:body>
</w:document>)"};
    write_test_archive_entries(
        source, {{test_content_types_xml_entry, test_content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, document_xml},
                 {"word/_rels/document.xml.rels", invalid_relationships}});
    write_binary_file(image_path, tiny_png_data());

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open(options));

    CHECK_EQ(document.replace_content_control_with_image_by_tag(
                 "protected-image", image_path),
             0U);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::invalid_package_structure);
    CHECK_EQ(document.last_error().entry_name, "word/_rels/document.xml.rels");

    REQUIRE_FALSE(document.save_as(output));
    const auto saved_document_xml =
        read_test_docx_entry(output, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("must remain"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("<w:drawing"), std::string::npos);
    CHECK_EQ(read_test_docx_entry(output, "word/_rels/document.xml.rels"),
             invalid_relationships);

    fs::remove(source);
    fs::remove(output);
    fs::remove(image_path);
}

TEST_CASE("tolerant open blocks header and footer relationship mutations "
          "when their relationships roots are invalid") {
    namespace fs = std::filesystem;
    const auto source =
        fs::current_path() / "invalid_related_rels_mutation.docx";
    const auto output =
        fs::current_path() / "invalid_related_rels_mutation_saved.docx";
    const auto image_path =
        fs::current_path() / "invalid_header_rels_content_control.png";
    const auto header_xml = std::string{
        R"(<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:sdt>
    <w:sdtPr><w:tag w:val="protected-header-image"/></w:sdtPr>
    <w:sdtContent><w:p><w:r><w:t>header placeholder must remain</w:t></w:r></w:p></w:sdtContent>
  </w:sdt>
</w:hdr>)"};
    write_docx_with_related_xml_parts(
        source, {{"rHeader", header_relationship_type, "header1.xml",
                  "/word/header1.xml", header_content_type, "word/header1.xml",
                  header_xml},
                 {"rFooter", footer_relationship_type, "footer1.xml",
                  "/word/footer1.xml", footer_content_type, "word/footer1.xml",
                  namespaced_word_root_prefix("w:ftr") +
                      namespaced_word_root_suffix("w:ftr")}});

    auto entries = read_test_archive_entries(source);
    for (auto &[entry_name, content] : entries) {
        if (entry_name == test_document_xml_entry) {
            content =
                R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
                               xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body><w:p/><w:sectPr>
    <w:headerReference w:type="default" r:id="rHeader"/>
    <w:footerReference w:type="default" r:id="rFooter"/>
  </w:sectPr></w:body>
</w:document>)";
        }
    }
    const auto invalid_header_relationships = std::string{
        R"(<BrokenRelationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" marker="preserve-header">
  <Relationship Id="header-sentinel" Type="urn:sentinel" Target="keep-header.bin"/>
</BrokenRelationships>)"};
    const auto invalid_footer_relationships = std::string{
        R"(<BrokenRelationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" marker="preserve-footer">
  <Relationship Id="footer-sentinel" Type="urn:sentinel" Target="keep-footer.bin"/>
</BrokenRelationships>)"};
    entries.emplace_back("word/_rels/header1.xml.rels",
                         invalid_header_relationships);
    entries.emplace_back("word/_rels/footer1.xml.rels",
                         invalid_footer_relationships);
    write_test_archive_entries(source, entries);
    write_binary_file(image_path, tiny_png_data());
    const auto original_content_types =
        read_test_docx_entry(source, test_content_types_xml_entry);

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open(options));
    CHECK_EQ(document.header_count(), 1U);
    CHECK_EQ(document.footer_count(), 1U);

    auto header = document.header_template();
    CHECK_EQ(header.replace_content_control_with_image_by_tag(
                 "protected-header-image", image_path),
             0U);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::invalid_package_structure);
    CHECK_EQ(document.last_error().entry_name, "word/_rels/header1.xml.rels");
    CHECK_EQ(
        header.append_hyperlink("blocked header", "https://example.com/header"),
        0U);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::invalid_package_structure);
    CHECK_EQ(document.last_error().entry_name, "word/_rels/header1.xml.rels");

    auto footer = document.footer_template();
    CHECK_EQ(
        footer.append_hyperlink("blocked footer", "https://example.com/footer"),
        0U);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::invalid_package_structure);
    CHECK_EQ(document.last_error().entry_name, "word/_rels/footer1.xml.rels");

    REQUIRE_FALSE(document.save_as(output));
    const auto check_preserved_relationships =
        [&](std::string_view entry_name, std::string_view expected_root,
            std::string_view expected_namespace,
            std::string_view expected_marker,
            std::string_view expected_sentinel_id) {
            const auto terminated_entry_name = std::string{entry_name};
            const auto saved_xml =
                read_test_docx_entry(output, terminated_entry_name.c_str());
            pugi::xml_document saved_relationships;
            REQUIRE(saved_relationships.load_buffer(saved_xml.data(),
                                                    saved_xml.size()));
            const auto root = saved_relationships.document_element();
            REQUIRE(root != pugi::xml_node{});
            CHECK_EQ(std::string_view{root.name()}, expected_root);
            CHECK_EQ(std::string_view{root.attribute("xmlns").value()},
                     expected_namespace);
            CHECK_EQ(std::string_view{root.attribute("marker").value()},
                     expected_marker);
            const auto sentinel = root.child("Relationship");
            REQUIRE(sentinel != pugi::xml_node{});
            CHECK_EQ(std::string_view{sentinel.attribute("Id").value()},
                     expected_sentinel_id);
            CHECK_EQ(sentinel.next_sibling("Relationship"), pugi::xml_node{});
            CHECK_EQ(saved_xml.find("relationships/hyperlink"),
                     std::string::npos);
        };
    check_preserved_relationships(
        "word/_rels/header1.xml.rels", "BrokenRelationships",
        "http://schemas.openxmlformats.org/package/2006/relationships",
        "preserve-header", "header-sentinel");
    check_preserved_relationships(
        "word/_rels/footer1.xml.rels", "BrokenRelationships",
        "http://schemas.openxmlformats.org/package/2006/relationships",
        "preserve-footer", "footer-sentinel");
    const auto saved_header = read_test_docx_entry(output, "word/header1.xml");
    CHECK_NE(saved_header.find("header placeholder must remain"),
             std::string::npos);
    CHECK_EQ(saved_header.find("<w:drawing"), std::string::npos);
    CHECK_EQ(read_test_docx_entry(output, test_content_types_xml_entry),
             original_content_types);
    const auto saved_entries = read_test_archive_entries(output);
    CHECK(std::ranges::none_of(saved_entries, [](const auto &entry) {
        return std::string_view{entry.first}.starts_with("word/media/");
    }));

    fs::remove(source);
    fs::remove(output);
    fs::remove(image_path);
}

TEST_CASE("Relationships MCE failures fail closed in strict and tolerant "
          "open modes at every eager package boundary") {
    namespace fs = std::filesystem;
    constexpr auto mismatch_root_relationships = R"(
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"
               xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"
               xmlns:x="urn:unsupported" mc:MustUnderstand="x">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
</Relationships>)";
    constexpr auto wrong_namespace_relationships = R"(
<Relationships xmlns="urn:unsupported">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
</Relationships>)";
    constexpr auto invalid_mce_root_relationships = R"(
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"
               xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"
               mc:ProcessContent="malformed">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
</Relationships>)";
    const auto check_modes = [](const fs::path &path,
                                std::string_view expected_entry,
                                featherdoc::document_errc expected_error) {
        for (const auto mode :
             {featherdoc::package_validation_mode::strict,
              featherdoc::package_validation_mode::tolerant}) {
            featherdoc::document_open_options options;
            options.validation = mode;
            featherdoc::Document document(path);
            CHECK_EQ(document.open(options), expected_error);
            CHECK_FALSE(document.is_open());
            CHECK_EQ(document.last_error().entry_name, expected_entry);
        }
    };

    const auto root_path = fs::current_path() / "root_relationships_mce.docx";
    write_test_archive_entries(
        root_path, {{test_content_types_xml_entry, test_content_types_xml},
                    {test_relationships_xml_entry, mismatch_root_relationships},
                    {test_document_xml_entry, valid_document_xml}});
    check_modes(root_path, test_relationships_xml_entry,
                featherdoc::document_errc::mce_mismatch);

    const auto invalid_mce_root_path =
        fs::current_path() / "invalid_root_relationships_mce.docx";
    write_test_archive_entries(
        invalid_mce_root_path,
        {{test_content_types_xml_entry, test_content_types_xml},
         {test_relationships_xml_entry, invalid_mce_root_relationships},
         {test_document_xml_entry, valid_document_xml}});
    check_modes(invalid_mce_root_path, test_relationships_xml_entry,
                featherdoc::document_errc::invalid_mce_markup);

    const auto wrong_namespace_path =
        fs::current_path() / "wrong_namespace_relationships_mce.docx";
    write_test_archive_entries(
        wrong_namespace_path,
        {{test_content_types_xml_entry, test_content_types_xml},
         {test_relationships_xml_entry, wrong_namespace_relationships},
         {test_document_xml_entry, valid_document_xml}});
    check_modes(wrong_namespace_path, test_relationships_xml_entry,
                featherdoc::document_errc::mce_mismatch);

    const auto document_path =
        fs::current_path() / "document_relationships_mce.docx";
    write_test_archive_entries(
        document_path,
        {{test_content_types_xml_entry, test_content_types_xml},
         {test_relationships_xml_entry, test_relationships_xml},
         {test_document_xml_entry, valid_document_xml},
         {"word/_rels/document.xml.rels", mismatch_root_relationships}});
    check_modes(document_path, "word/_rels/document.xml.rels",
                featherdoc::document_errc::mce_mismatch);

    const auto header_path =
        fs::current_path() / "header_relationships_mce.docx";
    const auto header_content_types = std::string{R"(
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
</Types>)"};
    const auto document_with_header = std::string{R"(
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body><w:p/><w:sectPr><w:headerReference w:type="default" r:id="rHeader"/></w:sectPr></w:body>
</w:document>)"};
    const auto document_relationships = std::string{R"(
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rHeader" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header" Target="header1.xml"/>
</Relationships>)"};
    write_test_archive_entries(
        header_path,
        {{test_content_types_xml_entry, header_content_types},
         {test_relationships_xml_entry, test_relationships_xml},
         {test_document_xml_entry, document_with_header},
         {"word/_rels/document.xml.rels", document_relationships},
         {"word/header1.xml", namespaced_word_root_prefix("w:hdr") +
                                  namespaced_word_root_suffix("w:hdr")},
         {"word/_rels/header1.xml.rels", mismatch_root_relationships}});
    check_modes(header_path, "word/_rels/header1.xml.rels",
                featherdoc::document_errc::mce_mismatch);

    fs::remove(root_path);
    fs::remove(invalid_mce_root_path);
    fs::remove(wrong_namespace_path);
    fs::remove(document_path);
    fs::remove(header_path);
}

TEST_CASE("legal Relationships MCE is sanitized before iterators and dirty "
          "relationship saves") {
    namespace fs = std::filesystem;
    const auto source = fs::current_path() / "legal_relationships_mce.docx";
    const auto output =
        fs::current_path() / "legal_relationships_mce_saved.docx";
    const auto content_types = std::string{R"(
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
</Types>)"};
    const auto root_relationships = std::string{R"(
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"
               xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"
               xmlns:x="urn:extension" mc:Ignorable="x" x:discard="yes">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
</Relationships>)"};
    const auto document_xml = std::string{R"(
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body><w:p/><w:sectPr><w:headerReference w:type="default" r:id="rHeader"/></w:sectPr></w:body>
</w:document>)"};
    const auto document_relationships = std::string{R"(
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"
               xmlns:rel="http://schemas.openxmlformats.org/package/2006/relationships"
               xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006">
  <mc:AlternateContent>
    <mc:Choice Requires="rel">
      <Relationship Id="rHeader" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header" Target="header1.xml"/>
    </mc:Choice>
  </mc:AlternateContent>
</Relationships>)"};
    const auto header_relationships =
        std::string{R"(<?xml version="1.0" encoding="UTF-8"?>
<!--关系注释-->
<?featherdoc 中文处理?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"
               xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"
               xmlns:x="urn:extension" mc:Ignorable="x">
  <x:Ignored mc:MustUnderstand="x"/>
</Relationships>)"};
    write_test_archive_entries(
        source, {{test_content_types_xml_entry, content_types},
                 {test_relationships_xml_entry, root_relationships},
                 {test_document_xml_entry, document_xml},
                 {"word/_rels/document.xml.rels", document_relationships},
                 {"word/header1.xml", namespaced_word_root_prefix("w:hdr") +
                                          namespaced_word_root_suffix("w:hdr")},
                 {"word/_rels/header1.xml.rels", header_relationships}});

    for (const auto mode : {featherdoc::package_validation_mode::strict,
                            featherdoc::package_validation_mode::tolerant}) {
        featherdoc::document_open_options options;
        options.validation = mode;
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open(options));
        CHECK_EQ(document.header_count(), 1U);
        auto header = document.header_template();
        REQUIRE_NE(header.append_hyperlink("中文链接", "https://example.test"),
                   0U);
        REQUIRE_FALSE(document.save_as(output));
        const auto saved_header_relationships =
            read_test_docx_entry(output, "word/_rels/header1.xml.rels");
        CHECK_EQ(saved_header_relationships.find("<x:Ignored"),
                 std::string::npos);
        CHECK_EQ(saved_header_relationships.find("mc:Ignorable"),
                 std::string::npos);
        CHECK_NE(saved_header_relationships.find("<?xml version=\"1.0\""),
                 std::string::npos);
        CHECK_NE(saved_header_relationships.find("<!--关系注释-->"),
                 std::string::npos);
        CHECK_NE(saved_header_relationships.find("<?featherdoc 中文处理?>"),
                 std::string::npos);
        CHECK_NE(saved_header_relationships.find("relationships/hyperlink"),
                 std::string::npos);
    }

    fs::remove(source);
    fs::remove(output);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "header and footer image relationship initialization is atomic on "
    "pugixml allocation failure") {
    namespace fs = std::filesystem;

    for (const bool use_header : {true, false}) {
        const auto part_label = std::string{use_header ? "header" : "footer"};
        CAPTURE(part_label);

        const auto target =
            fs::current_path() /
            (part_label + "_image_relationship_initialization_oom.docx");
        const auto image_path =
            fs::current_path() /
            (part_label + "_image_relationship_initialization_oom.png");
        const auto part_entry =
            std::string{use_header ? "word/header1.xml" : "word/footer1.xml"};
        const auto relationships_entry =
            std::string{use_header ? "word/_rels/header1.xml.rels"
                                   : "word/_rels/footer1.xml.rels"};
        const auto expected_text =
            std::string{use_header ? "header before OOM" : "footer before OOM"};

        fs::remove(target);
        fs::remove(image_path);
        write_test_docx_with_header_footer(target, "body", "header before OOM",
                                           "footer before OOM");
        write_binary_file(image_path, tiny_png_data());
        REQUIRE_FALSE(
            test_docx_entry_exists(target, relationships_entry.c_str()));
        const auto original_part_xml =
            read_test_docx_entry(target, part_entry.c_str());
        const auto normalized_original_part =
            normalized_xml_document_element(original_part_xml);
        REQUIRE(normalized_original_part.has_value());
        const auto original_content_types =
            read_test_docx_entry(target, test_content_types_xml_entry);

        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        auto template_part = use_header ? document.section_header_template(0U)
                                        : document.section_footer_template(0U);
        auto paragraph = use_header ? document.section_header_paragraphs(0U)
                                    : document.section_footer_paragraphs(0U);
        REQUIRE(static_cast<bool>(template_part));
        REQUIRE(paragraph.has_next());
        REQUIRE_EQ(paragraph.runs().get_text(), expected_text);
        REQUIRE(template_part.inline_images().empty());

        pugi_memory_management_guard allocation_guard;
        delegated_pugi_allocate = allocation_guard.allocation;
        pugi::set_memory_management_functions(controlled_pugi_allocate,
                                              allocation_guard.deallocation);
        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = 1U;

        CHECK_FALSE(template_part.append_image(image_path));
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK_EQ(document.last_error().entry_name, relationships_entry);
        CHECK_EQ(controlled_pugi_allocation_calls, 1U);
        controlled_pugi_failure_call = 0U;

        CHECK(static_cast<bool>(template_part));
        CHECK(paragraph.has_next());
        CHECK_EQ(paragraph.runs().get_text(), expected_text);
        CHECK(template_part.inline_images().empty());

        // Saving the failure state proves that neither the relationship-part
        // presence/dirty flags nor the WML/content-types DOMs were published.
        REQUIRE_FALSE(document.save());
        CHECK_FALSE(
            test_docx_entry_exists(target, relationships_entry.c_str()));
        CHECK_FALSE(test_docx_entry_exists(target, "word/media/image1.png"));
        const auto normalized_saved_part = normalized_xml_document_element(
            read_test_docx_entry(target, part_entry.c_str()));
        REQUIRE(normalized_saved_part.has_value());
        CHECK_EQ(*normalized_saved_part, *normalized_original_part);
        CHECK_EQ(read_test_docx_entry(target, test_content_types_xml_entry),
                 original_content_types);

        // The same pre-existing handles remain usable after allocator restore.
        REQUIRE(template_part.append_image(image_path));
        REQUIRE_FALSE(document.save());
        CHECK(test_docx_entry_exists(target, relationships_entry.c_str()));
        CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
        const auto saved_relationships =
            read_test_docx_entry(target, relationships_entry.c_str());
        CHECK_NE(saved_relationships.find("relationships/image"),
                 std::string::npos);

        featherdoc::Document reopened(target);
        REQUIRE_FALSE(reopened.open());
        auto reopened_part = use_header ? reopened.section_header_template(0U)
                                        : reopened.section_footer_template(0U);
        REQUIRE(static_cast<bool>(reopened_part));
        REQUIRE_EQ(reopened_part.inline_images().size(), 1U);

        fs::remove(target);
        fs::remove(image_path);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "related-part hyperlink relationship initialization is atomic on "
    "pugixml allocation failure") {
    namespace fs = std::filesystem;

    const auto target = fs::current_path() /
                        "header_hyperlink_relationship_initialization_oom.docx";
    constexpr auto part_entry = "word/header1.xml";
    constexpr auto relationships_entry = "word/_rels/header1.xml.rels";
    constexpr auto target_uri = "https://example.test/atomic-retry";
    fs::remove(target);
    write_test_docx_with_header_footer(target, "body", "header before OOM",
                                       "footer");
    REQUIRE_FALSE(test_docx_entry_exists(target, relationships_entry));
    const auto original_header_xml = read_test_docx_entry(target, part_entry);
    const auto normalized_original_header =
        normalized_xml_document_element(original_header_xml);
    REQUIRE(normalized_original_header.has_value());
    const auto original_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    auto header = document.section_header_template(0U);
    auto paragraph = document.section_header_paragraphs(0U);
    REQUIRE(static_cast<bool>(header));
    REQUIRE(paragraph.has_next());
    REQUIRE_EQ(paragraph.runs().get_text(), "header before OOM");
    REQUIRE(header.list_hyperlinks().empty());

    pugi_memory_management_guard allocation_guard;
    delegated_pugi_allocate = allocation_guard.allocation;
    pugi::set_memory_management_functions(controlled_pugi_allocate,
                                          allocation_guard.deallocation);
    controlled_pugi_allocation_calls = 0U;
    controlled_pugi_failure_call = 1U;

    CHECK_EQ(header.append_hyperlink("atomic link", target_uri), 0U);
    CHECK_EQ(document.last_error().code,
             std::make_error_code(std::errc::not_enough_memory));
    CHECK_EQ(document.last_error().entry_name, relationships_entry);
    CHECK_EQ(controlled_pugi_allocation_calls, 1U);
    controlled_pugi_failure_call = 0U;

    CHECK(static_cast<bool>(header));
    CHECK(paragraph.has_next());
    CHECK_EQ(paragraph.runs().get_text(), "header before OOM");
    CHECK(header.list_hyperlinks().empty());

    REQUIRE_FALSE(document.save());
    CHECK_FALSE(test_docx_entry_exists(target, relationships_entry));
    const auto normalized_saved_header = normalized_xml_document_element(
        read_test_docx_entry(target, part_entry));
    REQUIRE(normalized_saved_header.has_value());
    CHECK_EQ(*normalized_saved_header, *normalized_original_header);
    CHECK_EQ(read_test_docx_entry(target, test_content_types_xml_entry),
             original_content_types);

    REQUIRE_EQ(header.append_hyperlink("atomic link", target_uri), 1U);
    REQUIRE_FALSE(document.save());
    REQUIRE(test_docx_entry_exists(target, relationships_entry));
    const auto saved_relationships =
        read_test_docx_entry(target, relationships_entry);
    CHECK_NE(saved_relationships.find("relationships/hyperlink"),
             std::string::npos);
    CHECK_NE(saved_relationships.find("TargetMode=\"External\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    const auto links = reopened.section_header_template(0U).list_hyperlinks();
    REQUIRE_EQ(links.size(), 1U);
    CHECK_EQ(links.front().text, "atomic link");
    REQUIRE(links.front().target.has_value());
    CHECK_EQ(*links.front().target, target_uri);

    fs::remove(target);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "create_empty settings mutation is atomic for every pugixml "
    "allocation failure") {
    namespace fs = std::filesystem;

    pugi_memory_management_guard allocation_guard;
    delegated_pugi_allocate = allocation_guard.allocation;
    pugi::set_memory_management_functions(controlled_pugi_allocate,
                                          allocation_guard.deallocation);

    std::size_t successful_allocation_count = 0U;
    {
        const auto baseline_path =
            fs::current_path() / "settings_attachment_oom_baseline.docx";
        fs::remove(baseline_path);
        featherdoc::Document baseline(baseline_path);
        REQUIRE_FALSE(baseline.create_empty());
        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = 0U;
        REQUIRE(baseline.enable_update_fields_on_open());
        successful_allocation_count = controlled_pugi_allocation_calls;
        REQUIRE_GT(successful_allocation_count, 0U);
        fs::remove(baseline_path);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        const auto target =
            fs::current_path() / ("settings_attachment_oom_" +
                                  std::to_string(failure_call) + ".docx");
        fs::remove(target);

        featherdoc::Document document(target);
        controlled_pugi_failure_call = 0U;
        REQUIRE_FALSE(document.create_empty());
        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = failure_call;

        CHECK_FALSE(document.enable_update_fields_on_open());
        const auto mutation_error = document.last_error();
        controlled_pugi_failure_call = 0U;
        CHECK_EQ(mutation_error.code,
                 std::make_error_code(std::errc::not_enough_memory));

        // Saving the failed state proves that no relationship, override,
        // dirty flag, or settings part escaped the failed mutation.
        REQUIRE_FALSE(document.save());
        CHECK_FALSE(
            test_docx_entry_exists(target, "word/_rels/document.xml.rels"));
        CHECK_FALSE(test_docx_entry_exists(target, "word/settings.xml"));
        const auto failed_content_types =
            read_test_docx_entry(target, test_content_types_xml_entry);
        CHECK_EQ(failed_content_types.find("/word/settings.xml"),
                 std::string::npos);
        CHECK_EQ(failed_content_types.find(settings_content_type),
                 std::string::npos);

        controlled_pugi_allocation_calls = 0U;
        REQUIRE(document.enable_update_fields_on_open());
        REQUIRE_FALSE(document.save());
        CHECK(test_docx_entry_exists(target, "word/_rels/document.xml.rels"));
        CHECK(test_docx_entry_exists(target, "word/settings.xml"));
        CHECK_NE(read_test_docx_entry(target, "word/_rels/document.xml.rels")
                     .find(settings_relationship_type),
                 std::string::npos);

        featherdoc::Document reopened(target);
        REQUIRE_FALSE(reopened.open());
        CHECK(reopened.update_fields_on_open_enabled().value_or(false));
        fs::remove(target);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "create_empty numbering mutation is atomic for every pugixml "
    "allocation failure") {
    namespace fs = std::filesystem;

    featherdoc::numbering_definition definition;
    definition.name = "AtomicFailNthNumbering";
    definition.levels = {featherdoc::numbering_level_definition{
        featherdoc::list_kind::decimal, 1U, 0U, "%1."}};

    pugi_memory_management_guard allocation_guard;
    delegated_pugi_allocate = allocation_guard.allocation;
    pugi::set_memory_management_functions(controlled_pugi_allocate,
                                          allocation_guard.deallocation);

    std::size_t successful_allocation_count = 0U;
    {
        const auto baseline_path =
            fs::current_path() / "numbering_attachment_oom_baseline.docx";
        fs::remove(baseline_path);
        featherdoc::Document baseline(baseline_path);
        REQUIRE_FALSE(baseline.create_empty());
        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = 0U;
        REQUIRE(baseline.ensure_numbering_definition(definition).has_value());
        successful_allocation_count = controlled_pugi_allocation_calls;
        REQUIRE_GT(successful_allocation_count, 0U);
        fs::remove(baseline_path);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        const auto target =
            fs::current_path() / ("numbering_attachment_oom_" +
                                  std::to_string(failure_call) + ".docx");
        fs::remove(target);

        featherdoc::Document document(target);
        controlled_pugi_failure_call = 0U;
        REQUIRE_FALSE(document.create_empty());
        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = failure_call;

        CHECK_FALSE(
            document.ensure_numbering_definition(definition).has_value());
        const auto mutation_error = document.last_error();
        controlled_pugi_failure_call = 0U;
        CHECK_EQ(mutation_error.code,
                 std::make_error_code(std::errc::not_enough_memory));

        REQUIRE_FALSE(document.save());
        CHECK_FALSE(
            test_docx_entry_exists(target, "word/_rels/document.xml.rels"));
        CHECK_FALSE(test_docx_entry_exists(target, "word/numbering.xml"));
        const auto failed_content_types =
            read_test_docx_entry(target, test_content_types_xml_entry);
        CHECK_EQ(failed_content_types.find("/word/numbering.xml"),
                 std::string::npos);
        CHECK_EQ(failed_content_types.find(numbering_content_type),
                 std::string::npos);

        controlled_pugi_allocation_calls = 0U;
        REQUIRE(document.ensure_numbering_definition(definition).has_value());
        REQUIRE_FALSE(document.save());
        CHECK(test_docx_entry_exists(target, "word/_rels/document.xml.rels"));
        CHECK(test_docx_entry_exists(target, "word/numbering.xml"));
        CHECK_NE(read_test_docx_entry(target, "word/_rels/document.xml.rels")
                     .find(numbering_relationship_type),
                 std::string::npos);

        featherdoc::Document reopened(target);
        REQUIRE_FALSE(reopened.open());
        CHECK(std::ranges::any_of(reopened.list_numbering_definitions(),
                                  [&](const auto &summary) {
                                      return summary.name == definition.name;
                                  }));
        fs::remove(target);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "numbering catalog import is atomic for every pugixml allocation "
    "failure with missing and existing numbering parts") {
    namespace fs = std::filesystem;

    featherdoc::numbering_catalog catalog;
    for (std::uint32_t index = 0U; index < 3U; ++index) {
        featherdoc::numbering_catalog_definition catalog_definition;
        catalog_definition.definition.name =
            "PostAttachAllocationDefinition" + std::to_string(index);
        catalog_definition.definition.levels = {
            featherdoc::numbering_level_definition{
                featherdoc::list_kind::decimal, 1U, 0U,
                std::string(40U * 1024U, static_cast<char>('a' + index))}};
        auto instance = featherdoc::numbering_instance_summary{};
        auto level_override = featherdoc::numbering_level_override_summary{};
        level_override.level = 0U;
        level_override.start_override = index + 2U;
        instance.level_overrides.push_back(std::move(level_override));
        catalog_definition.instances.push_back(std::move(instance));
        catalog.definitions.push_back(std::move(catalog_definition));
    }

    constexpr auto existing_numbering_xml = std::string_view{R"(
<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="7">
    <w:name w:val="ExistingNumberingDefinition"/>
    <w:lvl w:ilvl="0"><w:start w:val="1"/><w:numFmt w:val="decimal"/><w:lvlText w:val="%1."/></w:lvl>
  </w:abstractNum>
  <w:num w:numId="9"><w:abstractNumId w:val="7"/></w:num>
</w:numbering>)"};
    constexpr auto existing_document_relationships_xml = std::string_view{R"(
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering" Target="numbering.xml"/>
</Relationships>)"};
    constexpr auto existing_content_types_xml = std::string_view{R"(
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/numbering.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"/>
</Types>)"};

    const auto write_source = [&](const fs::path &path,
                                  bool has_existing_numbering) {
        fs::remove(path);
        if (!has_existing_numbering) {
            write_test_docx(path, valid_document_xml);
            return;
        }
        write_test_archive_entries(
            path,
            {{test_content_types_xml_entry,
              std::string{existing_content_types_xml}},
             {test_relationships_xml_entry, test_relationships_xml},
             {test_document_xml_entry, valid_document_xml},
             {"word/_rels/document.xml.rels",
              std::string{existing_document_relationships_xml}},
             {"word/numbering.xml", std::string{existing_numbering_xml}}});
    };

    pugi_memory_management_guard allocation_guard;
    delegated_pugi_allocate = allocation_guard.allocation;
    pugi::set_memory_management_functions(controlled_pugi_allocate,
                                          allocation_guard.deallocation);

    for (const bool has_existing_numbering : {false, true}) {
        CAPTURE(has_existing_numbering);
        std::size_t successful_allocation_count = 0U;
        {
            const auto baseline =
                fs::current_path() /
                (has_existing_numbering
                     ? "numbering_catalog_atomic_existing_baseline.docx"
                     : "numbering_catalog_atomic_missing_baseline.docx");
            write_source(baseline, has_existing_numbering);
            featherdoc::Document document(baseline);
            REQUIRE_FALSE(document.open());
            controlled_pugi_allocation_calls = 0U;
            controlled_pugi_failure_call = 0U;
            REQUIRE(
                static_cast<bool>(document.import_numbering_catalog(catalog)));
            successful_allocation_count = controlled_pugi_allocation_calls;
            REQUIRE_GT(successful_allocation_count, 0U);
            fs::remove(baseline);
        }

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            const auto target =
                fs::current_path() /
                ((has_existing_numbering
                      ? "numbering_catalog_atomic_existing_oom_"
                      : "numbering_catalog_atomic_missing_oom_") +
                 std::to_string(failure_call) + ".docx");
            write_source(target, has_existing_numbering);
            const auto original_content_types =
                read_test_docx_entry(target, test_content_types_xml_entry);
            const auto original_relationships =
                has_existing_numbering
                    ? read_test_docx_entry(target,
                                           "word/_rels/document.xml.rels")
                    : std::string{};
            const auto original_numbering =
                has_existing_numbering
                    ? read_test_docx_entry(target, "word/numbering.xml")
                    : std::string{};

            featherdoc::Document document(target);
            controlled_pugi_failure_call = 0U;
            REQUIRE_FALSE(document.open());
            controlled_pugi_allocation_calls = 0U;
            controlled_pugi_failure_call = failure_call;
            const auto failed_summary =
                document.import_numbering_catalog(catalog);
            const auto mutation_error = document.last_error();
            controlled_pugi_failure_call = 0U;

            CHECK_FALSE(static_cast<bool>(failed_summary));
            CHECK_EQ(failed_summary.imported_definition_count, 0U);
            CHECK_EQ(failed_summary.imported_instance_count, 0U);
            CHECK(failed_summary.definitions.empty());
            CHECK_EQ(mutation_error.code,
                     std::make_error_code(std::errc::not_enough_memory));

            REQUIRE_FALSE(document.save());
            CHECK_EQ(read_test_docx_entry(target, test_content_types_xml_entry),
                     original_content_types);
            if (has_existing_numbering) {
                CHECK_EQ(read_test_docx_entry(target,
                                              "word/_rels/document.xml.rels"),
                         original_relationships);
                CHECK_EQ(read_test_docx_entry(target, "word/numbering.xml"),
                         original_numbering);
            } else {
                CHECK_FALSE(test_docx_entry_exists(
                    target, "word/_rels/document.xml.rels"));
                CHECK_FALSE(
                    test_docx_entry_exists(target, "word/numbering.xml"));
                CHECK_EQ(original_content_types.find("/word/numbering.xml"),
                         std::string::npos);
            }

            featherdoc::Document failed_state_reopened(target);
            REQUIRE_FALSE(failed_state_reopened.open());
            const auto unchanged_definitions =
                failed_state_reopened.list_numbering_definitions();
            REQUIRE_FALSE(failed_state_reopened.last_error());
            CHECK_EQ(unchanged_definitions.size(),
                     has_existing_numbering ? 1U : 0U);

            const auto retry_summary =
                document.import_numbering_catalog(catalog);
            REQUIRE(static_cast<bool>(retry_summary));
            CHECK_EQ(retry_summary.imported_definition_count,
                     catalog.definitions.size());
            CHECK_EQ(retry_summary.imported_instance_count,
                     catalog.definitions.size());
            REQUIRE_FALSE(document.save());

            featherdoc::Document reopened(target);
            REQUIRE_FALSE(reopened.open());
            const auto reopened_definitions =
                reopened.list_numbering_definitions();
            REQUIRE_FALSE(reopened.last_error());
            for (const auto &catalog_definition : catalog.definitions) {
                CHECK(std::ranges::any_of(
                    reopened_definitions, [&](const auto &definition) {
                        return definition.name ==
                               catalog_definition.definition.name;
                    }));
            }
            fs::remove(target);
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "opened source does not publish a newly attached settings part when a "
    "later pugixml allocation fails") {
    namespace fs = std::filesystem;

    pugi_memory_management_guard allocation_guard;
    delegated_pugi_allocate = allocation_guard.allocation;
    pugi::set_memory_management_functions(controlled_pugi_allocate,
                                          allocation_guard.deallocation);

    std::size_t successful_allocation_count = 0U;
    {
        const auto baseline = fs::current_path() /
                              "source_settings_post_attach_oom_baseline.docx";
        fs::remove(baseline);
        write_test_docx(baseline, valid_document_xml);
        featherdoc::Document document(baseline);
        REQUIRE_FALSE(document.open());
        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = 0U;
        REQUIRE(document
                    .ensure_section_header_paragraphs(
                        0U, featherdoc::section_reference_kind::even_page)
                    .has_next());
        successful_allocation_count = controlled_pugi_allocation_calls;
        REQUIRE_GT(successful_allocation_count, 0U);
        fs::remove(baseline);
    }

    bool observed_allocation_failure = false;
    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        const auto target =
            fs::current_path() / ("source_settings_post_attach_oom_" +
                                  std::to_string(failure_call) + ".docx");
        fs::remove(target);
        write_test_docx(target, valid_document_xml);
        REQUIRE_FALSE(test_docx_entry_exists(target, "word/settings.xml"));

        featherdoc::Document document(target);
        controlled_pugi_failure_call = 0U;
        REQUIRE_FALSE(document.open());
        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = failure_call;
        const auto mutation_succeeded =
            document
                .ensure_section_header_paragraphs(
                    0U, featherdoc::section_reference_kind::even_page)
                .has_next();
        const auto mutation_error = document.last_error();
        controlled_pugi_failure_call = 0U;

        if (!mutation_succeeded) {
            observed_allocation_failure = true;
            CHECK_EQ(mutation_error.code,
                     std::make_error_code(std::errc::not_enough_memory));
            REQUIRE_FALSE(document.save());
            const auto has_document_relationships =
                test_docx_entry_exists(target, "word/_rels/document.xml.rels");
            const auto saved_relationships =
                has_document_relationships
                    ? read_test_docx_entry(target,
                                           "word/_rels/document.xml.rels")
                    : std::string{};
            CHECK_EQ(saved_relationships.find(settings_relationship_type),
                     std::string::npos);
            CHECK_FALSE(test_docx_entry_exists(target, "word/settings.xml"));
            const auto unchanged_content_types =
                read_test_docx_entry(target, test_content_types_xml_entry);
            CHECK_EQ(unchanged_content_types.find("/word/settings.xml"),
                     std::string::npos);
            CHECK_EQ(unchanged_content_types.find(settings_content_type),
                     std::string::npos);

            REQUIRE(document
                        .ensure_section_header_paragraphs(
                            0U, featherdoc::section_reference_kind::even_page)
                        .has_next());
            REQUIRE_FALSE(document.last_error());
            REQUIRE_FALSE(document.save());
            CHECK(test_docx_entry_exists(target, "word/settings.xml"));
            const auto retried_relationships = read_test_docx_entry(
                target, "word/_rels/document.xml.rels");
            CHECK_NE(retried_relationships.find(settings_relationship_type),
                     std::string::npos);
            const auto retried_content_types =
                read_test_docx_entry(target, test_content_types_xml_entry);
            CHECK_NE(retried_content_types.find("/word/settings.xml"),
                     std::string::npos);
            CHECK_NE(retried_content_types.find(settings_content_type),
                     std::string::npos);

            featherdoc::Document reopened(target);
            REQUIRE_FALSE(reopened.open());
            const auto reopened_sections = reopened.inspect_sections();
            REQUIRE_FALSE(reopened.last_error());
            REQUIRE(reopened_sections.even_and_odd_headers_enabled.has_value());
            CHECK(*reopened_sections.even_and_odd_headers_enabled);
        }
        fs::remove(target);
    }

    CHECK(observed_allocation_failure);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "paragraph list mutation is atomic for every pugixml allocation "
    "failure and remains retryable with the original handles") {
    namespace fs = std::filesystem;

    constexpr auto paragraph_text = "atomic list item";

    pugi_memory_management_guard allocation_guard;
    delegated_pugi_allocate = allocation_guard.allocation;
    pugi::set_memory_management_functions(controlled_pugi_allocate,
                                          allocation_guard.deallocation);

    std::size_t successful_allocation_count = 0U;
    {
        const auto baseline_path =
            fs::current_path() / "paragraph_list_transaction_oom_baseline.docx";
        fs::remove(baseline_path);

        featherdoc::Document baseline(baseline_path);
        REQUIRE_FALSE(baseline.create_empty());
        auto paragraph = baseline.paragraphs();
        REQUIRE(paragraph.add_run(std::string{paragraph_text}).valid());

        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = 0U;
        REQUIRE(baseline.set_paragraph_list(
            paragraph, featherdoc::list_kind::decimal));
        successful_allocation_count = controlled_pugi_allocation_calls;
        REQUIRE_GT(successful_allocation_count, 0U);
        fs::remove(baseline_path);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);

        const auto target =
            fs::current_path() / ("paragraph_list_transaction_oom_" +
                                  std::to_string(failure_call) + ".docx");
        fs::remove(target);

        featherdoc::Document document(target);
        controlled_pugi_failure_call = 0U;
        REQUIRE_FALSE(document.create_empty());
        auto paragraph = document.paragraphs();
        auto run = paragraph.add_run(std::string{paragraph_text});
        REQUIRE(paragraph.valid());
        REQUIRE(run.valid());
        REQUIRE_FALSE(document.save());

        const auto original_document = normalized_xml_document_element(
            read_test_docx_entry(target, test_document_xml_entry));
        REQUIRE(original_document.has_value());
        const auto original_content_types =
            read_test_docx_entry(target, test_content_types_xml_entry);
        REQUIRE_FALSE(test_docx_entry_exists(
            target, "word/_rels/document.xml.rels"));
        REQUIRE_FALSE(test_docx_entry_exists(target, "word/numbering.xml"));

        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = failure_call;
        const auto mutation_succeeded = document.set_paragraph_list(
            paragraph, featherdoc::list_kind::decimal);
        const auto mutation_error = document.last_error();
        controlled_pugi_failure_call = 0U;

        CHECK_FALSE(mutation_succeeded);
        CHECK_EQ(mutation_error.code,
                 std::make_error_code(std::errc::not_enough_memory));

        // A rejected transaction must not retire either pre-existing handle.
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_EQ(paragraph.runs().get_text(), paragraph_text);
        CHECK_EQ(run.get_text(), paragraph_text);
        CHECK(document.list_numbering_definitions().empty());

        // Serializing the failed in-memory state exposes leaked WML, package
        // metadata, part-presence, and dirty-state changes.
        REQUIRE_FALSE(document.save());
        const auto failed_document = normalized_xml_document_element(
            read_test_docx_entry(target, test_document_xml_entry));
        REQUIRE(failed_document.has_value());
        CHECK_EQ(*failed_document, *original_document);
        CHECK_EQ(read_test_docx_entry(target, test_content_types_xml_entry),
                 original_content_types);
        CHECK_FALSE(test_docx_entry_exists(
            target, "word/_rels/document.xml.rels"));
        CHECK_FALSE(test_docx_entry_exists(target, "word/numbering.xml"));
        CHECK_EQ(original_content_types.find("/word/numbering.xml"),
                 std::string::npos);
        CHECK_EQ(original_content_types.find(numbering_content_type),
                 std::string::npos);

        // Restoring the allocator is sufficient: the same Document and the
        // exact handles supplied to the failed call must be reusable.
        controlled_pugi_allocation_calls = 0U;
        REQUIRE(document.set_paragraph_list(
            paragraph, featherdoc::list_kind::decimal));
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_EQ(run.get_text(), paragraph_text);
        REQUIRE_FALSE(document.save());

        CHECK(test_docx_entry_exists(target, "word/numbering.xml"));
        const auto saved_relationships =
            read_test_docx_entry(target, "word/_rels/document.xml.rels");
        CHECK_NE(saved_relationships.find(numbering_relationship_type),
                 std::string::npos);
        const auto saved_content_types =
            read_test_docx_entry(target, test_content_types_xml_entry);
        CHECK_NE(saved_content_types.find("/word/numbering.xml"),
                 std::string::npos);
        CHECK_NE(saved_content_types.find(numbering_content_type),
                 std::string::npos);
        CHECK_NE(read_test_docx_entry(target, test_document_xml_entry)
                     .find("<w:numPr>"),
                 std::string::npos);

        featherdoc::Document reopened(target);
        REQUIRE_FALSE(reopened.open());
        CHECK_EQ(reopened.paragraphs().runs().get_text(), paragraph_text);
        CHECK_FALSE(reopened.list_numbering_definitions().empty());
        fs::remove(target);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "style-linked numbering late pugixml allocation failure preserves "
    "styles numbering metadata and live paragraph handles") {
    namespace fs = std::filesystem;

    auto definition = featherdoc::numbering_definition{};
    definition.name = "AtomicStyleLinkedNumbering";
    definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };
    const auto style_links =
        std::vector<featherdoc::paragraph_style_numbering_link>{
            {"Normal", 0U}, {"Heading1", 1U}};

    pugi_memory_management_guard allocation_guard;
    delegated_pugi_allocate = allocation_guard.allocation;
    pugi::set_memory_management_functions(controlled_pugi_allocate,
                                          allocation_guard.deallocation);

    std::size_t late_failure_call = 0U;
    {
        const auto baseline_path = fs::current_path() /
                                   "style_numbering_transaction_oom_baseline.docx";
        fs::remove(baseline_path);
        featherdoc::Document baseline(baseline_path);
        REQUIRE_FALSE(baseline.create_empty());

        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = 0U;
        REQUIRE(baseline
                    .ensure_style_linked_numbering(definition, style_links)
                    .has_value());
        late_failure_call = controlled_pugi_allocation_calls;
        REQUIRE_GT(late_failure_call, 0U);
        fs::remove(baseline_path);
    }

    CAPTURE(late_failure_call);
    const auto target =
        fs::current_path() / "style_numbering_transaction_late_oom.docx";
    fs::remove(target);

    featherdoc::Document document(target);
    controlled_pugi_failure_call = 0U;
    REQUIRE_FALSE(document.create_empty());
    auto paragraph = document.paragraphs();
    auto run = paragraph.add_run("style transaction sentinel");
    REQUIRE(paragraph.valid());
    REQUIRE(run.valid());
    REQUIRE_FALSE(document.save());

    const auto original_document = normalized_xml_document_element(
        read_test_docx_entry(target, test_document_xml_entry));
    REQUIRE(original_document.has_value());
    const auto original_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    REQUIRE_FALSE(
        test_docx_entry_exists(target, "word/_rels/document.xml.rels"));
    REQUIRE_FALSE(test_docx_entry_exists(target, "word/styles.xml"));
    REQUIRE_FALSE(test_docx_entry_exists(target, "word/numbering.xml"));

    controlled_pugi_allocation_calls = 0U;
    controlled_pugi_failure_call = late_failure_call;
    const auto failed_identifier =
        document.ensure_style_linked_numbering(definition, style_links);
    const auto mutation_error = document.last_error();
    controlled_pugi_failure_call = 0U;

    CHECK_FALSE(failed_identifier.has_value());
    CHECK_EQ(mutation_error.code,
             std::make_error_code(std::errc::not_enough_memory));
    CHECK(paragraph.valid());
    CHECK(run.valid());
    CHECK_EQ(run.get_text(), "style transaction sentinel");

    const auto unchanged_normal = document.find_style("Normal");
    const auto unchanged_heading = document.find_style("Heading1");
    REQUIRE(unchanged_normal.has_value());
    REQUIRE(unchanged_heading.has_value());
    CHECK_FALSE(unchanged_normal->numbering.has_value());
    CHECK_FALSE(unchanged_heading->numbering.has_value());
    CHECK(document.list_numbering_definitions().empty());

    REQUIRE_FALSE(document.save());
    const auto failed_document = normalized_xml_document_element(
        read_test_docx_entry(target, test_document_xml_entry));
    REQUIRE(failed_document.has_value());
    CHECK_EQ(*failed_document, *original_document);
    CHECK_EQ(read_test_docx_entry(target, test_content_types_xml_entry),
             original_content_types);
    CHECK_FALSE(
        test_docx_entry_exists(target, "word/_rels/document.xml.rels"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/styles.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/numbering.xml"));
    CHECK_EQ(original_content_types.find("/word/styles.xml"),
             std::string::npos);
    CHECK_EQ(original_content_types.find(styles_content_type),
             std::string::npos);
    CHECK_EQ(original_content_types.find("/word/numbering.xml"),
             std::string::npos);
    CHECK_EQ(original_content_types.find(numbering_content_type),
             std::string::npos);

    controlled_pugi_allocation_calls = 0U;
    const auto retry_identifier =
        document.ensure_style_linked_numbering(definition, style_links);
    REQUIRE(retry_identifier.has_value());
    CHECK(paragraph.valid());
    CHECK(run.valid());
    CHECK_EQ(run.get_text(), "style transaction sentinel");

    const auto linked_normal = document.find_style("Normal");
    const auto linked_heading = document.find_style("Heading1");
    REQUIRE(linked_normal.has_value());
    REQUIRE(linked_heading.has_value());
    REQUIRE(linked_normal->numbering.has_value());
    REQUIRE(linked_heading->numbering.has_value());
    REQUIRE(linked_normal->numbering->num_id.has_value());
    REQUIRE(linked_heading->numbering->num_id.has_value());
    CHECK_EQ(*linked_normal->numbering->num_id,
             *linked_heading->numbering->num_id);

    REQUIRE_FALSE(document.save());
    REQUIRE(test_docx_entry_exists(target, "word/styles.xml"));
    REQUIRE(test_docx_entry_exists(target, "word/numbering.xml"));
    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find(styles_relationship_type),
             std::string::npos);
    CHECK_NE(saved_relationships.find(numbering_relationship_type),
             std::string::npos);
    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/styles.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find(styles_content_type), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/numbering.xml"),
             std::string::npos);
    CHECK_NE(saved_content_types.find(numbering_content_type),
             std::string::npos);

    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_normal = reopened.find_style("Normal");
    const auto reopened_heading = reopened.find_style("Heading1");
    REQUIRE(reopened_normal.has_value());
    REQUIRE(reopened_heading.has_value());
    REQUIRE(reopened_normal->numbering.has_value());
    REQUIRE(reopened_heading->numbering.has_value());
    fs::remove(target);
}
