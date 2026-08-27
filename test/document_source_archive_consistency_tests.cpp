#include "document_core_unit_test_support.hpp"
#include "document_save_failure_test_support.hpp"

#include <featherdoc/detail/utf8.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr auto source_content_types_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Default Extension="bin" ContentType="application/octet-stream"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/settings.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/>
</Types>
)"};

constexpr auto source_root_relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
</Relationships>
)"};

constexpr auto source_document_relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings" Target="settings.xml"/>
</Relationships>
)"};

constexpr auto source_settings_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:updateFields w:val="true"/>
</w:settings>
)"};

auto source_document_xml(std::string_view text) -> std::string {
    return std::string{
               R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p><w:r><w:t>)"} +
           std::string{text} +
           R"(</w:t></w:r></w:p></w:body>
</w:document>
)";
}

void write_source_archive(const std::filesystem::path &path,
                          std::string_view document_text,
                          std::string_view marker_text) {
    const std::vector<std::pair<std::string, std::string>> entries{
        {test_content_types_xml_entry, std::string{source_content_types_xml}},
        {test_relationships_xml_entry,
         std::string{source_root_relationships_xml}},
        {test_document_xml_entry, source_document_xml(document_text)},
        {"word/_rels/document.xml.rels",
         std::string{source_document_relationships_xml}},
        {"word/settings.xml", std::string{source_settings_xml}},
        {"word/media/source-marker.bin", std::string{marker_text}},
    };

    int zip_error = 0;
    const auto archive_path = featherdoc::detail::path_to_native_utf8(path);
    zip_t *archive =
        zip_openwitherror(archive_path.c_str(), 0, 'w', &zip_error);
    REQUIRE(archive != nullptr);
    for (const auto &[entry_name, content] : entries) {
        REQUIRE_EQ(zip_entry_open(archive, entry_name.c_str()), 0);
        REQUIRE_GE(zip_entry_write(archive, content.data(), content.size()), 0);
        REQUIRE_EQ(zip_entry_close(archive), 0);
    }
    REQUIRE_EQ(zip_close_ex(archive), 0);
}

void write_footnote_source_archive(const std::filesystem::path &path,
                                   std::string_view note_text) {
    auto content_types = std::string{source_content_types_xml};
    const auto closing_types = content_types.rfind("</Types>");
    REQUIRE_NE(closing_types, std::string::npos);
    content_types.insert(
        closing_types,
        "  <Override PartName=\"/word/footnotes.xml\" "
        "ContentType=\"application/vnd.openxmlformats-officedocument."
        "wordprocessingml.footnotes+xml\"/>\n");

    auto document_relationships =
        std::string{source_document_relationships_xml};
    const auto closing_relationships =
        document_relationships.rfind("</Relationships>");
    REQUIRE_NE(closing_relationships, std::string::npos);
    document_relationships.insert(
        closing_relationships,
        "  <Relationship Id=\"rId3\" "
        "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/"
        "relationships/footnotes\" Target=\"footnotes.xml\"/>\n");

    const auto footnotes_xml =
        std::string{
            R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:footnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:footnote w:id="1"><w:p><w:r><w:t>)"} +
        std::string{note_text} +
        R"(</w:t></w:r></w:p></w:footnote>
</w:footnotes>
)";

    const std::vector<std::pair<std::string, std::string>> entries{
        {test_content_types_xml_entry, std::move(content_types)},
        {test_relationships_xml_entry,
         std::string{source_root_relationships_xml}},
        {test_document_xml_entry, source_document_xml("AAAA")},
        {"word/_rels/document.xml.rels", std::move(document_relationships)},
        {"word/settings.xml", std::string{source_settings_xml}},
        {"word/footnotes.xml", footnotes_xml},
    };

    int zip_error = 0;
    const auto archive_path = featherdoc::detail::path_to_native_utf8(path);
    zip_t *archive =
        zip_openwitherror(archive_path.c_str(), 0, 'w', &zip_error);
    REQUIRE(archive != nullptr);
    for (const auto &[entry_name, content] : entries) {
        REQUIRE_EQ(zip_entry_open(archive, entry_name.c_str()), 0);
        REQUIRE_GE(zip_entry_write(archive, content.data(), content.size()), 0);
        REQUIRE_EQ(zip_entry_close(archive), 0);
    }
    REQUIRE_EQ(zip_close_ex(archive), 0);
}

auto read_file_bytes(const std::filesystem::path &path) -> std::string {
    std::ifstream stream(path, std::ios::binary);
    REQUIRE(stream.good());
    return {std::istreambuf_iterator<char>{stream},
            std::istreambuf_iterator<char>{}};
}

