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
#include "xml_namespace_helpers.hpp"
#include "xml_parse_error_helpers.hpp"
#include "xml_traversal_helpers.hpp"
#include <featherdoc/detail/path.hpp>

#include "document_table_style_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <zip.h>

namespace featherdoc::detail {
[[nodiscard]] auto next_named_sibling(pugi::xml_node node,
                                      std::string_view name) -> pugi::xml_node;
[[nodiscard]] auto collect_plain_text_from_xml(pugi::xml_node node)
    -> std::string;
} // namespace featherdoc::detail

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};
constexpr auto document_relationships_xml_entry =
    std::string_view{"word/_rels/document.xml.rels"};
constexpr auto content_types_xml_entry =
    std::string_view{"[Content_Types].xml"};
constexpr auto styles_relationship_type =
    std::string_view{"http://schemas.openxmlformats.org/officeDocument/2006/"
                     "relationships/styles"};
constexpr auto styles_content_type = std::string_view{
    "application/"
    "vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"};
constexpr auto default_styles_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:docDefaults>
    <w:rPrDefault><w:rPr/></w:rPrDefault>
    <w:pPrDefault><w:pPr/></w:pPrDefault>
  </w:docDefaults>
  <w:latentStyles w:defLockedState="0"
                  w:defUIPriority="99"
                  w:defSemiHidden="0"
                  w:defUnhideWhenUsed="0"
                  w:defQFormat="0"
                  w:count="0"/>
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
    <w:qFormat/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="Heading1">
    <w:name w:val="heading 1"/>
    <w:basedOn w:val="Normal"/>
    <w:next w:val="Normal"/>
    <w:uiPriority w:val="9"/>
    <w:qFormat/>
    <w:pPr><w:outlineLvl w:val="0"/></w:pPr>
    <w:rPr><w:b/><w:sz w:val="32"/></w:rPr>
  </w:style>
  <w:style w:type="paragraph" w:styleId="Heading2">
    <w:name w:val="heading 2"/>
    <w:basedOn w:val="Normal"/>
    <w:next w:val="Normal"/>
    <w:uiPriority w:val="9"/>
    <w:qFormat/>
    <w:pPr><w:outlineLvl w:val="1"/></w:pPr>
    <w:rPr><w:b/><w:sz w:val="28"/></w:rPr>
  </w:style>
  <w:style w:type="paragraph" w:styleId="Quote">
    <w:name w:val="Quote"/>
    <w:basedOn w:val="Normal"/>
    <w:uiPriority w:val="29"/>
    <w:qFormat/>
    <w:rPr><w:i/></w:rPr>
  </w:style>
  <w:style w:type="character" w:default="1" w:styleId="DefaultParagraphFont">
    <w:name w:val="Default Paragraph Font"/>
    <w:uiPriority w:val="1"/>
    <w:semiHidden/>
    <w:unhideWhenUsed/>
  </w:style>
  <w:style w:type="character" w:styleId="Emphasis">
    <w:name w:val="Emphasis"/>
    <w:basedOn w:val="DefaultParagraphFont"/>
    <w:uiPriority w:val="20"/>
    <w:qFormat/>
    <w:rPr><w:i/></w:rPr>
  </w:style>
  <w:style w:type="character" w:styleId="Strong">
    <w:name w:val="Strong"/>
    <w:basedOn w:val="DefaultParagraphFont"/>
    <w:uiPriority w:val="21"/>
    <w:qFormat/>
    <w:rPr><w:b/></w:rPr>
  </w:style>
  <w:style w:type="table" w:default="1" w:styleId="TableNormal">
    <w:name w:val="Normal Table"/>
    <w:uiPriority w:val="99"/>
    <w:semiHidden/>
  </w:style>
  <w:style w:type="table" w:styleId="TableGrid">
    <w:name w:val="Table Grid"/>
    <w:basedOn w:val="TableNormal"/>
    <w:uiPriority w:val="59"/>
    <w:tblPr>
      <w:tblBorders>
        <w:top w:val="single" w:sz="4" w:space="0" w:color="auto"/>
        <w:left w:val="single" w:sz="4" w:space="0" w:color="auto"/>
        <w:bottom w:val="single" w:sz="4" w:space="0" w:color="auto"/>
        <w:right w:val="single" w:sz="4" w:space="0" w:color="auto"/>
        <w:insideH w:val="single" w:sz="4" w:space="0" w:color="auto"/>
        <w:insideV w:val="single" w:sz="4" w:space="0" w:color="auto"/>
      </w:tblBorders>
    </w:tblPr>
  </w:style>
