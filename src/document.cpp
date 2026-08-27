#include "document_archive_limit_helpers.hpp"
#include "document_extension_state.hpp"
#include "document_section_xml_helpers.hpp"
#include "featherdoc.hpp"
#include "package_path_helpers.hpp"
#include "package_relationships_mce_helpers.hpp"
#include "singleton_part_attachment_helpers.hpp"
#include "table_xml_helpers.hpp"
#include "wordprocessingml_namespace_helpers.hpp"
#include "xml_document_clone_helpers.hpp"
#include "xml_document_initialization_helpers.hpp"
#include "xml_helpers.hpp"
#include "xml_parse_error_helpers.hpp"

#include <featherdoc/detail/path.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <memory>
#include <new>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
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
#include <sys/stat.h>
#include <unistd.h>
#endif

#if defined(ZIP_ENABLE_TEST_FAILURES)
namespace {
std::atomic<int> document_test_sync_failure_stage{0};
std::atomic<std::uint32_t> document_test_reserved_temp_mode{0U};
std::atomic<std::uint64_t> document_test_forced_temp_timestamp{0U};
std::atomic<std::uint64_t> document_test_forced_temp_sequence_start{0U};
std::atomic<bool> document_test_has_forced_temp_reservation{false};
} // namespace

extern "C" void document_test_fail_next_sync_stage(int stage) {
    if (stage != 1 && stage != 2) {
        stage = 0;
    }
    document_test_sync_failure_stage.store(stage, std::memory_order_release);
}

extern "C" std::uint32_t document_test_last_reserved_temp_mode() {
    return document_test_reserved_temp_mode.load(std::memory_order_acquire);
}

extern "C" void document_test_force_next_temp_reservation(
    std::uint64_t timestamp, std::uint64_t sequence_start) {
    document_test_forced_temp_timestamp.store(timestamp,
                                              std::memory_order_release);
    document_test_forced_temp_sequence_start.store(sequence_start,
                                                   std::memory_order_release);
    document_test_has_forced_temp_reservation.store(
        timestamp != 0U, std::memory_order_release);
}
#endif

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};
constexpr auto document_relationships_xml_entry =
    std::string_view{"word/_rels/document.xml.rels"};
constexpr auto relationships_xml_entry = std::string_view{"_rels/.rels"};
constexpr auto content_types_xml_entry =
    std::string_view{"[Content_Types].xml"};
