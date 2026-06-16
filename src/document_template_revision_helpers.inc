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

void collect_revisions_from_node(
    pugi::xml_node node, std::string_view entry_name,
    std::vector<featherdoc::revision_summary> &summaries) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto kind = revision_kind_from_node_name(child.name());
        if (kind != featherdoc::revision_kind::unknown) {
            featherdoc::revision_summary summary;
            summary.index = summaries.size();
            summary.kind = kind;
            summary.id = child.attribute("w:id").value();
            summary.author = xml_child_attribute_string(child, "w:author");
            summary.date = xml_child_attribute_string(child, "w:date");
            summary.part_entry_name = std::string{entry_name};
            summary.text = featherdoc::detail::collect_plain_text_from_xml(child);
            summaries.push_back(std::move(summary));
        }
        collect_revisions_from_node(child, entry_name, summaries);
    }
}

void collect_revision_nodes_in_order(pugi::xml_node node,
                                     std::vector<pugi::xml_node> &nodes) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (revision_kind_from_node_name(child.name()) !=
            featherdoc::revision_kind::unknown) {
            nodes.push_back(child);
        }
        collect_revision_nodes_in_order(child, nodes);
    }
}

void collect_revision_ids(pugi::xml_node node, long &next_id) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (revision_kind_from_node_name(child.name()) !=
            featherdoc::revision_kind::unknown) {
            const auto id_text = std::string_view{child.attribute("w:id").value()};
            if (!id_text.empty()) {
                char *end = nullptr;
                const auto id_storage = std::string{id_text};
                const auto id = std::strtol(id_storage.c_str(), &end, 10);
                if (end != nullptr && *end == '\0' && id >= next_id) {
                    next_id = id + 1;
                }
            }
        }
        collect_revision_ids(child, next_id);
    }
}

bool revision_text_needs_space_preserve(std::string_view text) {
    return !text.empty() &&
           (std::isspace(static_cast<unsigned char>(text.front())) != 0 ||
            std::isspace(static_cast<unsigned char>(text.back())) != 0);
}

bool append_revision_text_node(pugi::xml_node run, const char *text_node_name,
                               std::string_view text) {
    auto text_node = run.append_child(text_node_name);
    if (text_node == pugi::xml_node{}) {
        return false;
    }
    if (revision_text_needs_space_preserve(text)) {
        auto xml_space = text_node.append_attribute("xml:space");
        if (xml_space == pugi::xml_attribute{}) {
            return false;
        }
        xml_space.set_value("preserve");
    }
    const auto text_storage = std::string{text};
    return text_node.text().set(text_storage.c_str());
}