</w:styles>
)"};

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

void ensure_attribute_value(pugi::xml_node node, const char *name,
                            std::string_view value) {
    if (node == pugi::xml_node{}) {
        return;
    }

    auto attribute = node.attribute(name);
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute(name);
    }
    attribute.set_value(std::string{value}.c_str());
}

auto node_has_attributes(pugi::xml_node node) -> bool {
    return node.first_attribute() != pugi::xml_attribute{};
}

auto checked_insert_style_element(pugi::xml_node parent, const char *name,
                                  pugi::xml_node before = {})
    -> pugi::xml_node {
    if (parent == pugi::xml_node{} || name == nullptr) {
        return {};
    }

    auto child = before == pugi::xml_node{}
                     ? parent.append_child(pugi::node_element)
                     : parent.insert_child_before(pugi::node_element, before);
    if (child == pugi::xml_node{}) {
        return {};
    }
    if (!child.set_name(name)) {
        parent.remove_child(child);
        return {};
    }
    return child;
}

class applied_style_properties_transaction final {
  public:
    applied_style_properties_transaction() = default;
    applied_style_properties_transaction(
        const applied_style_properties_transaction &) = delete;
    auto operator=(const applied_style_properties_transaction &)
        -> applied_style_properties_transaction & = delete;

    ~applied_style_properties_transaction() noexcept { this->cleanup(); }

    [[nodiscard]] auto prepare(pugi::xml_node owner,
                               const char *properties_name) -> bool {
        if (owner == pugi::xml_node{} || properties_name == nullptr ||
            this->scratch_ != pugi::xml_node{}) {
            return false;
        }

        this->owner_ = owner;
        this->original_properties_ = owner.child(properties_name);
        this->scratch_ = owner.append_child(pugi::node_element);
        if (this->scratch_ == pugi::xml_node{}) {
            return false;
        }

        if (this->original_properties_ != pugi::xml_node{}) {
            if (featherdoc::detail::checked_append_copy_xml_node(
                    this->original_properties_, this->scratch_) !=
                featherdoc::detail::xml_document_clone_status::success) {
                return false;
            }
            this->working_properties_ = this->scratch_.child(properties_name);
        } else {
            this->working_properties_ =
                featherdoc::detail::checked_append_xml_element(
                    this->scratch_, properties_name);
        }
        return this->working_properties_ != pugi::xml_node{};
    }

    [[nodiscard]] auto working_properties() const noexcept -> pugi::xml_node {
        return this->working_properties_;
    }

    [[nodiscard]] auto commit() noexcept -> bool {
        if (this->owner_ == pugi::xml_node{} ||
            this->scratch_ == pugi::xml_node{} ||
            this->working_properties_ == pugi::xml_node{}) {
            return false;
        }

        const auto anchor = this->original_properties_ != pugi::xml_node{}
                                ? this->original_properties_
                                : this->owner_.first_child();
        const auto inserted = this->owner_.insert_move_before(
            this->working_properties_, anchor);
        if (inserted == pugi::xml_node{}) {
            return false;
        }
        this->working_properties_ = {};

        if (!this->owner_.remove_child(this->scratch_)) {
            (void)this->owner_.remove_child(inserted);
            return false;
        }
        this->scratch_ = {};

        if (this->original_properties_ != pugi::xml_node{} &&
            !this->owner_.remove_child(this->original_properties_)) {
            (void)this->owner_.remove_child(inserted);
            return false;
        }
        this->original_properties_ = {};
        return true;
    }

