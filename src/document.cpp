#include "featherdoc.hpp"
#include "document_section_xml_helpers.hpp"
#include "xml_helpers.hpp"

#include <featherdoc/detail/path.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include <zip.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};
constexpr auto document_relationships_xml_entry =
    std::string_view{"word/_rels/document.xml.rels"};
constexpr auto relationships_xml_entry = std::string_view{"_rels/.rels"};
constexpr auto content_types_xml_entry = std::string_view{"[Content_Types].xml"};
constexpr auto main_document_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"};
constexpr auto office_document_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"};
constexpr auto wordprocessingml_namespace = std::string_view{
    "http://schemas.openxmlformats.org/wordprocessingml/2006/main"};
constexpr auto package_relationships_namespace = std::string_view{
    "http://schemas.openxmlformats.org/package/2006/relationships"};
constexpr auto package_content_types_namespace = std::string_view{
    "http://schemas.openxmlformats.org/package/2006/content-types"};
constexpr auto settings_xml_entry = std::string_view{"word/settings.xml"};
constexpr auto numbering_xml_entry = std::string_view{"word/numbering.xml"};
constexpr auto styles_xml_entry = std::string_view{"word/styles.xml"};
constexpr auto footnotes_xml_entry = std::string_view{"word/footnotes.xml"};
constexpr auto endnotes_xml_entry = std::string_view{"word/endnotes.xml"};
constexpr auto comments_xml_entry = std::string_view{"word/comments.xml"};
constexpr auto comments_extended_xml_entry =
    std::string_view{"word/commentsExtended.xml"};
constexpr auto office_document_relationships_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships"};
constexpr auto header_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"};
constexpr auto footer_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"};
constexpr auto settings_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings"};
constexpr auto footnotes_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/footnotes"};
constexpr auto endnotes_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/endnotes"};
constexpr auto comments_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/comments"};
constexpr auto comments_extended_relationship_type = std::string_view{
    "http://schemas.microsoft.com/office/2011/relationships/commentsExtended"};
constexpr auto header_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"};
constexpr auto footer_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"};
constexpr auto settings_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"};
constexpr auto footnotes_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.footnotes+xml"};
constexpr auto endnotes_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.endnotes+xml"};
constexpr auto comments_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.comments+xml"};
constexpr auto comments_extended_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.commentsExtended+xml"};
constexpr auto empty_document_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p/>
  </w:body>
</w:document>
)"};
constexpr auto relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)"};
constexpr auto content_types_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)"};
constexpr auto empty_relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
</Relationships>
)"}; 
constexpr auto empty_header_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p/>
</w:hdr>
)"}; 
constexpr auto empty_footer_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p/>
</w:ftr>
)"}; 
constexpr auto empty_settings_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
</w:settings>
)"}; 
constexpr int docx_output_compression_level = 0;

struct packaged_entry final {
    std::string_view name;
    std::string_view content;
};

constexpr auto minimal_docx_entries = std::array{
    packaged_entry{relationships_xml_entry, relationships_xml},
};

struct related_part_entry final {
    std::string relationship_id;
    std::string relationship_type;
    std::string entry_name;
};

struct xml_zip_writer final : pugi::xml_writer {
    zip_t *archive{nullptr};
    bool failed{false};

    explicit xml_zip_writer(zip_t *archive_handle) : archive(archive_handle) {}

    void write(const void *data, size_t size) override {
        if (this->failed || size == 0) {
            return;
        }

        if (zip_entry_write(this->archive, data, size) < 0) {
            this->failed = true;
        }
    }
};

struct zip_entry_copy_context {
    zip_t *target_archive{nullptr};
    bool failed{false};
};

auto copy_zip_entry_chunk(void *arg, std::uint64_t /*offset*/, const void *data,
                          size_t size) -> size_t {
    auto *context = static_cast<zip_entry_copy_context *>(arg);
    if (context == nullptr || context->failed || size == 0) {
        return 0;
    }

    if (zip_entry_write(context->target_archive, data, size) < 0) {
        context->failed = true;
        return 0;
    }

    return size;
}

auto zip_error_text(int error_number) -> std::string {
    if (const char *message = zip_strerror(error_number); message != nullptr) {
        return message;
    }

    return "unknown zip error";
}

