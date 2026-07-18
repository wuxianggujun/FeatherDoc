#include "package_relationships_mce_helpers.hpp"

#include "xml_namespace_helpers.hpp"

#include <featherdoc/document_core.hpp>

#include <algorithm>
#include <cstddef>
#include <new>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace featherdoc::detail {
namespace {

constexpr auto package_relationships_namespace = std::string_view{
    "http://schemas.openxmlformats.org/package/2006/relationships"};
constexpr auto markup_compatibility_namespace = std::string_view{
    "http://schemas.openxmlformats.org/markup-compatibility/2006"};
constexpr auto xml_schema_instance_namespace =
    std::string_view{"http://www.w3.org/2001/XMLSchema-instance"};

struct namespace_work_limit_exceeded final {};

class namespace_work_budget final {
  public:
    explicit namespace_work_budget(std::size_t maximum_bytes) noexcept
        : maximum_bytes_(maximum_bytes) {}

    void consume(std::size_t bytes) {
        if (bytes > maximum_bytes_ - consumed_bytes_) {
            throw namespace_work_limit_exceeded{};
        }
        consumed_bytes_ += bytes;
    }

    void consume(std::string_view value) { consume(value.size()); }

    void consume_multiple(std::string_view value, std::size_t count) {
        if (value.empty() || count == 0U) {
            return;
        }
        const auto remaining = maximum_bytes_ - consumed_bytes_;
        if (count > remaining / value.size()) {
            throw namespace_work_limit_exceeded{};
        }
        consumed_bytes_ += value.size() * count;
    }

  private:
    std::size_t maximum_bytes_;
    std::size_t consumed_bytes_{0U};
};

enum class task_kind : std::uint8_t {
    process_node = 0U,
    process_selected_wrapper,
    leave_element,
};

struct processing_task final {
    pugi::xml_node source;
    pugi::xml_node output_parent;
    task_kind kind{task_kind::process_node};
    bool source_parent_was_copied{false};
};

enum class syntax_task_kind : std::uint8_t {
    enter_element = 0U,
    leave_element,
};

enum class output_append_status : std::uint8_t {
    success = 0U,
    output_limit,
    namespace_mismatch,
    allocation_failure,
};

struct syntax_validation_task final {
    pugi::xml_node source;
    syntax_task_kind kind{syntax_task_kind::enter_element};
    bool parent_is_alternate_content{false};
};

struct process_content_name final {
    std::string namespace_uri;
    std::string local_name;

    [[nodiscard]] bool operator==(const process_content_name &) const = default;
};

struct mce_frame final {
    std::vector<std::string> ignorable_namespaces;
    std::vector<process_content_name> process_content_names;
};

struct mce_context final {
    std::unordered_map<std::string, std::size_t, transparent_string_hash,
                       transparent_string_equal>
        ignorable_namespaces;
    std::vector<process_content_name> process_content_names;
    std::vector<mce_frame> frames;

    [[nodiscard]] bool is_ignorable(std::string_view namespace_uri,
                                    namespace_work_budget &budget) const {
        budget.consume(namespace_uri);
        const auto found = ignorable_namespaces.find(namespace_uri);
        return found != ignorable_namespaces.end() && found->second != 0U;
    }

    [[nodiscard]] bool
    should_process_content(std::string_view namespace_uri,
                           std::string_view local_name,
                           namespace_work_budget &budget) const {
        return std::ranges::any_of(
            process_content_names, [&](const auto &name) {
                budget.consume(namespace_uri);
                budget.consume(local_name);
                budget.consume(name.namespace_uri);
                budget.consume(name.local_name);
                return name.namespace_uri == namespace_uri &&
                       (name.local_name == "*" ||
                        name.local_name == local_name);
            });
    }

    void push(mce_frame frame, namespace_work_budget &budget) {
        for (const auto &namespace_uri : frame.ignorable_namespaces) {
            budget.consume(namespace_uri);
            ++ignorable_namespaces[namespace_uri];
        }
        for (const auto &name : frame.process_content_names) {
            budget.consume(name.namespace_uri);
            budget.consume(name.local_name);
            process_content_names.push_back(name);
        }
        frames.push_back(std::move(frame));
    }

