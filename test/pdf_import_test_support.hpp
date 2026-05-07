#ifndef FEATHERDOC_TEST_PDF_IMPORT_TEST_SUPPORT_HPP
#define FEATHERDOC_TEST_PDF_IMPORT_TEST_SUPPORT_HPP

#include "doctest.h"

#include <featherdoc.hpp>
#include <featherdoc/pdf/pdf_writer.hpp>

#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace featherdoc::test_support {

[[nodiscard]] inline featherdoc::pdf::PdfTextRun
make_pdf_text_run(double x_points, double y_points, std::string text) {
    return featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{x_points, y_points},
        std::move(text),
        "Helvetica",
        {},
        12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        false,
    };
}

[[nodiscard]] inline featherdoc::pdf::PdfTextRun
make_pdf_text_run(double y_points, std::string text) {
    return make_pdf_text_run(72.0, y_points, std::move(text));
}

[[nodiscard]] inline bool contains_text(const std::string &haystack,
                                        const std::string &needle) {
    return haystack.find(needle) != std::string::npos;
}

[[nodiscard]] inline std::string
collect_document_text(featherdoc::Document &document) {
    std::ostringstream stream;
    for (featherdoc::Paragraph paragraph = document.paragraphs();
         paragraph.has_next(); paragraph.next()) {
        for (featherdoc::Run run = paragraph.runs(); run.has_next();
             run.next()) {
            stream << run.get_text();
        }
        stream << '\n';
    }
    return stream.str();
}

inline void append_three_by_three_grid(featherdoc::pdf::PdfPageLayout &page,
                                       double left,
                                       double top,
                                       double cell_width,
                                       double cell_height) {
    constexpr std::size_t column_count = 3U;
    constexpr std::size_t row_count = 3U;
    const double right = left + cell_width * static_cast<double>(column_count);
    const double bottom = top - cell_height * static_cast<double>(row_count);

    for (std::size_t column = 0U; column <= column_count; ++column) {
        const double x = left + cell_width * static_cast<double>(column);
        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{x, bottom},
            featherdoc::pdf::PdfPoint{x, top},
            featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
            0.75,
        });
    }
    for (std::size_t row = 0U; row <= row_count; ++row) {
        const double y = top - cell_height * static_cast<double>(row);
        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{left, y},
            featherdoc::pdf::PdfPoint{right, y},
            featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
            0.75,
        });
    }
}

[[nodiscard]] inline std::filesystem::path
write_import_structure_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(720.0, "First paragraph line one"));
    page.text_runs.push_back(
        make_pdf_text_run(702.0, "First paragraph line two"));
    page.text_runs.push_back(
        make_pdf_text_run(650.0, "Second paragraph starts"));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_blank_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc blank PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_table_like_grid_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc table-like grid PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_three_by_three_grid(page, 72.0, 700.0, 120.0, 32.0);

    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Grid sample header"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 678.0, "Cell A1"));
    page.text_runs.push_back(make_pdf_text_run(206.0, 646.0, "Cell B2"));
    page.text_runs.push_back(make_pdf_text_run(326.0, 614.0, "Cell C3"));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_table_first_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc table-first PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_three_by_three_grid(page, 72.0, 700.0, 120.0, 32.0);
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 678.0, "Cell A1"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 646.0, "Cell B2"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 614.0, "Cell C3"));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_paragraph_table_paragraph_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Intro paragraph line one"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 706.0, "Intro paragraph line two"));
    append_three_by_three_grid(page, 72.0, 664.0, 120.0, 32.0);
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 642.0, "Cell A1"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 610.0, "Cell B2"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 578.0, "Cell C3"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 522.0, "Tail paragraph after table"));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_paragraph_table_table_paragraph_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Intro paragraph before tables"));
    append_three_by_three_grid(page, 72.0, 664.0, 120.0, 32.0);
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 642.0, "First table A1"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 610.0, "First table B2"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 578.0, "First table C3"));
    append_three_by_three_grid(page, 72.0, 520.0, 120.0, 32.0);
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 498.0, "Second table A1"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 466.0, "Second table B2"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 434.0, "Second table C3"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 360.0, "Tail paragraph after tables"));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_two_column_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc two-column PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{306.0, 608.0},
        featherdoc::pdf::PdfPoint{306.0, 704.0},
        featherdoc::pdf::PdfRgbColor{0.82, 0.84, 0.88},
        0.75,
    });
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Two column sample"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 684.0, "Left column alpha"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 660.0, "Left column beta"));
    page.text_runs.push_back(
        make_pdf_text_run(330.0, 684.0, "Right column alpha"));
    page.text_runs.push_back(
        make_pdf_text_run(330.0, 660.0, "Right column beta"));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_aligned_list_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc aligned list PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Aligned list sample"));
    page.text_runs.push_back(make_pdf_text_run(72.0, 684.0, "1."));
    page.text_runs.push_back(
        make_pdf_text_run(112.0, 684.0, "First action item"));
    page.text_runs.push_back(make_pdf_text_run(72.0, 660.0, "2."));
    page.text_runs.push_back(
        make_pdf_text_run(112.0, 660.0, "Second action item"));
    page.text_runs.push_back(make_pdf_text_run(72.0, 636.0, "3."));
    page.text_runs.push_back(
        make_pdf_text_run(112.0, 636.0, "Third action item"));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

}  // namespace featherdoc::test_support

#endif  // FEATHERDOC_TEST_PDF_IMPORT_TEST_SUPPORT_HPP
