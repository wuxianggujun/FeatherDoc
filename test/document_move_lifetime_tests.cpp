#include "doctest.h"

#include <featherdoc.hpp>

#include <filesystem>
#include <type_traits>
#include <utility>

static_assert(!std::is_copy_constructible_v<featherdoc::Document>);
static_assert(!std::is_copy_assignable_v<featherdoc::Document>);
static_assert(std::is_move_constructible_v<featherdoc::Document>);
static_assert(std::is_move_assignable_v<featherdoc::Document>);
static_assert(std::is_nothrow_move_constructible_v<featherdoc::Document>);
static_assert(std::is_nothrow_move_assignable_v<featherdoc::Document>);

TEST_CASE("moving a document invalidates handles owned by the source object") {
    featherdoc::Document source;
    REQUIRE_FALSE(source.create_empty());

    auto source_part = source.body_template();
    REQUIRE(source_part);
    auto source_paragraph = source_part.append_paragraph("before move");
    auto source_table = source_part.append_table(1U, 1U);
    REQUIRE(source_paragraph.valid());
    REQUIRE(source_table.valid());

    featherdoc::Document destination(std::move(source));

    CHECK(destination.is_open());
    CHECK_FALSE(source.is_open());
    CHECK_FALSE(source_part);
    CHECK_FALSE(source_paragraph.valid());
    CHECK_FALSE(source_table.valid());

    auto destination_part = destination.body_template();
    REQUIRE(destination_part);
    CHECK(destination_part.append_paragraph("after move").valid());

    REQUIRE_FALSE(source.create_empty());
    CHECK(source.is_open());
    CHECK(source.body_template().append_paragraph("reused source").valid());
}

TEST_CASE("move assignment invalidates handles from both document objects") {
    featherdoc::Document source;
    REQUIRE_FALSE(source.create_empty());
    auto source_part = source.body_template();
    auto source_paragraph = source_part.append_paragraph("source");

    featherdoc::Document destination;
    REQUIRE_FALSE(destination.create_empty());
    auto destination_part = destination.body_template();
    auto destination_paragraph =
        destination_part.append_paragraph("old destination");

    destination = std::move(source);

    CHECK(destination.is_open());
    CHECK_FALSE(source.is_open());
    CHECK_FALSE(source_part);
    CHECK_FALSE(source_paragraph.valid());
    CHECK_FALSE(destination_part);
    CHECK_FALSE(destination_paragraph.valid());

    auto moved_part = destination.body_template();
    REQUIRE(moved_part);
    CHECK(moved_part.append_table(1U, 1U).valid());
}

TEST_CASE("moving repeatedly from a moved-from document stays reusable") {
    featherdoc::Document original;
    REQUIRE_FALSE(original.create_empty());

    featherdoc::Document first(std::move(original));
    REQUIRE(first.is_open());
    REQUIRE_FALSE(original.is_open());

    // The second move transfers a null lifetime token. It must still be a
    // valid, allocation-free noexcept move and produce a closed Document.
    featherdoc::Document second(std::move(original));
    CHECK_FALSE(second.is_open());
    CHECK_FALSE(original.is_open());

    REQUIRE_FALSE(second.create_empty());
    CHECK(second.body_template().append_paragraph("second reused").valid());
    REQUIRE_FALSE(original.create_empty());
    CHECK(original.body_template().append_paragraph("source reused").valid());

    auto first_part = first.body_template();
    REQUIRE(first_part);
    auto first_paragraph = first_part.append_paragraph("first destination");
    REQUIRE(first_paragraph.valid());

    first = std::move(second);
    CHECK(first.is_open());
    CHECK_FALSE(first_part);
    CHECK_FALSE(first_paragraph.valid());

    // Moving a closed source with a lifetime token also leaves a reusable
    // destination.
    first = std::move(second);
    CHECK_FALSE(first.is_open());
    REQUIRE_FALSE(first.create_empty());
    CHECK(first.body_template().append_paragraph("assignment reused").valid());

    // A move-constructed source has no token until it is reused. Assigning
    // from that exact state exercises the null-token branch without allocating
    // inside the noexcept move operation.
    featherdoc::Document null_token_source;
    featherdoc::Document token_holder(std::move(null_token_source));
    CHECK_FALSE(token_holder.is_open());
    first = std::move(null_token_source);
    CHECK_FALSE(first.is_open());
    REQUIRE_FALSE(first.create_empty());
    CHECK(first.body_template().append_paragraph("null-token reused").valid());
}

TEST_CASE("a moved document keeps its UTF-8 path and remains saveable") {
    const auto output = std::filesystem::temp_directory_path() /
                        std::filesystem::path{u8"featherdoc-移动文档.docx"};
    std::error_code cleanup_error;
    std::filesystem::remove(output, cleanup_error);

    featherdoc::Document source(output);
    REQUIRE_FALSE(source.create_empty());
    REQUIRE(source.body_template().append_paragraph("中文内容").valid());
    auto source_header_paragraph = source.ensure_header_paragraphs();
    auto source_footer_paragraph = source.ensure_footer_paragraphs();
    REQUIRE(source_header_paragraph.valid());
    REQUIRE(source_footer_paragraph.valid());
    REQUIRE(source_header_paragraph.add_run("移动前页眉").valid());
    REQUIRE(source_footer_paragraph.add_run("移动前页脚").valid());
    auto source_header_part = source.header_template(0U);
    auto source_footer_part = source.footer_template(0U);
    REQUIRE(source_header_part);
    REQUIRE(source_footer_part);

    featherdoc::Document destination(std::move(source));
    CHECK(destination.path() == output);
    CHECK_FALSE(source_header_paragraph.valid());
    CHECK_FALSE(source_footer_paragraph.valid());
    CHECK_FALSE(source_header_part);
    CHECK_FALSE(source_footer_part);

    auto destination_header_part = destination.header_template(0U);
    auto destination_footer_part = destination.footer_template(0U);
    REQUIRE(destination_header_part);
    REQUIRE(destination_footer_part);
    CHECK(destination_header_part.append_paragraph("移动后页眉").valid());
    CHECK(destination_footer_part.append_paragraph("移动后页脚").valid());
    REQUIRE_FALSE(destination.save());

    featherdoc::Document reopened(output);
    REQUIRE_FALSE(reopened.open());
    CHECK(reopened.is_open());
    CHECK(reopened.header_template(0U));
    CHECK(reopened.footer_template(0U));

    std::filesystem::remove(output, cleanup_error);
}