void overwrite_file_bytes(const std::filesystem::path &path,
                          std::string_view bytes) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    REQUIRE(stream.good());
    stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    REQUIRE(stream.good());
}

void replace_path_atomically(const std::filesystem::path &replacement,
                             const std::filesystem::path &destination) {
#ifdef _WIN32
    REQUIRE(MoveFileExW(replacement.c_str(), destination.c_str(),
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) !=
            0);
#else
    std::error_code rename_error;
    std::filesystem::rename(replacement, destination, rename_error);
    REQUIRE_FALSE(rename_error);
#endif
}

void check_source_changed_error(const featherdoc::Document &document) {
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::source_archive_changed);
    CHECK(featherdoc::detail::is_valid_utf8(document.last_error().detail));
    CHECK(featherdoc::detail::is_valid_utf8(document.last_error().entry_name));
}

class source_consistency_test_directory final {
  private:
    std::filesystem::path path_;

  public:
    source_consistency_test_directory() {
        std::error_code error;
        auto parent = std::filesystem::temp_directory_path(error);
        if (error) {
            throw std::runtime_error{
                "failed to resolve the temporary test directory"};
        }

        parent /= "FeatherDoc_source_consistency";
        std::filesystem::create_directories(parent, error);
        if (error) {
            throw std::runtime_error{
                "failed to create the source-consistency test root"};
        }

        const auto nonce =
            std::chrono::high_resolution_clock::now()
                .time_since_epoch()
                .count();
        for (std::size_t attempt = 0U; attempt < 1024U; ++attempt) {
            auto candidate =
                parent / (std::to_string(nonce) + "-" +
                          std::to_string(attempt));
            error.clear();
            if (std::filesystem::create_directory(candidate, error)) {
                this->path_ = std::move(candidate);
                return;
            }
        }

        throw std::runtime_error{
            "failed to reserve a unique source-consistency test directory"};
    }

    source_consistency_test_directory(
        const source_consistency_test_directory &) = delete;
    auto operator=(const source_consistency_test_directory &)
        -> source_consistency_test_directory & = delete;

    ~source_consistency_test_directory() {
        std::error_code ignored;
        std::filesystem::remove_all(this->path_, ignored);
    }

    [[nodiscard]] const std::filesystem::path &path() const noexcept {
        return this->path_;
    }
};

[[nodiscard]] const std::filesystem::path &
source_consistency_test_root() {
    static const source_consistency_test_directory directory;
    return directory.path();
}

class scoped_current_path final {
  private:
    std::filesystem::path original_path_;

  public:
    explicit scoped_current_path(const std::filesystem::path &path)
        : original_path_(std::filesystem::current_path()) {
        std::filesystem::current_path(path);
    }

    scoped_current_path(const scoped_current_path &) = delete;
    auto operator=(const scoped_current_path &) -> scoped_current_path & =
        delete;

    ~scoped_current_path() {
        std::error_code ignored;
        std::filesystem::current_path(this->original_path_, ignored);
    }

    void set(const std::filesystem::path &path) {
        std::filesystem::current_path(path);
    }
};

