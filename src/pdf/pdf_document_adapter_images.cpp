#include "pdf_document_adapter_images.hpp"

#include "pdf_document_adapter_render.hpp"

#include <algorithm>
#include <functional>
#include <system_error>
#include <utility>

namespace featherdoc::pdf::detail {
namespace {

[[nodiscard]] double signed_pixels_to_points(
    std::int32_t pixels, const PdfDocumentAdapterOptions &options) {
    return static_cast<double>(pixels) * 72.0 / image_dpi(options);
}

[[nodiscard]] double floating_image_x(
    const featherdoc::floating_image_options &floating_options,
    const PdfDocumentAdapterOptions &options) {
    const auto offset_points =
        signed_pixels_to_points(floating_options.horizontal_offset_px, options);

    if (floating_options.horizontal_reference ==
        featherdoc::floating_image_horizontal_reference::page) {
        return offset_points;
    }

    return options.margin_left_points + offset_points;
}

[[nodiscard]] double floating_image_top_y(
    const featherdoc::floating_image_options &floating_options,
    double anchor_top_y, const PdfDocumentAdapterOptions &options) {
    const auto offset_points =
        signed_pixels_to_points(floating_options.vertical_offset_px, options);

    if (floating_options.vertical_reference ==
        featherdoc::floating_image_vertical_reference::page) {
        return options.page_size.height_points - offset_points;
    }
    if (floating_options.vertical_reference ==
        featherdoc::floating_image_vertical_reference::margin) {
        return options.page_size.height_points - options.margin_top_points -
               offset_points;
    }

    return anchor_top_y - offset_points;
}

} // namespace

std::string image_extension_for_content_type(std::string_view content_type) {
    if (content_type == "image/png") {
        return ".png";
    }
    if (content_type == "image/jpeg" || content_type == "image/jpg") {
        return ".jpg";
    }
    return {};
}

bool is_pdfio_supported_image_content_type(std::string_view content_type) {
    return content_type == "image/png" || content_type == "image/jpeg" ||
           content_type == "image/jpg";
}

double image_dpi(const PdfDocumentAdapterOptions &options) noexcept {
    return options.image_default_dpi > 0.0 ? options.image_default_dpi : 96.0;
}

double pixels_to_points(std::uint32_t pixels,
                        const PdfDocumentAdapterOptions &options) {
    return static_cast<double>(pixels) * 72.0 / image_dpi(options);
}

std::filesystem::path
inline_image_output_path(const featherdoc::inline_image_info &image,
                         std::string_view extension) {
    const auto key = image.entry_name.empty() ? image.display_name
                                             : image.entry_name;
    const auto hash = std::hash<std::string>{}(key);
    return std::filesystem::temp_directory_path() /
           ("featherdoc-pdf-inline-image-" + std::to_string(image.index) +
           "-" + std::to_string(hash) + std::string{extension});
}

std::filesystem::path
drawing_image_output_path(const featherdoc::drawing_image_info &image,
                          std::string_view extension) {
    const auto key = image.entry_name.empty() ? image.display_name
                                             : image.entry_name;
    const auto hash = std::hash<std::string>{}(key);
    return std::filesystem::temp_directory_path() /
           ("featherdoc-pdf-drawing-image-" + std::to_string(image.index) +
            "-" + std::to_string(hash) + std::string{extension});
}

void scale_image_to_fit(double &width_points, double &height_points,
                        double max_width_points,
                        const PdfDocumentAdapterOptions &options) {
    if (width_points <= 0.0 || height_points <= 0.0) {
        return;
    }

    if (width_points > max_width_points) {
        const auto scale = max_width_points / width_points;
        width_points *= scale;
        height_points *= scale;
    }

    const auto max_height =
        std::max(1.0, options.page_size.height_points -
                          options.margin_top_points -
                          options.margin_bottom_points);
    if (height_points > max_height) {
        const auto scale = max_height / height_points;
        width_points *= scale;
        height_points *= scale;
    }
}

InlineImageGroups group_inline_images_by_body_block(
    const std::vector<featherdoc::inline_image_info> &images) {
    InlineImageGroups grouped;
    for (const auto &image : images) {
        if (!image.body_block_index.has_value()) {
            continue;
        }
        grouped[*image.body_block_index].push_back(image);
    }
    return grouped;
}

std::vector<featherdoc::inline_image_info> unpositioned_inline_images(
    const std::vector<featherdoc::inline_image_info> &images) {
    std::vector<featherdoc::inline_image_info> unpositioned;
    for (const auto &image : images) {
        if (!image.paragraph_index.has_value()) {
            unpositioned.push_back(image);
        }
    }
    return unpositioned;
}

FloatingImageGroups group_floating_images_by_body_block(
    const std::vector<featherdoc::drawing_image_info> &images) {
    FloatingImageGroups grouped;
    for (const auto &image : images) {
        if (image.placement != featherdoc::drawing_image_placement::anchored_object ||
            !image.body_block_index.has_value()) {
            continue;
        }
        grouped[*image.body_block_index].push_back(image);
    }
    return grouped;
}

std::vector<featherdoc::drawing_image_info> unpositioned_floating_images(
    const std::vector<featherdoc::drawing_image_info> &images) {
    std::vector<featherdoc::drawing_image_info> unpositioned;
    for (const auto &image : images) {
        if (image.placement != featherdoc::drawing_image_placement::anchored_object ||
            image.paragraph_index.has_value()) {
            continue;
        }
        unpositioned.push_back(image);
    }
    return unpositioned;
}

std::vector<FloatingTextExclusion> floating_image_text_exclusions(
    const std::vector<featherdoc::drawing_image_info> &images,
    double anchor_top_y, const PdfDocumentAdapterOptions &options,
    double max_width_points) {
    std::vector<FloatingTextExclusion> exclusions;
    if (!options.render_inline_images) {
        return exclusions;
    }

    for (const auto &image : images) {
        if (!image.floating_options.has_value() ||
            image.floating_options->behind_text ||
            image.floating_options->wrap_mode ==
                featherdoc::floating_image_wrap_mode::none) {
            continue;
        }

        auto width_points = pixels_to_points(image.width_px, options);
        auto height_points = pixels_to_points(image.height_px, options);
        if (width_points <= 0.0 || height_points <= 0.0) {
            continue;
        }

        scale_image_to_fit(width_points, height_points, max_width_points,
                           options);
        const auto image_x = floating_image_x(*image.floating_options, options);
        const auto image_top =
            floating_image_top_y(*image.floating_options, anchor_top_y,
                                 options);

        exclusions.push_back(FloatingTextExclusion{
            PdfRect{image_x, image_top - height_points, width_points,
                    height_points},
            image.floating_options->wrap_mode,
            pixels_to_points(image.floating_options->wrap_distance_left_px,
                             options),
            pixels_to_points(image.floating_options->wrap_distance_right_px,
                             options),
            pixels_to_points(image.floating_options->wrap_distance_top_px,
                             options),
            pixels_to_points(image.floating_options->wrap_distance_bottom_px,
                             options),
        });
    }

    return exclusions;
}

void emit_inline_image(featherdoc::Document &document,
                       const featherdoc::inline_image_info &image,
                       PdfPageLayout *&page, double &current_y,
                       const PdfDocumentAdapterOptions &options,
                       double max_width_points,
                       const InlineImageRenderContext &context) {
    const auto extension = image_extension_for_content_type(image.content_type);
    if (extension.empty()) {
        return;
    }

    const auto source_path = inline_image_output_path(image, extension);
    std::error_code remove_error;
    std::filesystem::remove(source_path, remove_error);

    if (!document.extract_inline_image(image.index, source_path)) {
        return;
    }

    if (!is_pdfio_supported_image_content_type(image.content_type)) {
        std::filesystem::remove(source_path, remove_error);
        return;
    }

    auto width_points = pixels_to_points(image.width_px, options);
    auto height_points = pixels_to_points(image.height_px, options);
    if (width_points <= 0.0 || height_points <= 0.0) {
        std::filesystem::remove(source_path, remove_error);
        return;
    }

    scale_image_to_fit(width_points, height_points, max_width_points, options);

    const auto image_top = current_y + options.font_size_points;
    if (image_top - height_points < options.margin_bottom_points) {
        page = &context.new_page();
        current_y = first_baseline_y(options);
    }

    const auto top = current_y + options.font_size_points;
    const auto bottom = top - height_points;
    page->images.push_back(PdfImage{
        PdfRect{options.margin_left_points, bottom, width_points, height_points},
        source_path,
        image.content_type,
        image.display_name.empty() ? image.entry_name : image.display_name,
        true,
        true,
    });

    current_y = bottom - options.image_spacing_after_points;
}

void emit_floating_image(featherdoc::Document &document,
                         const featherdoc::drawing_image_info &image,
                         PdfPageLayout &page, double anchor_top_y,
                         const PdfDocumentAdapterOptions &options,
                         double max_width_points) {
    if (!image.floating_options.has_value()) {
        return;
    }

    const auto extension = image_extension_for_content_type(image.content_type);
    if (extension.empty()) {
        return;
    }

    const auto source_path = drawing_image_output_path(image, extension);
    std::error_code remove_error;
    std::filesystem::remove(source_path, remove_error);

    if (!document.extract_drawing_image(image.index, source_path)) {
        return;
    }

    if (!is_pdfio_supported_image_content_type(image.content_type)) {
        std::filesystem::remove(source_path, remove_error);
        return;
    }

    auto width_points = pixels_to_points(image.width_px, options);
    auto height_points = pixels_to_points(image.height_px, options);
    if (width_points <= 0.0 || height_points <= 0.0) {
        std::filesystem::remove(source_path, remove_error);
        return;
    }

    scale_image_to_fit(width_points, height_points, max_width_points, options);

    const auto image_x = floating_image_x(*image.floating_options, options);
    const auto image_top =
        floating_image_top_y(*image.floating_options, anchor_top_y, options);
    PdfImage pdf_image{
        PdfRect{image_x, image_top - height_points, width_points,
                height_points},
        source_path,
        image.content_type,
        image.display_name.empty() ? image.entry_name : image.display_name,
        true,
        true,
    };
    pdf_image.draw_behind_text = image.floating_options->behind_text;

    if (image.floating_options->crop.has_value()) {
        const auto &crop = *image.floating_options->crop;
        pdf_image.crop_left_per_mille = crop.left_per_mille;
        pdf_image.crop_top_per_mille = crop.top_per_mille;
        pdf_image.crop_right_per_mille = crop.right_per_mille;
        pdf_image.crop_bottom_per_mille = crop.bottom_per_mille;
    }

    page.images.push_back(std::move(pdf_image));
}

void emit_inline_images(featherdoc::Document &document,
                        const std::vector<featherdoc::inline_image_info> &images,
                        PdfPageLayout *&page, double &current_y,
                        const PdfDocumentAdapterOptions &options,
                        double max_width_points,
                        const InlineImageRenderContext &context) {
    if (!options.render_inline_images) {
        return;
    }

    for (const auto &image : images) {
        emit_inline_image(document, image, page, current_y, options,
                          max_width_points, context);
    }
}

void emit_floating_images(
    featherdoc::Document &document,
    const std::vector<featherdoc::drawing_image_info> &images,
    PdfPageLayout &page, double anchor_top_y,
    const PdfDocumentAdapterOptions &options, double max_width_points) {
    if (!options.render_inline_images) {
        return;
    }

    for (const auto &image : images) {
        emit_floating_image(document, image, page, anchor_top_y, options,
                            max_width_points);
    }
}

} // namespace featherdoc::pdf::detail