  private:
    void cleanup() noexcept {
        if (this->owner_ != pugi::xml_node{} &&
            this->scratch_ != pugi::xml_node{}) {
            (void)this->owner_.remove_child(this->scratch_);
        }
        this->scratch_ = {};
        this->working_properties_ = {};
    }

    pugi::xml_node owner_{};
    pugi::xml_node scratch_{};
    pugi::xml_node original_properties_{};
    pugi::xml_node working_properties_{};
};

[[nodiscard]] auto
set_applied_style_value(pugi::xml_node properties, const char *style_node_name,
                        std::string_view style_id) -> bool {
    if (properties == pugi::xml_node{} || style_node_name == nullptr) {
        return false;
    }

    auto style_node = properties.child(style_node_name);
    if (style_node == pugi::xml_node{}) {
        style_node = checked_insert_style_element(
            properties, style_node_name, properties.first_child());
    } else {
        for (auto duplicate = style_node.next_sibling(style_node_name);
             duplicate != pugi::xml_node{};) {
            const auto next_duplicate =
                duplicate.next_sibling(style_node_name);
            if (!properties.remove_child(duplicate)) {
                return false;
            }
            duplicate = next_duplicate;
        }
        if (style_node != properties.first_child()) {
            style_node = properties.prepend_move(style_node);
        }
    }
    return style_node != pugi::xml_node{} &&
           featherdoc::detail::checked_set_xml_attribute_value(
               style_node, "w:val", style_id);
}

#include "document_styles_summary_helpers.inc"

#include "document_styles_ensure_helpers.inc"

#include "document_styles_metadata_helpers.inc"

#include "document_styles_reference_helpers.inc"

#include "document_styles_usage_helpers.inc"

#include "document_styles_mutation_helpers.inc"

#include "document_styles_refactor_helpers.inc"
} // namespace