#ifdef _WIN32
auto enable_case_sensitive_directory(const std::filesystem::path &path)
    -> bool {
    const auto directory_handle = CreateFileW(
        path.c_str(), FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (directory_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    // FILE_CASE_SENSITIVE_INFO and FileCaseSensitiveInfo are not declared by
    // every supported Windows SDK. Their stable ABI is a single ULONG and
    // FILE_INFO_BY_HANDLE_CLASS value 23 respectively.
    struct case_sensitive_directory_info final {
        ULONG flags;
    };
    constexpr auto file_case_sensitive_info =
        static_cast<FILE_INFO_BY_HANDLE_CLASS>(23);
    constexpr ULONG case_sensitive_directory_flag = 0x00000001U;
    auto info = case_sensitive_directory_info{case_sensitive_directory_flag};
    const auto enabled =
        SetFileInformationByHandle(directory_handle, file_case_sensitive_info,
                                   &info, sizeof(info)) != 0;
    (void)CloseHandle(directory_handle);
    return enabled;
}
#endif

} // namespace

TEST_CASE("relative source paths stay bound after the working directory "
          "changes") {
    namespace fs = std::filesystem;
    const auto scenario_root =
        source_consistency_test_root() / "relative_source_path_binding";
    const auto source_directory = scenario_root / "source";
    const auto other_directory = scenario_root / "other";
    const auto relative_source = fs::path{"bound-source.docx"};
    const auto relative_copy = fs::path{"relative-copy.docx"};
    std::error_code cleanup_error;
    fs::remove_all(scenario_root, cleanup_error);
    REQUIRE(fs::create_directories(source_directory));
    REQUIRE(fs::create_directories(other_directory));

    const auto source = source_directory / relative_source;
    const auto decoy = other_directory / relative_source;
    write_source_archive(source, "source-document", "source-marker");
    write_source_archive(decoy, "decoy-document", "decoy-marker");

    {
        scoped_current_path current_path{source_directory};
        featherdoc::Document document(relative_source);
        REQUIRE_FALSE(document.open());
        CHECK_EQ(document.path(), relative_source);

        current_path.set(other_directory);
        const auto update_fields = document.update_fields_on_open_enabled();
        REQUIRE(update_fields.has_value());
        CHECK(*update_fields);

        REQUIRE(document.paragraphs().add_run(" bound-source edit").has_next());
        REQUIRE_FALSE(document.save());
        CHECK(fs::exists(source));
        CHECK(fs::exists(decoy));

        REQUIRE_FALSE(document.save_as(relative_copy));
        CHECK(fs::exists(other_directory / relative_copy));
        CHECK_FALSE(fs::exists(source_directory / relative_copy));
    }

    const auto source_xml = read_test_docx_entry(source, test_document_xml_entry);
    const auto decoy_xml = read_test_docx_entry(decoy, test_document_xml_entry);
    CHECK(source_xml.find("bound-source edit") != std::string::npos);
    CHECK(decoy_xml.find("bound-source edit") == std::string::npos);

    fs::remove_all(scenario_root, cleanup_error);
}

TEST_CASE("save rejects an atomically replaced source archive without touching "
          "the target") {
    namespace fs = std::filesystem;
    const auto source =
        source_consistency_test_root() / "source_consistency_atomic_源😀.docx";
    const auto replacement =
        source_consistency_test_root() /
        "source_consistency_atomic_replacement.docx";
    const auto target =
        source_consistency_test_root() /
        "source_consistency_atomic_target.docx";
    fs::remove(source);
    fs::remove(replacement);
    fs::remove(target);

    write_source_archive(source, "AAAA", "source-A");
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.paragraphs().add_run(" edited in memory").has_next());

    write_source_archive(replacement, "BBBB", "source-B");
    replace_path_atomically(replacement, source);
    overwrite_file_bytes(target, "existing-target");

    CHECK(document.save_as(target));
    check_source_changed_error(document);
    CHECK_EQ(read_file_bytes(target), "existing-target");

    fs::remove(source);
    fs::remove(target);
}

TEST_CASE("save detects an in-place source rewrite by CRC even when size and "
          "mtime are restored") {
    namespace fs = std::filesystem;
    const auto source = source_consistency_test_root() /
                        "source_consistency_in_place.docx";
    const auto rewritten =
        source_consistency_test_root() /
        "source_consistency_in_place_bytes.docx";
    const auto target =
        source_consistency_test_root() /
        "source_consistency_in_place_target.docx";
    fs::remove(source);
    fs::remove(rewritten);
    fs::remove(target);

    write_source_archive(source, "AAAA", "source-A");
    const auto original_size = fs::file_size(source);
    const auto original_mtime = fs::last_write_time(source);
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());

    write_source_archive(rewritten, "BBBB", "source-B");
    const auto rewritten_bytes = read_file_bytes(rewritten);
    REQUIRE_EQ(rewritten_bytes.size(), original_size);
    overwrite_file_bytes(source, rewritten_bytes);
    fs::last_write_time(source, original_mtime);
    REQUIRE_EQ(fs::file_size(source), original_size);
    REQUIRE_EQ(fs::last_write_time(source), original_mtime);
    overwrite_file_bytes(target, "existing-target");

    CHECK(document.save_as(target));
    check_source_changed_error(document);
    CHECK_EQ(read_file_bytes(target), "existing-target");

    fs::remove(source);
    fs::remove(rewritten);
    fs::remove(target);
}

