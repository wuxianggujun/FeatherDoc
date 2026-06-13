#pragma once

#include "pdf_import_test_support_common.hpp"

namespace featherdoc::test_support {

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
write_invoice_grid_merged_summary_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc invoice grid merged-summary PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Invoice merged summary grid sample"));
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
        72.0, 520.0, "Footer note: merged summary row spans label columns"));
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
write_invoice_grid_inline_subtotal_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc invoice grid inline-subtotal PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Invoice inline subtotal grid sample"));
    append_grid(page, 72.0, 700.0, 130.0, 28.0, 4U, 6U);
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
        make_pdf_text_run(86.0, 622.0, "Design subtotal"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 622.0, "USD 100"));
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 594.0, "Visual validation"));
    page.text_runs.push_back(make_pdf_text_run(230.0, 594.0, "1"));
    page.text_runs.push_back(make_pdf_text_run(364.0, 594.0, "USD 25"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 594.0, "USD 25"));
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 566.0, "Regression evidence"));
    page.text_runs.push_back(make_pdf_text_run(230.0, 566.0, "1"));
    page.text_runs.push_back(make_pdf_text_run(364.0, 566.0, "USD 10"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 566.0, "USD 10"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 538.0, "Grand total"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 538.0, "USD 135"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 492.0, "Footer note: inline subtotal keeps later data rows"));
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
write_invoice_grid_right_subtotal_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc invoice grid right-subtotal PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Invoice right subtotal grid sample"));
    append_grid(page, 72.0, 700.0, 130.0, 28.0, 4U, 6U);
    page.text_runs.push_back(make_pdf_text_run(86.0, 678.0, "Item"));
    page.text_runs.push_back(make_pdf_text_run(230.0, 678.0, "Qty"));
    page.text_runs.push_back(make_pdf_text_run(364.0, 678.0, "Unit"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 678.0, "Total"));
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 650.0, "PDF export design"));
    page.text_runs.push_back(make_pdf_text_run(230.0, 650.0, "2"));
    page.text_runs.push_back(make_pdf_text_run(364.0, 650.0, "USD 50"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 650.0, "USD 100"));
    page.text_runs.push_back(make_pdf_text_run(230.0, 622.0, "Design subtotal"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 622.0, "USD 100"));
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 594.0, "Visual validation"));
    page.text_runs.push_back(make_pdf_text_run(230.0, 594.0, "1"));
    page.text_runs.push_back(make_pdf_text_run(364.0, 594.0, "USD 25"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 594.0, "USD 25"));
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 566.0, "Regression evidence"));
    page.text_runs.push_back(make_pdf_text_run(230.0, 566.0, "1"));
    page.text_runs.push_back(make_pdf_text_run(364.0, 566.0, "USD 10"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 566.0, "USD 10"));
    page.text_runs.push_back(make_pdf_text_run(230.0, 538.0, "Grand total"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 538.0, "USD 135"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 492.0, "Footer note: right subtotal labels keep item column blank"));
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