namespace featherdoc {

std::error_code Document::ensure_styles_loaded() {
    if (this->styles_loaded) {
        return {};
    }

    this->styles.reset();
    this->has_styles_part = false;

    const auto load_default_styles = [&]() -> std::error_code {
        const auto initialization =
            featherdoc::detail::initialize_fixed_xml_document(
                this->styles, default_styles_xml);
        if (!initialization) {
            return featherdoc::detail::set_fixed_xml_initialization_failure(
                this->last_error_info, initialization,
                document_errc::styles_xml_parse_failed,
                this->extension_state().singleton_part_entries.styles);
        }

        this->styles_loaded = true;
        return {};
    };

    if (!this->has_source_archive) {
        return load_default_styles();
    }

    const auto relationships = featherdoc::detail::package_relationships_root(
        this->document_relationships);
    if (relationships == pugi::xml_node{} &&
        !this->ignored_singleton_relationship_type(styles_relationship_type)) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "word/_rels/document.xml.rels does not contain a "
                              "Relationships root",
                              std::string{document_relationships_xml_entry});
    }

    const auto styles_relationship =
        this->ignored_singleton_relationship_type(styles_relationship_type)
            ? pugi::xml_node{}
            : find_document_relationship_by_type(relationships,
                                                 styles_relationship_type);
    if (styles_relationship == pugi::xml_node{} &&
        this->ignored_singleton_relationship_type(styles_relationship_type)) {
        return load_default_styles();
    }

    if (styles_relationship != pugi::xml_node{}) {
        const auto target =
            std::string_view{styles_relationship.attribute("Target").value()};
        if (target.empty()) {
            return set_last_error(
                this->last_error_info, document_errc::styles_xml_read_failed,
                "styles relationship does not contain a Target attribute",
                std::string{document_relationships_xml_entry});
        }
        if (std::string_view{
                styles_relationship.attribute("TargetMode").value()} ==
            "External") {
            return set_last_error(
                this->last_error_info, document_errc::invalid_package_structure,
                "styles relationship must not use TargetMode=External",
                std::string{document_relationships_xml_entry});
        }
        const auto resolved_styles_target =
            featherdoc::detail::resolve_internal_package_relationship_target(
                document_xml_entry, target);
        if (!resolved_styles_target) {
            return set_last_error(
                this->last_error_info, document_errc::invalid_package_structure,
                std::string{featherdoc::detail::
                                package_relationship_target_error_message(
                                    resolved_styles_target.error)},
                std::string{document_relationships_xml_entry});
        }
        this->extension_state().singleton_part_entries.styles =
            resolved_styles_target.entry_name;
    }
    const auto &styles_entry_name =
        this->extension_state().singleton_part_entries.styles;

    const auto &source_archive_io_path =
        this->extension_state().source_archive_io_path;
    const auto source_archive_path =
        featherdoc::detail::path_to_utf8(source_archive_io_path);
    const auto native_source_archive_path =
        featherdoc::detail::path_to_native_utf8(source_archive_io_path);
    const auto source_archive =
        featherdoc::detail::open_read_zip_archive_with_limits(
            native_source_archive_path, this->opened_archive_limits);
    zip_t *source_zip = source_archive.archive;
    if (!source_zip) {
        return featherdoc::detail::set_reopened_source_archive_open_error(
            this->last_error_info, source_archive_path, source_archive,
            "loading styles metadata", styles_entry_name);
    }
    featherdoc::detail::read_zip_archive_guard source_zip_guard{source_zip};

    auto entry_catalog = featherdoc::detail::archive_entry_catalog{};
    if (const auto validation_error =
            featherdoc::detail::validate_reopened_source_archive_metadata(
                source_zip, this->opened_archive_limits, this->last_error_info,
                this->extension_state().opened_validation_mode,
                this->extension_state().source_archive_snapshot,
                source_archive_path, &entry_catalog)) {
        return validation_error;
    }

    if (styles_relationship == pugi::xml_node{} &&
        featherdoc::detail::find_archive_entry_record(
            entry_catalog, styles_entry_name) == nullptr) {
        if (const auto close_error = featherdoc::detail::close_read_archive(
                source_zip_guard, this->last_error_info,
                "failed to close source archive after checking for an "
                "orphaned styles part",
                styles_entry_name)) {
            return close_error;
        }
        return load_default_styles();
    }

    std::string styles_text;
    const auto read_status = read_zip_entry_text(
        source_zip, styles_entry_name, styles_text,
        &this->opened_archive_limits, &this->last_error_info, entry_catalog);

    if (read_status == zip_entry_read_status::limit_exceeded) {
        return this->last_error_info.code;
    }

    if (read_status != zip_entry_read_status::ok) {
        return set_last_error(
            this->last_error_info, document_errc::styles_xml_read_failed,
            "failed to read required styles part '" + styles_entry_name + "'",
            styles_entry_name);
    }

    if (const auto close_error = featherdoc::detail::close_read_archive(
            source_zip_guard, this->last_error_info,
            "failed to close source archive after loading styles",
            styles_entry_name)) {
        return close_error;
    }

    const auto parse_result =
        this->styles.load_buffer(styles_text.data(), styles_text.size());
    if (!parse_result) {
        return featherdoc::detail::set_xml_parse_failure(
            this->last_error_info, parse_result,
            document_errc::styles_xml_parse_failed, styles_entry_name);
    }

    const auto namespace_result =
        featherdoc::detail::canonicalize_wordprocessingml_part(
            this->styles,
            featherdoc::detail::wordprocessingml_part_kind::styles);
    if (!namespace_result) {
        this->styles.reset();
        return featherdoc::detail::set_wordprocessingml_namespace_last_error(
            this->last_error_info, namespace_result, styles_entry_name);
    }
    if (!namespace_result.root_matches) {
        this->styles.reset();
        return featherdoc::detail::
            set_wordprocessingml_root_mismatch_last_error(
                this->last_error_info, namespace_result,
                featherdoc::detail::wordprocessingml_part_kind::styles,
                styles_entry_name);
    }

    this->styles_loaded = true;
    this->has_styles_part = true;
    return {};
}

