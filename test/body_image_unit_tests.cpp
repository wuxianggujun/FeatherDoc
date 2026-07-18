#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

#ifndef _WIN32
#include <sys/stat.h>
#endif

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "basic_document_xml_test_support.hpp"
#include "basic_docx_archive_test_support.hpp"
#include "basic_image_fixture_test_support.hpp"
#include "doctest.h"

#include "image_helpers.hpp"
#include <featherdoc.hpp>
#include <featherdoc/detail/path.hpp>

TEST_CASE("external image files enforce bounded UTF-8 input sizes") {
    namespace fs = std::filesystem;

    constexpr std::uint64_t limit = 4096U;
    const auto below_path =
        fs::current_path() /
        featherdoc::detail::path_from_utf8("图片输入上限-减一.png");
    const auto exact_path =
        fs::current_path() /
        featherdoc::detail::path_from_utf8("图片输入上限-恰好.png");
    const auto above_path =
        fs::current_path() /
        featherdoc::detail::path_from_utf8("图片输入上限-加一.png");
    fs::remove(below_path);
    fs::remove(exact_path);
    fs::remove(above_path);

    const auto sized_png = [](std::size_t size) {
        auto data = tiny_png_data();
        data.resize(size, '\0');
        return data;
    };
    write_binary_file(below_path,
                      sized_png(static_cast<std::size_t>(limit - 1U)));
    write_binary_file(exact_path, sized_png(static_cast<std::size_t>(limit)));
    write_binary_file(above_path,
                      sized_png(static_cast<std::size_t>(limit + 1U)));

    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;
    CHECK(featherdoc::detail::load_image_file(below_path, image_info,
                                              error_code, detail, limit));
    CHECK_EQ(image_info.data.size(), limit - 1U);
    CHECK_EQ(error_code, featherdoc::document_errc::success);

    CHECK(featherdoc::detail::load_image_file(exact_path, image_info,
                                              error_code, detail, limit));
    CHECK_EQ(image_info.data.size(), limit);
    CHECK_EQ(error_code, featherdoc::document_errc::success);

    CHECK_FALSE(featherdoc::detail::load_image_file(above_path, image_info,
                                                    error_code, detail, limit));
    CHECK_EQ(error_code, featherdoc::document_errc::image_input_limit_exceeded);
    CHECK(image_info.data.empty());
    CHECK_NE(detail.find("actual_bytes=4097"), std::string::npos);
    CHECK_NE(detail.find("limit_bytes=4096"), std::string::npos);
    CHECK_NE(detail.find(featherdoc::detail::path_to_utf8(above_path)),
             std::string::npos);

    fs::remove(below_path);
    fs::remove(exact_path);
    fs::remove(above_path);
}

TEST_CASE("external image stream hard limit covers metadata races and read "
          "failures") {
    namespace fs = std::filesystem;

    constexpr std::uint64_t limit = 64U;
    const auto image_path =
        fs::current_path() /
        featherdoc::detail::path_from_utf8("读取增长-图片.png");
    auto error_code = featherdoc::document_errc::success;
    std::string detail;
    std::string data{"stale"};

    std::istringstream growing_stream(std::string(limit + 1U, 'x'));
    CHECK_FALSE(featherdoc::detail::read_image_stream_with_limit(
        growing_stream, image_path, limit, data, error_code, detail));
    CHECK_EQ(error_code, featherdoc::document_errc::image_input_limit_exceeded);
    CHECK_EQ(featherdoc::make_error_code(error_code).message(),
             "external image input limit exceeded");
    CHECK_EQ(featherdoc::make_error_code(error_code).message().find("DOCX"),
             std::string::npos);
    CHECK(data.empty());
    CHECK_NE(detail.find("actual_bytes=65"), std::string::npos);
    CHECK_NE(detail.find("limit_bytes=64"), std::string::npos);

    std::istringstream failed_stream("unreadable");
    failed_stream.setstate(std::ios::badbit);
    CHECK_FALSE(featherdoc::detail::read_image_stream_with_limit(
        failed_stream, image_path, limit, data, error_code, detail));
    CHECK_EQ(error_code, featherdoc::document_errc::image_file_read_failed);
    CHECK(data.empty());
    CHECK_NE(detail.find(featherdoc::detail::path_to_utf8(image_path)),
             std::string::npos);
}

TEST_CASE("external image loader accepts only supported regular files") {
    namespace fs = std::filesystem;

    const auto regular_path =
        fs::current_path() /
        featherdoc::detail::path_from_utf8("普通图片-链接目标.PNG");
    const auto missing_path = fs::current_path() / "missing_image.png";
    const auto directory_path = fs::current_path() / "image_directory.png";
    const auto regular_link = fs::current_path() / "regular_image_link.PnG";
    const auto directory_link = fs::current_path() / "directory_image_link.png";

    std::error_code cleanup_error;
    fs::remove(regular_link, cleanup_error);
    cleanup_error.clear();
    fs::remove(directory_link, cleanup_error);
    cleanup_error.clear();
    fs::remove(regular_path, cleanup_error);
    cleanup_error.clear();
    fs::remove(missing_path, cleanup_error);
    cleanup_error.clear();
    fs::remove(directory_path, cleanup_error);

    write_binary_file(regular_path, tiny_png_data());
    REQUIRE(fs::create_directory(directory_path));

    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;

    REQUIRE(featherdoc::detail::load_image_file(regular_path, image_info,
                                                error_code, detail));
    CHECK_EQ(image_info.extension, "png");
    CHECK_EQ(error_code, featherdoc::document_errc::success);

    CHECK_FALSE(featherdoc::detail::load_image_file(missing_path, image_info,
                                                    error_code, detail));
    CHECK_EQ(error_code, featherdoc::document_errc::image_file_status_failed);
    CHECK_NE(detail.find(featherdoc::detail::path_to_utf8(missing_path)),
             std::string::npos);

    CHECK_FALSE(featherdoc::detail::load_image_file(directory_path, image_info,
                                                    error_code, detail));
    CHECK_EQ(error_code, featherdoc::document_errc::image_file_not_regular);
    CHECK_NE(detail.find(featherdoc::detail::path_to_utf8(directory_path)),
             std::string::npos);

    std::error_code regular_link_error;
    fs::create_symlink(regular_path, regular_link, regular_link_error);
#ifndef _WIN32
    REQUIRE_FALSE(regular_link_error);
#endif
    if (!regular_link_error) {
        REQUIRE(featherdoc::detail::load_image_file(regular_link, image_info,
                                                    error_code, detail));
        CHECK_EQ(image_info.extension, "png");
        CHECK_EQ(error_code, featherdoc::document_errc::success);
    }

    std::error_code directory_link_error;
    fs::create_directory_symlink(directory_path, directory_link,
                                 directory_link_error);
#ifndef _WIN32
    REQUIRE_FALSE(directory_link_error);
#endif
    if (!directory_link_error) {
        CHECK_FALSE(featherdoc::detail::load_image_file(
            directory_link, image_info, error_code, detail));
        CHECK_EQ(error_code, featherdoc::document_errc::image_file_not_regular);
    }

#ifndef _WIN32
    const auto fifo_path = fs::current_path() / "external_image_fifo.png";
    fs::remove(fifo_path, cleanup_error);
    REQUIRE_EQ(::mkfifo(fifo_path.c_str(), 0600), 0);
    CHECK_FALSE(featherdoc::detail::load_image_file(fifo_path, image_info,
                                                    error_code, detail));
    CHECK_EQ(error_code, featherdoc::document_errc::image_file_not_regular);

    CHECK_FALSE(featherdoc::detail::load_image_file(
        fs::path{"/dev/null"}, image_info, error_code, detail));
    CHECK_EQ(error_code, featherdoc::document_errc::image_file_not_regular);

#ifdef __linux__
    const fs::path process_memory{"/proc/self/mem"};
    std::error_code process_memory_status_error;
    const auto process_memory_status =
        fs::status(process_memory, process_memory_status_error);
    if (!process_memory_status_error &&
        fs::is_regular_file(process_memory_status)) {
        const auto process_memory_link =
            fs::current_path() / "unreadable_process_memory.png";
        fs::remove(process_memory_link, cleanup_error);
        std::error_code process_memory_link_error;
        fs::create_symlink(process_memory, process_memory_link,
                           process_memory_link_error);
        REQUIRE_FALSE(process_memory_link_error);

        CHECK_FALSE(featherdoc::detail::load_image_file(
            process_memory_link, image_info, error_code, detail));
        CHECK_EQ(error_code, featherdoc::document_errc::image_file_read_failed);
        CHECK_NE(detail.find("native_error="), std::string::npos);
        CHECK_NE(detail.find("native_category="), std::string::npos);
        CHECK_NE(
            detail.find(featherdoc::detail::path_to_utf8(process_memory_link)),
            std::string::npos);
        fs::remove(process_memory_link, cleanup_error);
    }
#endif

    fs::remove(fifo_path, cleanup_error);
#endif

    fs::remove(regular_link, cleanup_error);
    cleanup_error.clear();
    fs::remove(directory_link, cleanup_error);
    cleanup_error.clear();
    fs::remove(regular_path, cleanup_error);
    cleanup_error.clear();
    fs::remove(directory_path, cleanup_error);
}

