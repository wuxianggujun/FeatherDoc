#include "featherdoc_cli_package_part.hpp"

#include "../src/document_archive_limit_helpers.hpp"

namespace featherdoc_cli {

auto package_part_name_identity(std::string_view part_name)
    -> std::optional<std::string> {
    return featherdoc::detail::package_part_name_identity(part_name);
}

auto package_part_names_equivalent(std::string_view left,
                                   std::string_view right) -> bool {
    return featherdoc::detail::package_part_names_equivalent(left, right);
}

} // namespace featherdoc_cli
