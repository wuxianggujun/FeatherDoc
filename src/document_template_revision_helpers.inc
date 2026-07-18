auto revision_kind_from_node_name(std::string_view node_name)
    -> featherdoc::revision_kind {
    if (node_name == "w:ins") {
        return featherdoc::revision_kind::insertion;
    }
    if (node_name == "w:del") {
        return featherdoc::revision_kind::deletion;
    }
    if (node_name == "w:moveFrom") {
        return featherdoc::revision_kind::move_from;
    }
    if (node_name == "w:moveTo") {
        return featherdoc::revision_kind::move_to;
    }
    if (node_name == "w:pPrChange") {
        return featherdoc::revision_kind::paragraph_property_change;
    }
    if (node_name == "w:rPrChange") {
        return featherdoc::revision_kind::run_property_change;
    }
    return featherdoc::revision_kind::unknown;
}

bool node_uses_review_revision_identifier(std::string_view node_name) {
    // The public revision_kind model intentionally exposes only revisions that
    // FeatherDoc can currently inspect and accept/reject. ID allocation has a
    // wider responsibility: every tracked-change annotation in the package
    // shares the WordprocessingML revision-ID space, including revisions that
    // are preserved but not yet surfaced by that model.
    constexpr auto tracked_change_node_names = std::array{
        std::string_view{"w:ins"},
        std::string_view{"w:del"},
        std::string_view{"w:moveFrom"},
        std::string_view{"w:moveTo"},
        std::string_view{"w:moveFromRangeStart"},
        std::string_view{"w:moveFromRangeEnd"},
        std::string_view{"w:moveToRangeStart"},
        std::string_view{"w:moveToRangeEnd"},
        std::string_view{"w:customXmlInsRangeStart"},
        std::string_view{"w:customXmlInsRangeEnd"},
        std::string_view{"w:customXmlDelRangeStart"},
        std::string_view{"w:customXmlDelRangeEnd"},
        std::string_view{"w:customXmlMoveFromRangeStart"},
        std::string_view{"w:customXmlMoveFromRangeEnd"},
        std::string_view{"w:customXmlMoveToRangeStart"},
        std::string_view{"w:customXmlMoveToRangeEnd"},
        std::string_view{"w:pPrChange"},
        std::string_view{"w:rPrChange"},
        std::string_view{"w:sectPrChange"},
        std::string_view{"w:tblPrChange"},
        std::string_view{"w:tblGridChange"},
        std::string_view{"w:trPrChange"},
        std::string_view{"w:tcPrChange"},
        std::string_view{"w:numberingChange"},
        std::string_view{"w:cellIns"},
        std::string_view{"w:cellDel"},
        std::string_view{"w:cellMerge"},
        std::string_view{"w14:conflictIns"},
        std::string_view{"w14:conflictDel"},
        std::string_view{"w14:customXmlConflictInsRangeStart"},
        std::string_view{"w14:customXmlConflictInsRangeEnd"},
        std::string_view{"w14:customXmlConflictDelRangeStart"},
        std::string_view{"w14:customXmlConflictDelRangeEnd"},
    };
    return std::find(tracked_change_node_names.begin(),
                     tracked_change_node_names.end(),
                     node_name) != tracked_change_node_names.end();
}

void collect_revisions_from_node(
    pugi::xml_node node, std::string_view entry_name,
    std::vector<featherdoc::revision_summary> &summaries) {
    for (auto current = node.first_child(); current != pugi::xml_node{};
         current = featherdoc::detail::next_xml_node_preorder(node, current)) {
        const auto kind = revision_kind_from_node_name(current.name());
        if (kind != featherdoc::revision_kind::unknown) {
            featherdoc::revision_summary summary;
            summary.index = summaries.size();
            summary.kind = kind;
            summary.id = current.attribute("w:id").value();
            summary.author = xml_child_attribute_string(current, "w:author");
            summary.date = xml_child_attribute_string(current, "w:date");
            summary.part_entry_name = std::string{entry_name};
            summary.text =
                featherdoc::detail::collect_plain_text_from_xml(current);
            summaries.push_back(std::move(summary));
        }
    }
}