TEST_CASE("unsupported image extensions are rejected before large-file input") {
    namespace fs = std::filesystem;

    const auto unsupported_path =
        fs::current_path() / "unsupported_sparse_image.TxT";
    std::error_code cleanup_error;
    fs::remove(unsupported_path, cleanup_error);
    write_binary_file(unsupported_path, "x");

    std::error_code resize_error;
    fs::resize_file(unsupported_path,
                    featherdoc::detail::max_external_image_file_bytes + 1U,
                    resize_error);
    REQUIRE_FALSE(resize_error);

    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;
    CHECK_FALSE(featherdoc::detail::load_image_file(
        unsupported_path, image_info, error_code, detail));
    CHECK_EQ(error_code, featherdoc::document_errc::image_format_unsupported);
    CHECK(image_info.data.empty());
    CHECK_NE(detail.find(featherdoc::detail::path_to_utf8(unsupported_path)),
             std::string::npos);

    fs::remove(unsupported_path, cleanup_error);
}

TEST_CASE("image I/O rejects embedded NUL paths before OS access") {
    namespace fs = std::filesystem;

    const auto image_prefix = fs::current_path() / "embedded_nul_image.png";
    const auto output_prefix = fs::current_path() / "embedded_nul_extract.png";
    fs::remove(image_prefix);
    fs::remove(output_prefix);
    write_binary_file(image_prefix, tiny_png_data());
    write_binary_file(output_prefix, "preserve output");

    auto image_path_text = featherdoc::detail::path_to_utf8(image_prefix);
    image_path_text.push_back('\0');
    image_path_text += ".ignored";
    const auto image_path = featherdoc::detail::path_from_utf8(image_path_text);

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK_FALSE(document.append_image(image_path));
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::image_file_read_failed);
    CHECK(document.drawing_images().empty());

    REQUIRE(document.append_image(image_prefix));
    auto output_path_text = featherdoc::detail::path_to_utf8(output_prefix);
    output_path_text.push_back('\0');
    output_path_text += ".ignored";
    const auto output_path =
        featherdoc::detail::path_from_utf8(output_path_text);
    CHECK_FALSE(document.extract_drawing_image(0U, output_path));
    CHECK_EQ(document.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    std::string preserved;
    {
        std::ifstream output_stream(output_prefix, std::ios::binary);
        preserved.assign(std::istreambuf_iterator<char>{output_stream},
                         std::istreambuf_iterator<char>{});
    }
    CHECK_EQ(preserved, "preserve output");

    fs::remove(image_prefix);
    fs::remove(output_prefix);
}

TEST_CASE("append_image writes inline media parts and preserves them across "
          "reopen save") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U,
        0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x00U, 0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU,
        0x15U, 0xC4U, 0x89U, 0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U,
        0x54U, 0x78U, 0x9CU, 0x63U, 0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U,
        0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U, 0x00U, 0x00U, 0x00U, 0x00U, 0x49U,
        0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U, 0x82U,
    };

    const fs::path target = fs::current_path() / "append_image_roundtrip.docx";
    const fs::path image_path = fs::current_path() / "tiny_image.png";
    fs::remove(target);
    fs::remove(image_path);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(image_path));
    CHECK(doc.append_image(image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);
    CHECK_EQ(read_test_docx_entry(target, "word/media/image2.png"), image_data);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_relationships,
                 "Type=\"http://schemas.openxmlformats.org/"
                 "officeDocument/2006/relationships/image\""),
             2);
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"media/image2.png\""),
             std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("Extension=\"png\""), std::string::npos);
    CHECK_NE(saved_content_types.find("ContentType=\"image/png\""),
             std::string::npos);

    const auto saved_document_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 2);
    CHECK_NE(saved_document_xml.find("cx=\"9525\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"9525\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cx=\"190500\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"95250\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.paragraphs().add_run("reopened edit").has_next());
    CHECK_FALSE(reopened.save());
    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));

    const auto relationships_after_resave =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(relationships_after_resave.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_NE(relationships_after_resave.find("Target=\"media/image2.png\""),
             std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("image allocation and content types use case-insensitive package "
          "identities") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "image_case_identity_allocation.docx";
    const auto first_image_path =
        fs::current_path() / "image_case_identity_first.png";
    const auto second_image_path =
        fs::current_path() / "image_case_identity_second.png";
    const auto replacement_image_path =
        fs::current_path() / "image_case_identity_replacement.png";
    const auto first_extracted_path =
        fs::current_path() / "image_case_identity_first_extracted.png";
    const auto second_extracted_path =
        fs::current_path() / "image_case_identity_second_extracted.png";
    fs::remove(target);
    fs::remove(first_image_path);
    fs::remove(second_image_path);
    fs::remove(replacement_image_path);
    fs::remove(first_extracted_path);
    fs::remove(second_extracted_path);

    const auto first_image_data = tiny_png_data();
    auto second_image_data = tiny_png_data();
    second_image_data.append("second-image-payload");
    auto replacement_image_data = tiny_png_data();
    replacement_image_data.append("replacement-image-payload");
    write_binary_file(first_image_path, first_image_data);
    write_binary_file(second_image_path, second_image_data);
    write_binary_file(replacement_image_path, replacement_image_data);

    featherdoc::Document created(target);
    REQUIRE_FALSE(created.create_empty());
    REQUIRE(created.append_image(first_image_path));
    REQUIRE_FALSE(created.save());

    auto relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    const auto relationship_target = relationships.find("media/image1.png");
    REQUIRE_NE(relationship_target, std::string::npos);
    relationships.replace(relationship_target,
                          std::string_view{"media/image1.png"}.size(),
                          "MEDIA/IMAGE1.PNG");
    rewrite_test_docx_entry(target, "word/_rels/document.xml.rels",
                            std::move(relationships));

    auto content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    const auto png_extension = content_types.find("Extension=\"png\"");
    REQUIRE_NE(png_extension, std::string::npos);
    content_types.replace(png_extension,
                          std::string_view{"Extension=\"png\""}.size(),
                          "Extension=\"PNG\"");
    rewrite_test_docx_entry(target, test_content_types_xml_entry,
                            std::move(content_types));

    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    REQUIRE(reopened.append_image(second_image_path));

    const auto pending_images = reopened.drawing_images();
    REQUIRE_EQ(pending_images.size(), 2U);
    CHECK_EQ(pending_images[0].entry_name, "word/MEDIA/IMAGE1.PNG");
    CHECK_EQ(pending_images[1].entry_name, "word/media/image2.png");
    REQUIRE_FALSE(reopened.save());

    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"),
             first_image_data);
    CHECK_EQ(read_test_docx_entry(target, "word/media/image2.png"),
             second_image_data);
    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_content_types,
                                         "ContentType=\"image/png\""),
             1U);
    CHECK_EQ(
        count_substring_occurrences(saved_content_types, "Extension=\"PNG\""),
        0U);
    CHECK_EQ(
        count_substring_occurrences(saved_content_types, "Extension=\"png\""),
        1U);

    featherdoc::Document verified(target);
    REQUIRE_FALSE(verified.open());
    REQUIRE_EQ(verified.drawing_images().size(), 2U);
    REQUIRE(verified.extract_drawing_image(0U, first_extracted_path));
    REQUIRE(verified.extract_drawing_image(1U, second_extracted_path));
    CHECK_EQ(read_binary_file(first_extracted_path),
             std::vector<unsigned char>(first_image_data.begin(),
                                        first_image_data.end()));
    CHECK_EQ(read_binary_file(second_extracted_path),
             std::vector<unsigned char>(second_image_data.begin(),
                                        second_image_data.end()));

    REQUIRE(verified.replace_drawing_image(1U, replacement_image_path));
    const auto replaced_images = verified.drawing_images();
    REQUIRE_EQ(replaced_images.size(), 2U);
    CHECK_EQ(replaced_images[0].entry_name, "word/MEDIA/IMAGE1.PNG");
    CHECK_EQ(replaced_images[1].entry_name, "word/media/image3.png");
    REQUIRE_FALSE(verified.save());
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"),
             first_image_data);
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image3.png"),
             replacement_image_data);

    fs::remove(target);
    fs::remove(first_image_path);
    fs::remove(second_image_path);
    fs::remove(replacement_image_path);
    fs::remove(first_extracted_path);
    fs::remove(second_extracted_path);
}

