#include "xml_helpers.hpp"
#include "xml_document_clone_helpers.hpp"
#include "xml_namespace_helpers.hpp"

#include <cctype>
#include <cstring>
#include <new>
#include <string>
#include <unordered_set>

namespace featherdoc::detail {
namespace {

auto should_preserve_xml_space(const char *text) -> bool {
    if (text == nullptr || *text == '\0') {
        return false;
    }

    const auto text_length = std::strlen(text);
    return std::isspace(static_cast<unsigned char>(text[0])) != 0 ||
           std::isspace(static_cast<unsigned char>(text[text_length - 1])) != 0;
}

auto node_has_attributes(pugi::xml_node node) -> bool {
    return node.first_attribute() != pugi::xml_attribute{};
}

auto append_plain_text_node(pugi::xml_node run, std::string_view text) -> bool {
    auto text_node = checked_append_xml_element(run, "w:t");
    if (text_node == pugi::xml_node{}) {
        return false;
    }

    const std::string text_buffer{text};
    return update_xml_space_attribute(text_node, text_buffer.c_str()) &&
           text_node.text().set(text_buffer.c_str());
}

auto append_plain_text_run_content(pugi::xml_node run, std::string_view text)
    -> bool {
    if (run == pugi::xml_node{}) {
        return false;
    }

    if (text.empty()) {
        return append_plain_text_node(run, {});
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
            !append_plain_text_node(
                run, text.substr(segment_start, offset - segment_start))) {
            return false;
        }

        if (checked_append_xml_element(run, "w:br") == pugi::xml_node{}) {
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
        return append_plain_text_node(run, text.substr(segment_start));
    }

    return true;
}

auto contains_only_xml_whitespace(std::string_view text) -> bool {
    for (const auto character : text) {
        if (character != ' ' && character != '\t' && character != '\r' &&
            character != '\n') {
            return false;
        }
    }
    return true;
}

constexpr auto package_relationships_namespace = std::string_view{
    "http://schemas.openxmlformats.org/package/2006/relationships"};
constexpr auto package_content_types_namespace = std::string_view{
    "http://schemas.openxmlformats.org/package/2006/content-types"};

auto node_has_unique_attribute_names(pugi::xml_node node) -> bool {
    std::unordered_set<std::string_view> attribute_names;
    for (auto attribute = node.first_attribute();
         attribute != pugi::xml_attribute{};
         attribute = attribute.next_attribute()) {
        if (!attribute_names.insert(attribute.name()).second) {
            return false;
        }
    }
    return true;
}

auto node_contains_forbidden_content(pugi::xml_node node) -> bool {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (child.type() == pugi::node_element ||
            child.type() == pugi::node_pcdata ||
            child.type() == pugi::node_cdata) {
            return true;
        }
    }
    return false;
}

auto declaration_has_only_allowed_attributes(pugi::xml_node declaration,
                                             std::string_view first_required,
                                             std::string_view second_required)
    -> bool {
    if (!node_has_unique_attribute_names(declaration)) {
        return false;
    }

    std::size_t first_count = 0U;
    std::size_t second_count = 0U;
    for (auto attribute = declaration.first_attribute();
         attribute != pugi::xml_attribute{};
         attribute = attribute.next_attribute()) {
        const auto name = std::string_view{attribute.name()};
        if (name == first_required) {
            ++first_count;
        } else if (name == second_required) {
            ++second_count;
        } else if (!xml_attribute_is_namespace_declaration(attribute)) {
            return false;
        }
    }

    // Missing required attributes are diagnosed by the semantic validator so
    // tolerant mode keeps its established fail-closed classification.
    return first_count <= 1U && second_count <= 1U;
}

auto relationships_root_has_only_namespace_attributes(pugi::xml_node root)
    -> bool {
    if (!node_has_unique_attribute_names(root)) {
        return false;
    }
    for (auto attribute = root.first_attribute();
         attribute != pugi::xml_attribute{};
         attribute = attribute.next_attribute()) {
        if (!xml_attribute_is_namespace_declaration(attribute)) {
            return false;
        }
    }
    return true;
}

auto relationship_has_only_allowed_attributes(pugi::xml_node relationship)
    -> bool {
    if (!node_has_unique_attribute_names(relationship)) {
        return false;
    }
    for (auto attribute = relationship.first_attribute();
         attribute != pugi::xml_attribute{};
         attribute = attribute.next_attribute()) {
        const auto name = std::string_view{attribute.name()};
        if (name != "Id" && name != "Type" && name != "Target" &&
            name != "TargetMode" &&
            !xml_attribute_is_namespace_declaration(attribute)) {
            return false;
        }
    }
    return true;
}

auto append_package_content_type_declaration(pugi::xml_node content_types_root,
                                             std::string_view local_name)
    -> pugi::xml_node {
    if (!xml_element_has_expanded_name(content_types_root, "Types",
                                       package_content_types_namespace)) {
        return {};
    }

    const auto root_name =
        parse_xml_qualified_name(std::string_view{content_types_root.name()});
    if (!root_name.valid) {
        return {};
    }
    if (root_name.prefix.empty()) {
        const auto terminated_name = std::string{local_name};
        return content_types_root.append_child(terminated_name.c_str());
    }

    auto qualified_name = std::string{root_name.prefix};
    qualified_name.push_back(':');
    qualified_name.append(local_name);
    return content_types_root.append_child(qualified_name.c_str());
}

auto first_named_package_relationship(pugi::xml_node node) -> pugi::xml_node {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (xml_element_has_expanded_name(child, "Relationship",
                                          package_relationships_namespace)) {
            return child;
        }
    }
    return {};
}

