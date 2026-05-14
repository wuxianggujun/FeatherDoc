#if defined(FEATHERDOC_ENABLE_PDF_FONT_SUBSET)
#include <featherdoc/pdf/pdf_font_subset.hpp>
#endif
#include <featherdoc/pdf/pdf_text_metrics.hpp>
#include <featherdoc/pdf/pdf_writer.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

extern "C" {
#include <pdfio-content.h>
#include <pdfio.h>
}

namespace featherdoc::pdf {
namespace {

constexpr double kPi = 3.14159265358979323846;

struct PdfioErrorState {
    std::string message;
};

bool pdfio_error_callback(pdfio_file_t *, const char *message, void *data) {
    auto *state = static_cast<PdfioErrorState *>(data);
    if (state != nullptr && message != nullptr) {
        state->message = message;
    }
    return true;
}

struct PdfioFileCloser {
    void operator()(pdfio_file_t *pdf) const {
        if (pdf != nullptr) {
            pdfioFileClose(pdf);
        }
    }
};

struct PdfioStreamCloser {
    void operator()(pdfio_stream_t *stream) const {
        if (stream != nullptr) {
            pdfioStreamClose(stream);
        }
    }
};

struct FontResourceKey {
    std::string family;
    std::filesystem::path file_path;
    bool bold{false};
    bool italic{false};
    bool unicode{false};

    [[nodiscard]] friend bool operator<(const FontResourceKey &left,
                                        const FontResourceKey &right) {
        return std::tie(left.family, left.file_path, left.bold, left.italic,
                        left.unicode) < std::tie(right.family, right.file_path,
                                                 right.bold, right.italic,
                                                 right.unicode);
    }
};

struct ImageResourceKey {
    std::filesystem::path file_path;
    bool interpolate{true};

    [[nodiscard]] friend bool operator<(const ImageResourceKey &left,
                                        const ImageResourceKey &right) {
        return std::tie(left.file_path, left.interpolate) <
               std::tie(right.file_path, right.interpolate);
    }
};

struct ImageSourceCleanup {
    std::vector<std::filesystem::path> paths;