TEST_CASE("image allocation preserves unreferenced source parts and derived "
          "part names") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "image_source_part_identity_allocation.docx";
    const auto seed_image_path =
        fs::current_path() / "image_source_part_seed.png";
    const auto appended_image_path =
        fs::current_path() / "image_source_part_appended.png";
    const auto replacement_image_path =
        fs::current_path() / "image_source_part_replacement.png";
    const auto first_extracted_path =
        fs::current_path() / "image_source_part_first_extracted.png";
    const auto second_extracted_path =
        fs::current_path() / "image_source_part_second_extracted.png";
    fs::remove(target);
    fs::remove(seed_image_path);
    fs::remove(appended_image_path);
    fs::remove(replacement_image_path);
    fs::remove(first_extracted_path);
    fs::remove(second_extracted_path);

    const auto seed_image_data = tiny_png_data();
    auto appended_image_data = tiny_png_data();
    appended_image_data.append("appended-source-identity-image");
    auto replacement_image_data = tiny_png_data();
    replacement_image_data.append("replacement-source-identity-image");
    const std::string orphan_image_payload{"orphan-image-one-payload"};
    const std::string derived_part_payload{"derived-image-two-payload"};
    write_binary_file(seed_image_path, seed_image_data);
    write_binary_file(appended_image_path, appended_image_data);
    write_binary_file(replacement_image_path, replacement_image_data);

    featherdoc::Document created(target);
    REQUIRE_FALSE(created.create_empty());
    REQUIRE(created.append_image(seed_image_path));
    REQUIRE_FALSE(created.save());

    auto entries = read_test_archive_entries(target);
    bool renamed_seed_part = false;
    bool retargeted_seed_relationship = false;
    for (auto &[entry_name, content] : entries) {
        if (entry_name == "word/media/image1.png") {
            entry_name = "word/media/image3.png";
            renamed_seed_part = true;
        } else if (entry_name == "word/_rels/document.xml.rels") {
            const auto target_position = content.find("media/image1.png");
            REQUIRE_NE(target_position, std::string::npos);
            content.replace(target_position,
                            std::string_view{"media/image1.png"}.size(),
                            "media/image3.png");
            retargeted_seed_relationship = true;
        }
    }
    REQUIRE(renamed_seed_part);
    REQUIRE(retargeted_seed_relationship);
    entries.emplace_back("word/media/image1.png", orphan_image_payload);
    entries.emplace_back("word/media/image2.png/descendant.png",
                         derived_part_payload);
    write_test_archive_entries(target, entries);

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.append_image(appended_image_path));
    auto images = document.drawing_images();
    REQUIRE_EQ(images.size(), 2U);
    CHECK_EQ(images[0].entry_name, "word/media/image3.png");
    CHECK_EQ(images[1].entry_name, "word/media/image4.png");

    REQUIRE(document.replace_drawing_image(0U, replacement_image_path));
    images = document.drawing_images();
    REQUIRE_EQ(images.size(), 2U);
    CHECK_EQ(images[0].entry_name, "word/media/image5.png");
    CHECK_EQ(images[1].entry_name, "word/media/image4.png");
    REQUIRE_FALSE(document.save());

    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"),
             orphan_image_payload);
    CHECK_EQ(
        read_test_docx_entry(target, "word/media/image2.png/descendant.png"),
        derived_part_payload);
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image3.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image4.png"),
             appended_image_data);
    CHECK_EQ(read_test_docx_entry(target, "word/media/image5.png"),
             replacement_image_data);

    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    const auto reopened_images = reopened.drawing_images();
    REQUIRE_EQ(reopened_images.size(), 2U);
    CHECK_EQ(reopened_images[0].entry_name, "word/media/image5.png");
    CHECK_EQ(reopened_images[1].entry_name, "word/media/image4.png");
    REQUIRE(reopened.extract_drawing_image(0U, first_extracted_path));
    REQUIRE(reopened.extract_drawing_image(1U, second_extracted_path));
    CHECK_EQ(read_binary_file(first_extracted_path),
             std::vector<unsigned char>(replacement_image_data.begin(),
                                        replacement_image_data.end()));
    CHECK_EQ(read_binary_file(second_extracted_path),
             std::vector<unsigned char>(appended_image_data.begin(),
                                        appended_image_data.end()));

    fs::remove(target);
    fs::remove(seed_image_path);
    fs::remove(appended_image_path);
    fs::remove(replacement_image_path);
    fs::remove(first_extracted_path);
    fs::remove(second_extracted_path);
}

TEST_CASE("removing one image relationship preserves a case and percent "
          "variant reference to the same package part") {
    namespace fs = std::filesystem;

    const auto source =
        fs::current_path() / "image_relationship_identity_variants.docx";
    const auto extracted =
        fs::current_path() / "image_relationship_identity_extracted.png";
    fs::remove(source);
    fs::remove(extracted);

    constexpr auto image_entry = "word/media/%E4%B8%AD%E6%96%87.PNG";
    const auto image_data = tiny_png_data();
    const auto content_types_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Default Extension="PNG" ContentType="image/png"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)"};
    const auto document_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"
            xmlns:wp="http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing"
            xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main"
            xmlns:pic="http://schemas.openxmlformats.org/drawingml/2006/picture">
  <w:body>
    <w:p><w:r><w:drawing><wp:inline>
      <wp:extent cx="9525" cy="9525"/>
      <wp:docPr id="1" name="canonical relationship"/>
      <a:graphic><a:graphicData><pic:pic><pic:blipFill>
        <a:blip r:embed="rIdImageCanonical"/>
      </pic:blipFill></pic:pic></a:graphicData></a:graphic>
    </wp:inline></w:drawing></w:r></w:p>
    <w:p><w:r><w:drawing><wp:inline>
      <wp:extent cx="9525" cy="9525"/>
      <wp:docPr id="2" name="identity variant relationship"/>
      <a:graphic><a:graphicData><pic:pic><pic:blipFill>
        <a:blip r:embed="rIdImageVariant"/>
      </pic:blipFill></pic:pic></a:graphicData></a:graphic>
    </wp:inline></w:drawing></w:r></w:p>
  </w:body>