std::error_code Document::prepare_styles_part_attachment(
    featherdoc::detail::singleton_part_attachment_work &attachment_work) {
    if (this->ignored_singleton_relationship_type(styles_relationship_type)) {
        return set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach styles while its source relationship is invalid",
            std::string{document_relationships_xml_entry});
    }
    if (!featherdoc::detail::package_relationships_document_allows_mutation(
            this->document_relationships,
            this->has_document_relationships_part)) {
        return set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cannot attach styles while document relationships are invalid",
            std::string{document_relationships_xml_entry});
    }
    if (const auto error = this->ensure_styles_loaded()) {
        return error;
    }

    if (const auto error = this->ensure_content_types_loaded()) {
        return error;
    }

    auto relationship_target = std::string{};
    try {
        relationship_target = make_document_relationship_target(
            this->extension_state().singleton_part_entries.styles);
    } catch (const std::bad_alloc &) {
        return featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_relationships_xml_entry);
    }
    auto override_part_name = std::string{};
    try {
        override_part_name = make_override_part_name(
            this->extension_state().singleton_part_entries.styles);
    } catch (const std::bad_alloc &) {
        return featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, content_types_xml_entry);
    }

    auto attachment_options =
        featherdoc::detail::singleton_part_attachment_options{};
    attachment_options.relationship_type = styles_relationship_type;
    attachment_options.relationship_target = relationship_target;
    attachment_options.override_part_name = override_part_name;
    attachment_options.content_type = styles_content_type;
    attachment_options.relationships_entry_name =
        document_relationships_xml_entry;
    attachment_options.content_types_entry_name = content_types_xml_entry;
    if (const auto error =
            featherdoc::detail::prepare_checked_singleton_part_attachment(
                this->document_relationships, this->content_types,
                attachment_options, this->last_error_info, attachment_work)) {
        return error;
    }

    return {};
}

void Document::publish_styles_part_attachment(
    featherdoc::detail::singleton_part_attachment_work &&attachment_work)
    noexcept {
    const auto had_styles_part = this->has_styles_part;
    featherdoc::detail::publish_checked_singleton_part_attachment(
        std::move(attachment_work), this->document_relationships,
        this->content_types, this->has_document_relationships_part,
        this->document_relationships_dirty, this->content_types_dirty);
    this->has_styles_part = true;
    if (!had_styles_part) {
        this->styles_dirty = true;
    }
}

std::error_code Document::ensure_styles_part_attached() {
    auto attachment_work = featherdoc::detail::singleton_part_attachment_work{};
    if (const auto error =
            this->prepare_styles_part_attachment(attachment_work)) {
        return error;
    }
    this->publish_styles_part_attachment(std::move(attachment_work));
    return {};
}

bool Document::owns_run_handle(
    const Run &target_run,
    std::string_view &owning_part_entry_name) const noexcept {
    owning_part_entry_name = {};
    const auto run_node = static_cast<pugi::xml_node>(target_run.current);
    const auto paragraph_node = static_cast<pugi::xml_node>(target_run.parent);
    if (run_node == pugi::xml_node{} || paragraph_node == pugi::xml_node{} ||
        std::string_view{run_node.name()} != "w:r" ||
        std::string_view{paragraph_node.name()} != "w:p" ||
        run_node.parent() != paragraph_node) {
        return false;
    }

    auto paragraph_handle =
        Paragraph{target_run.parent.parent(), paragraph_node};
    return this->owns_paragraph_handle(paragraph_handle,
                                       owning_part_entry_name);
}

#include "document_styles_catalog_methods.inc"

#include "document_styles_usage_methods.inc"

#include "document_styles_refactor_methods.inc"

#include "document_styles_ensure_methods.inc"

#include "document_styles_table_style_methods.inc"

#include "document_styles_defaults_methods.inc"

#include "document_styles_property_read_methods.inc"

#include "document_styles_property_write_methods.inc"

#include "document_styles_property_clear_methods.inc"

#include "document_styles_applied_style_methods.inc"

#include "document_styles_inspection_methods.inc"

} // namespace featherdoc
