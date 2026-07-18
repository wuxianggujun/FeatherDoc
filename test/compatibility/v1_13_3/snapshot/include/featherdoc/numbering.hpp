#pragma once

#include <constants.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace featherdoc {

struct numbering_level_definition {
    featherdoc::list_kind kind{};
    std::uint32_t start{1U};
    std::uint32_t level{0U};
    std::string text_pattern;
};

struct numbering_definition {
    std::string name;
    std::vector<featherdoc::numbering_level_definition> levels;
};

struct paragraph_style_numbering_link {
    std::string style_id;
    std::uint32_t level{0U};
};

struct numbering_level_override_summary {
    std::uint32_t level{};
    std::optional<std::uint32_t> start_override;
    std::optional<featherdoc::numbering_level_definition> level_definition;
};

struct numbering_instance_summary {
    std::uint32_t instance_id{};
    std::vector<featherdoc::numbering_level_override_summary> level_overrides;
};

struct numbering_definition_summary {
    std::uint32_t definition_id{};
    std::string name;
    std::vector<featherdoc::numbering_level_definition> levels;
    std::vector<std::uint32_t> instance_ids;
    std::vector<featherdoc::numbering_instance_summary> instances;
};

struct numbering_instance_lookup_summary {
    std::uint32_t definition_id{};
    std::string definition_name;
    featherdoc::numbering_instance_summary instance;
};

struct numbering_catalog_definition {
    featherdoc::numbering_definition definition{};
    std::vector<featherdoc::numbering_instance_summary> instances;
};

struct numbering_catalog {
    std::vector<featherdoc::numbering_catalog_definition> definitions;
};

struct imported_numbering_definition_summary {
    std::string name;
    std::uint32_t definition_id{};
    std::vector<std::uint32_t> instance_ids;
};

struct numbering_catalog_import_summary {
    std::size_t input_definition_count{};
    std::size_t imported_definition_count{};
    std::size_t imported_instance_count{};
    std::vector<featherdoc::imported_numbering_definition_summary> definitions;

    explicit operator bool() const noexcept {
        return this->input_definition_count == this->imported_definition_count;
    }
};

} // namespace featherdoc
