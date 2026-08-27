#include "wordprocessingml_namespace_helpers.hpp"

#include "xml_namespace_helpers.hpp"

#include <featherdoc/document_core.hpp>

#include <constants.hpp>

#include <cstddef>
#include <new>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc::detail {
namespace {

enum class traversal_task_kind : std::uint8_t {
    process_node = 0U,
    leave_element,
};

struct traversal_task final {
    pugi::xml_node source;
    pugi::xml_node output_parent;
    pugi::xml_node output_root;
    traversal_task_kind kind{traversal_task_kind::process_node};
};

auto expected_root_local_name(wordprocessingml_part_kind kind) noexcept
    -> std::string_view {
    switch (kind) {
    case wordprocessingml_part_kind::main_document:
        return "document";
    case wordprocessingml_part_kind::header:
        return "hdr";
    case wordprocessingml_part_kind::footer:
        return "ftr";
    case wordprocessingml_part_kind::styles:
        return "styles";
    case wordprocessingml_part_kind::numbering:
        return "numbering";
    case wordprocessingml_part_kind::settings:
        return "settings";
    case wordprocessingml_part_kind::footnotes:
        return "footnotes";
    case wordprocessingml_part_kind::endnotes:
        return "endnotes";
    case wordprocessingml_part_kind::comments:
        return "comments";
    }
    return {};
}

auto failure(wordprocessingml_namespace_status status, std::string detail)
    -> wordprocessingml_namespace_result {
    wordprocessingml_namespace_result result;
    result.status = status;
    result.detail = std::move(detail);
    return result;
}

auto allocation_failure() noexcept -> wordprocessingml_namespace_result {
    wordprocessingml_namespace_result result;
    result.status = wordprocessingml_namespace_status::allocation_failure;
    return result;
}

auto is_duplicate_expanded_attribute_error(std::string_view detail) noexcept
    -> bool {
    return detail.find("duplicate expanded attribute") !=
           std::string_view::npos;
}

auto copy_attribute(pugi::xml_node destination, pugi::xml_attribute source,
                    std::string_view output_name) -> bool {
    auto copied =
        destination.append_attribute(std::string{output_name}.c_str());
    return copied != pugi::xml_attribute{} && copied.set_value(source.value());
}

auto copy_non_element_node(pugi::xml_node source, pugi::xml_node output_parent)
    -> bool {
    auto output = output_parent.append_child(source.type());
    if (output == pugi::xml_node{}) {
        return false;
    }

    const auto name = std::string_view{source.name()};
    if (!name.empty() && !output.set_name(source.name())) {
        return false;
    }
    const auto value = std::string_view{source.value()};
    if (!value.empty() && !output.set_value(source.value())) {
        return false;
    }
    for (auto attribute = source.first_attribute();
         attribute != pugi::xml_attribute{};
         attribute = attribute.next_attribute()) {
        if (!copy_attribute(output, attribute, attribute.name())) {
            return false;
        }
    }
    return true;
}

auto set_allocation_failure(
    featherdoc::document_error_info &error_info) noexcept -> std::error_code {
    const auto code = std::make_error_code(std::errc::not_enough_memory);
    error_info.code = code;
    error_info.detail.clear();
    error_info.entry_name.clear();
    error_info.xml_offset.reset();
    return code;
}

} // namespace

