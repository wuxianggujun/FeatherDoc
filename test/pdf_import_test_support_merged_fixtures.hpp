#pragma once

#include "pdf_import_test_support_common.hpp"

namespace featherdoc::test_support {

[[nodiscard]] inline std::filesystem::path
write_invoice_grid_pagebreak_subtotal_local_anchor_drift_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc invoice grid pagebreak subtotal local-anchor-drift PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_standard_invoice_row =
        [](featherdoc::pdf::PdfPageLayout &page, double y_points,
           std::string item, std::string qty, std::string unit,
           std::string total) {
            page.text_runs.push_back(
                make_pdf_text_run(86.0, y_points, std::move(item)));
            if (!qty.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(230.0, y_points, std::move(qty)));
            }
            if (!unit.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(364.0, y_points, std::move(unit)));
            }
            if (!total.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(498.0, y_points, std::move(total)));
            }
        };

    auto add_drifted_invoice_row =
        [](featherdoc::pdf::PdfPageLayout &page, double y_points,
           std::string item, std::string qty, std::string unit,
           std::string total) {
            page.text_runs.push_back(
                make_pdf_text_run(86.0, y_points, std::move(item)));
            if (!qty.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(253.0, y_points, std::move(qty)));
            }
            if (!unit.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(387.0, y_points, std::move(unit)));
            }
            if (!total.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(521.0, y_points, std::move(total)));
            }
        };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Invoice pagebreak subtotal local anchor drift sample"));
    append_grid(first_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_standard_invoice_row(first_page, 678.0, "Item", "Qty", "Unit",
                             "Total");
    add_standard_invoice_row(first_page, 650.0, "PDF export design", "2",
                             "USD 50", "USD 100");
    add_standard_invoice_row(first_page, 622.0, "Design subtotal", "", "",
                             "USD 100");
    add_standard_invoice_row(first_page, 594.0, "Visual validation", "1",
                             "USD 25", "USD 25");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_grid(second_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_standard_invoice_row(second_page, 678.0, "Item", "Qty", "Unit",
                             "Total");
    add_drifted_invoice_row(second_page, 650.0, "Regression evidence", "1",
                            "USD 10", "USD 10");
    add_drifted_invoice_row(second_page, 622.0, "Documentation pass", "1",
                            "USD 15", "USD 15");
    add_drifted_invoice_row(second_page, 594.0, "Grand total", "", "",
                            "USD 150");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 540.0,
        "Footer note: local anchor drift keeps subtotal table separate"));
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
write_invoice_grid_pagebreak_subtotal_column_count_mismatch_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc invoice grid pagebreak subtotal column-count-mismatch PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_four_column_row =
        [](featherdoc::pdf::PdfPageLayout &page, double y_points,
           std::string item, std::string qty, std::string unit,
           std::string total) {
            page.text_runs.push_back(
                make_pdf_text_run(86.0, y_points, std::move(item)));
            if (!qty.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(230.0, y_points, std::move(qty)));
            }
            if (!unit.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(364.0, y_points, std::move(unit)));
            }
            if (!total.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(498.0, y_points, std::move(total)));
            }
        };

    auto add_three_column_row =
        [](featherdoc::pdf::PdfPageLayout &page, double y_points,
           std::string item, std::string qty, std::string total) {
            page.text_runs.push_back(
                make_pdf_text_run(86.0, y_points, std::move(item)));
            if (!qty.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(260.0, y_points, std::move(qty)));
            }
            if (!total.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(434.0, y_points, std::move(total)));
            }
        };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Invoice pagebreak subtotal column count mismatch sample"));
    append_grid(first_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_four_column_row(first_page, 678.0, "Item", "Qty", "Unit", "Total");
    add_four_column_row(first_page, 650.0, "PDF export design", "2",
                        "USD 50", "USD 100");
    add_four_column_row(first_page, 622.0, "Design subtotal", "", "",
                        "USD 100");
    add_four_column_row(first_page, 594.0, "Visual validation", "1",
                        "USD 25", "USD 25");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_grid(second_page, 72.0, 700.0, 170.0, 28.0, 3U, 4U);
    add_three_column_row(second_page, 678.0, "Item", "Qty", "Total");
    add_three_column_row(second_page, 650.0, "Regression evidence", "1",
                         "USD 10");
    add_three_column_row(second_page, 622.0, "Documentation pass", "1",
                         "USD 15");
    add_three_column_row(second_page, 594.0, "Grand total", "", "USD 150");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 540.0,
        "Footer note: column-count-mismatched subtotal table stays separate"));
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


}  // namespace featherdoc::test_support
