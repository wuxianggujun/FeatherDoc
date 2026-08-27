#pragma once

#include <featherdoc/detail/utf8.hpp>
#include <featherdoc/document_core.hpp>

#include "document_archive_fingerprint.hpp"
#include "package_path_helpers.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <zip.h>

namespace featherdoc::detail {

inline constexpr std::uint64_t max_archive_central_directory_bytes =
    64U * 1024U * 1024U;
inline constexpr std::uint64_t max_archive_entry_name_bytes = 511U;
inline constexpr std::uint64_t max_archive_total_entry_name_bytes =
    8U * 1024U * 1024U;

struct zip_reader_open_result {
    zip_t *archive{nullptr};
    int zip_error{0};
    zip_reader_limit_violation limit_violation{
        ZIP_READER_LIMIT_NONE, 0U, 0U,
        std::numeric_limits<std::uint64_t>::max()};
};

struct archive_entry_record {
    std::size_t zip_index{0U};
    std::string physical_name;
    std::string canonical_key;
    std::string identity_key;
    std::uint32_t crc32{0U};
    std::uint64_t compressed_size{0U};
    std::uint64_t uncompressed_size{0U};
    bool is_directory{false};
    bool noncanonical{false};
};

struct archive_entry_catalog {
    std::vector<archive_entry_record> records;
    std::unordered_map<std::string, std::size_t> by_identity;

    void clear() {
        this->records.clear();
        this->by_identity.clear();
    }
};

inline auto open_read_zip_archive_with_limits(
    std::string_view archive_path, const featherdoc::archive_limits &limits,
    int compression_level = ZIP_DEFAULT_COMPRESSION_LEVEL)
    -> zip_reader_open_result {
    const zip_reader_limits reader_limits{
        static_cast<std::uint64_t>(limits.max_entries),
        max_archive_central_directory_bytes, max_archive_entry_name_bytes,
        max_archive_total_entry_name_bytes};
    const std::string terminated_archive_path{archive_path};

    zip_reader_open_result result;
    result.archive = zip_openwitherror_limits(
        terminated_archive_path.c_str(), compression_level, 'r',
        &result.zip_error, &reader_limits, &result.limit_violation);
    return result;
}

class read_zip_archive_guard final {
  public:
    explicit read_zip_archive_guard(zip_t *archive) noexcept
        : archive_{archive} {}

    read_zip_archive_guard(const read_zip_archive_guard &) = delete;
    auto operator=(const read_zip_archive_guard &)
        -> read_zip_archive_guard & = delete;
    read_zip_archive_guard(read_zip_archive_guard &&) = delete;
    auto operator=(read_zip_archive_guard &&)
        -> read_zip_archive_guard & = delete;

    ~read_zip_archive_guard() noexcept {
        if (this->archive_ != nullptr) {
            (void)zip_close_ex(this->archive_);
        }
    }

    [[nodiscard]] auto get() const noexcept -> zip_t * {
        return this->archive_;
    }

    [[nodiscard]] auto release() noexcept -> zip_t * {
        return std::exchange(this->archive_, nullptr);
    }

