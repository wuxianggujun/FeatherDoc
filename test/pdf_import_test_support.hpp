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
#include <vector>

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

inline void append_grid(featherdoc::pdf::PdfPageLayout &page, double left,
                        double top, double cell_width, double cell_height,
                        std::size_t column_count, std::size_t row_count) {
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

inline void append_irregular_grid(featherdoc::pdf::PdfPageLayout &page,
                                  double left, double top,
                                  const std::vector<double> &column_widths,
                                  double cell_height,
                                  std::size_t row_count) {
    double right = left;
    for (const double column_width : column_widths) {
        right += column_width;
    }
    const double bottom = top - cell_height * static_cast<double>(row_count);

    double x = left;
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{x, bottom},
        featherdoc::pdf::PdfPoint{x, top},
        featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
        0.75,
    });
    for (const double column_width : column_widths) {
        x += column_width;
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

inline void append_three_by_three_grid(featherdoc::pdf::PdfPageLayout &page,
                                       double left, double top,
                                       double cell_width,
                                       double cell_height) {
    append_grid(page, left, top, cell_width, cell_height, 3U, 3U);
}

inline void append_three_by_three_grid_with_column_row_merge(
    featherdoc::pdf::PdfPageLayout &page, double left, double top,
    double cell_width, double cell_height, std::size_t merged_column_index,
    std::size_t merged_row_count) {
    REQUIRE(merged_column_index < 3U);
    REQUIRE(merged_row_count >= 2U);
    REQUIRE(merged_row_count <= 3U);

    const double right = left + cell_width * 3.0;
    const double bottom = top - cell_height * 3.0;
    const double merge_left =
        left + cell_width * static_cast<double>(merged_column_index);
    const double merge_right = merge_left + cell_width;

    for (std::size_t column = 0U; column <= 3U; ++column) {
        const double x = left + cell_width * static_cast<double>(column);
        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{x, bottom},
            featherdoc::pdf::PdfPoint{x, top},
            featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
            0.75,
        });
    }

    for (std::size_t row = 0U; row <= 3U; ++row) {
        const double y = top - cell_height * static_cast<double>(row);
        if (row > 0U && row < merged_row_count) {
            if (merged_column_index > 0U) {
                page.lines.push_back(featherdoc::pdf::PdfLine{
                    featherdoc::pdf::PdfPoint{left, y},
                    featherdoc::pdf::PdfPoint{merge_left, y},
                    featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
                    0.75,
                });
            }
            if (merged_column_index + 1U < 3U) {
                page.lines.push_back(featherdoc::pdf::PdfLine{
                    featherdoc::pdf::PdfPoint{merge_right, y},
                    featherdoc::pdf::PdfPoint{right, y},
                    featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
                    0.75,
                });
            }
            continue;
        }

        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{left, y},
            featherdoc::pdf::PdfPoint{right, y},
            featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
            0.75,
        });
    }
}

inline void append_three_by_three_grid_with_first_column_row_merge(
    featherdoc::pdf::PdfPageLayout &page, double left, double top,
    double cell_width, double cell_height) {
    append_three_by_three_grid_with_column_row_merge(
        page, left, top, cell_width, cell_height, 0U, 2U);
}

inline void append_three_by_three_grid_with_middle_column_row_merge(
    featherdoc::pdf::PdfPageLayout &page, double left, double top,
    double cell_width, double cell_height) {
    append_three_by_three_grid_with_column_row_merge(
        page, left, top, cell_width, cell_height, 1U, 3U);
}