</w:document>
)"};
    const auto document_relationships_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rIdImageCanonical"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image"
                Target="media/%E4%B8%AD%E6%96%87.PNG"/>
  <Relationship Id="rIdImageVariant"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image"
                Target="MEDIA/%e4%b8%ad%e6%96%87.png"/>
</Relationships>
)"};

    write_test_archive_entries(
        source, {{test_content_types_xml_entry, content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, document_xml},
                 {"word/_rels/document.xml.rels", document_relationships_xml},
                 {image_entry, image_data}});

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    const auto images = document.drawing_images();
    REQUIRE_EQ(images.size(), 2U);
    CHECK_EQ(images[0].entry_name, "word/media/%E4%B8%AD%E6%96%87.PNG");
    CHECK_EQ(images[1].entry_name, "word/MEDIA/%E4%B8%AD%E6%96%87.png");
    REQUIRE(document.remove_drawing_image(0U));
    REQUIRE_FALSE(document.save());

    CHECK(test_docx_entry_exists(source, image_entry));
    CHECK_EQ(read_test_docx_entry(source, image_entry), image_data);
    const auto saved_relationships =
        read_test_docx_entry(source, "word/_rels/document.xml.rels");
    CHECK_EQ(saved_relationships.find("rIdImageCanonical"), std::string::npos);
    CHECK_NE(saved_relationships.find("rIdImageVariant"), std::string::npos);

    featherdoc::Document verified(source);
    REQUIRE_FALSE(verified.open());
    const auto remaining_images = verified.drawing_images();
    REQUIRE_EQ(remaining_images.size(), 1U);
    CHECK_EQ(remaining_images[0].entry_name,
             "word/MEDIA/%E4%B8%AD%E6%96%87.png");
    REQUIRE(verified.extract_drawing_image(0U, extracted));
    CHECK_EQ(read_binary_file(extracted),
             std::vector<unsigned char>(image_data.begin(), image_data.end()));

    fs::remove(source);
    fs::remove(extracted);
}

