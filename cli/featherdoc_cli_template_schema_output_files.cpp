#include "featherdoc_cli_template_schema_output.hpp"

#include "featherdoc_cli_template_schema_patch_output.hpp"

#include <featherdoc.hpp>

#include <fstream>
#include <string>

namespace featherdoc_cli {

auto write_exported_template_schema_file(
    const path_type &output_path, const exported_template_schema_result &result,
    std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message = "failed to open schema output path: " + output_path.string();
        return false;
    }

    write_json_exported_template_schema(stream, result);
    if (!stream.good()) {
        error_message = "failed to write schema output path: " + output_path.string();
        return false;
    }

    return true;
}

auto write_template_schema_patch_file(const path_type &output_path,
                                      const template_schema_patch_document &patch,
                                      std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message = "failed to open patch output path: " + output_path.string();
        return false;
    }

    write_json_template_schema_patch_document(stream, patch);
    if (!stream.good()) {
        error_message = "failed to write patch output path: " + output_path.string();
        return false;
    }

    return true;
}

auto write_template_schema_patch_review_file(
    const path_type &output_path,
    const featherdoc::template_schema_patch_review_summary &summary,
    std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message =
            "failed to open schema patch review output path: " + output_path.string();
        return false;
    }

    featherdoc::write_template_schema_patch_review_json(stream, summary);
    stream << '\n';
    if (!stream.good()) {
        error_message =
            "failed to write schema patch review output path: " + output_path.string();
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
