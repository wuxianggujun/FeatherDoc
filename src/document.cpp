#include "featherdoc.hpp"
#include "xml_helpers.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};
constexpr auto document_relationships_xml_entry =
    std::string_view{"word/_rels/document.xml.rels"};
constexpr auto relationships_xml_entry = std::string_view{"_rels/.rels"};
constexpr auto content_types_xml_entry = std::string_view{"[Content_Types].xml"};
constexpr auto office_document_relationships_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships"};
constexpr auto header_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"};
constexpr auto footer_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"};
constexpr auto header_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"};
constexpr auto footer_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"};
constexpr auto empty_document_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p/>
  </w:body>
</w:document>
)"};
constexpr auto relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)"};
constexpr auto content_types_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)"};
constexpr auto empty_relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
</Relationships>
)"}; 
constexpr auto empty_header_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p/>
</w:hdr>
)"}; 
constexpr auto empty_footer_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p/>
</w:ftr>
)"}; 

struct packaged_entry final {
    std::string_view name;
    std::string_view content;
};

constexpr auto minimal_docx_entries = std::array{
    packaged_entry{relationships_xml_entry, relationships_xml},
};

struct related_part_entry final {
    std::string relationship_id;
    std::string relationship_type;
    std::string entry_name;
};

struct xml_zip_writer final : pugi::xml_writer {
    zip_t *archive{nullptr};
    bool failed{false};

    explicit xml_zip_writer(zip_t *archive_handle) : archive(archive_handle) {}

    void write(const void *data, size_t size) override {
        if (this->failed || size == 0) {
            return;
        }

        if (zip_entry_write(this->archive, data, size) < 0) {
            this->failed = true;
        }
    }
};

struct zip_entry_copy_context {
    zip_t *target_archive{nullptr};
    bool failed{false};
};

auto copy_zip_entry_chunk(void *arg, unsigned long long /*offset*/, const void *data,
                          size_t size) -> size_t {
    auto *context = static_cast<zip_entry_copy_context *>(arg);
    if (context == nullptr || context->failed || size == 0) {
        return 0;
    }

    if (zip_entry_write(context->target_archive, data, size) < 0) {
        context->failed = true;
        return 0;
    }

    return size;
}

auto zip_error_text(int error_number) -> std::string {
    if (const char *message = zip_strerror(error_number); message != nullptr) {
        return message;
    }

    return "unknown zip error";
}

auto initialize_xml_document(pugi::xml_document &xml_document, std::string_view xml_text)
    -> bool {
    xml_document.reset();
    return static_cast<bool>(
        xml_document.load_buffer(xml_text.data(), xml_text.size()));
}

auto initialize_empty_relationships_document(pugi::xml_document &xml_document) -> bool {
    return initialize_xml_document(xml_document, empty_relationships_xml);
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    std::error_code code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    error_info.code = code;
    error_info.detail = std::move(detail);
    error_info.entry_name = std::move(entry_name);
    error_info.xml_offset = xml_offset;
    return code;
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    featherdoc::document_errc code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    return set_last_error(error_info, featherdoc::make_error_code(code),
                          std::move(detail), std::move(entry_name), xml_offset);
}

enum class zip_entry_read_status {
    ok,
    missing,
    read_failed,
};

auto read_zip_entry_text(zip_t *archive, std::string_view entry_name, std::string &content)
    -> zip_entry_read_status {
    if (zip_entry_open(archive, entry_name.data()) != 0) {
        return zip_entry_read_status::missing;
    }

    void *buffer = nullptr;
    size_t buffer_size = 0;
    const auto read_result = zip_entry_read(archive, &buffer, &buffer_size);
    const auto close_result = zip_entry_close(archive);

    if (read_result < 0 || close_result != 0) {
        if (buffer != nullptr) {
            std::free(buffer);
        }
        return zip_entry_read_status::read_failed;
    }

    content.assign(static_cast<const char *>(buffer), buffer_size);
    std::free(buffer);
    return zip_entry_read_status::ok;
}

auto normalize_word_part_entry(std::string_view target) -> std::string {
    std::string normalized_target{target};
    std::replace(normalized_target.begin(), normalized_target.end(), '\\', '/');

    if (!normalized_target.empty() && normalized_target.front() == '/') {
        return std::filesystem::path{normalized_target.substr(1)}
            .lexically_normal()
            .generic_string();
    }

    return (std::filesystem::path{"word"} / std::filesystem::path{normalized_target})
        .lexically_normal()
        .generic_string();
}

auto make_override_part_name(std::string_view entry_name) -> std::string {
    if (entry_name.empty()) {
        return {};
    }

    if (entry_name.front() == '/') {
        return std::string{entry_name};
    }

    return "/" + std::string{entry_name};
}

auto make_document_relationship_target(std::string_view entry_name) -> std::string {
    const auto normalized_entry = std::filesystem::path{std::string{entry_name}}
                                      .lexically_normal();
    const auto relative_target =
        normalized_entry.lexically_relative(std::filesystem::path{"word"});
    if (!relative_target.empty()) {
        return relative_target.generic_string();
    }

    return normalized_entry.filename().generic_string();
}

auto find_section_reference(pugi::xml_node section_properties, const char *reference_name,
                            std::string_view xml_reference_type) -> pugi::xml_node {
    for (auto reference = section_properties.child(reference_name);
         reference != pugi::xml_node{};
         reference = reference.next_sibling(reference_name)) {
        if (std::string_view{reference.attribute("w:type").value()} == xml_reference_type) {
            return reference;
        }
    }

    return {};
}

