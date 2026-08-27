std::optional<featherdoc::text_range_preview> Document::preview_text_range(
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before previewing a text range",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    std::vector<pugi::xml_node> paragraphs;
    auto body =
        const_cast<pugi::xml_document &>(this->document)
            .child("w:document")
            .child("w:body");
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:p") {
            paragraphs.push_back(child);
        }
    }

    paragraph_revision_range_plan plan;
    std::string range_error;
    if (!build_paragraph_revision_range_plan(
            paragraphs, start_paragraph_index, start_text_offset,
            end_paragraph_index, end_text_offset, plan, range_error)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       range_error, std::string{document_xml_entry});
        return std::nullopt;
    }

    auto preview = preview_paragraph_revision_range_segments(
        start_paragraph_index, start_text_offset, end_paragraph_index,
        end_text_offset, plan);
    this->last_error_info.clear();
    return preview;
}

std::vector<featherdoc::text_range_preview> Document::find_text_ranges(
    std::string_view text) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before finding text ranges",
                       std::string{document_xml_entry});
        return {};
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "search text must not be empty",
                       std::string{document_xml_entry});
        return {};
    }

    struct paragraph_text_range {
        std::size_t paragraph_index{};
        std::size_t global_start{};
        std::size_t text_length{};
    };

    std::vector<pugi::xml_node> paragraphs;
    std::vector<paragraph_text_range> paragraph_ranges;
    std::string document_text;
    auto body =
        const_cast<pugi::xml_document &>(this->document)
            .child("w:document")
            .child("w:body");
    std::size_t paragraph_index = 0U;
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        paragraphs.push_back(child);
        paragraph_text_range range;
        range.paragraph_index = paragraph_index;
        range.global_start = document_text.size();
        for (const auto &span : collect_paragraph_revision_run_spans(child)) {
            document_text += span.text;
        }
        range.text_length = document_text.size() - range.global_start;
        paragraph_ranges.push_back(range);
        ++paragraph_index;
    }

    const auto map_match_start =
        [&paragraph_ranges](std::size_t global_offset, std::size_t &mapped_paragraph,
                            std::size_t &mapped_offset) -> bool {
        if (paragraph_ranges.empty()) {
            return false;
        }
        for (const auto &range : paragraph_ranges) {
            if (range.text_length == 0U) {
                continue;
            }
            const auto range_end = range.global_start + range.text_length;
            if (global_offset < range_end) {
                mapped_paragraph = range.paragraph_index;
                mapped_offset = global_offset - range.global_start;
                return true;
            }
        }

        const auto &last = paragraph_ranges.back();
        mapped_paragraph = last.paragraph_index;
        mapped_offset = last.text_length;
        return true;
    };

    const auto map_match_end =
        [&paragraph_ranges](std::size_t global_offset, std::size_t &mapped_paragraph,
                            std::size_t &mapped_offset) -> bool {
        if (paragraph_ranges.empty()) {
            return false;
        }

        std::optional<paragraph_text_range> previous_text_range;
        for (const auto &range : paragraph_ranges) {
            if (range.text_length == 0U) {
                continue;
            }
            const auto range_end = range.global_start + range.text_length;
            if (global_offset > range.global_start && global_offset <= range_end) {
                mapped_paragraph = range.paragraph_index;
                mapped_offset = global_offset - range.global_start;
                return true;
            }
            if (global_offset == range.global_start &&
                previous_text_range.has_value()) {
                mapped_paragraph = previous_text_range->paragraph_index;
                mapped_offset = previous_text_range->text_length;
                return true;
            }
            previous_text_range = range;
        }

        if (previous_text_range.has_value()) {
            mapped_paragraph = previous_text_range->paragraph_index;
            mapped_offset = previous_text_range->text_length;
            return true;
        }

        const auto &last = paragraph_ranges.back();
        mapped_paragraph = last.paragraph_index;
        mapped_offset = last.text_length;
        return true;
    };

    std::vector<featherdoc::text_range_preview> matches;
    std::size_t search_offset = 0U;
    while (search_offset <= document_text.size()) {
        const auto match_offset = document_text.find(text, search_offset);
        if (match_offset == std::string::npos) {
            break;
        }

        std::size_t start_paragraph_index = 0U;
        std::size_t start_text_offset = 0U;
        std::size_t end_paragraph_index = 0U;
        std::size_t end_text_offset = 0U;
        if (!map_match_start(match_offset, start_paragraph_index,
                             start_text_offset) ||
            !map_match_end(match_offset + text.size(), end_paragraph_index,
                           end_text_offset)) {
            break;
        }

        paragraph_revision_range_plan plan;
        std::string range_error;
        if (!build_paragraph_revision_range_plan(
                paragraphs, start_paragraph_index, start_text_offset,
                end_paragraph_index, end_text_offset, plan, range_error)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           range_error, std::string{document_xml_entry});
            return {};
        }

        matches.push_back(preview_paragraph_revision_range_segments(
            start_paragraph_index, start_text_offset, end_paragraph_index,
            end_text_offset, plan));
        search_offset = match_offset + 1U;
    }

    this->last_error_info.clear();
    return matches;
}

