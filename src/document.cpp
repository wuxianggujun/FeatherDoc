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
constexpr auto settings_xml_entry = std::string_view{"word/settings.xml"};
constexpr auto numbering_xml_entry = std::string_view{"word/numbering.xml"};
constexpr auto styles_xml_entry = std::string_view{"word/styles.xml"};
constexpr auto office_document_relationships_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships"};
constexpr auto header_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"};
constexpr auto footer_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"};
constexpr auto settings_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings"};
constexpr auto header_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"};
constexpr auto footer_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"};
constexpr auto settings_content_type = std::string_view{
    "application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"};
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
constexpr auto empty_settings_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
</w:settings>
)"}; 
constexpr int docx_output_compression_level = 0;

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

auto split_plain_text_paragraphs(std::string_view text) -> std::vector<std::string> {
    std::vector<std::string> paragraphs;
    std::size_t begin = 0U;
    while (begin <= text.size()) {
        const auto line_end = text.find('\n', begin);
        const auto end = line_end == std::string_view::npos ? text.size() : line_end;

        std::string paragraph_text(text.substr(begin, end - begin));
        if (!paragraph_text.empty() && paragraph_text.back() == '\r') {
            paragraph_text.pop_back();
        }
        paragraphs.push_back(std::move(paragraph_text));

        if (line_end == std::string_view::npos) {
            break;
        }

        begin = end + 1U;
    }

    if (paragraphs.empty()) {
        paragraphs.emplace_back();
    }

    if (!text.empty() && (text.ends_with('\n') || text.ends_with('\r'))) {
        while (paragraphs.size() > 1U && paragraphs.back().empty()) {
            paragraphs.pop_back();
        }
    }

    return paragraphs;
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

auto make_part_relationships_entry(std::string_view entry_name) -> std::string {
    if (entry_name.empty()) {
        return {};
    }

    const auto normalized_entry =
        std::filesystem::path{std::string{entry_name}}.lexically_normal();
    const auto filename = normalized_entry.filename().generic_string();
    if (filename.empty()) {
        return {};
    }

    return (normalized_entry.parent_path() / "_rels" /
            std::filesystem::path{filename + ".rels"})
        .lexically_normal()
        .generic_string();
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

void clear_section_header_footer_references(pugi::xml_node section_properties) {
    if (section_properties == pugi::xml_node{}) {
        return;
    }

    for (auto reference = section_properties.child("w:headerReference");
         reference != pugi::xml_node{};) {
        const auto next = reference.next_sibling("w:headerReference");
        section_properties.remove_child(reference);
        reference = next;
    }

    for (auto reference = section_properties.child("w:footerReference");
         reference != pugi::xml_node{};) {
        const auto next = reference.next_sibling("w:footerReference");
        section_properties.remove_child(reference);
        reference = next;
    }

    section_properties.remove_child("w:titlePg");
}

struct section_body_snapshot final {
    pugi::xml_document xml;
    pugi::xml_node root;
    pugi::xml_node content_root;
    pugi::xml_node properties_root;

    section_body_snapshot() {
        this->root = this->xml.append_child("section");
        this->content_root = this->root.append_child("content");
        this->properties_root = this->root.append_child("properties");
    }

    [[nodiscard]] auto section_properties() const -> pugi::xml_node {
        return this->properties_root.child("w:sectPr");
    }
};

auto node_has_attributes(pugi::xml_node node) -> bool {
    return node.first_attribute() != pugi::xml_attribute{};
}

void remove_empty_paragraph_properties(pugi::xml_node paragraph) {
    if (paragraph == pugi::xml_node{}) {
        return;
    }

    auto paragraph_properties = paragraph.child("w:pPr");
    if (paragraph_properties == pugi::xml_node{}) {
        return;
    }

    if (paragraph_properties.first_child() == pugi::xml_node{} &&
        !node_has_attributes(paragraph_properties)) {
        paragraph.remove_child(paragraph_properties);
    }
}

void remove_empty_paragraph(pugi::xml_node paragraph) {
    if (paragraph == pugi::xml_node{}) {
        return;
    }

    remove_empty_paragraph_properties(paragraph);
    if (paragraph.first_child() != pugi::xml_node{} || node_has_attributes(paragraph)) {
        return;
    }

    if (auto parent = paragraph.parent(); parent != pugi::xml_node{}) {
        parent.remove_child(paragraph);
    }
}

auto find_document_relationship_by_type(pugi::xml_node relationships,
                                        std::string_view relationship_type)
    -> pugi::xml_node {
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Type").value()} == relationship_type) {
            return relationship;
        }
    }

    return {};
}

auto ensure_on_off_node_enabled(pugi::xml_node parent, const char *child_name)
    -> pugi::xml_node {
    auto child = parent.child(child_name);
    if (child == pugi::xml_node{}) {
        child = parent.append_child(child_name);
    }
    if (child == pugi::xml_node{}) {
        return {};
    }

    auto value_attribute = child.attribute("w:val");
    if (value_attribute == pugi::xml_attribute{}) {
        value_attribute = child.append_attribute("w:val");
    }
    value_attribute.set_value("1");
    return child;
}

auto ensure_section_title_page_node(pugi::xml_node section_properties) -> pugi::xml_node {
    auto title_page = section_properties.child("w:titlePg");
    if (title_page == pugi::xml_node{}) {
        auto insertion_anchor = pugi::xml_node{};
        for (auto child = section_properties.first_child(); child != pugi::xml_node{};
             child = child.next_sibling()) {
            const auto child_name = std::string_view{child.name()};
            if (child_name == "w:textDirection" || child_name == "w:bidi" ||
                child_name == "w:rtlGutter" || child_name == "w:docGrid" ||
                child_name == "w:printerSettings" || child_name == "w:sectPrChange") {
                insertion_anchor = child;
                break;
            }
        }

        title_page = insertion_anchor == pugi::xml_node{}
                         ? section_properties.append_child("w:titlePg")
                         : section_properties.insert_child_before("w:titlePg",
                                                                  insertion_anchor);
    }

    if (title_page == pugi::xml_node{}) {
        return {};
    }

    auto value_attribute = title_page.attribute("w:val");
    if (value_attribute == pugi::xml_attribute{}) {
        value_attribute = title_page.append_attribute("w:val");
    }
    value_attribute.set_value("1");
    return title_page;
}

auto section_break_paragraph_for(pugi::xml_node section_properties) -> pugi::xml_node {
    if (section_properties == pugi::xml_node{}) {
        return {};
    }

    const auto paragraph_properties = section_properties.parent();
    if (paragraph_properties == pugi::xml_node{} ||
        std::string_view{paragraph_properties.name()} != "w:pPr") {
        return {};
    }

    const auto paragraph = paragraph_properties.parent();
    if (paragraph == pugi::xml_node{} || std::string_view{paragraph.name()} != "w:p") {
        return {};
    }

    return paragraph;
}