auto append_section_reference(pugi::xml_node section_properties, const char *reference_name)
    -> pugi::xml_node {
    const auto reference_name_view = std::string_view{reference_name};
    auto insertion_anchor = pugi::xml_node{};

    for (auto child = section_properties.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto child_name = std::string_view{child.name()};

        if (reference_name_view == "w:headerReference") {
            if (child_name == "w:footerReference") {
                insertion_anchor = child;
                break;
            }

            if (child_name != "w:headerReference") {
                insertion_anchor = child;
                break;
            }

            continue;
        }

        if (child_name != "w:headerReference" && child_name != "w:footerReference") {
            insertion_anchor = child;
            break;
        }
    }

    return insertion_anchor == pugi::xml_node{}
               ? section_properties.append_child(reference_name)
               : section_properties.insert_child_before(reference_name, insertion_anchor);
}

auto load_document_relationships_part(
    zip_t *archive, pugi::xml_document &relationships_xml_document,
    bool &has_relationships_part,
    featherdoc::document_error_info &last_error_info)
    -> std::optional<std::vector<related_part_entry>> {
    has_relationships_part = false;
    relationships_xml_document.reset();

    std::string relationships_xml_text;
    const auto relationships_status =
        read_zip_entry_text(archive, document_relationships_xml_entry, relationships_xml_text);
    if (relationships_status == zip_entry_read_status::missing) {
        return std::vector<related_part_entry>{};
    }

    if (relationships_status == zip_entry_read_status::read_failed) {
        set_last_error(last_error_info, featherdoc::document_errc::relationships_xml_read_failed,
                       "failed to read relationships entry 'word/_rels/document.xml.rels'",
                       std::string{document_relationships_xml_entry});
        return std::nullopt;
    }

    const auto parse_result = relationships_xml_document.load_buffer(
        relationships_xml_text.data(), relationships_xml_text.size());
    if (!parse_result) {
        set_last_error(
            last_error_info, featherdoc::document_errc::relationships_xml_parse_failed,
            parse_result.description(), std::string{document_relationships_xml_entry},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
        return std::nullopt;
    }

    has_relationships_part = true;
    std::unordered_set<std::string> seen_entries;
    std::vector<related_part_entry> related_parts;
    const auto relationships = relationships_xml_document.child("Relationships");
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{}; relationship = relationship.next_sibling("Relationship")) {
        const auto type = std::string_view{relationship.attribute("Type").value()};
        if (type != header_relationship_type && type != footer_relationship_type) {
            continue;
        }

        const auto target = std::string_view{relationship.attribute("Target").value()};
        if (target.empty()) {
            continue;
        }

        std::string entry_name = normalize_word_part_entry(target);
        if (!entry_name.empty() && seen_entries.insert(entry_name).second) {
            related_parts.push_back(related_part_entry{
                relationship.attribute("Id").value(), std::string{type},
                std::move(entry_name)});
        }
    }

    return related_parts;
}

auto load_related_xml_part(zip_t *archive, std::string_view entry_name,
                           pugi::xml_document &xml_document,
                           featherdoc::document_error_info &last_error_info)
    -> std::error_code {
    std::string xml_text;
    const auto read_status = read_zip_entry_text(archive, entry_name, xml_text);
    if (read_status == zip_entry_read_status::missing) {
        return set_last_error(last_error_info, featherdoc::document_errc::related_part_open_failed,
                              "failed to open related document part '" +
                                  std::string{entry_name} + "'",
                              std::string{entry_name});
    }

    if (read_status == zip_entry_read_status::read_failed) {
        return set_last_error(last_error_info, featherdoc::document_errc::related_part_read_failed,
                              "failed to read related document part '" +
                                  std::string{entry_name} + "'",
                              std::string{entry_name});
    }

    const auto parse_result = xml_document.load_buffer(xml_text.data(), xml_text.size());
    if (!parse_result) {
        return set_last_error(
            last_error_info, featherdoc::document_errc::related_part_parse_failed,
            parse_result.description(), std::string{entry_name},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    return {};
}

auto collect_named_bookmark_starts(pugi::xml_node node, std::string_view bookmark_name,
                                   std::vector<pugi::xml_node> &bookmark_starts)
    -> void {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:bookmarkStart" &&
            std::string_view{child.attribute("w:name").value()} == bookmark_name) {
            bookmark_starts.push_back(child);
        }

        collect_named_bookmark_starts(child, bookmark_name, bookmark_starts);
    }
}

auto find_matching_bookmark_end(pugi::xml_node bookmark_start) -> pugi::xml_node {
    const auto bookmark_id = std::string_view{bookmark_start.attribute("w:id").value()};
    if (bookmark_id.empty()) {
        return {};
    }

    for (auto node = bookmark_start.next_sibling(); node != pugi::xml_node{};
         node = node.next_sibling()) {
        if (std::string_view{node.name()} == "w:bookmarkEnd" &&
            std::string_view{node.attribute("w:id").value()} == bookmark_id) {
            return node;
        }
    }

    return {};
}

auto replace_bookmark_range(pugi::xml_node bookmark_start, const std::string &replacement)
    -> bool {
    auto parent = bookmark_start.parent();
    if (bookmark_start == pugi::xml_node{} || parent == pugi::xml_node{}) {
        return false;
    }

    const auto bookmark_end = find_matching_bookmark_end(bookmark_start);
    if (bookmark_end == pugi::xml_node{}) {
        return false;
    }

    for (auto node = bookmark_start.next_sibling();
         node != pugi::xml_node{} && node != bookmark_end;) {
        const auto next = node.next_sibling();
        parent.remove_child(node);
        node = next;
    }

    if (!replacement.empty()) {
        auto replacement_run = parent.insert_child_before("w:r", bookmark_end);
        auto replacement_text = replacement_run.append_child("w:t");
        featherdoc::detail::update_xml_space_attribute(replacement_text,
                                                       replacement.c_str());
        replacement_text.text().set(replacement.c_str());
    }

    return true;
}
} // namespace

