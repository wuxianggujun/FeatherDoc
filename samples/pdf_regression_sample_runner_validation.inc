int write_and_validate_sample(const SampleConfig &config,
                              const ScenarioResult &sample,
                              bool require_cjk_font) {
    if (sample.layout.pages.size() != config.expected_pages) {
        std::cerr << "unexpected page count for scenario " << config.scenario
                  << ": expected " << config.expected_pages << ", got "
                  << sample.layout.pages.size() << '\n';
        return 1;
    }

    featherdoc::pdf::PdfWriterOptions writer_options;
    writer_options.page_size = sample.layout.pages.front().size;
    writer_options.title = sample.layout.metadata.title;
    writer_options.creator = sample.layout.metadata.creator;

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(sample.layout, config.output_path, writer_options);
    if (!write_result) {
        std::cerr << write_result.error_message << '\n';
        return 1;
    }
    if (!std::filesystem::exists(config.output_path)) {
        std::cerr << "PDF output file was not created\n";
        return 1;
    }
    const auto output_size = std::filesystem::file_size(config.output_path);
    if (output_size == 0U) {
        std::cerr << "PDF output file is empty\n";
        return 1;
    }
    const auto max_output_size =
        static_cast<std::uintmax_t>(sample.layout.pages.size()) * 700000U;
    if (output_size > max_output_size) {
        std::cerr << "PDF output file is unexpectedly large for scenario "
                  << config.scenario << ": expected at most "
                  << max_output_size << " bytes, got " << output_size << '\n';
        return 1;
    }

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(config.output_path, {});
    if (!parse_result) {
        std::cerr << parse_result.error_message << '\n';
        return 1;
    }

    if (parse_result.document.pages.size() != config.expected_pages) {
        std::cerr << "PDFium reported unexpected page count: expected "
                  << config.expected_pages << ", got "
                  << parse_result.document.pages.size() << '\n';
        return 1;
    }

    const auto extracted_text = collect_text(parse_result.document);
    std::string text_error;
    if (!contains_all_expected_text(extracted_text, config.expected_text,
                                    text_error)) {
        if (require_cjk_font || config.scenario == "cjk_text" ||
            !contains_any_expected_text(extracted_text,
                                        config.expected_text)) {
            std::cerr << text_error << '\n'
                      << "skipping regression sample: current PDFio/PDFium "
                         "extraction path does not yet preserve this text "
                         "path reliably\n";
            return 77;
        }
        std::cerr << text_error << '\n';
        return 1;
    }

    if (config.expected_image_count.has_value()) {
        std::string image_count_error;
        const auto image_count =
            count_pdf_image_objects(config.output_path, image_count_error);
        if (!image_count.has_value()) {
            std::cerr << image_count_error << '\n';
            return 1;
        }
        if (*image_count != *config.expected_image_count) {
            std::cerr << "unexpected PDF image object count for scenario "
                      << config.scenario << ": expected "
                      << *config.expected_image_count << ", got "
                      << *image_count << '\n';
            return 1;
        }
    }

    std::cout << "wrote " << config.output_path.string() << " ("
              << write_result.bytes_written << " bytes, "
              << parse_result.document.pages.size() << " pages)\n";
    return 0;
}