TEST_CASE("lazy source reads reject replacement and unchanged source remains "
          "usable") {
    namespace fs = std::filesystem;
    const auto source =
        source_consistency_test_root() / "source_consistency_lazy.docx";
    const auto replacement =
        source_consistency_test_root() /
        "source_consistency_lazy_replacement.docx";
    fs::remove(source);
    fs::remove(replacement);

    write_source_archive(source, "AAAA", "source-A");
    featherdoc::Document unchanged(source);
    REQUIRE_FALSE(unchanged.open());
    const auto unchanged_setting = unchanged.update_fields_on_open_enabled();
    REQUIRE(unchanged_setting.has_value());
    CHECK(*unchanged_setting);

    featherdoc::Document replaced(source);
    REQUIRE_FALSE(replaced.open());
    featherdoc::Document custom_xml_replaced(source);
    REQUIRE_FALSE(custom_xml_replaced.open());
    write_source_archive(replacement, "BBBB", "source-B");
    replace_path_atomically(replacement, source);

    CHECK_FALSE(replaced.update_fields_on_open_enabled().has_value());
    check_source_changed_error(replaced);
    CHECK_FALSE(custom_xml_replaced.sync_content_controls_from_custom_xml()
                    .has_value());
    check_source_changed_error(custom_xml_replaced);

    fs::remove(source);
}

TEST_CASE("optional review XML lazy loads reject a replaced source archive") {
    namespace fs = std::filesystem;
    const auto source =
        source_consistency_test_root() /
        "source_consistency_review_helper.docx";
    const auto replacement =
        source_consistency_test_root() /
        "source_consistency_review_replacement.docx";
    fs::remove(source);
    fs::remove(replacement);

    write_footnote_source_archive(source, "note-A");
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());

    write_footnote_source_archive(replacement, "note-B");
    replace_path_atomically(replacement, source);

    CHECK(document.list_footnotes().empty());
    check_source_changed_error(document);
    CHECK_EQ(document.last_error().entry_name, "word/footnotes.xml");

    fs::remove(source);
}

TEST_CASE("optional review XML lazy loads report a deleted source archive") {
    namespace fs = std::filesystem;
    const auto source =
        source_consistency_test_root() /
        "source_consistency_review_deleted.docx";
    fs::remove(source);

    write_footnote_source_archive(source, "note-A");
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(fs::remove(source));

    CHECK(document.list_footnotes().empty());
    check_source_changed_error(document);
    CHECK_EQ(document.last_error().entry_name, "word/footnotes.xml");
}

TEST_CASE("lazy reopen wraps reader preflight limits as source changes") {
    namespace fs = std::filesystem;
    const auto source =
        source_consistency_test_root() /
        "source_consistency_reader_limit.docx";
    const auto replacement =
        source_consistency_test_root() /
        "source_consistency_reader_limit_replacement.docx";
    fs::remove(source);
    fs::remove(replacement);

    write_source_archive(source, "AAAA", "source-A");
    featherdoc::document_open_options options;
    options.limits.max_entries = 6U;
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open(options));

    write_source_archive(replacement, "BBBB", "source-B");
    auto replacement_entries = read_test_archive_entries(replacement);
    replacement_entries.emplace_back("word/media/extra.bin", "extra");
    write_test_archive_entries(replacement, replacement_entries);
    replace_path_atomically(replacement, source);

    CHECK_FALSE(document.update_fields_on_open_enabled().has_value());
    check_source_changed_error(document);
    CHECK(document.last_error().detail.find("entry count") !=
          std::string::npos);
    CHECK(document.last_error().detail.find("limit is 6") != std::string::npos);

    fs::remove(source);
}

TEST_CASE("lazy reopen preserves metadata limit detail when wrapping a source "
          "change") {
    namespace fs = std::filesystem;
    const auto source =
        source_consistency_test_root() /
        "source_consistency_metadata_limit.docx";
    const auto replacement = source_consistency_test_root() /
                             "source_consistency_metadata_limit_replacement.docx";
    fs::remove(source);
    fs::remove(replacement);

    write_source_archive(source, "AAAA", "12345678");
    featherdoc::document_open_options options;
    options.limits.max_binary_part_bytes = 8U;
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open(options));

    write_source_archive(replacement, "BBBB", "123456789");
    replace_path_atomically(replacement, source);

    CHECK_FALSE(document.update_fields_on_open_enabled().has_value());
    check_source_changed_error(document);
    CHECK_EQ(document.last_error().entry_name,
             "word/media/source-marker.bin");
    CHECK(document.last_error().detail.find("is 9 bytes") !=
          std::string::npos);
    CHECK(document.last_error().detail.find("limit is 8") !=
          std::string::npos);

    fs::remove(source);
}