void collect_revision_nodes_in_order(pugi::xml_node node,
                                     std::vector<pugi::xml_node> &nodes) {
    for (auto current = node.first_child(); current != pugi::xml_node{};
         current = featherdoc::detail::next_xml_node_preorder(node, current)) {
        if (revision_kind_from_node_name(current.name()) !=
            featherdoc::revision_kind::unknown) {
            nodes.push_back(current);
        }
    }
}

using review_identifier = std::int64_t;

class review_identifier_allocator {
  public:
    explicit review_identifier_allocator(review_identifier minimum_id)
        : next_id_(minimum_id) {}

    void observe(std::string_view value_text) {
        const auto value =
            featherdoc::detail::parse_integer_strict<review_identifier>(
                value_text);
        if (!value.has_value() || *value < next_id_) {
            return;
        }
        if (*value == std::numeric_limits<review_identifier>::max()) {
            exhausted_ = true;
            return;
        }
        next_id_ = *value + 1;
    }

    [[nodiscard]] auto reserve(std::size_t count) const
        -> std::optional<review_identifier> {
        if (count == 0U || exhausted_) {
            return std::nullopt;
        }

        const auto required_increment = count - 1U;
        const auto available_increment = static_cast<std::uint64_t>(
            std::numeric_limits<review_identifier>::max() - next_id_);
        if (required_increment > available_increment) {
            return std::nullopt;
        }
        return next_id_;
    }

  private:
    review_identifier next_id_;
    bool exhausted_{false};
};

void collect_revision_ids(pugi::xml_node node,
                          review_identifier_allocator &allocator) {
    for (auto current = node.first_child(); current != pugi::xml_node{};
         current = featherdoc::detail::next_xml_node_preorder(node, current)) {
        if (node_uses_review_revision_identifier(current.name())) {
            const auto id_text =
                std::string_view{current.attribute("w:id").value()};
            if (!id_text.empty()) {
                allocator.observe(id_text);
            }
        }
    }
}

bool revision_text_needs_space_preserve(std::string_view text) {
    return !text.empty() &&
           (std::isspace(static_cast<unsigned char>(text.front())) != 0 ||
            std::isspace(static_cast<unsigned char>(text.back())) != 0);
}

bool append_revision_text_node(pugi::xml_node run, const char *text_node_name,
                               std::string_view text) {
    auto text_node =
        featherdoc::detail::checked_append_xml_element(run, text_node_name);
    if (text_node == pugi::xml_node{}) {
        return false;
    }
    if (revision_text_needs_space_preserve(text)) {
        if (!featherdoc::detail::checked_append_xml_attribute(
                text_node, "xml:space", "preserve")) {
            return false;
        }
    }
    return text_node.text().set(text.data(), text.size());
}

bool set_revision_run_text_content(pugi::xml_node run,
                                   const char *text_node_name,
                                   std::string_view text) {
    if (run == pugi::xml_node{}) {
        return false;
    }

    if (text.empty()) {
        return append_revision_text_node(run, text_node_name, {});
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
            !append_revision_text_node(
                run, text_node_name,
                text.substr(segment_start, offset - segment_start))) {
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

    if (segment_start < text.size()) {
        return append_revision_text_node(run, text_node_name,
                                         text.substr(segment_start));
    }
    return true;
}

bool set_revision_identity_metadata_text(pugi::xml_node revision,
                                         std::string_view revision_id,
                                         std::string_view author,
                                         std::string_view date) {
    if (revision == pugi::xml_node{}) {
        return false;
    }
    if (!ensure_attribute_value(revision, "w:id", revision_id)) {
        return false;
    }
    if (!author.empty()) {
        if (!ensure_attribute_value(revision, "w:author", author)) {
            return false;
        }
    }
    if (!date.empty()) {
        if (!ensure_attribute_value(revision, "w:date", date)) {
            return false;
        }
    }
    return true;
}

bool set_revision_identity_metadata(pugi::xml_node revision,
                                    review_identifier revision_id,
                                    std::string_view author,
                                    std::string_view date) {
    return set_revision_identity_metadata_text(
        revision, std::to_string(revision_id), author, date);
}

pugi::xml_node paragraph_run_by_index(pugi::xml_node paragraph,
                                      std::size_t run_index) {
    if (paragraph == pugi::xml_node{}) {
        return {};
    }
    auto run = paragraph.child("w:r");
    for (std::size_t current_index = 0U;
         current_index < run_index && run != pugi::xml_node{};
         ++current_index) {
        run = featherdoc::detail::next_named_sibling(run, "w:r");
    }
    return run;
}

pugi::xml_node
revision_document_paragraph_by_index(pugi::xml_document &document,
                                     std::size_t paragraph_index) noexcept {
    auto paragraph = document.child("w:document").child("w:body").child("w:p");
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph != pugi::xml_node{};
         ++current_index) {
        paragraph = featherdoc::detail::next_named_sibling(paragraph, "w:p");
    }
    return paragraph;
}