auto replace_section_properties_contents(pugi::xml_node target_section_properties,
                                         pugi::xml_node source_section_properties) -> bool {
    if (target_section_properties == pugi::xml_node{} ||
        source_section_properties == pugi::xml_node{}) {
        return false;
    }

    for (auto attribute = target_section_properties.first_attribute();
         attribute != pugi::xml_attribute{};) {
        const auto next = attribute.next_attribute();
        target_section_properties.remove_attribute(attribute);
        attribute = next;
    }

    for (auto child = target_section_properties.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        target_section_properties.remove_child(child);
        child = next;
    }

    for (auto attribute = source_section_properties.first_attribute();
         attribute != pugi::xml_attribute{}; attribute = attribute.next_attribute()) {
        auto copied_attribute = target_section_properties.append_attribute(attribute.name());
        if (copied_attribute == pugi::xml_attribute{}) {
            return false;
        }
        copied_attribute.set_value(attribute.value());
    }

    for (auto child = source_section_properties.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (target_section_properties.append_copy(child) == pugi::xml_node{}) {
            return false;
        }
    }

    return true;
}

auto ensure_paragraph_properties_node(pugi::xml_node paragraph) -> pugi::xml_node {
    if (paragraph == pugi::xml_node{}) {
        return {};
    }

    auto paragraph_properties = paragraph.child("w:pPr");
    if (paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }

    const auto first_child = paragraph.first_child();
    return first_child == pugi::xml_node{}
               ? paragraph.append_child("w:pPr")
               : paragraph.insert_child_before("w:pPr", first_child);
}

auto capture_section_snapshot(pugi::xml_node first_child, pugi::xml_node end_exclusive,
                              pugi::xml_node section_properties,
                              bool strip_last_paragraph_section_properties)
    -> std::unique_ptr<section_body_snapshot> {
    auto snapshot = std::make_unique<section_body_snapshot>();
    if (snapshot->root == pugi::xml_node{} || snapshot->content_root == pugi::xml_node{} ||
        snapshot->properties_root == pugi::xml_node{}) {
        return nullptr;
    }

    for (auto child = first_child; child != pugi::xml_node{} && child != end_exclusive;
         child = child.next_sibling()) {
        if (snapshot->content_root.append_copy(child) == pugi::xml_node{}) {
            return nullptr;
        }
    }

    if (section_properties != pugi::xml_node{}) {
        if (snapshot->properties_root.append_copy(section_properties) == pugi::xml_node{}) {
            return nullptr;
        }
    } else if (snapshot->properties_root.append_child("w:sectPr") == pugi::xml_node{}) {
        return nullptr;
    }

    if (strip_last_paragraph_section_properties) {
        auto last_copied_child = snapshot->content_root.last_child();
        if (last_copied_child != pugi::xml_node{} &&
            std::string_view{last_copied_child.name()} == "w:p") {
            if (auto paragraph_properties = last_copied_child.child("w:pPr");
                paragraph_properties != pugi::xml_node{}) {
                paragraph_properties.remove_child("w:sectPr");
                remove_empty_paragraph_properties(last_copied_child);
            }
        }
    }

    return snapshot;
}

auto collect_section_snapshots(pugi::xml_node body)
    -> std::optional<std::vector<std::unique_ptr<section_body_snapshot>>> {
    if (body == pugi::xml_node{}) {
        return std::nullopt;
    }

    std::vector<std::unique_ptr<section_body_snapshot>> snapshots;
    auto body_section_properties = body.child("w:sectPr");
    auto current_section_first_child = body.first_child();

    for (auto child = body.first_child(); child != pugi::xml_node{} && child != body_section_properties;
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        auto paragraph_section_properties = child.child("w:pPr").child("w:sectPr");
        if (paragraph_section_properties == pugi::xml_node{}) {
            continue;
        }

        auto snapshot = capture_section_snapshot(current_section_first_child,
                                                child.next_sibling(),
                                                paragraph_section_properties, true);
        if (snapshot == nullptr) {
            return std::nullopt;
        }

        snapshots.push_back(std::move(snapshot));
        current_section_first_child = child.next_sibling();
    }

    auto final_snapshot = capture_section_snapshot(current_section_first_child,
                                                   body_section_properties,
                                                   body_section_properties, false);
    if (final_snapshot == nullptr) {
        return std::nullopt;
    }

    snapshots.push_back(std::move(final_snapshot));
    return snapshots;
}

auto rebuild_body_from_section_snapshots(
    pugi::xml_node body, const std::vector<std::unique_ptr<section_body_snapshot>> &snapshots)
    -> bool {
    if (body == pugi::xml_node{}) {
        return false;
    }

    for (auto child = body.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        body.remove_child(child);
        child = next;
    }

    for (std::size_t index = 0; index < snapshots.size(); ++index) {
        const auto &snapshot = snapshots[index];
        auto last_content_node = pugi::xml_node{};
        for (auto child = snapshot->content_root.first_child(); child != pugi::xml_node{};
             child = child.next_sibling()) {
            last_content_node = body.append_copy(child);
            if (last_content_node == pugi::xml_node{}) {
                return false;
            }
        }

        const auto section_properties = snapshot->section_properties();
        if (index + 1U < snapshots.size()) {
            auto host_paragraph = pugi::xml_node{};
            if (last_content_node != pugi::xml_node{} &&
                std::string_view{last_content_node.name()} == "w:p") {
                host_paragraph = last_content_node;
            } else {
                host_paragraph = featherdoc::detail::append_paragraph_node(body);
            }

            if (host_paragraph == pugi::xml_node{}) {
                return false;
            }

            auto paragraph_properties = ensure_paragraph_properties_node(host_paragraph);
            if (paragraph_properties == pugi::xml_node{}) {
                return false;
            }

            if (paragraph_properties.append_copy(section_properties) == pugi::xml_node{}) {
                return false;
            }
        } else if (body.append_copy(section_properties) == pugi::xml_node{}) {
            return false;
        }
    }

    return true;
}

auto section_has_reference_type(pugi::xml_node section_properties, const char *reference_name,
                                std::string_view xml_reference_type) -> bool {
    return find_section_reference(section_properties, reference_name, xml_reference_type) !=
           pugi::xml_node{};
}

auto document_has_reference_type(const pugi::xml_document &document, const char *reference_name,
                                 std::string_view xml_reference_type) -> bool {
    const auto body = document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        return false;
    }

    const auto section_has_reference = [&](pugi::xml_node section_properties) {
        return section_has_reference_type(section_properties, reference_name, xml_reference_type);
    };

    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        if (const auto section_properties = child.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{} && section_has_reference(section_properties)) {
            return true;
        }
    }

    if (const auto section_properties = body.child("w:sectPr");
        section_properties != pugi::xml_node{} && section_has_reference(section_properties)) {
        return true;
    }

    return false;
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

