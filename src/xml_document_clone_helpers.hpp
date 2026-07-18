#pragma once

#include <featherdoc/document_core.hpp>

#include <string_view>
#include <system_error>
#include <utility>

#include <pugixml.hpp>

namespace featherdoc::detail {

enum class xml_document_clone_status {
    success,
    allocation_failure,
};

// pugixml's append_attribute(name) overload can return a non-null attribute
// even when allocating/copying the name failed. Create an empty attribute and
// check both string assignments explicitly. This always appends, so checked
// clones preserve malformed duplicate attributes for later validation.
[[nodiscard]] inline auto checked_append_xml_attribute(pugi::xml_node node,
                                                       const char *name,
                                                       std::string_view value)
    -> bool {
    if (node == pugi::xml_node{} || name == nullptr) {
        return false;
    }

    auto attribute = node.append_attribute("");
    if (attribute == pugi::xml_attribute{}) {
        return false;
    }
    if (!attribute.set_name(name) ||
        !attribute.set_value(value.data(), value.size())) {
        node.remove_attribute(attribute);
        return false;
    }
    return true;
}

// Set an existing attribute or append it when absent. Callers that need
// atomicity must invoke this on an isolated work document.
[[nodiscard]] inline auto
checked_set_xml_attribute_value(pugi::xml_node node, const char *name,
                                std::string_view value) -> bool {
    if (node == pugi::xml_node{} || name == nullptr) {
        return false;
    }

    auto attribute = node.attribute(name);
    if (attribute == pugi::xml_attribute{}) {
        return checked_append_xml_attribute(node, name, value);
    }
    if (std::string_view{attribute.value()} == value) {
        return true;
    }
    return attribute.set_value(value.data(), value.size());
}

// append_child(name) has the same non-null-but-name-allocation-failed caveat as
// append_attribute(name). Returning a null handle lets callers treat the
// operation as a checked allocation failure.
[[nodiscard]] inline auto checked_append_xml_element(pugi::xml_node parent,
                                                     const char *name)
    -> pugi::xml_node {
    if (parent == pugi::xml_node{} || name == nullptr) {
        return {};
    }
    auto child = parent.append_child(pugi::node_element);
    if (child == pugi::xml_node{}) {
        return {};
    }
    if (!child.set_name(name)) {
        parent.remove_child(child);
        return {};
    }
    return child;
}

// insert_child_before(name, anchor) can likewise return a non-null element
// whose name allocation failed. Insert an unnamed element first, then verify
// the name assignment and remove the shell on failure.
[[nodiscard]] inline auto
checked_insert_xml_element_before(pugi::xml_node parent, const char *name,
                                  pugi::xml_node anchor) -> pugi::xml_node {
    if (parent == pugi::xml_node{} || name == nullptr ||
        anchor == pugi::xml_node{} || anchor.parent() != parent) {
        return {};
    }
    auto child = parent.insert_child_before(pugi::node_element, anchor);
    if (child == pugi::xml_node{}) {
        return {};
    }
    if (!child.set_name(name)) {
        parent.remove_child(child);
        return {};
    }
    return child;
}

namespace xml_document_clone_detail {

inline auto copy_node_contents(pugi::xml_node source,
                               pugi::xml_node destination) -> bool {
    if (source.type() == pugi::node_null ||
        source.type() == pugi::node_document) {
        return false;
    }

    switch (source.type()) {
    case pugi::node_element:
    case pugi::node_pi:
    case pugi::node_declaration:
        if (!destination.set_name(source.name())) {
            return false;
        }
        break;
    default:
        break;
    }

    switch (source.type()) {
    case pugi::node_pcdata:
    case pugi::node_cdata:
    case pugi::node_comment:
    case pugi::node_pi:
    case pugi::node_doctype:
        if (!destination.set_value(source.value())) {
            return false;
        }
        break;
    default:
        break;
    }

    for (auto source_attribute = source.first_attribute();
         source_attribute != pugi::xml_attribute{};
         source_attribute = source_attribute.next_attribute()) {
        if (!checked_append_xml_attribute(destination, source_attribute.name(),
                                          source_attribute.value())) {
            return false;
        }
    }

    return true;
}

} // namespace xml_document_clone_detail

// pugixml's reset(proto) API is intentionally void and its internal copy path
// can silently omit nodes, attributes, names, or values when its allocator
// fails. Build the clone in isolation and publish it only after every mutable
// operation has reported success.
[[nodiscard]] inline auto
checked_clone_xml_document(const pugi::xml_document &source,
                           pugi::xml_document &destination)
    -> xml_document_clone_status {
    try {
        pugi::xml_document clone;
        const pugi::xml_node source_root = source;
        const pugi::xml_node destination_root = clone;
        auto source_node = source_root.first_child();
        auto destination_parent = destination_root;

        while (source_node != pugi::xml_node{}) {
            auto destination_node =
                destination_parent.append_child(source_node.type());
            if (destination_node == pugi::xml_node{} ||
                !xml_document_clone_detail::copy_node_contents(
                    source_node, destination_node)) {
                return xml_document_clone_status::allocation_failure;
            }

            if (const auto first_child = source_node.first_child();
                first_child != pugi::xml_node{}) {
                source_node = first_child;
                destination_parent = destination_node;
                continue;
            }

            while (source_node != source_root &&
                   source_node.next_sibling() == pugi::xml_node{}) {
                source_node = source_node.parent();
                if (destination_parent != destination_root) {
                    destination_parent = destination_parent.parent();
                }
            }
            if (source_node == source_root) {
                break;
            }
            source_node = source_node.next_sibling();
        }

        destination = std::move(clone);
        return xml_document_clone_status::success;
    } catch (...) {
        return xml_document_clone_status::allocation_failure;
    }
}

// Append one complete subtree while checking every pugixml operation that can
// report allocation failure. The destination may contain a partial subtree on
// failure, so callers that require atomicity must append into a work document
// and publish that document only after this function succeeds.
[[nodiscard]] inline auto
checked_append_copy_xml_node(pugi::xml_node source,
                             pugi::xml_node destination_parent)
    -> xml_document_clone_status {
    if (source == pugi::xml_node{} || source.type() == pugi::node_document ||
        destination_parent == pugi::xml_node{}) {
        return xml_document_clone_status::allocation_failure;
    }

    try {
        const auto source_root = source;
        auto destination = destination_parent;
        while (source != pugi::xml_node{}) {
            auto destination_node = destination.append_child(source.type());
            if (destination_node == pugi::xml_node{} ||
                !xml_document_clone_detail::copy_node_contents(
                    source, destination_node)) {
                return xml_document_clone_status::allocation_failure;
            }

            if (const auto first_child = source.first_child();
                first_child != pugi::xml_node{}) {
                source = first_child;
                destination = destination_node;
                continue;
            }

            while (source != source_root &&
                   source.next_sibling() == pugi::xml_node{}) {
                source = source.parent();
                destination = destination.parent();
            }
            if (source == source_root) {
                break;
            }
            source = source.next_sibling();
        }
        return xml_document_clone_status::success;
    } catch (...) {
        return xml_document_clone_status::allocation_failure;
    }
}

[[nodiscard]] inline auto set_xml_document_allocation_failure(
    document_error_info &error_info, std::string_view detail,
    std::string_view entry_name = {}) noexcept -> std::error_code {
    const auto code = std::make_error_code(std::errc::not_enough_memory);
    error_info.code = code;
    error_info.detail.clear();
    error_info.entry_name.clear();
    error_info.xml_offset.reset();

    try {
        error_info.detail.assign(detail);
        error_info.entry_name.assign(entry_name);
    } catch (...) {
        error_info.detail.clear();
        error_info.entry_name.clear();
    }
    return code;
}

[[nodiscard]] inline auto set_xml_document_clone_allocation_failure(
    document_error_info &error_info, std::string_view entry_name = {}) noexcept
    -> std::error_code {
    return set_xml_document_allocation_failure(
        error_info, "XML document clone ran out of memory", entry_name);
}

[[nodiscard]] inline auto set_xml_document_mutation_allocation_failure(
    document_error_info &error_info, std::string_view entry_name = {}) noexcept
    -> std::error_code {
    return set_xml_document_allocation_failure(
        error_info, "XML document mutation ran out of memory", entry_name);
}

} // namespace featherdoc::detail