auto reserve_unique_temp_file(const std::filesystem::path &output_file,
                              std::filesystem::path &temp_file,
                              std::FILE *&temp_stream)
    -> std::error_code {
    static std::atomic<std::uint64_t> sequence{0U};
    const auto timestamp = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    temp_stream = nullptr;

    for (std::uint32_t attempt = 0; attempt < 64U; ++attempt) {
        temp_file = output_file;
        temp_file += ".featherdoc-" + std::to_string(timestamp) + "-" +
                     std::to_string(sequence.fetch_add(1U)) + ".tmp";

#ifdef _WIN32
        int descriptor = -1;
        const auto open_error = _wsopen_s(
            &descriptor, temp_file.c_str(),
            _O_CREAT | _O_EXCL | _O_RDWR | _O_BINARY | _O_NOINHERIT,
            _SH_DENYRW,
            _S_IREAD | _S_IWRITE);
        if (open_error == 0) {
            temp_stream = _wfdopen(descriptor, L"w+b");
            if (temp_stream != nullptr) {
                return {};
            }
            const auto stream_error = errno;
            _close(descriptor);
            std::error_code cleanup_error;
            std::filesystem::remove(temp_file, cleanup_error);
            return {stream_error, std::generic_category()};
        }
        if (open_error != EEXIST) {
            return {open_error, std::generic_category()};
        }
#else
        int open_flags = O_CREAT | O_EXCL | O_RDWR;
#ifdef O_CLOEXEC
        open_flags |= O_CLOEXEC;
#endif
        const int descriptor = ::open(temp_file.c_str(), open_flags, 0600);
        if (descriptor >= 0) {
            temp_stream = ::fdopen(descriptor, "w+b");
            if (temp_stream != nullptr) {
                return {};
            }
            const auto stream_error = errno;
            ::close(descriptor);
            std::error_code cleanup_error;
            std::filesystem::remove(temp_file, cleanup_error);
            return {stream_error, std::generic_category()};
        }
        if (errno != EEXIST) {
            return {errno, std::generic_category()};
        }
#endif
    }

    return std::make_error_code(std::errc::file_exists);
}

