#pragma once

#include <featherdoc/detail/utf8.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc::detail {

namespace package_path_detail {

[[nodiscard]] inline auto split_normalized_package_path(std::string_view path)
    -> std::vector<std::string> {
    std::vector<std::string> segments;
    std::size_t segment_begin = 0U;
    while (segment_begin <= path.size()) {
        const auto separator = path.find('/', segment_begin);
        const auto segment_end =
            separator == std::string_view::npos ? path.size() : separator;
        const auto segment =
            path.substr(segment_begin, segment_end - segment_begin);
        if (!segment.empty() && segment != ".") {
            if (segment == ".." && !segments.empty() &&
                segments.back() != "..") {
                segments.pop_back();
            } else {
                segments.emplace_back(segment);
            }
        }

        if (separator == std::string_view::npos) {
            break;
        }
        segment_begin = separator + 1U;
    }
    return segments;
}

[[nodiscard]] inline auto
join_package_path_segments(const std::vector<std::string> &segments)
    -> std::string {
    std::size_t output_size = segments.empty() ? 0U : segments.size() - 1U;
    for (const auto &segment : segments) {
        output_size += segment.size();
    }

    std::string output;
    output.reserve(output_size);
    for (const auto &segment : segments) {
        if (!output.empty()) {
            output.push_back('/');
        }
        output += segment;
    }
    return output;
}

} // namespace package_path_detail

// This normalizer is for already trusted package part names produced by the
// library. Relationship Target attributes are untrusted URI references and
// must go through resolve_internal_package_relationship_target() below.
[[nodiscard]] inline auto normalize_package_path(std::string_view path)
    -> std::string {
    if (path.empty()) {
        return {};
    }

    std::string slash_normalized{path};
    for (auto &character : slash_normalized) {
        if (character == '\\') {
            character = '/';
        }
    }
    std::size_t first_relative_byte = 0U;
    while (first_relative_byte < slash_normalized.size() &&
           slash_normalized[first_relative_byte] == '/') {
        ++first_relative_byte;
    }
    const auto relative_path =
        std::string_view{slash_normalized}.substr(first_relative_byte);

    auto normalized = package_path_detail::join_package_path_segments(
        package_path_detail::split_normalized_package_path(relative_path));
    if (normalized.empty() && !relative_path.empty()) {
        return ".";
    }
    return normalized;
}

[[nodiscard]] inline auto package_path_parent(std::string_view path)
    -> std::string {
    const auto normalized = normalize_package_path(path);
    const auto separator = normalized.rfind('/');
    return separator == std::string::npos ? std::string{}
                                          : normalized.substr(0U, separator);
}

[[nodiscard]] inline auto package_path_filename(std::string_view path)
    -> std::string {
    const auto normalized = normalize_package_path(path);
    const auto separator = normalized.rfind('/');
    return separator == std::string::npos ? normalized
                                          : normalized.substr(separator + 1U);
}

enum class package_relationship_target_error : std::uint8_t {
    none = 0U,
    empty,
    invalid_utf8,
    control_character,
    space_character,
    backslash_separator,
    authority_reference,
    absolute_uri,
    query_or_fragment,
    invalid_percent_encoding,
    encoded_path_delimiter,
    invalid_path_character,
    empty_path_segment,
    dot_only_path_segment,
    dot_terminated_path_segment,
    trailing_directory,
    escapes_package_root,
};

struct package_relationship_target_resolution {
    std::string entry_name;
    package_relationship_target_error error{
        package_relationship_target_error::none};

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == package_relationship_target_error::none;
    }
};

