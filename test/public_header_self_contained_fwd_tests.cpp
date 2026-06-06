#include <featherdoc/fwd.hpp>

#include <type_traits>

void featherdoc_public_header_self_contained_fwd() {
    static_assert(std::is_class_v<featherdoc::Document>);
    static_assert(std::is_class_v<featherdoc::Paragraph>);
    static_assert(std::is_class_v<featherdoc::Run>);
    static_assert(std::is_class_v<featherdoc::Table>);
    static_assert(std::is_class_v<featherdoc::TemplatePart>);
    static_assert(std::is_enum_v<featherdoc::style_kind>);
    static_assert(std::is_enum_v<featherdoc::field_kind>);
    static_assert(std::is_enum_v<featherdoc::template_slot_kind>);
}
