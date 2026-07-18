#pragma once

#include <featherdoc/document_core.hpp>
#include <featherdoc/reviews_fields.hpp>
#include <featherdoc/styles_numbering.hpp>
#include <featherdoc/tables.hpp>
#include <featherdoc/template_part.hpp>
#include <featherdoc/templates.hpp>
#include <featherdoc/text.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

#include <pugixml.hpp>

namespace featherdoc {
namespace detail {
struct document_extension_state;
struct singleton_part_attachment_work;
}

// Document contains whole the docx file
// and stores paragraphs
class Document {
  private:
    friend class Table;
    friend class TemplatePart;

    struct xml_part_state {
        std::string relationship_id;
        std::string entry_name;
        std::string relationships_entry_name;
        pugi::xml_document xml;
        pugi::xml_document relationships;
        Paragraph paragraph;
        bool has_relationships_part{false};
        bool relationships_dirty{false};
    };

    [[nodiscard]] bool
    related_part_has_relationship_id(const xml_part_state &part,
                                     std::string_view relationship_id) const;
    [[nodiscard]] const std::vector<std::string> &
    related_part_relationship_alias_ids(const xml_part_state &part) const;

#include <featherdoc/document_image_private_members.inc>
    [[nodiscard]] std::error_code ensure_content_types_loaded();
    [[nodiscard]] std::error_code ensure_settings_loaded();
    [[nodiscard]] std::error_code ensure_settings_part_attached();
    [[nodiscard]] std::error_code ensure_numbering_loaded();
    [[nodiscard]] std::error_code ensure_numbering_part_attached();
    [[nodiscard]] std::error_code ensure_styles_loaded();
    [[nodiscard]] std::error_code prepare_styles_part_attachment(
        detail::singleton_part_attachment_work &work);
    void publish_styles_part_attachment(
        detail::singleton_part_attachment_work &&work) noexcept;
    [[nodiscard]] std::error_code ensure_styles_part_attached();
    [[nodiscard]] bool
    mutate_style_identity_transaction(std::string_view old_style_id,
                                      std::string_view new_style_id,
                                      bool remove_old_style);
    [[nodiscard]] bool mutate_style_identities_transaction(
        const std::vector<featherdoc::style_refactor_request> &operations);
    [[nodiscard]] std::optional<featherdoc::style_refactor_restore_result>
    restore_style_refactor(
        const std::vector<featherdoc::style_refactor_rollback_entry>
            &rollback_entries,
        bool apply_changes);
    [[nodiscard]] std::optional<bool> inspect_update_fields_on_open_enabled();
#include <featherdoc/document_sections_private_members.inc>
    [[nodiscard]] std::vector<featherdoc::hyperlink_summary>
    list_hyperlinks_in_part(pugi::xml_document &xml_document,
                            std::string_view entry_name) const;
    [[nodiscard]] std::size_t
    append_hyperlink_in_part(pugi::xml_document &xml_document,
                             std::string_view entry_name, std::string_view text,
                             std::string_view target);
    [[nodiscard]] bool
    replace_hyperlink_in_part(pugi::xml_document &xml_document,
                              std::string_view entry_name,
                              std::size_t hyperlink_index,
                              std::string_view text, std::string_view target);
    [[nodiscard]] bool
    remove_hyperlink_in_part(pugi::xml_document &xml_document,
                             std::string_view entry_name,
                             std::size_t hyperlink_index);
    [[nodiscard]] bool
    ensure_review_notes_part(featherdoc::review_note_kind kind);
    [[nodiscard]] bool
    ensure_review_notes_part(featherdoc::review_note_kind kind,
                             bool create_if_missing);
    [[nodiscard]] bool ensure_comments_part();
    [[nodiscard]] bool ensure_comments_part(bool create_if_missing);
    [[nodiscard]] bool ensure_comments_extended_loaded();
    [[nodiscard]] bool ensure_comments_extended_part();
    [[nodiscard]] std::optional<std::int64_t>
    reserve_review_revision_identifiers(std::size_t count);