wordprocessingml_namespace_result canonicalize_wordprocessingml_part(
    pugi::xml_document &document, wordprocessingml_part_kind kind,
    const wordprocessingml_namespace_limits &limits) {
    try {
        pugi::xml_document canonical_document;
        xml_namespace_scope namespace_scope{limits.maximum_depth};
        std::vector<traversal_task> tasks;

        for (auto child = document.last_child(); child != pugi::xml_node{};
             child = child.previous_sibling()) {
            tasks.push_back({child,
                             canonical_document,
                             {},
                             traversal_task_kind::process_node});
        }

        std::size_t element_count = 0U;
        std::size_t attribute_count = 0U;
        std::size_t top_level_element_count = 0U;
        bool changed = false;
        wordprocessingml_namespace_result result;
        std::string namespace_detail;

        while (!tasks.empty()) {
            auto task = tasks.back();
            tasks.pop_back();
            if (task.kind == traversal_task_kind::leave_element) {
                namespace_scope.leave_element();
                continue;
            }

            if (task.source.type() != pugi::node_element) {
                for (auto attribute = task.source.first_attribute();
                     attribute != pugi::xml_attribute{};
                     attribute = attribute.next_attribute()) {
                    if (attribute_count >= limits.maximum_attributes) {
                        return failure(
                            wordprocessingml_namespace_status::resource_limit,
                            "WordprocessingML attribute count exceeds the "
                            "namespace canonicalization limit");
                    }
                    ++attribute_count;
                    if (std::string_view{attribute.name()} == "xmlns:w" &&
                        std::string_view{attribute.value()} !=
                            wordprocessingml_namespace_uri) {
                        return failure(
                            wordprocessingml_namespace_status::wrong_w_binding,
                            "XML binds the reserved internal w prefix to an "
                            "unsupported namespace URI");
                    }
                }
                if (!copy_non_element_node(task.source, task.output_parent)) {
                    return allocation_failure();
                }
                continue;
            }

            if (element_count >= limits.maximum_elements) {
                return failure(
                    wordprocessingml_namespace_status::resource_limit,
                    "WordprocessingML element count exceeds the namespace "
                    "canonicalization limit");
            }
            ++element_count;

            for (auto attribute = task.source.first_attribute();
                 attribute != pugi::xml_attribute{};
                 attribute = attribute.next_attribute()) {
                if (attribute_count >= limits.maximum_attributes) {
                    return failure(
                        wordprocessingml_namespace_status::resource_limit,
                        "WordprocessingML attribute count exceeds the "
                        "namespace canonicalization limit");
                }
                ++attribute_count;
                if (std::string_view{attribute.name()} == "xmlns:w" &&
                    std::string_view{attribute.value()} !=
                        wordprocessingml_namespace_uri) {
                    return failure(
                        wordprocessingml_namespace_status::wrong_w_binding,
                        "XML binds the reserved internal w prefix to an "
                        "unsupported namespace URI");
                }
            }

            namespace_detail.clear();
            if (namespace_scope.depth() >= limits.maximum_depth) {
                return failure(
                    wordprocessingml_namespace_status::resource_limit,
                    "WordprocessingML nesting depth exceeds the namespace "
                    "canonicalization limit");
            }
            if (!namespace_scope.enter_element(task.source, namespace_detail)) {
                const auto failure_status =
                    is_duplicate_expanded_attribute_error(namespace_detail)
                        ? wordprocessingml_namespace_status::
                              duplicate_expanded_attribute
                        : wordprocessingml_namespace_status::
                              invalid_namespace_markup;
                return failure(failure_status, std::move(namespace_detail));
            }

            const auto expanded_element_name =
                namespace_scope.element_expanded_name(task.source);
            const bool is_wordprocessingml_element =
                expanded_element_name.namespace_uri ==
                wordprocessingml_namespace_uri;
            bool element_needs_w_binding = is_wordprocessingml_element;

            auto output_element =
                task.output_parent.append_child(pugi::node_element);
            if (output_element == pugi::xml_node{}) {
                return allocation_failure();
            }
            const auto output_element_name =
                is_wordprocessingml_element
                    ? std::string{"w:"} +
                          std::string{expanded_element_name.local_name}
                    : std::string{task.source.name()};
            if (!output_element.set_name(output_element_name.c_str())) {
                return allocation_failure();
            }
            changed = changed || output_element_name !=
                                     std::string_view{task.source.name()};

            for (auto attribute = task.source.first_attribute();
                 attribute != pugi::xml_attribute{};
                 attribute = attribute.next_attribute()) {
                auto output_attribute_name = std::string{attribute.name()};
                if (!xml_attribute_is_namespace_declaration(attribute)) {
                    const auto expanded_attribute_name =
                        namespace_scope.attribute_expanded_name(attribute);
                    if (expanded_attribute_name.namespace_uri ==
                        wordprocessingml_namespace_uri) {
                        output_attribute_name =
                            std::string{"w:"} +
                            std::string{expanded_attribute_name.local_name};
                        element_needs_w_binding = true;
                    }
                }
                changed = changed || output_attribute_name !=
                                         std::string_view{attribute.name()};
                if (!copy_attribute(output_element, attribute,
                                    output_attribute_name)) {
                    return allocation_failure();
                }
            }

            auto output_root = task.output_root == pugi::xml_node{}
                                   ? output_element
                                   : task.output_root;
            if (element_needs_w_binding &&
                namespace_scope.resolve_prefix("w").empty() &&
                output_root.attribute("xmlns:w") == pugi::xml_attribute{}) {
                if (attribute_count >= limits.maximum_attributes) {
                    return failure(
                        wordprocessingml_namespace_status::resource_limit,
                        "WordprocessingML attribute count exceeds the "
                        "namespace canonicalization limit");
                }
                auto declaration = output_root.append_attribute("xmlns:w");
                if (declaration == pugi::xml_attribute{} ||
                    !declaration.set_value(
                        wordprocessingml_namespace_uri.data())) {
                    return allocation_failure();
                }
                ++attribute_count;
                changed = true;
            }

            if (task.output_parent.type() == pugi::node_document) {
                ++top_level_element_count;
                if (top_level_element_count == 1U) {
                    result.actual_root_namespace_uri =
                        expanded_element_name.namespace_uri;
                    result.actual_root_local_name =
                        expanded_element_name.local_name;
                }
            }

            tasks.push_back(
                {task.source, {}, {}, traversal_task_kind::leave_element});
            for (auto child = task.source.last_child();
                 child != pugi::xml_node{}; child = child.previous_sibling()) {
                tasks.push_back({child, output_element, output_root,
                                 traversal_task_kind::process_node});
            }
        }

        result.root_matches =
            top_level_element_count == 1U &&
            result.actual_root_namespace_uri ==
                wordprocessingml_namespace_uri &&
            result.actual_root_local_name == expected_root_local_name(kind);

        result.changed = changed;
        document = std::move(canonical_document);
        return result;
    } catch (const std::bad_alloc &) {
        return allocation_failure();
    }
}