[[nodiscard]] inline auto package_relationship_target_error_message(
    package_relationship_target_error error) -> std::string_view {
    switch (error) {
    case package_relationship_target_error::none:
        return {};
    case package_relationship_target_error::empty:
        return "relationship Target is empty";
    case package_relationship_target_error::invalid_utf8:
        return "relationship Target is not valid UTF-8";
    case package_relationship_target_error::control_character:
        return "relationship Target contains a control character";
    case package_relationship_target_error::space_character:
        return "relationship Target contains an unescaped space";
    case package_relationship_target_error::backslash_separator:
        return "relationship Target contains a backslash separator";
    case package_relationship_target_error::authority_reference:
        return "relationship Target contains a URI authority";
    case package_relationship_target_error::absolute_uri:
        return "relationship Target is an absolute URI";
    case package_relationship_target_error::query_or_fragment:
        return "relationship Target contains a query or fragment";
    case package_relationship_target_error::invalid_percent_encoding:
        return "relationship Target contains a non-canonical percent encoding";
    case package_relationship_target_error::encoded_path_delimiter:
        return "relationship Target percent-encodes a path delimiter or "
               "control byte";
    case package_relationship_target_error::invalid_path_character:
        return "relationship Target contains a character that is not valid "
               "in an OPC part name";
    case package_relationship_target_error::empty_path_segment:
        return "relationship Target contains an empty path segment";
    case package_relationship_target_error::dot_only_path_segment:
        return "relationship Target contains a path segment made only of "
               "dots";
    case package_relationship_target_error::dot_terminated_path_segment:
        return "relationship Target contains a path segment that ends with "
               "a dot";
    case package_relationship_target_error::trailing_directory:
        return "relationship Target resolves to a directory";
    case package_relationship_target_error::escapes_package_root:
        return "relationship Target escapes the package root";
    }
    return "relationship Target is invalid";
}

[[nodiscard]] inline auto package_path_hex_value(char character)
    -> std::optional<unsigned char> {
    const auto byte = static_cast<unsigned char>(character);
    if (byte >= '0' && byte <= '9') {
        return static_cast<unsigned char>(byte - '0');
    }
    if (byte >= 'A' && byte <= 'F') {
        return static_cast<unsigned char>(byte - 'A' + 10U);
    }
    if (byte >= 'a' && byte <= 'f') {
        return static_cast<unsigned char>(byte - 'a' + 10U);
    }
    return std::nullopt;
}

