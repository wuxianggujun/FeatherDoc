#include <featherdoc/fwd.hpp>
#include <featherdoc/core.hpp>

#include <type_traits>

int main() {
    static_assert(std::is_class_v<featherdoc::Document>);
    static_assert(std::is_class_v<featherdoc::Paragraph>);
    static_assert(std::is_class_v<featherdoc::Run>);
    static_assert(std::is_class_v<featherdoc::Table>);
    static_assert(std::is_enum_v<featherdoc::style_kind>);
    static_assert(std::is_enum_v<featherdoc::template_schema_part_kind>);

    featherdoc::document_error_info error_info{};
    featherdoc::document_semantic_diff_options diff_options{};
    featherdoc::template_schema schema{};
    featherdoc::Document doc;

    (void)error_info;
    (void)diff_options;
    (void)schema;
    (void)doc;
    return 0;
}
