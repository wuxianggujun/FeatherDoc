[[nodiscard]] double rect_bottom(const PdfRect &rect) noexcept {
    return rect.y_points;
}

[[nodiscard]] double rect_top(const PdfRect &rect) noexcept {
    return rect.y_points + rect.height_points;
}

[[nodiscard]] double rect_right(const PdfRect &rect) noexcept {
    return rect.x_points + rect.width_points;
}

[[nodiscard]] double span_center_y(const PdfParsedTextSpan &span) noexcept {
    return span.bounds.y_points + span.bounds.height_points / 2.0;
}

[[nodiscard]] double rect_area(const PdfRect &rect) noexcept {
    return (std::max)(0.0, rect.width_points) *
           (std::max)(0.0, rect.height_points);
}

[[nodiscard]] bool rect_contains_point(const PdfRect &rect, double x_points,
                                       double y_points) noexcept {
    return x_points >= rect.x_points && x_points <= rect_right(rect) &&
           y_points >= rect.y_points &&
           y_points <= rect.y_points + rect.height_points;
}

[[nodiscard]] double rect_intersection_area(const PdfRect &left,
                                            const PdfRect &right) noexcept {
    const double intersection_left =
        (std::max)(left.x_points, right.x_points);
    const double intersection_bottom =
        (std::max)(left.y_points, right.y_points);
    const double intersection_right =
        (std::min)(rect_right(left), rect_right(right));
    const double intersection_top = (std::min)(
        left.y_points + left.height_points, right.y_points + right.height_points);
    if (intersection_right <= intersection_left ||
        intersection_top <= intersection_bottom) {
        return 0.0;
    }

    return (intersection_right - intersection_left) *
           (intersection_top - intersection_bottom);
}

[[nodiscard]] bool rect_meaningfully_overlaps_rect(const PdfRect &subject,
                                                   const PdfRect &rect) noexcept {
    const double subject_area = rect_area(subject);
    if (subject_area <= 0.0) {
        return false;
    }

    const double center_x = subject.x_points + subject.width_points / 2.0;
    const double center_y = subject.y_points + subject.height_points / 2.0;
    const bool center_inside_rect =
        rect_contains_point(rect, center_x, center_y);
    const double overlap_ratio =
        rect_intersection_area(subject, rect) / subject_area;
    return center_inside_rect || overlap_ratio >= 0.5;
}

void expand_rect(PdfRect &target, const PdfRect &candidate) {
    if (target.width_points <= 0.0 && target.height_points <= 0.0) {
        target = candidate;
        return;
    }

    const double left = (std::min)(target.x_points, candidate.x_points);
    const double bottom = (std::min)(rect_bottom(target), rect_bottom(candidate));
    const double right = (std::max)(rect_right(target), rect_right(candidate));
    const double top = (std::max)(rect_top(target), rect_top(candidate));
    target = PdfRect{left, bottom, right - left, top - bottom};
}

void append_span_to_line(PdfParsedTextLine &line, const PdfParsedTextSpan &span) {
    if (!line.text.empty() && !span.text.empty() &&
        !std::isspace(static_cast<unsigned char>(line.text.back())) &&
        !std::isspace(static_cast<unsigned char>(span.text.front()))) {
        const double horizontal_gap = span.bounds.x_points - rect_right(line.bounds);
        const double line_height = (std::max)(line.bounds.height_points, 1.0);
        const double gap_threshold = (std::max)(3.0, line_height * 0.25);
        if (horizontal_gap > gap_threshold) {
            line.text.push_back(' ');
        }
    }
    line.text += span.text;
    expand_rect(line.bounds, span.bounds);
    line.text_spans.push_back(span);
}

void append_span_to_cluster(TextCluster &cluster,
                            const PdfParsedTextSpan &span) {
    cluster.text += span.text;
    expand_rect(cluster.bounds, span.bounds);
}

struct VisualTextRow {
    std::vector<PdfParsedTextSpan> spans;
    double center_y{0.0};
    double max_height{0.0};
};