auto next_named_package_relationship(pugi::xml_node node) -> pugi::xml_node {
    for (auto sibling = node.next_sibling(); sibling != pugi::xml_node{};
         sibling = sibling.next_sibling()) {
        if (xml_element_has_expanded_name(sibling, "Relationship",
                                          package_relationships_namespace)) {
            return sibling;
        }
    }
    return {};
}

} // namespace

pugi::xml_node
package_content_types_root(const pugi::xml_document &content_types_document) {
    const auto root = content_types_document.document_element();
    return xml_element_has_expanded_name(root, "Types",
                                         package_content_types_namespace)
               ? root
               : pugi::xml_node{};
}

pugi::xml_node
first_package_content_type_default(pugi::xml_node content_types_root) {
    for (auto child = content_types_root.first_child();
         child != pugi::xml_node{}; child = child.next_sibling()) {
        if (xml_element_has_expanded_name(child, "Default",
                                          package_content_types_namespace)) {
            return child;
        }
    }
    return {};
}

pugi::xml_node next_package_content_type_default(pugi::xml_node default_node) {
    for (auto sibling = default_node.next_sibling();
         sibling != pugi::xml_node{}; sibling = sibling.next_sibling()) {
        if (xml_element_has_expanded_name(sibling, "Default",
                                          package_content_types_namespace)) {
            return sibling;
        }
    }
    return {};
}

pugi::xml_node
first_package_content_type_override(pugi::xml_node content_types_root) {
    for (auto child = content_types_root.first_child();
         child != pugi::xml_node{}; child = child.next_sibling()) {
        if (xml_element_has_expanded_name(child, "Override",
                                          package_content_types_namespace)) {
            return child;
        }
    }
    return {};
}

pugi::xml_node
next_package_content_type_override(pugi::xml_node override_node) {
    for (auto sibling = override_node.next_sibling();
         sibling != pugi::xml_node{}; sibling = sibling.next_sibling()) {
        if (xml_element_has_expanded_name(sibling, "Override",
                                          package_content_types_namespace)) {
            return sibling;
        }
    }
    return {};
}

pugi::xml_node
append_package_content_type_default(pugi::xml_node content_types_root) {
    return append_package_content_type_declaration(content_types_root,
                                                   "Default");
}