std::vector<featherdoc::revision_summary> Document::list_revisions() const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing revisions",
                       std::string{document_xml_entry});
        return {};
    }

    std::vector<featherdoc::revision_summary> summaries;
    collect_revisions_from_node(const_cast<pugi::xml_document &>(this->document),
                                document_xml_entry, summaries);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revisions_from_node(part->xml, part->entry_name, summaries);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revisions_from_node(part->xml, part->entry_name, summaries);
        }
    }

    this->last_error_info.clear();
    return summaries;
}

namespace {

struct revision_story_view {
    pugi::xml_document *document{};
    pugi::xml_node root{};
    std::string_view entry_name{};
};

struct revision_story_work {
    revision_story_view live;
    pugi::xml_document work;
};

auto revision_story_node_count(pugi::xml_node root) noexcept -> std::size_t {
    std::size_t count = 0U;
    for (auto current = root.first_child(); current != pugi::xml_node{};
         current = featherdoc::detail::next_xml_node_preorder(root, current)) {
        if (revision_kind_from_node_name(current.name()) !=
            featherdoc::revision_kind::unknown) {
            ++count;
        }
    }
    return count;
}

auto revision_story_node_at(pugi::xml_node root, std::size_t revision_index)
    noexcept -> pugi::xml_node {
    std::size_t current_index = 0U;
    for (auto current = root.first_child(); current != pugi::xml_node{};
         current = featherdoc::detail::next_xml_node_preorder(root, current)) {
        if (revision_kind_from_node_name(current.name()) ==
            featherdoc::revision_kind::unknown) {
            continue;
        }
        if (current_index == revision_index) {
            return current;
        }
        ++current_index;
    }
    return {};
}

auto first_revision_story_node(pugi::xml_node root) noexcept -> pugi::xml_node {
    return revision_story_node_at(root, 0U);
}

bool select_revision_story(revision_story_view candidate,
                           std::size_t &remaining_index,
                           revision_story_view &selected,
                           std::size_t &local_index) noexcept {
    const auto candidate_count = revision_story_node_count(candidate.root);
    if (remaining_index >= candidate_count) {
        remaining_index -= candidate_count;
        return false;
    }
    selected = candidate;
    local_index = remaining_index;
    return true;
}

auto corresponding_revision_story_root(pugi::xml_document &document,
                                       pugi::xml_node live_root) noexcept
    -> pugi::xml_node {
    return std::string_view{live_root.name()} == "w:body"
               ? document.child("w:document").child("w:body")
               : document.child(live_root.name());
}

template <typename Mutation>
bool mutate_single_revision_story(
    revision_story_view story, std::size_t local_revision_index,
    featherdoc::detail::xml_handle_lifetime &lifetime,
    featherdoc::document_error_info &error_info, Mutation &&mutation) {
    pugi::xml_document work;
    if (featherdoc::detail::checked_clone_xml_document(*story.document, work) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            error_info, story.entry_name);
        return false;
    }

    try {
        const auto work_root =
            corresponding_revision_story_root(work, story.root);
        const auto revision =
            revision_story_node_at(work_root, local_revision_index);
        if (work_root == pugi::xml_node{} ||
            revision == pugi::xml_node{} || !mutation(revision)) {
            (void)featherdoc::detail::
                set_xml_document_mutation_allocation_failure(error_info,
                                                              story.entry_name);
            return false;
        }

        static_assert(noexcept(std::declval<pugi::xml_document &>() =
                               std::declval<pugi::xml_document &&>()));
        lifetime.retire_subtree(story.root);
        *story.document = std::move(work);
        return true;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            error_info,
            "revision story mutation ran out of memory before it could be published",
            story.entry_name);
        return false;
    }
}

void append_revision_story_work(std::vector<revision_story_work> &stories,
                                revision_story_view story) {
    if (revision_story_node_count(story.root) != 0U) {
        stories.push_back(revision_story_work{story, {}});
    }
}