  private:
    zip_t *archive_{nullptr};
};

inline auto set_archive_validation_error(
    featherdoc::document_error_info &last_error_info,
    featherdoc::document_errc error, std::string detail,
    std::string entry_name = {},
    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    last_error_info.code = featherdoc::make_error_code(error);
    last_error_info.detail = std::move(detail);
    last_error_info.entry_name =
        featherdoc::detail::sanitize_utf8_for_diagnostic(entry_name);
    last_error_info.xml_offset = std::move(xml_offset);
    return last_error_info.code;
}

inline auto set_archive_xml_limit_error(
    featherdoc::document_error_info &last_error_info, std::string detail,
    std::string entry_name,
    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    return set_archive_validation_error(
        last_error_info, featherdoc::document_errc::archive_limit_exceeded,
        std::move(detail), std::move(entry_name), std::move(xml_offset));
}

inline auto set_archive_reader_open_limit_error(
    featherdoc::document_error_info &last_error_info,
    std::string_view archive_path, const zip_reader_limit_violation &violation)
    -> std::error_code {
    std::string measured_resource;
    switch (violation.kind) {
    case ZIP_READER_LIMIT_ENTRIES:
        measured_resource = "entry count";
        break;
    case ZIP_READER_LIMIT_CENTRAL_DIRECTORY_BYTES:
        measured_resource = "central directory byte size";
        break;
    case ZIP_READER_LIMIT_ENTRY_NAME_BYTES:
        measured_resource = "entry name byte size at index " +
                            std::to_string(violation.entry_index);
        break;
    case ZIP_READER_LIMIT_TOTAL_ENTRY_NAME_BYTES:
        measured_resource = "total entry-name byte size";
        break;
    default:
        measured_resource = "archive reader metadata";
        break;
    }

    return set_archive_validation_error(
        last_error_info, featherdoc::document_errc::archive_limit_exceeded,
        "archive reader preflight rejected '" + std::string{archive_path} +
            "': " + measured_resource + " is " +
            std::to_string(violation.actual) + "; limit is " +
            std::to_string(violation.limit));
}

inline auto close_read_archive(zip_t *archive,
                               featherdoc::document_error_info &last_error_info,
                               std::string detail, std::string entry_name = {})
    -> std::error_code {
    if (archive == nullptr || zip_close_ex(archive) == 0) {
        return {};
    }
    return set_archive_validation_error(
        last_error_info, featherdoc::document_errc::archive_close_failed,
        std::move(detail), std::move(entry_name));
}

inline auto close_read_archive(read_zip_archive_guard &archive_guard,
                               featherdoc::document_error_info &last_error_info,
                               std::string detail, std::string entry_name = {})
    -> std::error_code {
    return close_read_archive(archive_guard.release(), last_error_info,
                              std::move(detail), std::move(entry_name));
}

inline auto archive_entry_name_ends_with_ascii_case_insensitive(
    std::string_view entry_name, std::string_view suffix) -> bool {
    if (suffix.size() > entry_name.size()) {
        return false;
    }

    const auto offset = entry_name.size() - suffix.size();
    for (std::size_t index = 0U; index < suffix.size(); ++index) {
        const auto left =
            static_cast<unsigned char>(entry_name[offset + index]);
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

inline auto archive_entry_uses_xml_size_limit(std::string_view entry_name)
    -> bool {
    return entry_name == "[Content_Types].xml" ||
           archive_entry_name_ends_with_ascii_case_insensitive(entry_name,
                                                               ".xml") ||
           archive_entry_name_ends_with_ascii_case_insensitive(entry_name,
                                                               ".rels");
}

inline auto ascii_case_folded_archive_entry_name(std::string_view entry_name)
    -> std::string {
    std::string folded_name{entry_name};
    for (auto &character : folded_name) {
        const auto byte = static_cast<unsigned char>(character);
        if (byte >= 'A' && byte <= 'Z') {
            character = static_cast<char>(byte + ('a' - 'A'));
        }
    }
    return folded_name;
}

inline auto canonical_package_part_key(std::string_view part_name,
                                       bool allow_raw_space = true)
    -> package_relationship_target_resolution {
    constexpr auto content_types_entry =
        std::string_view{"[Content_Types].xml"};
    if (ascii_case_folded_archive_entry_name(part_name) ==
        ascii_case_folded_archive_entry_name(content_types_entry)) {
        return {std::string{content_types_entry},
                package_relationship_target_error::none};
    }

    auto canonical = canonicalize_package_iri_path(part_name, allow_raw_space);
    if (!canonical) {
        return canonical;
    }
    const auto part_name_error = validate_canonical_package_part_path(
        canonical.entry_name, canonical.entry_name.ends_with('/'));
    if (part_name_error != package_relationship_target_error::none) {
        return {{}, part_name_error};
    }
    if (!canonical.entry_name.empty() && canonical.entry_name.front() == '/') {
        canonical.entry_name.erase(0U, 1U);
    }
    return canonical;
}

inline auto package_part_identity(std::string_view canonical_key)
    -> std::string {
    return ascii_case_folded_archive_entry_name(canonical_key);
}

inline auto package_part_identity_has_derivation_conflict(
    const std::set<std::string> &part_identities,
    std::string_view candidate_identity) -> bool {
    for (auto separator = candidate_identity.find('/');
         separator != std::string_view::npos;
         separator = candidate_identity.find('/', separator + 1U)) {
        if (part_identities.contains(
                std::string{candidate_identity.substr(0U, separator)})) {
            return true;
        }
    }

    std::string descendant_prefix{candidate_identity};
    descendant_prefix.push_back('/');
    const auto descendant = part_identities.lower_bound(descendant_prefix);
    return descendant != part_identities.end() &&
           descendant->starts_with(descendant_prefix);
}

inline auto package_part_name_identity(std::string_view part_name)
    -> std::optional<std::string> {
    const auto canonical = canonical_package_part_key(part_name);
    if (!canonical || canonical.entry_name.empty() ||
        canonical.entry_name.ends_with('/')) {
        return std::nullopt;
    }
    return package_part_identity(canonical.entry_name);
}

inline auto package_part_names_equivalent(std::string_view left,
                                          std::string_view right) -> bool {
    const auto left_identity = package_part_name_identity(left);
    const auto right_identity = package_part_name_identity(right);
    return left_identity.has_value() && right_identity.has_value() &&
           *left_identity == *right_identity;
}

inline auto find_archive_entry_record(const archive_entry_catalog &catalog,
                                      std::string_view package_part_name)
    -> const archive_entry_record * {
    const auto canonical = canonical_package_part_key(package_part_name);
    if (!canonical) {
        return nullptr;
    }
    const auto identity = package_part_identity(canonical.entry_name);
    const auto found = catalog.by_identity.find(identity);
    if (found == catalog.by_identity.end() ||
        found->second >= catalog.records.size()) {
        return nullptr;
    }
    return &catalog.records[found->second];
}

inline auto
open_archive_entry_by_package_name(zip_t *archive,
                                   const archive_entry_catalog &catalog,
                                   std::string_view package_part_name) -> int {
    const auto *record = find_archive_entry_record(catalog, package_part_name);
    if (record == nullptr || record->is_directory) {
        return -1;
    }
    return zip_entry_openbyindex(archive, record->zip_index);
}

inline auto is_canonical_archive_entry_name(std::string_view entry_name)
    -> bool {
    if (entry_name.empty() || entry_name.front() == '/' ||
        entry_name.find('\\') != std::string_view::npos) {
        return false;
    }

    constexpr auto content_types_entry =
        std::string_view{"[Content_Types].xml"};
    if (ascii_case_folded_archive_entry_name(entry_name) ==
        ascii_case_folded_archive_entry_name(content_types_entry)) {
        return true;
    }

    const auto canonical = canonicalize_package_iri_path(entry_name, true);
    return canonical && validate_canonical_package_part_path(
                            canonical.entry_name, entry_name.ends_with('/')) ==
                            package_relationship_target_error::none;
}

inline auto
archive_compression_ratio_exceeds_limit(std::uint64_t uncompressed_size,
                                        std::uint64_t compressed_size,
                                        std::uint64_t maximum_ratio) -> bool {
    return compressed_size > 0U &&
           (uncompressed_size / compressed_size > maximum_ratio ||
            (uncompressed_size / compressed_size == maximum_ratio &&
             uncompressed_size % compressed_size != 0U));
}

inline auto validate_open_zip_archive_metadata(
    zip_t *archive, const featherdoc::archive_limits &limits,
    featherdoc::document_error_info &last_error_info,
    featherdoc::package_validation_mode validation_mode,
    archive_entry_catalog *catalog) -> std::error_code {
    if (catalog != nullptr) {
        catalog->clear();
    }
    const auto entry_count = zip_entries_total(archive);
    if (entry_count < 0) {
        return set_archive_validation_error(
            last_error_info,
            featherdoc::document_errc::invalid_package_structure,
            "failed to enumerate archive entries while validating package "
            "metadata");
    }
    if (static_cast<std::uint64_t>(entry_count) > limits.max_entries) {
        return set_archive_validation_error(
            last_error_info, featherdoc::document_errc::archive_limit_exceeded,
            "archive contains " + std::to_string(entry_count) +
                " entries; limit is " + std::to_string(limits.max_entries));
    }

    std::uint64_t total_uncompressed_size = 0U;
    std::uint64_t total_canonical_entry_name_bytes = 0U;
    std::unordered_map<std::string, std::string> seen_entry_identities;
    std::set<std::string> seen_part_identities;
    seen_entry_identities.reserve(static_cast<std::size_t>(entry_count));
    if (catalog != nullptr) {
        catalog->records.reserve(static_cast<std::size_t>(entry_count));
        catalog->by_identity.reserve(static_cast<std::size_t>(entry_count));
    }
    for (ssize_t index = 0; index < entry_count; ++index) {
        if (zip_entry_openbyindex(archive, static_cast<std::size_t>(index)) !=
            0) {
            return set_archive_validation_error(
                last_error_info,
                featherdoc::document_errc::invalid_package_structure,
                "failed to inspect archive entry at index " +
                    std::to_string(index));
        }

        const auto *entry_name_text = zip_entry_name(archive);
        const auto entry_name_size = zip_entry_name_size(archive);
        const std::string_view entry_name_view =
            entry_name_text == nullptr
                ? std::string_view{}
                : std::string_view{entry_name_text, entry_name_size};
        const std::string entry_name{entry_name_view};
        const auto diagnostic_entry_name =
            featherdoc::detail::sanitize_utf8_for_diagnostic(entry_name);
        const auto uncompressed_size =
            static_cast<std::uint64_t>(zip_entry_size(archive));
        const auto compressed_size =
            static_cast<std::uint64_t>(zip_entry_comp_size(archive));
        const auto entry_crc32 =
            static_cast<std::uint32_t>(zip_entry_crc32(archive));
        auto canonical_entry = package_relationship_target_resolution{};
        auto entry_identity = std::string{};
        const bool is_directory = entry_name.ends_with('/');

        std::error_code validation_error;
        if (entry_name_text == nullptr) {
            validation_error = set_archive_validation_error(
                last_error_info,
                featherdoc::document_errc::invalid_package_structure,
                "archive entry name is unavailable at index " +
                    std::to_string(index));
        } else if (entry_name_view.find('\0') != std::string_view::npos) {
            validation_error = set_archive_validation_error(
                last_error_info,
                featherdoc::document_errc::invalid_package_structure,
                "archive entry name contains an embedded NUL byte",
                diagnostic_entry_name);
        } else if (!featherdoc::detail::is_valid_utf8(entry_name_view)) {
            validation_error = set_archive_validation_error(
                last_error_info,
                featherdoc::document_errc::invalid_package_structure,
                "archive entry name is not valid UTF-8", diagnostic_entry_name);
        } else if (!is_canonical_archive_entry_name(entry_name)) {
            validation_error = set_archive_validation_error(
                last_error_info,
                featherdoc::document_errc::invalid_package_structure,
                "archive entry name is not a canonical relative package path",
                diagnostic_entry_name);
        } else {
            canonical_entry = canonical_package_part_key(entry_name);
            if (!canonical_entry) {
                validation_error = set_archive_validation_error(
                    last_error_info,
                    featherdoc::document_errc::invalid_package_structure,
                    std::string{package_relationship_target_error_message(
                        canonical_entry.error)},
                    diagnostic_entry_name);
            } else if (validation_mode ==
                           featherdoc::package_validation_mode::strict &&
                       canonical_entry.entry_name != entry_name) {
                validation_error = set_archive_validation_error(
                    last_error_info,
                    featherdoc::document_errc::invalid_package_structure,
                    "strict OPC validation requires an ASCII canonical "
                    "physical entry name with uppercase percent escapes",
                    diagnostic_entry_name);
            }
        }
        if (!validation_error) {
            entry_identity = package_part_identity(canonical_entry.entry_name);
            const auto [existing, inserted] =
                seen_entry_identities.emplace(entry_identity, entry_name);
            if (!inserted) {
                validation_error = set_archive_validation_error(
                    last_error_info,
                    featherdoc::document_errc::invalid_package_structure,
                    "archive contains duplicate, ASCII-case-confusable, or "
                    "raw/percent-equivalent physical entries at index " +
                        std::to_string(index) + "; first physical name is '" +
                        featherdoc::detail::sanitize_utf8_for_diagnostic(
                            existing->second) +
                        "'",
                    diagnostic_entry_name);
            }
        }
        if (!validation_error && !is_directory &&
            entry_identity != "[content_types].xml") {
            if (package_part_identity_has_derivation_conflict(
                    seen_part_identities, entry_identity)) {
                validation_error = set_archive_validation_error(
                    last_error_info,
                    featherdoc::document_errc::invalid_package_structure,
                    "archive contains package part names where one is "
                    "derived from the other by appending path segments",
                    diagnostic_entry_name);
            }

            if (!validation_error) {
                seen_part_identities.insert(entry_identity);
            }
        }
        if (!validation_error &&
            canonical_entry.entry_name.size() > max_archive_entry_name_bytes) {
            validation_error = set_archive_validation_error(
                last_error_info,
                featherdoc::document_errc::archive_limit_exceeded,
                "canonical entry name byte size is " +
                    std::to_string(canonical_entry.entry_name.size()) +
                    "; limit is " +
                    std::to_string(max_archive_entry_name_bytes),
                diagnostic_entry_name);
        }
        if (!validation_error && (total_canonical_entry_name_bytes >
                                      max_archive_total_entry_name_bytes ||
                                  canonical_entry.entry_name.size() >
                                      max_archive_total_entry_name_bytes -
                                          total_canonical_entry_name_bytes)) {
            validation_error = set_archive_validation_error(
                last_error_info,
                featherdoc::document_errc::archive_limit_exceeded,
                "total canonical entry-name byte size exceeds " +
                    std::to_string(max_archive_total_entry_name_bytes),
                diagnostic_entry_name);
        }
        if (!validation_error) {
            const auto entry_size_limit =
                archive_entry_uses_xml_size_limit(canonical_entry.entry_name)
                    ? limits.max_xml_part_bytes
                    : limits.max_binary_part_bytes;
            if (uncompressed_size > entry_size_limit) {
                validation_error = set_archive_validation_error(
                    last_error_info,
                    featherdoc::document_errc::archive_limit_exceeded,
                    "archive entry is " + std::to_string(uncompressed_size) +
                        " bytes; limit is " + std::to_string(entry_size_limit),
                    diagnostic_entry_name);
            }
        }
        if (!validation_error &&
            (total_uncompressed_size > limits.max_total_uncompressed_bytes ||
             uncompressed_size > limits.max_total_uncompressed_bytes -
                                     total_uncompressed_size)) {
            validation_error = set_archive_validation_error(
                last_error_info,
                featherdoc::document_errc::archive_limit_exceeded,
                "archive total uncompressed size exceeds " +
                    std::to_string(limits.max_total_uncompressed_bytes) +
                    " bytes",
                diagnostic_entry_name);
        } else if (!validation_error && uncompressed_size > 0U &&
                   compressed_size == 0U) {
            validation_error = set_archive_validation_error(
                last_error_info,
                featherdoc::document_errc::archive_limit_exceeded,
                "non-empty archive entry reports a zero compressed size",
                diagnostic_entry_name);
        } else if (!validation_error && archive_compression_ratio_exceeds_limit(
                                            uncompressed_size, compressed_size,
                                            limits.max_compression_ratio)) {
            validation_error = set_archive_validation_error(
                last_error_info,
                featherdoc::document_errc::archive_limit_exceeded,
                "archive entry compression ratio exceeds " +
                    std::to_string(limits.max_compression_ratio),
                diagnostic_entry_name);
        }

        if (zip_entry_close(archive) != 0 && !validation_error) {
            validation_error = set_archive_validation_error(
                last_error_info,
                featherdoc::document_errc::invalid_package_structure,
                "failed to finish inspecting archive entry",
                diagnostic_entry_name);
        }
        if (validation_error) {
            return validation_error;
        }
        if (catalog != nullptr) {
            const auto catalog_index = catalog->records.size();
            archive_entry_record record;
            record.zip_index = static_cast<std::size_t>(index);
            record.physical_name = entry_name;
            record.canonical_key = canonical_entry.entry_name;
            record.identity_key = entry_identity;
            record.crc32 = entry_crc32;
            record.compressed_size = compressed_size;
            record.uncompressed_size = uncompressed_size;
            record.is_directory = is_directory;
            record.noncanonical = canonical_entry.entry_name != entry_name;
            catalog->records.push_back(std::move(record));
            catalog->by_identity.emplace(entry_identity, catalog_index);
        }
        total_uncompressed_size += uncompressed_size;
        total_canonical_entry_name_bytes += canonical_entry.entry_name.size();
    }
    return {};
}

inline auto
source_archive_fingerprint_from_catalog(const archive_entry_catalog &catalog)
    -> source_archive_fingerprint {
    source_archive_fingerprint fingerprint;
    fingerprint.reserve(catalog.records.size());
    for (const auto &entry : catalog.records) {
        archive_entry_fingerprint fingerprint_entry;
        fingerprint_entry.physical_name = entry.physical_name;
        fingerprint_entry.canonical_key = entry.canonical_key;
        fingerprint_entry.identity_key = entry.identity_key;
        fingerprint_entry.crc32 = entry.crc32;
        fingerprint_entry.compressed_size = entry.compressed_size;
        fingerprint_entry.uncompressed_size = entry.uncompressed_size;
        fingerprint_entry.is_directory = entry.is_directory;
        fingerprint.push_back(std::move(fingerprint_entry));
    }
    return fingerprint;
}

inline auto set_source_archive_changed_error(
    featherdoc::document_error_info &last_error_info,
    std::string_view archive_path, std::string detail,
    std::string entry_name = {}) -> std::error_code {
    const auto safe_archive_path =
        featherdoc::detail::sanitize_utf8_for_diagnostic(archive_path);
    return set_archive_validation_error(
        last_error_info, featherdoc::document_errc::source_archive_changed,
        "source archive '" + safe_archive_path +
            "' changed after open(); refusing to mix package state: " +
            std::move(detail),
        std::move(entry_name));
}

inline auto set_reopened_source_archive_open_error(
    featherdoc::document_error_info &last_error_info,
    std::string_view archive_path, const zip_reader_open_result &open_result,
    std::string_view operation, std::string entry_name = {})
    -> std::error_code {
    if (open_result.zip_error == ZIP_EARCHLIMIT) {
        featherdoc::document_error_info reader_error_info;
        const auto safe_archive_path =
            featherdoc::detail::sanitize_utf8_for_diagnostic(archive_path);
        (void)set_archive_reader_open_limit_error(
            reader_error_info, safe_archive_path, open_result.limit_violation);
        return set_source_archive_changed_error(
            last_error_info, safe_archive_path, reader_error_info.detail,
            std::move(entry_name));
    }

    return set_source_archive_changed_error(
        last_error_info, archive_path,
        "archive became unavailable or invalid while " +
            featherdoc::detail::sanitize_utf8_for_diagnostic(operation) +
            " (ZIP error " + std::to_string(open_result.zip_error) + ")",
        std::move(entry_name));
}

inline auto validate_reopened_source_archive_metadata(
    zip_t *archive, const featherdoc::archive_limits &limits,
    featherdoc::document_error_info &last_error_info,
    featherdoc::package_validation_mode validation_mode,
    const source_archive_fingerprint &expected_fingerprint,
    std::string_view archive_path, archive_entry_catalog *catalog)
    -> std::error_code {
    archive_entry_catalog reopened_catalog;
    auto *catalog_to_populate =
        catalog != nullptr ? catalog : &reopened_catalog;
    if (const auto validation_error = validate_open_zip_archive_metadata(
            archive, limits, last_error_info, validation_mode,
            catalog_to_populate)) {
        const auto validation_entry = last_error_info.entry_name;
        auto validation_detail = last_error_info.detail;
        if (validation_detail.empty()) {
            validation_detail = validation_error.message();
        }
        return set_source_archive_changed_error(
            last_error_info, archive_path,
            "reopened ZIP metadata is no longer valid: " +
                std::move(validation_detail),
            validation_entry);
    }

    const auto actual_fingerprint =
        source_archive_fingerprint_from_catalog(*catalog_to_populate);
    if (actual_fingerprint == expected_fingerprint) {
        return {};
    }

    const auto shared_count =
        (std::min)(actual_fingerprint.size(), expected_fingerprint.size());
    std::size_t mismatch_index = 0U;
    while (mismatch_index < shared_count &&
           actual_fingerprint[mismatch_index] ==
               expected_fingerprint[mismatch_index]) {
        ++mismatch_index;
    }

    std::string entry_name;
    if (mismatch_index < actual_fingerprint.size()) {
        entry_name = actual_fingerprint[mismatch_index].physical_name;
    } else if (mismatch_index < expected_fingerprint.size()) {
        entry_name = expected_fingerprint[mismatch_index].physical_name;
    }

    if (mismatch_index == shared_count) {
        return set_source_archive_changed_error(
            last_error_info, archive_path,
            "entry count changed from " +
                std::to_string(expected_fingerprint.size()) + " to " +
                std::to_string(actual_fingerprint.size()),
            std::move(entry_name));
    }

    const auto &expected = expected_fingerprint[mismatch_index];
    const auto &actual = actual_fingerprint[mismatch_index];
    std::string changed_fields;
    const auto append_changed_field = [&](std::string_view field_name) {
        if (!changed_fields.empty()) {
            changed_fields += ", ";
        }
        changed_fields += field_name;
    };
    if (actual.physical_name != expected.physical_name) {
        append_changed_field("physical name/order");
    }
    if (actual.canonical_key != expected.canonical_key ||
        actual.identity_key != expected.identity_key) {
        append_changed_field("canonical identity/order");
    }
    if (actual.crc32 != expected.crc32) {
        append_changed_field("CRC32");
    }
    if (actual.compressed_size != expected.compressed_size) {
        append_changed_field("compressed size");
    }
    if (actual.uncompressed_size != expected.uncompressed_size) {
        append_changed_field("uncompressed size");
    }
    if (actual.is_directory != expected.is_directory) {
        append_changed_field("directory flag");
    }

    return set_source_archive_changed_error(
        last_error_info, archive_path,
        "entry metadata differs at physical index " +
            std::to_string(mismatch_index) + " (" + changed_fields + ")",
        std::move(entry_name));
}

inline auto enforce_open_zip_entry_xml_size_limit(
    zip_t *archive, const featherdoc::archive_limits &limits,
    std::string_view entry_name,
    featherdoc::document_error_info &last_error_info) -> std::error_code {
    const auto uncompressed_size =
        static_cast<std::uint64_t>(zip_entry_size(archive));
    const auto compressed_size =
        static_cast<std::uint64_t>(zip_entry_comp_size(archive));
    if (uncompressed_size > limits.max_xml_part_bytes) {
        return set_archive_xml_limit_error(
            last_error_info,
            "archive XML entry is " + std::to_string(uncompressed_size) +
                " bytes; limit is " + std::to_string(limits.max_xml_part_bytes),
            std::string{entry_name});
    }
    if (uncompressed_size > 0U && compressed_size == 0U) {
        return set_archive_xml_limit_error(
            last_error_info,
            "non-empty archive XML entry reports a zero compressed size",
            std::string{entry_name});
    }
    if (archive_compression_ratio_exceeds_limit(
            uncompressed_size, compressed_size, limits.max_compression_ratio)) {
        return set_archive_xml_limit_error(
            last_error_info,
            "archive XML entry compression ratio exceeds " +
                std::to_string(limits.max_compression_ratio),
            std::string{entry_name});
    }

    return {};
}

} // namespace featherdoc::detail
