#include "featherdoc_cli_paragraph_inspect_load.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

auto inspection_error_message(const featherdoc::Document &document,
                              std::string fallback) -> std::string {
    const auto &error_info = document.last_error();
    if (!error_info.detail.empty()) {
        return error_info.detail;
    }
    if (error_info.code) {
        return error_info.code.message();
    }
    return fallback;
}

auto collect_body_paragraph_blocks(
    featherdoc::Document &document,
    std::vector<featherdoc::body_block_inspection_summary> &paragraph_blocks,
    std::string &error_message) -> bool {
    paragraph_blocks.clear();
    const auto body_blocks = document.inspect_body_blocks();
    if (document.last_error().code) {
        error_message = inspection_error_message(
            document, "failed to inspect body block metadata");
        return false;
    }

    paragraph_blocks.reserve(body_blocks.size());
    for (const auto &block : body_blocks) {
        if (block.kind == featherdoc::body_block_kind::paragraph) {
            paragraph_blocks.push_back(block);
        }
    }
    return true;
}

} // namespace

auto load_body_paragraph_summaries(
    featherdoc::Document &document,
    std::vector<inspected_body_paragraph> &paragraphs,
    std::string &error_message) -> bool {
    paragraphs.clear();

    auto options = featherdoc::paragraph_inspection_options{};
    options.resolve_numbering_metadata = false;
    const auto document_paragraphs =
        document.inspect_paragraphs_with_options(options);
    if (document.last_error().code) {
        error_message = inspection_error_message(
            document, "failed to inspect body paragraphs");
        return false;
    }

    auto paragraph_blocks =
        std::vector<featherdoc::body_block_inspection_summary>{};
    if (!collect_body_paragraph_blocks(document, paragraph_blocks,
                                       error_message)) {
        return false;
    }

    if (document_paragraphs.size() != paragraph_blocks.size()) {
        error_message =
            "loaded body paragraph metadata is internally inconsistent";
        return false;
    }

    paragraphs.reserve(document_paragraphs.size());
    for (std::size_t index = 0U; index < document_paragraphs.size(); ++index) {
        const auto &source = document_paragraphs[index];
        const auto &block = paragraph_blocks[index];
        if (source.index != block.item_index) {
            error_message =
                "loaded body paragraph order is internally inconsistent";
            return false;
        }

        auto summary = inspected_body_paragraph{};
        summary.index = source.index;
        summary.section_index = block.section_index;
        summary.style_id = source.style_id;
        if (source.numbering.has_value()) {
            summary.numbering_id = source.numbering->num_id;
            summary.numbering_level = source.numbering->level;
        }

        if (index + 1U < paragraph_blocks.size()) {
            summary.has_section_break =
                paragraph_blocks[index + 1U].section_index >
                block.section_index;
        } else {
            summary.has_section_break =
                document.section_count() > block.section_index + 1U;
        }
        summary.text = source.text;
        paragraphs.push_back(std::move(summary));
    }

    return true;
}

auto load_body_run_summaries(featherdoc::Document &document,
                             std::size_t paragraph_index,
                             std::vector<inspected_body_run> &runs,
                             bool &paragraph_found,
                             std::string &error_message) -> bool {
    runs.clear();
    paragraph_found = false;

    auto paragraph_blocks =
        std::vector<featherdoc::body_block_inspection_summary>{};
    if (!collect_body_paragraph_blocks(document, paragraph_blocks,
                                       error_message)) {
        return false;
    }
    for (const auto &block : paragraph_blocks) {
        if (block.item_index == paragraph_index) {
            paragraph_found = true;
            break;
        }
    }
    if (!paragraph_found) {
        return true;
    }

    const auto document_runs =
        document.inspect_paragraph_runs(paragraph_index);
    if (document.last_error().code) {
        error_message = inspection_error_message(
            document, "failed to inspect body paragraph runs");
        return false;
    }

    runs.reserve(document_runs.size());
    for (const auto &source : document_runs) {
        auto summary = inspected_body_run{};
        summary.index = source.index;
        summary.style_id = source.style_id;
        summary.font_family = source.font_family;
        summary.east_asia_font_family = source.east_asia_font_family;
        summary.text_color = source.text_color;
        summary.bold = source.bold;
        summary.italic = source.italic;
        summary.underline = source.underline;
        summary.font_size_points = source.font_size_points;
        summary.language = source.language;
        summary.text = source.text;
        runs.push_back(std::move(summary));
    }

    return true;
}

} // namespace featherdoc_cli