[[nodiscard]] inline auto package_path_byte_is_unreserved(unsigned char byte)
    -> bool {
    return (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') ||
           (byte >= '0' && byte <= '9') || byte == '-' || byte == '.' ||
           byte == '_' || byte == '~';
}

[[nodiscard]] inline auto package_path_byte_is_sub_delimiter(unsigned char byte)
    -> bool {
    switch (byte) {
    case '!':
    case '$':
    case '&':
    case '\'':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case ';':
    case '=':
        return true;
    default:
        return false;
    }
}

[[nodiscard]] inline auto package_path_byte_is_raw_pchar(unsigned char byte)
    -> bool {
    return byte >= 0x80U || package_path_byte_is_unreserved(byte) ||
           package_path_byte_is_sub_delimiter(byte) || byte == ':' ||
           byte == '@';
}

[[nodiscard]] inline auto package_path_upper_hex_digit(unsigned char value)
    -> char {
    return static_cast<char>(value < 10U ? '0' + value : 'A' + (value - 10U));
}

inline void append_percent_encoded_package_path_byte(std::string &output,
                                                     unsigned char byte) {
    output.push_back('%');
    output.push_back(package_path_upper_hex_digit(byte >> 4U));
    output.push_back(package_path_upper_hex_digit(byte & 0x0FU));
}

[[nodiscard]] inline auto canonicalize_package_iri_path(std::string_view path,
                                                        bool allow_raw_space)
    -> package_relationship_target_resolution {
    const auto fail = [](package_relationship_target_error error) {
        return package_relationship_target_resolution{{}, error};
    };

    if (!featherdoc::detail::is_valid_utf8(path)) {
        return fail(package_relationship_target_error::invalid_utf8);
    }
    if (path.find('\\') != std::string_view::npos) {
        return fail(package_relationship_target_error::backslash_separator);
    }
    if (path.find_first_of("?#") != std::string_view::npos) {
        return fail(package_relationship_target_error::query_or_fragment);
    }

    std::string decoded_path;
    decoded_path.reserve(path.size());
    std::string canonical_path;
    canonical_path.reserve(path.size());
    for (std::size_t index = 0U; index < path.size(); ++index) {
        const auto byte = static_cast<unsigned char>(path[index]);
        if (byte < 0x20U || byte == 0x7FU) {
            return fail(package_relationship_target_error::control_character);
        }
        if (byte == ' ') {
            if (!allow_raw_space) {
                return fail(package_relationship_target_error::space_character);
            }
            decoded_path.push_back(' ');
            append_percent_encoded_package_path_byte(canonical_path, byte);
            continue;
        }
        if (byte != '%') {
            if (byte != '/' && !package_path_byte_is_raw_pchar(byte)) {
                return fail(
                    package_relationship_target_error::invalid_path_character);
            }
            decoded_path.push_back(path[index]);
            if (byte >= 0x80U) {
                append_percent_encoded_package_path_byte(canonical_path, byte);
            } else {
                canonical_path.push_back(path[index]);
            }
            continue;
        }
        if (index + 2U >= path.size()) {
            return fail(
                package_relationship_target_error::invalid_percent_encoding);
        }
        const auto high = package_path_hex_value(path[index + 1U]);
        const auto low = package_path_hex_value(path[index + 2U]);
        if (!high.has_value() || !low.has_value()) {
            return fail(
                package_relationship_target_error::invalid_percent_encoding);
        }
        const auto decoded = static_cast<unsigned char>((*high << 4U) | *low);
        if (decoded == '/' || decoded == '\\' || decoded < 0x20U ||
            decoded == 0x7FU) {
            return fail(
                package_relationship_target_error::encoded_path_delimiter);
        }
        if (package_path_byte_is_unreserved(decoded)) {
            return fail(
                package_relationship_target_error::invalid_percent_encoding);
        }
        decoded_path.push_back(static_cast<char>(decoded));
        append_percent_encoded_package_path_byte(canonical_path, decoded);
        index += 2U;
    }

    if (!featherdoc::detail::is_valid_utf8(decoded_path)) {
        return fail(package_relationship_target_error::invalid_utf8);
    }
    for (std::size_t index = 0U; index + 1U < decoded_path.size(); ++index) {
        if (static_cast<unsigned char>(decoded_path[index]) == 0xC2U) {
            const auto next =
                static_cast<unsigned char>(decoded_path[index + 1U]);
            if (next >= 0x80U && next <= 0x9FU) {
                return fail(
                    package_relationship_target_error::control_character);
            }
        }
    }

    return {std::move(canonical_path), package_relationship_target_error::none};
}

// Validates the path-segment rules shared by all OPC PartName spellings.
// The input must already be URI-canonicalized by
// canonicalize_package_iri_path(). A single leading slash is accepted for an
// absolute PartName; a trailing slash is accepted only for a physical ZIP
// directory entry, which is not itself a package part.
[[nodiscard]] inline auto
validate_canonical_package_part_path(std::string_view canonical_path,
                                     bool allow_trailing_directory = false)
    -> package_relationship_target_error {
    if (canonical_path.empty()) {
        return package_relationship_target_error::empty;
    }
    if (canonical_path.front() == '/') {
        canonical_path.remove_prefix(1U);
        if (canonical_path.empty() || canonical_path.front() == '/') {
            return package_relationship_target_error::empty_path_segment;
        }
    }

    std::size_t segment_begin = 0U;
    while (segment_begin <= canonical_path.size()) {
        const auto separator = canonical_path.find('/', segment_begin);
        const auto segment_end = separator == std::string_view::npos
                                     ? canonical_path.size()
                                     : separator;
        const auto segment =
            canonical_path.substr(segment_begin, segment_end - segment_begin);
        if (segment.empty()) {
            if (allow_trailing_directory &&
                segment_begin == canonical_path.size() &&
                canonical_path.back() == '/') {
                return package_relationship_target_error::none;
            }
            return package_relationship_target_error::empty_path_segment;
        }

        const bool contains_non_dot =
            segment.find_first_not_of('.') != std::string_view::npos;
        if (!contains_non_dot) {
            return package_relationship_target_error::dot_only_path_segment;
        }
        if (segment.back() == '.') {
            return package_relationship_target_error::
                dot_terminated_path_segment;
        }

        if (separator == std::string_view::npos) {
            break;
        }
        segment_begin = separator + 1U;
    }
    return package_relationship_target_error::none;
}

[[nodiscard]] inline auto
resolve_internal_package_relationship_target(std::string_view source_part_name,
                                             std::string_view target)
    -> package_relationship_target_resolution {
    const auto fail = [](package_relationship_target_error error) {
        return package_relationship_target_resolution{{}, error};
    };

    if (target.empty()) {
        return fail(package_relationship_target_error::empty);
    }
    if (target.starts_with("//")) {
        return fail(package_relationship_target_error::authority_reference);
    }

    const auto canonical_target = canonicalize_package_iri_path(target, false);
    if (!canonical_target) {
        return fail(canonical_target.error);
    }
    const auto canonical_target_view =
        std::string_view{canonical_target.entry_name};

    const auto path_without_leading_slash =
        canonical_target_view.front() == '/' ? canonical_target_view.substr(1U)
                                             : canonical_target_view;
    if (path_without_leading_slash.empty()) {
        return fail(package_relationship_target_error::trailing_directory);
    }
    if (path_without_leading_slash.back() == '/') {
        return fail(package_relationship_target_error::trailing_directory);
    }

    const auto first_separator = path_without_leading_slash.find('/');
    const auto first_colon = path_without_leading_slash.find(':');
    if (first_colon != std::string_view::npos &&
        (first_separator == std::string_view::npos ||
         first_colon < first_separator)) {
        return fail(package_relationship_target_error::absolute_uri);
    }

    const auto canonical_source =
        source_part_name.empty()
            ? package_relationship_target_resolution{}
            : canonicalize_package_iri_path(source_part_name, true);
    if (!source_part_name.empty() && !canonical_source) {
        return fail(canonical_source.error);
    }
    auto resolved_segments = package_path_detail::split_normalized_package_path(
        canonical_target_view.front() == '/'
            ? std::string_view{}
            : package_path_parent(canonical_source.entry_name));
    std::size_t segment_begin = 0U;
    while (segment_begin <= path_without_leading_slash.size()) {
        const auto separator =
            path_without_leading_slash.find('/', segment_begin);
        const auto segment_end = separator == std::string_view::npos
                                     ? path_without_leading_slash.size()
                                     : separator;
        const auto segment = path_without_leading_slash.substr(
            segment_begin, segment_end - segment_begin);
        if (segment.empty()) {
            return fail(package_relationship_target_error::empty_path_segment);
        }
        if (segment == "..") {
            if (resolved_segments.empty()) {
                return fail(
                    package_relationship_target_error::escapes_package_root);
            }
            resolved_segments.pop_back();
        } else if (segment != ".") {
            resolved_segments.emplace_back(segment);
        }

        if (separator == std::string_view::npos) {
            if (segment == "." || segment == "..") {
                return fail(
                    package_relationship_target_error::trailing_directory);
            }
            break;
        }
        segment_begin = separator + 1U;
    }

    if (resolved_segments.empty()) {
        return fail(package_relationship_target_error::trailing_directory);
    }
    auto resolved_path =
        package_path_detail::join_package_path_segments(resolved_segments);
    const auto part_name_error =
        validate_canonical_package_part_path(resolved_path);
    if (part_name_error != package_relationship_target_error::none) {
        return fail(part_name_error);
    }
    return {std::move(resolved_path), package_relationship_target_error::none};
}

[[nodiscard]] inline auto
resolve_package_relationship_target(std::string_view source_part_name,
                                    std::string_view target) -> std::string {
    const auto resolved =
        resolve_internal_package_relationship_target(source_part_name, target);
    return resolved ? resolved.entry_name : std::string{};
}

// Validate the RFC media-type syntax used by [Content_Types].xml without
// imposing an application-specific or IANA allowlist. The type, subtype and
// unquoted parameter tokens are intentionally ASCII. Quoted values follow the
// OPC ST_ContentType boundary of Basic Latin plus U+00A0-U+00FF and never
// admit control characters or malformed escapes.
[[nodiscard]] inline auto
content_type_media_type_is_valid(std::string_view media_type) -> bool {
    if (media_type.empty() || !featherdoc::detail::is_valid_utf8(media_type)) {
        return false;
    }

    // Reject Unicode C1 controls as well as the ASCII controls handled below.
    for (std::size_t index = 0U; index + 1U < media_type.size(); ++index) {
        if (static_cast<unsigned char>(media_type[index]) != 0xC2U) {
            continue;
        }
        const auto next = static_cast<unsigned char>(media_type[index + 1U]);
        if (next >= 0x80U && next <= 0x9FU) {
            return false;
        }
    }

    const auto is_token_character = [](unsigned char byte) {
        if ((byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') ||
            (byte >= '0' && byte <= '9')) {
            return true;
        }
        return std::string_view{"!#$%&'*+-.^_`|~"}.find(
                   static_cast<char>(byte)) != std::string_view::npos;
    };
    const auto consume_token = [&](std::size_t &index) {
        const auto begin = index;
        while (
            index < media_type.size() &&
            is_token_character(static_cast<unsigned char>(media_type[index]))) {
            ++index;
        }
        return index != begin;
    };
    const auto consume_optional_whitespace = [&](std::size_t &index) {
        while (index < media_type.size() &&
               (media_type[index] == ' ' || media_type[index] == '\t')) {
            ++index;
        }
    };
    const auto consume_quoted_string = [&](std::size_t &index) {
        if (index >= media_type.size() || media_type[index] != '"') {
            return false;
        }
        ++index;
        while (index < media_type.size()) {
            const auto decoded =
                featherdoc::detail::decode_next_utf8(media_type, index);
            if (!decoded.valid || decoded.length == 0U) {
                return false;
            }
            const auto code_point = decoded.code_point;
            index += decoded.length;
            if (code_point == '"') {
                return true;
            }
            if (code_point == '\\') {
                if (index >= media_type.size()) {
                    return false;
                }
                const auto escaped =
                    featherdoc::detail::decode_next_utf8(media_type, index);
                if (!escaped.valid || escaped.length == 0U) {
                    return false;
                }
                if (escaped.code_point == '\t' || escaped.code_point == ' ' ||
                    (escaped.code_point >= 0x21U &&
                     escaped.code_point <= 0x7EU)) {
                    index += escaped.length;
                    continue;
                }
                return false;
            }
            if (code_point == '\t' || code_point == ' ' ||
                (code_point >= 0x21U && code_point <= 0x7EU) ||
                (code_point >= 0xA0U && code_point <= 0xFFU)) {
                continue;
            }
            return false;
        }
        return false;
    };

    std::size_t index = 0U;
    if (!consume_token(index) || index >= media_type.size() ||
        media_type[index] != '/') {
        return false;
    }
    ++index;
    if (!consume_token(index)) {
        return false;
    }
    if (index == media_type.size()) {
        return true;
    }

    while (index < media_type.size()) {
        consume_optional_whitespace(index);
        // OWS is only valid when it introduces another parameter; trailing
        // whitespace is not part of the media-type grammar.
        if (index >= media_type.size() || media_type[index] != ';') {
            return false;
        }
        ++index;
        consume_optional_whitespace(index);
        if (!consume_token(index) || index >= media_type.size() ||
            media_type[index] != '=') {
            return false;
        }
        ++index;
        if (index >= media_type.size()) {
            return false;
        }
        if (media_type[index] == '"') {
            if (!consume_quoted_string(index)) {
                return false;
            }
        } else if (!consume_token(index)) {
            return false;
        }
    }
    return true;
}

// [Content_Types].xml Default/@Extension values are case-insensitive OPC URI
// extension segments. Return a canonical comparison key for validation and
// duplicate detection.
[[nodiscard]] inline auto
content_type_extension_identity(std::string_view extension)
    -> std::optional<std::string> {
    if (extension.empty() || extension.front() == '.') {
        return std::nullopt;
    }

    // ST_Extension is an XML Schema lexical type, not an IRI. Non-ASCII
    // UTF-8 bytes therefore have to be represented explicitly as %HH bytes
    // in [Content_Types].xml; silently converting raw Unicode here would make
    // strict validation accept markup that does not match the OPC schema.
    for (const auto character : extension) {
        if (static_cast<unsigned char>(character) >= 0x80U) {
            return std::nullopt;
        }
    }

    // ST_Extension is a single, non-empty part-extension segment. Normalize
    // its percent escapes first, then enforce the OPC schema's character
    // repertoire. This preserves valid URI sub-delimiters and percent-encoded
    // Unicode while rejecting path separators and ambiguous delimiter
    // encodings through canonicalize_package_iri_path().
    const auto canonical = canonicalize_package_iri_path(extension, false);
    if (!canonical || canonical.entry_name.empty() ||
        canonical.entry_name.find('/') != std::string::npos) {
        return std::nullopt;
    }

    const auto is_ascii_alphanumeric = [](unsigned char byte) {
        return (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') ||
               (byte >= '0' && byte <= '9');
    };
    constexpr auto allowed_punctuation = std::string_view{"!$&'()*+,:=@-_~"};
    const auto canonical_extension = std::string_view{canonical.entry_name};
    for (std::size_t index = 0U; index < canonical_extension.size(); ++index) {
        const auto byte =
            static_cast<unsigned char>(canonical_extension[index]);
        if (is_ascii_alphanumeric(byte) ||
            allowed_punctuation.find(static_cast<char>(byte)) !=
                std::string_view::npos) {
            continue;
        }
        if (byte == '%' && index + 2U < canonical_extension.size() &&
            package_path_hex_value(canonical_extension[index + 1U])
                .has_value() &&
            package_path_hex_value(canonical_extension[index + 2U])
                .has_value()) {
            index += 2U;
            continue;
        }
        return std::nullopt;
    }

    std::string identity{canonical_extension};
    for (auto &character : identity) {
        const auto byte = static_cast<unsigned char>(character);
        if (byte >= 'A' && byte <= 'Z') {
            character = static_cast<char>(byte + ('a' - 'A'));
        }
    }
    return identity;
}

// [Content_Types].xml Override/@PartName values are absolute package part
// names. Compare their canonical URI spellings instead of their raw XML text:
// raw UTF-8 and percent-encoded UTF-8, percent-escape hex case, and ASCII path
// case all identify the same OPC part.
[[nodiscard]] inline auto
content_type_part_name_identity(std::string_view part_name)
    -> std::optional<std::string> {
    if (part_name.empty() || part_name.front() != '/' ||
        part_name.starts_with("//")) {
        return std::nullopt;
    }

    const auto canonical = canonicalize_package_iri_path(part_name, false);
    if (!canonical || canonical.entry_name.size() <= 1U ||
        canonical.entry_name.back() == '/') {
        return std::nullopt;
    }

    if (validate_canonical_package_part_path(canonical.entry_name) !=
        package_relationship_target_error::none) {
        return std::nullopt;
    }

    const auto relative_name =
        std::string_view{canonical.entry_name}.substr(1U);
    std::string identity{relative_name};
    for (auto &character : identity) {
        const auto byte = static_cast<unsigned char>(character);
        if (byte >= 'A' && byte <= 'Z') {
            character = static_cast<char>(byte + ('a' - 'A'));
        }
    }
    return identity;
}

[[nodiscard]] inline auto
content_type_part_names_equivalent(std::string_view left,
                                   std::string_view right) -> bool {
    const auto left_identity = content_type_part_name_identity(left);
    const auto right_identity = content_type_part_name_identity(right);
    return left_identity.has_value() && right_identity.has_value() &&
           *left_identity == *right_identity;
}

[[nodiscard]] inline auto
make_package_content_type_part_name(std::string_view entry_name)
    -> std::string {
    if (entry_name.empty()) {
        return {};
    }

    const auto canonical_entry =
        canonicalize_package_iri_path(entry_name, true);
    if (!canonical_entry) {
        return {};
    }
    std::string absolute_part_name;
    absolute_part_name.reserve(canonical_entry.entry_name.size() + 1U);
    if (canonical_entry.entry_name.front() != '/') {
        absolute_part_name.push_back('/');
    }
    absolute_part_name.append(canonical_entry.entry_name);

    if (!content_type_part_name_identity(absolute_part_name).has_value()) {
        return {};
    }
    return absolute_part_name;
}

[[nodiscard]] inline auto
make_package_relationship_target(std::string_view source_part_name,
                                 std::string_view target_part_name)
    -> std::string {
    const auto canonical_source =
        canonicalize_package_iri_path(source_part_name, true);
    const auto canonical_target =
        canonicalize_package_iri_path(target_part_name, true);
    if ((!source_part_name.empty() && !canonical_source) || !canonical_target) {
        return {};
    }

    if ((!source_part_name.empty() &&
         validate_canonical_package_part_path(canonical_source.entry_name) !=
             package_relationship_target_error::none) ||
        validate_canonical_package_part_path(canonical_target.entry_name) !=
            package_relationship_target_error::none ||
        (!canonical_source.entry_name.empty() &&
         canonical_source.entry_name.front() == '/') ||
        canonical_target.entry_name.empty() ||
        canonical_target.entry_name.front() == '/') {
        return {};
    }

    const auto source_parent = package_path_parent(canonical_source.entry_name);

    const auto source_segments =
        package_path_detail::split_normalized_package_path(source_parent);
    const auto target_segments =
        package_path_detail::split_normalized_package_path(
            canonical_target.entry_name);
    std::size_t common_count = 0U;
    while (common_count < source_segments.size() &&
           common_count < target_segments.size() &&
           source_segments[common_count] == target_segments[common_count]) {
        ++common_count;
    }

    std::vector<std::string> relative_segments;
    relative_segments.reserve(source_segments.size() - common_count +
                              target_segments.size() - common_count);
    for (std::size_t index = common_count; index < source_segments.size();
         ++index) {
        relative_segments.emplace_back("..");
    }
    for (std::size_t index = common_count; index < target_segments.size();
         ++index) {
        relative_segments.push_back(target_segments[index]);
    }

    const auto relative =
        package_path_detail::join_package_path_segments(relative_segments);
    if (relative.empty()) {
        return ".";
    }
    const auto first_separator = relative.find('/');
    const auto first_colon = relative.find(':');
    if (first_colon != std::string::npos &&
        (first_separator == std::string::npos ||
         first_colon < first_separator)) {
        return "./" + relative;
    }
    return relative;
}

[[nodiscard]] inline auto package_path_extension(std::string_view path)
    -> std::string {
    const auto filename = package_path_filename(path);
    const auto dot = filename.rfind('.');
    if (dot == std::string::npos || dot == 0U || filename == "." ||
        filename == "..") {
        return {};
    }
    return filename.substr(dot);
}

} // namespace featherdoc::detail
