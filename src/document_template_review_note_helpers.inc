auto summarize_notes_from_part(pugi::xml_document &xml_document,
                               featherdoc::review_note_kind kind)
    -> std::vector<featherdoc::review_note_summary> {
    std::vector<featherdoc::review_note_summary> summaries;
    const char *node_name = kind == featherdoc::review_note_kind::footnote
                                ? "w:footnote"
                                : "w:endnote";
    const auto root = kind == featherdoc::review_note_kind::footnote
                          ? xml_document.child("w:footnotes")
                          : xml_document.child("w:endnotes");
    for (auto note = root.child(node_name); note != pugi::xml_node{};
         note = note.next_sibling(node_name)) {
        const auto type = std::string_view{note.attribute("w:type").value()};
        if (type == "separator" || type == "continuationSeparator") {
            continue;
        }
        featherdoc::review_note_summary summary;
        summary.index = summaries.size();
        summary.kind = kind;
        summary.id = note.attribute("w:id").value();
        summary.text = featherdoc::detail::collect_plain_text_from_xml(note);
        summaries.push_back(std::move(summary));
    }
    return summaries;
}

auto summarize_comments_from_part(pugi::xml_document &xml_document)
    -> std::vector<featherdoc::review_note_summary> {
    std::vector<featherdoc::review_note_summary> summaries;
    const auto root = xml_document.child("w:comments");
    for (auto comment = root.child("w:comment"); comment != pugi::xml_node{};
         comment = comment.next_sibling("w:comment")) {
        featherdoc::review_note_summary summary;
        summary.index = summaries.size();
        summary.kind = featherdoc::review_note_kind::comment;
        summary.id = comment.attribute("w:id").value();
        summary.author = xml_child_attribute_string(comment, "w:author");
        summary.initials = xml_child_attribute_string(comment, "w:initials");
        summary.date = xml_child_attribute_string(comment, "w:date");
        summary.text = featherdoc::detail::collect_plain_text_from_xml(comment);
        summaries.push_back(std::move(summary));
    }
    return summaries;
}

class review_package_transaction final {
  public:
    review_package_transaction(
        std::initializer_list<pugi::xml_document *> documents,
        std::initializer_list<bool *> flags,
        std::initializer_list<std::string *> strings) {
        this->documents_.reserve(documents.size());
        for (auto *document : documents) {
            this->documents_.emplace_back();
            auto &snapshot = this->documents_.back();
            snapshot.target = document;
            if (document == nullptr ||
                featherdoc::detail::checked_clone_xml_document(
                    *document, snapshot.value) !=
                    featherdoc::detail::xml_document_clone_status::success) {
                return;
            }
        }

        this->flags_.reserve(flags.size());
        for (auto *flag : flags) {
            if (flag == nullptr) {
                return;
            }
            this->flags_.push_back({flag, *flag});
        }

        this->strings_.reserve(strings.size());
        for (auto *string : strings) {
            if (string == nullptr) {
                return;
            }
            this->strings_.push_back({string, *string});
        }
        this->ready_ = true;
        this->armed_ = true;
    }

    review_package_transaction(const review_package_transaction &) = delete;
    auto operator=(const review_package_transaction &)
        -> review_package_transaction & = delete;

    ~review_package_transaction() {
        if (this->armed_) {
            this->rollback();
        }
    }

    [[nodiscard]] bool ready() const noexcept { return this->ready_; }

    void commit() noexcept { this->armed_ = false; }

  private:
    struct document_snapshot {
        pugi::xml_document *target{nullptr};
        pugi::xml_document value;
    };

    struct flag_snapshot {
        bool *target{nullptr};
        bool value{false};
    };

    struct string_snapshot {
        std::string *target{nullptr};
        std::string value;
    };

    void rollback() noexcept {
        static_assert(noexcept(std::declval<pugi::xml_document &>() =
                               std::declval<pugi::xml_document &&>()));
        for (auto &snapshot : this->documents_) {
            *snapshot.target = std::move(snapshot.value);
        }
        for (const auto &snapshot : this->flags_) {
            *snapshot.target = snapshot.value;
        }
        for (auto &snapshot : this->strings_) {
            snapshot.target->swap(snapshot.value);
        }
        this->armed_ = false;
    }

    std::vector<document_snapshot> documents_;
    std::vector<flag_snapshot> flags_;
    std::vector<string_snapshot> strings_;
    bool ready_{false};
    bool armed_{false};
};

class review_node_rollback_guard final {
  public:
    review_node_rollback_guard() = default;
    review_node_rollback_guard(const review_node_rollback_guard &) = delete;
    auto operator=(const review_node_rollback_guard &)
        -> review_node_rollback_guard & = delete;

    ~review_node_rollback_guard() {
        if (this->armed_ && this->parent_ != pugi::xml_node{} &&
            this->node_ != pugi::xml_node{} &&
            this->node_.parent() == this->parent_) {
            (void)this->parent_.remove_child(this->node_);
        }
    }

    void track(pugi::xml_node parent, pugi::xml_node node) noexcept {
        this->parent_ = parent;
        this->node_ = node;
        this->armed_ = node != pugi::xml_node{};
    }

    void commit() noexcept { this->armed_ = false; }

  private:
    pugi::xml_node parent_;
    pugi::xml_node node_;
    bool armed_{false};
};