[[nodiscard]] bool belongs_to_visual_text_row(const VisualTextRow &row,
                                              const PdfParsedTextSpan &span) {
    const double tolerance =
        (std::max)(4.0, (std::max)(row.max_height,
                                   span.bounds.height_points) *
                            0.65);
    return std::abs(span_center_y(span) - row.center_y) <= tolerance;
}

void append_span_to_visual_text_row(VisualTextRow &row,
                                    const PdfParsedTextSpan &span) {
    const auto previous_size = row.spans.size();
    row.spans.push_back(span);
    row.center_y =
        ((row.center_y * static_cast<double>(previous_size)) +
         span_center_y(span)) /
        static_cast<double>(previous_size + 1U);
    row.max_height = (std::max)(row.max_height, span.bounds.height_points);
}

[[nodiscard]] std::vector<PdfParsedTextSpan>
order_spans_by_visual_rows(std::vector<PdfParsedTextSpan> spans) {
    std::stable_sort(spans.begin(), spans.end(),
                     [](const PdfParsedTextSpan &left,
                        const PdfParsedTextSpan &right) {
                         const auto left_center =
                             std::llround(span_center_y(left) * 100.0);
                         const auto right_center =
                             std::llround(span_center_y(right) * 100.0);
                         if (left_center != right_center) {
                             return left_center > right_center;
                         }

                         const auto left_x =
                             std::llround(left.bounds.x_points * 100.0);
                         const auto right_x =
                             std::llround(right.bounds.x_points * 100.0);
                         if (left_x != right_x) {
                             return left_x < right_x;
                         }

                         return false;
                     });

    std::vector<VisualTextRow> rows;
    for (const auto &span : spans) {
        if (rows.empty() ||
            !belongs_to_visual_text_row(rows.back(), span)) {
            rows.push_back(VisualTextRow{});
        }
        append_span_to_visual_text_row(rows.back(), span);
    }

    std::vector<PdfParsedTextSpan> ordered;
    ordered.reserve(spans.size());
    for (auto &row : rows) {
        std::stable_sort(row.spans.begin(), row.spans.end(),
                         [](const PdfParsedTextSpan &left,
                            const PdfParsedTextSpan &right) {
                             const auto left_x =
                                 std::llround(left.bounds.x_points * 100.0);
                             const auto right_x =
                                 std::llround(right.bounds.x_points * 100.0);
                             if (left_x != right_x) {
                                 return left_x < right_x;
                             }

                             return span_center_y(left) > span_center_y(right);
                         });
        ordered.insert(ordered.end(), row.spans.begin(), row.spans.end());
    }

    return ordered;
}

[[nodiscard]] bool should_start_new_line(const PdfParsedTextLine &line,
                                         const PdfParsedTextSpan &span) {
    if (line.text_spans.empty()) {
        return false;
    }

    const double line_height = (std::max)(line.bounds.height_points, 1.0);
    const double span_center_y = span.bounds.y_points + span.bounds.height_points / 2.0;
    const double line_center_y = line.bounds.y_points + line.bounds.height_points / 2.0;
    if (std::abs(span_center_y - line_center_y) > line_height * 0.65) {
        return true;
    }

    const double x_backtrack = line.bounds.x_points - span.bounds.x_points;
    const double x_backtrack_threshold = (std::max)(12.0, line_height * 1.5);
    return x_backtrack > x_backtrack_threshold;
}

[[nodiscard]] std::vector<PdfParsedTextLine>
group_text_spans_into_lines(const std::vector<PdfParsedTextSpan> &spans) {
    std::vector<PdfParsedTextLine> lines;

    std::vector<PdfParsedTextSpan> ordered_spans = spans;
    const bool has_explicit_line_break = std::any_of(
        ordered_spans.begin(), ordered_spans.end(),
        [](const PdfParsedTextSpan &span) {
            return span.text == "\r" || span.text == "\n";
        });
    if (!has_explicit_line_break) {
        ordered_spans = order_spans_by_visual_rows(std::move(ordered_spans));
    }

    for (const auto &span : ordered_spans) {
        if (span.text == "\r" || span.text == "\n") {
            if (!lines.empty() && !lines.back().text.empty()) {
                lines.emplace_back();
            }
            continue;
        }
        if (span.text.empty()) {
            continue;
        }

        if (lines.empty() || should_start_new_line(lines.back(), span)) {
            lines.emplace_back();
        }
        append_span_to_line(lines.back(), span);
    }

    lines.erase(std::remove_if(lines.begin(), lines.end(),
                               [](const PdfParsedTextLine &line) {
                                   return line.text.empty();
                               }),
                lines.end());
    return lines;
}