TEST_CASE("saving back to the source refreshes its fingerprint while save_as "
          "keeps the original source") {
    namespace fs = std::filesystem;
    const auto source =
        source_consistency_test_root() / "source_consistency_repeat.docx";
    const auto first_copy =
        source_consistency_test_root() /
        "source_consistency_repeat_copy.docx";
    const auto second_copy =
        source_consistency_test_root() /
        "source_consistency_repeat_second_copy.docx";
    fs::remove(source);
    fs::remove(first_copy);
    fs::remove(second_copy);

    write_source_archive(source, "AAAA", "source-A");
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.paragraphs().add_run(" first save").has_next());
    REQUIRE_FALSE(document.save());
    REQUIRE(document.paragraphs().add_run(" second save").has_next());
    REQUIRE_FALSE(document.save());

    REQUIRE_FALSE(document.save_as(first_copy));
    write_source_archive(first_copy, "BBBB", "source-B");
    REQUIRE_FALSE(document.save_as(second_copy));

    featherdoc::Document verification(second_copy);
    REQUIRE_FALSE(verification.open());
    const auto xml = read_test_docx_entry(second_copy, test_document_xml_entry);
    CHECK(xml.find("first save") != std::string::npos);
    CHECK(xml.find("second save") != std::string::npos);

    fs::remove(source);
    fs::remove(first_copy);
    fs::remove(second_copy);
}

TEST_CASE("saving to a distinct hard-link directory entry keeps the original "
          "source snapshot") {
    namespace fs = std::filesystem;
    const auto source =
        source_consistency_test_root() /
        "source_consistency_hard_link_source.docx";
    const auto hard_link =
        source_consistency_test_root() /
        "source_consistency_hard_link_target.docx";
    const auto second_copy =
        source_consistency_test_root() /
        "source_consistency_hard_link_second.docx";
    fs::remove(source);
    fs::remove(hard_link);
    fs::remove(second_copy);

    write_source_archive(source, "AAAA", "source-A");
    std::error_code hard_link_error;
    fs::create_hard_link(source, hard_link, hard_link_error);
    if (hard_link_error) {
        MESSAGE("skipping hard-link source identity test: "
                << hard_link_error.message());
        fs::remove(source);
        fs::remove(hard_link);
        return;
    }

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.paragraphs().add_run(" hard-link first save").has_next());
    REQUIRE_FALSE(document.save_as(hard_link));

    const auto unchanged_source_xml =
        read_test_docx_entry(source, test_document_xml_entry);
    CHECK(unchanged_source_xml.find("hard-link first save") ==
          std::string::npos);

    REQUIRE(document.paragraphs().add_run(" hard-link second save").has_next());
    REQUIRE_FALSE(document.save_as(second_copy));
    const auto saved_xml =
        read_test_docx_entry(second_copy, test_document_xml_entry);
    CHECK(saved_xml.find("hard-link first save") != std::string::npos);
    CHECK(saved_xml.find("hard-link second save") != std::string::npos);

    fs::remove(source);
    fs::remove(hard_link);
    fs::remove(second_copy);
}

TEST_CASE("saving over an output symlink keeps the original source snapshot") {
    namespace fs = std::filesystem;
    const auto source =
        source_consistency_test_root() /
        "source_consistency_output_symlink_source.docx";
    const auto output_symlink =
        source_consistency_test_root() /
        "source_consistency_output_symlink_target.docx";
    std::error_code cleanup_error;
    fs::remove(output_symlink, cleanup_error);
    cleanup_error.clear();
    fs::remove(source, cleanup_error);

    write_source_archive(source, "AAAA", "source-A");
    std::error_code symlink_error;
    fs::create_symlink(source, output_symlink, symlink_error);
    if (symlink_error) {
        MESSAGE("skipping output-symlink source identity test: "
                << symlink_error.message());
        fs::remove(source, cleanup_error);
        return;
    }

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.paragraphs().add_run(" output-symlink save").has_next());
    REQUIRE_FALSE(document.save_as(output_symlink));
    const auto output_status = fs::symlink_status(output_symlink);
    CHECK_FALSE(fs::is_symlink(output_status));

    const auto source_xml = read_test_docx_entry(source, test_document_xml_entry);
    CHECK(source_xml.find("output-symlink save") == std::string::npos);
    const auto output_xml =
        read_test_docx_entry(output_symlink, test_document_xml_entry);
    CHECK(output_xml.find("output-symlink save") != std::string::npos);

    const auto lazy_setting = document.update_fields_on_open_enabled();
    REQUIRE(lazy_setting.has_value());
    CHECK(*lazy_setting);

    fs::remove(output_symlink, cleanup_error);
    fs::remove(source, cleanup_error);
}