inline void append_four_by_three_grid_with_top_left_two_by_two_merge(
    featherdoc::pdf::PdfPageLayout &page, double left, double top,
    double cell_width, double cell_height) {
    const double right = left + cell_width * 4.0;
    const double bottom = top - cell_height * 3.0;
    const double merge_split_x = left + cell_width * 2.0;
    const double merge_split_y = top - cell_height;
    const double merge_bottom_y = top - cell_height * 2.0;

    for (std::size_t column = 0U; column <= 4U; ++column) {
        const double x = left + cell_width * static_cast<double>(column);
        if (column == 1U) {
            page.lines.push_back(featherdoc::pdf::PdfLine{
                featherdoc::pdf::PdfPoint{x, bottom},
                featherdoc::pdf::PdfPoint{x, merge_bottom_y},
                featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
                0.75,
            });
            continue;
        }

        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{x, bottom},
            featherdoc::pdf::PdfPoint{x, top},
            featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
            0.75,
        });
    }

    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{left, top},
        featherdoc::pdf::PdfPoint{right, top},
        featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
        0.75,
    });
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{merge_split_x, merge_split_y},
        featherdoc::pdf::PdfPoint{right, merge_split_y},
        featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
        0.75,
    });
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{left, merge_bottom_y},
        featherdoc::pdf::PdfPoint{right, merge_bottom_y},
        featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
        0.75,
    });
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{left, bottom},
        featherdoc::pdf::PdfPoint{right, bottom},
        featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
        0.75,
    });
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
write_two_row_header_data_table_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc two-row header-data table PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Two-row table sample"));
    append_grid(page, 72.0, 680.0, 132.0, 32.0, 3U, 2U);
    page.text_runs.push_back(make_pdf_text_run(86.0, 658.0, "Item"));
    page.text_runs.push_back(make_pdf_text_run(218.0, 658.0, "Owner"));
    page.text_runs.push_back(make_pdf_text_run(350.0, 658.0, "Due"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 626.0, "INV-2026-05"));
    page.text_runs.push_back(make_pdf_text_run(218.0, 626.0, "QA Team"));
    page.text_runs.push_back(make_pdf_text_run(350.0, 626.0, "2026-05-14"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 570.0, "Tail paragraph after two-row table"));
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
write_two_column_key_value_table_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc two-column key-value table PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Key-value table sample"));
    append_grid(page, 72.0, 680.0, 180.0, 30.0, 2U, 4U);
    page.text_runs.push_back(make_pdf_text_run(86.0, 660.0, "Invoice No"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 660.0, "INV-2026-0514"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 630.0, "Customer"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 630.0, "FeatherDoc QA"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 600.0, "Due Date"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 600.0, "2026-05-14"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 570.0, "Total"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 570.0, "USD 480"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 518.0, "Tail paragraph after key-value table"));
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
write_two_column_borderless_key_value_table_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc borderless key-value table PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Borderless key-value sample"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 660.0, "Invoice No"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 660.0, "INV-2026-0610"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 630.0, "Customer"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 630.0, "FeatherDoc Ops"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 600.0, "Due Date"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 600.0, "2026-06-10"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 570.0, "Amount"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 570.0, "USD 960"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 518.0, "Tail paragraph after borderless key-value table"));
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
write_irregular_width_header_table_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc irregular-width table PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Irregular width table sample"));
    append_irregular_grid(page, 72.0, 680.0, {92.0, 210.0, 96.0}, 30.0, 4U);
    page.text_runs.push_back(make_pdf_text_run(86.0, 660.0, "Item"));
    page.text_runs.push_back(make_pdf_text_run(178.0, 660.0, "Description"));
    page.text_runs.push_back(make_pdf_text_run(388.0, 660.0, "Amount"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 630.0, "SKU-01"));
    page.text_runs.push_back(make_pdf_text_run(178.0, 630.0, "Annual subscription"));
    page.text_runs.push_back(make_pdf_text_run(388.0, 630.0, "USD 700"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 600.0, "SKU-02"));
    page.text_runs.push_back(make_pdf_text_run(178.0, 600.0, "Visual support"));
    page.text_runs.push_back(make_pdf_text_run(388.0, 600.0, "USD 180"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 570.0, "Total"));
    page.text_runs.push_back(make_pdf_text_run(178.0, 570.0, "Invoice 2026"));
    page.text_runs.push_back(make_pdf_text_run(388.0, 570.0, "USD 880"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 518.0, "Tail paragraph after irregular width table"));
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
write_two_column_short_label_prose_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc two-column short-label prose PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Two-column labels sample"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 678.0, "Topic"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 678.0, "Scope"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 646.0, "Owner"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 646.0, "Review"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 614.0, "Status"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 614.0, "Closed"));
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
write_two_row_three_column_prose_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc two-row three-column prose PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Two-row prose sample"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 678.0, "Step 1"));
    page.text_runs.push_back(make_pdf_text_run(218.0, 678.0, "Build 2"));
    page.text_runs.push_back(make_pdf_text_run(350.0, 678.0, "Ship 3"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 646.0, "Alpha note"));
    page.text_runs.push_back(make_pdf_text_run(218.0, 646.0, "Beta note"));
    page.text_runs.push_back(make_pdf_text_run(350.0, 646.0, "Gamma note"));
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
write_paragraph_merged_header_table_paragraph_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-merged-header-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Intro paragraph before merged table"));
    append_three_by_three_grid(page, 72.0, 664.0, 120.0, 32.0);
    // 第一行左侧故意放一条明显宽于单列的文本簇，回归最小横向合并导入。
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 642.0, "Merged header spans two cols"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 642.0, "Status"));
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 610.0, "Design"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 610.0, "Alice"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 610.0, "Open"));
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 578.0, "Review"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 578.0, "Bob"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 578.0, "Done"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 522.0, "Tail paragraph after merged table"));
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
write_paragraph_vertical_merged_table_paragraph_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-vertical-merged-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Intro paragraph before vertical merged table"));
    append_three_by_three_grid_with_first_column_row_merge(
        page, 72.0, 664.0, 120.0, 32.0);
    // 第一列顶部单元格视觉上跨两行，回归最小纵向合并导入。
    page.text_runs.push_back(make_pdf_text_run(86.0, 642.0, "Project"));
    page.text_runs.push_back(make_pdf_text_run(206.0, 642.0, "Owner"));
    page.text_runs.push_back(make_pdf_text_run(326.0, 642.0, "Status"));
    page.text_runs.push_back(make_pdf_text_run(206.0, 610.0, "Alpha"));
    page.text_runs.push_back(make_pdf_text_run(326.0, 610.0, "Open"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 578.0, "Milestone"));
    page.text_runs.push_back(make_pdf_text_run(206.0, 578.0, "Beta"));
    page.text_runs.push_back(make_pdf_text_run(326.0, 578.0, "Done"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 522.0, "Tail paragraph after vertical merged table"));
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
write_paragraph_middle_column_merged_table_paragraph_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-middle-column-merged-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Intro paragraph before vertical merged table"));
    append_three_by_three_grid_with_middle_column_row_merge(
        page, 72.0, 664.0, 120.0, 32.0);
    // 中间列顶部单元格视觉上跨三行，回归更通用的纵向合并导入。
    page.text_runs.push_back(make_pdf_text_run(86.0, 642.0, "Project"));
    page.text_runs.push_back(make_pdf_text_run(206.0, 642.0, "Owner"));
    page.text_runs.push_back(make_pdf_text_run(326.0, 642.0, "Status"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 610.0, "Alpha"));
    page.text_runs.push_back(make_pdf_text_run(326.0, 610.0, "Open"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 578.0, "Milestone"));
    page.text_runs.push_back(make_pdf_text_run(326.0, 578.0, "Done"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 522.0, "Tail paragraph after vertical merged table"));
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
write_paragraph_merged_corner_table_paragraph_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-merged-corner-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Intro paragraph before merged corner table"));
    append_four_by_three_grid_with_top_left_two_by_two_merge(
        page, 72.0, 664.0, 130.0, 32.0);
    // 左上角单元格视觉上跨两列两行，回归组合合并导入。
    page.text_runs.push_back(make_pdf_text_run(
        86.0, 642.0, "Project overview and owner assignment"));
    page.text_runs.push_back(make_pdf_text_run(346.0, 642.0, "Status"));
    page.text_runs.push_back(make_pdf_text_run(476.0, 642.0, "Phase"));
    page.text_runs.push_back(make_pdf_text_run(346.0, 610.0, "Alpha"));
    page.text_runs.push_back(make_pdf_text_run(476.0, 610.0, "Open"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 578.0, "Milestone"));
    page.text_runs.push_back(make_pdf_text_run(216.0, 578.0, "Beta"));
    page.text_runs.push_back(make_pdf_text_run(346.0, 578.0, "Done"));
    page.text_runs.push_back(make_pdf_text_run(476.0, 578.0, "Closed"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 522.0, "Tail paragraph after merged corner table"));
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
write_out_of_order_paragraph_table_paragraph_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc out-of-order paragraph-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    // Visual order is still paragraph -> table -> paragraph, but the text
    // objects are appended in reverse to verify geometry-based ordering.
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 522.0, "Tail paragraph after table"));
    append_three_by_three_grid(page, 72.0, 664.0, 120.0, 32.0);
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 642.0, "Cell A1"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 610.0, "Cell B2"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 578.0, "Cell C3"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Intro paragraph line one"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 706.0, "Intro paragraph line two"));
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
write_table_title_paragraph_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc table-title paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Table 1. Imported table title"));
    append_three_by_three_grid(page, 72.0, 664.0, 120.0, 32.0);
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 642.0, "Table title A1"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 610.0, "Table title B2"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 578.0, "Table title C3"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 522.0, "Tail paragraph after table title"));
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
write_paragraph_table_touching_note_paragraph_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-touching-note-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Intro paragraph before touching note"));
    append_three_by_three_grid(page, 72.0, 664.0, 120.0, 32.0);
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 642.0, "Touching note A1"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 610.0, "Touching note B2"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 578.0, "Touching note C3"));
    // 视觉上这是表格后的近邻段落，但故意压到接近表格下沿，回归 importer
    // 不应仅因 bounds 轻微相交就把它吞掉。
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 554.0, "Note paragraph touching table border"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 520.0, "Tail paragraph after touching note"));
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
write_paragraph_table_touching_multiline_note_paragraph_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-touching-multiline-note import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Intro paragraph before touching multi-line note"));
    append_three_by_three_grid(page, 72.0, 664.0, 120.0, 32.0);
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 642.0, "Touching multi-line A1"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 610.0, "Touching multi-line B2"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 578.0, "Touching multi-line C3"));
    // 第一行贴近表格下沿，第二行已经落在表格外部；两行应继续被 parser
    // 聚成同一个段落，且 importer 不应仅因其中一行接近表格就吞掉整段。
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 560.0, "Note paragraph line one near table border"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 542.0, "Note paragraph line two stays outside table"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 502.0, "Tail paragraph after touching multi-line note"));
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
write_paragraph_table_partial_overlap_multiline_note_paragraph_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-partial-overlap-multiline-note import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Intro paragraph before partial-overlap note"));
    append_three_by_three_grid(page, 72.0, 664.0, 120.0, 32.0);
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 642.0, "Partial overlap A1"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 610.0, "Partial overlap B2"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 578.0, "Partial overlap C3"));
    // 第一行轻微压到表格下沿，第二行已经完全落在表格外部；
    // 两行应仍然被 parser 聚成同一个段落，而不是和表格行混成一行。
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 566.0, "Partial overlap note line one near table border"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 548.0, "Partial overlap note line two stays outside table"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 508.0, "Tail paragraph after partial-overlap note"));
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
write_paragraph_table_note_table_paragraph_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-note-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Intro paragraph before same-page tables"));
    append_three_by_three_grid(page, 72.0, 664.0, 120.0, 32.0);
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 642.0, "First table A1"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 610.0, "First table B2"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 578.0, "First table C3"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 532.0, "Note paragraph between tables"));
    append_three_by_three_grid(page, 72.0, 492.0, 120.0, 32.0);
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 470.0, "Second table A1"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 438.0, "Second table B2"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 406.0, "Second table C3"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 340.0, "Tail paragraph after same-page tables"));
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
write_paragraph_table_pagebreak_table_paragraph_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-pagebreak-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Intro paragraph before page break"));
    append_three_by_three_grid(first_page, 72.0, 664.0, 120.0, 32.0);
    first_page.text_runs.push_back(
        make_pdf_text_run(86.0, 642.0, "Page one table A1"));
    first_page.text_runs.push_back(
        make_pdf_text_run(206.0, 610.0, "Page one table B2"));
    first_page.text_runs.push_back(
        make_pdf_text_run(326.0, 578.0, "Page one table C3"));
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_three_by_three_grid(second_page, 72.0, 700.0, 120.0, 32.0);
    second_page.text_runs.push_back(
        make_pdf_text_run(86.0, 678.0, "Page two table A1"));
    second_page.text_runs.push_back(
        make_pdf_text_run(206.0, 646.0, "Page two table B2"));
    second_page.text_runs.push_back(
        make_pdf_text_run(326.0, 614.0, "Page two table C3"));
    second_page.text_runs.push_back(
        make_pdf_text_run(72.0, 548.0, "Tail paragraph after page break"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_paragraph_table_pagebreak_wide_table_paragraph_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-pagebreak-wide-table import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Intro paragraph before wide table"));
    append_three_by_three_grid(first_page, 72.0, 664.0, 120.0, 32.0);
    first_page.text_runs.push_back(
        make_pdf_text_run(86.0, 642.0, "Narrow page A1"));
    first_page.text_runs.push_back(
        make_pdf_text_run(206.0, 610.0, "Narrow page B2"));
    first_page.text_runs.push_back(
        make_pdf_text_run(326.0, 578.0, "Narrow page C3"));
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_three_by_three_grid(second_page, 72.0, 700.0, 150.0, 32.0);
    second_page.text_runs.push_back(
        make_pdf_text_run(86.0, 678.0, "Wide page A1"));
    second_page.text_runs.push_back(
        make_pdf_text_run(236.0, 646.0, "Wide page B2"));
    second_page.text_runs.push_back(
        make_pdf_text_run(386.0, 614.0, "Wide page C3"));
    second_page.text_runs.push_back(
        make_pdf_text_run(72.0, 548.0, "Tail paragraph after wide table"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_paragraph_table_pagebreak_caption_table_paragraph_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-pagebreak-caption-table import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Intro paragraph before caption page"));
    append_three_by_three_grid(first_page, 72.0, 664.0, 120.0, 32.0);
    first_page.text_runs.push_back(
        make_pdf_text_run(86.0, 642.0, "Caption first table A1"));
    first_page.text_runs.push_back(
        make_pdf_text_run(206.0, 610.0, "Caption first table B2"));
    first_page.text_runs.push_back(
        make_pdf_text_run(326.0, 578.0, "Caption first table C3"));
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    second_page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Continuation note before table"));
    append_three_by_three_grid(second_page, 72.0, 680.0, 120.0, 32.0);
    second_page.text_runs.push_back(
        make_pdf_text_run(86.0, 658.0, "Caption second table A1"));
    second_page.text_runs.push_back(
        make_pdf_text_run(206.0, 626.0, "Caption second table B2"));
    second_page.text_runs.push_back(
        make_pdf_text_run(326.0, 594.0, "Caption second table C3"));
    second_page.text_runs.push_back(
        make_pdf_text_run(72.0, 520.0, "Tail paragraph after caption table"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
    generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_paragraph_table_pagebreak_low_table_paragraph_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-pagebreak-low-table import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Intro paragraph before low table"));
    append_three_by_three_grid(first_page, 72.0, 664.0, 120.0, 32.0);
    first_page.text_runs.push_back(
        make_pdf_text_run(86.0, 642.0, "Low first page A1"));
    first_page.text_runs.push_back(
        make_pdf_text_run(206.0, 610.0, "Low first page B2"));
    first_page.text_runs.push_back(
        make_pdf_text_run(326.0, 578.0, "Low first page C3"));
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_three_by_three_grid(second_page, 72.0, 560.0, 120.0, 32.0);
    second_page.text_runs.push_back(
        make_pdf_text_run(86.0, 538.0, "Low second page A1"));
    second_page.text_runs.push_back(
        make_pdf_text_run(206.0, 506.0, "Low second page B2"));
    second_page.text_runs.push_back(
        make_pdf_text_run(326.0, 474.0, "Low second page C3"));
    second_page.text_runs.push_back(
        make_pdf_text_run(72.0, 420.0, "Tail paragraph after low table"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_invoice_grid_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc invoice grid PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Invoice grid sample"));
    append_grid(page, 72.0, 700.0, 130.0, 28.0, 4U, 5U);
    page.text_runs.push_back(make_pdf_text_run(86.0, 678.0, "Item"));
    page.text_runs.push_back(make_pdf_text_run(230.0, 678.0, "Qty"));
    page.text_runs.push_back(make_pdf_text_run(364.0, 678.0, "Unit"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 678.0, "Total"));
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 650.0, "PDF export design"));
    page.text_runs.push_back(make_pdf_text_run(230.0, 650.0, "2"));
    page.text_runs.push_back(make_pdf_text_run(364.0, 650.0, "USD 50"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 650.0, "USD 100"));
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 622.0, "Visual validation"));
    page.text_runs.push_back(make_pdf_text_run(230.0, 622.0, "1"));
    page.text_runs.push_back(make_pdf_text_run(364.0, 622.0, "USD 25"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 622.0, "USD 25"));
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 594.0, "Regression evidence"));
    page.text_runs.push_back(make_pdf_text_run(230.0, 594.0, "1"));
    page.text_runs.push_back(make_pdf_text_run(364.0, 594.0, "USD 10"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 594.0, "USD 10"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 566.0, "Grand total"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 566.0, "USD 135"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 520.0, "Footer note: invoice grid is intentionally regular"));
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
write_invoice_grid_repeat_header_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc invoice grid repeat-header PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_invoice_row = [](featherdoc::pdf::PdfPageLayout &page,
                              double y_points, std::string item,
                              std::string qty, std::string unit,
                              std::string total) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(item)));
        page.text_runs.push_back(
            make_pdf_text_run(230.0, y_points, std::move(qty)));
        page.text_runs.push_back(
            make_pdf_text_run(364.0, y_points, std::move(unit)));
        page.text_runs.push_back(
            make_pdf_text_run(498.0, y_points, std::move(total)));
    };

    auto add_invoice_page = [&](featherdoc::pdf::PdfPageLayout &page,
                                bool with_header, std::size_t start_index,
                                std::size_t row_count) {
        page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
        append_grid(page, 72.0, 700.0, 130.0, 28.0, 4U, row_count);

        if (with_header) {
            add_invoice_row(page, 678.0, "Item", "Qty", "Unit", "Total");
        }

        for (std::size_t row_offset = 0U; row_offset < row_count - (with_header ? 1U : 0U);
             ++row_offset) {
            const auto row_number = start_index + row_offset + 1U;
            const auto y_points = with_header ? 650.0 - 28.0 * static_cast<double>(row_offset)
                                              : 678.0 - 28.0 * static_cast<double>(row_offset);
            const auto item = "PDF export line item " + std::to_string(row_number);
            const auto qty = std::to_string(1U + (row_number % 3U));
            const auto unit = "USD " + std::to_string(40U + row_number * 3U);
            const auto total = "USD " +
                               std::to_string((40U + row_number * 3U) *
                                              (1U + (row_number % 3U)));
            add_invoice_row(page, y_points, item, qty, unit, total);
        }
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Invoice repeat-header sample"));
    add_invoice_page(first_page, true, 0U, 14U);
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    add_invoice_page(second_page, false, 13U, 14U);
    layout.pages.push_back(std::move(second_page));

    featherdoc::pdf::PdfPageLayout third_page;
    add_invoice_page(third_page, false, 27U, 14U);
    layout.pages.push_back(std::move(third_page));

    featherdoc::pdf::PdfPageLayout fourth_page;
    add_invoice_page(fourth_page, false, 41U, 14U);
    fourth_page.text_runs.push_back(
        make_pdf_text_run(72.0, 240.0, "Footer note: repeated header continues"));
    layout.pages.push_back(std::move(fourth_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_paragraph_table_pagebreak_repeated_header_table_paragraph_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-pagebreak-repeated-header-table import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_table_row = [](featherdoc::pdf::PdfPageLayout &page, double y_points,
                            std::string first, std::string second,
                            std::string third) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(first)));
        page.text_runs.push_back(
            make_pdf_text_run(246.0, y_points, std::move(second)));
        page.text_runs.push_back(
            make_pdf_text_run(406.0, y_points, std::move(third)));
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Repeated header source sample"));
    append_three_by_three_grid(first_page, 72.0, 664.0, 160.0, 32.0);
    add_table_row(first_page, 642.0, "Item", "Owner", "Status");
    add_table_row(first_page, 610.0, "Feature alpha", "Design crew", "Shipped");
    add_table_row(first_page, 578.0, "Metrics review", "QA group", "Approved");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_three_by_three_grid(second_page, 72.0, 700.0, 160.0, 32.0);
    add_table_row(second_page, 678.0, "Item", "Owner", "Status");
    add_table_row(second_page, 646.0, "Invoice merge", "Import lane", "Tracked");
    add_table_row(second_page, 614.0, "Header dedupe", "Parser team", "Verified");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 548.0, "Footer note: repeated source header should merge cleanly"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
    generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_paragraph_table_pagebreak_merged_repeated_header_table_paragraph_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-pagebreak-merged-repeated-header-table import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_table_row = [](featherdoc::pdf::PdfPageLayout &page, double y_points,
                            std::string first, std::string second,
                            std::string third) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(first)));
        page.text_runs.push_back(
            make_pdf_text_run(206.0, y_points, std::move(second)));
        page.text_runs.push_back(
            make_pdf_text_run(326.0, y_points, std::move(third)));
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Merged repeated header source sample"));
    append_three_by_three_grid(first_page, 72.0, 664.0, 120.0, 32.0);
    first_page.text_runs.push_back(
        make_pdf_text_run(86.0, 642.0, "Merged header spans two cols"));
    first_page.text_runs.push_back(
        make_pdf_text_run(326.0, 642.0, "Status"));
    add_table_row(first_page, 610.0, "Feature alpha", "Design crew", "Shipped");
    add_table_row(first_page, 578.0, "Metrics review", "QA group", "Approved");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_three_by_three_grid(second_page, 72.0, 700.0, 120.0, 32.0);
    second_page.text_runs.push_back(
        make_pdf_text_run(86.0, 678.0, "Merged header spans two cols"));
    second_page.text_runs.push_back(
        make_pdf_text_run(326.0, 678.0, "Status"));
    add_table_row(second_page, 646.0, "Invoice merge", "Import lane", "Tracked");
    add_table_row(second_page, 614.0, "Header dedupe", "Parser team", "Verified");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 548.0,
        "Footer note: merged repeated header should merge cleanly"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_paragraph_table_pagebreak_repeated_header_whitespace_variant_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-pagebreak-repeated-header-whitespace-variant import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_table_row = [](featherdoc::pdf::PdfPageLayout &page, double y_points,
                            std::string first, std::string second,
                            std::string third) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(first)));
        page.text_runs.push_back(
            make_pdf_text_run(246.0, y_points, std::move(second)));
        page.text_runs.push_back(
            make_pdf_text_run(406.0, y_points, std::move(third)));
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Repeated header whitespace variant sample"));
    append_three_by_three_grid(first_page, 72.0, 664.0, 160.0, 32.0);
    add_table_row(first_page, 642.0, "Item", "Owner Name", "Project Status");
    add_table_row(first_page, 610.0, "Feature alpha rollout",
                  "Design crew west", "Shipment cleared");
    add_table_row(first_page, 578.0, "Metrics review sync",
                  "QA operations lane", "Approval recorded");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_three_by_three_grid(second_page, 72.0, 700.0, 160.0, 32.0);
    add_table_row(second_page, 678.0, "Item", "Owner  Name",
                  "Project   Status");
    add_table_row(second_page, 646.0, "Invoice merge pass",
                  "Import lane west", "Tracking verified");
    add_table_row(second_page, 614.0, "Header dedupe audit",
                  "Parser team shift", "Verification closed");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 548.0,
        "Footer note: repeated source header whitespace should merge cleanly"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_paragraph_table_pagebreak_repeated_header_text_variant_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-pagebreak-repeated-header-text-variant import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_table_row = [](featherdoc::pdf::PdfPageLayout &page, double y_points,
                            std::string first, std::string second,
                            std::string third) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(first)));
        page.text_runs.push_back(
            make_pdf_text_run(246.0, y_points, std::move(second)));
        page.text_runs.push_back(
            make_pdf_text_run(406.0, y_points, std::move(third)));
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Repeated header text variant sample"));
    append_three_by_three_grid(first_page, 72.0, 664.0, 160.0, 32.0);
    add_table_row(first_page, 642.0, "Item", "Owner Name", "Project Status");
    add_table_row(first_page, 610.0, "Feature alpha rollout",
                  "Design crew west", "Shipment cleared");
    add_table_row(first_page, 578.0, "Metrics review sync",
                  "QA operations lane", "Approval recorded");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_three_by_three_grid(second_page, 72.0, 700.0, 160.0, 32.0);
    add_table_row(second_page, 678.0, "item", "owner-name",
                  "project/status");
    add_table_row(second_page, 646.0, "Invoice merge pass",
                  "Import lane west", "Tracking verified");
    add_table_row(second_page, 614.0, "Header dedupe audit",
                  "Parser team shift", "Verification closed");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 548.0,
        "Footer note: repeated source header text variant should merge cleanly"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_paragraph_table_pagebreak_repeated_header_plural_variant_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-pagebreak-repeated-header-plural-variant import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_table_row = [](featherdoc::pdf::PdfPageLayout &page, double y_points,
                            std::string first, std::string second,
                            std::string third) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(first)));
        page.text_runs.push_back(
            make_pdf_text_run(246.0, y_points, std::move(second)));
        page.text_runs.push_back(
            make_pdf_text_run(406.0, y_points, std::move(third)));
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Repeated header plural variant sample"));
    append_three_by_three_grid(first_page, 72.0, 664.0, 160.0, 32.0);
    add_table_row(first_page, 642.0, "Item", "Owner Name", "Project Status");
    add_table_row(first_page, 610.0, "Feature alpha rollout",
                  "Design crew west", "Shipment cleared");
    add_table_row(first_page, 578.0, "Metrics review sync",
                  "QA operations lane", "Approval recorded");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_three_by_three_grid(second_page, 72.0, 700.0, 160.0, 32.0);
    add_table_row(second_page, 678.0, "Items", "Owner Names",
                  "Project Statuses");
    add_table_row(second_page, 646.0, "Invoice merge pass",
                  "Import lane west", "Tracking verified");
    add_table_row(second_page, 614.0, "Header dedupe audit",
                  "Parser team shift", "Verification closed");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 548.0,
        "Footer note: repeated source header plural variant should merge cleanly"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_paragraph_table_pagebreak_repeated_header_abbreviation_variant_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-pagebreak-repeated-header-abbreviation-variant import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_table_row = [](featherdoc::pdf::PdfPageLayout &page,
                            double y_points, std::string first,
                            std::string second, std::string third) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(first)));
        page.text_runs.push_back(
            make_pdf_text_run(246.0, y_points, std::move(second)));
        page.text_runs.push_back(
            make_pdf_text_run(406.0, y_points, std::move(third)));
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Repeated header abbreviation variant sample"));
    append_three_by_three_grid(first_page, 72.0, 664.0, 160.0, 32.0);
    add_table_row(first_page, 642.0, "Item", "Quantity", "Amount");
    add_table_row(first_page, 610.0, "Feature alpha rollout",
                  "12 units requested", "USD 120 approved");
    add_table_row(first_page, 578.0, "Metrics review sync",
                  "8 units validated", "USD 80 approved");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_three_by_three_grid(second_page, 72.0, 700.0, 160.0, 32.0);
    add_table_row(second_page, 678.0, "Item", "Qty", "Amt");
    add_table_row(second_page, 646.0, "Invoice merge pass",
                  "6 units shipped", "USD 60 tracked");
    add_table_row(second_page, 614.0, "Header dedupe audit",
                  "4 units reviewed", "USD 40 closed");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 548.0,
        "Footer note: repeated source header abbreviation variant should merge cleanly"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_paragraph_table_pagebreak_repeated_header_word_order_variant_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-pagebreak-repeated-header-word-order-variant import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_table_row = [](featherdoc::pdf::PdfPageLayout &page,
                            double y_points, std::string first,
                            std::string second, std::string third) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(first)));
        page.text_runs.push_back(
            make_pdf_text_run(246.0, y_points, std::move(second)));
        page.text_runs.push_back(
            make_pdf_text_run(406.0, y_points, std::move(third)));
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Repeated header word order variant sample"));
    append_three_by_three_grid(first_page, 72.0, 664.0, 160.0, 32.0);
    add_table_row(first_page, 642.0, "Item", "Owner Name", "Project Status");
    add_table_row(first_page, 610.0, "Feature alpha rollout",
                  "Design crew west", "Shipment cleared");
    add_table_row(first_page, 578.0, "Metrics review sync",
                  "QA operations lane", "Approval recorded");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_three_by_three_grid(second_page, 72.0, 700.0, 160.0, 32.0);
    add_table_row(second_page, 678.0, "Item", "Name Owner", "Status Project");
    add_table_row(second_page, 646.0, "Invoice merge pass",
                  "Import lane west", "Tracking verified");
    add_table_row(second_page, 614.0, "Header dedupe audit",
                  "Parser team shift", "Verification closed");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 548.0,
        "Footer note: repeated source header word order variant should merge cleanly"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_paragraph_table_pagebreak_repeated_header_unit_variant_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-pagebreak-repeated-header-unit-variant import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_table_row = [](featherdoc::pdf::PdfPageLayout &page,
                            double y_points, std::string first,
                            std::string second, std::string third) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(first)));
        page.text_runs.push_back(
            make_pdf_text_run(246.0, y_points, std::move(second)));
        page.text_runs.push_back(
            make_pdf_text_run(406.0, y_points, std::move(third)));
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Repeated header unit variant sample"));
    append_three_by_three_grid(first_page, 72.0, 664.0, 160.0, 32.0);
    add_table_row(first_page, 642.0, "Item", "Owner Name", "Amount USD");
    add_table_row(first_page, 610.0, "Feature alpha rollout",
                  "Design crew west", "USD 120 approved");
    add_table_row(first_page, 578.0, "Metrics review sync",
                  "QA operations lane", "USD 80 approved");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_three_by_three_grid(second_page, 72.0, 700.0, 160.0, 32.0);
    add_table_row(second_page, 678.0, "Item", "Owner, Name", "Amount (USD)");
    add_table_row(second_page, 646.0, "Invoice merge pass",
                  "Import lane west", "USD 60 tracked");
    add_table_row(second_page, 614.0, "Header dedupe audit",
                  "Parser team shift", "USD 40 closed");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 548.0,
        "Footer note: repeated source header unit variant should merge cleanly"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_paragraph_table_pagebreak_repeated_header_semantic_variant_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-pagebreak-repeated-header-semantic-variant import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_table_row = [](featherdoc::pdf::PdfPageLayout &page, double y_points,
                            std::string first, std::string second,
                            std::string third) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(first)));
        page.text_runs.push_back(
            make_pdf_text_run(246.0, y_points, std::move(second)));
        page.text_runs.push_back(
            make_pdf_text_run(406.0, y_points, std::move(third)));
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Repeated header semantic variant sample"));
    append_three_by_three_grid(first_page, 72.0, 664.0, 160.0, 32.0);
    add_table_row(first_page, 642.0, "Item", "Owner Name", "Project Status");
    add_table_row(first_page, 610.0, "Feature alpha rollout",
                  "Design crew west", "Shipment cleared");
    add_table_row(first_page, 578.0, "Metrics review sync",
                  "QA operations lane", "Approval recorded");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_three_by_three_grid(second_page, 72.0, 700.0, 160.0, 32.0);
    add_table_row(second_page, 678.0, "Item", "Owner Team", "Project State");
    add_table_row(second_page, 646.0, "Invoice merge pass",
                  "Import lane west", "Tracking verified");
    add_table_row(second_page, 614.0, "Header dedupe audit",
                  "Parser team shift", "Verification closed");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 548.0,
        "Footer note: repeated source header semantic variant should keep a separate table"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_long_repeated_header_table_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc long repeated-header table PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_table_row = [](featherdoc::pdf::PdfPageLayout &page, double y_points,
                            std::string first, std::string second,
                            std::string third) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(first)));
        page.text_runs.push_back(
            make_pdf_text_run(246.0, y_points, std::move(second)));
        page.text_runs.push_back(
            make_pdf_text_run(406.0, y_points, std::move(third)));
    };

    auto add_table_page = [&](featherdoc::pdf::PdfPageLayout &page,
                              std::size_t start_index, bool add_title,
                              bool add_footer) {
        page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
        if (add_title) {
            page.text_runs.push_back(
                make_pdf_text_run(72.0, 724.0, "Long repeated-header sample"));
        }

        constexpr std::size_t kBodyRowCountPerPage = 13U;
        append_grid(page, 72.0, 700.0, 160.0, 28.0, 3U,
                    kBodyRowCountPerPage + 1U);
        add_table_row(page, 678.0, "Item", "Owner", "Status");

        for (std::size_t row_offset = 0U; row_offset < kBodyRowCountPerPage;
             ++row_offset) {
            const auto row_number = start_index + row_offset + 1U;
            const auto y_points = 650.0 - 28.0 * static_cast<double>(row_offset);
            const auto item = "Feature" + std::to_string(row_number);
            const auto owner = "Import" + std::to_string((row_number % 5U) + 1U);
            const auto status =
                (row_number % 2U == 0U) ? "Verified" : "Tracked";
            add_table_row(page, y_points, item, owner, status);
        }

        if (add_footer) {
            page.text_runs.push_back(make_pdf_text_run(
                72.0, 240.0, "Footer note: long repeated-header sample"));
        }
    };

    featherdoc::pdf::PdfPageLayout first_page;
    add_table_page(first_page, 0U, true, false);
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    add_table_page(second_page, 13U, false, false);
    layout.pages.push_back(std::move(second_page));

    featherdoc::pdf::PdfPageLayout third_page;
    add_table_page(third_page, 26U, false, false);
    layout.pages.push_back(std::move(third_page));

    featherdoc::pdf::PdfPageLayout fourth_page;
    add_table_page(fourth_page, 39U, false, true);
    layout.pages.push_back(std::move(fourth_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_invoice_summary_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc invoice summary PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Invoice summary"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 676.0, "Invoice No."));
    page.text_runs.push_back(
        make_pdf_text_run(240.0, 676.0, "INV-2026-0507"));
    page.text_runs.push_back(
        make_pdf_text_run(470.0, 676.0, "USD 135.00"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 642.0, "Bill To"));
    page.text_runs.push_back(
        make_pdf_text_run(240.0, 642.0, "FeatherDoc QA"));
    page.text_runs.push_back(
        make_pdf_text_run(470.0, 642.0, "Net 30"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 608.0, "Project"));
    page.text_runs.push_back(
        make_pdf_text_run(240.0, 608.0, "PDF import"));
    page.text_runs.push_back(
        make_pdf_text_run(470.0, 608.0, "Roundtrip"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 548.0, "Footer note: layout is intentionally uneven"));
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
write_out_of_order_two_column_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc out-of-order two-column PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{306.0, 608.0},
        featherdoc::pdf::PdfPoint{306.0, 704.0},
        featherdoc::pdf::PdfRgbColor{0.82, 0.84, 0.88},
        0.75,
    });
    // Append right-column text first so the parser has to recover geometry
    // order instead of trusting content-stream order.
    page.text_runs.push_back(
        make_pdf_text_run(330.0, 660.0, "Right column beta"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 660.0, "Left column beta"));
    page.text_runs.push_back(
        make_pdf_text_run(330.0, 684.0, "Right column alpha"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 684.0, "Left column alpha"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Two column sample"));
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