pugi::xml_node
append_package_content_type_override(pugi::xml_node content_types_root) {
    return append_package_content_type_declaration(content_types_root,
                                                   "Override");
}

package_content_types_document_inspection
inspect_package_content_types_document_structure(
    const pugi::xml_document &content_types_document) {
    const auto namespace_validation =
        validate_xml_namespace_well_formedness(content_types_document);
    if (!namespace_validation.valid) {
        return {package_content_types_document_state::invalid,
                "[Content_Types].xml contains invalid namespace markup"};
    }
    const auto root = content_types_document.document_element();
    if (root == pugi::xml_node{}) {
        return {package_content_types_document_state::missing,
                "[Content_Types].xml does not contain a Types root"};
    }
    if (!xml_element_has_expanded_name(root, "Types",
                                       package_content_types_namespace)) {
        return {package_content_types_document_state::invalid,
                "[Content_Types].xml Types root has an invalid expanded "
                "QName"};
    }
    if (!node_has_unique_attribute_names(root)) {
        return {package_content_types_document_state::invalid,
                "[Content_Types].xml Types root contains duplicate "
                "attributes"};
    }

    for (auto attribute = root.first_attribute();
         attribute != pugi::xml_attribute{};
         attribute = attribute.next_attribute()) {
        if (!xml_attribute_is_namespace_declaration(attribute)) {
            return {package_content_types_document_state::invalid,
                    "[Content_Types].xml Types root contains an invalid "
                    "attribute"};
        }
    }

    for (auto child = root.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (child.type() != pugi::node_element) {
            if ((child.type() == pugi::node_pcdata ||
                 child.type() == pugi::node_cdata) &&
                !contains_only_xml_whitespace(child.value())) {
                return {
                    package_content_types_document_state::invalid,
                    "[Content_Types].xml Types root contains non-whitespace "
                    "text"};
            }
            continue;
        }

        const bool is_default = xml_element_has_expanded_name(
            child, "Default", package_content_types_namespace);
        const bool is_override = xml_element_has_expanded_name(
            child, "Override", package_content_types_namespace);
        if (!is_default && !is_override) {
            return {package_content_types_document_state::invalid,
                    "[Content_Types].xml Types root contains an unexpected "
                    "element or namespace-shadowed declaration"};
        }
        if (node_contains_forbidden_content(child)) {
            return {package_content_types_document_state::invalid,
                    is_default
                        ? "[Content_Types].xml Default declaration contains "
                          "nested content"
                        : "[Content_Types].xml Override declaration contains "
                          "nested content"};
        }
        if (!declaration_has_only_allowed_attributes(
                child, is_default ? "Extension" : "PartName", "ContentType")) {
            return {package_content_types_document_state::invalid,
                    is_default
                        ? "[Content_Types].xml Default declaration contains "
                          "invalid attributes"
                        : "[Content_Types].xml Override declaration contains "
                          "invalid attributes"};
        }
    }

    for (auto document_child = content_types_document.first_child();
         document_child != pugi::xml_node{};
         document_child = document_child.next_sibling()) {
        if (document_child == root ||
            document_child.type() == pugi::node_declaration ||
            document_child.type() == pugi::node_comment ||
            document_child.type() == pugi::node_pi) {
            continue;
        }
        if (document_child.type() == pugi::node_element ||
            document_child.type() == pugi::node_doctype ||
            ((document_child.type() == pugi::node_pcdata ||
              document_child.type() == pugi::node_cdata) &&
             !contains_only_xml_whitespace(document_child.value()))) {
            return {package_content_types_document_state::invalid,
                    "[Content_Types].xml contains unexpected content outside "
                    "the Types root"};
        }
    }

    return {package_content_types_document_state::valid, {}};
}

bool package_content_types_document_allows_mutation(
    const pugi::xml_document &content_types_document) {
    return inspect_package_content_types_document_structure(
               content_types_document)
               .state == package_content_types_document_state::valid;
}

