#include "featherdoc.hpp"

#include <array>
#include <cstdlib>
#include <string_view>
#include <utility>

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};
constexpr auto relationships_xml_entry = std::string_view{"_rels/.rels"};
constexpr auto content_types_xml_entry = std::string_view{"[Content_Types].xml"};
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

struct packaged_entry final {
    std::string_view name;
    std::string_view content;
};

constexpr auto minimal_docx_entries = std::array{
    packaged_entry{content_types_xml_entry, content_types_xml},
    packaged_entry{relationships_xml_entry, relationships_xml},
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
} // namespace

namespace featherdoc {

Document::Document() = default;

Document::Document(std::filesystem::path file_path)
    : document_path(std::move(file_path)) {}

std::error_code Document::create_empty() {
    this->flag_is_open = false;
    this->has_source_archive = false;
    this->document.reset();
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

    auto body = this->document.child("w:document").child("w:body");
    this->paragraph.set_parent(body);
    this->table.set_parent(body);
    this->flag_is_open = true;
    return {};
}

void Document::set_path(std::filesystem::path file_path) {
    this->document_path = std::move(file_path);
    this->flag_is_open = false;
    this->has_source_archive = false;
    this->document.reset();
    this->last_error_info.clear();
}

const std::filesystem::path &Document::path() const { return this->document_path; }

std::error_code Document::open() {
    this->flag_is_open = false;
    this->has_source_archive = false;
    this->document.reset();
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
    zip_close(zip);

    // Keep zip-owned buffers on the zip side to avoid allocator/ABI mismatches
    // when FeatherDoc is consumed through shared-library boundaries.
    const auto parse_result = this->document.load_buffer(buf, bufsize);
    std::free(buf);
    buf = nullptr;

    this->flag_is_open = static_cast<bool>(parse_result);
    if (!this->flag_is_open) {
        this->document.reset();
        return set_last_error(
            this->last_error_info, document_errc::document_xml_parse_failed,
            parse_result.description(), std::string{document_xml_entry},
            parse_result.offset >= 0 ? std::optional<std::ptrdiff_t>{parse_result.offset}
                                     : std::nullopt);
    }

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

    if (!result && !this->has_source_archive) {
        for (const auto &[entry_name, entry_content] : minimal_docx_entries) {
            if (zip_entry_open(new_zip, entry_name.data()) != 0) {
                record_first_document_error(
                    document_errc::output_entry_open_failed,
                    "failed to create output entry '" + std::string{entry_name} + "'",
                    std::string{entry_name});
                break;
            }

            if (zip_entry_write(new_zip, entry_content.data(), entry_content.size()) <
                    0 ||
                zip_entry_close(new_zip) != 0) {
                record_first_document_error(
                    document_errc::output_entry_write_failed,
                    "failed to write output entry '" + std::string{entry_name} + "'",
                    std::string{entry_name});
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
                if (std::string_view{name} != document_xml_entry) {
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

Paragraph &Document::paragraphs() {
    this->paragraph.set_parent(document.child("w:document").child("w:body"));
    return this->paragraph;
}

Table &Document::tables() {
    this->table.set_parent(document.child("w:document").child("w:body"));
    return this->table;
}

} // namespace featherdoc