TEST_CASE("saving to a source symlink target refreshes the source snapshot") {
    namespace fs = std::filesystem;
    const auto source_target =
        source_consistency_test_root() /
        "source_consistency_source_symlink_target.docx";
    const auto source_symlink =
        source_consistency_test_root() /
        "source_consistency_source_symlink.docx";
    std::error_code cleanup_error;
    fs::remove(source_symlink, cleanup_error);
    cleanup_error.clear();
    fs::remove(source_target, cleanup_error);

    write_source_archive(source_target, "AAAA", "source-A");
    std::error_code symlink_error;
    fs::create_symlink(source_target, source_symlink, symlink_error);
    if (symlink_error) {
        MESSAGE("skipping source-symlink source identity test: "
                << symlink_error.message());
        fs::remove(source_target, cleanup_error);
        return;
    }

    featherdoc::Document document(source_symlink);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.paragraphs().add_run(" source-symlink save").has_next());
    REQUIRE_FALSE(document.save_as(source_target));

    const auto lazy_setting = document.update_fields_on_open_enabled();
    REQUIRE(lazy_setting.has_value());
    CHECK(*lazy_setting);
    const auto saved_xml =
        read_test_docx_entry(source_symlink, test_document_xml_entry);
    CHECK(saved_xml.find("source-symlink save") != std::string::npos);

    fs::remove(source_symlink, cleanup_error);
    fs::remove(source_target, cleanup_error);
}

TEST_CASE("distinct source and output symlinks to one target stay distinct") {
    namespace fs = std::filesystem;
    const auto shared_target =
        source_consistency_test_root() /
        "source_consistency_shared_symlink_target.docx";
    const auto source_symlink =
        source_consistency_test_root() /
        "source_consistency_shared_symlink_source.docx";
    const auto output_symlink =
        source_consistency_test_root() /
        "source_consistency_shared_symlink_output.docx";
    std::error_code cleanup_error;
    fs::remove(source_symlink, cleanup_error);
    cleanup_error.clear();
    fs::remove(output_symlink, cleanup_error);
    cleanup_error.clear();
    fs::remove(shared_target, cleanup_error);

    write_source_archive(shared_target, "AAAA", "source-A");
    std::error_code source_symlink_error;
    fs::create_symlink(shared_target, source_symlink, source_symlink_error);
    std::error_code output_symlink_error;
    fs::create_symlink(shared_target, output_symlink, output_symlink_error);
    if (source_symlink_error || output_symlink_error) {
        MESSAGE("skipping distinct-symlink source identity test: "
                << (source_symlink_error ? source_symlink_error.message()
                                         : output_symlink_error.message()));
        fs::remove(source_symlink, cleanup_error);
        fs::remove(output_symlink, cleanup_error);
        fs::remove(shared_target, cleanup_error);
        return;
    }

    featherdoc::Document document(source_symlink);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.paragraphs().add_run(" distinct-symlink save").has_next());
    REQUIRE_FALSE(document.save_as(output_symlink));
    const auto output_status = fs::symlink_status(output_symlink);
    CHECK_FALSE(fs::is_symlink(output_status));

    const auto unchanged_source_xml =
        read_test_docx_entry(source_symlink, test_document_xml_entry);
    CHECK(unchanged_source_xml.find("distinct-symlink save") ==
          std::string::npos);
    const auto output_xml =
        read_test_docx_entry(output_symlink, test_document_xml_entry);
    CHECK(output_xml.find("distinct-symlink save") != std::string::npos);

    const auto lazy_setting = document.update_fields_on_open_enabled();
    REQUIRE(lazy_setting.has_value());
    CHECK(*lazy_setting);

    fs::remove(source_symlink, cleanup_error);
    fs::remove(output_symlink, cleanup_error);
    fs::remove(shared_target, cleanup_error);
}