template <typename Mutation>
bool mutate_revision_body_document_transaction(
    pugi::xml_document &document,
    featherdoc::detail::xml_handle_lifetime &lifetime,
    featherdoc::document_error_info &error_info,
    std::string_view allocation_message, Mutation &&mutation) {
    try {
        pugi::xml_document work_document;
        if (featherdoc::detail::checked_clone_xml_document(document,
                                                           work_document) !=
            featherdoc::detail::xml_document_clone_status::success) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                error_info, document_xml_entry);
            return false;
        }

        const auto work_body =
            work_document.child("w:document").child("w:body");
        if (work_body == pugi::xml_node{} ||
            !std::forward<Mutation>(mutation)(work_document)) {
            (void)featherdoc::detail::
                set_xml_document_mutation_allocation_failure(
                    error_info, document_xml_entry);
            return false;
        }

        const auto live_body = document.child("w:document").child("w:body");
        if (live_body == pugi::xml_node{}) {
            set_last_error(
                error_info,
                featherdoc::document_errc::invalid_package_structure,
                "document body disappeared before revision publication",
                std::string{document_xml_entry});
            return false;
        }

        static_assert(noexcept(std::declval<pugi::xml_document &>() =
                                   std::declval<pugi::xml_document &&>()));
        lifetime.retire_subtree(live_body);
        document = std::move(work_document);
        return true;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            error_info, allocation_message, document_xml_entry);
        return false;
    }
}

bool append_copied_run_properties(pugi::xml_node target_run,
                                  pugi::xml_node source_run) {
    if (target_run == pugi::xml_node{}) {
        return false;
    }
    const auto source_properties = source_run.child("w:rPr");
    return source_properties == pugi::xml_node{} ||
           featherdoc::detail::checked_append_copy_xml_node(source_properties,
                                                            target_run) ==
               featherdoc::detail::xml_document_clone_status::success;
}

pugi::xml_node append_revision_run(pugi::xml_node revision,
                                   pugi::xml_node source_run,
                                   const char *text_node_name,
                                   std::string_view text) {
    auto run = featherdoc::detail::checked_append_xml_element(revision, "w:r");
    if (run == pugi::xml_node{} ||
        !append_copied_run_properties(run, source_run) ||
        !set_revision_run_text_content(run, text_node_name, text)) {
        return {};
    }
    return run;
}

struct paragraph_revision_run_span {
    pugi::xml_node run;
    std::size_t start_offset = 0U;
    std::size_t length = 0U;
    std::string text;
};

struct paragraph_revision_range_segment {
    pugi::xml_node paragraph;
    std::size_t paragraph_index = 0U;
    std::size_t text_offset = 0U;
    std::size_t text_length = 0U;
};

struct paragraph_revision_range_plan {
    std::vector<paragraph_revision_range_segment> segments;
    std::size_t selected_text_length = 0U;
};

