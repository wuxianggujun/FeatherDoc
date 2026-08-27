#include "document_archive_limit_helpers.hpp"
#include "document_extension_state.hpp"
#include "featherdoc.hpp"
#include "numeric_helpers.hpp"
#include "package_content_types_xml_helpers.hpp"
#include "package_path_helpers.hpp"
#include "package_relationships_xml_helpers.hpp"
#include "singleton_part_attachment_helpers.hpp"
#include "wordprocessingml_namespace_helpers.hpp"
#include "xml_document_clone_helpers.hpp"
#include "xml_document_initialization_helpers.hpp"
#include "xml_parse_error_helpers.hpp"
#include <featherdoc/detail/path.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include <zip.h>

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};
constexpr auto document_relationships_xml_entry =
    std::string_view{"word/_rels/document.xml.rels"};
constexpr auto content_types_xml_entry =
    std::string_view{"[Content_Types].xml"};
constexpr auto numbering_relationship_type =
    std::string_view{"http://schemas.openxmlformats.org/officeDocument/2006/"
                     "relationships/numbering"};
constexpr auto numbering_content_type = std::string_view{
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"};
constexpr auto styles_relationship_type =
    std::string_view{"http://schemas.openxmlformats.org/officeDocument/2006/"
                     "relationships/styles"};
constexpr auto styles_content_type = std::string_view{
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"};
constexpr auto empty_numbering_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
</w:numbering>
)"};
constexpr auto managed_bullet_list_name =
    std::string_view{"FeatherDocBulletList"};
constexpr auto managed_decimal_list_name =
    std::string_view{"FeatherDocDecimalList"};
constexpr auto max_list_level = 8U;

auto set_last_error(featherdoc::document_error_info &error_info,
                    std::error_code code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    error_info.code = code;
    error_info.detail = std::move(detail);
    error_info.entry_name = std::move(entry_name);
    error_info.xml_offset = std::move(xml_offset);
    return code;
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    featherdoc::document_errc code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    return set_last_error(error_info, featherdoc::make_error_code(code),
                          std::move(detail), std::move(entry_name),
                          std::move(xml_offset));
}

enum class zip_entry_read_status {
    ok,
    missing,
    limit_exceeded,
    read_failed,
};

auto zip_error_text(int error_number) -> std::string {
    if (const char *message = zip_strerror(error_number); message != nullptr) {
        return message;
    }

    return "unknown zip error";
}

auto read_zip_entry_text(
    zip_t *archive, std::string_view entry_name, std::string &content,
    const featherdoc::archive_limits *xml_limits,
    featherdoc::document_error_info *last_error_info,
    const featherdoc::detail::archive_entry_catalog &catalog)
    -> zip_entry_read_status {
    if (featherdoc::detail::open_archive_entry_by_package_name(
            archive, catalog, entry_name) != 0) {
        return zip_entry_read_status::missing;
    }

    if (xml_limits != nullptr && last_error_info != nullptr) {
        if (const auto limit_error =
                featherdoc::detail::enforce_open_zip_entry_xml_size_limit(
                    archive, *xml_limits, entry_name, *last_error_info)) {
            zip_entry_close(archive);
            content.clear();
            return zip_entry_read_status::limit_exceeded;
        }
    }

    void *buffer = nullptr;
    size_t buffer_size = 0;
    const auto read_result = zip_entry_read(archive, &buffer, &buffer_size);
    const auto close_result = zip_entry_close(archive);
    std::unique_ptr<void, decltype(&std::free)> buffer_guard{buffer, &std::free};

    if (read_result < 0 || close_result != 0 ||
        (buffer_size > 0U && buffer == nullptr)) {
        return zip_entry_read_status::read_failed;
    }

    if (buffer_size == 0U) {
        content.clear();
    } else {
        content.assign(static_cast<const char *>(buffer), buffer_size);
    }
    return zip_entry_read_status::ok;
}

auto make_override_part_name(std::string_view entry_name) -> std::string {
    return featherdoc::detail::make_package_content_type_part_name(entry_name);
}

auto make_document_relationship_target(std::string_view entry_name)
    -> std::string {
    return featherdoc::detail::make_package_relationship_target(
        document_xml_entry, entry_name);
}

struct singleton_part_attachment_option_storage final {
    std::string relationship_target;
    std::string override_part_name;
    featherdoc::detail::singleton_part_attachment_options options;

    void initialize(std::string_view relationship_type,
                    std::string_view entry_name,
                    std::string_view content_type) {
        this->relationship_target =
            make_document_relationship_target(entry_name);
        this->override_part_name = make_override_part_name(entry_name);
        this->options = {
            relationship_type,
            this->relationship_target,
            this->override_part_name,
            content_type,
            document_relationships_xml_entry,
            content_types_xml_entry,
            featherdoc::document_errc::relationships_xml_parse_failed,
            featherdoc::document_errc::content_types_xml_parse_failed};
    }
};

auto find_document_relationship_by_type(pugi::xml_node relationships,
                                        std::string_view relationship_type)
    -> pugi::xml_node {
    for (auto relationship =
             featherdoc::detail::first_package_relationship(relationships);
         relationship != pugi::xml_node{};
         relationship =
             featherdoc::detail::next_package_relationship(relationship)) {
        if (std::string_view{relationship.attribute("Type").value()} ==
            relationship_type) {
            return relationship;
        }
    }

    return {};
}

auto ensure_attribute_value(pugi::xml_node node, const char *name,
                            std::string_view value) -> bool {
    return featherdoc::detail::checked_set_xml_attribute_value(node, name,
                                                               value);
}

auto parse_u32_attribute_value(const char *text)
    -> std::optional<std::uint32_t> {
    return featherdoc::detail::parse_integer_strict<std::uint32_t>(text);
}

auto max_numbering_id(pugi::xml_node numbering_root, const char *child_name,
                      const char *attribute_name) -> std::uint32_t;

auto first_style_properties_successor(pugi::xml_node style) -> pugi::xml_node {
    constexpr auto successors = std::array<std::string_view, 6U>{
        "w:rPr", "w:tblPr", "w:trPr", "w:tcPr", "w:tblStylePr",
        "w:extLst"};
    for (auto child = style.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto child_name = std::string_view{child.name()};
        if (std::find(successors.begin(), successors.end(), child_name) !=
            successors.end()) {
            return child;
        }
    }
    return {};
}

auto ensure_style_paragraph_properties_node(pugi::xml_node style)
    -> pugi::xml_node {
    if (style == pugi::xml_node{}) {
        return {};
    }

    auto paragraph_properties = style.child("w:pPr");
    if (paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }

    if (const auto successor = first_style_properties_successor(style);
        successor != pugi::xml_node{}) {
        return featherdoc::detail::checked_insert_xml_element_before(
            style, "w:pPr", successor);
    }

    return featherdoc::detail::checked_append_xml_element(style, "w:pPr");
}

auto node_has_attributes(pugi::xml_node node) -> bool {
    return node.first_attribute() != pugi::xml_attribute{};
}

auto max_numbering_id(pugi::xml_node numbering_root, const char *child_name,
                      const char *attribute_name) -> std::uint32_t {
    std::uint32_t max_id = 0U;
    for (auto child = numbering_root.child(child_name);
         child != pugi::xml_node{}; child = child.next_sibling(child_name)) {
        if (const auto parsed_id = parse_u32_attribute_value(
                child.attribute(attribute_name).value())) {
            max_id = std::max(max_id, *parsed_id);
        }
    }
    return max_id;
}

auto list_name_for(featherdoc::list_kind kind) -> std::string_view {
    switch (kind) {
    case featherdoc::list_kind::bullet:
        return managed_bullet_list_name;
    case featherdoc::list_kind::decimal:
        return managed_decimal_list_name;
    }

    return managed_bullet_list_name;
}

auto numbering_format_name(featherdoc::list_kind kind) -> std::string_view {
    switch (kind) {
    case featherdoc::list_kind::bullet:
        return "bullet";
    case featherdoc::list_kind::decimal:
        return "decimal";
    }

    return "bullet";
}

auto bullet_symbol_for_level(std::uint32_t level) -> std::string_view {
    constexpr auto bullet = std::string_view{"\xE2\x80\xA2"};
    constexpr auto circle = std::string_view{"o"};
    constexpr auto square = std::string_view{"\xE2\x96\xAA"};
    constexpr std::array<std::string_view, 3U> symbols{bullet, circle, square};
    return symbols[level % symbols.size()];
}

auto decimal_level_text(std::uint32_t level) -> std::string {
    std::string text;
    for (std::uint32_t index = 0U; index <= level; ++index) {
        text += "%" + std::to_string(index + 1U);
        text.push_back('.');
    }
    return text;
}

auto find_abstract_numbering_by_name(pugi::xml_node numbering_root,
                                     std::string_view name) -> pugi::xml_node {
    for (auto abstract_num = numbering_root.child("w:abstractNum");
         abstract_num != pugi::xml_node{};
         abstract_num = abstract_num.next_sibling("w:abstractNum")) {
        if (std::string_view{
                abstract_num.child("w:name").attribute("w:val").value()} ==
            name) {
            return abstract_num;
        }
    }

    return {};
}

auto find_abstract_numbering_by_id(pugi::xml_node numbering_root,
                                   std::uint32_t abstract_num_id)
    -> pugi::xml_node {
    for (auto abstract_num = numbering_root.child("w:abstractNum");
         abstract_num != pugi::xml_node{};
         abstract_num = abstract_num.next_sibling("w:abstractNum")) {
        if (parse_u32_attribute_value(
                abstract_num.attribute("w:abstractNumId").value()) ==
            abstract_num_id) {
            return abstract_num;
        }
    }

    return {};
}

auto find_level_definition(pugi::xml_node abstract_num, std::uint32_t level)
    -> pugi::xml_node {
    for (auto level_node = abstract_num.child("w:lvl");
         level_node != pugi::xml_node{};
         level_node = level_node.next_sibling("w:lvl")) {
        if (parse_u32_attribute_value(level_node.attribute("w:ilvl").value()) ==
            level) {
            return level_node;
        }
    }

    return {};
}

auto remove_named_children(pugi::xml_node parent, const char *child_name)
    -> bool {
    if (parent == pugi::xml_node{}) {
        return false;
    }

    for (auto child = parent.child(child_name); child != pugi::xml_node{};) {
        const auto next = child.next_sibling(child_name);
        if (!parent.remove_child(child)) {
            return false;
        }
        child = next;
    }
    return true;
}

auto first_numbering_properties_successor(pugi::xml_node paragraph_properties)
    -> pugi::xml_node {
    constexpr auto successors = std::array<std::string_view, 26U>{
        "w:suppressLineNumbers", "w:pBdr",          "w:shd",
        "w:tabs",                "w:suppressAutoHyphens",
        "w:kinsoku",             "w:wordWrap",      "w:overflowPunct",
        "w:topLinePunct",        "w:autoSpaceDE",   "w:autoSpaceDN",
        "w:bidi",                "w:adjustRightInd", "w:snapToGrid",
        "w:spacing",             "w:ind",           "w:contextualSpacing",
        "w:mirrorIndents",       "w:suppressOverlap", "w:jc",
        "w:textDirection",       "w:textAlignment", "w:textboxTightWrap",
        "w:outlineLvl",          "w:divId",         "w:cnfStyle"};
    for (auto child = paragraph_properties.first_child();
         child != pugi::xml_node{}; child = child.next_sibling()) {
        const auto child_name = std::string_view{child.name()};
        if (std::find(successors.begin(), successors.end(), child_name) !=
            successors.end() || child_name == "w:rPr" ||
            child_name == "w:sectPr" || child_name == "w:pPrChange") {
            return child;
        }
    }
    return {};
}

auto append_numbering_properties_in_schema_order(
    pugi::xml_node paragraph_properties) -> pugi::xml_node {
    if (const auto successor =
            first_numbering_properties_successor(paragraph_properties);
        successor != pugi::xml_node{}) {
        return featherdoc::detail::checked_insert_xml_element_before(
            paragraph_properties, "w:numPr", successor);
    }
    return featherdoc::detail::checked_append_xml_element(
        paragraph_properties, "w:numPr");
}

auto has_direct_numbering_properties(pugi::xml_node parent) -> bool {
    for (auto paragraph_properties = parent.child("w:pPr");
         paragraph_properties != pugi::xml_node{};
         paragraph_properties =
             paragraph_properties.next_sibling("w:pPr")) {
        if (paragraph_properties.child("w:numPr") != pugi::xml_node{}) {
            return true;
        }
    }
    return false;
}

auto clear_direct_numbering_properties(pugi::xml_node parent, bool &changed)
    -> bool {
    changed = false;
    for (auto paragraph_properties = parent.child("w:pPr");
         paragraph_properties != pugi::xml_node{};) {
        const auto next_properties =
            paragraph_properties.next_sibling("w:pPr");
        bool removed_from_properties = false;
        for (auto numbering_properties =
                 paragraph_properties.child("w:numPr");
             numbering_properties != pugi::xml_node{};) {
            const auto next_numbering_properties =
                numbering_properties.next_sibling("w:numPr");
            if (!paragraph_properties.remove_child(numbering_properties)) {
                return false;
            }
            removed_from_properties = true;
            changed = true;
            numbering_properties = next_numbering_properties;
        }
        if (removed_from_properties &&
            paragraph_properties.first_child() == pugi::xml_node{} &&
            !node_has_attributes(paragraph_properties) &&
            !parent.remove_child(paragraph_properties)) {
            return false;
        }
        paragraph_properties = next_properties;
    }
    return true;
}

class paragraph_properties_transaction final {
  public:
    paragraph_properties_transaction() = default;
    paragraph_properties_transaction(const paragraph_properties_transaction &) =
        delete;
    auto operator=(const paragraph_properties_transaction &)
        -> paragraph_properties_transaction & = delete;

    ~paragraph_properties_transaction() noexcept { this->cleanup(); }

    [[nodiscard]] auto prepare(pugi::xml_node paragraph) -> bool {
        if (paragraph == pugi::xml_node{} ||
            this->scratch_ != pugi::xml_node{}) {
            return false;
        }
        this->paragraph_ = paragraph;
        this->original_properties_ = paragraph.child("w:pPr");
        this->scratch_ = paragraph.append_child(pugi::node_element);
        if (this->scratch_ == pugi::xml_node{}) {
            return false;
        }

        if (this->original_properties_ != pugi::xml_node{}) {
            if (featherdoc::detail::checked_append_copy_xml_node(
                    this->original_properties_, this->scratch_) !=
                featherdoc::detail::xml_document_clone_status::success) {
                return false;
            }
            this->working_properties_ = this->scratch_.child("w:pPr");
        } else {
            this->working_properties_ =
                featherdoc::detail::checked_append_xml_element(this->scratch_,
                                                                "w:pPr");
        }
        return this->working_properties_ != pugi::xml_node{};
    }

    [[nodiscard]] auto working_properties() const noexcept -> pugi::xml_node {
        return this->working_properties_;
    }

    [[nodiscard]] auto commit() noexcept -> bool {
        if (this->paragraph_ == pugi::xml_node{} ||
            this->scratch_ == pugi::xml_node{} ||
            this->working_properties_ == pugi::xml_node{}) {
            return false;
        }

        const auto anchor = this->original_properties_ != pugi::xml_node{}
                                ? this->original_properties_
                                : this->paragraph_.first_child();
        const auto inserted = this->paragraph_.insert_move_before(
            this->working_properties_, anchor);
        if (inserted == pugi::xml_node{}) {
            return false;
        }
        this->working_properties_ = {};

        if (!this->paragraph_.remove_child(this->scratch_)) {
            (void)this->paragraph_.remove_child(inserted);
            return false;
        }
        this->scratch_ = {};

        if (this->original_properties_ != pugi::xml_node{} &&
            !this->paragraph_.remove_child(this->original_properties_)) {
            (void)this->paragraph_.remove_child(inserted);
            return false;
        }
        this->original_properties_ = {};
        return true;
    }

  private:
    void cleanup() noexcept {
        if (this->paragraph_ != pugi::xml_node{} &&
            this->scratch_ != pugi::xml_node{}) {
            (void)this->paragraph_.remove_child(this->scratch_);
        }
        this->scratch_ = {};
        this->working_properties_ = {};
    }

    pugi::xml_node paragraph_{};
    pugi::xml_node scratch_{};
    pugi::xml_node original_properties_{};
    pugi::xml_node working_properties_{};
};

auto find_style_node(pugi::xml_node styles_root, std::string_view style_id)
    -> pugi::xml_node {
    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        if (std::string_view{style.attribute("w:styleId").value()} ==
            style_id) {
            return style;
        }
    }

    return {};
}