#ifdef _WIN32
TEST_CASE("saving through a Win32 trailing-dot alias refreshes the source "
          "snapshot") {
    namespace fs = std::filesystem;
    const auto source =
        source_consistency_test_root() /
        "source_consistency_trailing_dot.docx";
    const auto trailing_dot_alias = fs::path{source.native() + L"."};
    const auto second_copy =
        source_consistency_test_root() /
        "source_consistency_trailing_dot_second.docx";
    fs::remove(source);
    fs::remove(trailing_dot_alias);
    fs::remove(second_copy);

    write_source_archive(source, "AAAA", "source-A");
    std::error_code alias_error;
    const auto is_alias = fs::equivalent(source, trailing_dot_alias,
                                         alias_error);
    if (alias_error || !is_alias) {
        MESSAGE("skipping trailing-dot source identity test: the filesystem "
                "does not expose the Win32 alias");
        fs::remove(source);
        fs::remove(trailing_dot_alias);
        return;
    }

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.paragraphs().add_run(" trailing-dot first save")
                .has_next());
    REQUIRE_FALSE(document.save_as(trailing_dot_alias));
    REQUIRE(document.paragraphs().add_run(" trailing-dot second save")
                .has_next());
    REQUIRE_FALSE(document.save_as(second_copy));

    const auto saved_xml =
        read_test_docx_entry(second_copy, test_document_xml_entry);
    CHECK(saved_xml.find("trailing-dot first save") != std::string::npos);
    CHECK(saved_xml.find("trailing-dot second save") != std::string::npos);

    fs::remove(trailing_dot_alias);
    fs::remove(source);
    fs::remove(second_copy);
}

TEST_CASE("case-only aliases in a case-insensitive directory refresh the "
          "source snapshot") {
    namespace fs = std::filesystem;
    const auto source =
        source_consistency_test_root() /
        "Source_Consistency_Case_Alias.docx";
    const auto case_alias =
        source_consistency_test_root() /
        "source_consistency_case_alias.docx";
    const auto second_copy =
        source_consistency_test_root() /
        "source_consistency_case_alias_second.docx";
    fs::remove(source);
    fs::remove(case_alias);
    fs::remove(second_copy);

    write_source_archive(source, "AAAA", "source-A");
    std::error_code alias_error;
    const auto is_alias = fs::equivalent(source, case_alias, alias_error);
    if (alias_error || !is_alias) {
        MESSAGE("skipping case-only source identity test: the test directory "
                "is not case-insensitive");
        fs::remove(source);
        fs::remove(case_alias);
        return;
    }

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.paragraphs().add_run(" case-alias first save")
                .has_next());
    REQUIRE_FALSE(document.save_as(case_alias));
    REQUIRE(document.paragraphs().add_run(" case-alias second save")
                .has_next());
    REQUIRE_FALSE(document.save_as(second_copy));

    const auto saved_xml =
        read_test_docx_entry(second_copy, test_document_xml_entry);
    CHECK(saved_xml.find("case-alias first save") != std::string::npos);
    CHECK(saved_xml.find("case-alias second save") != std::string::npos);

    fs::remove(case_alias);
    fs::remove(source);
    fs::remove(second_copy);
}

TEST_CASE("case-sensitive Windows directories keep case-distinct source "
          "entries separate") {
    namespace fs = std::filesystem;
    const auto directory =
        source_consistency_test_root() /
        "source_consistency_case_sensitive_directory";
    std::error_code cleanup_error;
    fs::remove_all(directory, cleanup_error);
    cleanup_error.clear();
    fs::create_directory(directory, cleanup_error);
    if (cleanup_error || !enable_case_sensitive_directory(directory)) {
        MESSAGE("skipping case-sensitive source identity test: the temporary "
                "directory cannot safely enable case sensitivity");
        fs::remove_all(directory, cleanup_error);
        return;
    }

    const auto source = directory / "a.docx";
    const auto case_distinct_target = directory / "A.docx";
    const auto second_copy = directory / "second.docx";
    write_source_archive(source, "AAAA", "source-A");
    write_source_archive(case_distinct_target, "BBBB", "source-B");

    std::error_code identity_error;
    const auto same_entry =
        fs::equivalent(source, case_distinct_target, identity_error);
    if (identity_error || same_entry) {
        MESSAGE("skipping case-sensitive source identity test: distinct-case "
                "files could not be established");
        fs::remove_all(directory, cleanup_error);
        return;
    }

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.paragraphs().add_run(" case-sensitive first save")
                .has_next());
    REQUIRE_FALSE(document.save_as(case_distinct_target));

    const auto unchanged_source_xml =
        read_test_docx_entry(source, test_document_xml_entry);
    CHECK(unchanged_source_xml.find("case-sensitive first save") ==
          std::string::npos);

    REQUIRE(document.paragraphs().add_run(" case-sensitive second save")
                .has_next());
    REQUIRE_FALSE(document.save_as(second_copy));
    const auto saved_xml =
        read_test_docx_entry(second_copy, test_document_xml_entry);
    CHECK(saved_xml.find("case-sensitive first save") != std::string::npos);
    CHECK(saved_xml.find("case-sensitive second save") != std::string::npos);

    fs::remove_all(directory, cleanup_error);
}
#else
TEST_CASE("saving through a symlinked parent refreshes the source snapshot") {
    namespace fs = std::filesystem;
    const auto real_directory =
        source_consistency_test_root() / "source_consistency_real_parent";
    const auto alias_directory =
        source_consistency_test_root() / "source_consistency_alias_parent";
    const auto second_copy =
        source_consistency_test_root() /
        "source_consistency_symlink_parent_second.docx";
    std::error_code cleanup_error;
    fs::remove_all(alias_directory, cleanup_error);
    cleanup_error.clear();
    fs::remove_all(real_directory, cleanup_error);
    fs::remove(second_copy, cleanup_error);
    cleanup_error.clear();
    fs::create_directory(real_directory, cleanup_error);
    REQUIRE_FALSE(cleanup_error);

    std::error_code symlink_error;
    fs::create_directory_symlink(real_directory, alias_directory,
                                 symlink_error);
    if (symlink_error) {
        MESSAGE("skipping symlinked-parent source identity test: "
                << symlink_error.message());
        fs::remove_all(real_directory, cleanup_error);
        return;
    }

    const auto source = real_directory / "source.docx";
    const auto source_alias = alias_directory / "source.docx";
    write_source_archive(source, "AAAA", "source-A");

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.paragraphs().add_run(" symlink-parent first save")
                .has_next());
    REQUIRE_FALSE(document.save_as(source_alias));
    REQUIRE(document.paragraphs().add_run(" symlink-parent second save")
                .has_next());
    REQUIRE_FALSE(document.save_as(second_copy));

    const auto saved_xml =
        read_test_docx_entry(second_copy, test_document_xml_entry);
    CHECK(saved_xml.find("symlink-parent first save") != std::string::npos);
    CHECK(saved_xml.find("symlink-parent second save") != std::string::npos);

    fs::remove_all(alias_directory, cleanup_error);
    fs::remove_all(real_directory, cleanup_error);
    fs::remove(second_copy, cleanup_error);
}
#endif