bool revision_range_run_content_supported(pugi::xml_node run) {
    for (auto child = run.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto name = std::string_view{child.name()};
        if (name == "w:rPr" || name == "w:t" || name == "w:tab" ||
            name == "w:br" || name == "w:cr") {
            continue;
        }
        return false;
    }
    return true;
}

auto collect_paragraph_revision_run_spans(pugi::xml_node paragraph)
    -> std::vector<paragraph_revision_run_span> {
    std::vector<paragraph_revision_run_span> spans;
    std::size_t text_offset = 0U;
    for (auto child = paragraph.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:r") {
            continue;
        }

        auto text = featherdoc::detail::collect_plain_text_from_xml(child);
        if (text.empty()) {
            continue;
        }

        paragraph_revision_run_span span;
        span.run = child;
        span.start_offset = text_offset;
        span.length = text.size();
        span.text = std::move(text);
        text_offset += span.length;
        spans.push_back(std::move(span));
    }
    return spans;
}

std::size_t paragraph_revision_text_length(
    const std::vector<paragraph_revision_run_span> &spans) {
    if (spans.empty()) {
        return 0U;
    }
    const auto &last = spans.back();
    return last.start_offset + last.length;
}

auto collect_revision_document_paragraphs(pugi::xml_document &document)
    -> std::vector<pugi::xml_node> {
    std::vector<pugi::xml_node> paragraphs;
    const auto body = document.child("w:document").child("w:body");
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:p") {
            paragraphs.push_back(child);
        }
    }
    return paragraphs;
}

auto checked_insert_revision_element(pugi::xml_node paragraph, const char *name,
                                     pugi::xml_node insert_before)
    -> pugi::xml_node {
    return insert_before == pugi::xml_node{}
               ? featherdoc::detail::checked_append_xml_element(paragraph, name)
               : featherdoc::detail::checked_insert_xml_element_before(
                     paragraph, name, insert_before);
}

auto checked_insert_split_revision_run(pugi::xml_node paragraph,
                                       pugi::xml_node insert_before,
                                       pugi::xml_node source_run,
                                       std::string_view text)
    -> pugi::xml_node {
    auto run = featherdoc::detail::checked_insert_xml_element_before(
        paragraph, "w:r", insert_before);
    if (run == pugi::xml_node{} ||
        !append_copied_run_properties(run, source_run) ||
        !set_revision_run_text_content(run, "w:t", text)) {
        if (run != pugi::xml_node{}) {
            paragraph.remove_child(run);
        }
        return {};
    }
    return run;
}

bool checked_split_revision_text_run(pugi::xml_node paragraph,
                                     const paragraph_revision_run_span &span,
                                     std::size_t text_offset) {
    if (text_offset <= span.start_offset ||
        text_offset >= span.start_offset + span.length) {
        return true;
    }
    if (!revision_range_run_content_supported(span.run)) {
        return false;
    }

    const auto local_offset = text_offset - span.start_offset;
    const auto span_text = std::string_view{span.text};
    const auto prefix = checked_insert_split_revision_run(
        paragraph, span.run, span.run, span_text.substr(0U, local_offset));
    if (prefix == pugi::xml_node{}) {
        return false;
    }
    const auto suffix = checked_insert_split_revision_run(
        paragraph, span.run, span.run, span_text.substr(local_offset));
    if (suffix == pugi::xml_node{}) {
        paragraph.remove_child(prefix);
        return false;
    }
    if (!paragraph.remove_child(span.run)) {
        paragraph.remove_child(prefix);
        paragraph.remove_child(suffix);
        return false;
    }
    return true;
}

bool checked_split_paragraph_revision_text_at(pugi::xml_node paragraph,
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
            return checked_split_revision_text_run(paragraph, span,
                                                   text_offset);
        }
    }
    return true;
}

bool checked_split_paragraph_revision_range_segments(
    const std::vector<paragraph_revision_range_segment> &segments) {
    for (const auto &segment : segments) {
        if (!checked_split_paragraph_revision_text_at(
                segment.paragraph, segment.text_offset + segment.text_length) ||
            !checked_split_paragraph_revision_text_at(segment.paragraph,
                                                      segment.text_offset)) {
            return false;
        }
    }
    return true;
}

