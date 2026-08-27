#include "document_core_unit_test_support.hpp"

#include "../src/document_archive_limit_helpers.hpp"

#include <featherdoc/detail/utf8.hpp>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {

auto invalid_utf8_entry_name() -> std::string {
    return std::string{"word/", 5U} + std::string{"\xFF", 1U} + ".xml";
}

auto safe_invalid_utf8_entry_name() -> std::string { return "word/\\xFF.xml"; }

auto archive_utf8_test_entries(
    std::vector<std::pair<std::string, std::string>> extra_entries = {})
    -> std::vector<std::pair<std::string, std::string>> {
    std::vector<std::pair<std::string, std::string>> entries{
        {test_content_types_xml_entry, test_content_types_xml},
        {test_relationships_xml_entry, test_relationships_xml},
        {test_document_xml_entry,
         R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p><w:r><w:t>archive utf8</w:t></w:r></w:p></w:body>
</w:document>
)"},
    };
    entries.insert(entries.end(), extra_entries.begin(), extra_entries.end());
    return entries;
}

auto archive_utf8_open_options(featherdoc::package_validation_mode validation)
    -> featherdoc::document_open_options {
    featherdoc::document_open_options options;
    options.validation = validation;
    return options;
}

auto read_little_endian_u16(const std::vector<unsigned char> &bytes,
                            std::size_t offset) -> std::uint16_t {
    return static_cast<std::uint16_t>(bytes[offset]) |
           (static_cast<std::uint16_t>(bytes[offset + 1U]) << 8U);
}

auto read_little_endian_u32(const std::vector<unsigned char> &bytes,
                            std::size_t offset) -> std::uint32_t {
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
           (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
}

auto replace_central_directory_entry_name(const std::filesystem::path &path,
                                          std::string_view original_name,
                                          std::string_view replacement_name)
    -> bool {
    if (original_name.size() != replacement_name.size()) {
        return false;
    }

    std::ifstream input(path, std::ios::binary);
    const std::vector<unsigned char> bytes_from_file{
        std::istreambuf_iterator<char>{input},
        std::istreambuf_iterator<char>{}};
    input.close();
    if (!input.good() || bytes_from_file.size() < 22U) {
        return false;
    }
    auto bytes = bytes_from_file;

    constexpr std::uint32_t end_of_central_directory_signature = 0x06054B50U;
    constexpr std::uint32_t central_directory_header_signature = 0x02014B50U;
    constexpr std::size_t central_directory_fixed_header_size = 46U;
    auto end_of_central_directory_offset = bytes.size();
    for (std::size_t offset = bytes.size() - 22U;; --offset) {
        if (read_little_endian_u32(bytes, offset) ==
            end_of_central_directory_signature) {
            end_of_central_directory_offset = offset;
            break;
        }
        if (offset == 0U) {
            break;
        }
    }
    if (end_of_central_directory_offset == bytes.size() ||
        end_of_central_directory_offset + 22U > bytes.size()) {
        return false;
    }

    const auto entry_count =
        read_little_endian_u16(bytes, end_of_central_directory_offset + 10U);
    std::size_t entry_offset =
        read_little_endian_u32(bytes, end_of_central_directory_offset + 16U);
    for (std::uint16_t index = 0U; index < entry_count; ++index) {
        if (entry_offset > bytes.size() ||
            bytes.size() - entry_offset < central_directory_fixed_header_size ||
            read_little_endian_u32(bytes, entry_offset) !=
                central_directory_header_signature) {
            return false;
        }

        const auto name_size = static_cast<std::size_t>(
            read_little_endian_u16(bytes, entry_offset + 28U));
        const auto extra_size = static_cast<std::size_t>(
            read_little_endian_u16(bytes, entry_offset + 30U));
        const auto comment_size = static_cast<std::size_t>(
            read_little_endian_u16(bytes, entry_offset + 32U));
        const auto variable_size = name_size + extra_size + comment_size;
        if (variable_size >
            bytes.size() - entry_offset - central_directory_fixed_header_size) {
            return false;
        }

        const auto name_offset =
            entry_offset + central_directory_fixed_header_size;
        if (name_size == original_name.size() &&
            std::equal(original_name.begin(), original_name.end(),
                       bytes.begin() +
                           static_cast<std::ptrdiff_t>(name_offset))) {
            std::copy(replacement_name.begin(), replacement_name.end(),
                      bytes.begin() + static_cast<std::ptrdiff_t>(name_offset));
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output.write(reinterpret_cast<const char *>(bytes.data()),
                         static_cast<std::streamsize>(bytes.size()));
            return output.good();
        }
        entry_offset += central_directory_fixed_header_size + variable_size;
    }
    return false;
}

auto check_archive_limit_failure(const std::filesystem::path &path,
                                 const featherdoc::archive_limits &limits,
                                 std::string_view expected_resource) -> void {
    featherdoc::document_open_options options;
    options.limits = limits;
    featherdoc::Document document(path);
    const auto error = document.open(options);
    CHECK_EQ(error, featherdoc::document_errc::archive_limit_exceeded);
    CHECK_FALSE(document.is_open());
    CHECK(document.last_error().detail.find(expected_resource) !=
          std::string::npos);
    CHECK(document.last_error().detail.find("limit is") != std::string::npos);
}

#if defined(_WIN32)
auto windows_extended_path(const std::filesystem::path &path)
    -> std::filesystem::path {
    const auto absolute_path = std::filesystem::absolute(path).native();
    if (absolute_path.starts_with(L"\\\\?\\")) {
        return std::filesystem::path{absolute_path};
    }
    if (absolute_path.starts_with(L"\\\\")) {
        return std::filesystem::path{L"\\\\?\\UNC\\" +
                                     absolute_path.substr(2U)};
    }
    return std::filesystem::path{L"\\\\?\\" + absolute_path};
}
#endif

} // namespace