auto find_numbering_instance_for_abstract(pugi::xml_node numbering_root,
                                          std::uint32_t abstract_num_id)
    -> pugi::xml_node {
    for (auto num = numbering_root.child("w:num"); num != pugi::xml_node{};
         num = num.next_sibling("w:num")) {
        if (parse_u32_attribute_value(
                num.child("w:abstractNumId").attribute("w:val").value()) !=
            abstract_num_id) {
            continue;
        }
        if (!parse_u32_attribute_value(num.attribute("w:numId").value())
                 .has_value()) {
            continue;
        }
        return num;
    }

    return {};
}

auto find_numbering_instance_by_id(pugi::xml_node numbering_root,
                                   std::uint32_t num_id) -> pugi::xml_node {
    for (auto num = numbering_root.child("w:num"); num != pugi::xml_node{};
         num = num.next_sibling("w:num")) {
        if (parse_u32_attribute_value(num.attribute("w:numId").value()) ==
            num_id) {
            return num;
        }
    }

    return {};
}

auto append_custom_level_definition(
    pugi::xml_node abstract_num,
    const featherdoc::numbering_level_definition &definition) -> bool {
    auto level_node =
        featherdoc::detail::checked_append_xml_element(abstract_num, "w:lvl");
    if (level_node == pugi::xml_node{}) {
        return false;
    }
    if (!ensure_attribute_value(level_node, "w:ilvl",
                                std::to_string(definition.level))) {
        return false;
    }

    auto start =
        featherdoc::detail::checked_append_xml_element(level_node, "w:start");
    if (start == pugi::xml_node{}) {
        return false;
    }
    if (!ensure_attribute_value(start, "w:val",
                                std::to_string(definition.start))) {
        return false;
    }

    auto num_format =
        featherdoc::detail::checked_append_xml_element(level_node, "w:numFmt");
    auto level_text =
        featherdoc::detail::checked_append_xml_element(level_node, "w:lvlText");
    auto level_justification =
        featherdoc::detail::checked_append_xml_element(level_node, "w:lvlJc");
    auto paragraph_properties =
        featherdoc::detail::checked_append_xml_element(level_node, "w:pPr");
    if (num_format == pugi::xml_node{} || level_text == pugi::xml_node{} ||
        level_justification == pugi::xml_node{} ||
        paragraph_properties == pugi::xml_node{}) {
        return false;
    }

    if (!ensure_attribute_value(num_format, "w:val",
                                numbering_format_name(definition.kind)) ||
        !ensure_attribute_value(level_text, "w:val", definition.text_pattern) ||
        !ensure_attribute_value(level_justification, "w:val", "left")) {
        return false;
    }

    auto indentation = featherdoc::detail::checked_append_xml_element(
        paragraph_properties, "w:ind");
    if (indentation == pugi::xml_node{}) {
        return false;
    }
    const auto left_indent = std::to_string((definition.level + 1U) * 720U);
    return ensure_attribute_value(indentation, "w:left", left_indent) &&
           ensure_attribute_value(indentation, "w:hanging", "360");
}