    ~ImageSourceCleanup() {
        for (const auto &path : paths) {
            std::error_code error;
            std::filesystem::remove(path, error);
        }
    }
};

struct FontSubsetStorage {
    std::vector<std::vector<unsigned char>> buffers;
};

[[nodiscard]] std::string build_error(std::string fallback,
                                      const PdfioErrorState &state) {
    if (!state.message.empty()) {
        fallback += ": ";
        fallback += state.message;
    }
    return fallback;
}

[[nodiscard]] FontResourceKey make_font_key(const PdfTextRun &text_run) {
    return FontResourceKey{
        text_run.font_family.empty() ? "Helvetica" : text_run.font_family,
        text_run.font_file_path,
        text_run.bold,
        text_run.italic,
        text_run.unicode,
    };
}

[[nodiscard]] std::string base_font_family_name(const FontResourceKey &key) {
    if (key.family == "Helvetica") {
        if (key.bold && key.italic) {
            return "Helvetica-BoldOblique";
        }
        if (key.bold) {
            return "Helvetica-Bold";
        }
        if (key.italic) {
            return "Helvetica-Oblique";
        }
    } else if (key.family == "Times" || key.family == "Times-Roman") {
        if (key.bold && key.italic) {
            return "Times-BoldItalic";
        }
        if (key.bold) {
            return "Times-Bold";
        }
        if (key.italic) {
            return "Times-Italic";
        }
        return "Times-Roman";
    } else if (key.family == "Courier") {
        if (key.bold && key.italic) {
            return "Courier-BoldOblique";
        }
        if (key.bold) {
            return "Courier-Bold";
        }
        if (key.italic) {
            return "Courier-Oblique";
        }
    }

    return key.family;
}

[[nodiscard]] std::string font_label(const FontResourceKey &key) {
    if (!key.file_path.empty()) {
        return key.file_path.string();
    }
    return base_font_family_name(key);
}

[[nodiscard]] std::string image_label(const ImageResourceKey &key) {
    return key.file_path.string();
}

[[nodiscard]] std::uint32_t decode_utf8_codepoint(std::string_view text,
                                                  std::size_t &index) noexcept;

[[nodiscard]] pdfio_obj_t *
create_pdfio_font(pdfio_file_t *pdf, const FontResourceKey &key,
                  const std::string &used_text, bool subset_unicode_fonts,
                  FontSubsetStorage &subset_storage) {
    if (!key.file_path.empty()) {
#if defined(FEATHERDOC_ENABLE_PDF_FONT_SUBSET)
        if (subset_unicode_fonts && key.unicode && !used_text.empty()) {
            std::set<std::uint32_t> unique_codepoints;
            for (std::size_t index = 0U; index < used_text.size();) {
                unique_codepoints.insert(
                    decode_utf8_codepoint(used_text, index));
            }
            const auto subset_result = subset_font_file_for_codepoints(
                key.file_path,
                std::vector<std::uint32_t>(unique_codepoints.begin(),
                                           unique_codepoints.end()));
            if (subset_result.success && !subset_result.font_data.empty()) {
                subset_storage.buffers.push_back(
                    std::move(subset_result.font_data));
                const auto &font_data = subset_storage.buffers.back();
                pdfio_obj_t *font = pdfioFileCreateFontObjFromData(
                    pdf, font_data.data(), font_data.size(), key.unicode);
                if (font != nullptr) {
                    return font;
                }
                subset_storage.buffers.pop_back();
            }
        }
#else
        (void)used_text;
        (void)subset_unicode_fonts;
        (void)subset_storage;
#endif
        const auto font_path = key.file_path.string();
        return pdfioFileCreateFontObjFromFile(pdf, font_path.c_str(),
                                              key.unicode);
    }

    const auto family = base_font_family_name(key);
    return pdfioFileCreateFontObjFromBase(pdf, family.c_str());
}

[[nodiscard]] const char *make_pdfio_resource_name(pdfio_file_t *pdf,
                                                   const std::string &name,
                                                   std::string &error_message) {
    const auto *resource_name = pdfioStringCreate(pdf, name.c_str());
    if (resource_name == nullptr) {
        error_message = "Unable to allocate PDF resource name: " + name;
    }
    return resource_name;
}

[[nodiscard]] pdfio_linecap_t to_pdfio_line_cap(PdfLineCap line_cap) noexcept {
    switch (line_cap) {
    case PdfLineCap::butt:
        return PDFIO_LINECAP_BUTT;
    case PdfLineCap::round:
        return PDFIO_LINECAP_ROUND;
    case PdfLineCap::square:
        return PDFIO_LINECAP_SQUARE;
    }

    return PDFIO_LINECAP_BUTT;
}

void append_utf16be_hex_unit(std::string &output, std::uint16_t value) {
    constexpr char digits[] = "0123456789ABCDEF";
    output.push_back(digits[(value >> 12U) & 0x0FU]);
    output.push_back(digits[(value >> 8U) & 0x0FU]);
    output.push_back(digits[(value >> 4U) & 0x0FU]);
    output.push_back(digits[value & 0x0FU]);
}

[[nodiscard]] std::uint32_t decode_utf8_codepoint(std::string_view text,
                                                  std::size_t &index) noexcept {
    if (index >= text.size()) {
        return 0xFFFDU;
    }

    const auto first = static_cast<unsigned char>(text[index++]);
    if (first < 0x80U) {
        return first;
    }

    auto continuation = [&](std::uint32_t &value) noexcept {
        if (index >= text.size()) {
            return false;
        }
        const auto next = static_cast<unsigned char>(text[index]);
        if ((next & 0xC0U) != 0x80U) {
            return false;
        }
        ++index;
        value = (value << 6U) | (next & 0x3FU);
        return true;
    };

    if ((first & 0xE0U) == 0xC0U) {
        std::uint32_t value = first & 0x1FU;
        return continuation(value) ? value : 0xFFFDU;
    }
    if ((first & 0xF0U) == 0xE0U) {
        std::uint32_t value = first & 0x0FU;
        return continuation(value) && continuation(value) ? value : 0xFFFDU;
    }
    if ((first & 0xF8U) == 0xF0U) {
        std::uint32_t value = first & 0x07U;
        return continuation(value) && continuation(value) && continuation(value)
                   ? value
                   : 0xFFFDU;
    }

    return 0xFFFDU;
}

[[nodiscard]] std::string utf16be_hex_from_utf8(std::string_view text) {
    std::string hex{"FEFF"};
    for (std::size_t index = 0U; index < text.size();) {
        auto codepoint = decode_utf8_codepoint(text, index);
        if (codepoint > 0x10FFFFU ||
            (codepoint >= 0xD800U && codepoint <= 0xDFFFU)) {
            codepoint = 0xFFFDU;
        }

        if (codepoint <= 0xFFFFU) {
            append_utf16be_hex_unit(hex, static_cast<std::uint16_t>(codepoint));
        } else {
            codepoint -= 0x10000U;
            append_utf16be_hex_unit(
                hex, static_cast<std::uint16_t>(0xD800U +
                                                ((codepoint >> 10U) & 0x3FFU)));
            append_utf16be_hex_unit(hex, static_cast<std::uint16_t>(
                                             0xDC00U + (codepoint & 0x3FFU)));
        }
    }
    return hex;
}

[[nodiscard]] bool write_actual_text_begin(pdfio_stream_t *stream,
                                           std::string_view text,
                                           std::string &error_message) {
    const auto hex = utf16be_hex_from_utf8(text);
    const auto command =
        std::string{"/Span <</ActualText <"} + hex + ">>> BDC\n";
    if (!pdfioStreamPuts(stream, command.c_str())) {
        error_message = "Unable to write PDF ActualText marker";
        return false;
    }
    return true;
}

[[nodiscard]] bool write_actual_text_end(pdfio_stream_t *stream,
                                         std::string &error_message) {
    if (!pdfioStreamPuts(stream, "EMC\n")) {
        error_message = "Unable to close PDF ActualText marker";
        return false;
    }
    return true;
}

[[nodiscard]] bool write_text_underline(pdfio_stream_t *stream,
                                        const PdfTextRun &text_run,
                                        std::string &error_message) {
    if (!text_run.underline || text_run.text.empty() ||
        text_run.font_size_points <= 0.0 ||
        std::abs(text_run.rotation_degrees) > 0.0001) {
        return true;
    }

    const auto underline_width = measure_text_width_points(
        text_run.text, text_run.font_size_points,
        PdfTextMetricsOptions{text_run.font_family, text_run.font_file_path});
    if (underline_width <= 0.0) {
        return true;
    }

    const double underline_y =
        text_run.baseline_origin.y_points - text_run.font_size_points * 0.12;
    const double line_width = std::max(0.5, text_run.font_size_points / 16.0);

    if (!pdfioContentSetStrokeColorDeviceRGB(stream, text_run.fill_color.red,
                                             text_run.fill_color.green,
                                             text_run.fill_color.blue) ||
        !pdfioContentSetLineWidth(stream, line_width) ||
        !pdfioContentSetLineCap(stream, PDFIO_LINECAP_BUTT) ||
        !pdfioContentSetDashPattern(stream, 0.0, 0.0, 0.0) ||
        !pdfioContentPathMoveTo(stream, text_run.baseline_origin.x_points,
                                underline_y) ||
        !pdfioContentPathLineTo(
            stream, text_run.baseline_origin.x_points + underline_width,
            underline_y) ||
        !pdfioContentStroke(stream)) {
        error_message = "Unable to write PDF text underline";
        return false;
    }

    return true;
}

[[nodiscard]] bool set_text_run_matrix(pdfio_stream_t *stream,
                                       const PdfTextRun &text_run) {
    if (std::abs(text_run.rotation_degrees) <= 0.0001) {
        if (text_run.synthetic_italic && text_run.italic) {
            constexpr double kSyntheticItalicDegrees = 12.0;
            const auto skew =
                std::tan(kSyntheticItalicDegrees * kPi / 180.0);
            pdfio_matrix_t matrix = {
                {1.0, 0.0},
                {skew, 1.0},
                {text_run.baseline_origin.x_points,
                 text_run.baseline_origin.y_points},
            };
            return pdfioContentSetTextMatrix(stream, matrix);
        }
        return pdfioContentTextMoveTo(stream,
                                      text_run.baseline_origin.x_points,
                                      text_run.baseline_origin.y_points);
    }

    const auto radians = text_run.rotation_degrees * kPi / 180.0;
    const auto cosine = std::cos(radians);
    const auto sine = std::sin(radians);
    pdfio_matrix_t matrix = {
        {cosine, sine},
        {-sine, cosine},
        {text_run.baseline_origin.x_points,
         text_run.baseline_origin.y_points},
    };
    return pdfioContentSetTextMatrix(stream, matrix);
}

[[nodiscard]] bool set_text_rendering_style(pdfio_stream_t *stream,
                                            const PdfTextRun &text_run) {
    const bool synthetic_bold = text_run.synthetic_bold && text_run.bold;
    if (!pdfioContentSetTextRenderingMode(
            stream, synthetic_bold ? PDFIO_TEXTRENDERING_FILL_AND_STROKE
                                   : PDFIO_TEXTRENDERING_FILL)) {
        return false;
    }

    if (!synthetic_bold) {
        return true;
    }

    const double stroke_width =
        std::max(0.2, text_run.font_size_points / 48.0);
    return pdfioContentSetStrokeColorDeviceRGB(
               stream, text_run.fill_color.red, text_run.fill_color.green,
               text_run.fill_color.blue) &&
           pdfioContentSetLineWidth(stream, stroke_width);
}

[[nodiscard]] pdfio_rect_t
page_bounds_rect(const PdfPageSize &page_size) noexcept {
    return pdfio_rect_t{0.0, 0.0, page_size.width_points,
                        page_size.height_points};
}

[[nodiscard]] bool set_page_bounds(pdfio_dict_t *page_dict,
                                   const PdfPageLayout &page,
                                   std::string &error_message) {
    auto media_box = page_bounds_rect(page.size);
    auto crop_box = media_box;
    if (!pdfioDictSetRect(page_dict, "MediaBox", &media_box) ||
        !pdfioDictSetRect(page_dict, "CropBox", &crop_box)) {
        error_message = "Unable to set PDF page bounds";
        return false;
    }

    return true;
}

[[nodiscard]] bool has_image_crop(const PdfImage &image) noexcept {
    return image.crop_left_per_mille != 0U ||
           image.crop_top_per_mille != 0U ||
           image.crop_right_per_mille != 0U ||
           image.crop_bottom_per_mille != 0U;
}

[[nodiscard]] bool image_crop_is_valid(const PdfImage &image) noexcept {
    constexpr std::uint32_t max_crop_per_mille = 1000U;
    if (image.crop_left_per_mille > max_crop_per_mille ||
        image.crop_top_per_mille > max_crop_per_mille ||
        image.crop_right_per_mille > max_crop_per_mille ||
        image.crop_bottom_per_mille > max_crop_per_mille) {
        return false;
    }

    return image.crop_left_per_mille + image.crop_right_per_mille <
               max_crop_per_mille &&
           image.crop_top_per_mille + image.crop_bottom_per_mille <
               max_crop_per_mille;
}

[[nodiscard]] bool write_pdf_image(pdfio_stream_t *stream,
                                   const PdfImage &image,
                                   const std::string &resource_name,
                                   std::string &error_message) {
    if (!has_image_crop(image)) {
        if (!pdfioContentDrawImage(
                stream, resource_name.c_str(), image.bounds.x_points,
                image.bounds.y_points, image.bounds.width_points,
                image.bounds.height_points)) {
            error_message =
                "Unable to draw PDF image: " + image.source_path.string();
            return false;
        }
        return true;
    }

    if (!image_crop_is_valid(image)) {
        error_message =
            "Invalid PDF image crop: " + image.source_path.string();
        return false;
    }

    constexpr double crop_scale = 1000.0;
    const double crop_left =
        static_cast<double>(image.crop_left_per_mille) / crop_scale;
    const double crop_top =
        static_cast<double>(image.crop_top_per_mille) / crop_scale;
    const double crop_right =
        static_cast<double>(image.crop_right_per_mille) / crop_scale;
    const double crop_bottom =
        static_cast<double>(image.crop_bottom_per_mille) / crop_scale;
    const double visible_width = 1.0 - crop_left - crop_right;
    const double visible_height = 1.0 - crop_top - crop_bottom;
    const double draw_width = image.bounds.width_points / visible_width;
    const double draw_height = image.bounds.height_points / visible_height;

    const double draw_x = image.bounds.x_points - crop_left * draw_width;
    const double draw_y = image.bounds.y_points - crop_bottom * draw_height;

    if (!pdfioContentSave(stream) ||
        !pdfioContentPathRect(stream, image.bounds.x_points,
                              image.bounds.y_points,
                              image.bounds.width_points,
                              image.bounds.height_points) ||
        !pdfioContentClip(stream, false) ||
        !pdfioStreamPuts(stream, "n\n")) {
        error_message =
            "Unable to set PDF image crop: " + image.source_path.string();
        pdfioContentRestore(stream);
        return false;
    }

    if (!pdfioContentDrawImage(stream, resource_name.c_str(), draw_x, draw_y,
                               draw_width, draw_height)) {
        error_message =
            "Unable to draw cropped PDF image: " + image.source_path.string();
        pdfioContentRestore(stream);
        return false;
    }

    if (!pdfioContentRestore(stream)) {
        error_message =
            "Unable to restore PDF image crop: " + image.source_path.string();
        return false;
    }

    return true;
}

[[nodiscard]] bool write_page_images(
    pdfio_stream_t *stream, const PdfPageLayout &page,
    const std::map<ImageResourceKey, std::string> &image_resources,
    bool draw_behind_text, std::string &error_message) {
    for (const auto &image : page.images) {
        if (image.draw_behind_text != draw_behind_text) {
            continue;
        }

        const auto image_key =
            ImageResourceKey{image.source_path, image.interpolate};
        const auto image_resource = image_resources.find(image_key);
        if (image_resource == image_resources.end()) {
            error_message =
                "Missing PDF image resource: " + image.source_path.string();
            return false;
        }

        if (!write_pdf_image(stream, image, image_resource->second,
                             error_message)) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool write_probe_page(pdfio_file_t *pdf,
                                    const PdfPageLayout &page,
                                    const PdfWriterOptions &options,
                                    FontSubsetStorage &subset_storage,
                                    std::string &error_message) {
    pdfio_dict_t *page_dict = pdfioDictCreate(pdf);
    if (page_dict == nullptr) {
        error_message = "Unable to create PDF page dictionary";
        return false;
    }

    if (!set_page_bounds(page_dict, page, error_message)) {
        return false;
    }

    std::map<FontResourceKey, std::string> used_text_by_font;
    for (const auto &text_run : page.text_runs) {
        used_text_by_font[make_font_key(text_run)] += text_run.text;
    }

    std::map<FontResourceKey, std::string> font_resources;
    for (const auto &text_run : page.text_runs) {
        const auto font_key = make_font_key(text_run);
        if (font_resources.find(font_key) != font_resources.end()) {
            continue;
        }

        const std::string resource_name =
            "F" + std::to_string(font_resources.size() + 1);
        const auto *pdf_resource_name =
            make_pdfio_resource_name(pdf, resource_name, error_message);
        if (pdf_resource_name == nullptr) {
            return false;
        }
        if (font_key.unicode && font_key.file_path.empty()) {
            error_message =
                "Unicode PDF text requires an embedded font file: " +
                font_label(font_key);
            return false;
        }
        const auto used_text = used_text_by_font.find(font_key);
        pdfio_obj_t *font = create_pdfio_font(
            pdf, font_key,
            used_text == used_text_by_font.end() ? std::string{}
                                                 : used_text->second,
            options.subset_unicode_fonts, subset_storage);
        if (font == nullptr) {
            error_message =
                "Unable to create PDF font: " + font_label(font_key);
            return false;
        }

        if (!pdfioPageDictAddFont(page_dict, pdf_resource_name, font)) {
            error_message =
                "Unable to add PDF font resource: " + font_label(font_key);
            return false;
        }

        font_resources.emplace(font_key, resource_name);
    }

    std::map<ImageResourceKey, std::string> image_resources;
    for (const auto &image : page.images) {
        if (image.source_path.empty()) {
            error_message = "PDF image source path is empty";
            return false;
        }

        const auto image_key =
            ImageResourceKey{image.source_path, image.interpolate};
        if (image_resources.find(image_key) != image_resources.end()) {
            continue;
        }

        const std::string resource_name =
            "Im" + std::to_string(image_resources.size() + 1);
        const auto *pdf_resource_name =
            make_pdfio_resource_name(pdf, resource_name, error_message);
        if (pdf_resource_name == nullptr) {
            return false;
        }
        const auto image_path = image.source_path.string();
        pdfio_obj_t *image_object = pdfioFileCreateImageObjFromFile(
            pdf, image_path.c_str(), image.interpolate);
        if (image_object == nullptr) {
            error_message =
                "Unable to create PDF image: " + image_label(image_key);
            return false;
        }

        if (!pdfioPageDictAddImage(page_dict, pdf_resource_name,
                                   image_object)) {
            error_message =
                "Unable to add PDF image resource: " + image_label(image_key);
            return false;
        }

        image_resources.emplace(image_key, resource_name);
    }

    std::unique_ptr<pdfio_stream_t, PdfioStreamCloser> stream(
        pdfioFileCreatePage(pdf, page_dict));
    if (!stream) {
        error_message = "Unable to create PDF page stream";
        return false;
    }

    for (const auto &rectangle : page.rectangles) {
        if (!rectangle.stroke && !rectangle.fill) {
            continue;
        }

        if (rectangle.fill &&
            !pdfioContentSetFillColorDeviceRGB(
                stream.get(), rectangle.fill_color.red,
                rectangle.fill_color.green, rectangle.fill_color.blue)) {
            error_message = "Unable to set PDF rectangle fill color";
            return false;
        }

        if (rectangle.stroke &&
            (!pdfioContentSetStrokeColorDeviceRGB(
                 stream.get(), rectangle.stroke_color.red,
                 rectangle.stroke_color.green, rectangle.stroke_color.blue) ||
             !pdfioContentSetLineWidth(stream.get(),
                                       rectangle.line_width_points))) {
            error_message = "Unable to set PDF rectangle stroke";
            return false;
        }

        if (!pdfioContentPathRect(stream.get(), rectangle.bounds.x_points,
                                  rectangle.bounds.y_points,
                                  rectangle.bounds.width_points,
                                  rectangle.bounds.height_points)) {
            error_message = "Unable to write PDF rectangle path";
            return false;
        }

        if (rectangle.fill && rectangle.stroke) {
            if (!pdfioContentFillAndStroke(stream.get(), false)) {
                error_message = "Unable to fill and stroke PDF rectangle";
                return false;
            }
        } else if (rectangle.fill) {
            if (!pdfioContentFill(stream.get(), false)) {
                error_message = "Unable to fill PDF rectangle";
                return false;
            }
        } else if (!pdfioContentStroke(stream.get())) {
            error_message = "Unable to stroke PDF rectangle";
            return false;
        }
    }

    for (const auto &line : page.lines) {
        if (!pdfioContentSetStrokeColorDeviceRGB(
                stream.get(), line.stroke_color.red, line.stroke_color.green,
                line.stroke_color.blue) ||
            !pdfioContentSetLineWidth(stream.get(), line.line_width_points) ||
            !pdfioContentSetLineCap(stream.get(),
                                    to_pdfio_line_cap(line.line_cap)) ||
            !pdfioContentSetDashPattern(stream.get(), line.dash_phase_points,
                                        line.dash_on_points,
                                        line.dash_off_points) ||
            !pdfioContentPathMoveTo(stream.get(), line.start.x_points,
                                    line.start.y_points) ||
            !pdfioContentPathLineTo(stream.get(), line.end.x_points,
                                    line.end.y_points) ||
            !pdfioContentStroke(stream.get())) {
            error_message = "Unable to write PDF line";
            return false;
        }
    }

    if (!write_page_images(stream.get(), page, image_resources, true,
                           error_message)) {
        return false;
    }

    for (const auto &text_run : page.text_runs) {
        const auto font_key = make_font_key(text_run);
        const auto font_resource = font_resources.find(font_key);
        if (font_resource == font_resources.end()) {
            error_message =
                "Missing PDF font resource: " + font_label(font_key);
            return false;
        }

        if (!write_actual_text_begin(stream.get(), text_run.text,
                                     error_message)) {
            return false;
        }

        if (!pdfioContentTextBegin(stream.get()) ||
            !pdfioContentSetFillColorDeviceRGB(
                stream.get(), text_run.fill_color.red,
                text_run.fill_color.green, text_run.fill_color.blue) ||
            !pdfioContentSetTextFont(stream.get(),
                                     font_resource->second.c_str(),
                                     text_run.font_size_points) ||
            !set_text_rendering_style(stream.get(), text_run) ||
            !set_text_run_matrix(stream.get(), text_run) ||
            !pdfioContentTextShow(stream.get(), text_run.unicode,
                                  text_run.text.c_str()) ||
            !pdfioContentTextEnd(stream.get())) {
            error_message = "Unable to write PDF text run";
            return false;
        }

        if (!write_actual_text_end(stream.get(), error_message)) {
            return false;
        }

        if (!write_text_underline(stream.get(), text_run, error_message)) {
            return false;
        }
    }

    if (!write_page_images(stream.get(), page, image_resources, false,
                           error_message)) {
        return false;
    }

    if (!pdfioStreamClose(stream.release())) {
        error_message = "Unable to close PDF page stream";
        return false;
    }

    return true;
}

} // namespace

PdfDocumentLayout make_pdfio_probe_layout(const PdfWriterOptions &options) {
    PdfDocumentLayout layout;
    layout.metadata.title = options.title;
    layout.metadata.creator = options.creator;

    PdfPageLayout page;
    page.size = options.page_size;

    const double width = std::max(page.size.width_points, 144.0);
    const double height = std::max(page.size.height_points, 144.0);

    page.rectangles.push_back(PdfRectangle{
        PdfRect{36.0, 36.0, width - 72.0, height - 72.0},
        PdfRgbColor{0.12, 0.16, 0.22},
        PdfRgbColor{1.0, 1.0, 1.0},
        1.0,
        true,
        false,
    });

    page.text_runs.push_back(PdfTextRun{
        PdfPoint{72.0, height - 96.0},
        options.title,
        "Helvetica",
        {},
        18.0,
        PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        false,
    });
    page.text_runs.push_back(PdfTextRun{
        PdfPoint{72.0, height - 120.0},
        "Generated by the FeatherDoc IPdfGenerator/PDFio prototype.",
        "Helvetica",
        {},
        10.0,
        PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        false,
    });

    layout.pages.push_back(std::move(page));
    return layout;
}

PdfWriteResult write_pdfio_document(const std::filesystem::path &output_path,
                                    const PdfDocumentLayout &layout,
                                    const PdfWriterOptions &options) {
    PdfioErrorState error_state;
    PdfWriteResult result;
    if (layout.pages.empty()) {
        result.error_message = "PDF layout does not contain any pages";
        return result;
    }

    ImageSourceCleanup image_cleanup;
    for (const auto &page : layout.pages) {
        for (const auto &image : page.images) {
            if (image.cleanup_source_after_write) {
                image_cleanup.paths.push_back(image.source_path);
            }
        }
    }

    const auto output_string = output_path.string();
    const PdfPageSize first_page_size = layout.pages.front().size;
    pdfio_rect_t media_box{0.0, 0.0, first_page_size.width_points,
                           first_page_size.height_points};

    std::unique_ptr<pdfio_file_t, PdfioFileCloser> pdf(
        pdfioFileCreate(output_string.c_str(), "1.7", &media_box, &media_box,
                        pdfio_error_callback, &error_state));
    if (!pdf) {
        result.error_message =
            build_error("Unable to create PDF file", error_state);
        return result;
    }

    const std::string creator = layout.metadata.creator.empty()
                                    ? options.creator
                                    : layout.metadata.creator;
    const std::string title =
        layout.metadata.title.empty() ? options.title : layout.metadata.title;

    pdfioFileSetCreator(pdf.get(), creator.c_str());
    pdfioFileSetTitle(pdf.get(), title.c_str());

    FontSubsetStorage font_subset_storage;
    for (const auto &page : layout.pages) {
        std::string page_error;
        if (!write_probe_page(pdf.get(), page, options, font_subset_storage,
                              page_error)) {
            result.error_message = build_error(page_error, error_state);
            return result;
        }
    }

    if (!pdfioFileClose(pdf.release())) {
        result.error_message =
            build_error("Unable to close PDF file", error_state);
        return result;
    }

    result.success = true;
    if (std::filesystem::exists(output_path)) {
        result.bytes_written = std::filesystem::file_size(output_path);
    }
    return result;
}

PdfWriteResult PdfioGenerator::write(const PdfDocumentLayout &layout,
                                     const std::filesystem::path &output_path,
                                     const PdfWriterOptions &options) {
    return write_pdfio_document(output_path, layout, options);
}

PdfWriteResult
write_pdfio_probe_document(const std::filesystem::path &output_path,
                           const PdfWriterOptions &options) {
    PdfioGenerator generator;
    return generator.write(make_pdfio_probe_layout(options), output_path,
                           options);
}

} // namespace featherdoc::pdf