pugi::xml_node
package_relationships_root(const pugi::xml_document &relationships_document) {
    const auto root = relationships_document.document_element();
    return xml_element_has_expanded_name(root, "Relationships",
                                         package_relationships_namespace)
               ? root
               : pugi::xml_node{};
}

pugi::xml_node first_package_relationship(pugi::xml_node relationships_root) {
    return first_named_package_relationship(relationships_root);
}

pugi::xml_node next_package_relationship(pugi::xml_node relationship) {
    return next_named_package_relationship(relationship);
}

pugi::xml_node append_package_relationship(pugi::xml_node relationships_root) {
    if (!xml_element_has_expanded_name(relationships_root, "Relationships",
                                       package_relationships_namespace)) {
        return {};
    }

    const auto root_name =
        parse_xml_qualified_name(std::string_view{relationships_root.name()});
    if (!root_name.valid) {
        return {};
    }
    if (root_name.prefix.empty()) {
        return relationships_root.append_child("Relationship");
    }

    std::string relationship_name{root_name.prefix};
    relationship_name.append(":Relationship");
    return relationships_root.append_child(relationship_name.c_str());
}

static auto inspect_package_relationships_document_impl(
    const pugi::xml_document &relationships_document,
    bool validate_relationship_metadata, bool reject_duplicate_relationship_ids)
    -> package_relationships_document_state {
    if (!validate_xml_namespace_well_formedness(relationships_document).valid) {
        return package_relationships_document_state::invalid;
    }
    const auto root = relationships_document.document_element();
    if (root == pugi::xml_node{}) {
        return package_relationships_document_state::missing;
    }
    if (!xml_element_has_expanded_name(root, "Relationships",
                                       package_relationships_namespace)) {
        return package_relationships_document_state::invalid;
    }
    if (!relationships_root_has_only_namespace_attributes(root)) {
        return package_relationships_document_state::invalid;
    }

    std::unordered_set<std::string> relationship_ids;
    for (auto child = root.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (child.type() != pugi::node_element) {
            if ((child.type() == pugi::node_pcdata ||
                 child.type() == pugi::node_cdata) &&
                !contains_only_xml_whitespace(child.value())) {
                return package_relationships_document_state::invalid;
            }
            continue;
        }

        if (!xml_element_has_expanded_name(child, "Relationship",
                                           package_relationships_namespace)) {
            return package_relationships_document_state::invalid;
        }
        if (!relationship_has_only_allowed_attributes(child)) {
            return package_relationships_document_state::invalid;
        }

        if (validate_relationship_metadata) {
            const auto id = std::string_view{child.attribute("Id").value()};
            const auto type = std::string_view{child.attribute("Type").value()};
            const auto target =
                std::string_view{child.attribute("Target").value()};
            if (id.empty() || type.empty() || target.empty()) {
                return package_relationships_document_state::invalid;
            }
            if (reject_duplicate_relationship_ids &&
                !relationship_ids.emplace(id).second) {
                return package_relationships_document_state::invalid;
            }

            const auto target_mode_attribute = child.attribute("TargetMode");
            if (target_mode_attribute != pugi::xml_attribute{}) {
                const auto target_mode =
                    std::string_view{target_mode_attribute.value()};
                if (target_mode != "Internal" && target_mode != "External") {
                    return package_relationships_document_state::invalid;
                }
            }
        }

        for (auto relationship_child = child.first_child();
             relationship_child != pugi::xml_node{};
             relationship_child = relationship_child.next_sibling()) {
            if (relationship_child.type() == pugi::node_element) {
                return package_relationships_document_state::invalid;
            }
        }
    }

    for (auto sibling = root.next_sibling(); sibling != pugi::xml_node{};
         sibling = sibling.next_sibling()) {
        if (sibling.type() == pugi::node_element) {
            return package_relationships_document_state::invalid;
        }
    }
    return package_relationships_document_state::valid;
}

package_relationships_document_state inspect_package_relationships_document(
    const pugi::xml_document &relationships_document,
    bool reject_duplicate_relationship_ids) {
    return inspect_package_relationships_document_impl(
        relationships_document, true, reject_duplicate_relationship_ids);
}

