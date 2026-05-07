#ifndef FEATHERDOC_PDF_DOCUMENT_ADAPTER_IMAGES_HPP
#define FEATHERDOC_PDF_DOCUMENT_ADAPTER_IMAGES_HPP

#include <featherdoc/pdf/pdf_document_adapter.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc::pdf::detail {

using InlineImageGroups =
    std::map<std::size_t, std::vector<featherdoc::inline_image_info>>;
using FloatingImageGroups =
    std::map<std::size_t, std::vector<featherdoc::drawing_image_info>>;

struct InlineImageRenderContext {
    std::function<PdfPageLayout &()> new_page;
};

struct FloatingTextExclusion {
    PdfRect bounds;
    featherdoc::floating_image_wrap_mode wrap_mode{
        featherdoc::floating_image_wrap_mode::none};
    double distance_left_points{0.0};
    double distance_right_points{0.0};
    double distance_top_points{0.0};
    double distance_bottom_points{0.0};
};

[[nodiscard]] std::string
image_extension_for_content_type(std::string_view content_type);

[[nodiscard]] bool
is_pdfio_supported_image_content_type(std::string_view content_type);

[[nodiscard]] double image_dpi(
    const PdfDocumentAdapterOptions &options) noexcept;

[[nodiscard]] double pixels_to_points(
    std::uint32_t pixels, const PdfDocumentAdapterOptions &options);

[[nodiscard]] std::filesystem::path
inline_image_output_path(const featherdoc::inline_image_info &image,
                         std::string_view extension);

[[nodiscard]] std::filesystem::path
drawing_image_output_path(const featherdoc::drawing_image_info &image,
                          std::string_view extension);

void scale_image_to_fit(double &width_points, double &height_points,
                        double max_width_points,
                        const PdfDocumentAdapterOptions &options);

[[nodiscard]] InlineImageGroups group_inline_images_by_body_block(
    const std::vector<featherdoc::inline_image_info> &images);

[[nodiscard]] std::vector<featherdoc::inline_image_info>
unpositioned_inline_images(
    const std::vector<featherdoc::inline_image_info> &images);

[[nodiscard]] FloatingImageGroups group_floating_images_by_body_block(
    const std::vector<featherdoc::drawing_image_info> &images);

[[nodiscard]] std::vector<featherdoc::drawing_image_info>
unpositioned_floating_images(
    const std::vector<featherdoc::drawing_image_info> &images);

[[nodiscard]] std::vector<FloatingTextExclusion>
floating_image_text_exclusions(
    const std::vector<featherdoc::drawing_image_info> &images,
    double anchor_top_y, const PdfDocumentAdapterOptions &options,
    double max_width_points);

void emit_inline_images(featherdoc::Document &document,
                        const std::vector<featherdoc::inline_image_info> &images,
                        PdfPageLayout *&page, double &current_y,
                        const PdfDocumentAdapterOptions &options,
                        double max_width_points,
                        const InlineImageRenderContext &context);

void emit_floating_images(
    featherdoc::Document &document,
    const std::vector<featherdoc::drawing_image_info> &images,
    PdfPageLayout &page, double anchor_top_y,
    const PdfDocumentAdapterOptions &options, double max_width_points);

} // namespace featherdoc::pdf::detail

#endif // FEATHERDOC_PDF_DOCUMENT_ADAPTER_IMAGES_HPP