namespace featherdoc {

Document::Document() = default;

Document::Document(std::filesystem::path file_path)
    : document_path(std::move(file_path)) {}

std::error_code Document::create_empty() {
    this->flag_is_open = false;
    this->has_source_archive = false;
    this->has_document_relationships_part = false;
    this->document_relationships_dirty = false;
    this->content_types_loaded = false;
    this->content_types_dirty = false;
    this->document.reset();
    this->document_relationships.reset();
    this->content_types.reset();
    this->header_parts.clear();
    this->footer_parts.clear();
    this->detached_paragraph.set_parent({});
    this->last_error_info.clear();

    const auto parse_result =
        this->document.load_buffer(empty_document_xml.data(), empty_document_xml.size());
    if (!parse_result) {
        this->document.reset();
        return set_last_error(
            this->last_error_info, document_errc::document_xml_parse_failed,
            parse_result.description(), std::string{document_xml_entry},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    if (!initialize_empty_relationships_document(this->document_relationships)) {
        this->document.reset();
        return set_last_error(
            this->last_error_info, document_errc::relationships_xml_parse_failed,
            "failed to initialize default relationships XML",
            std::string{document_relationships_xml_entry});
    }

    const auto content_types_parse_result =
        this->content_types.load_buffer(content_types_xml.data(), content_types_xml.size());
    if (!content_types_parse_result) {
        this->document.reset();
        this->document_relationships.reset();
        return set_last_error(
            this->last_error_info, document_errc::content_types_xml_parse_failed,
            content_types_parse_result.description(),
            std::string{content_types_xml_entry},
            content_types_parse_result.offset >= 0
                ? std::optional<std::ptrdiff_t>{content_types_parse_result.offset}
                : std::nullopt);
    }

    auto body = this->document.child("w:document").child("w:body");
    this->paragraph.set_parent(body);
    this->table.set_parent(body);
    this->content_types_loaded = true;
    this->flag_is_open = true;
    return {};
}

void Document::set_path(std::filesystem::path file_path) {
    this->document_path = std::move(file_path);
    this->flag_is_open = false;
    this->has_source_archive = false;
    this->has_document_relationships_part = false;
    this->document_relationships_dirty = false;
    this->content_types_loaded = false;
    this->content_types_dirty = false;
    this->document.reset();
    this->document_relationships.reset();
    this->content_types.reset();
    this->header_parts.clear();
    this->footer_parts.clear();
    this->detached_paragraph.set_parent({});
    this->last_error_info.clear();
}

const std::filesystem::path &Document::path() const { return this->document_path; }

std::error_code Document::open() {
    this->flag_is_open = false;
    this->has_source_archive = false;
    this->has_document_relationships_part = false;
    this->document_relationships_dirty = false;
    this->content_types_loaded = false;
    this->content_types_dirty = false;
    this->document.reset();
    this->document_relationships.reset();
    this->content_types.reset();
    this->header_parts.clear();
    this->footer_parts.clear();
    this->detached_paragraph.set_parent({});
    this->last_error_info.clear();

    if (this->document_path.empty()) {
        return set_last_error(this->last_error_info, document_errc::empty_path,
                              "set_path() or the constructor must provide a .docx path");
    }

    std::error_code filesystem_error;
    const auto file_status =
        std::filesystem::status(this->document_path, filesystem_error);
    if (filesystem_error) {
        if (filesystem_error == std::errc::no_such_file_or_directory) {
            return set_last_error(this->last_error_info,
                                  std::make_error_code(
                                      std::errc::no_such_file_or_directory),
                                  "document file does not exist: " +
                                      this->document_path.string());
        }
        return set_last_error(this->last_error_info, filesystem_error,
                              "failed to inspect document path: " +
                                  this->document_path.string());
    }

    if (!std::filesystem::exists(file_status)) {
        return set_last_error(this->last_error_info,
                              std::make_error_code(
                                  std::errc::no_such_file_or_directory),
                              "document file does not exist: " +
                                  this->document_path.string());
    }

    if (!std::filesystem::is_regular_file(file_status)) {
        if (std::filesystem::is_directory(file_status)) {
            return set_last_error(this->last_error_info,
                                  std::make_error_code(std::errc::is_a_directory),
                                  "document path points to a directory: " +
                                      this->document_path.string());
        }
        return set_last_error(this->last_error_info,
                              std::make_error_code(std::errc::invalid_argument),
                              "document path is not a regular file: " +
                                  this->document_path.string());
    }

    const auto archive_path = this->document_path.string();
    void *buf = nullptr;
    size_t bufsize = 0;
    int zip_error = 0;

    zip_t *zip = zip_openwitherror(archive_path.c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'r', &zip_error);

    if (!zip) {
        return set_last_error(this->last_error_info,
                              document_errc::archive_open_failed,
                              "failed to open archive '" + archive_path +
                                  "': " + zip_error_text(zip_error));
    }

    if (zip_entry_open(zip, document_xml_entry.data()) != 0) {
        zip_close(zip);
        return set_last_error(this->last_error_info,
                              document_errc::document_xml_open_failed,
                              "failed to open required entry 'word/document.xml'",
                              std::string{document_xml_entry});
    }

    if (zip_entry_isencrypted(zip) > 0) {
        zip_entry_close(zip);
        zip_close(zip);
        return set_last_error(
            this->last_error_info, document_errc::encrypted_document_unsupported,
            "password-protected or encrypted .docx files are not supported",
            std::string{document_xml_entry});
    }

    const auto read_result = zip_entry_read(zip, &buf, &bufsize);
    if (read_result < 0) {
        zip_entry_close(zip);
        zip_close(zip);
        if (read_result == ZIP_EPASSWD) {
            return set_last_error(
                this->last_error_info, document_errc::encrypted_document_unsupported,
                "password-protected or encrypted .docx files are not supported",
                std::string{document_xml_entry});
        }
        return set_last_error(this->last_error_info,
                              document_errc::document_xml_read_failed,
                              "failed to read XML entry from archive '" +
                                  archive_path + "'",
                              std::string{document_xml_entry});
    }

    zip_entry_close(zip);

    // Keep zip-owned buffers on the zip side to avoid allocator/ABI mismatches
    // when FeatherDoc is consumed through shared-library boundaries.
    const auto parse_result = this->document.load_buffer(buf, bufsize);
    std::free(buf);
    buf = nullptr;

    this->flag_is_open = static_cast<bool>(parse_result);
    if (!this->flag_is_open) {
        zip_close(zip);
        this->document.reset();
        return set_last_error(
            this->last_error_info, document_errc::document_xml_parse_failed,
            parse_result.description(), std::string{document_xml_entry},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    const auto related_parts = load_document_relationships_part(
        zip, this->document_relationships, this->has_document_relationships_part,
        this->last_error_info);
    if (!related_parts.has_value()) {
        zip_close(zip);
        this->document.reset();
        this->flag_is_open = false;
        return this->last_error_info.code;
    }

    if (!this->has_document_relationships_part) {
        initialize_empty_relationships_document(this->document_relationships);
    }

    std::string content_types_text;
    const auto content_types_status =
        read_zip_entry_text(zip, content_types_xml_entry, content_types_text);
    if (content_types_status == zip_entry_read_status::ok) {
        const auto content_types_parse_result =
            this->content_types.load_buffer(content_types_text.data(), content_types_text.size());
        if (content_types_parse_result) {
            this->content_types_loaded = true;
        } else {
            this->content_types.reset();
        }
    }

    for (const auto &related_part : *related_parts) {
        auto part = std::make_unique<xml_part_state>();
        part->relationship_id = related_part.relationship_id;
        part->entry_name = related_part.entry_name;

        if (const auto error =
                load_related_xml_part(zip, part->entry_name, part->xml, this->last_error_info)) {
            zip_close(zip);
            this->document.reset();
            this->document_relationships.reset();
            this->content_types.reset();
            this->header_parts.clear();
            this->footer_parts.clear();
            this->flag_is_open = false;
            return error;
        }

        if (related_part.relationship_type == header_relationship_type) {
            this->header_parts.push_back(std::move(part));
        } else if (related_part.relationship_type == footer_relationship_type) {
            this->footer_parts.push_back(std::move(part));
        }
    }

    zip_close(zip);

    auto body = this->document.child("w:document").child("w:body");
    this->paragraph.set_parent(body);
    this->table.set_parent(body);
    this->has_source_archive = true;
    this->last_error_info.clear();
    return {};
}

std::error_code Document::save() const { return this->save_as(this->document_path); }

std::error_code Document::save_as(std::filesystem::path target_path) const {
    this->last_error_info.clear();

    if (target_path.empty()) {
        return set_last_error(this->last_error_info, document_errc::empty_path,
                              "save_as() target path must not be empty");
    }

    if (!this->is_open()) {
        return set_last_error(this->last_error_info,
                              document_errc::document_not_open,
                              "call open() successfully before save() or save_as()");
    }

    const std::filesystem::path output_file{std::move(target_path)};
    const std::filesystem::path source_file{this->document_path};

    auto temp_file = output_file;
    temp_file += ".tmp";
    auto backup_file = output_file;
    backup_file += ".bak";

    std::error_code filesystem_error;
    std::filesystem::remove(temp_file, filesystem_error);
    filesystem_error.clear();
    std::filesystem::remove(backup_file, filesystem_error);
    filesystem_error.clear();

    int output_zip_error = 0;
    zip_t *new_zip = zip_openwitherror(temp_file.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &output_zip_error);
    if (!new_zip) {
        return set_last_error(this->last_error_info,
                              document_errc::output_archive_open_failed,
                              "failed to create output archive '" +
                                  temp_file.string() + "': " +
                                  zip_error_text(output_zip_error));
    }

    std::error_code result;
    auto record_first_error =
        [&](std::error_code code, std::string detail = {},
            std::string entry_name = {},
            std::optional<std::ptrdiff_t> xml_offset = std::nullopt) {
            if (!result) {
                result = set_last_error(this->last_error_info, code,
                                        std::move(detail), std::move(entry_name),
                                        xml_offset);
            }
        };

    auto record_first_document_error =
        [&](document_errc code, std::string detail = {},
            std::string entry_name = {},
            std::optional<std::ptrdiff_t> xml_offset = std::nullopt) {
            if (!result) {
                result = set_last_error(this->last_error_info, code,
                                        std::move(detail), std::move(entry_name),
                                        xml_offset);
            }
        };

    std::unordered_set<std::string> rewritten_entries;
    rewritten_entries.insert(std::string{document_xml_entry});
    if (this->content_types_dirty || !this->has_source_archive) {
        rewritten_entries.insert(std::string{content_types_xml_entry});
    }
    if (this->has_document_relationships_part &&
        (this->document_relationships_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(std::string{document_relationships_xml_entry});
    }
    for (const auto &part : this->header_parts) {
        rewritten_entries.insert(part->entry_name);
    }
    for (const auto &part : this->footer_parts) {
        rewritten_entries.insert(part->entry_name);
    }

    auto write_text_entry = [&](std::string_view entry_name, std::string_view entry_content) {
        if (zip_entry_open(new_zip, entry_name.data()) != 0) {
            record_first_document_error(document_errc::output_entry_open_failed,
                                        "failed to create output entry '" +
                                            std::string{entry_name} + "'",
                                        std::string{entry_name});
            return;
        }

        if (zip_entry_write(new_zip, entry_content.data(), entry_content.size()) < 0 ||
            zip_entry_close(new_zip) != 0) {
            record_first_document_error(document_errc::output_entry_write_failed,
                                        "failed to write output entry '" +
                                            std::string{entry_name} + "'",
                                        std::string{entry_name});
        }
    };

    auto write_xml_entry = [&](std::string_view entry_name,
                               const pugi::xml_document &xml_document) {
        if (zip_entry_open(new_zip, entry_name.data()) != 0) {
            record_first_document_error(document_errc::output_entry_open_failed,
                                        "failed to create output entry '" +
                                            std::string{entry_name} + "'",
                                        std::string{entry_name});
            return;
        }

        xml_zip_writer writer{new_zip};
        xml_document.print(writer);
        if (writer.failed || zip_entry_close(new_zip) != 0) {
            record_first_document_error(document_errc::output_entry_write_failed,
                                        "failed to write output entry '" +
                                            std::string{entry_name} + "'",
                                        std::string{entry_name});
        }
    };

    auto write_related_xml_part = [&](const xml_part_state &part) {
        write_xml_entry(part.entry_name, part.xml);
    };

    if (zip_entry_open(new_zip, document_xml_entry.data()) != 0) {
        record_first_document_error(
            document_errc::output_document_xml_open_failed,
            "failed to create 'word/document.xml' in output archive '" +
                temp_file.string() + "'",
            std::string{document_xml_entry});
    } else {
        xml_zip_writer writer{new_zip};
        this->document.print(writer);

        if (writer.failed || zip_entry_close(new_zip) != 0) {
            record_first_document_error(
                document_errc::output_document_xml_write_failed,
                "failed while streaming 'word/document.xml' into output archive '" +
                    temp_file.string() + "'",
                std::string{document_xml_entry});
        }
    }

    if (!result && (this->content_types_dirty || !this->has_source_archive)) {
        if (this->content_types_loaded) {
            write_xml_entry(content_types_xml_entry, this->content_types);
        } else {
            write_text_entry(content_types_xml_entry, content_types_xml);
        }
    }

    if (!result && this->has_document_relationships_part &&
        (this->document_relationships_dirty || !this->has_source_archive)) {
        write_xml_entry(document_relationships_xml_entry, this->document_relationships);
    }

    if (!result) {
        for (const auto &part : this->header_parts) {
            write_related_xml_part(*part);
            if (result) {
                break;
            }
        }
    }

    if (!result) {
        for (const auto &part : this->footer_parts) {
            write_related_xml_part(*part);
            if (result) {
                break;
            }
        }
    }

    if (!result && !this->has_source_archive) {
        for (const auto &[entry_name, entry_content] : minimal_docx_entries) {
            write_text_entry(entry_name, entry_content);
            if (result) {
                break;
            }
        }
    }

    if (!result && this->has_source_archive) {
        int source_zip_error = 0;
        zip_t *orig_zip = zip_openwitherror(source_file.string().c_str(),
                                            ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                            &source_zip_error);
        if (!orig_zip) {
            record_first_document_error(
                document_errc::source_archive_open_failed,
                "failed to reopen source archive '" + source_file.string() +
                    "': " + zip_error_text(source_zip_error));
        }

        if (!result && orig_zip != nullptr) {
            const auto orig_zip_entry_count = zip_entries_total(orig_zip);
            if (orig_zip_entry_count < 0) {
                record_first_document_error(
                    document_errc::source_archive_entries_failed,
                    "failed to enumerate entries in source archive '" +
                        source_file.string() + "'");
            }
            for (ssize_t i = 0; !result && i < orig_zip_entry_count; ++i) {
                if (zip_entry_openbyindex(orig_zip, static_cast<size_t>(i)) != 0) {
                    record_first_document_error(
                        document_errc::source_entry_open_failed,
                        "failed to open source archive entry at index " +
                            std::to_string(static_cast<long long>(i)));
                    break;
                }

                const char *name = zip_entry_name(orig_zip);
                if (name == nullptr) {
                    zip_entry_close(orig_zip);
                    record_first_document_error(
                        document_errc::source_entry_name_failed,
                        "failed to read source archive entry name at index " +
                            std::to_string(static_cast<long long>(i)));
                    break;
                }

                const std::string entry_name{name};
                if (rewritten_entries.find(entry_name) == rewritten_entries.end()) {
                    if (zip_entry_open(new_zip, name) != 0) {
                        zip_entry_close(orig_zip);
                        record_first_document_error(
                            document_errc::output_entry_open_failed,
                            "failed to create output entry '" + entry_name + "'",
                            entry_name);
                        break;
                    }

                    if (zip_entry_isdir(orig_zip)) {
                        if (zip_entry_close(new_zip) != 0) {
                            zip_entry_close(orig_zip);
                            record_first_document_error(
                                document_errc::output_entry_write_failed,
                                "failed to finalize output directory entry '" +
                                    entry_name + "'",
                                entry_name);
                            break;
                        }
                    } else {
                        zip_entry_copy_context context{new_zip};
                        if (zip_entry_extract(orig_zip, copy_zip_entry_chunk,
                                              &context) < 0 ||
                            context.failed || zip_entry_close(new_zip) != 0) {
                            zip_entry_close(orig_zip);
                            record_first_document_error(
                                document_errc::output_entry_write_failed,
                                "failed to copy archive entry '" + entry_name +
                                    "' into output archive",
                                entry_name);
                            break;
                        }
                    }
                }

                if (zip_entry_close(orig_zip) != 0) {
                    record_first_document_error(
                        document_errc::source_entry_close_failed,
                        "failed to close source archive entry '" + entry_name + "'",
                        entry_name);
                    break;
                }
            }
        }

        if (orig_zip != nullptr) {
            zip_close(orig_zip);
        }
    }
    zip_close(new_zip);

    if (result) {
        std::filesystem::remove(temp_file, filesystem_error);
        return result;
    }

    if (std::filesystem::exists(output_file, filesystem_error)) {
        if (filesystem_error) {
            std::filesystem::remove(temp_file, filesystem_error);
            return set_last_error(this->last_error_info, filesystem_error,
                                  "failed to inspect output target '" +
                                      output_file.string() + "'");
        }

        std::filesystem::rename(output_file, backup_file, filesystem_error);
        if (filesystem_error) {
            std::filesystem::remove(temp_file, filesystem_error);
            return set_last_error(this->last_error_info, filesystem_error,
                                  "failed to move output file '" +
                                      output_file.string() + "' to backup '" +
                                      backup_file.string() + "'");
        }
    }

    std::filesystem::rename(temp_file, output_file, filesystem_error);
    if (filesystem_error) {
        std::error_code restore_error;
        if (std::filesystem::exists(backup_file, restore_error)) {
            std::filesystem::rename(backup_file, output_file, restore_error);
        }
        std::filesystem::remove(temp_file, restore_error);
        if (restore_error) {
            return set_last_error(
                this->last_error_info, restore_error,
                "failed to restore original file after rename failure; backup was '" +
                    backup_file.string() + "'");
        }
        return set_last_error(this->last_error_info, filesystem_error,
                              "failed to replace output file '" +
                                  output_file.string() + "' with temporary file '" +
                                  temp_file.string() + "'");
    }

    std::filesystem::remove(backup_file, filesystem_error);
    if (filesystem_error) {
        return set_last_error(this->last_error_info, filesystem_error,
                              "saved document but failed to remove backup file '" +
                                  backup_file.string() + "'");
    }

    this->last_error_info.clear();
    return {};
}

bool Document::is_open() const { return this->flag_is_open; }

const document_error_info &Document::last_error() const noexcept {
    return this->last_error_info;
}

std::size_t Document::section_count() const noexcept {
    const auto body = this->document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        return 0U;
    }

    std::size_t count = 1U;
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        if (child.child("w:pPr").child("w:sectPr") != pugi::xml_node{}) {
            ++count;
        }
    }

    return count;
}

std::size_t Document::header_count() const noexcept { return this->header_parts.size(); }

std::size_t Document::footer_count() const noexcept { return this->footer_parts.size(); }

pugi::xml_node Document::section_properties(std::size_t section_index) const {
    const auto body = this->document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        return {};
    }

    std::size_t current_index = 0U;
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        const auto paragraph_section_properties = child.child("w:pPr").child("w:sectPr");
        if (paragraph_section_properties == pugi::xml_node{}) {
            continue;
        }

        if (current_index == section_index) {
            return paragraph_section_properties;
        }
        ++current_index;
    }

    if (current_index == section_index) {
        return body.child("w:sectPr");
    }

    return {};
}

pugi::xml_node Document::ensure_section_properties(std::size_t section_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing header/footer parts");
        return {};
    }

    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return {};
    }

