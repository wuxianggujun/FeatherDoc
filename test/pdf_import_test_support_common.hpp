#pragma once

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

inline void append_four_by_four_grid_with_center_two_by_two_merge(
    featherdoc::pdf::PdfPageLayout &page, double left, double top,
    double cell_width, double cell_height) {
    const double right = left + cell_width * 4.0;
    const double bottom = top - cell_height * 4.0;
    const double merge_left = left + cell_width;
    const double merge_right = left + cell_width * 3.0;
    const double merge_top = top - cell_height;
    const double merge_middle = top - cell_height * 2.0;
    const double merge_bottom = top - cell_height * 3.0;

    for (std::size_t column = 0U; column <= 4U; ++column) {
        const double x = left + cell_width * static_cast<double>(column);
        if (column == 2U) {
            page.lines.push_back(featherdoc::pdf::PdfLine{
                featherdoc::pdf::PdfPoint{x, bottom},
                featherdoc::pdf::PdfPoint{x, merge_bottom},
                featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
                0.75,
            });
            page.lines.push_back(featherdoc::pdf::PdfLine{
                featherdoc::pdf::PdfPoint{x, merge_top},
                featherdoc::pdf::PdfPoint{x, top},
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

    for (std::size_t row = 0U; row <= 4U; ++row) {
        const double y = top - cell_height * static_cast<double>(row);
        if (row == 2U) {
            page.lines.push_back(featherdoc::pdf::PdfLine{
                featherdoc::pdf::PdfPoint{left, y},
                featherdoc::pdf::PdfPoint{merge_left, y},
                featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
                0.75,
            });
            page.lines.push_back(featherdoc::pdf::PdfLine{
                featherdoc::pdf::PdfPoint{merge_right, y},
                featherdoc::pdf::PdfPoint{right, y},
                featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
                0.75,
            });
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

inline void append_five_by_five_grid_with_center_two_by_three_merge(
    featherdoc::pdf::PdfPageLayout &page, double left, double top,
    double cell_width, double cell_height) {
    const double right = left + cell_width * 5.0;
    const double bottom = top - cell_height * 5.0;
    const double merge_left = left + cell_width;
    const double merge_right = left + cell_width * 3.0;
    const double merge_top = top - cell_height;
    const double merge_bottom = top - cell_height * 4.0;

    for (std::size_t column = 0U; column <= 5U; ++column) {
        const double x = left + cell_width * static_cast<double>(column);
        if (column == 2U) {
            page.lines.push_back(featherdoc::pdf::PdfLine{
                featherdoc::pdf::PdfPoint{x, bottom},
                featherdoc::pdf::PdfPoint{x, merge_bottom},
                featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
                0.75,
            });
            page.lines.push_back(featherdoc::pdf::PdfLine{
                featherdoc::pdf::PdfPoint{x, merge_top},
                featherdoc::pdf::PdfPoint{x, top},
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

    for (std::size_t row = 0U; row <= 5U; ++row) {
        const double y = top - cell_height * static_cast<double>(row);
        if (row == 2U || row == 3U) {
            page.lines.push_back(featherdoc::pdf::PdfLine{
                featherdoc::pdf::PdfPoint{left, y},
                featherdoc::pdf::PdfPoint{merge_left, y},
                featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
                0.75,
            });
            page.lines.push_back(featherdoc::pdf::PdfLine{
                featherdoc::pdf::PdfPoint{merge_right, y},
                featherdoc::pdf::PdfPoint{right, y},
                featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
                0.75,
            });
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

inline void append_four_column_grid_with_cross_column_header(
    featherdoc::pdf::PdfPageLayout &page, double left, double top,
    double cell_width, double cell_height, std::size_t row_count) {
    REQUIRE(row_count >= 3U);
    const double right = left + cell_width * 4.0;
    const double bottom = top - cell_height * static_cast<double>(row_count);
    const double group_header_bottom = top - cell_height;

    for (std::size_t column = 0U; column <= 4U; ++column) {
        const double x = left + cell_width * static_cast<double>(column);
        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{x, bottom},
            featherdoc::pdf::PdfPoint{
                x, column == 0U || column == 4U ? top : group_header_bottom},
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

inline void append_four_column_grid_with_parallel_group_headers(
    featherdoc::pdf::PdfPageLayout &page, double left, double top,
    double cell_width, double cell_height, std::size_t row_count) {
    REQUIRE(row_count >= 3U);
    const double right = left + cell_width * 4.0;
    const double bottom = top - cell_height * static_cast<double>(row_count);
    const double group_header_bottom = top - cell_height;

    for (std::size_t column = 0U; column <= 4U; ++column) {
        const double x = left + cell_width * static_cast<double>(column);
        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{x, bottom},
            featherdoc::pdf::PdfPoint{
                x, column == 0U || column == 2U || column == 4U
                       ? top
                       : group_header_bottom},
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

inline void append_four_column_grid_with_multilevel_group_headers(
    featherdoc::pdf::PdfPageLayout &page, double left, double top,
    double cell_width, double cell_height, std::size_t row_count) {
    REQUIRE(row_count >= 4U);
    const double right = left + cell_width * 4.0;
    const double bottom = top - cell_height * static_cast<double>(row_count);
    const double top_group_bottom = top - cell_height;
    const double subgroup_bottom = top - cell_height * 2.0;

    for (std::size_t column = 0U; column <= 4U; ++column) {
        const double x = left + cell_width * static_cast<double>(column);
        const bool outer_column = column == 0U || column == 4U;
        const bool subgroup_split = column == 2U;
        const double line_top =
            outer_column ? top : (subgroup_split ? top_group_bottom
                                                 : subgroup_bottom);
        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{x, bottom},
            featherdoc::pdf::PdfPoint{x, line_top},
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

}  // namespace featherdoc::test_support