    friend class IteratorHelper;
    friend class TemplatePart;
    std::filesystem::path document_path;
    Paragraph paragraph;
    Paragraph detached_paragraph;
    Table table;
    pugi::xml_document document;
    pugi::xml_document package_relationships;
    pugi::xml_document document_relationships;
    pugi::xml_document content_types;
    pugi::xml_document settings;
    pugi::xml_document numbering;
    pugi::xml_document styles;
    pugi::xml_document footnotes;
    pugi::xml_document endnotes;
    pugi::xml_document comments;
    pugi::xml_document comments_extended;
    std::shared_ptr<detail::xml_handle_lifetime> xml_handle_lifetime{
        std::make_shared<detail::xml_handle_lifetime>()};
    std::vector<std::unique_ptr<xml_part_state>> header_parts;
    std::vector<std::unique_ptr<xml_part_state>> footer_parts;
    std::vector<image_part_state> image_parts;
    bool flag_is_open{false};
    bool has_source_archive{false};
    bool package_relationships_loaded{false};
    bool package_relationships_dirty{false};
    bool package_repair_pending{false};
    bool has_document_relationships_part{false};
    bool has_settings_part{false};
    bool has_numbering_part{false};
    bool has_styles_part{false};
    bool has_footnotes_part{false};
    bool has_endnotes_part{false};
    bool has_comments_part{false};
    bool has_comments_extended_part{false};
    bool document_relationships_dirty{false};
    bool content_types_loaded{false};
    bool content_types_dirty{false};
    bool settings_loaded{false};
    bool settings_dirty{false};
    bool numbering_loaded{false};
    bool numbering_dirty{false};
    bool styles_loaded{false};
    bool styles_dirty{false};
    bool footnotes_loaded{false};
    bool footnotes_dirty{false};
    bool endnotes_loaded{false};
    bool endnotes_dirty{false};
    bool comments_loaded{false};
    bool comments_dirty{false};
    bool comments_extended_loaded{false};
    bool comments_extended_dirty{false};
    mutable std::unordered_set<std::string> removed_related_part_entries;
    std::unordered_set<std::string> removed_archive_entries;
    std::vector<package_diagnostic> package_diagnostic_list;
    archive_limits opened_archive_limits{};
    mutable document_error_info last_error_info;

    [[nodiscard]] bool
    source_package_part_name_conflicts(std::string_view entry_name) const;
    [[nodiscard]] detail::document_extension_state &extension_state();
    [[nodiscard]] const detail::document_extension_state &
    extension_state() const;
    void ensure_xml_handle_lifetime();

    [[nodiscard]] auto tracked_node(pugi::xml_node node) const
        -> detail::tracked_xml_node {
        const_cast<Document *>(this)->ensure_xml_handle_lifetime();
        return {node, this->xml_handle_lifetime};
    }

    void invalidate_xml_handles() {
        this->ensure_xml_handle_lifetime();
        this->xml_handle_lifetime->invalidate();
    }

    void rebind_internal_xml_handles();
    void reset_moved_from_state();
    void finish_move_construction_from(Document &other) noexcept;
    void move_assign_from(Document &other) noexcept;

    [[nodiscard]] bool ignored_singleton_relationship_type(
        std::string_view relationship_type) const;
    [[nodiscard]] bool owns_paragraph_handle(
        const Paragraph &paragraph,
        std::string_view &owning_part_entry_name) const noexcept;
    [[nodiscard]] bool owns_run_handle(
        const Run &run,
        std::string_view &owning_part_entry_name) const noexcept;