auto load_optional_relationships_part(
    zip_t *archive, std::string_view entry_name, pugi::xml_document &xml_document,
    bool &has_relationships_part,
    featherdoc::document_error_info &last_error_info) -> std::error_code {
    has_relationships_part = false;
    xml_document.reset();

    if (entry_name.empty()) {
        return {};
    }

    std::string xml_text;
    const auto read_status = read_zip_entry_text(archive, entry_name, xml_text);
    if (read_status == zip_entry_read_status::missing) {
        return {};
    }

    if (read_status == zip_entry_read_status::read_failed) {
        return set_last_error(last_error_info,
                              featherdoc::document_errc::relationships_xml_read_failed,
                              "failed to read relationships entry '" +
                                  std::string{entry_name} + "'",
                              std::string{entry_name});
    }

    const auto parse_result = xml_document.load_buffer(xml_text.data(), xml_text.size());
    if (!parse_result) {
        return set_last_error(
            last_error_info, featherdoc::document_errc::relationships_xml_parse_failed,
            parse_result.description(), std::string{entry_name},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    has_relationships_part = true;
    return {};
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
    this->has_settings_part = false;
    this->has_numbering_part = false;
    this->has_styles_part = false;
    this->document_relationships_dirty = false;
    this->content_types_loaded = false;
    this->content_types_dirty = false;
    this->settings_loaded = false;
    this->settings_dirty = false;
    this->numbering_loaded = false;
    this->numbering_dirty = false;
    this->styles_loaded = false;
    this->styles_dirty = false;
    this->removed_related_part_entries.clear();
    this->removed_archive_entries.clear();
    this->document.reset();
    this->document_relationships.reset();
    this->content_types.reset();
    this->settings.reset();
    this->numbering.reset();
    this->styles.reset();
    this->header_parts.clear();
    this->footer_parts.clear();
    this->image_parts.clear();
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
    this->table.set_owner(this);
    this->table.set_parent(body);
    this->content_types_loaded = true;
    this->flag_is_open = true;
    return {};
}

bool Document::append_section(bool inherit_header_footer) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a section");
        return false;
    }

    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return false;
    }

    auto body_section_properties = body.child("w:sectPr");
    if (body_section_properties == pugi::xml_node{}) {
        body_section_properties = body.append_child("w:sectPr");
    }
    if (body_section_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create body-level w:sectPr while appending a section",
                       std::string{document_xml_entry});
        return false;
    }

    const auto first_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::first_page);
    const bool body_has_first_page_header = section_has_reference_type(
        body_section_properties, "w:headerReference", first_page_reference_type);
    const bool body_has_first_page_footer = section_has_reference_type(
        body_section_properties, "w:footerReference", first_page_reference_type);
    if ((body_has_first_page_header || body_has_first_page_footer) &&
        !this->ensure_title_page_enabled(body_section_properties)) {
        return false;
    }

    auto section_break_paragraph = featherdoc::detail::append_paragraph_node(body);
    if (section_break_paragraph == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to insert a paragraph for the new section break",
                       std::string{document_xml_entry});
        return false;
    }

    auto paragraph_properties = section_break_paragraph.child("w:pPr");
    if (paragraph_properties == pugi::xml_node{}) {
        paragraph_properties = section_break_paragraph.append_child("w:pPr");
    }
    if (paragraph_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pPr for the new section break paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    if (paragraph_properties.append_copy(body_section_properties) == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to copy body-level w:sectPr onto the new section break paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    if (!inherit_header_footer) {
        clear_section_header_footer_references(body_section_properties);
    }

    this->cleanup_first_page_section_markers();
    const auto even_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::even_page);
    const bool has_any_even_header = document_has_reference_type(
        this->document, "w:headerReference", even_page_reference_type);
    const bool has_any_even_footer = document_has_reference_type(
        this->document, "w:footerReference", even_page_reference_type);
    if (has_any_even_header || has_any_even_footer) {
        if (const auto error = this->ensure_even_and_odd_headers_enabled()) {
            return false;
        }
    } else if (const auto error = this->cleanup_even_and_odd_headers_setting()) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::insert_section(std::size_t section_index, bool inherit_header_footer) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before inserting a section");
        return false;
    }

    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return false;
    }

    const auto count = this->section_count();
    if (section_index >= count) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for insertion",
                       std::string{document_xml_entry});
        return false;
    }

    if (section_index + 1U == count) {
        return this->append_section(inherit_header_footer);
    }

    auto source_section_properties = this->section_properties(section_index);
    if (source_section_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the source section break for insertion",
                       std::string{document_xml_entry});
        return false;
    }

    const auto source_break_paragraph = section_break_paragraph_for(source_section_properties);
    if (source_break_paragraph == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, std::make_error_code(std::errc::invalid_argument),
            "the target section does not end with a paragraph-level w:sectPr node",
            std::string{document_xml_entry});
        return false;
    }

    const auto first_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::first_page);
    const bool source_has_first_page_header = section_has_reference_type(
        source_section_properties, "w:headerReference", first_page_reference_type);
    const bool source_has_first_page_footer = section_has_reference_type(
        source_section_properties, "w:footerReference", first_page_reference_type);
    if ((source_has_first_page_header || source_has_first_page_footer) &&
        !this->ensure_title_page_enabled(source_section_properties)) {
        return false;
    }

    auto section_break_paragraph = body.insert_child_after("w:p", source_break_paragraph);
    if (section_break_paragraph == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to insert a paragraph for the new section break",
                       std::string{document_xml_entry});
        return false;
    }

    auto paragraph_properties = section_break_paragraph.child("w:pPr");
    if (paragraph_properties == pugi::xml_node{}) {
        paragraph_properties = section_break_paragraph.append_child("w:pPr");
    }
    if (paragraph_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pPr for the inserted section break paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    auto inserted_section_properties = paragraph_properties.append_copy(source_section_properties);
    if (inserted_section_properties == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, std::make_error_code(std::errc::not_enough_memory),
            "failed to copy w:sectPr onto the inserted section break paragraph",
            std::string{document_xml_entry});
        return false;
    }

    if (!inherit_header_footer) {
        clear_section_header_footer_references(inserted_section_properties);
    }

    this->cleanup_first_page_section_markers();
    const auto even_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::even_page);
    const bool has_any_even_header = document_has_reference_type(
        this->document, "w:headerReference", even_page_reference_type);
    const bool has_any_even_footer = document_has_reference_type(
        this->document, "w:footerReference", even_page_reference_type);
    if (has_any_even_header || has_any_even_footer) {
        if (const auto error = this->ensure_even_and_odd_headers_enabled()) {
            return false;
        }
    } else if (const auto error = this->cleanup_even_and_odd_headers_setting()) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::remove_section(std::size_t section_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing a section");
        return false;
    }

    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return false;
    }

    const auto count = this->section_count();
    if (count <= 1U) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "cannot remove the only section in the document",
                       std::string{document_xml_entry});
        return false;
    }

    if (section_index >= count) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for removal",
                       std::string{document_xml_entry});
        return false;
    }

    if (section_index + 1U < count) {
        auto section_properties = this->section_properties(section_index);
        if (section_properties == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "failed to resolve the section break for removal",
                           std::string{document_xml_entry});
            return false;
        }

        auto paragraph_properties = section_properties.parent();
        auto section_break_paragraph_node = section_break_paragraph_for(section_properties);
        if (paragraph_properties == pugi::xml_node{} ||
            section_break_paragraph_node == pugi::xml_node{} ||
            !paragraph_properties.remove_child(section_properties)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to remove the section break from the target paragraph",
                           std::string{document_xml_entry});
            return false;
        }

        remove_empty_paragraph(section_break_paragraph_node);
    } else {
        auto previous_section_properties = this->section_properties(section_index - 1U);
        auto body_section_properties = body.child("w:sectPr");
        if (previous_section_properties == pugi::xml_node{} ||
            body_section_properties == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "failed to resolve the final section break for removal",
                           std::string{document_xml_entry});
            return false;
        }

        if (!replace_section_properties_contents(body_section_properties,
                                                previous_section_properties)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to copy the previous section properties onto body-level w:sectPr",
                           std::string{document_xml_entry});
            return false;
        }

        auto previous_paragraph_properties = previous_section_properties.parent();
        auto previous_break_paragraph =
            section_break_paragraph_for(previous_section_properties);
        if (previous_paragraph_properties == pugi::xml_node{} ||
            previous_break_paragraph == pugi::xml_node{} ||
            !previous_paragraph_properties.remove_child(previous_section_properties)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to remove the previous section break while collapsing the final section",
                           std::string{document_xml_entry});
            return false;
        }

        remove_empty_paragraph(previous_break_paragraph);
    }

    this->cleanup_first_page_section_markers();
    if (const auto error = this->cleanup_even_and_odd_headers_setting()) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::move_section(std::size_t source_section_index,
                            std::size_t target_section_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before moving a section");
        return false;
    }

    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return false;
    }

    const auto count = this->section_count();
    if (source_section_index >= count || target_section_index >= count) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for move",
                       std::string{document_xml_entry});
        return false;
    }

    if (source_section_index == target_section_index) {
        this->last_error_info.clear();
        return true;
    }

    auto snapshots = collect_section_snapshots(body);
    if (!snapshots.has_value() || snapshots->size() != count) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to snapshot document sections for reordering",
                       std::string{document_xml_entry});
        return false;
    }

    auto reordered_snapshots = std::move(*snapshots);
    auto moved_snapshot =
        std::move(reordered_snapshots[source_section_index]);
    reordered_snapshots.erase(reordered_snapshots.begin() +
                              static_cast<std::ptrdiff_t>(source_section_index));
    reordered_snapshots.insert(reordered_snapshots.begin() +
                                   static_cast<std::ptrdiff_t>(target_section_index),
                               std::move(moved_snapshot));

    if (!rebuild_body_from_section_snapshots(body, reordered_snapshots)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to rebuild w:body while reordering sections",
                       std::string{document_xml_entry});
        return false;
    }

    this->paragraph.set_parent(body);
    this->table.set_owner(this);
    this->table.set_parent(body);
    this->cleanup_first_page_section_markers();

    const auto even_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::even_page);
    const bool has_any_even_header = document_has_reference_type(
        this->document, "w:headerReference", even_page_reference_type);
    const bool has_any_even_footer = document_has_reference_type(
        this->document, "w:footerReference", even_page_reference_type);
    if (has_any_even_header || has_any_even_footer) {
        if (const auto error = this->ensure_even_and_odd_headers_enabled()) {
            return false;
        }
    } else if (const auto error = this->cleanup_even_and_odd_headers_setting()) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}

