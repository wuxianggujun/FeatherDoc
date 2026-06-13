[[nodiscard]] bool contains_all_expected_text(
    const std::string &haystack,
    const std::vector<std::string> &needles,
    std::string &error_message) {
    auto compact_text = [](std::string_view text) {
        std::string output;
        output.reserve(text.size());
        for (const unsigned char value : text) {
            if (!std::isspace(value)) {
                output.push_back(static_cast<char>(value));
            }
        }
        return output;
    };
    auto preview = [](std::string_view text) {
        std::string output;
        constexpr std::size_t max_preview_bytes = 240U;
        output.reserve((std::min)(text.size(), max_preview_bytes));
        for (const char value : text.substr(0U, max_preview_bytes)) {
            const auto byte = static_cast<unsigned char>(value);
            if (value == '\r' || value == '\n' || value == '\t') {
                output.push_back(' ');
            } else if (byte < 0x20U) {
                output.push_back('?');
            } else {
                output.push_back(value);
            }
        }
        if (text.size() > max_preview_bytes) {
            output += "...";
        }
        return output;
    };

    const auto compact_haystack = compact_text(haystack);
    for (const auto &needle : needles) {
        if (haystack.find(needle) == std::string::npos &&
            compact_haystack.find(compact_text(needle)) == std::string::npos) {
            error_message = "Missing expected text: " + needle;
            error_message += haystack.empty()
                                 ? "; extracted text is empty"
                                 : "; extracted preview: " + preview(haystack);
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool contains_any_expected_text(
    const std::string &haystack,
    const std::vector<std::string> &needles) {
    auto compact_text = [](std::string_view text) {
        std::string output;
        output.reserve(text.size());
        for (const unsigned char value : text) {
            if (!std::isspace(value)) {
                output.push_back(static_cast<char>(value));
            }
        }
        return output;
    };
    const auto compact_haystack = compact_text(haystack);
    return std::any_of(
        needles.begin(), needles.end(),
        [&haystack, &compact_haystack, &compact_text](const std::string &needle) {
            return haystack.find(needle) != std::string::npos ||
                   compact_haystack.find(compact_text(needle)) !=
                       std::string::npos;
        });
}

[[nodiscard]] bool parse_arguments(const std::vector<std::string> &args,
                                   SampleConfig &config,
                                   bool &show_help,
                                   bool &require_cjk_font) {
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string_view arg = args[index];
        if (arg == "--help" || arg == "-h") {
            show_help = true;
            return true;
        }
        if (arg == "--scenario" && index + 1 < args.size()) {
            config.scenario = args[++index];
            continue;
        }
        if (arg == "--output" && index + 1 < args.size()) {
            config.output_path = args[++index];
            continue;
        }
        if (arg == "--expected-pages" && index + 1 < args.size()) {
            config.expected_pages =
                static_cast<std::size_t>(std::stoull(args[++index]));
            continue;
        }
        if (arg == "--expect-text" && index + 1 < args.size()) {
            config.expected_text.emplace_back(args[++index]);
            continue;
        }
        if (arg == "--expect-image-count" && index + 1 < args.size()) {
            config.expected_image_count =
                static_cast<std::size_t>(std::stoull(args[++index]));
            continue;
        }
        if (arg == "--require-cjk-font") {
            require_cjk_font = true;
            continue;
        }

        std::cerr << "unrecognized argument: " << arg << '\n';
        return false;
    }

    return !config.scenario.empty() && !config.output_path.empty() &&
           config.expected_pages > 0U && !config.expected_text.empty();
}

[[nodiscard]] std::vector<std::string> collect_utf8_arguments(
    int argc,
    char **argv) {
    std::vector<std::string> args;
    args.reserve(argc > 0 ? static_cast<std::size_t>(argc - 1) : 0U);
    for (int index = 1; index < argc; ++index) {
        args.emplace_back(argv[index]);
    }
    return args;
}

#if defined(_WIN32)
[[nodiscard]] std::vector<std::string> collect_utf8_arguments(
    int argc,
    wchar_t **argv) {
    std::vector<std::string> args;
    args.reserve(argc > 0 ? static_cast<std::size_t>(argc - 1) : 0U);
    for (int index = 1; index < argc; ++index) {
        args.emplace_back(utf8_from_wide(argv[index]));
    }
    return args;
}
#endif

[[nodiscard]] void print_usage() {
    std::cerr
        << "usage: featherdoc_pdf_regression_sample --scenario <name> "
           "--output <pdf> --expected-pages <count> "
           "[--expect-image-count <count>] [--require-cjk-font]\n";
}

} // namespace