#if defined(_WIN32)
TEST_CASE("Windows DOCX paths preserve native UTF-8 extended-path syntax") {
    namespace fs = std::filesystem;

    const auto root = windows_extended_path(
        fs::current_path() /
        featherdoc::detail::path_from_utf8("长路径-中文-日本語-🙂"));
    std::error_code cleanup_error;
    fs::remove_all(root, cleanup_error);

    auto deep_directory = root;
    const auto segment =
        featherdoc::detail::path_from_utf8("目录-日本語-🙂-0123456789");
    while (deep_directory.native().size() < 300U) {
        deep_directory /= segment;
    }
    REQUIRE(deep_directory.native().size() >= 300U);
    REQUIRE(fs::create_directories(deep_directory));

    const auto source = deep_directory / featherdoc::detail::path_from_utf8(
                                             "输入 文档-中文-日本語-🙂.docx");
    const auto saved = deep_directory / featherdoc::detail::path_from_utf8(
                                            "保存 文档-中文-日本語-🙂.docx");
    write_test_archive_entries(source, archive_utf8_test_entries());

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE_FALSE(document.save_as(saved));

    featherdoc::Document reopened(saved);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.is_open());

    fs::remove_all(root, cleanup_error);
    CHECK_FALSE(cleanup_error);
}

TEST_CASE("miniz rejects invalid UTF-8 Windows paths without replacement") {
    namespace fs = std::filesystem;

    const auto replacement_path =
        fs::current_path() /
        featherdoc::detail::path_from_utf8("miniz-invalid-�.docx");
    fs::remove(replacement_path);
    write_test_archive_entries(replacement_path, archive_utf8_test_entries());

    auto invalid_path =
        featherdoc::detail::path_to_native_utf8(replacement_path.parent_path());
    if (!invalid_path.empty() && invalid_path.back() != '\\' &&
        invalid_path.back() != '/') {
        invalid_path.push_back('\\');
    }
    invalid_path += "miniz-invalid-";
    invalid_path.push_back(static_cast<char>(0xFFU));
    invalid_path += ".docx";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(
        invalid_path.c_str(), ZIP_DEFAULT_COMPRESSION_LEVEL, 'r', &zip_error);
    CHECK_EQ(archive, nullptr);
    CHECK_NE(zip_error, 0);

    fs::remove(replacement_path);
}
#endif