void Document::set_path(std::filesystem::path file_path) {
    this->document_path = std::move(file_path);
    this->flag_is_open = false;
    this->has_source_archive = false;
    this->has_document_relationships_part = false;
    this->has_settings_part = false;
    this->has_numbering_part = false;
    this->has_styles_part = false;
    this->document_relationships_dirty = false;
    this->content_types_loaded = false;
    this->content_types_dirty = false;
    this->settings_loaded = false;
    this->settings_dirty = false;
    this->numbering_loaded = false;
    this->numbering_dirty = false;
    this->styles_loaded = false;
    this->styles_dirty = false;
    this->removed_related_part_entries.clear();
    this->removed_archive_entries.clear();
    this->document.reset();
    this->document_relationships.reset();
    this->content_types.reset();
    this->settings.reset();
    this->numbering.reset();
    this->styles.reset();
    this->header_parts.clear();
    this->footer_parts.clear();
    this->image_parts.clear();
    this->detached_paragraph.set_parent({});
    this->last_error_info.clear();
}

const std::filesystem::path &Document::path() const { return this->document_path; }

std::error_code Document::open() {
    this->flag_is_open = false;
    this->has_source_archive = false;
    this->has_document_relationships_part = false;
    this->has_settings_part = false;
    this->has_numbering_part = false;
    this->has_styles_part = false;
    this->document_relationships_dirty = false;
    this->content_types_loaded = false;
    this->content_types_dirty = false;
    this->settings_loaded = false;
    this->settings_dirty = false;
    this->numbering_loaded = false;
    this->numbering_dirty = false;
    this->styles_loaded = false;
    this->styles_dirty = false;
    this->removed_related_part_entries.clear();
    this->removed_archive_entries.clear();
    this->document.reset();
    this->document_relationships.reset();
    this->content_types.reset();
    this->settings.reset();
    this->numbering.reset();
    this->styles.reset();
    this->header_parts.clear();
    this->footer_parts.clear();
    this->image_parts.clear();
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
        part->relationships_entry_name = make_part_relationships_entry(part->entry_name);

        if (const auto error =
                load_related_xml_part(zip, part->entry_name, part->xml, this->last_error_info)) {
            zip_close(zip);
            this->document.reset();
            this->document_relationships.reset();
            this->content_types.reset();
            this->settings.reset();
            this->numbering.reset();
            this->styles.reset();
            this->header_parts.clear();
            this->footer_parts.clear();
            this->image_parts.clear();
            this->flag_is_open = false;
            return error;
        }

        if (const auto error = load_optional_relationships_part(
                zip, part->relationships_entry_name, part->relationships,
                part->has_relationships_part, this->last_error_info)) {
            zip_close(zip);
            this->document.reset();
            this->document_relationships.reset();
            this->content_types.reset();
            this->settings.reset();
            this->numbering.reset();
            this->styles.reset();
            this->header_parts.clear();
            this->footer_parts.clear();
            this->image_parts.clear();
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
    this->table.set_owner(this);
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
                                       docx_output_compression_level, 'w',
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

    const auto collect_referenced_relationship_ids = [&](const char *reference_name) {
        std::unordered_set<std::string> relationship_ids;
        const auto body = this->document.child("w:document").child("w:body");
        if (body == pugi::xml_node{}) {
            return relationship_ids;
        }

        const auto collect_from_section_properties = [&](pugi::xml_node section_properties) {
            for (auto reference = section_properties.child(reference_name);
                 reference != pugi::xml_node{};
                 reference = reference.next_sibling(reference_name)) {
                const auto relationship_id = reference.attribute("r:id").value();
                if (*relationship_id != '\0') {
                    relationship_ids.emplace(relationship_id);
                }
            }
        };

        for (auto child = body.first_child(); child != pugi::xml_node{};
             child = child.next_sibling()) {
            if (std::string_view{child.name()} != "w:p") {
                continue;
            }

            const auto section_properties = child.child("w:pPr").child("w:sectPr");
            if (section_properties != pugi::xml_node{}) {
                collect_from_section_properties(section_properties);
            }
        }

        if (const auto section_properties = body.child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            collect_from_section_properties(section_properties);
        }

        return relationship_ids;
    };

    const auto referenced_header_relationship_ids =
        collect_referenced_relationship_ids("w:headerReference");
    const auto referenced_footer_relationship_ids =
        collect_referenced_relationship_ids("w:footerReference");

    std::vector<const xml_part_state *> active_header_parts;
    active_header_parts.reserve(this->header_parts.size());
    for (const auto &part : this->header_parts) {
        if (referenced_header_relationship_ids.contains(part->relationship_id)) {
            active_header_parts.push_back(part.get());
        }
    }

    std::vector<const xml_part_state *> active_footer_parts;
    active_footer_parts.reserve(this->footer_parts.size());
    for (const auto &part : this->footer_parts) {
        if (referenced_footer_relationship_ids.contains(part->relationship_id)) {
            active_footer_parts.push_back(part.get());
        }
    }

    const bool prune_unused_related_parts =
        active_header_parts.size() != this->header_parts.size() ||
        active_footer_parts.size() != this->footer_parts.size() ||
        !this->removed_related_part_entries.empty();

    std::unordered_set<std::string> active_override_part_names;
    for (const auto *part : active_header_parts) {
        active_override_part_names.insert(make_override_part_name(part->entry_name));
    }
    for (const auto *part : active_footer_parts) {
        active_override_part_names.insert(make_override_part_name(part->entry_name));
    }

    pugi::xml_document document_relationships_to_write;
    const pugi::xml_document *document_relationships_source = &this->document_relationships;
    const bool write_document_relationships =
        this->has_document_relationships_part &&
        (this->document_relationships_dirty || !this->has_source_archive ||
         prune_unused_related_parts);
    if (write_document_relationships && prune_unused_related_parts) {
        document_relationships_to_write.reset(this->document_relationships);
        auto relationships = document_relationships_to_write.child("Relationships");
        for (auto relationship = relationships.child("Relationship");
             relationship != pugi::xml_node{};) {
            const auto next = relationship.next_sibling("Relationship");
            const auto relationship_type =
                std::string_view{relationship.attribute("Type").value()};
            const auto relationship_id = std::string{relationship.attribute("Id").value()};

            const bool remove_header_relationship =
                relationship_type == header_relationship_type &&
                !referenced_header_relationship_ids.contains(relationship_id);
            const bool remove_footer_relationship =
                relationship_type == footer_relationship_type &&
                !referenced_footer_relationship_ids.contains(relationship_id);
            if (remove_header_relationship || remove_footer_relationship) {
                relationships.remove_child(relationship);
            }

            relationship = next;
        }

        document_relationships_source = &document_relationships_to_write;
    }

    pugi::xml_document content_types_to_write;
    const pugi::xml_document *content_types_source = &this->content_types;
    const bool write_content_types =
        this->content_types_dirty || !this->has_source_archive ||
        (prune_unused_related_parts && this->content_types_loaded);
    if (write_content_types && this->content_types_loaded && prune_unused_related_parts) {
        content_types_to_write.reset(this->content_types);
        auto types = content_types_to_write.child("Types");
        for (auto override_node = types.child("Override"); override_node != pugi::xml_node{};) {
            const auto next = override_node.next_sibling("Override");
            const auto content_type =
                std::string_view{override_node.attribute("ContentType").value()};
            const auto part_name =
                std::string{override_node.attribute("PartName").value()};

            const bool is_related_part_override =
                content_type == header_content_type || content_type == footer_content_type;
            if (is_related_part_override &&
                !active_override_part_names.contains(part_name)) {
                types.remove_child(override_node);
            }

            override_node = next;
        }

        content_types_source = &content_types_to_write;
    }

    std::unordered_set<std::string> rewritten_entries;
    rewritten_entries.insert(std::string{document_xml_entry});
    if (write_content_types) {
        rewritten_entries.insert(std::string{content_types_xml_entry});
    }
    if (write_document_relationships) {
        rewritten_entries.insert(std::string{document_relationships_xml_entry});
    }
    if (this->has_settings_part && (this->settings_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(std::string{settings_xml_entry});
    }
    if (this->has_numbering_part && (this->numbering_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(std::string{numbering_xml_entry});
    }
    if (this->has_styles_part && (this->styles_dirty || !this->has_source_archive)) {
        rewritten_entries.insert(std::string{styles_xml_entry});
    }
    for (const auto &part : this->header_parts) {
        rewritten_entries.insert(part->entry_name);
        if (!part->relationships_entry_name.empty()) {
            rewritten_entries.insert(part->relationships_entry_name);
        }
    }
    for (const auto &part : this->footer_parts) {
        rewritten_entries.insert(part->entry_name);
        if (!part->relationships_entry_name.empty()) {
            rewritten_entries.insert(part->relationships_entry_name);
        }
    }
    for (const auto &part : this->image_parts) {
        rewritten_entries.insert(part.entry_name);
    }
    for (const auto &entry_name : this->removed_related_part_entries) {
        rewritten_entries.insert(entry_name);
    }
    for (const auto &entry_name : this->removed_archive_entries) {
        rewritten_entries.insert(entry_name);
    }

    auto write_buffer_entry = [&](std::string_view entry_name, std::string_view entry_content) {
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

    auto write_text_entry = [&](std::string_view entry_name, std::string_view entry_content) {
        write_buffer_entry(entry_name, entry_content);
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

    auto write_related_part_relationships = [&](const xml_part_state &part) {
        if (!part.has_relationships_part || part.relationships_entry_name.empty()) {
            return;
        }

        write_xml_entry(part.relationships_entry_name, part.relationships);
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

    if (!result && write_content_types) {
        if (this->content_types_loaded) {
            write_xml_entry(content_types_xml_entry, *content_types_source);
        } else {
            write_text_entry(content_types_xml_entry, content_types_xml);
        }
    }

    if (!result && write_document_relationships) {
        write_xml_entry(document_relationships_xml_entry, *document_relationships_source);
    }

    if (!result && this->has_settings_part &&
        (this->settings_dirty || !this->has_source_archive)) {
        write_xml_entry(settings_xml_entry, this->settings);
    }

    if (!result && this->has_numbering_part &&
        (this->numbering_dirty || !this->has_source_archive)) {
        write_xml_entry(numbering_xml_entry, this->numbering);
    }

    if (!result && this->has_styles_part &&
        (this->styles_dirty || !this->has_source_archive)) {
        write_xml_entry(styles_xml_entry, this->styles);
    }

    if (!result) {
        for (const auto *part : active_header_parts) {
            write_related_xml_part(*part);
            if (result) {
                break;
            }
            write_related_part_relationships(*part);
            if (result) {
                break;
            }
        }
    }

    if (!result) {
        for (const auto *part : active_footer_parts) {
            write_related_xml_part(*part);
            if (result) {
                break;
            }
            write_related_part_relationships(*part);
            if (result) {
                break;
            }
        }
    }

    if (!result) {
        for (const auto &part : this->image_parts) {
            write_buffer_entry(part.entry_name, part.data);
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
    auto section_properties = this->section_properties(section_index);
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

Document::xml_part_state *Document::section_related_part_state(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind,
    std::vector<std::unique_ptr<xml_part_state>> &parts, const char *reference_name) {
    const auto section_properties = this->section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        return nullptr;
    }

    const auto xml_reference_type = featherdoc::to_xml_reference_type(reference_kind);
    for (auto reference = section_properties.child(reference_name);
         reference != pugi::xml_node{};
         reference = reference.next_sibling(reference_name)) {
        if (std::string_view{reference.attribute("w:type").value()} != xml_reference_type) {
            continue;
        }

        const auto relationship_id = std::string_view{reference.attribute("r:id").value()};
        if (relationship_id.empty()) {
            return nullptr;
        }

        for (auto &part : parts) {
            if (part->relationship_id == relationship_id) {
                return part.get();
            }
        }

        return nullptr;
    }

    return nullptr;
}

Document::xml_part_state *Document::find_related_part_state(std::string_view entry_name) {
    for (auto &part : this->header_parts) {
        if (part->entry_name == entry_name) {
            return part.get();
        }
    }

    for (auto &part : this->footer_parts) {
        if (part->entry_name == entry_name) {
            return part.get();
        }
    }

    return nullptr;
}

const Document::xml_part_state *Document::find_related_part_state(
    std::string_view entry_name) const {
    for (const auto &part : this->header_parts) {
        if (part->entry_name == entry_name) {
            return part.get();
        }
    }

    for (const auto &part : this->footer_parts) {
        if (part->entry_name == entry_name) {
            return part.get();
        }
    }

    return nullptr;
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

std::error_code Document::ensure_settings_loaded() {
    if (this->settings_loaded) {
        return {};
    }

    this->settings.reset();
    this->has_settings_part = false;

    if (!this->has_source_archive) {
        const auto parse_result =
            this->settings.load_buffer(empty_settings_xml.data(), empty_settings_xml.size());
        if (!parse_result) {
            return set_last_error(
                this->last_error_info, document_errc::settings_xml_parse_failed,
                parse_result.description(), std::string{settings_xml_entry},
                parse_result.offset >= 0
                    ? std::optional<std::ptrdiff_t>{parse_result.offset}
                    : std::nullopt);
        }

        this->settings_loaded = true;
        return {};
    }

    const auto relationships = this->document_relationships.child("Relationships");
    if (relationships == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "word/_rels/document.xml.rels does not contain a Relationships root",
                              std::string{document_relationships_xml_entry});
    }

    const auto settings_relationship =
        find_document_relationship_by_type(relationships, settings_relationship_type);
    if (settings_relationship == pugi::xml_node{}) {
        const auto parse_result =
            this->settings.load_buffer(empty_settings_xml.data(), empty_settings_xml.size());
        if (!parse_result) {
            return set_last_error(
                this->last_error_info, document_errc::settings_xml_parse_failed,
                parse_result.description(), std::string{settings_xml_entry},
                parse_result.offset >= 0
                    ? std::optional<std::ptrdiff_t>{parse_result.offset}
                    : std::nullopt);
        }

        this->settings_loaded = true;
        return {};
    }

    const auto target = std::string_view{settings_relationship.attribute("Target").value()};
    if (target.empty()) {
        return set_last_error(this->last_error_info, document_errc::settings_xml_read_failed,
                              "settings relationship does not contain a Target attribute",
                              std::string{document_relationships_xml_entry});
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
                                  "' while loading word/settings.xml: " +
                                  zip_error_text(source_zip_error));
    }

    const auto settings_entry_name = normalize_word_part_entry(target);
    std::string settings_text;
    const auto read_status =
        read_zip_entry_text(source_zip, settings_entry_name, settings_text);
    zip_close(source_zip);

    if (read_status != zip_entry_read_status::ok) {
        return set_last_error(this->last_error_info,
                              document_errc::settings_xml_read_failed,
                              "failed to read required settings part '" +
                                  settings_entry_name + "'",
                              settings_entry_name);
    }

    const auto parse_result =
        this->settings.load_buffer(settings_text.data(), settings_text.size());
    if (!parse_result) {
        return set_last_error(
            this->last_error_info, document_errc::settings_xml_parse_failed,
            parse_result.description(), settings_entry_name,
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

    this->settings_loaded = true;
    this->has_settings_part = true;
    return {};
}

std::error_code Document::ensure_settings_part_attached() {
    if (const auto error = this->ensure_settings_loaded()) {
        return error;
    }

    if (const auto error = this->ensure_content_types_loaded()) {
        return error;
    }

    if (this->document_relationships.child("Relationships") == pugi::xml_node{} &&
        !initialize_empty_relationships_document(this->document_relationships)) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "failed to initialize default relationships XML",
                              std::string{document_relationships_xml_entry});
    }

    auto relationships = this->document_relationships.child("Relationships");
    if (relationships == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "word/_rels/document.xml.rels does not contain a Relationships root",
                              std::string{document_relationships_xml_entry});
    }

    auto types = this->content_types.child("Types");
    if (types == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::content_types_xml_parse_failed,
                              "[Content_Types].xml does not contain a Types root",
                              std::string{content_types_xml_entry});
    }

    auto settings_relationship =
        find_document_relationship_by_type(relationships, settings_relationship_type);
    if (settings_relationship == pugi::xml_node{}) {
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

        settings_relationship = relationships.append_child("Relationship");
        if (settings_relationship == pugi::xml_node{}) {
            return set_last_error(this->last_error_info,
                                  std::make_error_code(std::errc::not_enough_memory),
                                  "failed to create a document relationship for settings.xml",
                                  std::string{document_relationships_xml_entry});
        }

        settings_relationship.append_attribute("Id").set_value(relationship_id.c_str());
        settings_relationship.append_attribute("Type").set_value(
            settings_relationship_type.data());
        const auto relationship_target =
            make_document_relationship_target(settings_xml_entry);
        settings_relationship.append_attribute("Target").set_value(
            relationship_target.c_str());

        this->has_document_relationships_part = true;
        this->document_relationships_dirty = true;
    }

    const auto override_part_name = make_override_part_name(settings_xml_entry);
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
        return set_last_error(this->last_error_info,
                              std::make_error_code(std::errc::not_enough_memory),
                              "failed to append a content type override for settings.xml",
                              std::string{content_types_xml_entry});
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
    content_type_attribute.set_value(settings_content_type.data());

    this->has_settings_part = true;
    this->content_types_dirty = true;
    return {};
}

std::error_code Document::ensure_even_and_odd_headers_enabled() {
    if (const auto error = this->ensure_settings_part_attached()) {
        return error;
    }

    auto settings_root = this->settings.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::settings_xml_parse_failed,
                              "word/settings.xml does not contain the expected w:settings root",
                              std::string{settings_xml_entry});
    }

    if (ensure_on_off_node_enabled(settings_root, "w:evenAndOddHeaders") ==
        pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              std::make_error_code(std::errc::not_enough_memory),
                              "failed to enable w:evenAndOddHeaders in word/settings.xml",
                              std::string{settings_xml_entry});
    }

    this->settings_dirty = true;
    return {};
}

bool Document::ensure_title_page_enabled(pugi::xml_node section_properties) {
    if (section_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "cannot enable first-page header/footer without a w:sectPr node",
                       std::string{document_xml_entry});
        return false;
    }

    if (ensure_section_title_page_node(section_properties) == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to enable w:titlePg for the target section",
                       std::string{document_xml_entry});
        return false;
    }

    return true;
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

    if (reference_kind == featherdoc::section_reference_kind::first_page &&
        !this->ensure_title_page_enabled(section_properties)) {
        return this->detached_paragraph;
    }

    if (reference_kind == featherdoc::section_reference_kind::even_page) {
        if (const auto error = this->ensure_even_and_odd_headers_enabled()) {
            return this->detached_paragraph;
        }
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
    part->relationships_entry_name = make_part_relationships_entry(entry_name);
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

Paragraph &Document::assign_section_related_part_paragraphs(
    std::size_t section_index, std::size_t part_index,
    featherdoc::section_reference_kind reference_kind,
    std::vector<std::unique_ptr<xml_part_state>> &parts, const char *part_root_name,
    const char *reference_name) {
    this->detached_paragraph.set_parent({});

    if (part_index >= parts.size()) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "header/footer part index is out of range for section assignment",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    const auto section_properties = this->ensure_section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        return this->detached_paragraph;
    }

    if (reference_kind == featherdoc::section_reference_kind::first_page &&
        !this->ensure_title_page_enabled(section_properties)) {
        return this->detached_paragraph;
    }

    if (reference_kind == featherdoc::section_reference_kind::even_page) {
        if (const auto error = this->ensure_even_and_odd_headers_enabled()) {
            return this->detached_paragraph;
        }
    }

    auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document root",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    auto relationships_namespace = document_root.attribute("xmlns:r");
    if (relationships_namespace == pugi::xml_attribute{}) {
        relationships_namespace = document_root.append_attribute("xmlns:r");
    }
    relationships_namespace.set_value(
        office_document_relationships_namespace_uri.data());

    const auto xml_reference_type = featherdoc::to_xml_reference_type(reference_kind);
    auto reference =
        find_section_reference(section_properties, reference_name, xml_reference_type);
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

    auto &part = *parts[part_index];
    if (part.relationship_id.empty()) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "target header/footer part does not have a relationship id",
                       part.entry_name);
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
    relationship_attribute.set_value(part.relationship_id.c_str());

    part.paragraph.set_parent(part.xml.child(part_root_name));
    this->last_error_info.clear();
    return part.paragraph;
}