package_relationships_document_state
inspect_package_relationships_document_structure(
    const pugi::xml_document &relationships_document) {
    return inspect_package_relationships_document_impl(relationships_document,
                                                       false, false);
}

bool package_relationships_document_allows_mutation(
    const pugi::xml_document &relationships_document,
    bool has_relationships_part) {
    const auto state =
        inspect_package_relationships_document(relationships_document);
    return state == package_relationships_document_state::valid ||
           (state == package_relationships_document_state::missing &&
            !has_relationships_part);
}

bool package_relationships_document_is_invalid(
    const pugi::xml_document &relationships_document) {
    return inspect_package_relationships_document(relationships_document) ==
           package_relationships_document_state::invalid;
}

bool update_xml_space_attribute(pugi::xml_node text_node, const char *text) {
    if (text_node == pugi::xml_node{}) {
        return false;
    }

    if (should_preserve_xml_space(text)) {
        auto xml_space = text_node.attribute("xml:space");
        if (xml_space == pugi::xml_attribute{}) {
            return checked_append_xml_attribute(text_node, "xml:space",
                                                "preserve");
        }
        return xml_space.set_value("preserve");
    }

    (void)text_node.remove_attribute("xml:space");
    return true;
}

void append_plain_text_from_xml(std::string &text, pugi::xml_node node) {
    if (node == pugi::xml_node{}) {
        return;
    }

    for (auto current = node.first_child(); current != pugi::xml_node{};) {
        const auto name = std::string_view{current.name()};
        auto descend_into_children = true;
        if (name == "w:t" || name == "w:delText") {
            text += current.text().get();
            descend_into_children = false;
        } else if (name == "w:tab") {
            text.push_back('\t');
            descend_into_children = false;
        } else if (name == "w:br" || name == "w:cr") {
            text.push_back('\n');
            descend_into_children = false;
        }

        current = next_xml_node_preorder(node, current, descend_into_children);
    }
}

std::string collect_plain_text_from_xml(pugi::xml_node node) {
    std::string text;
    append_plain_text_from_xml(text, node);
    return text;
}

pugi::xml_node next_xml_node_preorder(pugi::xml_node root,
                                      pugi::xml_node current,
                                      bool descend_into_children) {
    if (root == pugi::xml_node{} || current == pugi::xml_node{}) {
        return {};
    }

    if (descend_into_children) {
        if (const auto child = current.first_child();
            child != pugi::xml_node{}) {
            return child;
        }
    }

    while (current != root) {
        if (const auto sibling = current.next_sibling();
            sibling != pugi::xml_node{}) {
            return sibling;
        }
        current = current.parent();
    }
    return {};
}

pugi::xml_node next_named_sibling(pugi::xml_node node,
                                  std::string_view node_name) {
    for (auto sibling = node.next_sibling(); sibling != pugi::xml_node{};
         sibling = sibling.next_sibling()) {
        if (std::string_view{sibling.name()} == node_name) {
            return sibling;
        }
    }

    return {};
}

pugi::xml_node previous_named_sibling(pugi::xml_node node,
                                      std::string_view node_name) {
    for (auto sibling = node.previous_sibling(); sibling != pugi::xml_node{};
         sibling = sibling.previous_sibling()) {
        if (std::string_view{sibling.name()} == node_name) {
            return sibling;
        }
    }

    return {};
}

pugi::xml_node ensure_run_properties_node(pugi::xml_node run) {
    if (run == pugi::xml_node{}) {
        return {};
    }

    auto run_properties = run.child("w:rPr");
    if (run_properties != pugi::xml_node{}) {
        return run_properties;
    }

    if (const auto first_child = run.first_child();
        first_child != pugi::xml_node{}) {
        return run.insert_child_before("w:rPr", first_child);
    }

    return run.append_child("w:rPr");
}

