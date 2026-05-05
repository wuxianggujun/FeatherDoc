#include <featherdoc/pdf/pdf_writer.hpp>

#include <algorithm>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <utility>

extern "C" {
#include <pdfio-content.h>
#include <pdfio.h>
}

namespace featherdoc::pdf {
namespace {

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

[[nodiscard]] std::string build_error(std::string fallback,
                                      const PdfioErrorState &state) {
    if (!state.message.empty()) {
        fallback += ": ";
        fallback += state.message;
    }
    return fallback;
}

[[nodiscard]] bool write_probe_page(pdfio_file_t *pdf,
                                    const PdfPageLayout &page,
                                    std::string &error_message) {
    pdfio_dict_t *page_dict = pdfioDictCreate(pdf);
    if (page_dict == nullptr) {
        error_message = "Unable to create PDF page dictionary";
        return false;
    }

    std::map<std::string, std::string> font_resources;
    for (const auto &text_run : page.text_runs) {
        const std::string font_family =
            text_run.font_family.empty() ? "Helvetica" : text_run.font_family;
        if (font_resources.find(font_family) != font_resources.end()) {
            continue;
        }

        const std::string resource_name =
            "F" + std::to_string(font_resources.size() + 1);
        pdfio_obj_t *font =
            pdfioFileCreateFontObjFromBase(pdf, font_family.c_str());
        if (font == nullptr) {
            error_message = "Unable to create base PDF font: " + font_family;
            return false;
        }

        if (!pdfioPageDictAddFont(page_dict, resource_name.c_str(), font)) {
            error_message = "Unable to add PDF font resource: " + font_family;
            return false;
        }

        font_resources.emplace(font_family, resource_name);
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

    for (const auto &text_run : page.text_runs) {
        const std::string font_family =
            text_run.font_family.empty() ? "Helvetica" : text_run.font_family;
        const auto font_resource = font_resources.find(font_family);
        if (font_resource == font_resources.end()) {
            error_message = "Missing PDF font resource: " + font_family;
            return false;
        }

        if (!pdfioContentTextBegin(stream.get()) ||
            !pdfioContentSetFillColorDeviceRGB(
                stream.get(), text_run.fill_color.red,
                text_run.fill_color.green, text_run.fill_color.blue) ||
            !pdfioContentSetTextFont(stream.get(), font_resource->second.c_str(),
                                     text_run.font_size_points) ||
            !pdfioContentTextMoveTo(stream.get(),
                                    text_run.baseline_origin.x_points,
                                    text_run.baseline_origin.y_points) ||
            !pdfioContentTextShow(stream.get(), text_run.unicode,
                                  text_run.text.c_str()) ||
            !pdfioContentTextEnd(stream.get())) {
            error_message = "Unable to write PDF text run";
            return false;
        }
    }

    if (!pdfioStreamClose(stream.release())) {
        error_message = "Unable to close PDF page stream";
        return false;
    }

    return true;
}

}  // namespace

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
        18.0,
        PdfRgbColor{0.0, 0.0, 0.0},
        false,
    });
    page.text_runs.push_back(PdfTextRun{
        PdfPoint{72.0, height - 120.0},
        "Generated by the FeatherDoc IPdfGenerator/PDFio prototype.",
        "Helvetica",
        10.0,
        PdfRgbColor{0.0, 0.0, 0.0},
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

    const auto output_string = output_path.string();
    const PdfPageSize first_page_size = layout.pages.front().size;
    pdfio_rect_t media_box{0.0, 0.0, first_page_size.width_points,
                           first_page_size.height_points};

    std::unique_ptr<pdfio_file_t, PdfioFileCloser> pdf(pdfioFileCreate(
        output_string.c_str(), "1.7", &media_box, &media_box,
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

    for (const auto &page : layout.pages) {
        std::string page_error;
        if (!write_probe_page(pdf.get(), page, page_error)) {
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

PdfWriteResult write_pdfio_probe_document(const std::filesystem::path &output_path,
                                          const PdfWriterOptions &options) {
    PdfioGenerator generator;
    return generator.write(make_pdfio_probe_layout(options), output_path,
                           options);
}

}  // namespace featherdoc::pdf
