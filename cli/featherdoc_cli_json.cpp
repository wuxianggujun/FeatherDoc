#include "featherdoc_cli_json.hpp"

#include <featherdoc/detail/utf8.hpp>

#include <ostream>

namespace featherdoc_cli {

namespace {

void append_json_hex_byte_escape(std::string &escaped, unsigned char byte) {
    escaped += "\\\\x";
    escaped.push_back(featherdoc::detail::hex_digit_upper(
        static_cast<unsigned char>(byte >> 4U)));
    escaped.push_back(featherdoc::detail::hex_digit_upper(
        static_cast<unsigned char>(byte & 0x0FU)));
}

void append_json_unicode_control_escape(std::string &escaped,
                                        unsigned char byte) {
    escaped += "\\u00";
    escaped.push_back(featherdoc::detail::hex_digit_upper(
        static_cast<unsigned char>(byte >> 4U)));
    escaped.push_back(featherdoc::detail::hex_digit_upper(
        static_cast<unsigned char>(byte & 0x0FU)));
}

} // namespace

auto json_escape(std::string_view text) -> std::string {
    std::string escaped;
    escaped.reserve(text.size());

    std::size_t index = 0U;
    while (index < text.size()) {
        const auto decoded = featherdoc::detail::decode_next_utf8(text, index);
        if (!decoded.valid || decoded.length == 0U) {
            const auto invalid_length = decoded.length == 0U ? 1U : decoded.length;
            const auto bounded_length =
                invalid_length > text.size() - index ? text.size() - index
                                                     : invalid_length;
            for (std::size_t offset = 0U; offset < bounded_length; ++offset) {
                append_json_hex_byte_escape(
                    escaped,
                    static_cast<unsigned char>(text[index + offset]));
            }
            index += bounded_length;
            continue;
        }

        if (decoded.length != 1U) {
            escaped.append(text.substr(index, decoded.length));
            index += decoded.length;
            continue;
        }

        const auto ch = text[index];
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
            if (static_cast<unsigned char>(ch) < 0x20U) {
                append_json_unicode_control_escape(
                    escaped, static_cast<unsigned char>(ch));
            } else {
                escaped += ch;
            }
            break;
        }
        ++index;
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