auto paragraph_revision_spans_in_range(pugi::xml_node paragraph,
                                       std::size_t text_offset,
                                       std::size_t text_length)
    -> std::vector<paragraph_revision_run_span> {
    std::vector<paragraph_revision_run_span> selected;
    const auto end_offset = text_offset + text_length;
    for (auto span : collect_paragraph_revision_run_spans(paragraph)) {
        if (span.start_offset >= text_offset &&
            span.start_offset + span.length <= end_offset) {
            selected.push_back(std::move(span));
        }
    }
    return selected;
}

pugi::xml_node paragraph_revision_insert_before(pugi::xml_node paragraph,
                                                std::size_t text_offset) {
    for (const auto &span : collect_paragraph_revision_run_spans(paragraph)) {
        if (span.start_offset >= text_offset) {
            return span.run;
        }
    }
    return {};
}

pugi::xml_node paragraph_revision_style_run(pugi::xml_node paragraph,
                                            std::size_t text_offset) {
    pugi::xml_node previous_text_run;
    for (const auto &span : collect_paragraph_revision_run_spans(paragraph)) {
        if (span.start_offset >= text_offset) {
            return previous_text_run != pugi::xml_node{} ? previous_text_run
                                                         : span.run;
        }
        previous_text_run = span.run;
    }
    return previous_text_run;
}

pugi::xml_node insert_revision_node(pugi::xml_node paragraph, const char *name,
                                    pugi::xml_node insert_before) {
    return insert_before == pugi::xml_node{}
               ? paragraph.append_child(name)
               : paragraph.insert_child_before(name, insert_before);
}

bool append_revision_runs_from_spans(
    pugi::xml_node revision,
    const std::vector<paragraph_revision_run_span> &spans,
    const char *text_node_name) {
    for (const auto &span : spans) {
        if (!revision_range_run_content_supported(span.run) ||
            append_revision_run(revision, span.run, text_node_name,
                                span.text) == pugi::xml_node{}) {
            return false;
        }
    }
    return true;
}

bool selected_paragraph_revision_spans_supported(
    const std::vector<std::vector<paragraph_revision_run_span>>
        &selected_segments) {
    for (const auto &selected : selected_segments) {
        for (const auto &span : selected) {
            if (!revision_range_run_content_supported(span.run)) {
                return false;
            }
        }
    }
    return true;
}

bool paragraph_revision_range_segments_supported(
    const std::vector<paragraph_revision_range_segment> &segments) {
    for (const auto &segment : segments) {
        if (segment.text_length >
            std::numeric_limits<std::size_t>::max() - segment.text_offset) {
            return false;
        }
        const auto segment_end = segment.text_offset + segment.text_length;
        for (const auto &span :
             collect_paragraph_revision_run_spans(segment.paragraph)) {
            if (span.length >
                std::numeric_limits<std::size_t>::max() - span.start_offset) {
                return false;
            }
            const auto span_end = span.start_offset + span.length;
            if (span_end <= segment.text_offset ||
                span.start_offset >= segment_end) {
                continue;
            }
            if (!revision_range_run_content_supported(span.run)) {
                return false;
            }
        }
    }
    return true;
}

