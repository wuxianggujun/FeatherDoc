#include <featherdoc.hpp>
#ifndef MINIZ_HEADER_FILE_ONLY
#define MINIZ_HEADER_FILE_ONLY
#endif
#include <miniz.h>

#include <cstddef>
#include <filesystem>
#include <system_error>
#include <type_traits>

namespace {

static_assert(std::is_nothrow_move_constructible_v<featherdoc::Document>);
static_assert(std::is_nothrow_move_assignable_v<featherdoc::Document>);

using document_open_signature =
    std::error_code (featherdoc::Document::*)();
using document_save_signature =
    std::error_code (featherdoc::Document::*)() const;
using document_save_as_signature =
    std::error_code (featherdoc::Document::*)(std::filesystem::path) const;
using document_inspect_paragraphs_signature =
    std::vector<featherdoc::paragraph_inspection_summary> (
        featherdoc::Document::*)();
using template_part_inspect_paragraphs_signature =
    std::vector<featherdoc::paragraph_inspection_summary> (
        featherdoc::TemplatePart::*)();
using template_part_inspect_paragraph_signature =
    std::optional<featherdoc::paragraph_inspection_summary> (
        featherdoc::TemplatePart::*)(std::size_t);

static_assert(std::is_same_v<
              decltype(static_cast<document_open_signature>(
                  &featherdoc::Document::open)),
              document_open_signature>);
static_assert(std::is_same_v<decltype(&featherdoc::Document::save),
                             document_save_signature>);
static_assert(std::is_same_v<decltype(&featherdoc::Document::save_as),
                             document_save_as_signature>);
static_assert(std::is_same_v<decltype(&featherdoc::Document::inspect_paragraphs),
                             document_inspect_paragraphs_signature>);
static_assert(std::is_same_v<
              decltype(&featherdoc::TemplatePart::inspect_paragraphs),
              template_part_inspect_paragraphs_signature>);
static_assert(std::is_same_v<
              decltype(&featherdoc::TemplatePart::inspect_paragraph),
              template_part_inspect_paragraph_signature>);

// v1.13.2 exposed archive_limits as a five-field aggregate. Keep both its
// positional mapping and binary layout stable within the 1.13 ABI line.
constexpr featherdoc::archive_limits legacy_archive_limits{
    11U, 22U, 33U, 44U, 55U};
static_assert(legacy_archive_limits.max_entries == 11U);
static_assert(legacy_archive_limits.max_xml_part_bytes == 22U);
static_assert(legacy_archive_limits.max_binary_part_bytes == 33U);
static_assert(legacy_archive_limits.max_total_uncompressed_bytes == 44U);
static_assert(legacy_archive_limits.max_compression_ratio == 55U);

struct legacy_archive_limits_layout {
    std::size_t max_entries;
    std::uint64_t max_xml_part_bytes;
    std::uint64_t max_binary_part_bytes;
    std::uint64_t max_total_uncompressed_bytes;
    std::uint32_t max_compression_ratio;
};

struct legacy_document_open_options_layout {
    featherdoc::package_validation_mode validation;
    legacy_archive_limits_layout limits;
};

static_assert(sizeof(featherdoc::archive_limits) ==
              sizeof(legacy_archive_limits_layout));
static_assert(alignof(featherdoc::archive_limits) ==
              alignof(legacy_archive_limits_layout));
static_assert(sizeof(featherdoc::document_open_options) ==
              sizeof(legacy_document_open_options_layout));
static_assert(alignof(featherdoc::document_open_options) ==
              alignof(legacy_document_open_options_layout));

#define FEATHERDOC_CHECK_ARCHIVE_LIMIT_OFFSET(member)                         \
    static_assert(offsetof(featherdoc::archive_limits, member) ==              \
                  offsetof(legacy_archive_limits_layout, member))
FEATHERDOC_CHECK_ARCHIVE_LIMIT_OFFSET(max_entries);
FEATHERDOC_CHECK_ARCHIVE_LIMIT_OFFSET(max_xml_part_bytes);
FEATHERDOC_CHECK_ARCHIVE_LIMIT_OFFSET(max_binary_part_bytes);
FEATHERDOC_CHECK_ARCHIVE_LIMIT_OFFSET(max_total_uncompressed_bytes);
FEATHERDOC_CHECK_ARCHIVE_LIMIT_OFFSET(max_compression_ratio);
#undef FEATHERDOC_CHECK_ARCHIVE_LIMIT_OFFSET

static_assert(offsetof(featherdoc::document_open_options, validation) ==
              offsetof(legacy_document_open_options_layout, validation));
static_assert(offsetof(featherdoc::document_open_options, limits) ==
              offsetof(legacy_document_open_options_layout, limits));

// miniz.h is installed as part of FeatherDoc's public dependency surface.
// Preserve the v1.13.2 mz_zip_archive object layout so an already-compiled
// consumer never passes a smaller object to the updated shared library.
struct legacy_mz_zip_archive_layout {
    mz_uint64 m_archive_size;
    mz_uint64 m_central_directory_file_ofs;
    mz_uint32 m_total_files;
    mz_zip_mode m_zip_mode;
    mz_zip_type m_zip_type;
    mz_zip_error m_last_error;
    mz_uint64 m_file_offset_alignment;
    mz_alloc_func m_pAlloc;
    mz_free_func m_pFree;
    mz_realloc_func m_pRealloc;
    void *m_pAlloc_opaque;
    mz_file_read_func m_pRead;
    mz_file_write_func m_pWrite;
    mz_file_needs_keepalive m_pNeeds_keepalive;
    void *m_pIO_opaque;
    mz_zip_internal_state *m_pState;
};

static_assert(sizeof(mz_zip_archive) ==
              sizeof(legacy_mz_zip_archive_layout));
static_assert(alignof(mz_zip_archive) ==
              alignof(legacy_mz_zip_archive_layout));
#define FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(member)                         \
    static_assert(offsetof(mz_zip_archive, member) ==                          \
                  offsetof(legacy_mz_zip_archive_layout, member))
FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(m_archive_size);
FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(m_central_directory_file_ofs);
FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(m_total_files);
FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(m_zip_mode);
FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(m_zip_type);
FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(m_last_error);
FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(m_file_offset_alignment);
FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(m_pAlloc);
FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(m_pFree);
FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(m_pRealloc);
FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(m_pAlloc_opaque);
FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(m_pRead);
FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(m_pWrite);
FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(m_pNeeds_keepalive);
FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(m_pIO_opaque);
FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET(m_pState);
#undef FEATHERDOC_CHECK_MINIZ_ARCHIVE_OFFSET

} // namespace

// Frozen source-level consumer for the v1.13.2 public API. Extend this fixture
// only when preserving an existing 1.13.x call pattern.
int main() {
    featherdoc::Document document;
    if (document.create_empty()) {
        return 1;
    }

    auto paragraph = document.paragraphs();
    if (!paragraph.has_next()) {
        return 2;
    }

    auto run = paragraph.add_run("FeatherDoc v1.13.2 source compatibility");
    if (!run.has_next()) {
        return 3;
    }

    return document.is_open() ? 0 : 4;
}