    void pop(namespace_work_budget &budget) {
        if (frames.empty()) {
            return;
        }
        const auto &frame = frames.back();
        for (const auto &namespace_uri : frame.ignorable_namespaces) {
            budget.consume(namespace_uri);
            const auto found = ignorable_namespaces.find(namespace_uri);
            if (found == ignorable_namespaces.end()) {
                continue;
            }
            if (--found->second == 0U) {
                ignorable_namespaces.erase(found);
            }
        }
        process_content_names.resize(process_content_names.size() -
                                     frame.process_content_names.size());
        frames.pop_back();
    }
};

auto failure(package_relationships_mce_status status, std::string detail)
    -> package_relationships_mce_result {
    return {status, std::move(detail)};
}

auto allocation_failure() noexcept -> package_relationships_mce_result {
    package_relationships_mce_result result;
    result.status = package_relationships_mce_status::allocation_failure;
    return result;
}

auto namespace_resource_limit_failure() noexcept
    -> package_relationships_mce_result {
    package_relationships_mce_result result;
    result.status = package_relationships_mce_status::resource_limit;
    try {
        result.detail = "Relationships MCE cumulative namespace work exceeds "
                        "the supported limit";
    } catch (const std::bad_alloc &) {
        return allocation_failure();
    }
    return result;
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

auto is_xml_whitespace(std::string_view text) noexcept -> bool {
    return std::ranges::all_of(text, [](char character) {
        return character == ' ' || character == '\t' || character == '\r' ||
               character == '\n';
    });
}

auto namespace_is_understood(std::string_view namespace_uri) noexcept -> bool {
    // Part 3 evaluates Requires/MustUnderstand against the application and
    // markup-configuration namespaces. Relationships preprocessing has an
    // empty markup configuration and only the OPC Relationships application
    // namespace.
    return namespace_uri == package_relationships_namespace;
}

auto next_whitespace_token(std::string_view value, std::size_t &offset) noexcept
    -> std::string_view {
    while (offset < value.size()) {
        while (offset < value.size() &&
               (value[offset] == ' ' || value[offset] == '\t' ||
                value[offset] == '\r' || value[offset] == '\n')) {
            ++offset;
        }
        const auto begin = offset;
        while (offset < value.size() && value[offset] != ' ' &&
               value[offset] != '\t' && value[offset] != '\r' &&
               value[offset] != '\n') {
            ++offset;
        }
        if (begin != offset) {
            return value.substr(begin, offset - begin);
        }
    }
    return {};
}

auto parse_prefix_list(std::string_view value, xml_namespace_scope &scope,
                       std::vector<std::string> &namespace_uris,
                       std::string_view directive_name, bool allow_empty,
                       bool reject_mce_namespace, namespace_work_budget &budget,
                       std::string &detail) -> bool {
    bool saw_token = false;
    std::size_t offset = 0U;
    for (;;) {
        const auto token = next_whitespace_token(value, offset);
        if (token.empty()) {
            break;
        }
        saw_token = true;
        budget.consume(token);
        const auto name = parse_xml_qualified_name(token);
        if (!name.valid || !name.prefix.empty()) {
            detail = std::string{directive_name} +
                     " contains a malformed namespace prefix";
            return false;
        }
        const auto namespace_uri = scope.resolve_prefix(name.local_name);
        if (namespace_uri.empty()) {
            detail = std::string{directive_name} +
                     " references an undeclared namespace prefix";
            return false;
        }
        // Account separately for validation/comparison work and for the copy
        // retained in the current MCE frame.
        budget.consume(namespace_uri);
        if (reject_mce_namespace &&
            namespace_uri == markup_compatibility_namespace) {
            detail =
                std::string{directive_name} + " cannot name the MCE namespace";
            return false;
        }
        budget.consume(namespace_uri);
        namespace_uris.emplace_back(namespace_uri);
    }
    if (!saw_token) {
        if (allow_empty) {
            return true;
        }
        detail = std::string{directive_name} +
                 " must contain at least one namespace prefix";
        return false;
    }
    return true;
}

auto parse_process_content(std::string_view value, xml_namespace_scope &scope,
                           const mce_context &context,
                           const std::vector<std::string> &local_ignorable,
                           std::vector<process_content_name> &names,
                           namespace_work_budget &budget, std::string &detail)
    -> bool {
    std::size_t offset = 0U;
    for (;;) {
        const auto token = next_whitespace_token(value, offset);
        if (token.empty()) {
            break;
        }
        budget.consume(token);
        const auto separator = token.find(':');
        if (separator == std::string_view::npos || separator == 0U ||
            separator + 1U == token.size() ||
            token.find(':', separator + 1U) != std::string_view::npos) {
            detail = "mc:ProcessContent contains a malformed QName";
            return false;
        }
        const auto prefix_name =
            parse_xml_qualified_name(token.substr(0U, separator));
        const auto local_name_text = token.substr(separator + 1U);
        const auto local_name = parse_xml_qualified_name(local_name_text);
        if (!prefix_name.valid || !prefix_name.prefix.empty() ||
            (local_name_text != "*" &&
             (!local_name.valid || !local_name.prefix.empty()))) {
            detail = "mc:ProcessContent contains a malformed QName";
            return false;
        }
        const auto namespace_uri = scope.resolve_prefix(prefix_name.local_name);
        if (namespace_uri.empty()) {
            detail =
                "mc:ProcessContent references an undeclared namespace prefix";
            return false;
        }
        bool is_locally_ignorable = false;
        for (const auto &candidate : local_ignorable) {
            budget.consume(namespace_uri);
            budget.consume(candidate);
            if (candidate == namespace_uri) {
                is_locally_ignorable = true;
                break;
            }
        }
        if (!context.is_ignorable(namespace_uri, budget) &&
            !is_locally_ignorable) {
            detail = "mc:ProcessContent references a namespace that is not "
                     "mc:Ignorable in the current scope";
            return false;
        }
        budget.consume(namespace_uri);
        budget.consume(local_name_text);
        process_content_name process_content;
        process_content.namespace_uri = namespace_uri;
        process_content.local_name = local_name_text;
        names.push_back(std::move(process_content));
    }
    return true;
}

auto find_mce_attribute(pugi::xml_node element,
                        const xml_namespace_scope &scope,
                        std::string_view local_name) -> pugi::xml_attribute {
    for (auto attribute = element.first_attribute();
         attribute != pugi::xml_attribute{};
         attribute = attribute.next_attribute()) {
        if (xml_attribute_is_namespace_declaration(attribute)) {
            continue;
        }
        const auto name = scope.attribute_expanded_name(attribute);
        if (name.valid &&
            name.namespace_uri == markup_compatibility_namespace &&
            name.local_name == local_name) {
            return attribute;
        }
    }
    return {};
}

auto build_mce_frame(pugi::xml_node element, xml_namespace_scope &scope,
                     const mce_context &parent_context, mce_frame &frame,
                     namespace_work_budget &budget, std::string &detail)
    -> bool {
    if (const auto ignorable = find_mce_attribute(element, scope, "Ignorable");
        ignorable != pugi::xml_attribute{} &&
        !parse_prefix_list(ignorable.value(), scope, frame.ignorable_namespaces,
                           "mc:Ignorable", true, true, budget, detail)) {
        return false;
    }

    if (const auto process_content =
            find_mce_attribute(element, scope, "ProcessContent");
        process_content != pugi::xml_attribute{} &&
        !parse_process_content(process_content.value(), scope, parent_context,
                               frame.ignorable_namespaces,
                               frame.process_content_names, budget, detail)) {
        return false;
    }

    for (auto attribute = element.first_attribute();
         attribute != pugi::xml_attribute{};
         attribute = attribute.next_attribute()) {
        if (xml_attribute_is_namespace_declaration(attribute)) {
            continue;
        }
        const auto name = scope.attribute_expanded_name(attribute);
        if (!name.valid ||
            name.namespace_uri != markup_compatibility_namespace) {
            continue;
        }
        if (name.local_name == "Ignorable" ||
            name.local_name == "ProcessContent" ||
            name.local_name == "MustUnderstand") {
            continue;
        }
        if (name.local_name == "PreserveElements" ||
            name.local_name == "PreserveAttributes") {
            detail = "unsupported pre-5th-edition MCE Preserve* directive";
        } else {
            detail = "Relationships XML contains an unknown MCE attribute";
        }
        return false;
    }
    return true;
}

auto check_must_understand(pugi::xml_node element, xml_namespace_scope &scope,
                           bool enforce_understanding,
                           namespace_work_budget &budget, std::string &detail)
    -> package_relationships_mce_status {
    const auto attribute = find_mce_attribute(element, scope, "MustUnderstand");
    if (attribute == pugi::xml_attribute{}) {
        return package_relationships_mce_status::success;
    }

    std::vector<std::string> namespaces;
    if (!parse_prefix_list(attribute.value(), scope, namespaces,
                           "mc:MustUnderstand", true, true, budget, detail)) {
        return package_relationships_mce_status::invalid_mce_markup;
    }
    if (enforce_understanding &&
        std::ranges::any_of(namespaces, [](const auto &namespace_uri) {
            return !namespace_is_understood(namespace_uri);
        })) {
        detail = "mc:MustUnderstand requires an unsupported namespace";
        return package_relationships_mce_status::mismatch;
    }
    return package_relationships_mce_status::success;
}

auto namespace_is_locally_ignorable(std::string_view namespace_uri,
                                    const mce_context &context,
                                    const mce_frame &frame,
                                    namespace_work_budget &budget) -> bool {
    if (context.is_ignorable(namespace_uri, budget)) {
        return true;
    }
    for (const auto &candidate : frame.ignorable_namespaces) {
        budget.consume(namespace_uri);
        budget.consume(candidate);
        if (candidate == namespace_uri) {
            return true;
        }
    }
    return false;
}

auto frame_processes_content(const mce_frame &frame,
                             std::string_view namespace_uri,
                             std::string_view local_name,
                             namespace_work_budget &budget) -> bool {
    return std::ranges::any_of(
        frame.process_content_names, [&](const auto &name) {
            budget.consume(namespace_uri);
            budget.consume(local_name);
            budget.consume(name.namespace_uri);
            budget.consume(name.local_name);
            return name.namespace_uri == namespace_uri &&
                   (name.local_name == "*" || name.local_name == local_name);
        });
}

auto validate_mce_wrapper_attributes(
    pugi::xml_node wrapper, std::string_view wrapper_local_name,
    const xml_namespace_scope &scope, const mce_context &context,
    const mce_frame &frame, namespace_work_budget &budget, std::string &detail)
    -> package_relationships_mce_status {
    for (auto attribute = wrapper.first_attribute();
         attribute != pugi::xml_attribute{};
         attribute = attribute.next_attribute()) {
        if (xml_attribute_is_namespace_declaration(attribute)) {
            continue;
        }
        const auto name = scope.attribute_expanded_name(attribute);
        if (name.namespace_uri == markup_compatibility_namespace) {
            if (name.local_name == "Ignorable" ||
                name.local_name == "ProcessContent" ||
                name.local_name == "MustUnderstand") {
                continue;
            }
            if (name.local_name == "PreserveElements" ||
                name.local_name == "PreserveAttributes") {
                detail = "unsupported pre-5th-edition MCE Preserve* directive";
            } else {
                detail = "MCE wrapper contains an unknown MCE attribute";
            }
            return package_relationships_mce_status::invalid_mce_markup;
        }
        if (name.namespace_uri == xml_namespace_uri) {
            detail = "MCE wrapper contains an xml attribute that is not "
                     "declared by the OPC schema";
            return package_relationships_mce_status::mismatch;
        }
        if (name.namespace_uri.empty()) {
            if (wrapper_local_name == "Choice" &&
                name.local_name == "Requires") {
                continue;
            }
            detail = "MCE wrapper contains an unexpected unqualified "
                     "attribute";
            return package_relationships_mce_status::invalid_mce_markup;
        }
        if (namespace_is_locally_ignorable(name.namespace_uri, context, frame,
                                           budget)) {
            continue;
        }
        detail = "MCE wrapper contains a non-ignorable unsupported attribute";
        return package_relationships_mce_status::mismatch;
    }
    return package_relationships_mce_status::success;
}

auto validate_process_content_wrapper_reserved_attributes(
    pugi::xml_node wrapper, const xml_namespace_scope &scope,
    std::string &detail) -> package_relationships_mce_status {
    for (auto attribute = wrapper.first_attribute();
         attribute != pugi::xml_attribute{};
         attribute = attribute.next_attribute()) {
        if (xml_attribute_is_namespace_declaration(attribute)) {
            continue;
        }
        const auto name = scope.attribute_expanded_name(attribute);
        if (name.namespace_uri == xml_namespace_uri &&
            (name.local_name == "base" || name.local_name == "lang" ||
             name.local_name == "space")) {
            detail = "mc:ProcessContent wrapper contains an inherited xml "
                     "attribute whose semantics would change after unwrapping";
            return package_relationships_mce_status::mismatch;
        }
    }
    return package_relationships_mce_status::success;
}

auto validate_choice_requires(pugi::xml_node choice, xml_namespace_scope &scope,
                              std::vector<std::string> &required_namespaces,
                              namespace_work_budget &budget,
                              std::string &detail)
    -> package_relationships_mce_status {
    const auto requires_attribute = choice.attribute("Requires");
    if (requires_attribute == pugi::xml_attribute{}) {
        detail = "mc:Choice is missing its required Requires attribute";
        return package_relationships_mce_status::invalid_mce_markup;
    }
    if (!parse_prefix_list(requires_attribute.value(), scope,
                           required_namespaces, "mc:Choice/@Requires", false,
                           true, budget, detail)) {
        return package_relationships_mce_status::invalid_mce_markup;
    }
    return package_relationships_mce_status::success;
}

auto inspect_choice_for_selection(pugi::xml_node choice,
                                  xml_namespace_scope &scope,
                                  const mce_context &context,
                                  bool evaluate_support, bool &supported,
                                  namespace_work_budget &budget,
                                  std::string &detail)
    -> package_relationships_mce_status {
    supported = false;
    if (!scope.enter_element(choice, detail)) {
        return package_relationships_mce_status::invalid_namespace_markup;
    }

    // ECMA-376 Part 3 syntax constraints apply to every MCE construct,
    // including a Choice that is not selected. evaluate_support controls only
    // the semantic branch-selection decision; it does not disable syntax
    // validation, and MustUnderstand is deliberately not enforced here.
    mce_frame frame;
    if (!build_mce_frame(choice, scope, context, frame, budget, detail)) {
        scope.leave_element();
        return package_relationships_mce_status::invalid_mce_markup;
    }
    const auto wrapper_status = validate_mce_wrapper_attributes(
        choice, "Choice", scope, context, frame, budget, detail);
    if (wrapper_status != package_relationships_mce_status::success) {
        scope.leave_element();
        return wrapper_status;
    }
    const auto must_understand_status =
        check_must_understand(choice, scope, false, budget, detail);
    if (must_understand_status != package_relationships_mce_status::success) {
        scope.leave_element();
        return must_understand_status;
    }

    std::vector<std::string> namespaces;
    const auto requires_status =
        validate_choice_requires(choice, scope, namespaces, budget, detail);
    if (requires_status == package_relationships_mce_status::success &&
        evaluate_support) {
        supported =
            std::ranges::all_of(namespaces, [](const auto &namespace_uri) {
                return namespace_is_understood(namespace_uri);
            });
    }
    scope.leave_element();
    return requires_status;
}

auto inspect_fallback_for_selection(pugi::xml_node fallback,
                                    xml_namespace_scope &scope,
                                    const mce_context &context,
                                    namespace_work_budget &budget,
                                    std::string &detail)
    -> package_relationships_mce_status {
    if (!scope.enter_element(fallback, detail)) {
        return package_relationships_mce_status::invalid_namespace_markup;
    }
    mce_frame frame;
    if (!build_mce_frame(fallback, scope, context, frame, budget, detail)) {
        scope.leave_element();
        return package_relationships_mce_status::invalid_mce_markup;
    }
    const auto wrapper_status = validate_mce_wrapper_attributes(
        fallback, "Fallback", scope, context, frame, budget, detail);
    if (wrapper_status != package_relationships_mce_status::success) {
        scope.leave_element();
        return wrapper_status;
    }
    const auto status =
        check_must_understand(fallback, scope, false, budget, detail);
    scope.leave_element();
    return status;
}

auto select_alternate_content_branch(pugi::xml_node alternate_content,
                                     xml_namespace_scope &scope,
                                     const mce_context &context,
                                     pugi::xml_node &selected,
                                     namespace_work_budget &budget,
                                     std::string &detail)
    -> package_relationships_mce_status {
    selected = {};
    pugi::xml_node fallback;
    bool saw_choice = false;
    bool saw_fallback = false;
    for (auto child = alternate_content.first_child();
         child != pugi::xml_node{}; child = child.next_sibling()) {
        if (child.type() != pugi::node_element) {
            if ((child.type() == pugi::node_pcdata ||
                 child.type() == pugi::node_cdata) &&
                !is_xml_whitespace(child.value())) {
                detail = "mc:AlternateContent contains unexpected text";
                return package_relationships_mce_status::invalid_mce_markup;
            }
            continue;
        }

        // Enter the child once and resolve its expanded name from the current
        // namespace scope. This includes child-local declarations without an
        // O(depth) walk through all ancestors for every Choice/Fallback.
        if (!scope.enter_element(child, detail)) {
            return package_relationships_mce_status::invalid_namespace_markup;
        }
        const auto name = scope.element_expanded_name(child);
        if (!name.valid) {
            scope.leave_element();
            detail = "mc:AlternateContent contains a malformed element name";
            return package_relationships_mce_status::invalid_namespace_markup;
        }
        if (name.namespace_uri != markup_compatibility_namespace) {
            mce_frame frame;
            if (!build_mce_frame(child, scope, context, frame, budget,
                                 detail)) {
                scope.leave_element();
                return package_relationships_mce_status::invalid_mce_markup;
            }
            const bool ignorable = namespace_is_locally_ignorable(
                name.namespace_uri, context, frame, budget);
            const bool process_content =
                context.should_process_content(name.namespace_uri,
                                               name.local_name, budget) ||
                frame_processes_content(frame, name.namespace_uri,
                                        name.local_name, budget);
            scope.leave_element();
            if (ignorable && !process_content) {
                continue;
            }
            detail = "mc:AlternateContent contains a non-ignorable or "
                     "ProcessContent foreign child";
            return package_relationships_mce_status::mismatch;
        }

        const auto local_name = name.local_name;
        scope.leave_element();

        if (local_name == "Choice") {
            if (saw_fallback) {
                detail = "mc:Choice appears after mc:Fallback";
                return package_relationships_mce_status::invalid_mce_markup;
            }
            saw_choice = true;
            bool supported = false;
            const auto status = inspect_choice_for_selection(
                child, scope, context, selected == pugi::xml_node{}, supported,
                budget, detail);
            if (status != package_relationships_mce_status::success) {
                return status;
            }
            if (selected == pugi::xml_node{} && supported) {
                selected = child;
            }
            continue;
        }
        if (local_name == "Fallback") {
            if (saw_fallback) {
                detail = "mc:AlternateContent contains multiple Fallback nodes";
                return package_relationships_mce_status::invalid_mce_markup;
            }
            saw_fallback = true;
            const auto status = inspect_fallback_for_selection(
                child, scope, context, budget, detail);
            if (status != package_relationships_mce_status::success) {
                return status;
            }
            fallback = child;
            continue;
        }

        detail = "mc:AlternateContent contains an unsupported MCE child";
        return package_relationships_mce_status::invalid_mce_markup;
    }

    if (!saw_choice) {
        detail = "mc:AlternateContent does not contain an mc:Choice";
        return package_relationships_mce_status::invalid_mce_markup;
    }
    if (selected == pugi::xml_node{}) {
        selected = fallback;
    }
    return package_relationships_mce_status::success;
}

auto sorted_effective_bindings(const xml_namespace_scope &scope,
                               namespace_work_budget &budget)
    -> std::vector<std::pair<std::string_view, std::string_view>> {
    auto bindings = scope.effective_bindings();

    // std::ranges::sort compares prefixes O(N log N) times. Reserve a
    // conservative comparison budget before sorting so an inherited set of
    // long prefixes cannot consume unbounded CPU for every unwrapped child.
    std::size_t logarithmic_rounds = 1U;
    for (auto count = bindings.size(); count > 1U; count = (count + 1U) / 2U) {
        ++logarithmic_rounds;
    }
    for (const auto &[prefix, namespace_uri] : bindings) {
        budget.consume_multiple(prefix, logarithmic_rounds * 4U);
        budget.consume(namespace_uri);
    }
    std::ranges::sort(bindings, {},
                      [](const auto &binding) { return binding.first; });
    return bindings;
}

auto append_namespace_binding(pugi::xml_node output, std::string_view prefix,
                              std::string_view namespace_uri,
                              std::size_t &output_attribute_count,
                              std::size_t maximum_output_attributes,
                              namespace_work_budget &budget)
    -> output_append_status {
    // Cover the attribute-name construction, an existing-value comparison,
    // and the value copy performed by pugixml.
    budget.consume(prefix);
    budget.consume(namespace_uri);
    budget.consume(namespace_uri);
    auto attribute_name = std::string{"xmlns"};
    if (!prefix.empty()) {
        attribute_name.push_back(':');
        attribute_name.append(prefix);
    }
    if (const auto existing = output.attribute(attribute_name.c_str());
        existing != pugi::xml_attribute{}) {
        return std::string_view{existing.value()} == namespace_uri
                   ? output_append_status::success
                   : output_append_status::namespace_mismatch;
    }
    if (output_attribute_count >= maximum_output_attributes) {
        return output_append_status::output_limit;
    }
    ++output_attribute_count;
    auto attribute = output.append_attribute(attribute_name.c_str());
    if (attribute == pugi::xml_attribute{}) {
        return output_append_status::allocation_failure;
    }
    return attribute.set_value(std::string{namespace_uri}.c_str())
               ? output_append_status::success
               : output_append_status::allocation_failure;
}

auto append_selected_attribute(pugi::xml_node output,
                               pugi::xml_attribute source,
                               std::size_t &output_attribute_count,
                               std::size_t maximum_output_attributes)
    -> output_append_status {
    if (output_attribute_count >= maximum_output_attributes) {
        return output_append_status::output_limit;
    }
    ++output_attribute_count;
    auto attribute = output.append_attribute(source.name());
    if (attribute == pugi::xml_attribute{}) {
        return output_append_status::allocation_failure;
    }
    return attribute.set_value(source.value())
               ? output_append_status::success
               : output_append_status::allocation_failure;
}

auto append_non_element_node(pugi::xml_node output_parent,
                             pugi::xml_node source,
                             std::size_t &output_attribute_count,
                             std::size_t maximum_output_attributes)
    -> output_append_status {
    switch (source.type()) {
    case pugi::node_pcdata:
    case pugi::node_cdata:
    case pugi::node_comment: {
        auto output = output_parent.append_child(source.type());
        if (output == pugi::xml_node{}) {
            return output_append_status::allocation_failure;
        }
        return output.set_value(source.value())
                   ? output_append_status::success
                   : output_append_status::allocation_failure;
    }
    case pugi::node_pi: {
        auto output = output_parent.append_child(source.type());
        if (output == pugi::xml_node{} || !output.set_name(source.name()) ||
            !output.set_value(source.value())) {
            return output_append_status::allocation_failure;
        }
        return output_append_status::success;
    }
    case pugi::node_declaration: {
        auto output = output_parent.append_child(source.type());
        if (output == pugi::xml_node{} || !output.set_name(source.name())) {
            return output_append_status::allocation_failure;
        }
        for (auto attribute = source.first_attribute();
             attribute != pugi::xml_attribute{};
             attribute = attribute.next_attribute()) {
            if (output_attribute_count >= maximum_output_attributes) {
                return output_append_status::output_limit;
            }
            ++output_attribute_count;
            auto copied = output.append_attribute(attribute.name());
            if (copied == pugi::xml_attribute{} ||
                !copied.set_value(attribute.value())) {
                return output_append_status::allocation_failure;
            }
        }
        return output_append_status::success;
    }
    default:
        return source.type() == pugi::node_null
                   ? output_append_status::success
                   : output_append_status::namespace_mismatch;
    }
}

auto push_children_reverse(std::vector<processing_task> &tasks,
                           pugi::xml_node source_parent,
                           pugi::xml_node output_parent,
                           bool source_parent_was_copied) -> void {
    for (auto child = source_parent.last_child(); child != pugi::xml_node{};
         child = child.previous_sibling()) {
        tasks.push_back({child, output_parent, task_kind::process_node,
                         source_parent_was_copied});
    }
}

auto validate_all_mce_syntax(pugi::xml_document &document,
                             const package_relationships_mce_limits &limits,
                             namespace_work_budget &budget)
    -> package_relationships_mce_result {
    std::vector<syntax_validation_task> tasks;
    for (auto child = document.last_child(); child != pugi::xml_node{};
         child = child.previous_sibling()) {
        tasks.push_back({child, syntax_task_kind::enter_element, false});
    }

    xml_namespace_scope namespace_scope;
    mce_context context;
    std::size_t processed_elements = 0U;
    std::string detail;

    while (!tasks.empty()) {
        const auto task = tasks.back();
        tasks.pop_back();
        if (task.kind == syntax_task_kind::leave_element) {
            context.pop(budget);
            namespace_scope.leave_element();
            continue;
        }
        if (task.source.type() != pugi::node_element) {
            continue;
        }
        if (processed_elements >= limits.maximum_elements) {
            return failure(package_relationships_mce_status::invalid_mce_markup,
                           "Relationships MCE element count exceeds the "
                           "supported limit");
        }
        ++processed_elements;
        if (!namespace_scope.enter_element(task.source, detail)) {
            return failure(
                package_relationships_mce_status::invalid_namespace_markup,
                std::move(detail));
        }

        const auto element_name =
            namespace_scope.element_expanded_name(task.source);
        mce_frame frame;
        if (!build_mce_frame(task.source, namespace_scope, context, frame,
                             budget, detail)) {
            namespace_scope.leave_element();
            return failure(package_relationships_mce_status::invalid_mce_markup,
                           std::move(detail));
        }
        context.push(std::move(frame), budget);

        const auto fail_current = [&](package_relationships_mce_status status) {
            context.pop(budget);
            namespace_scope.leave_element();
            return failure(status, std::move(detail));
        };

        const auto must_understand_status = check_must_understand(
            task.source, namespace_scope, false, budget, detail);
        if (must_understand_status !=
            package_relationships_mce_status::success) {
            return fail_current(must_understand_status);
        }

        if (element_name.namespace_uri == markup_compatibility_namespace) {
            if (element_name.local_name == "AlternateContent") {
                const auto wrapper_status = validate_mce_wrapper_attributes(
                    task.source, "AlternateContent", namespace_scope, context,
                    context.frames.back(), budget, detail);
                if (wrapper_status !=
                    package_relationships_mce_status::success) {
                    return fail_current(wrapper_status);
                }
                pugi::xml_node selected;
                const auto selection_status = select_alternate_content_branch(
                    task.source, namespace_scope, context, selected, budget,
                    detail);
                if (selection_status !=
                    package_relationships_mce_status::success) {
                    return fail_current(selection_status);
                }
            } else if (element_name.local_name == "Choice" ||
                       element_name.local_name == "Fallback") {
                if (!task.parent_is_alternate_content) {
                    detail = "mc:" + std::string{element_name.local_name} +
                             " appears outside mc:AlternateContent";
                    return fail_current(
                        package_relationships_mce_status::invalid_mce_markup);
                }

                const auto wrapper_status = validate_mce_wrapper_attributes(
                    task.source, element_name.local_name, namespace_scope,
                    context, context.frames.back(), budget, detail);
                if (wrapper_status !=
                    package_relationships_mce_status::success) {
                    return fail_current(wrapper_status);
                }
                if (element_name.local_name == "Choice") {
                    std::vector<std::string> required_namespaces;
                    const auto requires_status = validate_choice_requires(
                        task.source, namespace_scope, required_namespaces,
                        budget, detail);
                    if (requires_status !=
                        package_relationships_mce_status::success) {
                        return fail_current(requires_status);
                    }
                }
            } else {
                detail = element_name.local_name == "PreserveElements" ||
                                 element_name.local_name == "PreserveAttributes"
                             ? "unsupported pre-5th-edition MCE Preserve* "
                               "element"
                             : "Relationships XML contains an unsupported "
                               "MCE element";
                return fail_current(
                    package_relationships_mce_status::invalid_mce_markup);
            }
        }

        tasks.push_back({task.source, syntax_task_kind::leave_element, false});
        const bool child_parent_is_alternate_content =
            element_name.namespace_uri == markup_compatibility_namespace &&
            element_name.local_name == "AlternateContent";
        for (auto child = task.source.last_child(); child != pugi::xml_node{};
             child = child.previous_sibling()) {
            tasks.push_back({child, syntax_task_kind::enter_element,
                             child_parent_is_alternate_content});
        }
    }

    return {};
}

} // namespace

package_relationships_mce_result
preprocess_package_relationships_mce(pugi::xml_document &document) {
    return preprocess_package_relationships_mce(
        document, package_relationships_mce_limits{});
}

static auto preprocess_package_relationships_mce_impl(
    pugi::xml_document &document,
    const package_relationships_mce_limits &limits)
    -> package_relationships_mce_result {
    namespace_work_budget namespace_budget{limits.maximum_namespace_work_bytes};
    const auto namespace_validation =
        validate_xml_namespace_well_formedness(document);
    if (!namespace_validation.valid) {
        return failure(
            package_relationships_mce_status::invalid_namespace_markup,
            namespace_validation.detail);
    }

    const auto syntax_validation =
        validate_all_mce_syntax(document, limits, namespace_budget);
    if (!syntax_validation) {
        return syntax_validation;
    }

    pugi::xml_document sanitized;
    std::vector<processing_task> tasks;
    for (auto child = document.last_child(); child != pugi::xml_node{};
         child = child.previous_sibling()) {
        tasks.push_back({child, sanitized, task_kind::process_node, false});
    }

    xml_namespace_scope namespace_scope;
    mce_context context;
    std::size_t processed_elements = 0U;
    std::size_t output_attribute_count = 0U;
    std::string detail;

    while (!tasks.empty()) {
        auto task = tasks.back();
        tasks.pop_back();
        if (task.kind == task_kind::leave_element) {
            context.pop(namespace_budget);
            namespace_scope.leave_element();
            continue;
        }
        if (task.source.type() != pugi::node_element) {
            const auto append_status = append_non_element_node(
                task.output_parent, task.source, output_attribute_count,
                limits.maximum_output_attributes);
            if (append_status == output_append_status::allocation_failure) {
                return allocation_failure();
            }
            if (append_status != output_append_status::success) {
                return failure(
                    package_relationships_mce_status::invalid_mce_markup,
                    "failed to construct sanitized Relationships XML");
            }
            continue;
        }
        if (processed_elements >= limits.maximum_elements) {
            return failure(package_relationships_mce_status::invalid_mce_markup,
                           "Relationships MCE element count exceeds the "
                           "supported limit");
        }
        ++processed_elements;
        if (!namespace_scope.enter_element(task.source, detail)) {
            return failure(
                package_relationships_mce_status::invalid_namespace_markup,
                std::move(detail));
        }

        const auto element_name =
            namespace_scope.element_expanded_name(task.source);
        const bool selected_wrapper =
            task.kind == task_kind::process_selected_wrapper;
        const bool is_relationships_element =
            element_name.namespace_uri == package_relationships_namespace;
        const bool is_mce_element =
            element_name.namespace_uri == markup_compatibility_namespace;

        // The element's own Ignorable/ProcessContent declarations participate
        // in marking that element. Parse them for conformance even when the
        // resulting mark discards the subtree; semantic MustUnderstand handling
        // remains below the discard decision.
        mce_frame frame;
        if (!build_mce_frame(task.source, namespace_scope, context, frame,
                             namespace_budget, detail)) {
            namespace_scope.leave_element();
            return failure(package_relationships_mce_status::invalid_mce_markup,
                           std::move(detail));
        }

        bool unwrap = selected_wrapper;
        if (!is_relationships_element && !is_mce_element && !selected_wrapper) {
            if (element_name.namespace_uri == xml_namespace_uri) {
                namespace_scope.leave_element();
                return failure(
                    package_relationships_mce_status::mismatch,
                    "Relationships XML contains an xml element that is not "
                    "declared by the OPC schema");
            }
            if (!namespace_is_locally_ignorable(element_name.namespace_uri,
                                                context, frame,
                                                namespace_budget)) {
                namespace_scope.leave_element();
                return failure(
                    package_relationships_mce_status::mismatch,
                    "Relationships XML contains a non-ignorable unsupported "
                    "element namespace");
            }
            if (!context.should_process_content(element_name.namespace_uri,
                                                element_name.local_name,
                                                namespace_budget) &&
                !frame_processes_content(frame, element_name.namespace_uri,
                                         element_name.local_name,
                                         namespace_budget)) {
                // A completely ignored subtree is namespace-validated, but its
                // MCE directives (including MustUnderstand) are not executed.
                namespace_scope.leave_element();
                continue;
            }
            unwrap = true;
        }

        if (is_mce_element && !selected_wrapper &&
            element_name.local_name != "AlternateContent") {
            namespace_scope.leave_element();
            return failure(
                package_relationships_mce_status::invalid_mce_markup,
                element_name.local_name == "PreserveElements" ||
                        element_name.local_name == "PreserveAttributes"
                    ? "unsupported pre-5th-edition MCE Preserve* element"
                    : "Relationships XML contains an MCE element outside "
                      "mc:AlternateContent");
        }

        context.push(std::move(frame), namespace_budget);
        const auto must_understand_status = check_must_understand(
            task.source, namespace_scope, true, namespace_budget, detail);
        if (must_understand_status !=
            package_relationships_mce_status::success) {
            context.pop(namespace_budget);
            namespace_scope.leave_element();
            return failure(must_understand_status, std::move(detail));
        }

        tasks.push_back({task.source, {}, task_kind::leave_element, false});

        if (is_mce_element && element_name.local_name == "AlternateContent") {
            const auto wrapper_status = validate_mce_wrapper_attributes(
                task.source, "AlternateContent", namespace_scope, context,
                context.frames.back(), namespace_budget, detail);
            if (wrapper_status != package_relationships_mce_status::success) {
                return failure(wrapper_status, std::move(detail));
            }
            pugi::xml_node selected;
            const auto selection_status = select_alternate_content_branch(
                task.source, namespace_scope, context, selected,
                namespace_budget, detail);
            if (selection_status != package_relationships_mce_status::success) {
                return failure(selection_status, std::move(detail));
            }
            if (selected != pugi::xml_node{}) {
                tasks.push_back({selected, task.output_parent,
                                 task_kind::process_selected_wrapper, false});
            }
            continue;
        }

        if (selected_wrapper) {
            if (!is_mce_element || (element_name.local_name != "Choice" &&
                                    element_name.local_name != "Fallback")) {
                return failure(
                    package_relationships_mce_status::invalid_mce_markup,
                    "invalid selected mc:AlternateContent branch wrapper");
            }
            const auto wrapper_status = validate_mce_wrapper_attributes(
                task.source, element_name.local_name, namespace_scope, context,
                context.frames.back(), namespace_budget, detail);
            if (wrapper_status != package_relationships_mce_status::success) {
                return failure(wrapper_status, std::move(detail));
            }
            push_children_reverse(tasks, task.source, task.output_parent,
                                  false);
            continue;
        }

        if (unwrap) {
            const auto wrapper_status =
                validate_process_content_wrapper_reserved_attributes(
                    task.source, namespace_scope, detail);
            if (wrapper_status != package_relationships_mce_status::success) {
                return failure(wrapper_status, std::move(detail));
            }
            push_children_reverse(tasks, task.source, task.output_parent,
                                  false);
            continue;
        }

        auto output_element =
            task.output_parent.append_child(task.source.name());
        if (output_element == pugi::xml_node{}) {
            return allocation_failure();
        }

        if (!task.source_parent_was_copied) {
            if (namespace_scope.effective_binding_count() >
                limits.maximum_hoisted_bindings_per_element) {
                return failure(
                    package_relationships_mce_status::invalid_mce_markup,
                    "Relationships MCE namespace binding hoist exceeds the "
                    "per-element limit");
            }
            for (const auto &[prefix, namespace_uri] :
                 sorted_effective_bindings(namespace_scope, namespace_budget)) {
                const auto append_status = append_namespace_binding(
                    output_element, prefix, namespace_uri,
                    output_attribute_count, limits.maximum_output_attributes,
                    namespace_budget);
                if (append_status == output_append_status::allocation_failure) {
                    return allocation_failure();
                }
                if (append_status != output_append_status::success) {
                    return failure(
                        package_relationships_mce_status::invalid_mce_markup,
                        "failed to preserve namespace bindings while "
                        "sanitizing Relationships XML");
                }
            }
        }

        for (auto attribute = task.source.first_attribute();
             attribute != pugi::xml_attribute{};
             attribute = attribute.next_attribute()) {
            if (xml_attribute_is_namespace_declaration(attribute)) {
                const auto qualified_name =
                    parse_xml_qualified_name(attribute.name());
                const auto prefix =
                    std::string_view{attribute.name()} == "xmlns"
                        ? std::string_view{}
                        : qualified_name.local_name;
                const auto append_status = append_namespace_binding(
                    output_element, prefix, attribute.value(),
                    output_attribute_count, limits.maximum_output_attributes,
                    namespace_budget);
                if (append_status == output_append_status::allocation_failure) {
                    return allocation_failure();
                }
                if (append_status != output_append_status::success) {
                    return failure(
                        package_relationships_mce_status::invalid_mce_markup,
                        "namespace binding changed while sanitizing "
                        "Relationships XML");
                }
                continue;
            }

            const auto attribute_name =
                namespace_scope.attribute_expanded_name(attribute);
            if (attribute_name.namespace_uri ==
                markup_compatibility_namespace) {
                continue;
            }
            if (!attribute_name.namespace_uri.empty() &&
                attribute_name.namespace_uri !=
                    package_relationships_namespace) {
                if (context.is_ignorable(attribute_name.namespace_uri,
                                         namespace_budget)) {
                    continue;
                }
                if (attribute_name.namespace_uri == xml_namespace_uri) {
                    return failure(
                        package_relationships_mce_status::mismatch,
                        "Relationships XML contains an XML namespace "
                        "attribute that is not declared by the OPC schema");
                }
                if (attribute_name.namespace_uri ==
                    xml_schema_instance_namespace) {
                    return failure(
                        package_relationships_mce_status::mismatch,
                        "Relationships XML contains an xsi attribute that is "
                        "not declared by the OPC schema");
                }
                return failure(
                    package_relationships_mce_status::mismatch,
                    "Relationships XML contains a non-ignorable unsupported "
                    "attribute namespace");
            }
            const auto append_status = append_selected_attribute(
                output_element, attribute, output_attribute_count,
                limits.maximum_output_attributes);
            if (append_status == output_append_status::allocation_failure) {
                return allocation_failure();
            }
            if (append_status != output_append_status::success) {
                return failure(
                    package_relationships_mce_status::invalid_mce_markup,
                    "failed to construct sanitized Relationships XML");
            }
        }
        push_children_reverse(tasks, task.source, output_element, true);
    }

    const auto sanitized_namespace_validation =
        validate_xml_namespace_well_formedness(sanitized);
    if (!sanitized_namespace_validation.valid) {
        return failure(
            package_relationships_mce_status::invalid_mce_markup,
            "sanitized Relationships XML changed a namespace binding");
    }

    document = std::move(sanitized);
    return {};
}

package_relationships_mce_result preprocess_package_relationships_mce(
    pugi::xml_document &document,
    const package_relationships_mce_limits &limits) {
    try {
        return preprocess_package_relationships_mce_impl(document, limits);
    } catch (const namespace_work_limit_exceeded &) {
        return namespace_resource_limit_failure();
    } catch (const std::bad_alloc &) {
        // Do not allocate a detail string while already handling allocation
        // failure. Callers still receive std::errc::not_enough_memory and the
        // package entry name from their own error context.
        return allocation_failure();
    }
}

std::error_code package_relationships_mce_error_code(
    package_relationships_mce_status status) noexcept {
    switch (status) {
    case package_relationships_mce_status::success:
        return {};
    case package_relationships_mce_status::invalid_namespace_markup:
    case package_relationships_mce_status::invalid_mce_markup:
        return featherdoc::make_error_code(
            featherdoc::document_errc::invalid_mce_markup);
    case package_relationships_mce_status::mismatch:
        return featherdoc::make_error_code(
            featherdoc::document_errc::mce_mismatch);
    case package_relationships_mce_status::resource_limit:
        return featherdoc::make_error_code(
            featherdoc::document_errc::archive_limit_exceeded);
    case package_relationships_mce_status::allocation_failure:
        return std::make_error_code(std::errc::not_enough_memory);
    }
    return featherdoc::make_error_code(
        featherdoc::document_errc::invalid_mce_markup);
}

std::error_code set_package_relationships_mce_last_error(
    featherdoc::document_error_info &error_info,
    const package_relationships_mce_result &result,
    std::string_view entry_name) {
    const auto code = package_relationships_mce_error_code(result.status);
    if (result.status == package_relationships_mce_status::allocation_failure) {
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

} // namespace featherdoc::detail