    const auto count = this->section_count();
    if (section_index >= count) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for header/footer editing",
                       std::string{document_xml_entry});
        return {};
    }

    if (const auto existing = this->section_properties(section_index);
        existing != pugi::xml_node{}) {
        return existing;
    }

    if (section_index + 1U != count) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "the target section does not expose an editable w:sectPr node",
                       std::string{document_xml_entry});
        return {};
    }

    auto section_properties = body.child("w:sectPr");
    if (section_properties != pugi::xml_node{}) {
        return section_properties;
    }

    section_properties = body.append_child("w:sectPr");
    if (section_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:sectPr for header/footer references",
                       std::string{document_xml_entry});
        return {};
    }

    return section_properties;
}

Paragraph &Document::header_paragraphs(std::size_t index) {
    if (index >= this->header_parts.size()) {
        this->detached_paragraph.set_parent({});
        return this->detached_paragraph;
    }

    auto &part = *this->header_parts[index];
    part.paragraph.set_parent(part.xml.child("w:hdr"));
    return part.paragraph;
}

Paragraph &Document::footer_paragraphs(std::size_t index) {
    if (index >= this->footer_parts.size()) {
        this->detached_paragraph.set_parent({});
        return this->detached_paragraph;
    }

    auto &part = *this->footer_parts[index];
    part.paragraph.set_parent(part.xml.child("w:ftr"));
    return part.paragraph;
}