void remove_empty_run_properties(pugi::xml_node run) {
    if (run == pugi::xml_node{}) {
        return;
    }

    auto run_properties = run.child("w:rPr");
    if (run_properties == pugi::xml_node{}) {
        return;
    }

    if (run_properties.first_child() == pugi::xml_node{} &&
        !node_has_attributes(run_properties)) {
        run.remove_child(run_properties);
    }
}

pugi::xml_node insert_paragraph_node(pugi::xml_node parent,
                                     pugi::xml_node insert_before) {
    if (parent == pugi::xml_node{}) {
        return {};
    }

    if (insert_before != pugi::xml_node{}) {
        if (insert_before.parent() != parent) {
            return {};
        }
        return checked_insert_xml_element_before(parent, "w:p",
                                                 insert_before);
    }

    if (std::string_view{parent.name()} == "w:body") {
        if (const auto section_properties = parent.child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            return checked_insert_xml_element_before(parent, "w:p",
                                                     section_properties);
        }
    }

    return checked_append_xml_element(parent, "w:p");
}

pugi::xml_node append_paragraph_node(pugi::xml_node parent) {
    return insert_paragraph_node(parent, {});
}

bool set_plain_text_run_content(pugi::xml_node run, std::string_view text) {
    if (run == pugi::xml_node{}) {
        return false;
    }

    auto parent = run.parent();
    if (parent == pugi::xml_node{}) {
        return false;
    }

    auto prepared_run =
        checked_insert_xml_element_before(parent, "w:r", run);
    if (prepared_run == pugi::xml_node{}) {
        return false;
    }

    try {
        if (!append_plain_text_run_content(prepared_run, text)) {
            (void)parent.remove_child(prepared_run);
            return false;
        }
    } catch (const std::bad_alloc &) {
        (void)parent.remove_child(prepared_run);
        return false;
    }

    for (auto child = run.first_child(); child != pugi::xml_node{};) {
        const auto next_child = child.next_sibling();
        if (std::string_view{child.name()} != "w:rPr") {
            (void)run.remove_child(child);
        }
        child = next_child;
    }

    while (const auto child = prepared_run.first_child()) {
        // FeatherDoc vendors non-compact pugixml; same-document node moves do
        // not allocate and therefore form the publish phase of this update.
        if (run.append_move(child) == pugi::xml_node{}) {
            (void)parent.remove_child(prepared_run);
            return false;
        }
    }
    (void)parent.remove_child(prepared_run);
    return true;
}

bool append_plain_text_run(pugi::xml_node parent, std::string_view text) {
    auto run = checked_append_xml_element(parent, "w:r");
    if (run == pugi::xml_node{}) {
        return false;
    }

    if (!set_plain_text_run_content(run, text)) {
        (void)parent.remove_child(run);
        return false;
    }
    return true;
}

