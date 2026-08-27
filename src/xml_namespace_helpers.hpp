#ifndef FEATHERDOC_XML_NAMESPACE_HELPERS_HPP
#define FEATHERDOC_XML_NAMESPACE_HELPERS_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <pugixml.hpp>

namespace featherdoc::detail {

inline constexpr auto xml_namespace_uri =
    std::string_view{"http://www.w3.org/XML/1998/namespace"};
inline constexpr auto xmlns_namespace_uri =
    std::string_view{"http://www.w3.org/2000/xmlns/"};

struct xml_qualified_name_view final {
    std::string_view prefix;
    std::string_view local_name;
    bool valid{false};
};

struct xml_expanded_name_view final {
    std::string_view namespace_uri;
    std::string_view local_name;
    bool valid{false};
};

struct xml_namespace_validation_result final {
    bool valid{false};
    std::string detail;
};

// Enables allocation-free lookup of std::string keys by std::string_view.
// Keeping these operations noexcept is important because namespace resolution
// is used inside failure-reporting paths that must never terminate on OOM.
struct transparent_string_hash final {
    using is_transparent = void;

    [[nodiscard]] std::size_t
    operator()(std::string_view value) const noexcept {
        return std::hash<std::string_view>{}(value);
    }
};

struct transparent_string_equal final {
    using is_transparent = void;

    [[nodiscard]] bool operator()(std::string_view left,
                                  std::string_view right) const noexcept {
        return left == right;
    }
};

[[nodiscard]] xml_qualified_name_view
parse_xml_qualified_name(std::string_view name) noexcept;
[[nodiscard]] bool
xml_attribute_is_namespace_declaration(pugi::xml_attribute attribute) noexcept;
[[nodiscard]] std::string_view
resolve_xml_namespace_uri(pugi::xml_node node, std::string_view prefix);
[[nodiscard]] bool
xml_element_has_expanded_name(pugi::xml_node node, std::string_view local_name,
                              std::string_view namespace_uri);

class xml_namespace_scope final {
  public:
    explicit xml_namespace_scope(std::size_t maximum_depth = 1024U);

    [[nodiscard]] bool enter_element(pugi::xml_node element,
                                     std::string &detail);
    void leave_element() noexcept;

    [[nodiscard]] std::string_view
    resolve_prefix(std::string_view prefix) const noexcept;
    [[nodiscard]] xml_expanded_name_view
    element_expanded_name(pugi::xml_node element) const noexcept;
    [[nodiscard]] xml_expanded_name_view
    attribute_expanded_name(pugi::xml_attribute attribute) const noexcept;
    [[nodiscard]]
    std::vector<std::pair<std::string_view, std::string_view>>
    effective_bindings() const;
    [[nodiscard]] std::size_t effective_binding_count() const noexcept;
    [[nodiscard]] std::size_t depth() const noexcept;

  private:
    std::size_t maximum_depth_;
    std::size_t depth_{0U};
    std::unordered_map<std::string, std::vector<std::string>,
                       transparent_string_hash, transparent_string_equal>
        bindings_;
    std::vector<std::vector<std::string>> declaration_frames_;
};

[[nodiscard]] xml_namespace_validation_result
validate_xml_namespace_well_formedness(
    const pugi::xml_document &document, std::size_t maximum_depth = 1024U,
    std::size_t maximum_elements = 1'000'000U);

} // namespace featherdoc::detail

#endif