auto append_level_definition(pugi::xml_node abstract_num,
                             featherdoc::list_kind kind, std::uint32_t level)
    -> bool {
    auto definition = featherdoc::numbering_level_definition{};
    definition.kind = kind;
    definition.start = 1U;
    definition.level = level;
    definition.text_pattern = kind == featherdoc::list_kind::bullet
                                  ? std::string{bullet_symbol_for_level(level)}
                                  : decimal_level_text(level);
    return append_custom_level_definition(abstract_num, definition);
}

auto sort_numbering_levels(
    const std::vector<featherdoc::numbering_level_definition> &levels)
    -> std::vector<featherdoc::numbering_level_definition> {
    auto sorted_levels = levels;
    std::sort(
        sorted_levels.begin(), sorted_levels.end(),
        [](const auto &lhs, const auto &rhs) { return lhs.level < rhs.level; });
    return sorted_levels;
}

#include "document_numbering_summary_helpers.inc"

#include "document_numbering_instance_helpers.inc"

} // namespace

namespace featherdoc {

bool Document::owns_paragraph_handle(
    const Paragraph &target_paragraph,
    std::string_view &owning_part_entry_name) const noexcept {
    owning_part_entry_name = {};
    const auto paragraph_node =
        static_cast<pugi::xml_node>(target_paragraph.current);
    const auto parent = static_cast<pugi::xml_node>(target_paragraph.parent);
    if (paragraph_node == pugi::xml_node{} || parent == pugi::xml_node{} ||
        std::string_view{paragraph_node.name()} != "w:p" ||
        paragraph_node.parent() != parent) {
        return false;
    }

    const auto is_descendant_of = [](pugi::xml_node node,
                                     pugi::xml_node story_root) noexcept {
        for (; node != pugi::xml_node{}; node = node.parent()) {
            if (node == story_root) {
                return true;
            }
        }
        return false;
    };

    const auto body = this->document.child("w:document").child("w:body");
    if (paragraph_node.root() == this->document &&
        is_descendant_of(paragraph_node, body)) {
        owning_part_entry_name = document_xml_entry;
        return true;
    }

    for (const auto &part : this->header_parts) {
        if (part && paragraph_node.root() == part->xml &&
            is_descendant_of(paragraph_node, part->xml.child("w:hdr"))) {
            owning_part_entry_name = part->entry_name;
            return true;
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part && paragraph_node.root() == part->xml &&
            is_descendant_of(paragraph_node, part->xml.child("w:ftr"))) {
            owning_part_entry_name = part->entry_name;
            return true;
        }
    }
    return false;
}

#include "document_numbering_catalog_methods.inc"

std::optional<std::uint32_t> Document::ensure_style_linked_numbering(
    const featherdoc::numbering_definition &definition,
    const std::vector<featherdoc::paragraph_style_numbering_link>
        &style_links) try {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing "
                       "style-linked numbering");
        return std::nullopt;
    }

    auto validation_detail = std::string{};
    if (!validate_numbering_definition(definition, validation_detail)) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            std::move(validation_detail),
            this->extension_state().singleton_part_entries.numbering);
        return std::nullopt;
    }

    if (style_links.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "expected at least one paragraph style link",
                       this->extension_state().singleton_part_entries.styles);
        return std::nullopt;
    }

    for (std::size_t index = 0; index < style_links.size(); ++index) {
        const auto &style_link = style_links[index];
        if (style_link.style_id.empty()) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::invalid_argument),
                "style link style id must not be empty",
                this->extension_state().singleton_part_entries.styles);
            return std::nullopt;
        }

        if (style_link.level > max_list_level) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::invalid_argument),
                "style link numbering level must be in the range [0, 8]",
                this->extension_state().singleton_part_entries.styles);
            return std::nullopt;
        }

        const auto level_is_defined =
            std::any_of(definition.levels.begin(), definition.levels.end(),
                        [&](const auto &level_definition) {
                            return level_definition.level == style_link.level;
                        });
        if (!level_is_defined) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::invalid_argument),
                "requested numbering level is not defined by the "
                "target definition",
                this->extension_state().singleton_part_entries.numbering);
            return std::nullopt;
        }

        for (std::size_t duplicate_index = index + 1U;
             duplicate_index < style_links.size(); ++duplicate_index) {
            if (style_links[duplicate_index].style_id == style_link.style_id) {
                set_last_error(
                    this->last_error_info,
                    std::make_error_code(std::errc::invalid_argument),
                    "style id '" + style_link.style_id +
                        "' appears more than once in the style link list",
                    this->extension_state().singleton_part_entries.styles);
                return std::nullopt;
            }
        }
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }
    if (const auto error = this->ensure_numbering_loaded()) {
        return std::nullopt;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::styles_xml_parse_failed,
            "'" + this->extension_state().singleton_part_entries.styles +
                "' does not contain a w:styles root",
            this->extension_state().singleton_part_entries.styles);
        return std::nullopt;
    }

    for (const auto &style_link : style_links) {
        const auto style = find_style_node(styles_root, style_link.style_id);
        if (style == pugi::xml_node{}) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::invalid_argument),
                "style id '" + style_link.style_id + "' was not found in '" +
                    this->extension_state().singleton_part_entries.styles + "'",
                this->extension_state().singleton_part_entries.styles);
            return std::nullopt;
        }

        if (std::string_view{style.attribute("w:type").value()} !=
            "paragraph") {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::invalid_argument),
                "style id '" + style_link.style_id +
                    "' is not a paragraph style and cannot carry "
                    "paragraph numbering",
                this->extension_state().singleton_part_entries.styles);
            return std::nullopt;
        }
    }

    auto numbering_root = this->numbering.child("w:numbering");
    if (numbering_root == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::numbering_xml_parse_failed,
            "'" + this->extension_state().singleton_part_entries.numbering +
                "' does not contain the expected w:numbering root",
            this->extension_state().singleton_part_entries.numbering);
        return std::nullopt;
    }

    const auto instance_plan =
        plan_numbering_definition_instance(numbering_root, definition.name);
    if (!instance_plan.succeeded()) {
        set_numbering_mutation_error(
            this->last_error_info,
            this->extension_state().singleton_part_entries.numbering,
            instance_plan.status, "numbering id has reached UINT32_MAX",
            "failed to reserve a style-linked numbering definition and "
            "instance");
        return std::nullopt;
    }

    if (this->ignored_singleton_relationship_type(styles_relationship_type) ||
        this->ignored_singleton_relationship_type(
            numbering_relationship_type)) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach styles or numbering while a source relationship is invalid",
            std::string{document_relationships_xml_entry});
        return std::nullopt;
    }
    if (!featherdoc::detail::package_relationships_document_allows_mutation(
            this->document_relationships,
            this->has_document_relationships_part)) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach styles or numbering while document relationships are invalid",
            std::string{document_relationships_xml_entry});
        return std::nullopt;
    }
    if (const auto error = this->ensure_content_types_loaded()) {
        return std::nullopt;
    }

    auto working_styles = pugi::xml_document{};
    auto working_numbering = pugi::xml_document{};
    if (featherdoc::detail::checked_clone_xml_document(this->styles,
                                                       working_styles) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info,
            this->extension_state().singleton_part_entries.styles);
        return std::nullopt;
    }
    if (featherdoc::detail::checked_clone_xml_document(
            this->numbering, working_numbering) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info,
            this->extension_state().singleton_part_entries.numbering);
        return std::nullopt;
    }
    styles_root = working_styles.child("w:styles");
    numbering_root = working_numbering.child("w:numbering");

    const auto numbering_definition_id = ensure_custom_abstract_numbering(
        numbering_root, definition,
        instance_plan.plan->reserved_abstract_num_id);
    if (!numbering_definition_id.succeeded()) {
        set_numbering_mutation_error(
            this->last_error_info,
            this->extension_state().singleton_part_entries.numbering,
            numbering_definition_id.status,
            "abstract numbering id has reached UINT32_MAX",
            "failed to create or update the style-linked numbering definition");
        return std::nullopt;
    }

    const auto num_id = ensure_numbering_instance_for_abstract(
        numbering_root, *numbering_definition_id.identifier, std::nullopt,
        false, instance_plan.plan->reserved_num_id);
    if (!num_id.succeeded()) {
        set_numbering_mutation_error(
            this->last_error_info,
            this->extension_state().singleton_part_entries.numbering,
            num_id.status, "numbering instance id has reached UINT32_MAX",
            "failed to create or reuse a numbering instance for the target "
            "definition");
        return std::nullopt;
    }

    for (const auto &style_link : style_links) {
        const auto style = find_style_node(styles_root, style_link.style_id);
        if (!apply_numbering_to_style(style, style_link.level,
                                      *num_id.identifier)) {
            set_last_error(
                this->last_error_info,
                std::make_error_code(std::errc::not_enough_memory),
                "failed to populate numbering metadata for the target style",
                this->extension_state().singleton_part_entries.styles);
            return std::nullopt;
        }
    }

    auto styles_attachment = singleton_part_attachment_option_storage{};
    styles_attachment.initialize(
        styles_relationship_type,
        this->extension_state().singleton_part_entries.styles,
        styles_content_type);
    auto numbering_attachment = singleton_part_attachment_option_storage{};
    numbering_attachment.initialize(
        numbering_relationship_type,
        this->extension_state().singleton_part_entries.numbering,
        numbering_content_type);
    const auto attachment_options = std::array{
        styles_attachment.options, numbering_attachment.options};
    auto attachment_work =
        featherdoc::detail::singleton_part_attachment_work{};
    if (const auto error =
            featherdoc::detail::prepare_checked_singleton_part_attachments(
                this->document_relationships, this->content_types,
                attachment_options, this->last_error_info, attachment_work)) {
        return std::nullopt;
    }

    const auto final_identifier = numbering_definition_id.identifier;
    static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
    this->styles = std::move(working_styles);
    this->numbering = std::move(working_numbering);
    featherdoc::detail::publish_checked_singleton_part_attachment(
        std::move(attachment_work), this->document_relationships,
        this->content_types, this->has_document_relationships_part,
        this->document_relationships_dirty, this->content_types_dirty);
    this->has_styles_part = true;
    this->has_numbering_part = true;
    this->numbering_dirty = true;
    this->styles_dirty = true;
    this->last_error_info.clear();
    return final_identifier;
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info,
        this->extension_state().singleton_part_entries.numbering);
    return std::nullopt;
}

#include "document_numbering_paragraph_methods.inc"

} // namespace featherdoc