bool set_revision_run_text_content(pugi::xml_node run, const char *text_node_name,
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
            !append_revision_text_node(run, text_node_name,
                                       text.substr(segment_start, offset - segment_start))) {
            return false;
        }
        if (run.append_child("w:br") == pugi::xml_node{}) {
            return false;
        }
        if (current == '\r' && offset + 1U < text.size() && text[offset + 1U] == '\n') {
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

bool set_revision_identity_metadata(pugi::xml_node revision, long revision_id,
                                    std::string_view author,
                                    std::string_view date) {
    if (revision == pugi::xml_node{}) {
        return false;
    }
    ensure_attribute_value(revision, "w:id", std::to_string(revision_id));
    if (!author.empty()) {
        ensure_attribute_value(revision, "w:author", author);
    }
    if (!date.empty()) {
        ensure_attribute_value(revision, "w:date", date);
    }
    return true;
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

bool append_copied_run_properties(pugi::xml_node target_run, pugi::xml_node source_run) {
    if (target_run == pugi::xml_node{}) {
        return false;
    }
    const auto source_properties = source_run.child("w:rPr");
    return source_properties == pugi::xml_node{} ||
           target_run.append_copy(source_properties) != pugi::xml_node{};
}

pugi::xml_node append_revision_run(pugi::xml_node revision, pugi::xml_node source_run,
                                   const char *text_node_name, std::string_view text) {
    auto run = revision.append_child("w:r");
    if (run == pugi::xml_node{} || !append_copied_run_properties(run, source_run) ||
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

pugi::xml_node split_revision_text_run(
    pugi::xml_node paragraph, const paragraph_revision_run_span &span,
    std::size_t text_offset) {
    if (text_offset <= span.start_offset ||
        text_offset >= span.start_offset + span.length) {
        return span.run;
    }
    if (!revision_range_run_content_supported(span.run)) {
        return {};
    }

    const auto local_offset = text_offset - span.start_offset;
    auto suffix_run = paragraph.insert_copy_after(span.run, span.run);
    if (suffix_run == pugi::xml_node{}) {
        return {};
    }

    if (!featherdoc::detail::set_plain_text_run_content(
            suffix_run, std::string_view{span.text}.substr(local_offset))) {
        paragraph.remove_child(suffix_run);
        return {};
    }
    if (!featherdoc::detail::set_plain_text_run_content(
            span.run, std::string_view{span.text}.substr(0U, local_offset))) {
        paragraph.remove_child(suffix_run);
        return {};
    }
    return suffix_run;
}

bool split_paragraph_revision_text_at(pugi::xml_node paragraph,
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
            return split_revision_text_run(paragraph, span, text_offset) !=
                   pugi::xml_node{};
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
            append_revision_run(revision, span.run, text_node_name, span.text) ==
                pugi::xml_node{}) {
            return false;
        }
    }
    return true;
}

bool selected_paragraph_revision_spans_supported(
    const std::vector<std::vector<paragraph_revision_run_span>> &selected_segments) {
    for (const auto &selected : selected_segments) {
        for (const auto &span : selected) {
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
        const auto paragraph_text_length = paragraph_revision_text_length(spans);
        const auto segment_start =
            paragraph_index == start_paragraph_index ? start_text_offset : 0U;
        const auto segment_end =
            paragraph_index == end_paragraph_index ? end_text_offset
                                                   : paragraph_text_length;
        if (segment_start > paragraph_text_length ||
            segment_end > paragraph_text_length || segment_end < segment_start) {
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

bool split_paragraph_revision_range_segments(
    const std::vector<paragraph_revision_range_segment> &segments) {
    for (const auto &segment : segments) {
        if (!split_paragraph_revision_text_at(
                segment.paragraph, segment.text_offset + segment.text_length) ||
            !split_paragraph_revision_text_at(segment.paragraph,
                                              segment.text_offset)) {
            return false;
        }
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
    const paragraph_revision_range_plan &plan) -> featherdoc::text_range_preview {
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

            const auto selected_start = std::max(span_start, segment.text_offset);
            const auto selected_end = std::min(span_end, segment_end);
            const auto local_start = selected_start - span_start;
            preview_segment.text.append(
                std::string_view{span.text}.substr(local_start,
                                                   selected_end - selected_start));
        }

        preview.text += preview_segment.text;
        preview.segments.push_back(std::move(preview_segment));
    }

    return preview;
}

auto revision_expected_text_mismatch_detail(
    std::string_view expected_text,
    const featherdoc::text_range_preview &preview)
    -> std::string {
    std::ostringstream detail;
    detail << "expected text did not match selected text"
           << " (expected: " << expected_text
           << ", actual: " << preview.text
           << ", start_paragraph_index: " << preview.start_paragraph_index
           << ", start_text_offset: " << preview.start_text_offset
           << ", end_paragraph_index: " << preview.end_paragraph_index
           << ", end_text_offset: " << preview.end_text_offset
           << ", text_length: " << preview.text_length
           << ", segments: [";
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

    set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                   revision_expected_text_mismatch_detail(*options.expected_text,
                                                          preview),
                   std::string{document_xml_entry});
    return false;
}

bool insert_comment_markers_for_selected_spans(
    const std::vector<paragraph_revision_range_segment> &segments,
    const std::vector<std::vector<paragraph_revision_run_span>> &selected_segments,
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
    auto reference_run = range_end == pugi::xml_node{}
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

void convert_deleted_text_to_text(pugi::xml_node node) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:delText") {
            child.set_name("w:t");
        }
        convert_deleted_text_to_text(child);
    }
}

bool unwrap_revision_node(pugi::xml_node revision, bool convert_deleted_text) {
    auto parent = revision.parent();
    if (parent == pugi::xml_node{}) {
        return false;
    }
    for (auto child = revision.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        auto copy = parent.insert_copy_before(child, revision);
        if (copy == pugi::xml_node{}) {
            return false;
        }
        if (convert_deleted_text) {
            convert_deleted_text_to_text(copy);
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