Paragraph &Document::related_part_paragraphs_by_relationship_id(
    std::string_view relationship_id, std::vector<std::unique_ptr<xml_part_state>> &parts,
    const char *part_root_name) {
    if (relationship_id.empty()) {
        this->detached_paragraph.set_parent({});
        return this->detached_paragraph;
    }

    for (auto &part : parts) {
        if (part->relationship_id == relationship_id) {
            part->paragraph.set_parent(part->xml.child(part_root_name));
            return part->paragraph;
        }
    }

    this->detached_paragraph.set_parent({});
    return this->detached_paragraph;
}

Paragraph &Document::section_related_part_paragraphs(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind,
    std::vector<std::unique_ptr<xml_part_state>> &parts, const char *reference_name,
    const char *part_root_name) {
    const auto section_properties = this->section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        this->detached_paragraph.set_parent({});
        return this->detached_paragraph;
    }

    const auto xml_reference_type = featherdoc::to_xml_reference_type(reference_kind);
    for (auto reference = section_properties.child(reference_name);
         reference != pugi::xml_node{};
         reference = reference.next_sibling(reference_name)) {
        if (std::string_view{reference.attribute("w:type").value()} != xml_reference_type) {
            continue;
        }

        return this->related_part_paragraphs_by_relationship_id(
            reference.attribute("r:id").value(), parts, part_root_name);
    }

    this->detached_paragraph.set_parent({});
    return this->detached_paragraph;
}

