#include "featherdoc_cli_command_support.hpp"

#include "featherdoc_cli_errors.hpp"

namespace featherdoc_cli {

auto save_document(featherdoc::Document &doc,
                   const std::optional<path_type> &output_path,
                   std::string_view command, bool json_output) -> bool {
    const auto error =
        output_path.has_value() ? doc.save_as(*output_path) : doc.save();
    if (error) {
        return report_document_error(command, "save", doc.last_error(), json_output);
    }

    return true;
}

auto open_document(const path_type &input_path, featherdoc::Document &doc,
                   std::string_view command, bool json_output) -> bool {
    doc.set_path(input_path);
    const auto error = doc.open();
    if (error) {
        return report_document_error(command, "open", doc.last_error(), json_output);
    }

    return true;
}

void write_json_mutation_result(std::string_view command, featherdoc::Document &doc,
                                const std::optional<path_type> &output_path) {
    write_json_mutation_result(command, doc, output_path,
                               [](std::ostream &) {});
}

} // namespace featherdoc_cli