bool Document::remove_section_related_part_reference(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind,
    const char *reference_name) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing header/footer references");
        return false;
    }

    if (const auto error = this->ensure_removal_prerequisites_loaded()) {
        return false;
    }

    if (section_index >= this->section_count()) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for header/footer reference removal",
                       std::string{document_xml_entry});
        return false;
    }

    auto section_properties = this->section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        this->last_error_info.clear();
        return false;
    }

    const auto xml_reference_type = featherdoc::to_xml_reference_type(reference_kind);
    const auto reference =
        find_section_reference(section_properties, reference_name, xml_reference_type);
    if (reference == pugi::xml_node{}) {
        this->last_error_info.clear();
        return false;
    }

    const auto removed = section_properties.remove_child(reference);
    if (!removed) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to remove a section header/footer reference from w:sectPr",
                       std::string{document_xml_entry});
        return false;
    }

    if (reference_kind == featherdoc::section_reference_kind::first_page) {
        this->cleanup_first_page_section_markers();
    }

    if (reference_kind == featherdoc::section_reference_kind::even_page) {
        if (const auto error = this->cleanup_even_and_odd_headers_setting()) {
            return false;
        }
    }

    this->last_error_info.clear();
    return true;
}

std::error_code Document::ensure_removal_prerequisites_loaded() {
    if (const auto error = this->ensure_content_types_loaded()) {
        return error;
    }

    if (this->document_relationships.child("Relationships") == pugi::xml_node{} &&
        !initialize_empty_relationships_document(this->document_relationships)) {
        return set_last_error(this->last_error_info,
                              document_errc::relationships_xml_parse_failed,
                              "failed to initialize default relationships XML",
                              std::string{document_relationships_xml_entry});
    }

    return {};
}

