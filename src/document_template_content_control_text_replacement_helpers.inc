struct content_control_plain_text_replacement_target final {
    pugi::xml_node content_control;
    pugi::xml_node original_content;
    pugi::xml_node staging_content;
    featherdoc::content_control_kind kind{
        featherdoc::content_control_kind::unknown};
};

void collect_content_control_plain_text_replacement_targets(
    pugi::xml_node node, std::string_view value, bool match_tag,
    std::vector<content_control_plain_text_replacement_target> &targets) {
    for (auto current = node.first_child(); current != pugi::xml_node{};) {
        if (std::string_view{current.name()} == "w:sdt" &&
            content_control_matches_tag_or_alias(current, value, match_tag)) {
            const auto next = featherdoc::detail::next_xml_node_preorder(
                node, current, false);
            targets.push_back(content_control_plain_text_replacement_target{
                current, current.child("w:sdtContent"), {},
                content_control_kind_from_node(current)});
            current = next;
            continue;
        }

        current =
            featherdoc::detail::next_xml_node_preorder(node, current);
    }
}

void discard_content_control_plain_text_replacements(
    std::span<content_control_plain_text_replacement_target> targets) noexcept {
    for (auto &target : targets) {
        if (target.staging_content == pugi::xml_node{}) {
            continue;
        }

        if (target.original_content != pugi::xml_node{}) {
            for (auto child = target.staging_content.next_sibling();
                 child != pugi::xml_node{};) {
                const auto next = child.next_sibling();
                (void)target.staging_content.append_move(child);
                child = next;
            }
        }

        if (auto parent = target.staging_content.parent();
            parent != pugi::xml_node{}) {
            (void)parent.remove_child(target.staging_content);
        }
        target.staging_content = {};
    }
}

auto replace_content_control_text_by_tag_or_alias_in_part(
    featherdoc::document_error_info &last_error_info,
    featherdoc::detail::xml_handle_lifetime &handle_lifetime,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view value, std::string_view replacement, bool match_tag)
    -> std::size_t {
    if (value.empty()) {
        set_last_error(
            last_error_info, std::make_error_code(std::errc::invalid_argument),
            match_tag ? "content control tag must not be empty"
                      : "content control alias must not be empty",
            std::string{entry_name});
        return 0U;
    }

    auto targets =
        std::vector<content_control_plain_text_replacement_target>{};
    auto retirement_roots = std::vector<pugi::xml_node>{};
    try {
        collect_content_control_plain_text_replacement_targets(
            document, value, match_tag, targets);
        if (targets.empty()) {
            last_error_info.clear();
            return 0U;
        }

        for (const auto &target : targets) {
            for (auto child = target.original_content.first_child();
                 child != pugi::xml_node{}; child = child.next_sibling()) {
                retirement_roots.push_back(child);
            }

            const auto properties = target.content_control.child("w:sdtPr");
            for (auto placeholder = properties.child("w:showingPlcHdr");
                 placeholder != pugi::xml_node{};
                 placeholder =
                     placeholder.next_sibling("w:showingPlcHdr")) {
                retirement_roots.push_back(placeholder);
            }
        }

        for (auto &target : targets) {
            const auto staging_parent =
                target.original_content != pugi::xml_node{}
                    ? target.original_content
                    : target.content_control;
            target.staging_content =
                featherdoc::detail::checked_append_xml_element(
                    staging_parent, "w:sdtContent");
            if (target.staging_content == pugi::xml_node{} ||
                !append_content_control_plain_text(
                    target.staging_content, target.kind, replacement)) {
                discard_content_control_plain_text_replacements(targets);
                set_last_error(
                    last_error_info,
                    std::make_error_code(std::errc::not_enough_memory),
                    "failed to prepare content control text replacement",
                    std::string{entry_name});
                return 0U;
            }
        }
    } catch (const std::bad_alloc &) {
        discard_content_control_plain_text_replacements(targets);
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to prepare content control text replacement",
                       std::string{entry_name});
        return 0U;
    }

    for (auto &target : targets) {
        if (target.original_content == pugi::xml_node{}) {
            continue;
        }
        while (const auto child = target.staging_content.first_child()) {
            // FeatherDoc vendors non-compact pugixml. These same-document
            // moves do not allocate and can be rolled back before retirement.
            if (target.original_content.append_move(child) ==
                pugi::xml_node{}) {
                discard_content_control_plain_text_replacements(targets);
                set_last_error(
                    last_error_info,
                    featherdoc::document_errc::invalid_package_structure,
                    "failed to stage content control text replacement",
                    std::string{entry_name});
                return 0U;
            }
        }
    }

    try {
        handle_lifetime.retire_subtrees(retirement_roots);
    } catch (const std::bad_alloc &) {
        discard_content_control_plain_text_replacements(targets);
        set_last_error(
            last_error_info,
            std::make_error_code(std::errc::not_enough_memory),
            "content control XML handle retirement ran out of memory",
            std::string{entry_name});
        return 0U;
    } catch (const std::system_error &error) {
        discard_content_control_plain_text_replacements(targets);
        set_last_error(
            last_error_info, error.code(),
            "failed to synchronize XML handle retirement before content "
            "control text replacement",
            std::string{entry_name});
        return 0U;
    }

    for (auto &target : targets) {
        if (target.original_content != pugi::xml_node{}) {
            for (auto child = target.original_content.first_child();
                 child != target.staging_content;) {
                const auto next = child.next_sibling();
                (void)target.original_content.remove_child(child);
                child = next;
            }
            (void)target.original_content.remove_child(target.staging_content);
        }
        target.staging_content = {};
        clear_content_control_placeholder_flag(target.content_control);
    }

    last_error_info.clear();
    return targets.size();
}