  public:
    Document();
    explicit Document(std::filesystem::path);
    Document(const Document &) = delete;
    Document &operator=(const Document &) = delete;
    // Moving transfers package state, invalidates handles previously returned
    // by both objects, and leaves the source closed but reusable.
    // Keep the special member inline, like the implicit v1.13 implementation,
    // so old-header static consumers do not collide with a new strong symbol.
    Document(Document &&other) noexcept
        : document_path(std::move(other.document_path)),
          paragraph(std::move(other.paragraph)),
          detached_paragraph(std::move(other.detached_paragraph)),
          table(std::move(other.table)), document(std::move(other.document)),
          package_relationships(std::move(other.package_relationships)),
          document_relationships(std::move(other.document_relationships)),
          content_types(std::move(other.content_types)),
          settings(std::move(other.settings)),
          numbering(std::move(other.numbering)),
          styles(std::move(other.styles)),
          footnotes(std::move(other.footnotes)),
          endnotes(std::move(other.endnotes)),
          comments(std::move(other.comments)),
          comments_extended(std::move(other.comments_extended)),
          xml_handle_lifetime(std::move(other.xml_handle_lifetime)),
          header_parts(std::move(other.header_parts)),
          footer_parts(std::move(other.footer_parts)),
          image_parts(std::move(other.image_parts)),
          flag_is_open(other.flag_is_open),
          has_source_archive(other.has_source_archive),
          package_relationships_loaded(other.package_relationships_loaded),
          package_relationships_dirty(other.package_relationships_dirty),
          package_repair_pending(other.package_repair_pending),
          has_document_relationships_part(
              other.has_document_relationships_part),
          has_settings_part(other.has_settings_part),
          has_numbering_part(other.has_numbering_part),
          has_styles_part(other.has_styles_part),
          has_footnotes_part(other.has_footnotes_part),
          has_endnotes_part(other.has_endnotes_part),
          has_comments_part(other.has_comments_part),
          has_comments_extended_part(other.has_comments_extended_part),
          document_relationships_dirty(other.document_relationships_dirty),
          content_types_loaded(other.content_types_loaded),
          content_types_dirty(other.content_types_dirty),
          settings_loaded(other.settings_loaded),
          settings_dirty(other.settings_dirty),
          numbering_loaded(other.numbering_loaded),
          numbering_dirty(other.numbering_dirty),
          styles_loaded(other.styles_loaded), styles_dirty(other.styles_dirty),
          footnotes_loaded(other.footnotes_loaded),
          footnotes_dirty(other.footnotes_dirty),
          endnotes_loaded(other.endnotes_loaded),
          endnotes_dirty(other.endnotes_dirty),
          comments_loaded(other.comments_loaded),
          comments_dirty(other.comments_dirty),
          comments_extended_loaded(other.comments_extended_loaded),
          comments_extended_dirty(other.comments_extended_dirty),
          removed_related_part_entries(
              std::move(other.removed_related_part_entries)),
          removed_archive_entries(std::move(other.removed_archive_entries)),
          package_diagnostic_list(std::move(other.package_diagnostic_list)),
          opened_archive_limits(other.opened_archive_limits),
          last_error_info(std::move(other.last_error_info)) {
        this->finish_move_construction_from(other);
    }
    Document &operator=(Document &&other) noexcept {
        if (this != &other) {
            this->move_assign_from(other);
        }
        return *this;
    }
    [[nodiscard]] std::error_code create_empty();
    void set_path(std::filesystem::path);
    [[nodiscard]] const std::filesystem::path &path() const;
    // Reloading/resetting the package invalidates all previously returned
    // XML-backed handles, including paragraphs, runs, tables, rows, cells, and
    // template parts.
    [[nodiscard]] std::error_code open();
    [[nodiscard]] std::error_code open(const document_open_options &options);
    [[nodiscard]] const std::vector<package_diagnostic> &
    package_diagnostics() const noexcept;
    [[nodiscard]] std::optional<package_repair_report>
    repair_package(const document_repair_options &options = {});
    [[nodiscard]] bool enable_update_fields_on_open();
    [[nodiscard]] bool clear_update_fields_on_open();
    [[nodiscard]] std::optional<bool> update_fields_on_open_enabled();
    [[nodiscard]] std::error_code save() const;
    [[nodiscard]] std::error_code save_as(std::filesystem::path) const;
    [[nodiscard]] bool is_open() const;
    [[nodiscard]] const document_error_info &last_error() const noexcept;
#include <featherdoc/document_body_content_members.inc>
#include <featherdoc/document_image_members.inc>
#include <featherdoc/document_reviews_fields_members.inc>
#include <featherdoc/document_sections_members.inc>
#include <featherdoc/document_styles_numbering_members.inc>
#include <featherdoc/document_templates_members.inc>
};

} // namespace featherdoc