void Document::cleanup_first_page_section_markers() {
    const auto first_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::first_page);
    const auto body = this->document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        return;
    }

    const auto cleanup_section = [&](pugi::xml_node section_properties) {
        if (section_properties == pugi::xml_node{}) {
            return;
        }

        const bool has_first_page_header = section_has_reference_type(
            section_properties, "w:headerReference", first_page_reference_type);
        const bool has_first_page_footer = section_has_reference_type(
            section_properties, "w:footerReference", first_page_reference_type);
        if (!has_first_page_header && !has_first_page_footer) {
            section_properties.remove_child("w:titlePg");
        }
    };

    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        cleanup_section(child.child("w:pPr").child("w:sectPr"));
    }

    cleanup_section(body.child("w:sectPr"));
}

std::error_code Document::cleanup_even_and_odd_headers_setting() {
    const auto even_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::even_page);
    const bool has_any_even_header = document_has_reference_type(
        this->document, "w:headerReference", even_page_reference_type);
    const bool has_any_even_footer = document_has_reference_type(
        this->document, "w:footerReference", even_page_reference_type);
    if (has_any_even_header || has_any_even_footer) {
        return {};
    }

    if (!this->settings_loaded && !this->has_settings_part && !this->has_source_archive) {
        return {};
    }

    if (const auto error = this->ensure_settings_loaded()) {
        return error;
    }

    auto settings_root = this->settings.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        return {};
    }

    if (settings_root.child("w:evenAndOddHeaders") != pugi::xml_node{}) {
        settings_root.remove_child("w:evenAndOddHeaders");
        this->settings_dirty = true;
    }

    return {};
}