bool review_document_relationship_matches(
    const pugi::xml_document &relationships_document,
    std::string_view relationship_type, std::string_view target) noexcept {
    const auto relationships = featherdoc::detail::package_relationships_root(
        relationships_document);
    for (auto relationship =
             featherdoc::detail::first_package_relationship(relationships);
         relationship != pugi::xml_node{};
         relationship =
             featherdoc::detail::next_package_relationship(relationship)) {
        if (std::string_view{relationship.attribute("Type").value()} ==
                relationship_type &&
            std::string_view{relationship.attribute("Target").value()} ==
                target &&
            !std::string_view{relationship.attribute("Id").value()}.empty()) {
            return true;
        }
    }
    return false;
}

bool review_content_type_override_matches(
    const pugi::xml_document &content_types, std::string_view part_name,
    std::string_view content_type) {
    const auto normalized_part_name =
        featherdoc::detail::make_package_content_type_part_name(part_name);
    const auto types =
        featherdoc::detail::package_content_types_root(content_types);
    for (auto override_node =
             featherdoc::detail::first_package_content_type_override(types);
         override_node != pugi::xml_node{};
         override_node = featherdoc::detail::next_package_content_type_override(
             override_node)) {
        if (featherdoc::detail::content_type_part_names_equivalent(
                override_node.attribute("PartName").value(),
                normalized_part_name) &&
            std::string_view{override_node.attribute("ContentType").value()} ==
                content_type) {
            return true;
        }
    }
    return false;
}

void collect_comment_anchor_text_by_id(
    pugi::xml_node node,
    std::unordered_map<std::string, std::string> &anchor_text_by_id,
    std::vector<std::string> &active_comment_ids) {
    for (auto current = node.first_child(); current != pugi::xml_node{};) {
        const auto name = std::string_view{current.name()};
        auto descend_into_children = true;
        if (name == "w:commentRangeStart") {
            const auto id = std::string{current.attribute("w:id").value()};
            if (!id.empty()) {
                active_comment_ids.push_back(id);
                anchor_text_by_id.try_emplace(id, std::string{});
            }
            descend_into_children = false;
        } else if (name == "w:commentRangeEnd") {
            const auto id = std::string{current.attribute("w:id").value()};
            if (!id.empty()) {
                const auto match =
                    std::find(active_comment_ids.rbegin(),
                              active_comment_ids.rend(), id);
                if (match != active_comment_ids.rend()) {
                    active_comment_ids.erase(std::next(match).base());
                }
            }
            descend_into_children = false;
        } else if (!active_comment_ids.empty()) {
            if (name == "w:t" || name == "w:delText") {
                for (const auto &active_comment_id : active_comment_ids) {
                    anchor_text_by_id[active_comment_id] += current.text().get();
                }
            } else if (name == "w:tab") {
                for (const auto &active_comment_id : active_comment_ids) {
                    anchor_text_by_id[active_comment_id].push_back('\t');
                }
            } else if (name == "w:br" || name == "w:cr") {
                for (const auto &active_comment_id : active_comment_ids) {
                    anchor_text_by_id[active_comment_id].push_back('\n');
                }
            }
        }

        current = featherdoc::detail::next_xml_node_preorder(
            node, current, descend_into_children);
    }
}

auto comment_anchor_text_by_id_from_part(pugi::xml_node root)
    -> std::unordered_map<std::string, std::string> {
    std::unordered_map<std::string, std::string> anchor_text_by_id;
    std::vector<std::string> active_comment_ids;
    collect_comment_anchor_text_by_id(root, anchor_text_by_id,
                                      active_comment_ids);
    return anchor_text_by_id;
}

void merge_comment_anchor_text_by_id(
    std::unordered_map<std::string, std::string> &target, pugi::xml_node root) {
    auto source = comment_anchor_text_by_id_from_part(root);
    for (auto &[id, anchor_text] : source) {
        if (anchor_text.empty()) {
            target.try_emplace(std::move(id), std::string{});
            continue;
        }
        auto [position, inserted] = target.try_emplace(id, anchor_text);
        if (!inserted && position->second.empty()) {
            position->second = anchor_text;
        }
    }
}

void attach_comment_anchor_text(
    std::vector<featherdoc::review_note_summary> &summaries,
    const std::unordered_map<std::string, std::string> &anchor_text_by_id) {
    for (auto &summary : summaries) {
        const auto match = anchor_text_by_id.find(summary.id);
        if (match != anchor_text_by_id.end()) {
            summary.anchor_text = match->second;
        }
    }
}

pugi::xml_node comment_by_summary_index(pugi::xml_node root, std::size_t comment_index) {
    std::size_t current_index = 0;
    for (auto comment = root.child("w:comment"); comment != pugi::xml_node{};
         comment = comment.next_sibling("w:comment")) {
        if (current_index == comment_index) {
            return comment;
        }
        ++current_index;
    }
    return {};
}

bool on_off_attribute_is_true(std::string_view value) {
    return value == "1" || value == "true" || value == "on";
}

std::string uppercase_hex_string(std::string_view value) {
    std::string result;
    result.reserve(value.size());
    for (const char ch : value) {
        result.push_back(static_cast<char>(
            std::toupper(static_cast<unsigned char>(ch))));
    }
    return result;
}

std::string format_hex8(std::uint32_t value) {
    constexpr auto digits = std::string_view{"0123456789ABCDEF"};
    std::string result(8U, '0');
    for (std::size_t index = 0U; index < result.size(); ++index) {
        const auto shift = static_cast<unsigned>((result.size() - index - 1U) * 4U);
        result[index] = digits[(value >> shift) & 0x0FU];
    }
    return result;
}