bool build_paragraph_revision_range_plan(
    const std::vector<pugi::xml_node> &paragraphs,
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset,
    paragraph_revision_range_plan &plan, std::string &error_message) {
    plan = {};
    error_message.clear();
    if (start_paragraph_index > end_paragraph_index) {
        error_message = "text range start must not be after end";
        return false;
    }
    if (start_paragraph_index >= paragraphs.size() ||
        end_paragraph_index >= paragraphs.size()) {
        error_message = "paragraph range is out of range";
        return false;
    }

    const auto start_spans =
        collect_paragraph_revision_run_spans(paragraphs[start_paragraph_index]);
    const auto start_text_length = paragraph_revision_text_length(start_spans);
    if (start_text_offset > start_text_length) {
        error_message = "start text offset is out of range";
        return false;
    }

    const auto end_spans =
        collect_paragraph_revision_run_spans(paragraphs[end_paragraph_index]);
    const auto end_text_length = paragraph_revision_text_length(end_spans);
    if (end_text_offset > end_text_length) {
        error_message = "end text offset is out of range";
        return false;
    }
    if (start_paragraph_index == end_paragraph_index &&
        end_text_offset <= start_text_offset) {
        error_message = "text range must not be empty";
        return false;
    }

    for (std::size_t paragraph_index = start_paragraph_index;
         paragraph_index <= end_paragraph_index; ++paragraph_index) {
        const auto spans =
            collect_paragraph_revision_run_spans(paragraphs[paragraph_index]);
        const auto paragraph_text_length =
            paragraph_revision_text_length(spans);
        const auto segment_start =
            paragraph_index == start_paragraph_index ? start_text_offset : 0U;
        const auto segment_end = paragraph_index == end_paragraph_index
                                     ? end_text_offset
                                     : paragraph_text_length;
        if (segment_start > paragraph_text_length ||
            segment_end > paragraph_text_length ||
            segment_end < segment_start) {
            error_message = "text range is out of range";
            return false;
        }
        if (segment_end == segment_start) {
            continue;
        }

        paragraph_revision_range_segment segment;
        segment.paragraph = paragraphs[paragraph_index];
        segment.paragraph_index = paragraph_index;
        segment.text_offset = segment_start;
        segment.text_length = segment_end - segment_start;
        plan.selected_text_length += segment.text_length;
        plan.segments.push_back(segment);
    }

    if (plan.selected_text_length == 0U) {
        error_message = "text range must not be empty";
        return false;
    }
    return true;
}

auto collect_selected_paragraph_revision_spans(
    const std::vector<paragraph_revision_range_segment> &segments)
    -> std::vector<std::vector<paragraph_revision_run_span>> {
    std::vector<std::vector<paragraph_revision_run_span>> selected_segments;
    selected_segments.reserve(segments.size());
    for (const auto &segment : segments) {
        auto selected = paragraph_revision_spans_in_range(
            segment.paragraph, segment.text_offset, segment.text_length);
        if (selected.empty()) {
            return {};
        }
        selected_segments.push_back(std::move(selected));
    }
    return selected_segments;
}

auto preview_paragraph_revision_range_segments(
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset,
    const paragraph_revision_range_plan &plan)
    -> featherdoc::text_range_preview {
    featherdoc::text_range_preview preview;
    preview.start_paragraph_index = start_paragraph_index;
    preview.start_text_offset = start_text_offset;
    preview.end_paragraph_index = end_paragraph_index;
    preview.end_text_offset = end_text_offset;
    preview.text_length = plan.selected_text_length;

    for (const auto &segment : plan.segments) {
        featherdoc::text_range_preview_segment preview_segment;
        preview_segment.paragraph_index = segment.paragraph_index;
        preview_segment.text_offset = segment.text_offset;
        preview_segment.text_length = segment.text_length;

        const auto segment_end = segment.text_offset + segment.text_length;
        for (const auto &span :
             collect_paragraph_revision_run_spans(segment.paragraph)) {
            const auto span_start = span.start_offset;
            const auto span_end = span.start_offset + span.length;
            if (span_end <= segment.text_offset || span_start >= segment_end) {
                continue;
            }
            if (!revision_range_run_content_supported(span.run)) {
                preview.plain_text_runs_supported = false;
            }

            const auto selected_start =
                std::max(span_start, segment.text_offset);
            const auto selected_end = std::min(span_end, segment_end);
            const auto local_start = selected_start - span_start;
            preview_segment.text.append(std::string_view{span.text}.substr(
                local_start, selected_end - selected_start));
        }

        preview.text += preview_segment.text;
        preview.segments.push_back(std::move(preview_segment));
    }

    return preview;
}