TEST_CASE("DOCX physical entry names must be valid UTF-8 in strict and "
          "tolerant modes") {
    namespace fs = std::filesystem;
    const auto invalid_entry = invalid_utf8_entry_name();

    for (const auto validation :
         {featherdoc::package_validation_mode::strict,
          featherdoc::package_validation_mode::tolerant}) {
        const auto path =
            fs::current_path() /
            (validation == featherdoc::package_validation_mode::strict
                 ? "archive_invalid_utf8_strict.docx"
                 : "archive_invalid_utf8_tolerant.docx");
        write_test_archive_entries(
            path, archive_utf8_test_entries({{invalid_entry, "<x/>"}}));

        featherdoc::Document document(path);
        const auto error = document.open(archive_utf8_open_options(validation));
        CHECK_EQ(error, featherdoc::document_errc::invalid_package_structure);
        CHECK_FALSE(document.is_open());
        CHECK(featherdoc::detail::is_valid_utf8(
            document.last_error().entry_name));
        CHECK_EQ(document.last_error().entry_name,
                 safe_invalid_utf8_entry_name());
        CHECK(document.last_error().detail.find("valid UTF-8") !=
              std::string::npos);

        fs::remove(path);
    }
}

TEST_CASE("strict rejects raw Unicode physical names while tolerant accepts "
          "valid UTF-8 Chinese Japanese and emoji") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "archive_valid_utf8_entries.docx";
    std::vector<std::pair<std::string, std::string>> unicode_entries;
    unicode_entries.emplace_back(utf8_from_u8(u8"word/中文.xml"), "<x/>");
    unicode_entries.emplace_back(utf8_from_u8(u8"word/日本語.xml"), "<x/>");
    unicode_entries.emplace_back(utf8_from_u8(u8"word/emoji-🙂.bin"), "binary");
    write_test_archive_entries(
        path, archive_utf8_test_entries(std::move(unicode_entries)));

    featherdoc::Document strict_document(path);
    CHECK_EQ(strict_document.open(archive_utf8_open_options(
                 featherdoc::package_validation_mode::strict)),
             featherdoc::document_errc::invalid_package_structure);
    CHECK_FALSE(strict_document.is_open());
    CHECK(strict_document.last_error().detail.find("ASCII canonical") !=
          std::string::npos);

    featherdoc::Document tolerant_document(path);
    CHECK_FALSE(tolerant_document.open(archive_utf8_open_options(
        featherdoc::package_validation_mode::tolerant)));
    CHECK(tolerant_document.is_open());

    fs::remove(path);
}

TEST_CASE("tolerant save canonicalizes raw Unicode physical entry names") {
    namespace fs = std::filesystem;
    const auto source =
        fs::current_path() / "archive_raw_unicode_save_source.docx";
    const auto target =
        fs::current_path() / "archive_raw_unicode_save_target.docx";
    const auto raw_entry_name = utf8_from_u8(u8"word/自定义-🙂.xml");
    const auto canonical_entry =
        featherdoc::detail::canonical_package_part_key(raw_entry_name);
    REQUIRE(canonical_entry);
    REQUIRE_NE(canonical_entry.entry_name, raw_entry_name);

    write_test_archive_entries(
        source,
        archive_utf8_test_entries({{raw_entry_name, "<custom>保留</custom>"}}));

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open(archive_utf8_open_options(
        featherdoc::package_validation_mode::tolerant)));
    REQUIRE_FALSE(document.save_as(target));

    const auto saved_entries = read_test_archive_entries(target);
    CHECK(std::ranges::any_of(saved_entries, [&](const auto &entry) {
        return entry.first == canonical_entry.entry_name;
    }));
    CHECK_FALSE(std::ranges::any_of(saved_entries, [&](const auto &entry) {
        return entry.first == raw_entry_name;
    }));
    CHECK_EQ(read_test_docx_entry(target, canonical_entry.entry_name.c_str()),
             "<custom>保留</custom>");

    featherdoc::Document strict_reopen(target);
    CHECK_FALSE(strict_reopen.open());
    CHECK(strict_reopen.is_open());

    fs::remove(source);
    fs::remove(target);
}

