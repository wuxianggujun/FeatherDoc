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

void write_json_size_array(std::ostream &stream,
                           const std::vector<std::size_t> &values) {
    stream << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0U) {
            stream << ',';
        }
        stream << values[index];
    }
    stream << ']';
}

void write_json_strings(std::ostream &stream,
                        const std::vector<std::string> &values) {
    stream << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_string(stream, values[index]);
    }
    stream << ']';
}

void write_json_lines(std::ostream &stream,
                      const std::vector<std::string> &lines) {
    write_json_strings(stream, lines);
}

void write_json_optional_string(std::ostream &stream,
                                const std::optional<std::string> &value) {
    if (value.has_value()) {
        write_json_string(stream, *value);
        return;
    }

    stream << "null";
}

void write_json_optional_u32(std::ostream &stream,
                             const std::optional<std::uint32_t> &value) {
    if (value.has_value()) {
        stream << *value;
        return;
    }

    stream << "null";
}

void write_json_optional_double(std::ostream &stream,
                                const std::optional<double> &value) {
    if (value.has_value()) {
        stream << *value;
        return;
    }

    stream << "null";
}

void write_json_optional_bool(std::ostream &stream,
                              const std::optional<bool> &value) {
    if (value.has_value()) {
        stream << (*value ? "true" : "false");
        return;
    }

    stream << "null";
}

void write_json_optional_size(std::ostream &stream,
                              const std::optional<std::size_t> &value) {
    if (value.has_value()) {
        stream << *value;
        return;
    }

    stream << "null";
}

void write_json_optional_u32_value(
    std::ostream &stream, const std::optional<std::uint32_t> &value) {
    write_json_optional_u32(stream, value);
}

void write_json_optional_bool_value(std::ostream &stream,
                                    const std::optional<bool> &value) {
    write_json_optional_bool(stream, value);
}

} // namespace featherdoc_cli