bool Document::remove_related_part(
    std::size_t part_index, std::vector<std::unique_ptr<xml_part_state>> &parts,
    const char *reference_name) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing header/footer parts");
        return false;
    }

    if (const auto error = this->ensure_removal_prerequisites_loaded()) {
        return false;
    }

    if (part_index >= parts.size()) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "header/footer part index is out of range for removal",
                       std::string{document_xml_entry});
        return false;
    }

    const auto relationship_id = parts[part_index]->relationship_id;
    const auto entry_name = parts[part_index]->entry_name;
    const auto relationships_entry_name = parts[part_index]->relationships_entry_name;
    bool removed_first_reference = false;
    bool removed_even_reference = false;

    const auto remove_from_section = [&](pugi::xml_node section_properties) {
        for (auto reference = section_properties.child(reference_name);
             reference != pugi::xml_node{};) {
            const auto next = reference.next_sibling(reference_name);
            if (std::string_view{reference.attribute("r:id").value()} == relationship_id) {
                const auto reference_type =
                    std::string_view{reference.attribute("w:type").value()};
                if (reference_type == featherdoc::to_xml_reference_type(
                                          featherdoc::section_reference_kind::first_page)) {
                    removed_first_reference = true;
                } else if (reference_type ==
                           featherdoc::to_xml_reference_type(
                               featherdoc::section_reference_kind::even_page)) {
                    removed_even_reference = true;
                }
                section_properties.remove_child(reference);
            }

            reference = next;
        }
    };

    const auto body = this->document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return false;
    }

    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        if (auto section_properties = child.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            remove_from_section(section_properties);
        }
    }

    if (auto section_properties = body.child("w:sectPr"); section_properties != pugi::xml_node{}) {
        remove_from_section(section_properties);
    }

    if (removed_first_reference) {
        this->cleanup_first_page_section_markers();
    }
    if (removed_even_reference) {
        if (const auto error = this->cleanup_even_and_odd_headers_setting()) {
            return false;
        }
    }

    this->removed_related_part_entries.insert(entry_name);
    if (!relationships_entry_name.empty()) {
        this->removed_related_part_entries.insert(relationships_entry_name);
    }
    parts.erase(parts.begin() + static_cast<std::ptrdiff_t>(part_index));

    this->last_error_info.clear();
    return true;
}