auto replace_file_atomically(const std::filesystem::path &temp_file,
                             const std::filesystem::path &output_file)
    -> std::error_code {
#ifdef _WIN32
    constexpr std::uint32_t max_replace_attempts = 16U;
    const auto is_transient_replace_error = [](DWORD error) {
        return error == ERROR_SHARING_VIOLATION ||
               error == ERROR_LOCK_VIOLATION ||
               error == ERROR_ACCESS_DENIED ||
               error == ERROR_UNABLE_TO_MOVE_REPLACEMENT ||
               error == ERROR_UNABLE_TO_REMOVE_REPLACED;
    };

    for (std::uint32_t attempt = 0U; attempt < max_replace_attempts; ++attempt) {
        DWORD operation_error = ERROR_SUCCESS;
        const auto attributes = GetFileAttributesW(output_file.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES) {
            if (ReplaceFileW(output_file.c_str(), temp_file.c_str(), nullptr,
                             REPLACEFILE_WRITE_THROUGH, nullptr, nullptr) != 0) {
                return {};
            }
            operation_error = GetLastError();
            if (operation_error != ERROR_FILE_NOT_FOUND &&
                operation_error != ERROR_PATH_NOT_FOUND) {
                if (!is_transient_replace_error(operation_error) ||
                    attempt + 1U == max_replace_attempts) {
                    return {static_cast<int>(operation_error),
                            std::system_category()};
                }
                std::this_thread::sleep_for(std::chrono::milliseconds{2});
                continue;
            }
        }
        else {
            operation_error = GetLastError();
            if (operation_error != ERROR_FILE_NOT_FOUND &&
                operation_error != ERROR_PATH_NOT_FOUND) {
                return {static_cast<int>(operation_error),
                        std::system_category()};
            }
        }

        if (MoveFileExW(temp_file.c_str(), output_file.c_str(),
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0) {
            return {};
        }
        operation_error = GetLastError();
        if (!is_transient_replace_error(operation_error) ||
            attempt + 1U == max_replace_attempts) {
            return {static_cast<int>(operation_error), std::system_category()};
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    return {static_cast<int>(ERROR_RETRY), std::system_category()};
#else
    if (::rename(temp_file.c_str(), output_file.c_str()) == 0) {
        return {};
    }
    return {errno, std::generic_category()};
#endif
}

auto split_plain_text_paragraphs(std::string_view text) -> std::vector<std::string> {
    std::vector<std::string> paragraphs;
    std::size_t begin = 0U;
    while (begin <= text.size()) {
        const auto line_end = text.find('\n', begin);
        const auto end = line_end == std::string_view::npos ? text.size() : line_end;

        std::string paragraph_text(text.substr(begin, end - begin));
        if (!paragraph_text.empty() && paragraph_text.back() == '\r') {
            paragraph_text.pop_back();
        }
        paragraphs.push_back(std::move(paragraph_text));

        if (line_end == std::string_view::npos) {
            break;
        }

        begin = end + 1U;
    }

    if (paragraphs.empty()) {
        paragraphs.emplace_back();
    }

    if (!text.empty() && (text.ends_with('\n') || text.ends_with('\r'))) {
        while (paragraphs.size() > 1U && paragraphs.back().empty()) {
            paragraphs.pop_back();
        }
    }

    return paragraphs;
}

auto initialize_xml_document(pugi::xml_document &xml_document, std::string_view xml_text)
    -> bool {
    xml_document.reset();
    return static_cast<bool>(
        xml_document.load_buffer(xml_text.data(), xml_text.size()));
}

auto initialize_empty_relationships_document(pugi::xml_document &xml_document) -> bool {
    return initialize_xml_document(xml_document, empty_relationships_xml);
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    std::error_code code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    error_info.code = code;
    error_info.detail = std::move(detail);
    error_info.entry_name = std::move(entry_name);
    error_info.xml_offset = xml_offset;
    return code;
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    featherdoc::document_errc code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    return set_last_error(error_info, featherdoc::make_error_code(code),
                          std::move(detail), std::move(entry_name), xml_offset);
}

enum class xml_uint_attribute_status {
    ok,
    missing,
    invalid,
};

auto parse_xml_uint32_attribute(pugi::xml_node node, const char *attribute_name,
                                std::uint32_t &value) -> xml_uint_attribute_status {
    const auto attribute = node.attribute(attribute_name);
    if (attribute == pugi::xml_attribute{}) {
        return xml_uint_attribute_status::missing;
    }

    const auto text = std::string_view{attribute.value()};
    if (text.empty()) {
        return xml_uint_attribute_status::invalid;
    }

    const auto *begin = text.data();
    const auto *end = begin + text.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return xml_uint_attribute_status::invalid;
    }

    return xml_uint_attribute_status::ok;
}

enum class zip_entry_read_status {
    ok,
    missing,
    read_failed,
};

auto read_zip_entry_text(zip_t *archive, std::string_view entry_name, std::string &content)
    -> zip_entry_read_status {
    if (zip_entry_open(archive, entry_name.data()) != 0) {
        return zip_entry_read_status::missing;
    }

    void *buffer = nullptr;
    size_t buffer_size = 0;
    const auto read_result = zip_entry_read(archive, &buffer, &buffer_size);
    const auto close_result = zip_entry_close(archive);

    if (read_result < 0 || close_result != 0) {
        if (buffer != nullptr) {
            std::free(buffer);
        }
        return zip_entry_read_status::read_failed;
    }

    content.assign(static_cast<const char *>(buffer), buffer_size);
    std::free(buffer);
    return zip_entry_read_status::ok;
}

auto ends_with_ascii_case_insensitive(std::string_view text,
                                      std::string_view suffix) -> bool {
    if (suffix.size() > text.size()) {
        return false;
    }
    const auto offset = text.size() - suffix.size();
    for (std::size_t index = 0U; index < suffix.size(); ++index) {
        const auto left = static_cast<unsigned char>(text[offset + index]);
        const auto right = static_cast<unsigned char>(suffix[index]);
        const auto lower_left =
            left >= 'A' && left <= 'Z' ? left + ('a' - 'A') : left;
        const auto lower_right =
            right >= 'A' && right <= 'Z' ? right + ('a' - 'A') : right;
        if (lower_left != lower_right) {
            return false;
        }
    }
    return true;
}

auto is_xml_archive_entry(std::string_view entry_name) -> bool {
    return entry_name == content_types_xml_entry ||
           ends_with_ascii_case_insensitive(entry_name, ".xml") ||
           ends_with_ascii_case_insensitive(entry_name, ".rels");
}

auto validate_archive_limits(
    zip_t *archive, const featherdoc::archive_limits &limits,
    featherdoc::document_error_info &last_error_info) -> std::error_code {
    const auto entry_count = zip_entries_total(archive);
    if (entry_count < 0) {
        return set_last_error(
            last_error_info, featherdoc::document_errc::archive_limit_exceeded,
            "failed to enumerate archive entries while enforcing resource limits");
    }
    if (static_cast<std::uint64_t>(entry_count) > limits.max_entries) {
        return set_last_error(
            last_error_info, featherdoc::document_errc::archive_limit_exceeded,
            "archive contains " + std::to_string(entry_count) +
                " entries; limit is " + std::to_string(limits.max_entries));
    }

    std::uint64_t total_uncompressed = 0U;
    for (ssize_t index = 0; index < entry_count; ++index) {
        if (zip_entry_openbyindex(archive, static_cast<std::size_t>(index)) != 0) {
            return set_last_error(
                last_error_info, featherdoc::document_errc::archive_limit_exceeded,
                "failed to inspect archive entry at index " +
                    std::to_string(index));
        }

        const auto *entry_name_text = zip_entry_name(archive);
        const std::string entry_name =
            entry_name_text == nullptr ? std::string{} : std::string{entry_name_text};
        const auto uncompressed =
            static_cast<std::uint64_t>(zip_entry_size(archive));
        const auto compressed =
            static_cast<std::uint64_t>(zip_entry_comp_size(archive));
        const auto entry_limit = is_xml_archive_entry(entry_name)
                                     ? limits.max_xml_part_bytes
                                     : limits.max_binary_part_bytes;

        std::error_code limit_error;
        if (uncompressed > entry_limit) {
            limit_error = set_last_error(
                last_error_info, featherdoc::document_errc::archive_limit_exceeded,
                "archive entry is " + std::to_string(uncompressed) +
                    " bytes; limit is " + std::to_string(entry_limit),
                entry_name);
        } else if (total_uncompressed > limits.max_total_uncompressed_bytes ||
                   uncompressed > limits.max_total_uncompressed_bytes -
                                      total_uncompressed) {
            limit_error = set_last_error(
                last_error_info, featherdoc::document_errc::archive_limit_exceeded,
                "archive total uncompressed size exceeds " +
                    std::to_string(limits.max_total_uncompressed_bytes) + " bytes",
                entry_name);
        } else if (uncompressed > 0U && compressed == 0U) {
            limit_error = set_last_error(
                last_error_info, featherdoc::document_errc::archive_limit_exceeded,
                "non-empty archive entry reports a zero compressed size",
                entry_name);
        } else if (compressed > 0U &&
                   (uncompressed / compressed > limits.max_compression_ratio ||
                    (uncompressed / compressed == limits.max_compression_ratio &&
                     uncompressed % compressed != 0U))) {
            limit_error = set_last_error(
                last_error_info, featherdoc::document_errc::archive_limit_exceeded,
                "archive entry compression ratio exceeds " +
                    std::to_string(limits.max_compression_ratio),
                entry_name);
        }

        if (zip_entry_close(archive) != 0 && !limit_error) {
            limit_error = set_last_error(
                last_error_info, featherdoc::document_errc::archive_limit_exceeded,
                "failed to finish inspecting archive entry", entry_name);
        }
        if (limit_error) {
            return limit_error;
        }
        total_uncompressed += uncompressed;
    }
    return {};
}

auto normalized_package_target(std::string target) -> std::string {
    std::replace(target.begin(), target.end(), '\\', '/');
    while (!target.empty() && target.front() == '/') {
        target.erase(target.begin());
    }
    return std::filesystem::path{target}.lexically_normal().generic_string();
}

auto inspect_root_relationships(zip_t *archive,
                                pugi::xml_document &relationships)
    -> std::optional<featherdoc::package_diagnostic> {
    std::string relationships_text;
    const auto status =
        read_zip_entry_text(archive, relationships_xml_entry, relationships_text);
    if (status != zip_entry_read_status::ok) {
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::missing_root_relationships,
            status == zip_entry_read_status::missing
                ? featherdoc::package_diagnostic_severity::warning
                : featherdoc::package_diagnostic_severity::error,
            std::string{relationships_xml_entry},
            status == zip_entry_read_status::missing
                ? "required OPC root relationships part is missing"
                : "required OPC root relationships part is unreadable",
            status == zip_entry_read_status::missing};
    }

    const auto parse_result = relationships.load_buffer(
        relationships_text.data(), relationships_text.size());
    if (!parse_result) {
        relationships.reset();
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::malformed_root_relationships,
            featherdoc::package_diagnostic_severity::error,
            std::string{relationships_xml_entry}, parse_result.description(), false};
    }

    const auto root = relationships.child("Relationships");
    if (root == pugi::xml_node{} ||
        std::string_view{root.attribute("xmlns").value()} !=
            package_relationships_namespace) {
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::invalid_root_relationships_root,
            featherdoc::package_diagnostic_severity::error,
            std::string{relationships_xml_entry},
            "root relationships part does not contain a valid Relationships root",
            false};
    }

    pugi::xml_node main_relationship;
    std::size_t main_relationship_count = 0U;
    for (auto relationship = root.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Type").value()} ==
            office_document_relationship_type) {
            main_relationship = relationship;
            ++main_relationship_count;
        }
    }

    if (main_relationship_count == 0U) {
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::missing_main_document_relationship,
            featherdoc::package_diagnostic_severity::warning,
            std::string{relationships_xml_entry},
            "OPC root relationships do not declare the main document", true};
    }
    if (main_relationship_count > 1U) {
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::ambiguous_main_document_relationship,
            featherdoc::package_diagnostic_severity::error,
            std::string{relationships_xml_entry},
            "OPC root relationships declare multiple main documents", false};
    }

    const auto target = normalized_package_target(
        std::string{main_relationship.attribute("Target").value()});
    const bool external =
        std::string_view{main_relationship.attribute("TargetMode").value()} ==
        "External";
    if (external || target != document_xml_entry) {
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::invalid_main_document_relationship,
            external ? featherdoc::package_diagnostic_severity::error
                     : featherdoc::package_diagnostic_severity::warning,
            std::string{relationships_xml_entry},
            external
                ? "main document relationship must not be external"
                : "main document relationship does not target word/document.xml",
            !external};
    }
    return std::nullopt;
}