auto revision_expected_text_mismatch_detail(
    std::string_view expected_text,
    const featherdoc::text_range_preview &preview) -> std::string {
    std::ostringstream detail;
    detail << "expected text did not match selected text"
           << " (expected: " << expected_text << ", actual: " << preview.text
           << ", start_paragraph_index: " << preview.start_paragraph_index
           << ", start_text_offset: " << preview.start_text_offset
           << ", end_paragraph_index: " << preview.end_paragraph_index
           << ", end_text_offset: " << preview.end_text_offset
           << ", text_length: " << preview.text_length << ", segments: [";
    for (std::size_t index = 0U; index < preview.segments.size(); ++index) {
        if (index != 0U) {
            detail << "; ";
        }
        const auto &segment = preview.segments[index];
        detail << "paragraph_index=" << segment.paragraph_index
               << " text_offset=" << segment.text_offset
               << " text_length=" << segment.text_length
               << " text=" << segment.text;
    }
    detail << "])";
    return detail.str();
}

auto validate_revision_expected_text(
    featherdoc::document_error_info &last_error_info,
    const featherdoc::text_range_preview &preview,
    const featherdoc::revision_text_range_options &options) -> bool {
    if (!options.expected_text.has_value()) {
        return true;
    }

    if (preview.text == *options.expected_text) {
        return true;
    }

    set_last_error(
        last_error_info, std::make_error_code(std::errc::invalid_argument),
        revision_expected_text_mismatch_detail(*options.expected_text, preview),
        std::string{document_xml_entry});
    return false;
}

bool insert_comment_markers_for_selected_spans(
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
    auto range_start =
        first_paragraph.insert_child_before("w:commentRangeStart", first_run);
    auto range_end =
        last_paragraph.insert_child_after("w:commentRangeEnd", last_run);
    auto reference_run =
        range_end == pugi::xml_node{}
            ? pugi::xml_node{}
            : last_paragraph.insert_child_after("w:r", range_end);
    auto reference = reference_run == pugi::xml_node{}
                         ? pugi::xml_node{}
                         : reference_run.append_child("w:commentReference");
    if (range_start == pugi::xml_node{} || range_end == pugi::xml_node{} ||
        reference_run == pugi::xml_node{} || reference == pugi::xml_node{}) {
        return false;
    }
    ensure_attribute_value(range_start, "w:id", comment_id);
    ensure_attribute_value(range_end, "w:id", comment_id);
    ensure_attribute_value(reference, "w:id", comment_id);
    return true;
}

bool convert_deleted_text_to_text(pugi::xml_node node) {
    for (auto current = node.first_child(); current != pugi::xml_node{};
         current = featherdoc::detail::next_xml_node_preorder(node, current)) {
        if (std::string_view{current.name()} == "w:delText") {
            if (!current.set_name("w:t")) {
                return false;
            }
        }
    }
    return true;
}

bool unwrap_revision_node(pugi::xml_node revision, bool convert_deleted_text) {
    auto parent = revision.parent();
    if (parent == pugi::xml_node{}) {
        return false;
    }
    while (const auto child = revision.first_child()) {
        if (convert_deleted_text && !convert_deleted_text_to_text(child)) {
            return false;
        }
        if (parent.insert_move_before(child, revision) == pugi::xml_node{}) {
            return false;
        }
    }
    return parent.remove_child(revision);
}

bool apply_revision_decision(pugi::xml_node revision, bool accept) {
    const auto kind = revision_kind_from_node_name(revision.name());
    switch (kind) {
    case featherdoc::revision_kind::insertion:
    case featherdoc::revision_kind::move_to:
        return accept ? unwrap_revision_node(revision, false)
                      : revision.parent().remove_child(revision);
    case featherdoc::revision_kind::deletion:
    case featherdoc::revision_kind::move_from:
        return accept ? revision.parent().remove_child(revision)
                      : unwrap_revision_node(revision, true);
    case featherdoc::revision_kind::paragraph_property_change:
    case featherdoc::revision_kind::run_property_change:
        return revision.parent().remove_child(revision);
    case featherdoc::revision_kind::unknown:
        return false;
    }
    return false;
}