bool Document::copy_section_related_part_references(
    std::size_t source_section_index, std::size_t target_section_index,
    const char *reference_name) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before copying header/footer references");
        return false;
    }

    if (source_section_index >= this->section_count() ||
        target_section_index >= this->section_count()) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for header/footer reference copy",
                       std::string{document_xml_entry});
        return false;
    }

    if (source_section_index == target_section_index) {
        this->last_error_info.clear();
        return true;
    }

    auto target_section_properties = this->ensure_section_properties(target_section_index);
    if (target_section_properties == pugi::xml_node{}) {
        return false;
    }

    const auto source_section_properties = this->section_properties(source_section_index);
    struct copied_reference_state {
        std::string type;
        std::string relationship_id;
    };
    std::vector<copied_reference_state> copied_references;
    if (source_section_properties != pugi::xml_node{}) {
        for (auto reference = source_section_properties.child(reference_name);
             reference != pugi::xml_node{}; reference = reference.next_sibling(reference_name)) {
            copied_references.push_back(copied_reference_state{
                reference.attribute("w:type").value(), reference.attribute("r:id").value()});
        }
    }

    auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document root",
                       std::string{document_xml_entry});
        return false;
    }

    auto relationships_namespace = document_root.attribute("xmlns:r");
    if (relationships_namespace == pugi::xml_attribute{}) {
        relationships_namespace = document_root.append_attribute("xmlns:r");
    }
    relationships_namespace.set_value(
        office_document_relationships_namespace_uri.data());

    for (auto reference = target_section_properties.child(reference_name);
         reference != pugi::xml_node{};) {
        const auto next = reference.next_sibling(reference_name);
        target_section_properties.remove_child(reference);
        reference = next;
    }

    for (const auto &copied_reference : copied_references) {
        auto reference = append_section_reference(target_section_properties, reference_name);
        if (reference == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to attach a copied section header/footer reference to w:sectPr",
                           std::string{document_xml_entry});
            return false;
        }

        auto type_attribute = reference.attribute("w:type");
        if (type_attribute == pugi::xml_attribute{}) {
            type_attribute = reference.append_attribute("w:type");
        }
        type_attribute.set_value(copied_reference.type.c_str());

        auto relationship_attribute = reference.attribute("r:id");
        if (relationship_attribute == pugi::xml_attribute{}) {
            relationship_attribute = reference.append_attribute("r:id");
        }
        relationship_attribute.set_value(copied_reference.relationship_id.c_str());
    }

    this->cleanup_first_page_section_markers();
    const auto first_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::first_page);
    const bool target_has_first_page_header = section_has_reference_type(
        target_section_properties, "w:headerReference", first_page_reference_type);
    const bool target_has_first_page_footer = section_has_reference_type(
        target_section_properties, "w:footerReference", first_page_reference_type);
    if ((target_has_first_page_header || target_has_first_page_footer) &&
        !this->ensure_title_page_enabled(target_section_properties)) {
        return false;
    }

    const auto even_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::even_page);
    const bool has_any_even_header = document_has_reference_type(
        this->document, "w:headerReference", even_page_reference_type);
    const bool has_any_even_footer = document_has_reference_type(
        this->document, "w:footerReference", even_page_reference_type);
    if (has_any_even_header || has_any_even_footer) {
        if (const auto error = this->ensure_even_and_odd_headers_enabled()) {
            return false;
        }
    } else if (const auto error = this->cleanup_even_and_odd_headers_setting()) {
        return false;
    }

    this->last_error_info.clear();
    return true;
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

Paragraph &Document::assign_section_header_paragraphs(
    std::size_t section_index, std::size_t header_index,
    featherdoc::section_reference_kind reference_kind) {
    return this->assign_section_related_part_paragraphs(
        section_index, header_index, reference_kind, this->header_parts, "w:hdr",
        "w:headerReference");
}

Paragraph &Document::assign_section_footer_paragraphs(
    std::size_t section_index, std::size_t footer_index,
    featherdoc::section_reference_kind reference_kind) {
    return this->assign_section_related_part_paragraphs(
        section_index, footer_index, reference_kind, this->footer_parts, "w:ftr",
        "w:footerReference");
}

bool Document::remove_section_header_reference(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    return this->remove_section_related_part_reference(
        section_index, reference_kind, "w:headerReference");
}

bool Document::remove_section_footer_reference(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    return this->remove_section_related_part_reference(
        section_index, reference_kind, "w:footerReference");
}

bool Document::remove_header_part(std::size_t index) {
    return this->remove_related_part(index, this->header_parts, "w:headerReference");
}

bool Document::remove_footer_part(std::size_t index) {
    return this->remove_related_part(index, this->footer_parts, "w:footerReference");
}

bool Document::copy_section_header_references(std::size_t source_section_index,
                                              std::size_t target_section_index) {
    return this->copy_section_related_part_references(
        source_section_index, target_section_index, "w:headerReference");
}

bool Document::copy_section_footer_references(std::size_t source_section_index,
                                              std::size_t target_section_index) {
    return this->copy_section_related_part_references(
        source_section_index, target_section_index, "w:footerReference");
}

bool Document::replace_section_header_text(
    std::size_t section_index, std::string_view replacement_text,
    featherdoc::section_reference_kind reference_kind) {
    this->ensure_section_header_paragraphs(section_index, reference_kind);

    auto *part = this->section_related_part_state(section_index, reference_kind,
                                                  this->header_parts,
                                                  "w:headerReference");
    if (part == nullptr) {
        if (!this->last_error_info) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "failed to resolve the requested section header part",
                           std::string{document_xml_entry});
        }
        return false;
    }

    auto root = part->xml.child("w:hdr");
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::related_part_parse_failed,
                       "header part does not contain the expected w:hdr root",
                       part->entry_name);
        return false;
    }

    root.remove_children();
    for (const auto &paragraph_text : split_plain_text_paragraphs(replacement_text)) {
        if (!featherdoc::detail::append_plain_text_paragraph(root, paragraph_text)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to rebuild the requested section header paragraphs",
                           part->entry_name);
            return false;
        }
    }

    part->paragraph.set_parent(root);
    this->last_error_info.clear();
    return true;
}

bool Document::replace_section_footer_text(
    std::size_t section_index, std::string_view replacement_text,
    featherdoc::section_reference_kind reference_kind) {
    this->ensure_section_footer_paragraphs(section_index, reference_kind);

    auto *part = this->section_related_part_state(section_index, reference_kind,
                                                  this->footer_parts,
                                                  "w:footerReference");
    if (part == nullptr) {
        if (!this->last_error_info) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "failed to resolve the requested section footer part",
                           std::string{document_xml_entry});
        }
        return false;
    }

    auto root = part->xml.child("w:ftr");
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::related_part_parse_failed,
                       "footer part does not contain the expected w:ftr root",
                       part->entry_name);
        return false;
    }

    root.remove_children();
    for (const auto &paragraph_text : split_plain_text_paragraphs(replacement_text)) {
        if (!featherdoc::detail::append_plain_text_paragraph(root, paragraph_text)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to rebuild the requested section footer paragraphs",
                           part->entry_name);
            return false;
        }
    }

    part->paragraph.set_parent(root);
    this->last_error_info.clear();
    return true;
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

Paragraph &Document::paragraphs() {
    this->paragraph.set_parent(document.child("w:document").child("w:body"));
    return this->paragraph;
}

Table &Document::tables() {
    this->table.set_owner(this);
    this->table.set_parent(document.child("w:document").child("w:body"));
    return this->table;
}

Table Document::append_table(std::size_t row_count, std::size_t column_count) {
    const auto body = document.child("w:document").child("w:body");
    const auto table_node = detail::append_table_node(body);
    auto created_table = Table(body, table_node);
    created_table.set_owner(this);

    for (std::size_t row_index = 0; row_index < row_count; ++row_index) {
        created_table.append_row(column_count);
    }

    return created_table;
}

} // namespace featherdoc