auto inspect_main_content_type(const pugi::xml_document &content_types)
    -> std::optional<featherdoc::package_diagnostic> {
    const auto types = content_types.child("Types");
    if (types == pugi::xml_node{} ||
        std::string_view{types.attribute("xmlns").value()} !=
            package_content_types_namespace) {
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::invalid_content_types_root,
            featherdoc::package_diagnostic_severity::error,
            std::string{content_types_xml_entry},
            "[Content_Types].xml does not contain a valid Types root", false};
    }

    pugi::xml_node main_override;
    std::size_t main_override_count = 0U;
    for (auto override_node = types.child("Override");
         override_node != pugi::xml_node{};
         override_node = override_node.next_sibling("Override")) {
        if (std::string_view{override_node.attribute("PartName").value()} ==
            "/word/document.xml") {
            main_override = override_node;
            ++main_override_count;
        }
    }

    if (main_override_count == 0U) {
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::missing_main_document_content_type,
            featherdoc::package_diagnostic_severity::warning,
            std::string{content_types_xml_entry},
            "[Content_Types].xml does not declare the main document part", true};
    }
    if (main_override_count > 1U) {
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::ambiguous_main_document_content_type,
            featherdoc::package_diagnostic_severity::error,
            std::string{content_types_xml_entry},
            "[Content_Types].xml contains duplicate main document overrides", false};
    }
    if (std::string_view{main_override.attribute("ContentType").value()} !=
        main_document_content_type) {
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::invalid_main_document_content_type,
            featherdoc::package_diagnostic_severity::warning,
            std::string{content_types_xml_entry},
            "main document override has an invalid content type", true};
    }
    return std::nullopt;
}