TEST_CASE("append_floating_image writes anchored media parts and preserves "
          "them across reopen save") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U,
        0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x00U, 0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU,
        0x15U, 0xC4U, 0x89U, 0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U,
        0x54U, 0x78U, 0x9CU, 0x63U, 0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U,
        0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U, 0x00U, 0x00U, 0x00U, 0x00U, 0x49U,
        0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U, 0x82U,
    };

    const fs::path target =
        fs::current_path() / "append_floating_image_roundtrip.docx";
    const fs::path image_path = fs::current_path() / "floating_tiny_image.png";
    fs::remove(target);
    fs::remove(image_path);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::floating_image_options options;
    options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    options.horizontal_offset_px = 24;
    options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    options.vertical_offset_px = -8;
    options.behind_text = true;
    options.allow_overlap = false;
    options.z_order = 64U;
    options.wrap_mode = featherdoc::floating_image_wrap_mode::square;
    options.wrap_distance_left_px = 8U;
    options.wrap_distance_right_px = 10U;
    options.wrap_distance_top_px = 6U;
    options.wrap_distance_bottom_px = 12U;
    options.crop = featherdoc::floating_image_crop{15U, 25U, 35U, 45U};

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_floating_image(image_path, 20U, 10U, options));
    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_relationships,
                 "Type=\"http://schemas.openxmlformats.org/"
                 "officeDocument/2006/relationships/image\""),
             1);
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);

    const auto saved_document_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:anchor"), 1U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 0U);
    CHECK_NE(saved_document_xml.find("relativeFrom=\"page\""),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("relativeFrom=\"margin\""),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:posOffset>228600</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:posOffset>-76200</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("behindDoc=\"1\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("allowOverlap=\"0\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("relativeHeight=\"64\""),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("distT=\"57150\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("distB=\"114300\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("distL=\"76200\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("distR=\"95250\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:wrapSquare wrapText=\"bothSides\""),
             std::string::npos);
    CHECK_NE(saved_document_xml.find(
                 "<a:srcRect l=\"1500\" t=\"2500\" r=\"3500\" b=\"4500\""),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("cx=\"190500\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"95250\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto drawing_images = reopened.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(drawing_images[0].content_type, "image/png");
    CHECK_EQ(drawing_images[0].width_px, 20U);
    CHECK_EQ(drawing_images[0].height_px, 10U);
    REQUIRE(drawing_images[0].floating_options.has_value());
    CHECK_EQ(drawing_images[0].floating_options->horizontal_reference,
             featherdoc::floating_image_horizontal_reference::page);
    CHECK_EQ(drawing_images[0].floating_options->horizontal_offset_px, 24);
    CHECK_EQ(drawing_images[0].floating_options->vertical_reference,
             featherdoc::floating_image_vertical_reference::margin);
    CHECK_EQ(drawing_images[0].floating_options->vertical_offset_px, -8);
    CHECK(drawing_images[0].floating_options->behind_text);
    CHECK_FALSE(drawing_images[0].floating_options->allow_overlap);
    CHECK_EQ(drawing_images[0].floating_options->z_order, 64U);
    CHECK_EQ(drawing_images[0].floating_options->wrap_mode,
             featherdoc::floating_image_wrap_mode::square);
    CHECK_EQ(drawing_images[0].floating_options->wrap_distance_left_px, 8U);
    CHECK_EQ(drawing_images[0].floating_options->wrap_distance_right_px, 10U);
    CHECK_EQ(drawing_images[0].floating_options->wrap_distance_top_px, 6U);
    CHECK_EQ(drawing_images[0].floating_options->wrap_distance_bottom_px, 12U);
    REQUIRE(drawing_images[0].floating_options->crop.has_value());
    CHECK_EQ(drawing_images[0].floating_options->crop->left_per_mille, 15U);
    CHECK_EQ(drawing_images[0].floating_options->crop->top_per_mille, 25U);
    CHECK_EQ(drawing_images[0].floating_options->crop->right_per_mille, 35U);
    CHECK_EQ(drawing_images[0].floating_options->crop->bottom_per_mille, 45U);
    CHECK_EQ(reopened.inline_images().size(), 0U);
    CHECK(reopened.paragraphs().add_run("reopened edit").has_next());
    CHECK_FALSE(reopened.save());

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("append_floating_image rejects crop values that remove the visible "
          "image area") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "append_floating_image_invalid_crop.docx";
    const fs::path image_path =
        fs::current_path() / "floating_invalid_crop.png";
    fs::remove(target);
    fs::remove(image_path);

    write_binary_file(image_path, tiny_png_data());

    featherdoc::floating_image_options options;
    options.crop = featherdoc::floating_image_crop{600U, 0U, 400U, 0U};

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK_FALSE(doc.append_floating_image(image_path, 20U, 10U, options));
    CHECK_EQ(doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("crop"), std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE(
    "floating image parsing clamps UINT32_MAX crop values without overflow") {
    namespace fs = std::filesystem;

    const auto target = fs::current_path() / "floating_crop_uint32_max.docx";
    const auto image_path = fs::current_path() / "floating_crop_uint32_max.png";
    fs::remove(target);
    fs::remove(image_path);
    write_binary_file(image_path, tiny_png_data());

    featherdoc::floating_image_options options;
    options.crop = featherdoc::floating_image_crop{1U, 0U, 0U, 0U};
    featherdoc::Document document(target);
    REQUIRE_FALSE(document.create_empty());
    REQUIRE(document.append_floating_image(image_path, 20U, 10U, options));
    REQUIRE_FALSE(document.save());

    auto document_xml = read_test_docx_entry(target, test_document_xml_entry);
    const auto original_crop = std::string{"l=\"100\""};
    const auto crop_position = document_xml.find(original_crop);
    REQUIRE_NE(crop_position, std::string::npos);
    document_xml.replace(crop_position, original_crop.size(),
                         "l=\"4294967295\"");
    rewrite_test_docx_entry(target, test_document_xml_entry,
                            std::move(document_xml));

    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    const auto images = reopened.drawing_images();
    REQUIRE_EQ(images.size(), 1U);
    REQUIRE(images[0].floating_options.has_value());
    REQUIRE(images[0].floating_options->crop.has_value());
    CHECK_EQ(images[0].floating_options->crop->left_per_mille, 1000U);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("drawing ID allocation rejects UINT32_MAX") {
    namespace fs = std::filesystem;
    const auto target = fs::current_path() / "drawing_id_exhausted.docx";
    const auto image_path = fs::current_path() / "drawing_id_exhausted.png";
    fs::remove(target);
    fs::remove(image_path);
    write_binary_file(image_path, tiny_png_data());
    write_test_docx(
        target,
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main" xmlns:wp="http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing"><w:body><w:p><w:r><w:drawing><wp:inline><wp:docPr id="4294967295"/></wp:inline></w:drawing></w:r></w:p></w:body></w:document>)");

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    CHECK_FALSE(document.append_image(image_path));
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::identifier_space_exhausted);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE(
    "floating image parsing handles INT64_MIN EMU offsets without overflow") {
    namespace fs = std::filesystem;
    const auto target = fs::current_path() / "floating_int64_min.docx";
    const auto image_path = fs::current_path() / "floating_int64_min.png";
    fs::remove(target);
    fs::remove(image_path);
    write_binary_file(image_path, tiny_png_data());

    featherdoc::floating_image_options options;
    options.horizontal_offset_px = 0;
    options.vertical_offset_px = 0;
    featherdoc::Document document(target);
    REQUIRE_FALSE(document.create_empty());
    REQUIRE(document.append_floating_image(image_path, 1U, 1U, options));
    REQUIRE_FALSE(document.save());

    auto document_xml = read_test_docx_entry(target, test_document_xml_entry);
    const auto zero_offset = std::string{"<wp:posOffset>0</wp:posOffset>"};
    const auto offset_position = document_xml.find(zero_offset);
    REQUIRE_NE(offset_position, std::string::npos);
    document_xml.replace(offset_position, zero_offset.size(),
                         "<wp:posOffset>-9223372036854775808</wp:posOffset>");
    rewrite_test_docx_entry(target, test_document_xml_entry,
                            std::move(document_xml));

    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    const auto images = reopened.drawing_images();
    REQUIRE_EQ(images.size(), 1U);
    REQUIRE(images[0].floating_options.has_value());
    CHECK_EQ(images[0].floating_options->horizontal_offset_px,
             std::numeric_limits<std::int32_t>::min());

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("image parsing handles UINT64_MAX EMU extents without overflow") {
    namespace fs = std::filesystem;
    const auto target = fs::current_path() / "image_uint64_max_extent.docx";
    const auto image_path = fs::current_path() / "image_uint64_max_extent.png";
    fs::remove(target);
    fs::remove(image_path);
    write_binary_file(image_path, tiny_png_data());

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.create_empty());
    REQUIRE(document.append_image(image_path, 1U, 1U));
    REQUIRE_FALSE(document.save());

    auto document_xml = read_test_docx_entry(target, test_document_xml_entry);
    const auto original_extent = std::string{"cx=\"9525\""};
    const auto extent_position = document_xml.find(original_extent);
    REQUIRE_NE(extent_position, std::string::npos);
    document_xml.replace(extent_position, original_extent.size(),
                         "cx=\"18446744073709551615\"");
    rewrite_test_docx_entry(target, test_document_xml_entry,
                            std::move(document_xml));

    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    const auto images = reopened.drawing_images();
    REQUIRE_EQ(images.size(), 1U);
    CHECK_EQ(images[0].width_px, std::numeric_limits<std::uint32_t>::max());

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("inline_images lists existing body images and can extract them") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U,
        0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x00U, 0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU,
        0x15U, 0xC4U, 0x89U, 0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U,
        0x54U, 0x78U, 0x9CU, 0x63U, 0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U,
        0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U, 0x00U, 0x00U, 0x00U, 0x00U, 0x49U,
        0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U, 0x82U,
    };

    const fs::path target = fs::current_path() / "inline_images_roundtrip.docx";
    const fs::path image_path =
        fs::current_path() / "inline_images_roundtrip.png";
    const fs::path extracted_path =
        fs::current_path() / "inline_images_extracted.png";
    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    const std::vector<unsigned char> expected_image_data(image_data.begin(),
                                                         image_data.end());
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(image_path));
    CHECK(doc.append_image(image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto images = reopened.inline_images();
    REQUIRE_EQ(images.size(), 2U);

    CHECK_EQ(images[0].index, 0U);
    CHECK_EQ(images[0].entry_name, "word/media/image1.png");
    CHECK_EQ(images[0].display_name, image_path.filename().string());
    CHECK_EQ(images[0].content_type, "image/png");
    CHECK_EQ(images[0].width_px, 1U);
    CHECK_EQ(images[0].height_px, 1U);

    CHECK_EQ(images[1].index, 1U);
    CHECK_EQ(images[1].entry_name, "word/media/image2.png");
    CHECK_EQ(images[1].display_name, image_path.filename().string());
    CHECK_EQ(images[1].content_type, "image/png");
    CHECK_EQ(images[1].width_px, 20U);
    CHECK_EQ(images[1].height_px, 10U);

    CHECK(reopened.extract_inline_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_image_data);

    CHECK_FALSE(reopened.extract_inline_image(2U, extracted_path));
    CHECK_EQ(reopened.last_error().code,
             std::make_error_code(std::errc::result_out_of_range));

    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);
}

TEST_CASE("externally targeted image relationships are not exposed as embedded "
          "images") {
    namespace fs = std::filesystem;

    const auto target = fs::current_path() / "external_image_relationship.docx";
    const auto image_path =
        fs::current_path() / "external_image_relationship.png";
    fs::remove(target);
    fs::remove(image_path);
    write_binary_file(image_path, tiny_png_data());

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.create_empty());
    REQUIRE(document.append_image(image_path));
    REQUIRE_FALSE(document.save());

    auto relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    constexpr auto internal_target = R"(Target="media/image1.png")";
    const auto target_position = relationships.find(internal_target);
    REQUIRE_NE(target_position, std::string::npos);
    relationships.replace(
        target_position, std::string_view{internal_target}.size(),
        R"(Target="https://example.invalid/image.png" TargetMode="External")");
    rewrite_test_docx_entry(target, "word/_rels/document.xml.rels",
                            std::move(relationships));

    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    CHECK(reopened.inline_images().empty());
    CHECK(reopened.drawing_images().empty());

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("replace_inline_image updates only the selected body image and "
          "preserves layout") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U,
        0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x00U, 0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU,
        0x15U, 0xC4U, 0x89U, 0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U,
        0x54U, 0x78U, 0x9CU, 0x63U, 0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U,
        0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U, 0x00U, 0x00U, 0x00U, 0x00U, 0x49U,
        0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U, 0x82U,
    };
    constexpr unsigned char tiny_gif_bytes[] = {
        0x47U, 0x49U, 0x46U, 0x38U, 0x39U, 0x61U, 0x01U, 0x00U, 0x01U,
        0x00U, 0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU,
        0xFFU, 0x21U, 0xF9U, 0x04U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U,
        0x2CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x01U, 0x00U,
        0x00U, 0x02U, 0x02U, 0x44U, 0x01U, 0x00U, 0x3BU,
    };

    const fs::path target = fs::current_path() / "replace_inline_image.docx";
    const fs::path source_image_path =
        fs::current_path() / "replace_inline_image_source.png";
    const fs::path replacement_image_path =
        fs::current_path() / "replace_inline_image_replacement.gif";
    const fs::path extracted_path =
        fs::current_path() / "replace_inline_image_extracted.gif";
    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);

    const std::string source_image_data(
        reinterpret_cast<const char *>(tiny_png_bytes), sizeof(tiny_png_bytes));
    const std::string replacement_image_data(
        reinterpret_cast<const char *>(tiny_gif_bytes), sizeof(tiny_gif_bytes));
    const std::vector<unsigned char> expected_replacement_data(
        replacement_image_data.begin(), replacement_image_data.end());
    write_binary_file(source_image_path, source_image_data);
    write_binary_file(replacement_image_path, replacement_image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(source_image_path));
    CHECK(doc.append_image(source_image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.replace_inline_image(1U, replacement_image_path));

    auto images = reopened.inline_images();
    REQUIRE_EQ(images.size(), 2U);
    CHECK_EQ(images[0].entry_name, "word/media/image1.png");
    CHECK_EQ(images[0].content_type, "image/png");
    CHECK_EQ(images[1].width_px, 20U);
    CHECK_EQ(images[1].height_px, 10U);
    CHECK_EQ(images[1].content_type, "image/gif");
    CHECK(images[1].entry_name.ends_with(".gif"));
    CHECK_NE(images[1].entry_name, "word/media/image2.png");

    const auto replacement_entry_name = images[1].entry_name;
    const auto replacement_target =
        (fs::path{"media"} / fs::path{replacement_entry_name}.filename())
            .generic_string();

    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK(test_docx_entry_exists(target, replacement_entry_name.c_str()));
    CHECK_EQ(read_test_docx_entry(target, replacement_entry_name.c_str()),
             replacement_image_data);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"media/image2.png\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"" + replacement_target + "\""),
             std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("Extension=\"gif\""), std::string::npos);
    CHECK_NE(saved_content_types.find("ContentType=\"image/gif\""),
             std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    images = reopened_again.inline_images();
    REQUIRE_EQ(images.size(), 2U);
    CHECK_EQ(images[1].content_type, "image/gif");
    CHECK_EQ(images[1].width_px, 20U);
    CHECK_EQ(images[1].height_px, 10U);
    CHECK_EQ(images[1].entry_name, replacement_entry_name);
    CHECK(reopened_again.extract_inline_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_replacement_data);

    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);
}

