#include "xml_namespace_helpers.hpp"

#include <featherdoc/detail/utf8.hpp>

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace featherdoc::detail {
namespace {

auto namespace_declaration_prefix(pugi::xml_attribute attribute,
                                  std::string_view &prefix) noexcept -> bool {
    const auto name = std::string_view{attribute.name()};
    if (name == "xmlns") {
        prefix = {};
        return true;
    }

    const auto qualified_name = parse_xml_qualified_name(name);
    if (!qualified_name.valid || qualified_name.prefix != "xmlns" ||
        qualified_name.local_name.empty()) {
        return false;
    }
    prefix = qualified_name.local_name;
    return true;
}

auto expanded_attribute_key(const xml_expanded_name_view &name) -> std::string {
    auto key = std::string{name.namespace_uri};
    key.push_back('\0');
    key.append(name.local_name);
    return key;
}

auto is_xml_name_start_character(std::uint32_t code_point) noexcept -> bool {
    return code_point == '_' || (code_point >= 'A' && code_point <= 'Z') ||
           (code_point >= 'a' && code_point <= 'z') ||
           (code_point >= 0xC0U && code_point <= 0xD6U) ||
           (code_point >= 0xD8U && code_point <= 0xF6U) ||
           (code_point >= 0xF8U && code_point <= 0x2FFU) ||
           (code_point >= 0x370U && code_point <= 0x37DU) ||
           (code_point >= 0x37FU && code_point <= 0x1FFFU) ||
           (code_point >= 0x200CU && code_point <= 0x200DU) ||
           (code_point >= 0x2070U && code_point <= 0x218FU) ||
           (code_point >= 0x2C00U && code_point <= 0x2FEFU) ||
           (code_point >= 0x3001U && code_point <= 0xD7FFU) ||
           (code_point >= 0xF900U && code_point <= 0xFDCFU) ||
           (code_point >= 0xFDF0U && code_point <= 0xFFFDU) ||
           (code_point >= 0x10000U && code_point <= 0xEFFFFU);
}

auto is_xml_name_character(std::uint32_t code_point) noexcept -> bool {
    return is_xml_name_start_character(code_point) || code_point == '-' ||
           code_point == '.' || (code_point >= '0' && code_point <= '9') ||
           code_point == 0xB7U ||
           (code_point >= 0x300U && code_point <= 0x36FU) ||
           (code_point >= 0x203FU && code_point <= 0x2040U);
}

auto is_valid_xml_ncname(std::string_view name) noexcept -> bool {
    if (name.empty()) {
        return false;
    }
    std::size_t offset = 0U;
    auto decoded = decode_next_utf8(name, offset);
    if (!decoded.valid || decoded.length == 0U ||
        !is_xml_name_start_character(decoded.code_point)) {
        return false;
    }
    offset += decoded.length;
    while (offset < name.size()) {
        decoded = decode_next_utf8(name, offset);
        if (!decoded.valid || decoded.length == 0U ||
            !is_xml_name_character(decoded.code_point)) {
            return false;
        }
        offset += decoded.length;
    }
    return true;
}

} // namespace

xml_qualified_name_view
parse_xml_qualified_name(std::string_view name) noexcept {
    if (name.empty()) {
        return {};
    }

    const auto separator = name.find(':');
    if (separator == std::string_view::npos) {
        return is_valid_xml_ncname(name)
                   ? xml_qualified_name_view{{}, name, true}
                   : xml_qualified_name_view{};
    }
    if (separator == 0U || separator + 1U == name.size() ||
        name.find(':', separator + 1U) != std::string_view::npos) {
        return {};
    }
    const auto prefix = name.substr(0U, separator);
    const auto local_name = name.substr(separator + 1U);
    return is_valid_xml_ncname(prefix) && is_valid_xml_ncname(local_name)
               ? xml_qualified_name_view{prefix, local_name, true}
               : xml_qualified_name_view{};
}

bool xml_attribute_is_namespace_declaration(
    pugi::xml_attribute attribute) noexcept {
    const auto name = std::string_view{attribute.name()};
    return name == "xmlns" || name.starts_with("xmlns:");
}

std::string_view resolve_xml_namespace_uri(pugi::xml_node node,
                                           std::string_view prefix) {
    if (prefix == "xml") {
        return xml_namespace_uri;
    }
    if (prefix == "xmlns") {
        return xmlns_namespace_uri;
    }

    std::string declaration_name;
    if (!prefix.empty()) {
        declaration_name = "xmlns:";
        declaration_name.append(prefix);
    }

    for (auto current = node; current != pugi::xml_node{};
         current = current.parent()) {
        if (current.type() != pugi::node_element) {
            continue;
        }
        const auto declaration =
            prefix.empty() ? current.attribute("xmlns")
                           : current.attribute(declaration_name.c_str());
        if (declaration != pugi::xml_attribute{}) {
            return declaration.value();
        }
    }
    return {};
}

bool xml_element_has_expanded_name(pugi::xml_node node,
                                   std::string_view local_name,
                                   std::string_view namespace_uri) {
    if (node.type() != pugi::node_element) {
        return false;
    }
    const auto qualified_name =
        parse_xml_qualified_name(std::string_view{node.name()});
    return qualified_name.valid && qualified_name.local_name == local_name &&
           resolve_xml_namespace_uri(node, qualified_name.prefix) ==
               namespace_uri;
}

xml_namespace_scope::xml_namespace_scope(std::size_t maximum_depth)
    : maximum_depth_(maximum_depth) {
    bindings_["xml"].emplace_back(xml_namespace_uri);
}

bool xml_namespace_scope::enter_element(pugi::xml_node element,
                                        std::string &detail) {
    if (element.type() != pugi::node_element) {
        detail = "namespace validation encountered a non-element node";
        return false;
    }
    if (depth_ >= maximum_depth_) {
        detail = "XML namespace nesting depth exceeds the supported limit";
        return false;
    }

    std::vector<std::string> declarations;
    std::unordered_set<std::string> declared_prefixes;
    for (auto attribute = element.first_attribute();
         attribute != pugi::xml_attribute{};
         attribute = attribute.next_attribute()) {
        if (!xml_attribute_is_namespace_declaration(attribute)) {
            continue;
        }

        std::string_view prefix;
        if (!namespace_declaration_prefix(attribute, prefix)) {
            detail = "XML contains a malformed namespace declaration name";
            goto rollback;
        }
        if (prefix == "xmlns") {
            detail = "XML attempts to declare the reserved xmlns prefix";
            goto rollback;
        }
        if (!declared_prefixes.emplace(prefix).second) {
            detail = "XML contains duplicate namespace declarations";
            goto rollback;
        }

        const auto namespace_value = std::string_view{attribute.value()};
        if (namespace_value == xmlns_namespace_uri) {
            detail = "XML binds a prefix to the reserved xmlns namespace";
            goto rollback;
        }
        if (prefix == "xml") {
            if (namespace_value != xml_namespace_uri) {
                detail = "XML illegally rebinds the reserved xml prefix";
                goto rollback;
            }
        } else if (namespace_value == xml_namespace_uri) {
            detail = "XML binds a non-xml prefix to the reserved xml namespace";
            goto rollback;
        }
        if (!prefix.empty() && namespace_value.empty()) {
            detail = "XML undeclares a non-default namespace prefix";
            goto rollback;
        }

        auto prefix_string = std::string{prefix};
        bindings_[prefix_string].emplace_back(namespace_value);
        declarations.push_back(std::move(prefix_string));
    }

    {
        const auto element_name = element_expanded_name(element);
        if (!element_name.valid) {
            detail =
                "XML contains an element with a malformed or unbound QName";
            goto rollback;
        }

        std::unordered_set<std::string> expanded_attributes;
        for (auto attribute = element.first_attribute();
             attribute != pugi::xml_attribute{};
             attribute = attribute.next_attribute()) {
            if (xml_attribute_is_namespace_declaration(attribute)) {
                continue;
            }
            const auto attribute_name = attribute_expanded_name(attribute);
            if (!attribute_name.valid) {
                detail = "XML contains an attribute with a malformed or "
                         "unbound QName";
                goto rollback;
            }
            if (!expanded_attributes
                     .emplace(expanded_attribute_key(attribute_name))
                     .second) {
                detail = "XML contains duplicate expanded attribute names";
                goto rollback;
            }
        }
    }

    declaration_frames_.push_back(std::move(declarations));
    ++depth_;
    return true;

rollback:
    for (auto iterator = declarations.rbegin(); iterator != declarations.rend();
         ++iterator) {
        auto binding = bindings_.find(*iterator);
        if (binding == bindings_.end()) {
            continue;
        }
        binding->second.pop_back();
        if (binding->second.empty()) {
            bindings_.erase(binding);
        }
    }
    return false;
}

void xml_namespace_scope::leave_element() noexcept {
    if (declaration_frames_.empty()) {
        return;
    }
    for (auto iterator = declaration_frames_.back().rbegin();
         iterator != declaration_frames_.back().rend(); ++iterator) {
        auto binding = bindings_.find(*iterator);
        if (binding == bindings_.end()) {
            continue;
        }
        binding->second.pop_back();
        if (binding->second.empty()) {
            bindings_.erase(binding);
        }
    }
    declaration_frames_.pop_back();
    --depth_;
}

std::string_view
xml_namespace_scope::resolve_prefix(std::string_view prefix) const noexcept {
    const auto binding = bindings_.find(prefix);
    if (binding == bindings_.end() || binding->second.empty()) {
        return {};
    }
    return binding->second.back();
}

xml_expanded_name_view xml_namespace_scope::element_expanded_name(
    pugi::xml_node element) const noexcept {
    const auto qualified_name =
        parse_xml_qualified_name(std::string_view{element.name()});
    if (!qualified_name.valid || qualified_name.prefix == "xmlns") {
        return {};
    }
    if (qualified_name.prefix.empty()) {
        return {resolve_prefix({}), qualified_name.local_name, true};
    }
    const auto namespace_value = resolve_prefix(qualified_name.prefix);
    return namespace_value.empty()
               ? xml_expanded_name_view{}
               : xml_expanded_name_view{namespace_value,
                                        qualified_name.local_name, true};
}

xml_expanded_name_view xml_namespace_scope::attribute_expanded_name(
    pugi::xml_attribute attribute) const noexcept {
    if (xml_attribute_is_namespace_declaration(attribute)) {
        std::string_view declared_prefix;
        if (!namespace_declaration_prefix(attribute, declared_prefix)) {
            return {};
        }
        return {xmlns_namespace_uri, declared_prefix, true};
    }

    const auto qualified_name =
        parse_xml_qualified_name(std::string_view{attribute.name()});
    if (!qualified_name.valid || qualified_name.prefix == "xmlns") {
        return {};
    }
    if (qualified_name.prefix.empty()) {
        // The default namespace never applies to attributes.
        return {{}, qualified_name.local_name, true};
    }
    const auto namespace_value = resolve_prefix(qualified_name.prefix);
    return namespace_value.empty()
               ? xml_expanded_name_view{}
               : xml_expanded_name_view{namespace_value,
                                        qualified_name.local_name, true};
}

std::vector<std::pair<std::string_view, std::string_view>>
xml_namespace_scope::effective_bindings() const {
    std::vector<std::pair<std::string_view, std::string_view>> result;
    result.reserve(bindings_.size());
    for (const auto &[prefix, values] : bindings_) {
        if (prefix == "xml" || values.empty()) {
            continue;
        }
        result.emplace_back(prefix, values.back());
    }
    return result;
}

std::size_t xml_namespace_scope::effective_binding_count() const noexcept {
    return static_cast<std::size_t>(
        std::ranges::count_if(bindings_, [](const auto &binding) {
            return binding.first != "xml" && !binding.second.empty();
        }));
}

std::size_t xml_namespace_scope::depth() const noexcept { return depth_; }

xml_namespace_validation_result
validate_xml_namespace_well_formedness(const pugi::xml_document &document,
                                       std::size_t maximum_depth,
                                       std::size_t maximum_elements) {
    struct traversal_task final {
        pugi::xml_node node;
        bool leave{false};
    };

    std::vector<traversal_task> tasks;
    for (auto child = document.last_child(); child != pugi::xml_node{};
         child = child.previous_sibling()) {
        if (child.type() == pugi::node_element) {
            tasks.push_back({child, false});
        }
    }

    xml_namespace_scope scope{maximum_depth};
    std::size_t element_count = 0U;
    std::string detail;
    while (!tasks.empty()) {
        const auto task = tasks.back();
        tasks.pop_back();
        if (task.leave) {
            scope.leave_element();
            continue;
        }
        if (++element_count > maximum_elements) {
            return {false,
                    "XML element count exceeds the namespace validation limit"};
        }
        if (!scope.enter_element(task.node, detail)) {
            return {false, std::move(detail)};
        }
        tasks.push_back({task.node, true});
        for (auto child = task.node.last_child(); child != pugi::xml_node{};
             child = child.previous_sibling()) {
            if (child.type() == pugi::node_element) {
                tasks.push_back({child, false});
            }
        }
    }
    return {true, {}};
}

} // namespace featherdoc::detail