auto comment_paragraph_id(pugi::xml_node comment) -> std::optional<std::string> {
    const auto paragraph = comment.child("w:p");
    if (paragraph == pugi::xml_node{}) {
        return std::nullopt;
    }
    auto value = std::string_view{paragraph.attribute("w14:paraId").value()};
    if (value.empty()) {
        value = std::string_view{paragraph.attribute("w15:paraId").value()};
    }
    if (value.empty()) {
        return std::nullopt;
    }
    return uppercase_hex_string(value);
}

auto next_comment_paragraph_id(pugi::xml_node comments_root) -> std::string {
    std::unordered_set<std::string> used_ids;
    for (auto comment = comments_root.child("w:comment");
         comment != pugi::xml_node{};
         comment = comment.next_sibling("w:comment")) {
        if (const auto id = comment_paragraph_id(comment); id.has_value()) {
            used_ids.insert(*id);
        }
    }

    for (std::uint32_t value = 1U;; ++value) {
        auto candidate = format_hex8(value);
        if (!used_ids.contains(candidate)) {
            return candidate;
        }
    }
}

bool review_whitespace_token_list_contains(std::string_view value,
                                           std::string_view token) noexcept {
    const auto is_whitespace = [](char character) noexcept {
        return character == ' ' || character == '\t' || character == '\r' ||
               character == '\n';
    };
    std::size_t offset = 0U;
    while (offset < value.size()) {
        while (offset < value.size() && is_whitespace(value[offset])) {
            ++offset;
        }
        const auto begin = offset;
        while (offset < value.size() && !is_whitespace(value[offset])) {
            ++offset;
        }
        if (value.substr(begin, offset - begin) == token) {
            return true;
        }
    }
    return false;
}

bool checked_ensure_review_ignorable_namespace(pugi::xml_node root,
                                               std::string_view prefix) {
    ensure_markup_compatibility_ignorable(root, prefix);
    return std::string_view{root.attribute("xmlns:mc").value()} ==
               markup_compatibility_namespace_uri &&
           review_whitespace_token_list_contains(
               root.attribute("mc:Ignorable").value(), prefix);
}