pugi::xml_node insert_formatted_text_run(
    pugi::xml_node parent, pugi::xml_node insert_before, const char *text,
    featherdoc::formatting_flag formatting) {
    if (parent == pugi::xml_node{} || text == nullptr ||
        (insert_before != pugi::xml_node{} &&
         insert_before.parent() != parent)) {
        return {};
    }

    auto run = insert_before == pugi::xml_node{}
                   ? checked_append_xml_element(parent, "w:r")
                   : checked_insert_xml_element_before(parent, "w:r",
                                                       insert_before);
    if (run == pugi::xml_node{}) {
        return {};
    }

    const auto rollback = [&]() -> pugi::xml_node {
        (void)parent.remove_child(run);
        return {};
    };
    const auto append_property = [&](const char *name) -> pugi::xml_node {
        return checked_append_xml_element(run.child("w:rPr"), name);
    };
    const auto append_property_with_value =
        [&](const char *name, const char *value) -> bool {
        const auto property = append_property(name);
        return property != pugi::xml_node{} &&
               checked_append_xml_attribute(property, "w:val", value);
    };

    try {
        const auto run_properties = checked_append_xml_element(run, "w:rPr");
        if (run_properties == pugi::xml_node{}) {
            return rollback();
        }

        if (featherdoc::has_flag(formatting,
                                 featherdoc::formatting_flag::bold) &&
            append_property("w:b") == pugi::xml_node{}) {
            return rollback();
        }
        if (featherdoc::has_flag(formatting,
                                 featherdoc::formatting_flag::italic) &&
            append_property("w:i") == pugi::xml_node{}) {
            return rollback();
        }
        if (featherdoc::has_flag(formatting,
                                 featherdoc::formatting_flag::underline) &&
            !append_property_with_value("w:u", "single")) {
            return rollback();
        }
        if (featherdoc::has_flag(
                formatting, featherdoc::formatting_flag::strikethrough) &&
            !append_property_with_value("w:strike", "true")) {
            return rollback();
        }
        if (featherdoc::has_flag(formatting,
                                 featherdoc::formatting_flag::superscript)) {
            if (!append_property_with_value("w:vertAlign", "superscript")) {
                return rollback();
            }
        } else if (featherdoc::has_flag(
                       formatting, featherdoc::formatting_flag::subscript) &&
                   !append_property_with_value("w:vertAlign", "subscript")) {
            return rollback();
        }
        if (featherdoc::has_flag(formatting,
                                 featherdoc::formatting_flag::smallcaps) &&
            !append_property_with_value("w:smallCaps", "true")) {
            return rollback();
        }
        if (featherdoc::has_flag(formatting,
                                 featherdoc::formatting_flag::shadow) &&
            !append_property_with_value("w:shadow", "true")) {
            return rollback();
        }
        if (!append_plain_text_run_content(run, text)) {
            return rollback();
        }
    } catch (const std::bad_alloc &) {
        return rollback();
    }

    return run;
}

bool insert_plain_text_paragraph(pugi::xml_node parent,
                                 pugi::xml_node insert_before,
                                 std::string_view text) {
    auto paragraph = insert_paragraph_node(parent, insert_before);
    if (paragraph == pugi::xml_node{}) {
        return false;
    }

    if (text.empty()) {
        return true;
    }

    if (!append_plain_text_run(paragraph, text)) {
        (void)parent.remove_child(paragraph);
        return false;
    }
    return true;
}

bool append_plain_text_paragraph(pugi::xml_node parent, std::string_view text) {
    return insert_plain_text_paragraph(parent, {}, text);
}

pugi::xml_node insert_table_node(pugi::xml_node parent,
                                 pugi::xml_node insert_before) {
    if (parent == pugi::xml_node{}) {
        return {};
    }

    if (insert_before != pugi::xml_node{}) {
        if (insert_before.parent() != parent) {
            return {};
        }
        return checked_insert_xml_element_before(parent, "w:tbl",
                                                 insert_before);
    }

    if (std::string_view{parent.name()} == "w:body") {
        if (const auto section_properties = parent.child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            return checked_insert_xml_element_before(parent, "w:tbl",
                                                     section_properties);
        }
    }

    return checked_append_xml_element(parent, "w:tbl");
}

pugi::xml_node append_table_node(pugi::xml_node parent) {
    return insert_table_node(parent, {});
}

std::size_t count_remaining_block_children(pugi::xml_node parent,
                                           pugi::xml_node skipped_child) {
    std::size_t count = 0U;
    const auto parent_name = std::string_view{parent.name()};

    for (auto child = parent.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (child == skipped_child || child.type() != pugi::node_element) {
            continue;
        }

        const auto child_name = std::string_view{child.name()};
        if ((parent_name == "w:body" && child_name == "w:sectPr") ||
            (parent_name == "w:tc" && child_name == "w:tcPr")) {
            continue;
        }

        ++count;
    }

    return count;
}

bool parent_requires_nonempty_block_content(pugi::xml_node parent) {
    const auto parent_name = std::string_view{parent.name()};
    return parent_name == "w:body" || parent_name == "w:hdr" ||
           parent_name == "w:ftr" || parent_name == "w:tc";
}

} // namespace featherdoc::detail