auto normalize_word_part_entry(std::string_view target) -> std::string {
    std::string normalized_target{target};
    std::replace(normalized_target.begin(), normalized_target.end(), '\\', '/');

    if (!normalized_target.empty() && normalized_target.front() == '/') {
        return std::filesystem::path{normalized_target.substr(1)}
            .lexically_normal()
            .generic_string();
    }

    return (std::filesystem::path{"word"} / std::filesystem::path{normalized_target})
        .lexically_normal()
        .generic_string();
}

auto make_override_part_name(std::string_view entry_name) -> std::string {
    if (entry_name.empty()) {
        return {};
    }

    if (entry_name.front() == '/') {
        return std::string{entry_name};
    }

    return "/" + std::string{entry_name};
}

auto make_document_relationship_target(std::string_view entry_name) -> std::string {
    const auto normalized_entry = std::filesystem::path{std::string{entry_name}}
                                      .lexically_normal();
    const auto relative_target =
        normalized_entry.lexically_relative(std::filesystem::path{"word"});
    if (!relative_target.empty()) {
        return relative_target.generic_string();
    }

    return normalized_entry.filename().generic_string();
}

auto make_part_relationships_entry(std::string_view entry_name) -> std::string {
    if (entry_name.empty()) {
        return {};
    }

    const auto normalized_entry =
        std::filesystem::path{std::string{entry_name}}.lexically_normal();
    const auto filename = normalized_entry.filename().generic_string();
    if (filename.empty()) {
        return {};
    }

    return (normalized_entry.parent_path() / "_rels" /
            std::filesystem::path{filename + ".rels"})
        .lexically_normal()
        .generic_string();
}