Paragraph &Document::section_header_paragraphs(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    return this->section_related_part_paragraphs(
        section_index, reference_kind, this->header_parts, "w:headerReference", "w:hdr");
}

Paragraph &Document::section_footer_paragraphs(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    return this->section_related_part_paragraphs(
        section_index, reference_kind, this->footer_parts, "w:footerReference", "w:ftr");
}

std::error_code Document::ensure_content_types_loaded() {
    if (this->content_types_loaded) {
        return {};
    }

    this->content_types.reset();
    if (!this->has_source_archive) {
        const auto parse_result =
            this->content_types.load_buffer(content_types_xml.data(), content_types_xml.size());
        if (!parse_result) {
            return set_last_error(
                this->last_error_info, document_errc::content_types_xml_parse_failed,
                parse_result.description(), std::string{content_types_xml_entry},
                parse_result.offset >= 0
                    ? std::optional<std::ptrdiff_t>{parse_result.offset}
                    : std::nullopt);
        }

        this->content_types_loaded = true;
        return {};
    }

    int source_zip_error = 0;
    zip_t *source_zip = zip_openwitherror(this->document_path.string().c_str(),
                                          ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                          &source_zip_error);
    if (!source_zip) {
        return set_last_error(this->last_error_info,
                              document_errc::source_archive_open_failed,
                              "failed to reopen source archive '" +
                                  this->document_path.string() +
                                  "' while loading [Content_Types].xml: " +
                                  zip_error_text(source_zip_error));
    }

    std::string content_types_text;
    const auto read_status =
        read_zip_entry_text(source_zip, content_types_xml_entry, content_types_text);
    zip_close(source_zip);

    if (read_status != zip_entry_read_status::ok) {
        return set_last_error(this->last_error_info,
                              document_errc::content_types_xml_read_failed,
                              "failed to read required entry '[Content_Types].xml'",
                              std::string{content_types_xml_entry});
    }

    const auto parse_result =
        this->content_types.load_buffer(content_types_text.data(), content_types_text.size());
    if (!parse_result) {
        return set_last_error(
            this->last_error_info, document_errc::content_types_xml_parse_failed,
            parse_result.description(), std::string{content_types_xml_entry},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    this->content_types_loaded = true;
    return {};
}

Paragraph &Document::ensure_related_part_paragraphs(
    std::vector<std::unique_ptr<xml_part_state>> &parts, const char *part_root_name,
    const char *reference_name, const char *relationship_type,
    const char *content_type) {
    this->detached_paragraph.set_parent({});

    const auto count = this->section_count();
    const auto last_section_index = count == 0U ? 0U : count - 1U;
    return this->ensure_section_related_part_paragraphs(
        last_section_index, featherdoc::section_reference_kind::default_reference, parts,
        part_root_name, reference_name, relationship_type, content_type);
}

Paragraph &Document::ensure_section_related_part_paragraphs(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind,
    std::vector<std::unique_ptr<xml_part_state>> &parts, const char *part_root_name,
    const char *reference_name, const char *relationship_type,
    const char *content_type) {
    this->detached_paragraph.set_parent({});

    const auto section_properties = this->ensure_section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        return this->detached_paragraph;
    }

    const auto xml_reference_type = featherdoc::to_xml_reference_type(reference_kind);
    auto reference =
        find_section_reference(section_properties, reference_name, xml_reference_type);
    if (reference != pugi::xml_node{}) {
        const auto relationship_id = std::string_view{reference.attribute("r:id").value()};
        if (!relationship_id.empty()) {
            for (const auto &part : parts) {
                if (part->relationship_id == relationship_id) {
                    part->paragraph.set_parent(part->xml.child(part_root_name));
                    this->last_error_info.clear();
                    return part->paragraph;
                }
            }
        }
    }

    if (const auto error = this->ensure_content_types_loaded()) {
        return this->detached_paragraph;
    }

    auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document root",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    if (this->document_relationships.child("Relationships") == pugi::xml_node{} &&
        !initialize_empty_relationships_document(this->document_relationships)) {
        set_last_error(this->last_error_info,
                       document_errc::relationships_xml_parse_failed,
                       "failed to initialize default relationships XML",
                       std::string{document_relationships_xml_entry});
        return this->detached_paragraph;
    }

    auto relationships = this->document_relationships.child("Relationships");
    if (relationships == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::relationships_xml_parse_failed,
                       "word/_rels/document.xml.rels does not contain a Relationships root",
                       std::string{document_relationships_xml_entry});
        return this->detached_paragraph;
    }

    auto types = this->content_types.child("Types");
    if (types == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::content_types_xml_parse_failed,
                       "[Content_Types].xml does not contain a Types root",
                       std::string{content_types_xml_entry});
        return this->detached_paragraph;
    }

    auto relationships_namespace = document_root.attribute("xmlns:r");
    if (relationships_namespace == pugi::xml_attribute{}) {
        relationships_namespace = document_root.append_attribute("xmlns:r");
    }
    relationships_namespace.set_value(
        office_document_relationships_namespace_uri.data());

    std::unordered_set<std::string> used_relationship_ids;
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        const auto id = relationship.attribute("Id").value();
        if (*id != '\0') {
            used_relationship_ids.emplace(id);
        }
    }

    std::string relationship_id;
    for (std::size_t next_index = 1U; relationship_id.empty(); ++next_index) {
        auto candidate = "rId" + std::to_string(next_index);
        if (!used_relationship_ids.contains(candidate)) {
            relationship_id = std::move(candidate);
        }
    }

    const auto part_prefix =
        std::string_view{part_root_name} == "w:hdr" ? "header" : "footer";
    std::unordered_set<std::string> used_part_entries;
    for (const auto &part : parts) {
        used_part_entries.insert(part->entry_name);
    }

    std::string entry_name;
    for (std::size_t next_index = 1U; entry_name.empty(); ++next_index) {
        auto candidate = std::string{"word/"} + part_prefix +
                         std::to_string(next_index) + ".xml";
        if (!used_part_entries.contains(candidate)) {
            entry_name = std::move(candidate);
        }
    }

    auto part = std::make_unique<xml_part_state>();
    part->relationship_id = relationship_id;
    part->entry_name = entry_name;
    const auto empty_part_xml =
        std::string_view{part_root_name} == "w:hdr" ? empty_header_xml : empty_footer_xml;
    const auto parse_result =
        part->xml.load_buffer(empty_part_xml.data(), empty_part_xml.size());
    if (!parse_result) {
        set_last_error(this->last_error_info, document_errc::related_part_parse_failed,
                       parse_result.description(), entry_name,
                       parse_result.offset >= 0
                           ? std::optional<std::ptrdiff_t>{parse_result.offset}
                           : std::nullopt);
        return this->detached_paragraph;
    }

    auto relationship = relationships.append_child("Relationship");
    if (relationship == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create a new document relationship for header/footer",
                       std::string{document_relationships_xml_entry});
        return this->detached_paragraph;
    }

    relationship.append_attribute("Id").set_value(relationship_id.c_str());
    relationship.append_attribute("Type").set_value(relationship_type);
    const auto relationship_target = make_document_relationship_target(entry_name);
    relationship.append_attribute("Target").set_value(relationship_target.c_str());

    const auto override_part_name = make_override_part_name(entry_name);
    auto override_node = pugi::xml_node{};
    for (auto existing_override = types.child("Override");
         existing_override != pugi::xml_node{};
         existing_override = existing_override.next_sibling("Override")) {
        if (std::string_view{existing_override.attribute("PartName").value()} ==
            override_part_name) {
            override_node = existing_override;
            break;
        }
    }
    if (override_node == pugi::xml_node{}) {
        override_node = types.append_child("Override");
    }
    if (override_node == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append a content type override for header/footer",
                       std::string{content_types_xml_entry});
        return this->detached_paragraph;
    }
    auto part_name_attribute = override_node.attribute("PartName");
    if (part_name_attribute == pugi::xml_attribute{}) {
        part_name_attribute = override_node.append_attribute("PartName");
    }
    part_name_attribute.set_value(override_part_name.c_str());

    auto content_type_attribute = override_node.attribute("ContentType");
    if (content_type_attribute == pugi::xml_attribute{}) {
        content_type_attribute = override_node.append_attribute("ContentType");
    }
    content_type_attribute.set_value(content_type);

    if (reference == pugi::xml_node{}) {
        reference = append_section_reference(section_properties, reference_name);
    }

    if (reference == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to attach a section header/footer reference to w:sectPr",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    auto type_attribute = reference.attribute("w:type");
    if (type_attribute == pugi::xml_attribute{}) {
        type_attribute = reference.append_attribute("w:type");
    }
    type_attribute.set_value(xml_reference_type.data());

    auto relationship_attribute = reference.attribute("r:id");
    if (relationship_attribute == pugi::xml_attribute{}) {
        relationship_attribute = reference.append_attribute("r:id");
    }
    relationship_attribute.set_value(relationship_id.c_str());

    parts.push_back(std::move(part));
    auto &created_part = *parts.back();
    created_part.paragraph.set_parent(created_part.xml.child(part_root_name));

    this->has_document_relationships_part = true;
    this->document_relationships_dirty = true;
    this->content_types_dirty = true;
    this->last_error_info.clear();
    return created_part.paragraph;
}

