[[nodiscard]] std::vector<CandidateLine>
collect_candidate_lines(const std::vector<PdfParsedTextLine> &lines) {
    std::vector<CandidateLine> candidate_lines;
    candidate_lines.reserve(lines.size());

    for (std::size_t line_index = 0U; line_index < lines.size(); ++line_index) {
        const auto &line = lines[line_index];
        auto clusters = split_line_into_cell_clusters(line);
        if (clusters.empty()) {
            continue;
        }

        candidate_lines.push_back(
            CandidateLine{line_index, std::move(clusters), line_center_y(line)});
    }

    return candidate_lines;
}

[[nodiscard]] std::vector<double> build_column_anchors(
    const std::vector<CandidateLine> &rows) {
    std::vector<double> anchors;
    std::vector<std::size_t> anchor_counts;
    std::vector<double> cluster_positions;

    std::size_t max_cluster_count = 0U;
    for (const auto &row : rows) {
        max_cluster_count = (std::max)(max_cluster_count, row.clusters.size());
    }
    const auto full_width_row_count = static_cast<std::size_t>(std::count_if(
        rows.begin(), rows.end(), [max_cluster_count](const CandidateLine &row) {
            return row.clusters.size() == max_cluster_count;
        }));
    const bool has_full_table_width_rows =
        max_cluster_count >= 3U && full_width_row_count >= 2U;

    for (const auto &row : rows) {
        if (has_full_table_width_rows &&
            row.clusters.size() < max_cluster_count) {
            continue;
        }
        for (const auto &cluster : row.clusters) {
            cluster_positions.push_back(cluster.bounds.x_points);
        }
    }

    std::sort(cluster_positions.begin(), cluster_positions.end());
    for (const double position : cluster_positions) {
        if (anchors.empty() || position - anchors.back() > 24.0) {
            anchors.push_back(position);
            anchor_counts.push_back(1U);
            continue;
        }

        const auto last_index = anchors.size() - 1U;
        const auto count = anchor_counts[last_index];
        anchors[last_index] =
            ((anchors[last_index] * static_cast<double>(count)) + position) /
            static_cast<double>(count + 1U);
        anchor_counts[last_index] = count + 1U;
    }

    return anchors;
}

[[nodiscard]] bool has_regular_column_spacing(
    const std::vector<double> &anchors) {
    if (anchors.size() < 3U) {
        return true;
    }

    double min_gap = 0.0;
    double max_gap = 0.0;
    double total_gap = 0.0;
    bool has_gap = false;

    for (std::size_t index = 1U; index < anchors.size(); ++index) {
        const double gap = anchors[index] - anchors[index - 1U];
        if (!has_gap) {
            min_gap = gap;
            max_gap = gap;
            has_gap = true;
        } else {
            min_gap = (std::min)(min_gap, gap);
            max_gap = (std::max)(max_gap, gap);
        }
        total_gap += gap;
    }

    if (!has_gap) {
        return false;
    }

    const double average_gap =
        total_gap / static_cast<double>(anchors.size() - 1U);
    return (max_gap - min_gap) <= (std::max)(24.0, average_gap * 0.25);
}

[[nodiscard]] bool is_short_header_label(const std::string &text) {
    if (text.empty() || text.size() > 24U) {
        return false;
    }

    bool has_alpha = false;
    for (unsigned char ch : text) {
        if (std::isdigit(ch)) {
            return false;
        }
        if (std::isalpha(ch)) {
            has_alpha = true;
        }
    }

    return has_alpha;
}

[[nodiscard]] bool is_data_like_cell_text(const std::string &text) {
    if (text.empty()) {
        return false;
    }

    for (unsigned char ch : text) {
        if (std::isdigit(ch)) {
            return true;
        }
    }

    return !is_short_header_label(text);
}

[[nodiscard]] bool is_conservative_two_row_table(
    const std::vector<CandidateLine> &rows) {
    if (rows.size() != 2U) {
        return false;
    }

    const auto &header_row = rows.front().clusters;
    const auto &data_row = rows.back().clusters;
    if (header_row.size() < 3U || data_row.size() < 3U) {
        return false;
    }

    if (!std::all_of(header_row.begin(), header_row.end(),
                     [](const TextCluster &cluster) {
                         return is_short_header_label(cluster.text);
                     })) {
        return false;
    }

    const auto data_like_count = static_cast<std::size_t>(std::count_if(
        data_row.begin(), data_row.end(), [](const TextCluster &cluster) {
            return is_data_like_cell_text(cluster.text);
        }));
    return data_like_count >= 1U;
}

