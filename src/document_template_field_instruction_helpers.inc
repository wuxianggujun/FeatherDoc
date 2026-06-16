auto field_identifier_is_valid(std::string_view value) -> bool {
    if (value.empty()) {
        return false;
    }

    for (const auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (std::isspace(byte) != 0 || character == '"' || character == '\\') {
            return false;
        }
    }
    return true;
}

auto field_argument_is_valid(std::string_view value) -> bool {
    if (value.empty()) {
        return false;
    }

    for (const auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (std::iscntrl(byte) != 0 || character == '"' || character == '\\') {
            return false;
        }
    }
    return true;
}

void append_field_argument(std::ostringstream &instruction,
                           std::string_view value) {
    const auto requires_quotes = std::any_of(
        value.begin(), value.end(), [](char character) {
            return std::isspace(static_cast<unsigned char>(character)) != 0;
        });
    if (requires_quotes) {
        instruction << '"' << value << '"';
        return;
    }
    instruction << value;
}

void append_quoted_field_argument(std::ostringstream &instruction,
                                  std::string_view value) {
    instruction << '"' << value << '"';
}

auto build_table_of_contents_instruction(
    featherdoc::table_of_contents_field_options options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    if (options.min_outline_level == 0U || options.max_outline_level == 0U ||
        options.min_outline_level > options.max_outline_level ||
        options.max_outline_level > 9U) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "TOC outline levels must be between 1 and 9 and ordered",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " TOC \\o \"" << options.min_outline_level << '-'
                << options.max_outline_level << "\"";
    if (options.hyperlinks) {
        instruction << " \\h";
    }
    if (options.hide_page_numbers_in_web_layout) {
        instruction << " \\z";
    }
    if (options.use_outline_levels) {
        instruction << " \\u";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_reference_instruction(
    std::string_view bookmark_name,
    featherdoc::reference_field_options options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    if (!field_identifier_is_valid(bookmark_name)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "REF bookmark name must not be empty or contain whitespace, quotes, or backslashes",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " REF " << bookmark_name;
    if (options.hyperlink) {
        instruction << " \\h";
    }
    if (options.preserve_formatting) {
        instruction << " \\* MERGEFORMAT";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_page_reference_instruction(
    std::string_view bookmark_name,
    featherdoc::page_reference_field_options options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    if (!field_identifier_is_valid(bookmark_name)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "PAGEREF bookmark name must not be empty or contain whitespace, quotes, or backslashes",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " PAGEREF " << bookmark_name;
    if (options.hyperlink) {
        instruction << " \\h";
    }
    if (options.relative_position) {
        instruction << " \\p";
    }
    if (options.preserve_formatting) {
        instruction << " \\* MERGEFORMAT";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_style_reference_instruction(
    std::string_view style_name,
    featherdoc::style_reference_field_options options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    if (!field_argument_is_valid(style_name)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "STYLEREF style name must not be empty or contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " STYLEREF ";
    append_field_argument(instruction, style_name);
    if (options.paragraph_number) {
        instruction << " \\n";
    }
    if (options.relative_position) {
        instruction << " \\p";
    }
    if (options.preserve_formatting) {
        instruction << " \\* MERGEFORMAT";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_document_property_instruction(
    std::string_view property_name,
    featherdoc::document_property_field_options options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    if (!field_argument_is_valid(property_name)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "DOCPROPERTY property name must not be empty or contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " DOCPROPERTY ";
    append_field_argument(instruction, property_name);
    if (options.preserve_formatting) {
        instruction << " \\* MERGEFORMAT";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_date_instruction(featherdoc::date_field_options options,
                            featherdoc::document_error_info &last_error_info,
                            std::string_view entry_name)
    -> std::optional<std::string> {
    if (!options.format.empty() && !field_argument_is_valid(options.format)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "DATE format must not contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " DATE";
    if (!options.format.empty()) {
        instruction << " \\@ ";
        append_quoted_field_argument(instruction, options.format);
    }
    if (options.preserve_formatting) {
        instruction << " \\* MERGEFORMAT";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_hyperlink_instruction(
    std::string_view target,
    featherdoc::hyperlink_field_options options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    if (!field_argument_is_valid(target)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "HYPERLINK target must not be empty or contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }
    if (options.anchor.has_value() &&
        !field_argument_is_valid(*options.anchor)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "HYPERLINK anchor must not be empty or contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }
    if (options.tooltip.has_value() &&
        !field_argument_is_valid(*options.tooltip)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "HYPERLINK tooltip must not be empty or contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " HYPERLINK ";
    append_quoted_field_argument(instruction, target);
    if (options.anchor.has_value()) {
        instruction << " \\l ";
        append_quoted_field_argument(instruction, *options.anchor);
    }
    if (options.tooltip.has_value()) {
        instruction << " \\o ";
        append_quoted_field_argument(instruction, *options.tooltip);
    }
    if (options.preserve_formatting) {
        instruction << " \\* MERGEFORMAT";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_sequence_instruction(
    std::string_view identifier,
    const featherdoc::sequence_field_options &options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    if (!field_identifier_is_valid(identifier)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "SEQ identifier must not be empty or contain whitespace, quotes, or backslashes",
                       std::string{entry_name});
        return std::nullopt;
    }
    if (!field_identifier_is_valid(options.number_format)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "SEQ number format must not be empty or contain whitespace, quotes, or backslashes",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " SEQ " << identifier << " \\* " << options.number_format;
    if (options.restart_value.has_value()) {
        instruction << " \\r " << *options.restart_value;
    }
    if (options.preserve_formatting) {
        instruction << " \\* MERGEFORMAT";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_caption_sequence_instruction(
    std::string_view label,
    const featherdoc::caption_field_options &options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    auto sequence_options = featherdoc::sequence_field_options{};
    sequence_options.number_format = options.number_format;
    sequence_options.restart_value = options.restart_value;
    sequence_options.preserve_formatting = options.preserve_formatting;
    sequence_options.state = options.state;
    return build_sequence_instruction(label, sequence_options, last_error_info,
                                      entry_name);
}

auto build_index_instruction(featherdoc::index_field_options options,
                             featherdoc::document_error_info &last_error_info,
                             std::string_view entry_name)
    -> std::optional<std::string> {
    if (options.columns.has_value() && *options.columns == 0U) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "INDEX columns must be greater than zero",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::ostringstream instruction;
    instruction << " INDEX";
    if (options.columns.has_value()) {
        instruction << " \\c \"" << *options.columns << "\"";
    }
    if (options.preserve_formatting) {
        instruction << " \\* MERGEFORMAT";
    }
    instruction << ' ';
    return instruction.str();
}

auto build_index_entry_instruction(
    std::string_view entry_text,
    const featherdoc::index_entry_field_options &options,
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name) -> std::optional<std::string> {
    if (!field_argument_is_valid(entry_text)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "XE entry text must not be empty or contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }
    if (options.subentry.has_value() &&
        !field_argument_is_valid(*options.subentry)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "XE subentry text must not be empty or contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }
    if (options.bookmark_name.has_value() &&
        !field_identifier_is_valid(*options.bookmark_name)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "XE bookmark name must not be empty or contain whitespace, quotes, or backslashes",
                       std::string{entry_name});
        return std::nullopt;
    }
    if (options.cross_reference.has_value() &&
        !field_argument_is_valid(*options.cross_reference)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "XE cross reference must not be empty or contain quotes, backslashes, or control characters",
                       std::string{entry_name});
        return std::nullopt;
    }

    std::string full_entry{entry_text};
    if (options.subentry.has_value()) {
        full_entry += ':';
        full_entry += *options.subentry;
    }

    std::ostringstream instruction;
    instruction << " XE ";
    append_quoted_field_argument(instruction, full_entry);
    if (options.bookmark_name.has_value()) {
        instruction << " \\r " << *options.bookmark_name;
    }
    if (options.cross_reference.has_value()) {
        instruction << " \\t ";
        append_quoted_field_argument(instruction, *options.cross_reference);
    }
    if (options.bold_page_number) {
        instruction << " \\b";
    }
    if (options.italic_page_number) {
        instruction << " \\i";
    }
    instruction << ' ';
    return instruction.str();
}