std::error_code wordprocessingml_namespace_error_code(
    wordprocessingml_namespace_status status) noexcept {
    switch (status) {
    case wordprocessingml_namespace_status::success:
        return {};
    case wordprocessingml_namespace_status::invalid_namespace_markup:
    case wordprocessingml_namespace_status::wrong_w_binding:
    case wordprocessingml_namespace_status::duplicate_expanded_attribute:
        return featherdoc::make_error_code(
            featherdoc::document_errc::invalid_package_structure);
    case wordprocessingml_namespace_status::resource_limit:
        return featherdoc::make_error_code(
            featherdoc::document_errc::archive_limit_exceeded);
    case wordprocessingml_namespace_status::allocation_failure:
        return std::make_error_code(std::errc::not_enough_memory);
    }
    return featherdoc::make_error_code(
        featherdoc::document_errc::invalid_package_structure);
}

std::error_code set_wordprocessingml_namespace_last_error(
    featherdoc::document_error_info &error_info,
    const wordprocessingml_namespace_result &result,
    std::string_view entry_name) {
    const auto code = wordprocessingml_namespace_error_code(result.status);
    if (result.status ==
        wordprocessingml_namespace_status::allocation_failure) {
        // Avoid allocating entry/detail strings while recovering from OOM.
        return set_allocation_failure(error_info);
    }

    try {
        error_info.code = code;
        error_info.detail = result.detail;
        error_info.entry_name = entry_name;
        error_info.xml_offset.reset();
        return code;
    } catch (const std::bad_alloc &) {
        return set_allocation_failure(error_info);
    }
}

std::error_code set_wordprocessingml_root_mismatch_last_error(
    featherdoc::document_error_info &error_info,
    const wordprocessingml_namespace_result &result,
    wordprocessingml_part_kind kind, std::string_view entry_name) {
    const auto code = featherdoc::make_error_code(
        featherdoc::document_errc::invalid_package_structure);
    try {
        error_info.code = code;
        error_info.detail = "WordprocessingML part '" +
                            std::string{entry_name} + "' has root '{" +
                            result.actual_root_namespace_uri + "}" +
                            result.actual_root_local_name + "'; expected '{" +
                            std::string{wordprocessingml_namespace_uri} + "}" +
                            std::string{expected_root_local_name(kind)} +
                            "'; entry_name='" + std::string{entry_name} +
                            "'; part_role='" +
                            std::string{expected_root_local_name(kind)} +
                            "'; expected_namespace_uri='" +
                            std::string{wordprocessingml_namespace_uri} +
                            "'; expected_local_name='" +
                            std::string{expected_root_local_name(kind)} +
                            "'; actual_namespace_uri='" +
                            result.actual_root_namespace_uri +
                            "'; actual_local_name='" +
                            result.actual_root_local_name + "'";
        error_info.entry_name = entry_name;
        error_info.xml_offset.reset();
        return code;
    } catch (const std::bad_alloc &) {
        return set_allocation_failure(error_info);
    }
}

} // namespace featherdoc::detail
