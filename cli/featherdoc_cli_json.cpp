#include "featherdoc_cli_json.hpp"

#include <ostream>

namespace featherdoc_cli {

auto json_escape(std::string_view text) -> std::string {
    std::string escaped;
    escaped.reserve(text.size());
    for (const char ch : text) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\b':
            escaped += "\\b";
            break;
        case '\f':
            escaped += "\\f";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped += ch;
            break;
        }
    }
    return escaped;
}

void write_json_string(std::ostream &stream, std::string_view text) {
    stream << '"' << json_escape(text) << '"';
}

} // namespace featherdoc_cli