#ifndef _WIN32
TEST_CASE("source snapshot refreshes after replacement even when directory "
          "sync reports a durability error") {
    namespace fs = std::filesystem;
    const auto source =
        source_consistency_test_root() /
        "source_consistency_directory_sync.docx";
    fs::remove(source);

    write_source_archive(source, "AAAA", "source-A");
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.paragraphs().add_run(" first save").has_next());

    featherdoc_test::document_fail_next_sync_stage(
        featherdoc_test::document_sync_failure_parent_directory);
    const auto first_save_error = document.save();
    CHECK_EQ(
        first_save_error,
        featherdoc::document_errc::output_directory_sync_failed_after_replace);

    REQUIRE(document.paragraphs().add_run(" retry").has_next());
    CHECK_FALSE(document.save());

    fs::remove(source);
}
#endif

TEST_CASE("same-source save rejects output one byte over the opened XML limit "
          "without replacing the source") {
    namespace fs = std::filesystem;
    const auto source =
        source_consistency_test_root() /
        "source_consistency_output_limit.docx";
    fs::remove(source);
    const auto original_text = std::string(4096U, 'A');

    featherdoc::Document seed(source);
    REQUIRE_FALSE(seed.create_empty());
    auto seed_paragraph = seed.paragraphs();
    REQUIRE(seed_paragraph.has_next());
    REQUIRE(seed_paragraph.set_text(original_text));
    REQUIRE_FALSE(seed.save());

    const auto original_archive_bytes = read_file_bytes(source);
    const auto original_document_xml =
        read_test_docx_entry(source, test_document_xml_entry);
    REQUIRE_FALSE(original_document_xml.empty());

    featherdoc::document_open_options options;
    options.limits.max_xml_part_bytes = original_document_xml.size();
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open(options));
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.set_text(original_text + "B"));

    const auto save_error = document.save();
    CHECK_EQ(save_error, featherdoc::document_errc::archive_limit_exceeded);
    CHECK_EQ(document.last_error().entry_name, test_document_xml_entry);
    CHECK(document.last_error().detail.find(
              "is " + std::to_string(original_document_xml.size() + 1U) +
              " bytes") != std::string::npos);
    CHECK(document.last_error().detail.find(
              "limit is " + std::to_string(original_document_xml.size())) !=
          std::string::npos);
    CHECK_EQ(read_file_bytes(source), original_archive_bytes);
    CHECK(document.is_open());

    REQUIRE(paragraph.set_text(original_text));
    CHECK_FALSE(document.save());
    featherdoc::Document reopened(source);
    CHECK_FALSE(reopened.open(options));

    fs::remove(source);
}