TEST_CASE("raw and percent-equivalent physical entries are rejected as one "
          "logical part") {
    namespace fs = std::filesystem;
    const auto path =
        fs::current_path() / "archive_raw_percent_identity_collision.docx";
    const auto raw_entry_name = utf8_from_u8(u8"word/重复.xml");
    const auto canonical_entry =
        featherdoc::detail::canonical_package_part_key(raw_entry_name);
    REQUIRE(canonical_entry);
    REQUIRE_NE(canonical_entry.entry_name, raw_entry_name);

    write_test_archive_entries(
        path, archive_utf8_test_entries(
                  {{raw_entry_name, "<raw/>"},
                   {canonical_entry.entry_name, "<encoded/>"}}));

    for (const auto validation :
         {featherdoc::package_validation_mode::strict,
          featherdoc::package_validation_mode::tolerant}) {
        featherdoc::Document document(path);
        CHECK_EQ(document.open(archive_utf8_open_options(validation)),
                 featherdoc::document_errc::invalid_package_structure);
        CHECK_FALSE(document.is_open());
        if (validation == featherdoc::package_validation_mode::tolerant) {
            CHECK(document.last_error().detail.find("raw/percent-equivalent") !=
                  std::string::npos);
        }
    }

    fs::remove(path);
}

TEST_CASE("save_as rejects changed source archive entry names before copying") {
    namespace fs = std::filesystem;
    const auto source =
        fs::current_path() / "archive_utf8_source_replaced.docx";
    const auto target =
        fs::current_path() / "archive_utf8_target_unchanged.docx";
    const auto invalid_entry = invalid_utf8_entry_name();

    write_test_archive_entries(source, archive_utf8_test_entries());
    write_test_archive_entries(
        target,
        archive_utf8_test_entries({{"word/custom.xml", "<original-target/>"}}));
    const auto original_target_document =
        read_test_docx_entry(target, test_document_xml_entry);
    const auto original_target_custom =
        read_test_docx_entry(target, "word/custom.xml");

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.is_open());

    write_test_archive_entries(
        source, archive_utf8_test_entries({{invalid_entry, "<x/>"}}));

    const auto save_error = document.save_as(target);
    CHECK_EQ(save_error, featherdoc::document_errc::source_archive_changed);
    CHECK(featherdoc::detail::is_valid_utf8(document.last_error().entry_name));
    CHECK_EQ(document.last_error().entry_name, safe_invalid_utf8_entry_name());
    CHECK_EQ(read_test_docx_entry(target, test_document_xml_entry),
             original_target_document);
    CHECK_EQ(read_test_docx_entry(target, "word/custom.xml"),
             original_target_custom);

    fs::remove(source);
    fs::remove(target);
}