TEST_CASE("drawing_images includes anchored body images and "
          "replace_drawing_image preserves them") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U,
        0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x00U, 0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU,
        0x15U, 0xC4U, 0x89U, 0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U,
        0x54U, 0x78U, 0x9CU, 0x63U, 0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U,
        0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U, 0x00U, 0x00U, 0x00U, 0x00U, 0x49U,
        0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U, 0x82U,
    };
    constexpr unsigned char tiny_gif_bytes[] = {
        0x47U, 0x49U, 0x46U, 0x38U, 0x39U, 0x61U, 0x01U, 0x00U, 0x01U,
        0x00U, 0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU,
        0xFFU, 0x21U, 0xF9U, 0x04U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U,
        0x2CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x01U, 0x00U,
        0x00U, 0x02U, 0x02U, 0x44U, 0x01U, 0x00U, 0x3BU,
    };

    const fs::path target = fs::current_path() / "drawing_images_anchor.docx";
    const fs::path source_image_path =
        fs::current_path() / "drawing_images_anchor_source.png";
    const fs::path replacement_image_path =
        fs::current_path() / "drawing_images_anchor_replacement.gif";
    const fs::path extracted_path =
        fs::current_path() / "drawing_images_anchor_extracted.gif";
    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);

    const std::string source_image_data(
        reinterpret_cast<const char *>(tiny_png_bytes), sizeof(tiny_png_bytes));
    const std::string replacement_image_data(
        reinterpret_cast<const char *>(tiny_gif_bytes), sizeof(tiny_gif_bytes));
    const std::vector<unsigned char> expected_replacement_data(
        replacement_image_data.begin(), replacement_image_data.end());
    write_binary_file(source_image_path, source_image_data);
    write_binary_file(replacement_image_path, replacement_image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(source_image_path));
    CHECK(doc.append_image(source_image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    auto anchored_document_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    anchored_document_xml =
        convert_nth_inline_drawing_to_anchor(anchored_document_xml, 1U);
    CHECK_EQ(count_substring_occurrences(anchored_document_xml, "<wp:anchor"),
             1U);
    rewrite_test_docx_entry(target, test_document_xml_entry,
                            std::move(anchored_document_xml));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto drawing_images = reopened.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 2U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(drawing_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(drawing_images[0].content_type, "image/png");
    CHECK_EQ(drawing_images[1].content_type, "image/png");
    CHECK_EQ(drawing_images[1].width_px, 20U);
    CHECK_EQ(drawing_images[1].height_px, 10U);

    const auto inline_images = reopened.inline_images();
    REQUIRE_EQ(inline_images.size(), 1U);
    CHECK_EQ(inline_images[0].entry_name, "word/media/image1.png");

    CHECK(reopened.extract_drawing_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path),
             std::vector<unsigned char>(source_image_data.begin(),
                                        source_image_data.end()));
    CHECK_FALSE(reopened.extract_inline_image(1U, extracted_path));
    CHECK_EQ(reopened.last_error().code,
             std::make_error_code(std::errc::result_out_of_range));

    CHECK(reopened.replace_drawing_image(1U, replacement_image_path));

    auto updated_images = reopened.drawing_images();
    REQUIRE_EQ(updated_images.size(), 2U);
    CHECK_EQ(updated_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(updated_images[1].width_px, 20U);
    CHECK_EQ(updated_images[1].height_px, 10U);
    CHECK_EQ(updated_images[1].content_type, "image/gif");
    CHECK(updated_images[1].entry_name.ends_with(".gif"));
    const auto replacement_entry_name = updated_images[1].entry_name;
    const auto replacement_target =
        (fs::path{"media"} / fs::path{replacement_entry_name}.filename())
            .generic_string();

    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK(test_docx_entry_exists(target, replacement_entry_name.c_str()));
    CHECK_EQ(read_test_docx_entry(target, replacement_entry_name.c_str()),
             replacement_image_data);

    const auto saved_document_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 1U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:anchor"), 1U);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"media/image2.png\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"" + replacement_target + "\""),
             std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("Extension=\"gif\""), std::string::npos);
    CHECK_NE(saved_content_types.find("ContentType=\"image/gif\""),
             std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    updated_images = reopened_again.drawing_images();
    REQUIRE_EQ(updated_images.size(), 2U);
    CHECK_EQ(updated_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(updated_images[1].content_type, "image/gif");
    CHECK_EQ(updated_images[1].entry_name, replacement_entry_name);
    CHECK(reopened_again.extract_drawing_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_replacement_data);

    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);
}

TEST_CASE(
    "remove_drawing_image and remove_inline_image prune body media parts") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U,
        0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x00U, 0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU,
        0x15U, 0xC4U, 0x89U, 0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U,
        0x54U, 0x78U, 0x9CU, 0x63U, 0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U,
        0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U, 0x00U, 0x00U, 0x00U, 0x00U, 0x49U,
        0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U, 0x82U,
    };

    const fs::path target = fs::current_path() / "drawing_images_remove.docx";
    const fs::path image_path =
        fs::current_path() / "drawing_images_remove_source.png";
    fs::remove(target);
    fs::remove(image_path);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(image_path));
    CHECK(doc.append_image(image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    auto anchored_document_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    anchored_document_xml =
        convert_nth_inline_drawing_to_anchor(anchored_document_xml, 1U);
    CHECK_EQ(count_substring_occurrences(anchored_document_xml, "<wp:anchor"),
             1U);
    rewrite_test_docx_entry(target, test_document_xml_entry,
                            std::move(anchored_document_xml));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto drawing_images = reopened.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 2U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(drawing_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);

    CHECK(reopened.remove_drawing_image(1U));
    drawing_images = reopened.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(reopened.inline_images().size(), 1U);

    CHECK(reopened.remove_inline_image(0U));
    CHECK(reopened.drawing_images().empty());
    CHECK(reopened.inline_images().empty());
    CHECK_FALSE(reopened.remove_inline_image(0U));
    CHECK_EQ(reopened.last_error().code,
             std::make_error_code(std::errc::result_out_of_range));

    CHECK_FALSE(reopened.save());

    const auto saved_document_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 0U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:anchor"), 0U);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(saved_relationships.find("relationships/image"),
             std::string::npos);

    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image2.png"));

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    CHECK(reopened_again.drawing_images().empty());
    CHECK(reopened_again.inline_images().empty());

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("append_image detects raster image dimensions through stb_image") {
    namespace fs = std::filesystem;

    struct raster_case final {
        const char *extension;
        const char *content_type;
        std::string (*data_factory)();
        std::uint32_t expected_width;
        std::uint32_t expected_height;
    };

    const raster_case cases[] = {
        {"png", "image/png", tiny_png_data, 1U, 1U},
        {"jpg", "image/jpeg", tiny_jpeg_data, 3U, 2U},
        {"gif", "image/gif", tiny_gif_data, 1U, 1U},
        {"bmp", "image/bmp", tiny_bmp_data, 3U, 2U},
    };

    const fs::path target = fs::current_path() / "stb_raster_dimensions.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    for (const auto &test_case : cases) {
        const fs::path image_path =
            fs::current_path() /
            ("stb_raster_dimensions." + std::string{test_case.extension});
        fs::remove(image_path);
        write_binary_file(image_path, test_case.data_factory());

        CHECK(doc.append_image(image_path));

        fs::remove(image_path);
    }

    const auto images = doc.drawing_images();
    REQUIRE_EQ(images.size(), std::size(cases));
    for (std::size_t index = 0U; index < std::size(cases); ++index) {
        CHECK_EQ(images[index].content_type, cases[index].content_type);
        CHECK_EQ(images[index].width_px, cases[index].expected_width);
        CHECK_EQ(images[index].height_px, cases[index].expected_height);
        CHECK_NE(images[index].entry_name.find(
                     "." + std::string{cases[index].extension}),
                 std::string::npos);
    }

    CHECK_FALSE(doc.save());
    const auto content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(content_types.find("ContentType=\"image/png\""),
             std::string::npos);
    CHECK_NE(content_types.find("ContentType=\"image/jpeg\""),
             std::string::npos);
    CHECK_NE(content_types.find("ContentType=\"image/gif\""),
             std::string::npos);
    CHECK_NE(content_types.find("ContentType=\"image/bmp\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_images = reopened.drawing_images();
    REQUIRE_EQ(reopened_images.size(), std::size(cases));
    for (std::size_t index = 0U; index < std::size(cases); ++index) {
        CHECK_EQ(reopened_images[index].content_type,
                 cases[index].content_type);
        CHECK_EQ(reopened_images[index].width_px, cases[index].expected_width);
        CHECK_EQ(reopened_images[index].height_px,
                 cases[index].expected_height);
    }

    fs::remove(target);
}

TEST_CASE("append_image lets stb_image identify raster bytes while package "
          "metadata follows extension") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "stb_raster_extension_metadata.docx";
    const fs::path image_path = fs::current_path() / "jpeg_bytes_named_png.png";
    fs::remove(target);
    fs::remove(image_path);

    const auto jpeg_bytes = tiny_jpeg_data();
    write_binary_file(image_path, jpeg_bytes);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(image_path));

    const auto images = doc.drawing_images();
    REQUIRE_EQ(images.size(), 1U);
    CHECK_EQ(images[0].content_type, "image/png");
    CHECK_EQ(images[0].width_px, 3U);
    CHECK_EQ(images[0].height_px, 2U);
    CHECK_NE(images[0].entry_name.find(".png"), std::string::npos);

    CHECK_FALSE(doc.save());
    const auto content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(content_types.find("Extension=\"png\""), std::string::npos);
    CHECK_NE(content_types.find("ContentType=\"image/png\""),
             std::string::npos);
    CHECK_EQ(read_test_docx_entry(target, images[0].entry_name.c_str()),
             jpeg_bytes);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("append_image supports SVG WebP and TIFF inputs") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "extended_image_formats.docx";
    const fs::path svg_path = fs::current_path() / "extended_image_format.svg";
    const fs::path webp_path =
        fs::current_path() / "extended_image_format.webp";
    const fs::path tiff_path =
        fs::current_path() / "extended_image_format.tiff";
    fs::remove(target);
    fs::remove(svg_path);
    fs::remove(webp_path);
    fs::remove(tiff_path);

    write_binary_file(svg_path, tiny_svg_data());
    write_binary_file(webp_path, tiny_webp_data());
    write_binary_file(tiff_path, tiny_tiff_data());

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(svg_path));
    CHECK(doc.append_image(webp_path));
    CHECK(doc.append_image(tiff_path));

    const auto images = doc.drawing_images();
    REQUIRE(images.size() == 3U);
    CHECK_EQ(images[0].content_type, "image/svg+xml");
    CHECK_EQ(images[0].width_px, 3U);
    CHECK_EQ(images[0].height_px, 2U);
    CHECK_EQ(images[1].content_type, "image/webp");
    CHECK_EQ(images[1].width_px, 3U);
    CHECK_EQ(images[1].height_px, 2U);
    CHECK_EQ(images[2].content_type, "image/tiff");
    CHECK_EQ(images[2].width_px, 3U);
    CHECK_EQ(images[2].height_px, 2U);

    CHECK_FALSE(doc.save());
    const auto content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(content_types.find("Extension=\"svg\""), std::string::npos);
    CHECK_NE(content_types.find("ContentType=\"image/svg+xml\""),
             std::string::npos);
    CHECK_NE(content_types.find("Extension=\"webp\""), std::string::npos);
    CHECK_NE(content_types.find("ContentType=\"image/webp\""),
             std::string::npos);
    CHECK_NE(content_types.find("Extension=\"tiff\""), std::string::npos);
    CHECK_NE(content_types.find("ContentType=\"image/tiff\""),
             std::string::npos);
    CHECK_NE(images[0].entry_name.find(".svg"), std::string::npos);
    CHECK_NE(images[1].entry_name.find(".webp"), std::string::npos);
    CHECK_NE(images[2].entry_name.find(".tiff"), std::string::npos);
    CHECK(test_docx_entry_exists(target, images[0].entry_name.c_str()));
    CHECK(test_docx_entry_exists(target, images[1].entry_name.c_str()));
    CHECK(test_docx_entry_exists(target, images[2].entry_name.c_str()));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_images = reopened.drawing_images();
    REQUIRE(reopened_images.size() == 3U);
    CHECK_EQ(reopened_images[0].content_type, "image/svg+xml");
    CHECK_EQ(reopened_images[1].content_type, "image/webp");
    CHECK_EQ(reopened_images[2].content_type, "image/tiff");

    fs::remove(target);
    fs::remove(svg_path);
    fs::remove(webp_path);
    fs::remove(tiff_path);
}

