#pragma once

#include <featherdoc/document_core.hpp>

#include "document_archive_fingerprint.hpp"

#include <filesystem>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace featherdoc::detail {

// Extra package bookkeeping lives behind xml_handle_lifetime's existing
// opaque state so patch releases do not change Document's public object
// layout. This is intentionally an implementation-only type.
struct document_singleton_part_entries final {
    std::string settings{"word/settings.xml"};
    std::string numbering{"word/numbering.xml"};
    std::string styles{"word/styles.xml"};
    std::string footnotes{"word/footnotes.xml"};
    std::string endnotes{"word/endnotes.xml"};
    std::string comments{"word/comments.xml"};
    std::string comments_extended{"word/commentsExtended.xml"};
};

struct document_extension_state final {
    document_singleton_part_entries singleton_part_entries;
    // Keep package I/O bound to the path that open() resolved. document_path
    // intentionally preserves the caller-provided spelling for path().
    std::filesystem::path source_archive_io_path;
    std::set<std::string> source_archive_part_identities;
    mutable source_archive_fingerprint source_archive_snapshot;
    std::unordered_set<std::string> ignored_singleton_relationship_types;
    std::unordered_map<std::string, std::vector<std::string>>
        related_part_relationship_alias_ids;
    package_validation_mode opened_validation_mode{
        package_validation_mode::strict};
};

} // namespace featherdoc::detail