auto mutate_all_revision_stories(
    std::vector<revision_story_work> &stories, bool accept,
    featherdoc::detail::xml_handle_lifetime &lifetime,
    featherdoc::document_error_info &error_info)
    -> std::optional<std::size_t> {
    if (stories.empty()) {
        return std::size_t{0U};
    }

    auto current_entry = std::string_view{document_xml_entry};
    std::size_t mutation_count = 0U;
    try {
        for (auto &story : stories) {
            current_entry = story.live.entry_name;
            if (featherdoc::detail::checked_clone_xml_document(
                    *story.live.document, story.work) !=
                featherdoc::detail::xml_document_clone_status::success) {
                (void)featherdoc::detail::
                    set_xml_document_clone_allocation_failure(
                        error_info, story.live.entry_name);
                return std::nullopt;
            }
        }

        for (auto &story : stories) {
            current_entry = story.live.entry_name;
            const auto work_root = corresponding_revision_story_root(
                story.work, story.live.root);
            if (work_root == pugi::xml_node{}) {
                (void)featherdoc::detail::
                    set_xml_document_mutation_allocation_failure(
                        error_info, story.live.entry_name);
                return std::nullopt;
            }
            while (const auto revision =
                       first_revision_story_node(work_root)) {
                if (!apply_revision_decision(revision, accept)) {
                    (void)featherdoc::detail::
                        set_xml_document_mutation_allocation_failure(
                            error_info, story.live.entry_name);
                    return std::nullopt;
                }
                ++mutation_count;
            }
        }

        std::vector<pugi::xml_node> retired_roots;
        retired_roots.reserve(stories.size());
        for (const auto &story : stories) {
            retired_roots.push_back(story.live.root);
        }
        lifetime.retire_subtrees(retired_roots);
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            error_info,
            "revision batch mutation ran out of memory before it could be published",
            current_entry);
        return std::nullopt;
    }

    static_assert(noexcept(std::declval<pugi::xml_document &>() =
                           std::declval<pugi::xml_document &&>()));
    for (auto &story : stories) {
        *story.live.document = std::move(story.work);
    }
    return mutation_count;
}

} // namespace

bool Document::set_revision_metadata(
    std::size_t revision_index,
    const featherdoc::revision_metadata_update &metadata) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before setting revision metadata",
                       std::string{document_xml_entry});
        return false;
    }
    const auto has_change = metadata.author.has_value() ||
                            metadata.date.has_value() ||
                            metadata.clear_author || metadata.clear_date;
    if (!has_change) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision metadata update must contain at least one change",
                       std::string{document_xml_entry});
        return false;
    }
    if ((metadata.author.has_value() && metadata.clear_author) ||
        (metadata.date.has_value() && metadata.clear_date)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision metadata cannot be set and cleared in the same update",
                       std::string{document_xml_entry});
        return false;
    }

    auto remaining_index = revision_index;
    auto local_revision_index = std::size_t{0U};
    auto story = revision_story_view{};
    auto found = select_revision_story(
        {&this->document,
         this->document.child("w:document").child("w:body"),
         document_xml_entry},
        remaining_index, story, local_revision_index);
    for (auto &part : this->header_parts) {
        if (!found && part != nullptr) {
            found = select_revision_story(
                {&part->xml, part->xml.child("w:hdr"), part->entry_name},
                remaining_index, story, local_revision_index);
        }
    }
    for (auto &part : this->footer_parts) {
        if (!found && part != nullptr) {
            found = select_revision_story(
                {&part->xml, part->xml.child("w:ftr"), part->entry_name},
                remaining_index, story, local_revision_index);
        }
    }
    if (!found) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision index is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    const auto mutated = mutate_single_revision_story(
        story, local_revision_index, *this->xml_handle_lifetime,
        this->last_error_info, [&](pugi::xml_node revision) {
            if (metadata.author.has_value()) {
                if (!ensure_attribute_value(revision, "w:author",
                                            *metadata.author)) {
                    return false;
                }
            } else if (metadata.clear_author) {
                revision.remove_attribute("w:author");
            }
            if (metadata.date.has_value()) {
                if (!ensure_attribute_value(revision, "w:date",
                                            *metadata.date)) {
                    return false;
                }
            } else if (metadata.clear_date) {
                revision.remove_attribute("w:date");
            }
            return true;
        });
    if (!mutated) {
        return false;
    }

    this->rebind_internal_xml_handles();
    this->last_error_info.clear();
    return true;
}