TEST_CASE("append_image rejects supported extensions with unreadable image "
          "dimensions") {
    namespace fs = std::filesystem;

    const fs::path corrupt_path = fs::current_path() / "corrupt_image.png";
    fs::remove(corrupt_path);

    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());

    write_binary_file(corrupt_path, "not an image");
    CHECK_FALSE(doc.append_image(corrupt_path));
    CHECK_EQ(doc.last_error().code,
             featherdoc::document_errc::image_size_read_failed);
    CHECK(doc.drawing_images().empty());
    CHECK(doc.inline_images().empty());

    fs::remove(corrupt_path);
}

TEST_CASE("append_image rejects non-finite SVG dimensions without mutation") {
    namespace fs = std::filesystem;

    const auto nan_size_path = fs::current_path() / "non_finite_size.svg";
    const auto infinite_size_path = fs::current_path() / "infinite_size.svg";
    const auto nan_viewbox_path = fs::current_path() / "non_finite_viewbox.svg";
    fs::remove(nan_size_path);
    fs::remove(infinite_size_path);
    fs::remove(nan_viewbox_path);
    write_binary_file(
        nan_size_path,
        R"(<svg xmlns="http://www.w3.org/2000/svg" width="nan" height="1"/>)");
    write_binary_file(
        infinite_size_path,
        R"(<svg xmlns="http://www.w3.org/2000/svg" width="inf" height="1"/>)");
    write_binary_file(
        nan_viewbox_path,
        R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="nan 0 1 1"/>)");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK_FALSE(document.append_image(nan_size_path));
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::image_size_read_failed);
    CHECK(document.drawing_images().empty());
    CHECK_FALSE(document.append_image(infinite_size_path));
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::image_size_read_failed);
    CHECK(document.drawing_images().empty());
    CHECK_FALSE(document.append_image(nan_viewbox_path));
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::image_size_read_failed);
    CHECK(document.drawing_images().empty());

    fs::remove(nan_size_path);
    fs::remove(infinite_size_path);
    fs::remove(nan_viewbox_path);
}