Paragraph &Document::ensure_section_header_paragraphs(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    return this->ensure_section_related_part_paragraphs(
        section_index, reference_kind, this->header_parts, "w:hdr",
        "w:headerReference", header_relationship_type.data(),
        header_content_type.data());
}

Paragraph &Document::ensure_section_footer_paragraphs(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    return this->ensure_section_related_part_paragraphs(
        section_index, reference_kind, this->footer_parts, "w:ftr",
        "w:footerReference", footer_relationship_type.data(),
        footer_content_type.data());
}

Paragraph &Document::ensure_header_paragraphs() {
    return this->ensure_related_part_paragraphs(
        this->header_parts, "w:hdr", "w:headerReference",
        header_relationship_type.data(), header_content_type.data());
}

Paragraph &Document::ensure_footer_paragraphs() {
    return this->ensure_related_part_paragraphs(
        this->footer_parts, "w:ftr", "w:footerReference",
        footer_relationship_type.data(), footer_content_type.data());
}

std::size_t Document::replace_bookmark_text(const std::string &bookmark_name,
                                            const std::string &replacement) {
    if (!this->is_open() || bookmark_name.empty()) {
        return 0;
    }

    std::vector<pugi::xml_node> bookmark_starts;
    collect_named_bookmark_starts(this->document, bookmark_name, bookmark_starts);

    std::size_t replaced = 0;
    for (const auto bookmark_start : bookmark_starts) {
        if (replace_bookmark_range(bookmark_start, replacement)) {
            ++replaced;
        }
    }

    return replaced;
}

std::size_t Document::replace_bookmark_text(const char *bookmark_name,
                                            const char *replacement) {
    if (bookmark_name == nullptr || replacement == nullptr) {
        return 0;
    }

    return this->replace_bookmark_text(std::string{bookmark_name},
                                       std::string{replacement});
}

Paragraph &Document::paragraphs() {
    this->paragraph.set_parent(document.child("w:document").child("w:body"));
    return this->paragraph;
}

Table &Document::tables() {
    this->table.set_parent(document.child("w:document").child("w:body"));
    return this->table;
}

} // namespace featherdoc
