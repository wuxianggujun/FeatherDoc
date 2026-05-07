#include <featherdoc/pdf/pdf_font_subset.hpp>

#include <memory>
#include <string>

#if defined(FEATHERDOC_ENABLE_PDF_FONT_SUBSET)
#include <hb-subset.h>
#include <hb.h>
#endif

namespace featherdoc::pdf {
namespace {

#if defined(FEATHERDOC_ENABLE_PDF_FONT_SUBSET)
struct HbBlobCloser {
    void operator()(hb_blob_t *blob) const noexcept {
        hb_blob_destroy(blob);
    }
};

struct HbFaceCloser {
    void operator()(hb_face_t *face) const noexcept {
        hb_face_destroy(face);
    }
};

struct HbSubsetInputCloser {
    void operator()(hb_subset_input_t *input) const noexcept {
        hb_subset_input_destroy(input);
    }
};

template <typename T, typename Closer> using HbPtr = std::unique_ptr<T, Closer>;
#endif

[[nodiscard]] bool is_valid_scalar_value(std::uint32_t codepoint) noexcept {
    return codepoint <= 0x10FFFFU &&
           !(codepoint >= 0xD800U && codepoint <= 0xDFFFU);
}

} // namespace

PdfFontSubsetResult
subset_font_file_for_codepoints(const std::filesystem::path &font_file_path,
                                const std::vector<std::uint32_t> &codepoints) {
    PdfFontSubsetResult result;
#if !defined(FEATHERDOC_ENABLE_PDF_FONT_SUBSET)
    (void)font_file_path;
    (void)codepoints;
    result.error_message = "PDF font subsetting is not enabled";
    return result;
#else
    if (font_file_path.empty()) {
        result.error_message = "Font path is empty";
        return result;
    }
    if (codepoints.empty()) {
        result.error_message = "Font subset codepoint set is empty";
        return result;
    }

    const auto font_path = font_file_path.string();
    HbPtr<hb_blob_t, HbBlobCloser> source_blob(
        hb_blob_create_from_file_or_fail(font_path.c_str()));
    if (!source_blob) {
        result.error_message = "Unable to load font for subsetting: " +
                               font_file_path.string();
        return result;
    }

    HbPtr<hb_face_t, HbFaceCloser> source_face(
        hb_face_create_or_fail(source_blob.get(), 0));
    if (!source_face) {
        result.error_message = "Unable to create HarfBuzz face: " +
                               font_file_path.string();
        return result;
    }

    HbPtr<hb_subset_input_t, HbSubsetInputCloser> input(
        hb_subset_input_create_or_fail());
    if (!input) {
        result.error_message = "Unable to create HarfBuzz subset input";
        return result;
    }

    hb_subset_input_set_flags(input.get(), HB_SUBSET_FLAGS_RETAIN_GIDS);
    hb_set_t *unicode_set = hb_subset_input_unicode_set(input.get());
    hb_set_add(unicode_set, 0U);
    hb_set_add(unicode_set, 0x20U);
    for (const auto codepoint : codepoints) {
        if (is_valid_scalar_value(codepoint)) {
            hb_set_add(unicode_set, static_cast<hb_codepoint_t>(codepoint));
        }
    }

    HbPtr<hb_face_t, HbFaceCloser> subset_face(
        hb_subset_or_fail(source_face.get(), input.get()));
    if (!subset_face) {
        result.error_message = "HarfBuzz font subset failed: " +
                               font_file_path.string();
        return result;
    }

    HbPtr<hb_blob_t, HbBlobCloser> subset_blob(
        hb_face_reference_blob(subset_face.get()));
    if (!subset_blob) {
        result.error_message = "Unable to serialize HarfBuzz subset font";
        return result;
    }

    unsigned int length = 0U;
    const char *data = hb_blob_get_data(subset_blob.get(), &length);
    if (data == nullptr || length == 0U) {
        result.error_message = "HarfBuzz subset font is empty";
        return result;
    }

    result.font_data.assign(data, data + length);
    result.success = !result.font_data.empty();
    return result;
#endif
}

} // namespace featherdoc::pdf