constexpr auto main_document_content_type = std::string_view{
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"};
constexpr auto office_document_relationship_type =
    std::string_view{"http://schemas.openxmlformats.org/officeDocument/2006/"
                     "relationships/officeDocument"};
constexpr auto office_document_relationships_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships"};
constexpr auto header_relationship_type =
    std::string_view{"http://schemas.openxmlformats.org/officeDocument/2006/"
                     "relationships/header"};
constexpr auto footer_relationship_type =
    std::string_view{"http://schemas.openxmlformats.org/officeDocument/2006/"
                     "relationships/footer"};
constexpr auto settings_relationship_type =
    std::string_view{"http://schemas.openxmlformats.org/officeDocument/2006/"
                     "relationships/settings"};
constexpr auto numbering_relationship_type =
    std::string_view{"http://schemas.openxmlformats.org/officeDocument/2006/"
                     "relationships/numbering"};
constexpr auto styles_relationship_type =
    std::string_view{"http://schemas.openxmlformats.org/officeDocument/2006/"
                     "relationships/styles"};
constexpr auto footnotes_relationship_type =
    std::string_view{"http://schemas.openxmlformats.org/officeDocument/2006/"
                     "relationships/footnotes"};
constexpr auto endnotes_relationship_type =
    std::string_view{"http://schemas.openxmlformats.org/officeDocument/2006/"
                     "relationships/endnotes"};
constexpr auto comments_relationship_type =
    std::string_view{"http://schemas.openxmlformats.org/officeDocument/2006/"
                     "relationships/comments"};
constexpr auto comments_extended_relationship_type = std::string_view{
    "http://schemas.microsoft.com/office/2011/relationships/commentsExtended"};
constexpr auto header_content_type = std::string_view{
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.header+xml"};
constexpr auto footer_content_type = std::string_view{
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"};
constexpr auto settings_content_type = std::string_view{
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"};
constexpr auto footnotes_content_type = std::string_view{
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.footnotes+xml"};
constexpr auto endnotes_content_type = std::string_view{
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.endnotes+xml"};
constexpr auto comments_content_type = std::string_view{
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.comments+xml"};
constexpr auto comments_extended_content_type = std::string_view{
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.commentsExtended+xml"};
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
    std::vector<std::string> relationship_ids;
    std::string relationship_type;
    std::string entry_name;
};

struct xml_zip_writer final : pugi::xml_writer {
    zip_t *archive{nullptr};
    std::uint64_t max_bytes{0U};
    std::uint64_t bytes_written{0U};
    std::uint64_t attempted_bytes{0U};
    bool failed{false};
    bool limit_exceeded{false};

    xml_zip_writer(zip_t *archive_handle, std::uint64_t byte_limit)
        : archive(archive_handle), max_bytes(byte_limit) {}

    void write(const void *data, size_t size) override {
        if (this->failed || size == 0) {
            return;
        }

        const auto chunk_size = static_cast<std::uint64_t>(size);
        this->attempted_bytes =
            this->bytes_written >
                    (std::numeric_limits<std::uint64_t>::max)() - chunk_size
                ? (std::numeric_limits<std::uint64_t>::max)()
                : this->bytes_written + chunk_size;
        if (chunk_size > this->max_bytes ||
            this->bytes_written > this->max_bytes - chunk_size) {
            this->limit_exceeded = true;
            this->failed = true;
            return;
        }

        if (zip_entry_write(this->archive, data, size) < 0) {
            this->failed = true;
            return;
        }
        this->bytes_written += chunk_size;
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

auto consume_document_test_sync_failure(int stage) noexcept -> bool {
#if defined(ZIP_ENABLE_TEST_FAILURES)
    int expected = stage;
    return document_test_sync_failure_stage.compare_exchange_strong(
        expected, 0, std::memory_order_acq_rel);
#else
    (void)stage;
    return false;
#endif
}

#ifdef _WIN32
class native_path_handle final {
public:
    explicit native_path_handle(HANDLE value) noexcept : value_(value) {}
    native_path_handle(const native_path_handle &) = delete;
    auto operator=(const native_path_handle &) -> native_path_handle & = delete;

    ~native_path_handle() noexcept {
        if (this->value_ != INVALID_HANDLE_VALUE) {
            (void)CloseHandle(this->value_);
        }
    }

    [[nodiscard]] auto get() const noexcept -> HANDLE { return this->value_; }

private:
    HANDLE value_{INVALID_HANDLE_VALUE};
};

auto resolve_existing_directory_entry_path(
    const std::filesystem::path &path, bool follow_final_reparse_point,
    std::wstring &resolved_path) -> bool {
    auto open_flags = static_cast<DWORD>(FILE_FLAG_BACKUP_SEMANTICS);
    if (!follow_final_reparse_point) {
        open_flags |= FILE_FLAG_OPEN_REPARSE_POINT;
    }
    native_path_handle handle{CreateFileW(
        path.c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, open_flags, nullptr)};
    if (handle.get() == INVALID_HANDLE_VALUE) {
        return false;
    }

    constexpr auto final_path_flags =
        static_cast<DWORD>(FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    const auto required_size =
        GetFinalPathNameByHandleW(handle.get(), nullptr, 0U, final_path_flags);
    if (required_size == 0U) {
        return false;
    }

    auto buffer = std::wstring(static_cast<std::size_t>(required_size), L'\0');
    auto copied_size = GetFinalPathNameByHandleW(
        handle.get(), buffer.data(), static_cast<DWORD>(buffer.size()),
        final_path_flags);
    if (copied_size == 0U) {
        return false;
    }
    if (copied_size >= buffer.size()) {
        buffer.resize(static_cast<std::size_t>(copied_size) + 1U);
        copied_size = GetFinalPathNameByHandleW(
            handle.get(), buffer.data(), static_cast<DWORD>(buffer.size()),
            final_path_flags);
        if (copied_size == 0U || copied_size >= buffer.size()) {
            return false;
        }
    }

    buffer.resize(static_cast<std::size_t>(copied_size));
    resolved_path = std::move(buffer);
    return true;
}
#endif

auto output_path_replaces_source_archive(
    const std::filesystem::path &output_path,
    const std::filesystem::path &source_path) -> bool {
    std::error_code output_error;
    std::error_code source_error;
    const auto normalized_output =
        std::filesystem::absolute(output_path, output_error).lexically_normal();
    const auto normalized_source =
        std::filesystem::absolute(source_path, source_error).lexically_normal();
    if (output_error || source_error) {
        return output_path == source_path;
    }
    if (normalized_output == normalized_source) {
        return true;
    }

#ifdef _WIN32
    // Atomic replacement publishes to the output directory entry itself; it
    // never follows the output's final reparse point. The source path is
    // different: a caller may have opened through a symlink and later replace
    // its target directly, so compare against both the source entry and the
    // fully resolved source target. Exact resolved names preserve per-directory
    // case sensitivity, expand 8.3/Win32 aliases, and keep hard-link entries
    // distinct.
    auto output_entry = std::wstring{};
    auto source_entry = std::wstring{};
    auto source_target = std::wstring{};
    if (!resolve_existing_directory_entry_path(normalized_output, false,
                                               output_entry) ||
        !resolve_existing_directory_entry_path(normalized_source, false,
                                               source_entry)) {
        return false;
    }
    if (output_entry == source_entry) {
        return true;
    }
    return resolve_existing_directory_entry_path(normalized_source, true,
                                                 source_target) &&
           output_entry == source_target;
#else
    // rename() replaces the output directory entry without following its final
    // symlink. Resolve only the output parent, then compare that entry with the
    // source entry and the source's fully resolved target. This preserves
    // symlinked-parent aliases without conflating a different output symlink or
    // a distinct hard-link name with the opened source path.
    auto resolve_parent_preserving_leaf = [](const std::filesystem::path &path,
                                             std::error_code &error) {
        const auto resolved_parent =
            std::filesystem::weakly_canonical(path.parent_path(), error);
        return error ? std::filesystem::path{}
                     : (resolved_parent / path.filename()).lexically_normal();
    };

    const auto output_entry =
        resolve_parent_preserving_leaf(normalized_output, output_error);
    const auto source_entry =
        resolve_parent_preserving_leaf(normalized_source, source_error);
    if (output_error || source_error) {
        return false;
    }
    if (output_entry == source_entry) {
        return true;
    }

    const auto source_target =
        std::filesystem::weakly_canonical(normalized_source, source_error);
    return !source_error && output_entry == source_target;
#endif
}

class temporary_output_transaction final {
public:
    temporary_output_transaction() noexcept = default;
    temporary_output_transaction(const temporary_output_transaction &) = delete;
    auto operator=(const temporary_output_transaction &)
        -> temporary_output_transaction & = delete;
    temporary_output_transaction(temporary_output_transaction &&) = delete;
    auto operator=(temporary_output_transaction &&)
        -> temporary_output_transaction & = delete;

    ~temporary_output_transaction() noexcept {
        (void)this->close_archive();
        (void)this->close_stream();
        if (!this->cleanup_armed_ || this->path_.empty()) {
            return;
        }
#ifdef _WIN32
        (void)DeleteFileW(this->path_.c_str());
#else
        (void)::unlink(this->path_.c_str());
#endif
    }

    [[nodiscard]] auto path_slot() noexcept -> std::filesystem::path & {
        return this->path_;
    }

    [[nodiscard]] auto stream_slot() noexcept -> std::FILE *& {
        return this->stream_;
    }

    void arm_reserved_file_cleanup() noexcept {
        this->cleanup_armed_ = this->stream_ != nullptr && !this->path_.empty();
    }

    void attach_archive(zip_t *archive) noexcept { this->archive_ = archive; }

    [[nodiscard]] auto close_archive() noexcept -> int {
        auto *archive = std::exchange(this->archive_, nullptr);
        return archive == nullptr ? 0 : zip_close_ex(archive);
    }

    [[nodiscard]] auto close_stream() noexcept -> int {
        auto *stream = std::exchange(this->stream_, nullptr);
        return stream == nullptr ? 0 : std::fclose(stream);
    }

    void commit() noexcept { this->cleanup_armed_ = false; }

private:
    std::filesystem::path path_;
    std::FILE *stream_{nullptr};
    zip_t *archive_{nullptr};
    bool cleanup_armed_{false};
};

auto reserve_unique_temp_file(const std::filesystem::path &output_file,
                              std::filesystem::path &temp_file,
                              std::FILE *&temp_stream,
                              std::uint32_t &new_file_permissions)
    -> std::error_code {
    static std::atomic<std::uint64_t> sequence{0U};
    auto timestamp = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
#ifdef _WIN32
    const auto process_id = static_cast<std::uint64_t>(GetCurrentProcessId());
#else
    const auto process_id = static_cast<std::uint64_t>(getpid());
#endif
    temp_stream = nullptr;
    temp_file.clear();
    new_file_permissions = 0U;
#if defined(ZIP_ENABLE_TEST_FAILURES)
    document_test_reserved_temp_mode.store(0U, std::memory_order_release);
#endif
    std::uint64_t forced_sequence_start = 0U;
    bool use_forced_temp_reservation = false;
#if defined(ZIP_ENABLE_TEST_FAILURES)
    use_forced_temp_reservation =
        document_test_has_forced_temp_reservation.exchange(
            false, std::memory_order_acq_rel);
    if (use_forced_temp_reservation) {
        timestamp = document_test_forced_temp_timestamp.load(
            std::memory_order_acquire);
        forced_sequence_start =
            document_test_forced_temp_sequence_start.load(
                std::memory_order_acquire);
    }
#endif

    for (std::uint32_t attempt = 0; attempt < 64U; ++attempt) {
        // Keep the temporary basename independent of the destination basename.
        // Appending the transaction suffix to a long DOCX filename can exceed
        // the Windows legacy path limit even when the destination itself is
        // valid. The file remains beside the destination, so the final rename
        // is still an atomic same-filesystem replacement.
        const auto sequence_value =
            use_forced_temp_reservation
                ? forced_sequence_start + static_cast<std::uint64_t>(attempt)
                : sequence.fetch_add(1U);
        const auto temp_name = ".featherdoc-" + std::to_string(process_id) +
                               "-" + std::to_string(timestamp) + "-" +
                               std::to_string(sequence_value) + ".tmp";
        const auto candidate_temp_file = output_file.parent_path() / temp_name;

#ifdef _WIN32
        temp_file = candidate_temp_file;
        int descriptor = -1;
        const auto open_error =
            _wsopen_s(&descriptor, candidate_temp_file.c_str(),
                      _O_CREAT | _O_EXCL | _O_RDWR | _O_BINARY | _O_NOINHERIT,
                      _SH_DENYRW, _S_IREAD | _S_IWRITE);
        if (open_error == 0) {
            temp_stream = _wfdopen(descriptor, L"w+b");
            if (temp_stream != nullptr) {
                return {};
            }
            const auto stream_error = errno;
            _close(descriptor);
            std::error_code cleanup_error;
            std::filesystem::remove(candidate_temp_file, cleanup_error);
            temp_stream = nullptr;
            temp_file.clear();
            return {stream_error, std::generic_category()};
        }
        if (open_error != EEXIST) {
            temp_file.clear();
            return {open_error, std::generic_category()};
        }
        temp_file.clear();
#else
        int open_flags = O_CREAT | O_EXCL | O_RDWR;
#ifdef O_CLOEXEC
        open_flags |= O_CLOEXEC;
#endif
        // Derive the process-umask result from an empty probe inode. The probe
        // is unlinked before the real transaction file is created, so a
        // process that opens the probe can never observe document bytes.
        const int probe_descriptor =
            ::open(candidate_temp_file.c_str(), open_flags, 0666);
        if (probe_descriptor < 0) {
            if (errno == EEXIST) {
                continue;
            }
            temp_file.clear();
            return {errno, std::generic_category()};
        }

        int probe_error = 0;
        struct stat probe_status{};
        if (::fstat(probe_descriptor, &probe_status) != 0) {
            probe_error = errno;
        } else {
            new_file_permissions =
                static_cast<std::uint32_t>(probe_status.st_mode & 07777);
        }
        if (::close(probe_descriptor) != 0 && probe_error == 0) {
            probe_error = errno;
        }
        if (::unlink(candidate_temp_file.c_str()) != 0 && probe_error == 0) {
            probe_error = errno;
        }
        if (probe_error != 0) {
            temp_file.clear();
            return {probe_error, std::generic_category()};
        }

        temp_file = candidate_temp_file;
        const int descriptor =
            ::open(candidate_temp_file.c_str(), open_flags, 0600);
        if (descriptor < 0) {
            if (errno == EEXIST) {
                temp_file.clear();
                continue;
            }
            temp_file.clear();
            return {errno, std::generic_category()};
        }
        if (::fchmod(descriptor, 0600) != 0) {
            const auto permission_error = errno;
            ::close(descriptor);
            std::error_code cleanup_error;
            std::filesystem::remove(candidate_temp_file, cleanup_error);
            temp_file.clear();
            return {permission_error, std::generic_category()};
        }
#if defined(ZIP_ENABLE_TEST_FAILURES)
        struct stat transaction_status{};
        if (::fstat(descriptor, &transaction_status) != 0) {
            const auto status_error = errno;
            ::close(descriptor);
            std::error_code cleanup_error;
            std::filesystem::remove(candidate_temp_file, cleanup_error);
            temp_file.clear();
            return {status_error, std::generic_category()};
        }
        document_test_reserved_temp_mode.store(
            static_cast<std::uint32_t>(transaction_status.st_mode & 07777),
            std::memory_order_release);
#endif
        temp_stream = ::fdopen(descriptor, "w+b");
        if (temp_stream != nullptr) {
            return {};
        }
        const auto stream_error = errno;
        ::close(descriptor);
        std::error_code cleanup_error;
        std::filesystem::remove(candidate_temp_file, cleanup_error);
        temp_file.clear();
        return {stream_error, std::generic_category()};
#endif
    }

    temp_file.clear();
    temp_stream = nullptr;
    return std::make_error_code(std::errc::file_exists);
}

auto sync_temporary_output(std::FILE *temp_stream,
                           const std::filesystem::path &output_file,
                           std::uint32_t new_file_permissions)
    -> std::error_code {
    if (std::fflush(temp_stream) != 0) {
        return {errno, std::generic_category()};
    }

#ifdef _WIN32
    (void)output_file;
    (void)new_file_permissions;
    if (consume_document_test_sync_failure(1)) {
        return std::make_error_code(std::errc::io_error);
    }
    if (_commit(_fileno(temp_stream)) != 0) {
        return {errno, std::generic_category()};
    }
#else
    auto final_permissions = static_cast<mode_t>(new_file_permissions);
    struct stat output_status{};
    if (::stat(output_file.c_str(), &output_status) == 0) {
        final_permissions = output_status.st_mode & 07777;
    } else if (errno != ENOENT) {
        return {errno, std::generic_category()};
    }

    const int descriptor = ::fileno(temp_stream);
    if (descriptor < 0) {
        return {errno, std::generic_category()};
    }
    if (::fchmod(descriptor, final_permissions) != 0) {
        return {errno, std::generic_category()};
    }
    if (consume_document_test_sync_failure(1)) {
        return std::make_error_code(std::errc::io_error);
    }
    if (::fsync(descriptor) != 0) {
        return {errno, std::generic_category()};
    }
#endif

    return {};
}

auto sync_output_parent_directory(
    const std::filesystem::path &output_parent_directory) -> std::error_code {
    if (consume_document_test_sync_failure(2)) {
        return std::make_error_code(std::errc::io_error);
    }

#ifdef _WIN32
    (void)output_parent_directory;
    return {};
#else
    int open_flags = O_RDONLY;
#ifdef O_CLOEXEC
    open_flags |= O_CLOEXEC;
#endif
#ifdef O_DIRECTORY
    open_flags |= O_DIRECTORY;
#endif
    const int directory_descriptor =
        ::open(output_parent_directory.c_str(), open_flags);
    if (directory_descriptor < 0) {
        return {errno, std::generic_category()};
    }

    if (::fsync(directory_descriptor) != 0) {
        const auto sync_error = errno;
        ::close(directory_descriptor);
        return {sync_error, std::generic_category()};
    }
    if (::close(directory_descriptor) != 0) {
        return {errno, std::generic_category()};
    }
    return {};
#endif
}

auto set_post_replace_directory_sync_error(
    featherdoc::document_error_info &error_info,
    std::string_view output_file_utf8,
    const std::error_code &directory_sync_error) noexcept -> std::error_code {
    const auto code = featherdoc::make_error_code(
        featherdoc::document_errc::output_directory_sync_failed_after_replace);
    error_info.code = code;
    error_info.detail.clear();
    error_info.entry_name.clear();
    error_info.xml_offset.reset();
    try {
        error_info.detail =
            "atomically replaced output document '" +
            std::string{output_file_utf8} +
            "', but failed to synchronize its parent directory; the new "
            "document is visible but crash durability is not confirmed: " +
            directory_sync_error.message();
    } catch (...) {
        error_info.detail.clear();
    }
    return code;
}

#ifdef _WIN32
auto replace_reparse_point_entry_atomically(
    const std::filesystem::path &temp_file,
    const std::filesystem::path &output_file) -> std::error_code {
    const auto &output_name = output_file.native();
    if (output_name.size() >
        static_cast<std::size_t>((std::numeric_limits<DWORD>::max)()) /
            sizeof(wchar_t)) {
        return {static_cast<int>(ERROR_FILENAME_EXCED_RANGE),
                std::system_category()};
    }

    native_path_handle temp_handle{CreateFileW(
        temp_file.c_str(), DELETE | SYNCHRONIZE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (temp_handle.get() == INVALID_HANDLE_VALUE) {
        return {static_cast<int>(GetLastError()), std::system_category()};
    }

    const auto output_name_bytes = output_name.size() * sizeof(wchar_t);
    constexpr auto rename_info_overhead =
        offsetof(FILE_RENAME_INFO, FileName) + sizeof(wchar_t);
    constexpr auto maximum_rename_buffer =
        static_cast<std::size_t>((std::numeric_limits<DWORD>::max)());
    if (rename_info_overhead > maximum_rename_buffer ||
        output_name_bytes > maximum_rename_buffer - rename_info_overhead) {
        return {static_cast<int>(ERROR_FILENAME_EXCED_RANGE),
                std::system_category()};
    }
    const auto buffer_size = rename_info_overhead + output_name_bytes;

    auto buffer = std::vector<std::byte>(buffer_size);
    auto *rename_info =
        reinterpret_cast<FILE_RENAME_INFO *>(buffer.data());
    constexpr DWORD file_rename_replace_if_exists = 0x00000001U;
    constexpr DWORD file_rename_posix_semantics = 0x00000002U;
    rename_info->Flags =
        file_rename_replace_if_exists | file_rename_posix_semantics;
    rename_info->RootDirectory = nullptr;
    rename_info->FileNameLength = static_cast<DWORD>(output_name_bytes);
    std::copy(output_name.begin(), output_name.end(), rename_info->FileName);
    rename_info->FileName[output_name.size()] = L'\0';

    if (SetFileInformationByHandle(
            temp_handle.get(), FileRenameInfoEx, rename_info,
            static_cast<DWORD>(buffer.size())) == 0) {
        return {static_cast<int>(GetLastError()), std::system_category()};
    }
    return {};
}
#endif

auto replace_file_atomically(const std::filesystem::path &temp_file,
                             const std::filesystem::path &output_file)
    -> std::error_code {
#ifdef _WIN32
    constexpr std::uint32_t max_replace_attempts = 16U;
    const auto is_transient_replace_error = [](DWORD error) {
        return error == ERROR_SHARING_VIOLATION ||
               error == ERROR_LOCK_VIOLATION || error == ERROR_ACCESS_DENIED ||
               error == ERROR_UNABLE_TO_MOVE_REPLACEMENT ||
               error == ERROR_UNABLE_TO_REMOVE_REPLACED;
    };

    for (std::uint32_t attempt = 0U; attempt < max_replace_attempts;
         ++attempt) {
        DWORD operation_error = ERROR_SUCCESS;
        const auto attributes = GetFileAttributesW(output_file.c_str());
        // ReplaceFileW follows a final reparse point on supported Windows
        // filesystems. Rename the temporary file by handle with POSIX replace
        // semantics for that case so publication replaces the named directory
        // entry, matching rename() and the path-identity decision used to
        // refresh the source snapshot.
        const bool output_is_reparse_point =
            attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0U;
        if (output_is_reparse_point) {
            const auto rename_error =
                replace_reparse_point_entry_atomically(temp_file, output_file);
            if (!rename_error) {
                return {};
            }
            operation_error = static_cast<DWORD>(rename_error.value());
            if (!is_transient_replace_error(operation_error) ||
                attempt + 1U == max_replace_attempts) {
                return rename_error;
            }
            Sleep(2U);
            continue;
        }
        if (attributes != INVALID_FILE_ATTRIBUTES) {
            if (ReplaceFileW(output_file.c_str(), temp_file.c_str(), nullptr,
                             REPLACEFILE_WRITE_THROUGH, nullptr,
                             nullptr) != 0) {
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
                Sleep(2U);
                continue;
            }
        } else {
            operation_error = GetLastError();
            if (operation_error != ERROR_FILE_NOT_FOUND &&
                operation_error != ERROR_PATH_NOT_FOUND) {
                return {static_cast<int>(operation_error),
                        std::system_category()};
            }
        }

        if (MoveFileExW(temp_file.c_str(), output_file.c_str(),
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) !=
            0) {
            return {};
        }
        operation_error = GetLastError();
        if (!is_transient_replace_error(operation_error) ||
            attempt + 1U == max_replace_attempts) {
            return {static_cast<int>(operation_error), std::system_category()};
        }
        Sleep(2U);
    }
    return {static_cast<int>(ERROR_RETRY), std::system_category()};
#else
    if (::rename(temp_file.c_str(), output_file.c_str()) == 0) {
        return {};
    }
    return {errno, std::generic_category()};
#endif
}

auto split_plain_text_paragraphs(std::string_view text)
    -> std::vector<std::string> {
    std::vector<std::string> paragraphs;
    std::size_t begin = 0U;
    while (begin <= text.size()) {
        const auto line_end = text.find('\n', begin);
        const auto end =
            line_end == std::string_view::npos ? text.size() : line_end;

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

auto relationships_document_allows_mutation(
    const pugi::xml_document &relationships_document,
    bool has_relationships_part, std::string_view relationships_entry_name,
    featherdoc::document_error_info &last_error_info) -> bool {
    if (featherdoc::detail::package_relationships_document_allows_mutation(
            relationships_document, has_relationships_part)) {
        return true;
    }

    set_last_error(
        last_error_info, featherdoc::document_errc::invalid_package_structure,
        std::string{relationships_entry_name} +
            " does not contain a valid Relationships root in the package "
            "relationships namespace; relationship mutations are disabled",
        std::string{relationships_entry_name});
    return false;
}

enum class xml_uint_attribute_status {
    ok,
    missing,
    invalid,
};

auto parse_xml_uint32_attribute(pugi::xml_node node, const char *attribute_name,
                                std::uint32_t &value)
    -> xml_uint_attribute_status {
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
    limit_exceeded,
    read_failed,
};

auto read_zip_entry_text(
    zip_t *archive, std::string_view entry_name, std::string &content,
    const featherdoc::archive_limits *xml_limits,
    featherdoc::document_error_info *last_error_info,
    const featherdoc::detail::archive_entry_catalog &catalog)
    -> zip_entry_read_status {
    if (featherdoc::detail::open_archive_entry_by_package_name(
            archive, catalog, entry_name) != 0) {
        return zip_entry_read_status::missing;
    }

    if (xml_limits != nullptr && last_error_info != nullptr) {
        if (const auto limit_error =
                featherdoc::detail::enforce_open_zip_entry_xml_size_limit(
                    archive, *xml_limits, entry_name, *last_error_info)) {
            zip_entry_close(archive);
            content.clear();
            return zip_entry_read_status::limit_exceeded;
        }
    }

    void *buffer = nullptr;
    size_t buffer_size = 0;
    const auto read_result = zip_entry_read(archive, &buffer, &buffer_size);
    const auto close_result = zip_entry_close(archive);
    std::unique_ptr<void, decltype(&std::free)> buffer_guard{buffer, &std::free};

    if (read_result < 0 || close_result != 0) {
        return zip_entry_read_status::read_failed;
    }
    if (buffer_size > 0U && buffer == nullptr) {
        return zip_entry_read_status::read_failed;
    }

    if (buffer_size == 0U) {
        content.clear();
    } else {
        content.assign(static_cast<const char *>(buffer), buffer_size);
    }
    return zip_entry_read_status::ok;
}

struct root_relationships_inspection_result final {
    std::optional<featherdoc::package_diagnostic> diagnostic;
    std::error_code fatal_error;

    root_relationships_inspection_result() = default;
    root_relationships_inspection_result(
        featherdoc::package_diagnostic diagnostic_value)
        : diagnostic(std::move(diagnostic_value)) {}
};

auto inspect_root_relationships(
    zip_t *archive, const featherdoc::detail::archive_entry_catalog &catalog,
    pugi::xml_document &relationships,
    featherdoc::document_error_info &last_error_info)
    -> root_relationships_inspection_result {
    std::string relationships_text;
    const auto status =
        read_zip_entry_text(archive, relationships_xml_entry,
                            relationships_text, nullptr, nullptr, catalog);
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
        relationships_text.data(), relationships_text.size(),
        featherdoc::detail::package_relationships_xml_parse_options);
    if (!parse_result) {
        relationships.reset();
        if (parse_result.status == pugi::status_out_of_memory) {
            root_relationships_inspection_result result;
            result.fatal_error =
                featherdoc::detail::set_xml_parse_allocation_failure(
                    last_error_info, relationships_xml_entry);
            return result;
        }
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::malformed_root_relationships,
            featherdoc::package_diagnostic_severity::error,
            std::string{relationships_xml_entry}, parse_result.description(),
            false};
    }

    const auto mce_result =
        featherdoc::detail::preprocess_package_relationships_mce(relationships);
    if (!mce_result) {
        root_relationships_inspection_result result;
        result.fatal_error =
            featherdoc::detail::set_package_relationships_mce_last_error(
                last_error_info, mce_result, relationships_xml_entry);
        return result;
    }

    if (featherdoc::detail::inspect_package_relationships_document(
            relationships) !=
        featherdoc::detail::package_relationships_document_state::valid) {
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::
                invalid_root_relationships_root,
            featherdoc::package_diagnostic_severity::error,
            std::string{relationships_xml_entry},
            "root relationships part does not contain a valid Relationships "
            "root",
            false};
    }
    const auto root =
        featherdoc::detail::package_relationships_root(relationships);

    pugi::xml_node main_relationship;
    std::size_t main_relationship_count = 0U;
    for (auto relationship =
             featherdoc::detail::first_package_relationship(root);
         relationship != pugi::xml_node{};
         relationship =
             featherdoc::detail::next_package_relationship(relationship)) {
        if (std::string_view{relationship.attribute("Type").value()} ==
            office_document_relationship_type) {
            main_relationship = relationship;
            ++main_relationship_count;
        }
    }

    if (main_relationship_count == 0U) {
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::
                missing_main_document_relationship,
            featherdoc::package_diagnostic_severity::warning,
            std::string{relationships_xml_entry},
            "OPC root relationships do not declare the main document", true};
    }
    if (main_relationship_count > 1U) {
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::
                ambiguous_main_document_relationship,
            featherdoc::package_diagnostic_severity::error,
            std::string{relationships_xml_entry},
            "OPC root relationships declare multiple main documents", false};
    }

    const auto target =
        featherdoc::detail::resolve_internal_package_relationship_target(
            {}, main_relationship.attribute("Target").value());
    const bool external =
        std::string_view{main_relationship.attribute("TargetMode").value()} ==
        "External";
    const bool targets_main_document =
        target &&
        featherdoc::detail::package_part_identity(target.entry_name) ==
            featherdoc::detail::package_part_identity(document_xml_entry);
    if (external || !targets_main_document) {
        const auto invalid_target_detail =
            !target ? std::string{featherdoc::detail::
                                      package_relationship_target_error_message(
                                          target.error)}
                    : std::string{"main document relationship does not target "
                                  "word/document.xml"};
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::
                invalid_main_document_relationship,
            external ? featherdoc::package_diagnostic_severity::error
                     : featherdoc::package_diagnostic_severity::warning,
            std::string{relationships_xml_entry},
            external ? "main document relationship must not be external"
                     : invalid_target_detail,
            !external};
    }
    return {};
}

auto inspect_main_content_type(const pugi::xml_document &content_types)
    -> std::optional<featherdoc::package_diagnostic> {
    const auto structure =
        featherdoc::detail::inspect_package_content_types_document_structure(
            content_types);
    if (structure.state !=
        featherdoc::detail::package_content_types_document_state::valid) {
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::invalid_content_types_root,
            featherdoc::package_diagnostic_severity::error,
            std::string{content_types_xml_entry},
            structure.detail.empty()
                ? "[Content_Types].xml does not contain a valid Types root"
                : std::string{structure.detail},
            false};
    }
    const auto types =
        featherdoc::detail::package_content_types_root(content_types);

    const auto main_part_identity =
        featherdoc::detail::content_type_part_name_identity(
            "/word/document.xml");
    std::unordered_set<std::string> default_extension_identities;
    for (auto default_node =
             featherdoc::detail::first_package_content_type_default(types);
         default_node != pugi::xml_node{};
         default_node = featherdoc::detail::next_package_content_type_default(
             default_node)) {
        const auto media_type =
            std::string_view{default_node.attribute("ContentType").value()};
        if (!featherdoc::detail::content_type_media_type_is_valid(media_type)) {
            return featherdoc::package_diagnostic{
                featherdoc::package_diagnostic_code::
                    invalid_content_type_media_type,
                featherdoc::package_diagnostic_severity::error,
                std::string{content_types_xml_entry},
                "[Content_Types].xml contains a Default with a missing or "
                "invalid ContentType media type",
                false};
        }
        const auto extension_identity =
            featherdoc::detail::content_type_extension_identity(
                default_node.attribute("Extension").value());
        if (!extension_identity.has_value()) {
            return featherdoc::package_diagnostic{
                featherdoc::package_diagnostic_code::
                    invalid_content_type_extension,
                featherdoc::package_diagnostic_severity::error,
                std::string{content_types_xml_entry},
                "[Content_Types].xml contains an invalid Default Extension",
                false};
        }
        if (!default_extension_identities.insert(*extension_identity).second) {
            return featherdoc::package_diagnostic{
                featherdoc::package_diagnostic_code::
                    duplicate_content_type_default,
                featherdoc::package_diagnostic_severity::error,
                std::string{content_types_xml_entry},
                "[Content_Types].xml contains duplicate logical Default "
                "Extension values",
                false};
        }
    }

    std::unordered_set<std::string> override_part_identities;
    pugi::xml_node main_override;
    std::size_t main_override_count = 0U;
    for (auto override_node =
             featherdoc::detail::first_package_content_type_override(types);
         override_node != pugi::xml_node{};
         override_node = featherdoc::detail::next_package_content_type_override(
             override_node)) {
        const auto media_type =
            std::string_view{override_node.attribute("ContentType").value()};
        if (!featherdoc::detail::content_type_media_type_is_valid(media_type)) {
            return featherdoc::package_diagnostic{
                featherdoc::package_diagnostic_code::
                    invalid_content_type_media_type,
                featherdoc::package_diagnostic_severity::error,
                std::string{content_types_xml_entry},
                "[Content_Types].xml contains an Override with a missing or "
                "invalid ContentType media type",
                false};
        }
        const auto part_identity =
            featherdoc::detail::content_type_part_name_identity(
                override_node.attribute("PartName").value());
        if (!part_identity.has_value()) {
            return featherdoc::package_diagnostic{
                featherdoc::package_diagnostic_code::
                    invalid_content_type_part_name,
                featherdoc::package_diagnostic_severity::error,
                std::string{content_types_xml_entry},
                "[Content_Types].xml contains an invalid Override PartName",
                false};
        }
        const bool is_main_part = main_part_identity.has_value() &&
                                  *part_identity == *main_part_identity;
        if (!override_part_identities.insert(*part_identity).second) {
            return featherdoc::package_diagnostic{
                is_main_part ? featherdoc::package_diagnostic_code::
                                   ambiguous_main_document_content_type
                             : featherdoc::package_diagnostic_code::
                                   duplicate_content_type_override,
                featherdoc::package_diagnostic_severity::error,
                std::string{content_types_xml_entry},
                is_main_part
                    ? "[Content_Types].xml contains duplicate logical main "
                      "document overrides"
                    : "[Content_Types].xml contains duplicate logical "
                      "Override PartName values",
                false};
        }
        if (is_main_part) {
            main_override = override_node;
            ++main_override_count;
        }
    }

    if (main_override_count == 0U) {
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::
                missing_main_document_content_type,
            featherdoc::package_diagnostic_severity::warning,
            std::string{content_types_xml_entry},
            "[Content_Types].xml does not declare the main document part",
            true};
    }
    if (main_override_count > 1U) {
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::
                ambiguous_main_document_content_type,
            featherdoc::package_diagnostic_severity::error,
            std::string{content_types_xml_entry},
            "[Content_Types].xml contains duplicate main document overrides",
            false};
    }
    if (featherdoc::detail::ascii_case_folded_archive_entry_name(
            main_override.attribute("ContentType").value()) !=
        featherdoc::detail::ascii_case_folded_archive_entry_name(
            main_document_content_type)) {
        return featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::
                invalid_main_document_content_type,
            featherdoc::package_diagnostic_severity::warning,
            std::string{content_types_xml_entry},
            "main document override has an invalid content type", true};
    }
    return std::nullopt;
}

auto make_override_part_name(std::string_view entry_name) -> std::string {
    return featherdoc::detail::make_package_content_type_part_name(entry_name);
}

auto make_document_relationship_target(std::string_view entry_name)
    -> std::string {
    return featherdoc::detail::make_package_relationship_target(
        document_xml_entry, entry_name);
}

auto make_part_relationships_entry(std::string_view entry_name) -> std::string {
    if (entry_name.empty()) {
        return {};
    }

    const auto normalized_entry =
        featherdoc::detail::normalize_package_path(entry_name);
    const auto filename =
        featherdoc::detail::package_path_filename(normalized_entry);
    if (filename.empty()) {
        return {};
    }

    const auto parent =
        featherdoc::detail::package_path_parent(normalized_entry);
    return featherdoc::detail::normalize_package_path(
        (parent.empty() ? std::string{} : parent + "/") + "_rels/" + filename +
        ".rels");
}

auto find_document_relationship_by_type(pugi::xml_node relationships,
                                        std::string_view relationship_type)
    -> pugi::xml_node {
    for (auto relationship =
             featherdoc::detail::first_package_relationship(relationships);
         relationship != pugi::xml_node{};
         relationship =
             featherdoc::detail::next_package_relationship(relationship)) {
        if (std::string_view{relationship.attribute("Type").value()} ==
            relationship_type) {
            return relationship;
        }
    }

    return {};
}

auto is_singleton_document_relationship_type(std::string_view type) -> bool {
    constexpr auto singleton_types = std::array{
        settings_relationship_type,
        numbering_relationship_type,
        styles_relationship_type,
        footnotes_relationship_type,
        endnotes_relationship_type,
        comments_relationship_type,
        comments_extended_relationship_type,
    };
    return std::ranges::find(singleton_types, type) != singleton_types.end();
}

auto is_related_xml_document_relationship_type(std::string_view type) -> bool {
    return type == header_relationship_type || type == footer_relationship_type;
}

auto load_document_relationships_part(
    zip_t *archive, pugi::xml_document &relationships_xml_document,
    bool &has_relationships_part,
    featherdoc::document_error_info &last_error_info,
    const featherdoc::detail::archive_entry_catalog &catalog,
    featherdoc::package_validation_mode validation_mode,
    std::vector<featherdoc::package_diagnostic> &diagnostics,
    std::unordered_set<std::string> &ignored_singleton_types)
    -> std::optional<std::vector<related_part_entry>> {
    has_relationships_part = false;
    relationships_xml_document.reset();

    std::string relationships_xml_text;
    const auto relationships_status =
        read_zip_entry_text(archive, document_relationships_xml_entry,
                            relationships_xml_text, nullptr, nullptr, catalog);
    if (relationships_status == zip_entry_read_status::missing) {
        return std::vector<related_part_entry>{};
    }

    if (relationships_status == zip_entry_read_status::read_failed) {
        set_last_error(
            last_error_info,
            featherdoc::document_errc::relationships_xml_read_failed,
            "failed to read relationships entry 'word/_rels/document.xml.rels'",
            std::string{document_relationships_xml_entry});
        return std::nullopt;
    }

    const auto parse_result = relationships_xml_document.load_buffer(
        relationships_xml_text.data(), relationships_xml_text.size(),
        featherdoc::detail::package_relationships_xml_parse_options);
    if (!parse_result) {
        featherdoc::detail::set_xml_parse_failure(
            last_error_info, parse_result,
            featherdoc::document_errc::relationships_xml_parse_failed,
            document_relationships_xml_entry);
        return std::nullopt;
    }

    const auto mce_result =
        featherdoc::detail::preprocess_package_relationships_mce(
            relationships_xml_document);
    if (!mce_result) {
        (void)featherdoc::detail::set_package_relationships_mce_last_error(
            last_error_info, mce_result, document_relationships_xml_entry);
        return std::nullopt;
    }

    has_relationships_part = true;
    const auto record_issue = [&](featherdoc::package_diagnostic_code code,
                                  std::string detail) -> bool {
        if (validation_mode == featherdoc::package_validation_mode::strict) {
            set_last_error(last_error_info,
                           featherdoc::document_errc::invalid_package_structure,
                           detail,
                           std::string{document_relationships_xml_entry});
            return false;
        }
        diagnostics.push_back(featherdoc::package_diagnostic{
            code, featherdoc::package_diagnostic_severity::warning,
            std::string{document_relationships_xml_entry}, std::move(detail),
            false});
        return true;
    };
    const auto fail_closed = [&](featherdoc::package_diagnostic_code code,
                                 std::string detail) -> bool {
        (void)code;
        set_last_error(last_error_info,
                       featherdoc::document_errc::invalid_package_structure,
                       std::move(detail),
                       std::string{document_relationships_xml_entry});
        return false;
    };

    const auto relationships_state =
        featherdoc::detail::inspect_package_relationships_document(
            relationships_xml_document, false);
    if (relationships_state !=
        featherdoc::detail::package_relationships_document_state::valid) {
        if (!record_issue(
                featherdoc::package_diagnostic_code::invalid_relationships_part,
                "word/_rels/document.xml.rels does not contain a valid "
                "Relationships structure in the package relationships "
                "namespace")) {
            return std::nullopt;
        }
        if (featherdoc::detail::
                inspect_package_relationships_document_structure(
                    relationships_xml_document) !=
            featherdoc::detail::package_relationships_document_state::valid) {
            ignored_singleton_types.insert(
                std::string{settings_relationship_type});
            ignored_singleton_types.insert(
                std::string{numbering_relationship_type});
            ignored_singleton_types.insert(
                std::string{styles_relationship_type});
            ignored_singleton_types.insert(
                std::string{footnotes_relationship_type});
            ignored_singleton_types.insert(
                std::string{endnotes_relationship_type});
            ignored_singleton_types.insert(
                std::string{comments_relationship_type});
            ignored_singleton_types.insert(
                std::string{comments_extended_relationship_type});
            return std::vector<related_part_entry>{};
        }
    }
    auto relationships = featherdoc::detail::package_relationships_root(
        relationships_xml_document);

    std::vector<pugi::xml_node> relationship_nodes;
    std::unordered_map<std::string, std::size_t> relationship_id_counts;
    std::unordered_map<std::string, std::size_t> singleton_type_counts;
    for (auto relationship =
             featherdoc::detail::first_package_relationship(relationships);
         relationship != pugi::xml_node{};
         relationship =
             featherdoc::detail::next_package_relationship(relationship)) {
        relationship_nodes.push_back(relationship);
        const auto id = std::string{relationship.attribute("Id").value()};
        if (!id.empty()) {
            ++relationship_id_counts[id];
        }
        const auto type = std::string{relationship.attribute("Type").value()};
        if (is_singleton_document_relationship_type(type)) {
            ++singleton_type_counts[type];
        }
    }

    for (const auto &[id, count] : relationship_id_counts) {
        if (count > 1U) {
            fail_closed(featherdoc::package_diagnostic_code::
                            invalid_document_relationship,
                        "document relationship Id '" + id +
                            "' is declared more than once");
            return std::nullopt;
        }
    }

    for (const auto &[type, count] : singleton_type_counts) {
        if (count > 1U) {
            fail_closed(featherdoc::package_diagnostic_code::
                            duplicate_singleton_relationship,
                        "singleton document relationship Type '" + type +
                            "' is declared more than once");
            return std::nullopt;
        }
    }

    std::unordered_map<std::string, std::size_t> related_part_by_identity;
    std::vector<related_part_entry> related_parts;
    for (const auto relationship : relationship_nodes) {
        const auto id = std::string{relationship.attribute("Id").value()};
        const auto type = std::string{relationship.attribute("Type").value()};
        std::optional<featherdoc::package_diagnostic_code> issue_code;
        std::string issue_detail;

        const auto target =
            std::string_view{relationship.attribute("Target").value()};
        const auto target_mode_attribute = relationship.attribute("TargetMode");
        const auto target_mode =
            std::string_view{target_mode_attribute.value()};
        if (id.empty()) {
            issue_code = featherdoc::package_diagnostic_code::
                invalid_document_relationship;
            issue_detail = "document relationship Id must not be empty";
        } else if (type.empty()) {
            issue_code = featherdoc::package_diagnostic_code::
                invalid_document_relationship;
            issue_detail =
                "document relationship '" + id + "' Type must not be empty";
        } else if (target.empty()) {
            issue_code = featherdoc::package_diagnostic_code::
                invalid_document_relationship;
            issue_detail =
                "document relationship '" + id + "' Target must not be empty";
        } else if (target_mode_attribute != pugi::xml_attribute{} &&
                   target_mode != "Internal" && target_mode != "External") {
            issue_code = featherdoc::package_diagnostic_code::
                invalid_document_relationship;
            issue_detail =
                "document relationship '" + id + "' has an invalid TargetMode";
        }

        const bool tracked_internal_part =
            is_singleton_document_relationship_type(type) ||
            is_related_xml_document_relationship_type(type);
        auto resolved_target =
            featherdoc::detail::package_relationship_target_resolution{};
        if (!issue_code.has_value() && tracked_internal_part) {
            if (!target_mode.empty() && target_mode != "Internal") {
                issue_code = featherdoc::package_diagnostic_code::
                    invalid_document_relationship;
                issue_detail = "internal document relationship '" + id +
                               "' has a non-Internal TargetMode";
            } else {
                resolved_target = featherdoc::detail::
                    resolve_internal_package_relationship_target(
                        document_xml_entry, target);
                if (!resolved_target) {
                    issue_code = featherdoc::package_diagnostic_code::
                        invalid_document_relationship;
                    issue_detail =
                        "document relationship '" + id +
                        "' has an invalid "
                        "Target: " +
                        std::string{
                            featherdoc::detail::
                                package_relationship_target_error_message(
                                    resolved_target.error)};
                } else {
                    const auto *target_record =
                        featherdoc::detail::find_archive_entry_record(
                            catalog, resolved_target.entry_name);
                    if (target_record == nullptr ||
                        target_record->is_directory) {
                        issue_code = featherdoc::package_diagnostic_code::
                            dangling_relationship;
                        issue_detail = "document relationship '" + id +
                                       "' targets missing package part '" +
                                       resolved_target.entry_name + "'";
                    }
                }
            }
        }

        if (issue_code.has_value()) {
            if (!record_issue(*issue_code, std::move(issue_detail))) {
                return std::nullopt;
            }
            if (is_singleton_document_relationship_type(type)) {
                ignored_singleton_types.insert(type);
            }
            continue;
        }

        if (!is_related_xml_document_relationship_type(type)) {
            continue;
        }

        const auto entry_identity = featherdoc::detail::package_part_identity(
            resolved_target.entry_name);
        const auto existing = related_part_by_identity.find(entry_identity);
        if (existing == related_part_by_identity.end()) {
            related_part_by_identity.emplace(entry_identity,
                                             related_parts.size());
            related_parts.push_back(related_part_entry{
                {id}, type, std::move(resolved_target.entry_name)});
        } else if (existing->second < related_parts.size() &&
                   related_parts[existing->second].relationship_type == type) {
            related_parts[existing->second].relationship_ids.push_back(id);
        } else {
            const auto detail =
                "header and footer relationships must not share the same "
                "logical package part target";
            fail_closed(featherdoc::package_diagnostic_code::
                            invalid_document_relationship,
                        detail);
            return std::nullopt;
        }
    }

    return related_parts;
}

auto load_related_xml_part(
    zip_t *archive, std::string_view entry_name,
    pugi::xml_document &xml_document,
    featherdoc::document_error_info &last_error_info,
    const featherdoc::archive_limits &limits,
    const featherdoc::detail::archive_entry_catalog &catalog,
    featherdoc::detail::wordprocessingml_part_kind part_kind)
    -> std::error_code {
    std::string xml_text;
    const auto read_status = read_zip_entry_text(
        archive, entry_name, xml_text, &limits, &last_error_info, catalog);
    if (read_status == zip_entry_read_status::missing) {
        return set_last_error(
            last_error_info,
            featherdoc::document_errc::related_part_open_failed,
            "failed to open related document part '" + std::string{entry_name} +
                "'",
            std::string{entry_name});
    }

    if (read_status == zip_entry_read_status::limit_exceeded) {
        return last_error_info.code;
    }

    if (read_status == zip_entry_read_status::read_failed) {
        return set_last_error(
            last_error_info,
            featherdoc::document_errc::related_part_read_failed,
            "failed to read related document part '" + std::string{entry_name} +
                "'",
            std::string{entry_name});
    }

    const auto parse_result =
        xml_document.load_buffer(xml_text.data(), xml_text.size());
    if (!parse_result) {
        return featherdoc::detail::set_xml_parse_failure(
            last_error_info, parse_result,
            featherdoc::document_errc::related_part_parse_failed, entry_name);
    }

    const auto namespace_result =
        featherdoc::detail::canonicalize_wordprocessingml_part(xml_document,
                                                               part_kind);
    if (!namespace_result) {
        return featherdoc::detail::set_wordprocessingml_namespace_last_error(
            last_error_info, namespace_result, entry_name);
    }
    if (!namespace_result.root_matches) {
        const auto error =
            featherdoc::detail::set_wordprocessingml_root_mismatch_last_error(
                last_error_info, namespace_result, part_kind, entry_name);
        xml_document.reset();
        return error;
    }

    return {};
}

auto load_optional_relationships_part(
    zip_t *archive, std::string_view entry_name,
    pugi::xml_document &xml_document, bool &has_relationships_part,
    featherdoc::document_error_info &last_error_info,
    const featherdoc::archive_limits &limits,
    const featherdoc::detail::archive_entry_catalog &catalog,
    featherdoc::package_validation_mode validation_mode,
    std::vector<featherdoc::package_diagnostic> &diagnostics)
    -> std::error_code {
    has_relationships_part = false;
    xml_document.reset();

    if (entry_name.empty()) {
        return {};
    }

    std::string xml_text;
    const auto read_status = read_zip_entry_text(
        archive, entry_name, xml_text, &limits, &last_error_info, catalog);
    if (read_status == zip_entry_read_status::missing) {
        return {};
    }

    if (read_status == zip_entry_read_status::limit_exceeded) {
        return last_error_info.code;
    }

    if (read_status == zip_entry_read_status::read_failed) {
        return set_last_error(
            last_error_info,
            featherdoc::document_errc::relationships_xml_read_failed,
            "failed to read relationships entry '" + std::string{entry_name} +
                "'",
            std::string{entry_name});
    }

    const auto parse_result = xml_document.load_buffer(
        xml_text.data(), xml_text.size(),
        featherdoc::detail::package_relationships_xml_parse_options);
    if (!parse_result) {
        return featherdoc::detail::set_xml_parse_failure(
            last_error_info, parse_result,
            featherdoc::document_errc::relationships_xml_parse_failed,
            entry_name);
    }

    const auto mce_result =
        featherdoc::detail::preprocess_package_relationships_mce(xml_document);
    if (!mce_result) {
        return featherdoc::detail::set_package_relationships_mce_last_error(
            last_error_info, mce_result, entry_name);
    }

    has_relationships_part = true;
    if (featherdoc::detail::inspect_package_relationships_document(
            xml_document) !=
        featherdoc::detail::package_relationships_document_state::valid) {
        const auto detail = std::string{entry_name} +
                            " does not contain a valid Relationships root in "
                            "the package relationships namespace";
        if (validation_mode == featherdoc::package_validation_mode::strict) {
            return set_last_error(
                last_error_info,
                featherdoc::document_errc::invalid_package_structure, detail,
                std::string{entry_name});
        }

        diagnostics.push_back(featherdoc::package_diagnostic{
            featherdoc::package_diagnostic_code::invalid_relationships_part,
            featherdoc::package_diagnostic_severity::warning,
            std::string{entry_name}, detail, false});
    }
    return {};
}

} // namespace

namespace featherdoc {

using detail::clear_section_header_footer_references;
using detail::collect_section_snapshots;
using detail::ensure_section_property_node;
using detail::ensure_xml_uint32_attribute;
using detail::find_section_reference;
using detail::publish_checked_section_document_update;
using detail::read_on_off_value;
using detail::rebuild_body_from_section_snapshots;
using detail::remove_empty_node;
using detail::remove_empty_paragraph;
using detail::replace_section_properties_contents;
using detail::section_break_paragraph_for;
using detail::section_document_publish_result;
using detail::section_has_reference_type;

void Document::ensure_xml_handle_lifetime() {
    if (this->xml_handle_lifetime == nullptr) {
        this->xml_handle_lifetime =
            std::make_shared<detail::xml_handle_lifetime>();
    }
}

detail::document_extension_state &Document::extension_state() {
    this->ensure_xml_handle_lifetime();
    auto *state = static_cast<detail::document_extension_state *>(
        this->xml_handle_lifetime->opaque_owner_state());
    if (state == nullptr) {
        auto owner_state = std::make_shared<detail::document_extension_state>();
        state = owner_state.get();
        this->xml_handle_lifetime->set_opaque_owner_state(
            std::move(owner_state));
    }
    return *state;
}

const detail::document_extension_state &Document::extension_state() const {
    return const_cast<Document *>(this)->extension_state();
}

const std::vector<std::string> &Document::related_part_relationship_alias_ids(
    const xml_part_state &part) const {
    static const auto empty_alias_ids = std::vector<std::string>{};
    const auto &alias_ids =
        this->extension_state().related_part_relationship_alias_ids;
    const auto iterator = alias_ids.find(part.entry_name);
    return iterator != alias_ids.end() ? iterator->second : empty_alias_ids;
}

bool Document::related_part_has_relationship_id(
    const xml_part_state &part, std::string_view relationship_id) const {
    if (part.relationship_id == relationship_id) {
        return true;
    }
    return std::ranges::any_of(
        this->related_part_relationship_alias_ids(part),
        [&](const auto &alias_id) { return alias_id == relationship_id; });
}

bool Document::ignored_singleton_relationship_type(
    std::string_view relationship_type) const {
    return this->extension_state()
        .ignored_singleton_relationship_types.contains(
            std::string{relationship_type});
}

bool Document::source_package_part_name_conflicts(
    std::string_view entry_name) const {
    const auto identity = detail::package_part_name_identity(entry_name);
    if (!identity.has_value()) {
        return true;
    }

    return this->extension_state().source_archive_part_identities.contains(
               *identity) ||
           detail::package_part_identity_has_derivation_conflict(
               this->extension_state().source_archive_part_identities,
               *identity);
}

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
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a table",
                       std::string{document_xml_entry});
        return {};
    }

    if (row_count == 0U || column_count == 0U) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "table row and column counts must be greater than zero",
                       std::string{document_xml_entry});
        return {};
    }

    if (column_count > detail::max_table_grid_columns) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "table column count exceeds supported limit",
                       std::string{document_xml_entry});
        return {};
    }

    auto body = document.child("w:document").child("w:body");
    const auto table_node = detail::append_table_node(body);
    if (table_node == pugi::xml_node{}) {
        (void)detail::set_xml_document_allocation_failure(
            this->last_error_info, "failed to append table to document",
            document_xml_entry);
        return {};
    }

    try {
        for (std::size_t row_index = 0; row_index < row_count; ++row_index) {
            if (detail::append_row_node(table_node, column_count) ==
                pugi::xml_node{}) {
                (void)body.remove_child(table_node);
                (void)detail::set_xml_document_allocation_failure(
                    this->last_error_info,
                    "failed to append table row to document",
                    document_xml_entry);
                return {};
            }
        }
    } catch (const std::bad_alloc &) {
        (void)body.remove_child(table_node);
        (void)detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return {};
    } catch (...) {
        (void)body.remove_child(table_node);
        throw;
    }

    auto created_table = Table(this->tracked_node(body), table_node);
    created_table.set_owner(this);
    this->last_error_info.clear();
    return created_table;
}

#include "document_section_methods.inc"

#include "document_lifecycle_methods.inc"

} // namespace featherdoc
