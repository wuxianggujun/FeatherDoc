auto run_rebase_character_style_based_on_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    return run_rebase_style_based_on_command(
        command, arguments,
        "rebase-character-style-based-on expects an input path, a style id, and a target basedOn style id",
        style_rebase_kind::character);
}

auto run_rebase_paragraph_style_based_on_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    return run_rebase_style_based_on_command(
        command, arguments,
        "rebase-paragraph-style-based-on expects an input path, a style id, and a target basedOn style id",
        style_rebase_kind::paragraph);
}