TEST_CASE(
    "append_image rejects wrapped WebP and TIFF offsets without mutation") {
    namespace fs = std::filesystem;

    const auto webp_path = fs::current_path() / "oversized_chunk.webp";
    const auto tiff_path = fs::current_path() / "oversized_offset.tiff";
    fs::remove(webp_path);
    fs::remove(tiff_path);
    write_binary_file(webp_path, oversized_webp_chunk_data());
    write_binary_file(tiff_path, oversized_tiff_value_offset_data());

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK_FALSE(document.append_image(webp_path));
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::image_size_read_failed);
    CHECK(document.drawing_images().empty());
    CHECK_FALSE(document.append_image(tiff_path));
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::image_size_read_failed);
    CHECK(document.drawing_images().empty());

    fs::remove(webp_path);
    fs::remove(tiff_path);
}

TEST_CASE("append_image reports unsupported image extensions explicitly") {
    namespace fs = std::filesystem;

    const fs::path image_path = fs::current_path() / "unsupported_image.txt";
    fs::remove(image_path);
    write_binary_file(image_path, "not an image");

    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());
    CHECK_FALSE(doc.append_image(image_path));
    CHECK_EQ(doc.last_error().code,
             featherdoc::document_errc::image_format_unsupported);
    CHECK_FALSE(doc.last_error().detail.empty());

    fs::remove(image_path);
}

TEST_CASE("drawing image extraction revalidates a replaced source archive") {
    namespace fs = std::filesystem;

    const auto source =
        fs::current_path() / "drawing_image_replaced_source.docx";
    const auto image_path =
        fs::current_path() / "drawing_image_replaced_source.png";
    const auto extracted_path =
        fs::current_path() / "drawing_image_replaced_extracted.png";
    fs::remove(source);
    fs::remove(image_path);
    fs::remove(extracted_path);

    const auto image_data = tiny_png_data();
    write_binary_file(image_path, image_data);
    featherdoc::Document created(source);
    REQUIRE_FALSE(created.create_empty());
    REQUIRE(created.append_image(image_path));
    REQUIRE_FALSE(created.save());

    featherdoc::document_open_options options;
    options.limits.max_binary_part_bytes = image_data.size();
    featherdoc::Document reopened(source);
    REQUIRE_FALSE(reopened.open(options));
    const auto images = reopened.drawing_images();
    REQUIRE_EQ(images.size(), 1U);

    rewrite_test_docx_entry(source, images.front().entry_name.c_str(),
                            std::string(image_data.size() + 1U, 'x'));

    CHECK_FALSE(reopened.extract_drawing_image(0U, extracted_path));
    CHECK_EQ(reopened.last_error().code,
             featherdoc::document_errc::source_archive_changed);
    CHECK_EQ(reopened.last_error().entry_name, images.front().entry_name);
    CHECK_FALSE(fs::exists(extracted_path));

    fs::remove(source);
    fs::remove(image_path);
    fs::remove(extracted_path);
}

TEST_CASE("tolerant drawing image extraction reads raw Unicode archive names "
          "through the canonical catalog") {
    namespace fs = std::filesystem;

    const auto source =
        fs::current_path() / "drawing_image_raw_unicode_entry.docx";
    const auto extracted_path =
        fs::current_path() / "drawing_image_raw_unicode_extracted.png";
    fs::remove(source);
    fs::remove(extracted_path);

    const auto raw_image_filename = utf8_from_u8(u8"中文图像.png");
    const auto raw_image_entry = "word/media/" + raw_image_filename;
    const auto canonical_image_entry =
        "word/media/%E4%B8%AD%E6%96%87%E5%9B%BE%E5%83%8F.png";
    const auto image_data = tiny_png_data();

    const auto content_types_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Default Extension="png" ContentType="image/png"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)"};
    const auto document_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"
            xmlns:wp="http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing"
            xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main"
            xmlns:pic="http://schemas.openxmlformats.org/drawingml/2006/picture">
  <w:body>
    <w:p><w:r><w:drawing><wp:inline>
      <wp:extent cx="9525" cy="9525"/>
      <wp:docPr id="1" name="raw Unicode image"/>
      <a:graphic><a:graphicData><pic:pic><pic:blipFill>
        <a:blip r:embed="rIdImage"/>
      </pic:blipFill></pic:pic></a:graphicData></a:graphic>
    </wp:inline></w:drawing></w:r></w:p>
  </w:body>
</w:document>
)"};
    const auto document_relationships_xml =
        std::string{
            R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rIdImage"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image"
                Target="media/)"} +
        raw_image_filename +
        R"("/>
</Relationships>
)";

    write_test_archive_entries(
        source, {{test_content_types_xml_entry, content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, document_xml},
                 {"word/_rels/document.xml.rels", document_relationships_xml},
                 {raw_image_entry, image_data}});

    featherdoc::Document strict_document(source);
    CHECK_EQ(strict_document.open(),
             featherdoc::document_errc::invalid_package_structure);
    CHECK_FALSE(strict_document.is_open());

    featherdoc::document_open_options tolerant_options;
    tolerant_options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document tolerant_document(source);
    REQUIRE_FALSE(tolerant_document.open(tolerant_options));

    const auto images = tolerant_document.drawing_images();
    REQUIRE_FALSE(tolerant_document.last_error());
    REQUIRE_EQ(images.size(), 1U);
    CHECK_EQ(images.front().entry_name, canonical_image_entry);
    REQUIRE(tolerant_document.extract_drawing_image(0U, extracted_path));

    const std::vector<unsigned char> expected_image_data(image_data.begin(),
                                                         image_data.end());
    CHECK_EQ(read_binary_file(extracted_path), expected_image_data);

    fs::remove(source);
    fs::remove(extracted_path);
}