auto ensure_comment_paragraph_id(pugi::xml_node comments_root,
                                 pugi::xml_node comment, bool &comments_dirty)
    -> std::optional<std::string> {
    if (const auto existing = comment_paragraph_id(comment); existing.has_value()) {
        return existing;
    }

    auto paragraph = comment.child("w:p");
    if (paragraph == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto para_id = next_comment_paragraph_id(comments_root);
    ensure_wordprocessingml_2010_namespace(comments_root);
    if (std::string_view{comments_root.attribute("xmlns:w14").value()} !=
            wordprocessingml_2010_namespace_uri ||
        !checked_ensure_review_ignorable_namespace(comments_root, "w14") ||
        !ensure_attribute_value(paragraph, "w14:paraId", para_id)) {
        return std::nullopt;
    }
    comments_dirty = true;
    return para_id;
}

auto comment_paragraph_id_can_be_ensured(pugi::xml_node comment) -> bool {
    return comment != pugi::xml_node{} &&
           comment.child("w:p") != pugi::xml_node{};
}

struct comment_extended_metadata {
    bool has_done{false};
    bool done{false};
    std::optional<std::string> parent_para_id;
};

auto comment_metadata_by_paragraph_id(const pugi::xml_document &comments_extended)
    -> std::unordered_map<std::string, comment_extended_metadata> {
    std::unordered_map<std::string, comment_extended_metadata> metadata_by_para_id;
    const auto root = comments_extended.child("w15:commentsEx");
    for (auto comment_ex = root.child("w15:commentEx");
         comment_ex != pugi::xml_node{};
         comment_ex = comment_ex.next_sibling("w15:commentEx")) {
        const auto para_id =
            std::string_view{comment_ex.attribute("w15:paraId").value()};
        if (para_id.empty()) {
            continue;
        }
        comment_extended_metadata metadata;
        const auto done =
            std::string_view{comment_ex.attribute("w15:done").value()};
        if (!done.empty()) {
            metadata.has_done = true;
            metadata.done = on_off_attribute_is_true(done);
        }
        const auto parent_para_id =
            std::string_view{comment_ex.attribute("w15:paraIdParent").value()};
        if (!parent_para_id.empty()) {
            metadata.parent_para_id = uppercase_hex_string(parent_para_id);
        }
        metadata_by_para_id[uppercase_hex_string(para_id)] = std::move(metadata);
    }
    return metadata_by_para_id;
}

void attach_comment_extended_metadata(
    std::vector<featherdoc::review_note_summary> &summaries,
    const pugi::xml_document &comments_document,
    const pugi::xml_document &comments_extended) {
    if (summaries.empty()) {
        return;
    }
    const auto metadata_by_para_id =
        comment_metadata_by_paragraph_id(comments_extended);
    if (metadata_by_para_id.empty()) {
        return;
    }

    const auto root = comments_document.child("w:comments");
    std::vector<std::optional<std::string>> para_ids_by_index(summaries.size());
    std::unordered_map<std::string, std::size_t> index_by_para_id;
    std::unordered_map<std::string, std::string> id_by_para_id;
    std::size_t index = 0U;
    for (auto comment = root.child("w:comment");
         comment != pugi::xml_node{} && index < summaries.size();
         comment = comment.next_sibling("w:comment"), ++index) {
        const auto para_id = comment_paragraph_id(comment);
        if (!para_id.has_value()) {
            continue;
        }
        para_ids_by_index[index] = *para_id;
        index_by_para_id[*para_id] = index;
        id_by_para_id[*para_id] = summaries[index].id;
    }

    for (std::size_t summary_index = 0U; summary_index < summaries.size();
         ++summary_index) {
        const auto &para_id = para_ids_by_index[summary_index];
        if (!para_id.has_value()) {
            continue;
        }
        const auto match = metadata_by_para_id.find(*para_id);
        if (match == metadata_by_para_id.end()) {
            continue;
        }
        const auto &metadata = match->second;
        if (metadata.has_done) {
            summaries[summary_index].resolved = metadata.done;
        }
        if (!metadata.parent_para_id.has_value()) {
            continue;
        }
        if (const auto parent_index =
                index_by_para_id.find(*metadata.parent_para_id);
            parent_index != index_by_para_id.end()) {
            summaries[summary_index].parent_index = parent_index->second;
        }
        if (const auto parent_id = id_by_para_id.find(*metadata.parent_para_id);
            parent_id != id_by_para_id.end()) {
            summaries[summary_index].parent_id = parent_id->second;
        }
    }
}

auto comment_extended_by_paragraph_id(pugi::xml_node root, std::string_view para_id)
    -> pugi::xml_node {
    const auto normalized_para_id = uppercase_hex_string(para_id);
    for (auto comment_ex = root.child("w15:commentEx");
         comment_ex != pugi::xml_node{};
         comment_ex = comment_ex.next_sibling("w15:commentEx")) {
        const auto candidate =
            std::string_view{comment_ex.attribute("w15:paraId").value()};
        if (uppercase_hex_string(candidate) == normalized_para_id) {
            return comment_ex;
        }
    }
    return {};
}

auto ensure_comment_extended_by_paragraph_id(pugi::xml_node root,
                                             std::string_view para_id)
    -> pugi::xml_node {
    auto comment_ex = comment_extended_by_paragraph_id(root, para_id);
    const bool created = comment_ex == pugi::xml_node{};
    if (comment_ex == pugi::xml_node{}) {
        comment_ex = featherdoc::detail::checked_append_xml_element(
            root, "w15:commentEx");
    }
    if (comment_ex == pugi::xml_node{} ||
        !ensure_attribute_value(comment_ex, "w15:paraId",
                                uppercase_hex_string(para_id))) {
        if (created && comment_ex != pugi::xml_node{}) {
            (void)root.remove_child(comment_ex);
        }
        return {};
    }
    return comment_ex;
}

bool remove_comment_extended_by_paragraph_id(pugi::xml_document &comments_extended,
                                             std::string_view para_id) {
    auto root = comments_extended.child("w15:commentsEx");
    auto comment_ex = comment_extended_by_paragraph_id(root, para_id);
    if (root == pugi::xml_node{} || comment_ex == pugi::xml_node{}) {
        return false;
    }
    return root.remove_child(comment_ex);
}

auto child_comment_para_ids_by_parent_para_id(
    const pugi::xml_document &comments_extended)
    -> std::unordered_map<std::string, std::vector<std::string>> {
    std::unordered_map<std::string, std::vector<std::string>> children_by_parent;
    const auto root = comments_extended.child("w15:commentsEx");
    for (auto comment_ex = root.child("w15:commentEx");
         comment_ex != pugi::xml_node{};
         comment_ex = comment_ex.next_sibling("w15:commentEx")) {
        const auto para_id =
            std::string_view{comment_ex.attribute("w15:paraId").value()};
        const auto parent_para_id =
            std::string_view{comment_ex.attribute("w15:paraIdParent").value()};
        if (para_id.empty() || parent_para_id.empty()) {
            continue;
        }
        children_by_parent[uppercase_hex_string(parent_para_id)].push_back(
            uppercase_hex_string(para_id));
    }
    return children_by_parent;
}

auto comment_thread_para_ids(const pugi::xml_document &comments_extended,
                             std::string_view root_para_id)
    -> std::unordered_set<std::string> {
    const auto children_by_parent =
        child_comment_para_ids_by_parent_para_id(comments_extended);
    std::unordered_set<std::string> para_ids;
    std::vector<std::string> pending{uppercase_hex_string(root_para_id)};
    while (!pending.empty()) {
        auto current = std::move(pending.back());
        pending.pop_back();
        if (!para_ids.insert(current).second) {
            continue;
        }
        const auto children = children_by_parent.find(current);
        if (children == children_by_parent.end()) {
            continue;
        }
        pending.insert(pending.end(), children->second.begin(),
                       children->second.end());
    }
    return para_ids;
}

struct review_node_removal_plan {
    std::vector<pugi::xml_node> matched_nodes;
    std::vector<pugi::xml_node> candidate_runs;
    std::unordered_set<const void *> candidate_run_keys;
    std::vector<pugi::xml_node> standalone_nodes;
    std::vector<pugi::xml_node> removal_runs;
};

bool node_is_within_any_review_root(
    pugi::xml_node node, std::span<const pugi::xml_node> roots) {
    for (auto current = node; current != pugi::xml_node{};
         current = current.parent()) {
        for (const auto root : roots) {
            if (current == root) {
                return true;
            }
        }
    }
    return false;
}

template <typename MatchesNode>
void collect_review_nodes_for_removal(
    review_node_removal_plan &plan, pugi::xml_node root,
    std::string_view run_removing_node_name, MatchesNode matches_node,
    std::span<const pugi::xml_node> excluded_roots = {}) {
    for (auto current = root.first_child(); current != pugi::xml_node{};) {
        const auto excluded = node_is_within_any_review_root(
            current, excluded_roots);
        const auto matches = !excluded && matches_node(current);
        if (matches) {
            plan.matched_nodes.push_back(current);
            if (std::string_view{current.name()} == run_removing_node_name) {
                const auto parent = current.parent();
                if (std::string_view{parent.name()} == "w:r" &&
                    plan.candidate_run_keys.insert(parent.internal_object())
                        .second) {
                    plan.candidate_runs.push_back(parent);
                }
            }
        }
        current = featherdoc::detail::next_xml_node_preorder(
            root, current, !matches && !excluded);
    }
}

bool finalize_review_node_removal_plan(review_node_removal_plan &plan) {
    plan.standalone_nodes.clear();
    plan.removal_runs.clear();
    plan.standalone_nodes.reserve(plan.matched_nodes.size());
    plan.removal_runs.reserve(plan.candidate_runs.size());

    for (const auto run : plan.candidate_runs) {
        auto nested_in_scheduled_run = false;
        for (auto ancestor = run.parent(); ancestor != pugi::xml_node{};
             ancestor = ancestor.parent()) {
            if (plan.candidate_run_keys.contains(
                    ancestor.internal_object())) {
                nested_in_scheduled_run = true;
                break;
            }
        }
        if (!nested_in_scheduled_run) {
            plan.removal_runs.push_back(run);
        }
    }

    for (const auto node : plan.matched_nodes) {
        auto removed_with_run = false;
        for (auto ancestor = node.parent(); ancestor != pugi::xml_node{};
             ancestor = ancestor.parent()) {
            if (plan.candidate_run_keys.contains(
                    ancestor.internal_object())) {
                removed_with_run = true;
                break;
            }
        }
        if (!removed_with_run) {
            plan.standalone_nodes.push_back(node);
        }
    }

    const auto has_parent = [](pugi::xml_node node) {
        return node != pugi::xml_node{} &&
               node.parent() != pugi::xml_node{};
    };
    return std::ranges::all_of(plan.standalone_nodes, has_parent) &&
           std::ranges::all_of(plan.removal_runs, has_parent);
}

bool publish_review_node_removal_plan(
    const review_node_removal_plan &plan) noexcept {
    for (const auto node : plan.standalone_nodes) {
        auto parent = node.parent();
        if (parent == pugi::xml_node{} || !parent.remove_child(node)) {
            return false;
        }
    }
    for (const auto run : plan.removal_runs) {
        auto parent = run.parent();
        if (parent == pugi::xml_node{} || !parent.remove_child(run)) {
            return false;
        }
    }
    return true;
}

void collect_comment_markers_by_ids(
    review_node_removal_plan &plan, pugi::xml_node node,
    const std::unordered_set<std::string> &ids,
    std::span<const pugi::xml_node> excluded_roots = {}) {
    collect_review_nodes_for_removal(
        plan, node, "w:commentReference",
        [&ids](pugi::xml_node current) {
            const auto name = std::string_view{current.name()};
            return (name == "w:commentRangeStart" ||
                    name == "w:commentRangeEnd" ||
                    name == "w:commentReference") &&
                   ids.contains(current.attribute("w:id").value());
        },
        excluded_roots);
}

bool remove_comment_markers_by_id(pugi::xml_node node, std::string_view id) {
    auto ids = std::unordered_set<std::string>{std::string{id}};
    auto plan = review_node_removal_plan{};
    collect_comment_markers_by_ids(plan, node, ids);
    return finalize_review_node_removal_plan(plan) &&
           publish_review_node_removal_plan(plan);
}

bool append_review_separator(pugi::xml_node root, const char *note_node_name,
                             const char *separator_child_name, std::string_view type,
                             std::string_view id) {
    auto note = featherdoc::detail::checked_append_xml_element(
        root, note_node_name);
    auto paragraph = note == pugi::xml_node{}
                         ? pugi::xml_node{}
                         : featherdoc::detail::checked_append_xml_element(
                               note, "w:p");
    auto run = paragraph == pugi::xml_node{}
                   ? pugi::xml_node{}
                   : featherdoc::detail::checked_append_xml_element(paragraph,
                                                                     "w:r");
    auto separator =
        run == pugi::xml_node{}
            ? pugi::xml_node{}
            : featherdoc::detail::checked_append_xml_element(
                  run, separator_child_name);
    if (separator == pugi::xml_node{} ||
        !ensure_attribute_value(note, "w:type", type) ||
        !ensure_attribute_value(note, "w:id", id)) {
        if (note != pugi::xml_node{}) {
            (void)root.remove_child(note);
        }
        return false;
    }
    return true;
}

bool initialize_review_notes_document(pugi::xml_document &xml_document,
                                      featherdoc::review_note_kind kind) {
    xml_document.reset();
    const bool footnote = kind == featherdoc::review_note_kind::footnote;
    auto root = xml_document.append_child(footnote ? "w:footnotes" : "w:endnotes");
    if (root == pugi::xml_node{}) {
        return false;
    }
    if (!ensure_attribute_value(root, "xmlns:w",
                                wordprocessingml_namespace_uri)) {
        return false;
    }
    const char *note_node_name = footnote ? "w:footnote" : "w:endnote";
    return append_review_separator(root, note_node_name, "w:separator",
                                   "separator", "-1") &&
           append_review_separator(root, note_node_name,
                                   "w:continuationSeparator",
                                   "continuationSeparator", "0");
}

bool initialize_comments_document(pugi::xml_document &xml_document) {
    xml_document.reset();
    auto root = xml_document.append_child("w:comments");
    if (root == pugi::xml_node{}) {
        return false;
    }
    return ensure_attribute_value(root, "xmlns:w",
                                  wordprocessingml_namespace_uri);
}

bool initialize_comments_extended_document(pugi::xml_document &xml_document) {
    xml_document.reset();
    auto root = xml_document.append_child("w15:commentsEx");
    if (root == pugi::xml_node{}) {
        return false;
    }
    ensure_wordprocessingml_2012_namespace(root);
    return std::string_view{root.attribute("xmlns:w15").value()} ==
               wordprocessingml_2012_namespace_uri &&
           checked_ensure_review_ignorable_namespace(root, "w15");
}

auto next_numeric_id(pugi::xml_node root, const char *node_name,
                     review_identifier minimum_id)
    -> std::optional<review_identifier> {
    review_identifier_allocator allocator{minimum_id};
    for (auto node = root.child(node_name); node != pugi::xml_node{};
         node = node.next_sibling(node_name)) {
        const auto value_text = std::string_view{node.attribute("w:id").value()};
        if (value_text.empty()) {
            continue;
        }
        allocator.observe(value_text);
    }
    return allocator.reserve(1U);
}

pugi::xml_node note_by_summary_index(pugi::xml_node root,
                                     featherdoc::review_note_kind kind,
                                     std::size_t note_index) {
    const char *node_name = kind == featherdoc::review_note_kind::footnote
                                ? "w:footnote"
                                : "w:endnote";
    std::size_t current_index = 0U;
    for (auto note = root.child(node_name); note != pugi::xml_node{};
         note = note.next_sibling(node_name)) {
        const auto type = std::string_view{note.attribute("w:type").value()};
        if (type == "separator" || type == "continuationSeparator") {
            continue;
        }
        if (current_index == note_index) {
            return note;
        }
        ++current_index;
    }
    return {};
}

bool set_review_note_text(pugi::xml_node note, const char *reference_child_name,
                          std::string_view note_text) {
    clear_children(note);
    auto paragraph = note.append_child("w:p");
    auto reference_run = paragraph.append_child("w:r");
    auto text_run = paragraph.append_child("w:r");
    if (paragraph == pugi::xml_node{} || reference_run == pugi::xml_node{} ||
        text_run == pugi::xml_node{} ||
        reference_run.append_child(reference_child_name) == pugi::xml_node{}) {
        return false;
    }
    return featherdoc::detail::set_plain_text_run_content(text_run, note_text);
}

bool set_comment_text(pugi::xml_node comment, std::string_view comment_text) {
    clear_children(comment);
    auto paragraph = comment.append_child("w:p");
    auto run = paragraph.append_child("w:r");
    if (paragraph == pugi::xml_node{} || run == pugi::xml_node{}) {
        return false;
    }
    return featherdoc::detail::set_plain_text_run_content(run, comment_text);
}

bool checked_append_review_text_node(pugi::xml_node run,
                                     std::string_view text) {
    auto text_node =
        featherdoc::detail::checked_append_xml_element(run, "w:t");
    if (text_node == pugi::xml_node{}) {
        return false;
    }
    if (!text.empty() &&
        (std::isspace(static_cast<unsigned char>(text.front())) != 0 ||
         std::isspace(static_cast<unsigned char>(text.back())) != 0) &&
        !ensure_attribute_value(text_node, "xml:space", "preserve")) {
        return false;
    }
    auto text_value = text_node.append_child(pugi::node_pcdata);
    return text_value != pugi::xml_node{} &&
           text_value.set_value(text.data(), text.size());
}

bool checked_set_review_plain_text_run_content(pugi::xml_node run,
                                               std::string_view text) {
    if (run == pugi::xml_node{}) {
        return false;
    }
    for (auto child = run.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        if (std::string_view{child.name()} != "w:rPr") {
            if (!run.remove_child(child)) {
                return false;
            }
        }
        child = next;
    }

    if (text.empty()) {
        return checked_append_review_text_node(run, {});
    }

    std::size_t segment_start = 0U;
    std::size_t offset = 0U;
    while (offset < text.size()) {
        const auto current = text[offset];
        if (current != '\n' && current != '\r') {
            ++offset;
            continue;
        }
        if (offset > segment_start &&
            !checked_append_review_text_node(
                run, text.substr(segment_start, offset - segment_start))) {
            return false;
        }
        if (featherdoc::detail::checked_append_xml_element(run, "w:br") ==
            pugi::xml_node{}) {
            return false;
        }
        if (current == '\r' && offset + 1U < text.size() &&
            text[offset + 1U] == '\n') {
            ++offset;
        }
        ++offset;
        segment_start = offset;
    }
    return segment_start == text.size() ||
           checked_append_review_text_node(run, text.substr(segment_start));
}

pugi::xml_node checked_append_review_plain_text_run(pugi::xml_node parent,
                                                    std::string_view text) {
    auto run =
        featherdoc::detail::checked_append_xml_element(parent, "w:r");
    if (run == pugi::xml_node{} ||
        !checked_set_review_plain_text_run_content(run, text)) {
        if (run != pugi::xml_node{}) {
            (void)parent.remove_child(run);
        }
        return {};
    }
    return run;
}

pugi::xml_node checked_append_review_body_paragraph(pugi::xml_node body) {
    if (body == pugi::xml_node{}) {
        return {};
    }
    const auto section_properties = body.child("w:sectPr");
    return section_properties == pugi::xml_node{}
               ? featherdoc::detail::checked_append_xml_element(body, "w:p")
               : featherdoc::detail::checked_insert_xml_element_before(
                     body, "w:p", section_properties);
}

bool checked_set_comment_text(pugi::xml_node comment,
                              std::string_view comment_text) {
    if (comment == pugi::xml_node{}) {
        return false;
    }
    if (!comment.remove_children()) {
        return false;
    }
    auto paragraph =
        featherdoc::detail::checked_append_xml_element(comment, "w:p");
    auto run = paragraph == pugi::xml_node{}
                   ? pugi::xml_node{}
                   : featherdoc::detail::checked_append_xml_element(paragraph,
                                                                     "w:r");
    return run != pugi::xml_node{} &&
           checked_set_review_plain_text_run_content(run, comment_text);
}

bool checked_set_review_note_text(pugi::xml_node note,
                                  const char *reference_child_name,
                                  std::string_view note_text) {
    if (note == pugi::xml_node{} || reference_child_name == nullptr ||
        !note.remove_children()) {
        return false;
    }
    auto paragraph =
        featherdoc::detail::checked_append_xml_element(note, "w:p");
    auto reference_run =
        paragraph == pugi::xml_node{}
            ? pugi::xml_node{}
            : featherdoc::detail::checked_append_xml_element(paragraph,
                                                              "w:r");
    auto reference =
        reference_run == pugi::xml_node{}
            ? pugi::xml_node{}
            : featherdoc::detail::checked_append_xml_element(
                  reference_run, reference_child_name);
    auto text_run =
        paragraph == pugi::xml_node{}
            ? pugi::xml_node{}
            : featherdoc::detail::checked_append_xml_element(paragraph,
                                                              "w:r");
    return reference != pugi::xml_node{} && text_run != pugi::xml_node{} &&
           checked_set_review_plain_text_run_content(text_run, note_text);
}

pugi::xml_node checked_append_review_note(
    pugi::xml_node root, const char *note_node_name,
    const char *reference_child_name, std::string_view note_id,
    std::string_view note_text) {
    auto note = featherdoc::detail::checked_append_xml_element(
        root, note_node_name);
    if (note == pugi::xml_node{} ||
        !ensure_attribute_value(note, "w:id", note_id) ||
        !checked_set_review_note_text(note, reference_child_name, note_text)) {
        if (note != pugi::xml_node{}) {
            (void)root.remove_child(note);
        }
        return {};
    }
    return note;
}

pugi::xml_node checked_insert_review_subtree_before(pugi::xml_node source) {
    auto parent = source.parent();
    if (source == pugi::xml_node{} || parent == pugi::xml_node{}) {
        return {};
    }

    auto destination_root = parent.insert_child_before(source.type(), source);
    if (destination_root == pugi::xml_node{}) {
        return {};
    }

    try {
        const auto source_root = source;
        auto destination = destination_root;
        if (!featherdoc::detail::xml_document_clone_detail::
                copy_node_contents(source, destination)) {
            parent.remove_child(destination_root);
            return {};
        }

        while (source != pugi::xml_node{}) {
            if (const auto first_child = source.first_child();
                first_child != pugi::xml_node{}) {
                source = first_child;
                auto child = destination.append_child(source.type());
                if (child == pugi::xml_node{} ||
                    !featherdoc::detail::xml_document_clone_detail::
                        copy_node_contents(source, child)) {
                    parent.remove_child(destination_root);
                    return {};
                }
                destination = child;
                continue;
            }

            while (source != source_root &&
                   source.next_sibling() == pugi::xml_node{}) {
                source = source.parent();
                destination = destination.parent();
            }
            if (source == source_root) {
                break;
            }
            source = source.next_sibling();
            auto sibling = destination.parent().append_child(source.type());
            if (sibling == pugi::xml_node{} ||
                !featherdoc::detail::xml_document_clone_detail::
                    copy_node_contents(source, sibling)) {
                parent.remove_child(destination_root);
                return {};
            }
            destination = sibling;
        }
        return destination_root;
    } catch (...) {
        parent.remove_child(destination_root);
        return {};
    }
}

struct review_paragraph_replacement {
    pugi::xml_node original;
    pugi::xml_node replacement;
};

void discard_review_paragraph_replacements(
    std::span<const review_paragraph_replacement> replacements) noexcept {
    for (const auto &replacement : replacements) {
        auto parent = replacement.replacement.parent();
        if (parent != pugi::xml_node{}) {
            (void)parent.remove_child(replacement.replacement);
        }
    }
}

bool prepare_review_paragraph_replacements(
    const paragraph_revision_range_plan &plan,
    paragraph_revision_range_plan &work_plan,
    std::vector<review_paragraph_replacement> &replacements) {
    work_plan = plan;
    replacements.clear();
    replacements.reserve(plan.segments.size());
    for (std::size_t index = 0U; index < plan.segments.size(); ++index) {
        const auto original = plan.segments[index].paragraph;
        auto replacement = checked_insert_review_subtree_before(original);
        if (replacement == pugi::xml_node{}) {
            discard_review_paragraph_replacements(replacements);
            replacements.clear();
            return false;
        }
        replacements.push_back({original, replacement});
        work_plan.segments[index].paragraph = replacement;
    }
    return true;
}

bool review_paragraph_replacements_are_publishable(
    std::span<const review_paragraph_replacement> replacements) noexcept {
    return std::ranges::all_of(
        replacements, [](const review_paragraph_replacement &replacement) {
            return replacement.original != pugi::xml_node{} &&
                   replacement.replacement != pugi::xml_node{} &&
                   replacement.original.parent() != pugi::xml_node{} &&
                   replacement.original.parent() ==
                       replacement.replacement.parent();
        });
}

bool publish_review_paragraph_replacements(
    std::span<const review_paragraph_replacement> replacements) noexcept {
    for (const auto &replacement : replacements) {
        auto parent = replacement.original.parent();
        if (parent == pugi::xml_node{} ||
            !parent.remove_child(replacement.original)) {
            return false;
        }
    }
    return true;
}

pugi::xml_node checked_insert_review_element_after(pugi::xml_node parent,
                                                   const char *name,
                                                   pugi::xml_node anchor) {
    if (parent == pugi::xml_node{} || anchor == pugi::xml_node{} ||
        anchor.parent() != parent) {
        return {};
    }
    const auto next = anchor.next_sibling();
    return next == pugi::xml_node{}
               ? featherdoc::detail::checked_append_xml_element(parent, name)
               : featherdoc::detail::checked_insert_xml_element_before(
                     parent, name, next);
}

bool checked_split_review_text_run(
    pugi::xml_node paragraph, const paragraph_revision_run_span &span,
    std::size_t text_offset) {
    if (text_offset <= span.start_offset ||
        text_offset >= span.start_offset + span.length) {
        return true;
    }
    if (!revision_range_run_content_supported(span.run)) {
        return false;
    }

    const auto local_offset = text_offset - span.start_offset;
    auto suffix = checked_insert_review_element_after(paragraph, "w:r",
                                                      span.run);
    if (suffix == pugi::xml_node{} ||
        !append_copied_run_properties(suffix, span.run) ||
        !checked_set_review_plain_text_run_content(
            suffix, std::string_view{span.text}.substr(local_offset)) ||
        !checked_set_review_plain_text_run_content(
            span.run, std::string_view{span.text}.substr(0U, local_offset))) {
        return false;
    }
    return true;
}

bool checked_split_review_text_at(pugi::xml_node paragraph,
                                  std::size_t text_offset) {
    const auto spans = collect_paragraph_revision_run_spans(paragraph);
    const auto total_length = paragraph_revision_text_length(spans);
    if (text_offset == 0U || text_offset == total_length) {
        return true;
    }
    if (text_offset > total_length) {
        return false;
    }
    for (const auto &span : spans) {
        if (text_offset > span.start_offset &&
            text_offset < span.start_offset + span.length) {
            return checked_split_review_text_run(paragraph, span,
                                                 text_offset);
        }
    }
    return true;
}

bool checked_split_review_range_segments(
    const std::vector<paragraph_revision_range_segment> &segments) {
    for (const auto &segment : segments) {
        if (!checked_split_review_text_at(
                segment.paragraph,
                segment.text_offset + segment.text_length) ||
            !checked_split_review_text_at(segment.paragraph,
                                          segment.text_offset)) {
            return false;
        }
    }
    return true;
}

bool checked_insert_comment_markers_for_selected_spans(
    const std::vector<paragraph_revision_range_segment> &segments,
    const std::vector<std::vector<paragraph_revision_run_span>>
        &selected_segments,
    std::string_view comment_id) {
    if (segments.empty() || selected_segments.empty() ||
        selected_segments.front().empty() || selected_segments.back().empty()) {
        return false;
    }

    auto first_paragraph = segments.front().paragraph;
    auto last_paragraph = segments.back().paragraph;
    auto first_run = selected_segments.front().front().run;
    auto last_run = selected_segments.back().back().run;
    auto range_start = featherdoc::detail::checked_insert_xml_element_before(
        first_paragraph, "w:commentRangeStart", first_run);
    auto range_end = checked_insert_review_element_after(
        last_paragraph, "w:commentRangeEnd", last_run);
    auto reference_run =
        range_end == pugi::xml_node{}
            ? pugi::xml_node{}
            : checked_insert_review_element_after(last_paragraph, "w:r",
                                                  range_end);
    auto reference = reference_run == pugi::xml_node{}
                         ? pugi::xml_node{}
                         : featherdoc::detail::checked_append_xml_element(
                               reference_run, "w:commentReference");
    return range_start != pugi::xml_node{} &&
           range_end != pugi::xml_node{} &&
           reference_run != pugi::xml_node{} &&
           reference != pugi::xml_node{} &&
           ensure_attribute_value(range_start, "w:id", comment_id) &&
           ensure_attribute_value(range_end, "w:id", comment_id) &&
           ensure_attribute_value(reference, "w:id", comment_id);
}

pugi::xml_node checked_append_comment(
    pugi::xml_node root, std::string_view comment_id,
    std::string_view comment_text, std::string_view author,
    std::string_view initials, std::string_view date) {
    auto comment =
        featherdoc::detail::checked_append_xml_element(root, "w:comment");
    if (comment == pugi::xml_node{} ||
        !ensure_attribute_value(comment, "w:id", comment_id) ||
        (!author.empty() &&
         !ensure_attribute_value(comment, "w:author", author)) ||
        (!initials.empty() &&
         !ensure_attribute_value(comment, "w:initials", initials)) ||
        (!date.empty() && !ensure_attribute_value(comment, "w:date", date)) ||
        !checked_set_comment_text(comment, comment_text)) {
        if (comment != pugi::xml_node{}) {
            (void)root.remove_child(comment);
        }
        return {};
    }
    return comment;
}

void collect_references_by_id(review_node_removal_plan &plan,
                              pugi::xml_node node,
                              const char *reference_node_name,
                              std::string_view id) {
    collect_review_nodes_for_removal(
        plan, node, reference_node_name,
        [reference_node_name, id](pugi::xml_node current) {
            return std::string_view{current.name()} == reference_node_name &&
                   std::string_view{current.attribute("w:id").value()} == id;
        });
}

bool remove_references_by_id(pugi::xml_node node,
                             const char *reference_node_name,
                             std::string_view id) {
    auto plan = review_node_removal_plan{};
    collect_references_by_id(plan, node, reference_node_name, id);
    return finalize_review_node_removal_plan(plan) &&
           publish_review_node_removal_plan(plan);
}