auto find_document_relationship_by_type(pugi::xml_node relationships,
                                        std::string_view relationship_type)
    -> pugi::xml_node {
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Type").value()} == relationship_type) {
            return relationship;
        }
    }

    return {};
}

auto load_document_relationships_part(
    zip_t *archive, pugi::xml_document &relationships_xml_document,
    bool &has_relationships_part,
    featherdoc::document_error_info &last_error_info)
    -> std::optional<std::vector<related_part_entry>> {
    has_relationships_part = false;
    relationships_xml_document.reset();

    std::string relationships_xml_text;
    const auto relationships_status =
        read_zip_entry_text(archive, document_relationships_xml_entry, relationships_xml_text);
    if (relationships_status == zip_entry_read_status::missing) {
        return std::vector<related_part_entry>{};
    }

    if (relationships_status == zip_entry_read_status::read_failed) {
        set_last_error(last_error_info, featherdoc::document_errc::relationships_xml_read_failed,
                       "failed to read relationships entry 'word/_rels/document.xml.rels'",
                       std::string{document_relationships_xml_entry});
        return std::nullopt;
    }

    const auto parse_result = relationships_xml_document.load_buffer(
        relationships_xml_text.data(), relationships_xml_text.size());
    if (!parse_result) {
        set_last_error(
            last_error_info, featherdoc::document_errc::relationships_xml_parse_failed,
            parse_result.description(), std::string{document_relationships_xml_entry},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
        return std::nullopt;
    }

    has_relationships_part = true;
    std::unordered_set<std::string> seen_entries;
    std::vector<related_part_entry> related_parts;
    const auto relationships = relationships_xml_document.child("Relationships");
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{}; relationship = relationship.next_sibling("Relationship")) {
        const auto type = std::string_view{relationship.attribute("Type").value()};
        if (type != header_relationship_type && type != footer_relationship_type) {
            continue;
        }

        const auto target = std::string_view{relationship.attribute("Target").value()};
        if (target.empty()) {
            continue;
        }

        std::string entry_name = normalize_word_part_entry(target);
        if (!entry_name.empty() && seen_entries.insert(entry_name).second) {
            related_parts.push_back(related_part_entry{
                relationship.attribute("Id").value(), std::string{type},
                std::move(entry_name)});
        }
    }

    return related_parts;
}