[[nodiscard]] bool is_conservative_two_column_key_value_table(
    const std::vector<CandidateLine> &rows) {
    if (rows.size() < 3U) {
        return false;
    }

    std::size_t data_like_value_count = 0U;
    for (const auto &row : rows) {
        if (row.clusters.size() != 2U) {
            return false;
        }

        if (!is_short_header_label(row.clusters[0].text)) {
            return false;
        }

        if (is_data_like_cell_text(row.clusters[1].text)) {
            ++data_like_value_count;
        }
    }

    const auto minimum_data_values =
        (std::max)(std::size_t{2U}, (rows.size() + 1U) / 2U);
    return data_like_value_count >= minimum_data_values;
}

[[nodiscard]] bool is_conservative_irregular_header_table(
    const std::vector<CandidateLine> &rows,
    const std::vector<double> &anchors) {
    if (rows.size() < 3U || anchors.size() < 3U) {
        return false;
    }

    const auto &header_row = rows.front().clusters;
    if (header_row.size() != anchors.size()) {
        return false;
    }
    if (!std::all_of(header_row.begin(), header_row.end(),
                     [](const TextCluster &cluster) {
                         return is_short_header_label(cluster.text);
                     })) {
        return false;
    }

    const auto minimum_data_cells = (anchors.size() + 1U) / 2U;
    for (std::size_t row_index = 1U; row_index < rows.size(); ++row_index) {
        const auto &row = rows[row_index].clusters;
        if (row.size() != anchors.size()) {
            return false;
        }

        const auto data_like_count = static_cast<std::size_t>(std::count_if(
            row.begin(), row.end(), [](const TextCluster &cluster) {
                return is_data_like_cell_text(cluster.text);
            }));
        if (data_like_count < minimum_data_cells) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] std::size_t nearest_column_index(
    const std::vector<double> &anchors, double x_points) {
    std::size_t best_index = 0U;
    double best_distance = std::abs(x_points - anchors.front());

    for (std::size_t index = 1U; index < anchors.size(); ++index) {
        const double distance = std::abs(x_points - anchors[index]);
        if (distance < best_distance) {
            best_distance = distance;
            best_index = index;
        }
    }

    return best_index;
}

[[nodiscard]] double average_column_gap(
    const std::vector<double> &anchors) noexcept {
    if (anchors.size() < 2U) {
        return 0.0;
    }

    return (anchors.back() - anchors.front()) /
           static_cast<double>(anchors.size() - 1U);
}

[[nodiscard]] std::size_t detect_column_span(
    const std::vector<double> &anchors, const TextCluster &cluster,
    std::size_t column_index) {
    if (anchors.size() < 2U || column_index >= anchors.size()) {
        return 1U;
    }

    const double average_gap = average_column_gap(anchors);
    if (average_gap <= 0.0 ||
        cluster.bounds.width_points < average_gap * 1.35) {
        return 1U;
    }

    // 仅对明显宽于单列的文本簇做最小跨列推断，避免把普通长单元格过度提升成合并单元格。
    const auto estimated_span = static_cast<std::size_t>(
        std::ceil(cluster.bounds.width_points / average_gap));
    if (estimated_span <= 1U) {
        return 1U;
    }

    return (std::min)(anchors.size() - column_index, estimated_span);
}

[[nodiscard]] double cluster_center_x(const TextCluster &cluster) noexcept {
    return cluster.bounds.x_points + cluster.bounds.width_points / 2.0;
}

struct GroupHeaderSpan final {
    std::size_t column_index{0U};
    std::size_t column_span{1U};
};

[[nodiscard]] std::optional<GroupHeaderSpan>
infer_group_header_span(const std::vector<double> &anchors,
                        const CandidateLine &row,
                        std::size_t cluster_index) {
    if (anchors.size() < 3U || row.clusters.empty() ||
        row.clusters.size() >= anchors.size() ||
        cluster_index >= row.clusters.size()) {
        return std::nullopt;
    }

    const double average_gap = average_column_gap(anchors);
    if (average_gap <= 0.0) {
        return std::nullopt;
    }

    const auto &cluster = row.clusters[cluster_index];
    const double cluster_center = cluster_center_x(cluster);
    const double table_center = (anchors.front() + anchors.back()) / 2.0;
    if (row.clusters.size() == 1U) {
        const double center_tolerance = (std::max)(36.0, average_gap * 0.4);
        if (std::abs(cluster_center - table_center) > center_tolerance) {
            return std::nullopt;
        }
    }

    double left_boundary = anchors.front() - average_gap;
    if (cluster_index > 0U) {
        left_boundary =
            (cluster_center_x(row.clusters[cluster_index - 1U]) +
             cluster_center) /
            2.0;
    }

    double right_boundary = anchors.back() + average_gap;
    if (cluster_index + 1U < row.clusters.size()) {
        right_boundary =
            (cluster_center +
             cluster_center_x(row.clusters[cluster_index + 1U])) /
            2.0;
    }

    std::optional<std::size_t> first_column;
    std::size_t covered_columns = 0U;
    const double boundary_tolerance = (std::max)(4.0, average_gap * 0.05);
    for (std::size_t column_index = 0U; column_index < anchors.size();
         ++column_index) {
        const double anchor = anchors[column_index];
        if (anchor + boundary_tolerance < left_boundary ||
            anchor - boundary_tolerance > right_boundary) {
            continue;
        }

        if (!first_column.has_value()) {
            first_column = column_index;
        }
        ++covered_columns;
    }

    if (!first_column.has_value() || covered_columns <= 1U) {
        return std::nullopt;
    }

    return GroupHeaderSpan{*first_column, covered_columns};
}

[[nodiscard]] std::vector<std::optional<GroupHeaderSpan>>
detect_group_header_row_spans(const std::vector<double> &anchors,
                              const CandidateLine &row) {
    std::vector<std::optional<GroupHeaderSpan>> spans;
    spans.reserve(row.clusters.size());

    std::size_t next_column = 0U;
    for (std::size_t cluster_index = 0U; cluster_index < row.clusters.size();
         ++cluster_index) {
        auto span = infer_group_header_span(anchors, row, cluster_index);
        if (!span.has_value() || span->column_span <= 1U ||
            span->column_index != next_column ||
            span->column_index + span->column_span > anchors.size()) {
            return {};
        }

        next_column = span->column_index + span->column_span;
        spans.push_back(span);
    }

    if (next_column != anchors.size()) {
        return {};
    }

    return spans;
}

[[nodiscard]] bool is_summary_label_text(std::string_view text) {
    std::string normalized;
    normalized.reserve(text.size());
    for (unsigned char ch : text) {
        if (std::isalnum(ch)) {
            normalized.push_back(
                static_cast<char>(std::tolower(ch)));
        }
    }

    return normalized.find("total") != std::string::npos;
}

[[nodiscard]] std::optional<GroupHeaderSpan>
detect_summary_label_span(const std::vector<double> &anchors,
                          const CandidateLine &row,
                          std::size_t cluster_index,
                          std::size_t first_complete_row_index,
                          std::size_t row_index) {
    if (anchors.size() < 3U || row_index <= first_complete_row_index ||
        row.clusters.size() != 2U || cluster_index != 0U) {
        return std::nullopt;
    }

    const auto first_column =
        nearest_column_index(anchors, row.clusters.front().bounds.x_points);
    const auto last_column =
        nearest_column_index(anchors, row.clusters.back().bounds.x_points);
    if (first_column >= last_column) {
        return std::nullopt;
    }

    if (!is_summary_label_text(row.clusters.front().text) ||
        !is_short_header_label(row.clusters.front().text) ||
        !is_data_like_cell_text(row.clusters.back().text)) {
        return std::nullopt;
    }

    return GroupHeaderSpan{first_column, last_column - first_column};
}

[[nodiscard]] std::size_t count_text_cells(const PdfParsedTableRow &row) {
    return static_cast<std::size_t>(std::count_if(
        row.cells.begin(), row.cells.end(),
        [](const PdfParsedTableCell &cell) { return cell.has_text; }));
}

[[nodiscard]] bool is_column_covered_by_text_span(
    const PdfParsedTableRow &row, std::size_t column_index) {
    return std::any_of(row.cells.begin(), row.cells.end(),
                       [column_index](const PdfParsedTableCell &cell) {
                           return cell.has_text &&
                                  cell.column_index < column_index &&
                                  cell.column_index + cell.column_span >
                                      column_index;
                       });
}

[[nodiscard]] bool row_has_text_span(const PdfParsedTableRow &row) {
    return std::any_of(row.cells.begin(), row.cells.end(),
                       [](const PdfParsedTableCell &cell) {
                           return cell.has_text && cell.column_span > 1U;
                       });
}

[[nodiscard]] std::size_t detect_row_span(
    const std::vector<PdfParsedTableRow> &rows, std::size_t row_index,
    std::size_t column_index) {
    if (row_index >= rows.size()) {
        return 1U;
    }

    const auto &current_row = rows[row_index];
    if (current_row.cells.size() <= column_index) {
        return 1U;
    }

    const auto &anchor_cell = current_row.cells[column_index];
    if (!anchor_cell.has_text || count_text_cells(current_row) < 2U) {
        return 1U;
    }

    std::size_t row_span = 1U;
    for (std::size_t next_row_index = row_index + 1U;
         next_row_index < rows.size(); ++next_row_index) {
        const auto &next_row = rows[next_row_index];
        if (next_row.cells.size() <= column_index) {
            break;
        }
        if (count_text_cells(next_row) < 2U) {
            break;
        }

        const auto &continuation_cell = next_row.cells[column_index];
        if (continuation_cell.has_text) {
            break;
        }
        if (row_has_text_span(next_row)) {
            break;
        }
        if (is_column_covered_by_text_span(next_row, column_index)) {
            break;
        }

        ++row_span;
    }

    return row_span;
}

[[nodiscard]] bool build_table_candidate(
    const std::vector<CandidateLine> &rows, PdfParsedTableCandidate &candidate) {
    const bool is_two_row_table = is_conservative_two_row_table(rows);
    const bool is_two_column_key_value_table =
        is_conservative_two_column_key_value_table(rows);

    if (rows.size() < 3U && !is_two_row_table) {
        return false;
    }

    const auto anchors = build_column_anchors(rows);
    const bool has_regular_spacing = has_regular_column_spacing(anchors);
    const bool is_irregular_header_table =
        !has_regular_spacing &&
        is_conservative_irregular_header_table(rows, anchors);
    if (anchors.size() < 3U &&
        !(anchors.size() == 2U && is_two_column_key_value_table)) {
        return false;
    }
    if (!has_regular_spacing && !is_irregular_header_table) {
        return false;
    }

    candidate.column_anchor_x_points = anchors;
    candidate.rows.reserve(rows.size());

    const auto first_complete_column_row = std::find_if(
        rows.begin(), rows.end(), [&anchors](const CandidateLine &row) {
            return row.clusters.size() == anchors.size();
        });
    const std::optional<std::size_t> first_complete_column_row_index =
        first_complete_column_row == rows.end()
            ? std::nullopt
            : std::make_optional(static_cast<std::size_t>(
                  std::distance(rows.begin(), first_complete_column_row)));
    std::vector<std::vector<std::optional<GroupHeaderSpan>>> group_header_spans(
        rows.size());
    if (first_complete_column_row_index.has_value()) {
        for (std::size_t row_index = 0U;
             row_index < *first_complete_column_row_index;
             ++row_index) {
            auto row_spans =
                detect_group_header_row_spans(anchors, rows[row_index]);
            if (row_spans.empty()) {
                break;
            }
            group_header_spans[row_index] = std::move(row_spans);
        }
    }

    for (std::size_t source_row_index = 0U; source_row_index < rows.size();
         ++source_row_index) {
        const auto &source_row = rows[source_row_index];
        PdfParsedTableRow row;
        row.cells.reserve(anchors.size());
        for (std::size_t column_index = 0U; column_index < anchors.size();
             ++column_index) {
            PdfParsedTableCell cell;
            cell.column_index = column_index;
            row.cells.push_back(cell);
        }

        for (std::size_t cluster_index = 0U;
             cluster_index < source_row.clusters.size(); ++cluster_index) {
            const auto &cluster = source_row.clusters[cluster_index];
            std::optional<GroupHeaderSpan> group_header_span;
            if (source_row_index < group_header_spans.size() &&
                cluster_index < group_header_spans[source_row_index].size()) {
                group_header_span =
                    group_header_spans[source_row_index][cluster_index];
            }
            if (!group_header_span.has_value() &&
                first_complete_column_row_index.has_value()) {
                group_header_span = detect_summary_label_span(
                    anchors, source_row, cluster_index,
                    *first_complete_column_row_index, source_row_index);
            }
            const auto column_index =
                group_header_span.has_value()
                    ? group_header_span->column_index
                    : nearest_column_index(anchors, cluster.bounds.x_points);
            auto &cell = row.cells[column_index];
            cell.column_span =
                group_header_span.has_value()
                    ? group_header_span->column_span
                    : detect_column_span(anchors, cluster, column_index);
            if (!cell.has_text) {
                cell.text = cluster.text;
                cell.bounds = cluster.bounds;
                cell.has_text = true;
            } else {
                cell.text.push_back('\n');
                cell.text += cluster.text;
                expand_rect(cell.bounds, cluster.bounds);
            }

            expand_rect(row.bounds, cluster.bounds);
            expand_rect(candidate.bounds, cluster.bounds);
        }

        candidate.rows.push_back(std::move(row));
    }

    for (std::size_t row_index = 0U; row_index < candidate.rows.size();
         ++row_index) {
        auto &row = candidate.rows[row_index];
        for (std::size_t column_index = 0U; column_index < row.cells.size();
             ++column_index) {
            row.cells[column_index].row_span =
                detect_row_span(candidate.rows, row_index, column_index);
        }
    }

    return !candidate.rows.empty();
}
