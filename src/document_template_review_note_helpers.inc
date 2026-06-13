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

void collect_comment_anchor_text_by_id(
    pugi::xml_node node,
    std::unordered_map<std::string, std::string> &anchor_text_by_id,
    std::vector<std::string> &active_comment_ids) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto name = std::string_view{child.name()};
        if (name == "w:commentRangeStart") {
            const auto id = std::string{child.attribute("w:id").value()};
            if (!id.empty()) {
                active_comment_ids.push_back(id);
                anchor_text_by_id.try_emplace(id, std::string{});
            }
            continue;
        }

        if (name == "w:commentRangeEnd") {
            const auto id = std::string{child.attribute("w:id").value()};
            if (!id.empty()) {
                const auto match =
                    std::find(active_comment_ids.rbegin(),
                              active_comment_ids.rend(), id);
                if (match != active_comment_ids.rend()) {
                    active_comment_ids.erase(std::next(match).base());
                }
            }
            continue;
        }

        if (!active_comment_ids.empty()) {
            if (name == "w:t" || name == "w:delText") {
                for (const auto &active_comment_id : active_comment_ids) {
                    anchor_text_by_id[active_comment_id] += child.text().get();
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

        collect_comment_anchor_text_by_id(child, anchor_text_by_id,
                                          active_comment_ids);
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
    ensure_markup_compatibility_ignorable(comments_root, "w14");
    ensure_attribute_value(paragraph, "w14:paraId", para_id);
    comments_dirty = true;
    return para_id;
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
    if (comment_ex == pugi::xml_node{}) {
        comment_ex = root.append_child("w15:commentEx");
    }
    ensure_attribute_value(comment_ex, "w15:paraId", uppercase_hex_string(para_id));
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

void remove_comment_markers_by_id(pugi::xml_node node, std::string_view id) {
    for (auto child = node.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        const auto name = std::string_view{child.name()};
        const bool is_marker =
            (name == "w:commentRangeStart" || name == "w:commentRangeEnd" ||
             name == "w:commentReference") &&
            std::string_view{child.attribute("w:id").value()} == id;
        if (is_marker) {
            auto parent = child.parent();
            if (name == "w:commentReference" && std::string_view{parent.name()} == "w:r") {
                parent.parent().remove_child(parent);
            } else {
                parent.remove_child(child);
            }
        } else {
            remove_comment_markers_by_id(child, id);
        }
        child = next;
    }
}

bool append_review_separator(pugi::xml_node root, const char *note_node_name,
                             const char *separator_child_name, std::string_view type,
                             std::string_view id) {
    auto note = root.append_child(note_node_name);
    auto paragraph = note.append_child("w:p");
    auto run = paragraph.append_child("w:r");
    if (note == pugi::xml_node{} || paragraph == pugi::xml_node{} ||
        run == pugi::xml_node{} || run.append_child(separator_child_name) == pugi::xml_node{}) {
        return false;
    }
    ensure_attribute_value(note, "w:type", type);
    ensure_attribute_value(note, "w:id", id);
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
    ensure_wordprocessingml_namespace(root);
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
    ensure_wordprocessingml_namespace(root);
    return true;
}

bool initialize_comments_extended_document(pugi::xml_document &xml_document) {
    xml_document.reset();
    auto root = xml_document.append_child("w15:commentsEx");
    if (root == pugi::xml_node{}) {
        return false;
    }
    ensure_wordprocessingml_2012_namespace(root);
    ensure_markup_compatibility_ignorable(root, "w15");
    return true;
}

long next_numeric_id(pugi::xml_node root, const char *node_name, long minimum_id) {
    long next_id = minimum_id;
    for (auto node = root.child(node_name); node != pugi::xml_node{};
         node = node.next_sibling(node_name)) {
        const auto value_text = std::string_view{node.attribute("w:id").value()};
        if (value_text.empty()) {
            continue;
        }
        char *end = nullptr;
        const auto value_storage = std::string{value_text};
        const auto value = std::strtol(value_storage.c_str(), &end, 10);
        if (end != nullptr && *end == '\0' && value >= next_id) {
            next_id = value + 1;
        }
    }
    return next_id;
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

void remove_references_by_id(pugi::xml_node node, const char *reference_node_name,
                             std::string_view id) {
    for (auto child = node.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        const bool is_reference = std::string_view{child.name()} == reference_node_name &&
                                  std::string_view{child.attribute("w:id").value()} == id;
        if (is_reference) {
            auto parent = child.parent();
            if (std::string_view{parent.name()} == "w:r") {
                parent.parent().remove_child(parent);
            } else {
                parent.remove_child(child);
            }
        } else {
            remove_references_by_id(child, reference_node_name, id);
        }
        child = next;
    }
}