[[nodiscard]] bool should_start_new_paragraph(
    const PdfParsedTextLine &previous, const PdfParsedTextLine &current,
    const std::optional<std::size_t> &previous_table_candidate_index,
    const std::optional<std::size_t> &current_table_candidate_index) {
    if (previous_table_candidate_index != current_table_candidate_index) {
        return true;
    }

    const double previous_height = (std::max)(previous.bounds.height_points, 1.0);
    const double vertical_gap = previous.bounds.y_points - rect_top(current.bounds);
    if (vertical_gap > previous_height * 0.85) {
        return true;
    }

    const double indent_delta = current.bounds.x_points - previous.bounds.x_points;
    return indent_delta > previous_height * 1.5;
}

void append_line_to_paragraph(PdfParsedParagraph &paragraph,
                              const PdfParsedTextLine &line) {
    if (!paragraph.text.empty()) {
        paragraph.text.push_back('\n');
    }
    paragraph.text += line.text;
    expand_rect(paragraph.bounds, line.bounds);
    paragraph.lines.push_back(line);
}

[[nodiscard]] bool should_start_new_cell_cluster(
    const TextCluster &cluster, const PdfParsedTextSpan &span,
    double line_height) {
    if (cluster.text.empty()) {
        return false;
    }

    const double horizontal_gap = span.bounds.x_points - rect_right(cluster.bounds);
    const double gap_threshold = (std::max)(18.0, line_height * 1.5);
    return horizontal_gap > gap_threshold;
}

[[nodiscard]] std::vector<TextCluster>
split_line_into_cell_clusters(const PdfParsedTextLine &line) {
    std::vector<TextCluster> clusters;
    const double line_height = (std::max)(line.bounds.height_points, 1.0);

    for (const auto &span : line.text_spans) {
        if (span.text == "\r" || span.text == "\n" || span.text.empty()) {
            continue;
        }

        if (clusters.empty() ||
            should_start_new_cell_cluster(clusters.back(), span, line_height)) {
            clusters.emplace_back();
        }
        append_span_to_cluster(clusters.back(), span);
    }

    clusters.erase(std::remove_if(clusters.begin(), clusters.end(),
                                  [](const TextCluster &cluster) {
                                      return cluster.text.empty();
                                  }),
                   clusters.end());
    return clusters;
}

[[nodiscard]] double line_center_y(const PdfParsedTextLine &line) noexcept {
    return line.bounds.y_points + line.bounds.height_points / 2.0;
}

[[nodiscard]] double line_top_y(const PdfParsedTextLine &line) noexcept {
    return rect_top(line.bounds);
}

[[nodiscard]] double line_left_x(const PdfParsedTextLine &line) noexcept {
    return line.bounds.x_points;
}

void sort_lines_by_reading_order(std::vector<PdfParsedTextLine> &lines) {
    std::stable_sort(lines.begin(), lines.end(),
                     [](const PdfParsedTextLine &left,
                        const PdfParsedTextLine &right) {
                         const auto left_top_key =
                             std::llround(line_top_y(left) * 100.0);
                         const auto right_top_key =
                             std::llround(line_top_y(right) * 100.0);
                         if (left_top_key != right_top_key) {
                             return left_top_key > right_top_key;
                         }

                         const auto left_x_key =
                             std::llround(line_left_x(left) * 100.0);
                         const auto right_x_key =
                             std::llround(line_left_x(right) * 100.0);
                         if (left_x_key != right_x_key) {
                             return left_x_key < right_x_key;
                         }

                         return false;
                     });
}
