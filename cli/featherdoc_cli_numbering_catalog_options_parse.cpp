#include "featherdoc_cli_numbering_catalog_options_parse.hpp"

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_export_numbering_catalog_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    export_numbering_catalog_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

auto parse_import_numbering_catalog_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    import_numbering_catalog_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--catalog-file") {
            if (options.catalog_path.has_value()) {
                error_message = "duplicate --catalog-file option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --catalog-file";
                return false;
            }

            options.catalog_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (!options.catalog_path.has_value()) {
        error_message = "missing --catalog-file <catalog.json>";
        return false;
    }

    return true;
}

auto parse_check_numbering_catalog_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    check_numbering_catalog_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--catalog-file") {
            if (options.catalog_path.has_value()) {
                error_message = "duplicate --catalog-file option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --catalog-file";
                return false;
            }

            options.catalog_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (!options.catalog_path.has_value()) {
        error_message = "missing --catalog-file <catalog.json>";
        return false;
    }

    return true;
}

auto parse_patch_numbering_catalog_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    patch_numbering_catalog_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--patch-file") {
            if (options.patch_path.has_value()) {
                error_message = "duplicate --patch-file option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --patch-file";
                return false;
            }

            options.patch_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (!options.patch_path.has_value()) {
        error_message = "missing --patch-file <patch.json>";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