auto load_related_xml_part(zip_t *archive, std::string_view entry_name,
                           pugi::xml_document &xml_document,
                           featherdoc::document_error_info &last_error_info)
    -> std::error_code {
    std::string xml_text;
    const auto read_status = read_zip_entry_text(archive, entry_name, xml_text);
    if (read_status == zip_entry_read_status::missing) {
        return set_last_error(last_error_info, featherdoc::document_errc::related_part_open_failed,
                              "failed to open related document part '" +
                                  std::string{entry_name} + "'",
                              std::string{entry_name});
    }

    if (read_status == zip_entry_read_status::read_failed) {
        return set_last_error(last_error_info, featherdoc::document_errc::related_part_read_failed,
                              "failed to read related document part '" +
                                  std::string{entry_name} + "'",
                              std::string{entry_name});
    }

    const auto parse_result = xml_document.load_buffer(xml_text.data(), xml_text.size());
    if (!parse_result) {
        return set_last_error(
            last_error_info, featherdoc::document_errc::related_part_parse_failed,
            parse_result.description(), std::string{entry_name},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    return {};
}

auto load_optional_relationships_part(
    zip_t *archive, std::string_view entry_name, pugi::xml_document &xml_document,
    bool &has_relationships_part,
    featherdoc::document_error_info &last_error_info) -> std::error_code {
    has_relationships_part = false;
    xml_document.reset();

    if (entry_name.empty()) {
        return {};
    }

    std::string xml_text;
    const auto read_status = read_zip_entry_text(archive, entry_name, xml_text);
    if (read_status == zip_entry_read_status::missing) {
        return {};
    }

    if (read_status == zip_entry_read_status::read_failed) {
        return set_last_error(last_error_info,
                              featherdoc::document_errc::relationships_xml_read_failed,
                              "failed to read relationships entry '" +
                                  std::string{entry_name} + "'",
                              std::string{entry_name});
    }

    const auto parse_result = xml_document.load_buffer(xml_text.data(), xml_text.size());
    if (!parse_result) {
        return set_last_error(
            last_error_info, featherdoc::document_errc::relationships_xml_parse_failed,
            parse_result.description(), std::string{entry_name},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    has_relationships_part = true;
    return {};
}

} // namespace

namespace featherdoc {

using detail::append_section_reference;
using detail::clear_section_header_footer_references;
using detail::collect_section_snapshots;
using detail::document_has_reference_type;
using detail::ensure_on_off_node_enabled;
using detail::ensure_section_property_node;
using detail::ensure_section_title_page_node;
using detail::ensure_xml_uint32_attribute;
using detail::find_section_reference;
using detail::read_on_off_value;
using detail::rebuild_body_from_section_snapshots;
using detail::remove_empty_node;
using detail::remove_empty_paragraph;
using detail::replace_section_properties_contents;
using detail::section_break_paragraph_for;
using detail::section_has_reference_type;

#include "document_settings_methods.inc"
Paragraph &Document::ensure_header_paragraphs() {
    return this->ensure_related_part_paragraphs(
        this->header_parts, "w:hdr", "w:headerReference",
        header_relationship_type.data(), header_content_type.data());
}

Paragraph &Document::ensure_footer_paragraphs() {
    return this->ensure_related_part_paragraphs(
        this->footer_parts, "w:ftr", "w:footerReference",
        footer_relationship_type.data(), footer_content_type.data());
}

Paragraph &Document::paragraphs() {
    this->paragraph.set_parent(
        this->tracked_node(document.child("w:document").child("w:body")));
    return this->paragraph;
}

Table &Document::tables() {
    this->table.set_owner(this);
    this->table.set_parent(
        this->tracked_node(document.child("w:document").child("w:body")));
    return this->table;
}

Table Document::append_table(std::size_t row_count, std::size_t column_count) {
    const auto body = document.child("w:document").child("w:body");
    const auto table_node = detail::append_table_node(body);
    auto created_table = Table(this->tracked_node(body), table_node);
    created_table.set_owner(this);

    for (std::size_t row_index = 0; row_index < row_count; ++row_index) {
        created_table.append_row(column_count);
    }

    return created_table;
}

#include "document_section_methods.inc"

#include "document_lifecycle_methods.inc"

} // namespace featherdoc