TEST_CASE("DOCX central-directory names preserve raw length and reject "
          "embedded NUL") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "archive_embedded_nul_name.docx";
    const std::string original_name = "word/custom.xmlSAFE";
    const std::string embedded_nul_name{"word/custom.xml\0BAD", 19U};
    REQUIRE_EQ(original_name.size(), embedded_nul_name.size());

    write_test_archive_entries(
        path, archive_utf8_test_entries({{original_name, "<x/>"}}));
    REQUIRE(replace_central_directory_entry_name(path, original_name,
                                                 embedded_nul_name));

    int zip_error = 0;
    const auto archive_path = featherdoc::detail::path_to_native_utf8(path);
    zip_t *archive = zip_openwitherror(
        archive_path.c_str(), ZIP_DEFAULT_COMPRESSION_LEVEL, 'r', &zip_error);
    REQUIRE_MESSAGE(archive != nullptr, zip_strerror(zip_error));
    bool found_embedded_nul_name = false;
    const auto entry_count = zip_entries_total(archive);
    REQUIRE(entry_count >= 0);
    for (ssize_t index = 0; index < entry_count; ++index) {
        REQUIRE_EQ(
            zip_entry_openbyindex(archive, static_cast<std::size_t>(index)), 0);
        const auto *name = zip_entry_name(archive);
        if (name != nullptr &&
            std::string_view{name, zip_entry_name_size(archive)} ==
                std::string_view{embedded_nul_name}) {
            found_embedded_nul_name = true;
            CHECK_EQ(zip_entry_name_size(archive), embedded_nul_name.size());
        }
        REQUIRE_EQ(zip_entry_close(archive), 0);
    }
    CHECK(found_embedded_nul_name);
    REQUIRE_EQ(zip_close_ex(archive), 0);

    for (const auto validation :
         {featherdoc::package_validation_mode::strict,
          featherdoc::package_validation_mode::tolerant}) {
        featherdoc::Document document(path);
        const auto error = document.open(archive_utf8_open_options(validation));
        CHECK_EQ(error, featherdoc::document_errc::invalid_package_structure);
        CHECK_FALSE(document.is_open());
        CHECK(document.last_error().detail.find("embedded NUL") !=
              std::string::npos);
        CHECK(document.last_error().entry_name.find("\\x00") !=
              std::string::npos);
    }

    fs::remove(path);
}

TEST_CASE("DOCX reader metadata limits reject archives during preflight") {
    namespace fs = std::filesystem;
    const auto path =
        fs::current_path() / "archive_reader_preflight_limits.docx";
    write_test_archive_entries(path, archive_utf8_test_entries());
    const auto archive_path = featherdoc::detail::path_to_native_utf8(path);

    const auto check_direct_limit =
        [&](const zip_reader_limits &direct_limits,
            zip_reader_limit_kind expected_kind) {
            zip_reader_limit_violation violation{};
            int zip_error = 0;
            zip_t *archive = zip_openwitherror_limits(
                archive_path.c_str(), ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                &zip_error, &direct_limits, &violation);
            CHECK_EQ(archive, nullptr);
            CHECK_EQ(zip_error, ZIP_EARCHLIMIT);
            CHECK_EQ(violation.kind, expected_kind);
            CHECK(violation.actual > violation.limit);
        };
    const auto unlimited = std::numeric_limits<std::uint64_t>::max();
    check_direct_limit(zip_reader_limits{unlimited, 1U, unlimited, unlimited},
                       ZIP_READER_LIMIT_CENTRAL_DIRECTORY_BYTES);
    check_direct_limit(zip_reader_limits{unlimited, unlimited, 4U, unlimited},
                       ZIP_READER_LIMIT_ENTRY_NAME_BYTES);
    check_direct_limit(zip_reader_limits{unlimited, unlimited, unlimited, 1U},
                       ZIP_READER_LIMIT_TOTAL_ENTRY_NAME_BYTES);

    featherdoc::archive_limits limits;
    limits.max_entries = 2U;
    check_archive_limit_failure(path, limits, "entry count");

    fs::remove(path);
}

TEST_CASE("DOCX default reader rejects overlong physical entry names") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "archive_overlong_entry_name.docx";
    const std::string overlong_name = "word/" + std::string(507U, 'a');
    REQUIRE_EQ(overlong_name.size(), 512U);
    write_test_archive_entries(
        path, archive_utf8_test_entries({{overlong_name, "x"}}));

    featherdoc::Document document(path);
    CHECK_EQ(document.open(),
             featherdoc::document_errc::archive_limit_exceeded);
    CHECK_FALSE(document.is_open());
    CHECK(document.last_error().detail.find("entry name byte size") !=
          std::string::npos);
    CHECK(document.last_error().detail.find("512") != std::string::npos);
    CHECK(document.last_error().detail.find("511") != std::string::npos);

    fs::remove(path);
}