bool Document::accept_revision(std::size_t revision_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accepting a revision",
                       std::string{document_xml_entry});
        return false;
    }

    auto remaining_index = revision_index;
    auto local_revision_index = std::size_t{0U};
    auto story = revision_story_view{};
    auto found = select_revision_story(
        {&this->document,
         this->document.child("w:document").child("w:body"),
         document_xml_entry},
        remaining_index, story, local_revision_index);
    for (auto &part : this->header_parts) {
        if (!found && part != nullptr) {
            found = select_revision_story(
                {&part->xml, part->xml.child("w:hdr"), part->entry_name},
                remaining_index, story, local_revision_index);
        }
    }
    for (auto &part : this->footer_parts) {
        if (!found && part != nullptr) {
            found = select_revision_story(
                {&part->xml, part->xml.child("w:ftr"), part->entry_name},
                remaining_index, story, local_revision_index);
        }
    }
    if (!found) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision index is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (!mutate_single_revision_story(
            story, local_revision_index, *this->xml_handle_lifetime,
            this->last_error_info, [](pugi::xml_node revision) {
                return apply_revision_decision(revision, true);
            })) {
        return false;
    }
    this->rebind_internal_xml_handles();
    this->last_error_info.clear();
    return true;
}

bool Document::reject_revision(std::size_t revision_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before rejecting a revision",
                       std::string{document_xml_entry});
        return false;
    }

    auto remaining_index = revision_index;
    auto local_revision_index = std::size_t{0U};
    auto story = revision_story_view{};
    auto found = select_revision_story(
        {&this->document,
         this->document.child("w:document").child("w:body"),
         document_xml_entry},
        remaining_index, story, local_revision_index);
    for (auto &part : this->header_parts) {
        if (!found && part != nullptr) {
            found = select_revision_story(
                {&part->xml, part->xml.child("w:hdr"), part->entry_name},
                remaining_index, story, local_revision_index);
        }
    }
    for (auto &part : this->footer_parts) {
        if (!found && part != nullptr) {
            found = select_revision_story(
                {&part->xml, part->xml.child("w:ftr"), part->entry_name},
                remaining_index, story, local_revision_index);
        }
    }
    if (!found) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision index is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (!mutate_single_revision_story(
            story, local_revision_index, *this->xml_handle_lifetime,
            this->last_error_info, [](pugi::xml_node revision) {
                return apply_revision_decision(revision, false);
            })) {
        return false;
    }
    this->rebind_internal_xml_handles();
    this->last_error_info.clear();
    return true;
}

std::size_t Document::accept_all_revisions() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accepting revisions",
                       std::string{document_xml_entry});
        return 0U;
    }

    std::vector<revision_story_work> stories;
    auto current_entry = std::string_view{document_xml_entry};
    try {
        stories.reserve(1U + this->header_parts.size() +
                        this->footer_parts.size());
        append_revision_story_work(
            stories,
            {&this->document,
             this->document.child("w:document").child("w:body"),
             document_xml_entry});
        for (auto &part : this->header_parts) {
            if (part != nullptr) {
                current_entry = part->entry_name;
                append_revision_story_work(
                    stories,
                    {&part->xml, part->xml.child("w:hdr"), part->entry_name});
            }
        }
        for (auto &part : this->footer_parts) {
            if (part != nullptr) {
                current_entry = part->entry_name;
                append_revision_story_work(
                    stories,
                    {&part->xml, part->xml.child("w:ftr"), part->entry_name});
            }
        }
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "revision batch planning ran out of memory", current_entry);
        return 0U;
    }

    const auto accepted = mutate_all_revision_stories(
        stories, true, *this->xml_handle_lifetime, this->last_error_info);
    if (!accepted.has_value()) {
        return 0U;
    }
    if (!stories.empty()) {
        this->rebind_internal_xml_handles();
    }

    this->last_error_info.clear();
    return *accepted;
}

std::size_t Document::reject_all_revisions() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before rejecting revisions",
                       std::string{document_xml_entry});
        return 0U;
    }

    std::vector<revision_story_work> stories;
    auto current_entry = std::string_view{document_xml_entry};
    try {
        stories.reserve(1U + this->header_parts.size() +
                        this->footer_parts.size());
        append_revision_story_work(
            stories,
            {&this->document,
             this->document.child("w:document").child("w:body"),
             document_xml_entry});
        for (auto &part : this->header_parts) {
            if (part != nullptr) {
                current_entry = part->entry_name;
                append_revision_story_work(
                    stories,
                    {&part->xml, part->xml.child("w:hdr"), part->entry_name});
            }
        }
        for (auto &part : this->footer_parts) {
            if (part != nullptr) {
                current_entry = part->entry_name;
                append_revision_story_work(
                    stories,
                    {&part->xml, part->xml.child("w:ftr"), part->entry_name});
            }
        }
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "revision batch planning ran out of memory", current_entry);
        return 0U;
    }

    const auto rejected = mutate_all_revision_stories(
        stories, false, *this->xml_handle_lifetime, this->last_error_info);
    if (!rejected.has_value()) {
        return 0U;
    }
    if (!stories.empty()) {
        this->rebind_internal_xml_handles();
    }

    this->last_error_info.clear();
    return *rejected;
}
