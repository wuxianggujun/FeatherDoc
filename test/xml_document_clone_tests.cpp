#include "../src/singleton_part_attachment_helpers.hpp"
#include "../src/xml_document_clone_helpers.hpp"
#include "../src/xml_document_initialization_helpers.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "allocation_failure_test_case.hpp"

namespace {

pugi::allocation_function delegated_allocate = nullptr;
std::size_t allocation_calls = 0U;
std::size_t failure_call = 0U;

auto controlled_allocate(std::size_t size) -> void * {
    ++allocation_calls;
    if (failure_call != 0U && allocation_calls == failure_call) {
        return nullptr;
    }
    return delegated_allocate(size);
}

class pugi_allocator_guard final {
  public:
    pugi_allocator_guard()
        : previous_allocate_(pugi::get_memory_allocation_function()),
          previous_deallocate_(pugi::get_memory_deallocation_function()) {
        delegated_allocate = this->previous_allocate_;
        allocation_calls = 0U;
        failure_call = 0U;
        pugi::set_memory_management_functions(controlled_allocate,
                                              this->previous_deallocate_);
    }

    pugi_allocator_guard(const pugi_allocator_guard &) = delete;
    auto operator=(const pugi_allocator_guard &)
        -> pugi_allocator_guard & = delete;

    ~pugi_allocator_guard() {
        pugi::set_memory_management_functions(this->previous_allocate_,
                                              this->previous_deallocate_);
        delegated_allocate = nullptr;
        allocation_calls = 0U;
        failure_call = 0U;
    }

  private:
    pugi::allocation_function previous_allocate_;
    pugi::deallocation_function previous_deallocate_;
};

class string_xml_writer final : public pugi::xml_writer {
  public:
    void write(const void *data, std::size_t size) override {
        this->text.append(static_cast<const char *>(data), size);
    }

    std::string text;
};

auto serialize(const pugi::xml_document &document) -> std::string {
    string_xml_writer writer;
    document.save(writer, "", pugi::format_raw, pugi::encoding_utf8);
    return writer.text;
}

auto make_source_document() -> pugi::xml_document {
    pugi::xml_document document;

    auto declaration = document.append_child(pugi::node_declaration);
    REQUIRE(declaration != pugi::xml_node{});
    REQUIRE(declaration.append_attribute("version").set_value("1.0"));
    REQUIRE(declaration.append_attribute("encoding").set_value("UTF-8"));

    auto doctype = document.append_child(pugi::node_doctype);
    REQUIRE(doctype != pugi::xml_node{});
    REQUIRE(doctype.set_value("root SYSTEM \"fixture.dtd\""));

    auto processing_instruction = document.append_child(pugi::node_pi);
    REQUIRE(processing_instruction != pugi::xml_node{});
    REQUIRE(processing_instruction.set_name("fixture"));
    REQUIRE(processing_instruction.set_value("mode=checked"));

    auto root = document.append_child("root");
    REQUIRE(root != pugi::xml_node{});
    REQUIRE(root.append_attribute("language").set_value("zh-CN"));
    const std::string large_attribute(70U * 1024U, 'a');
    REQUIRE(root.append_attribute("large").set_value(large_attribute.c_str()));

    auto comment = root.append_child(pugi::node_comment);
    REQUIRE(comment != pugi::xml_node{});
    REQUIRE(comment.set_value("中文注释"));

    auto child = root.append_child("child");
    REQUIRE(child != pugi::xml_node{});
    auto text = child.append_child(pugi::node_pcdata);
    REQUIRE(text != pugi::xml_node{});
    REQUIRE(text.set_value("中文、かな、emoji: 😀"));

    const std::string large_cdata(70U * 1024U, 'c');
    auto cdata = child.append_child(pugi::node_cdata);
    REQUIRE(cdata != pugi::xml_node{});
    REQUIRE(cdata.set_value(large_cdata.c_str()));

    return document;
}

auto make_destination_document() -> pugi::xml_document {
    pugi::xml_document document;
    auto sentinel = document.append_child("sentinel");
    REQUIRE(sentinel != pugi::xml_node{});
    REQUIRE(sentinel.append_attribute("state").set_value("unchanged"));
    REQUIRE(sentinel.append_child(pugi::node_pcdata).set_value("keep me"));
    return document;
}

} // namespace

TEST_CASE("checked XML clone preserves every supported node payload") {
    auto source = make_source_document();
    auto destination = make_destination_document();

    CHECK_EQ(
        featherdoc::detail::checked_clone_xml_document(source, destination),
        featherdoc::detail::xml_document_clone_status::success);
    CHECK_EQ(serialize(destination), serialize(source));
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "checked XML clone is atomic for every pugixml allocation failure") {
    auto baseline_source = make_source_document();
    auto baseline_destination = make_destination_document();

    std::size_t successful_clone_allocation_count = 0U;
    {
        pugi_allocator_guard guard;
        REQUIRE_EQ(featherdoc::detail::checked_clone_xml_document(
                       baseline_source, baseline_destination),
                   featherdoc::detail::xml_document_clone_status::success);
        successful_clone_allocation_count = allocation_calls;
    }
    REQUIRE_GT(successful_clone_allocation_count, 2U);

    for (std::size_t current_failure_call = 1U;
         current_failure_call <= successful_clone_allocation_count;
         ++current_failure_call) {
        auto source = make_source_document();
        auto destination = make_destination_document();
        const auto source_before = serialize(source);
        const auto destination_before = serialize(destination);

        featherdoc::detail::xml_document_clone_status status{};
        {
            pugi_allocator_guard guard;
            failure_call = current_failure_call;
            status = featherdoc::detail::checked_clone_xml_document(
                source, destination);
            CAPTURE(current_failure_call);
            CAPTURE(successful_clone_allocation_count);
            CAPTURE(allocation_calls);
            CHECK_EQ(status, featherdoc::detail::xml_document_clone_status::
                                 allocation_failure);
        }

        CHECK_EQ(serialize(source), source_before);
        CHECK_EQ(serialize(destination), destination_before);
    }
}

TEST_CASE(
    "checked XML clone atomically replaces a document with an empty one") {
    pugi::xml_document source;
    auto destination = make_destination_document();

    CHECK_EQ(
        featherdoc::detail::checked_clone_xml_document(source, destination),
        featherdoc::detail::xml_document_clone_status::success);
    CHECK(destination.first_child() == pugi::xml_node{});
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "checked XML attribute creation removes its shell on every pugixml "
    "allocation failure") {
    const std::string large_value(70U * 1024U, 'v');

    for (const bool use_set_or_append : {false, true}) {
        CAPTURE(use_set_or_append);
        std::size_t successful_allocation_count = 0U;
        {
            auto baseline = make_destination_document();
            pugi_allocator_guard guard;
            const auto root = baseline.document_element();
            const auto succeeded =
                use_set_or_append
                    ? featherdoc::detail::checked_set_xml_attribute_value(
                          root, "large-value", large_value)
                    : featherdoc::detail::checked_append_xml_attribute(
                          root, "large-value", large_value);
            REQUIRE(succeeded);
            successful_allocation_count = allocation_calls;
        }
        REQUIRE_GT(successful_allocation_count, 0U);

        for (std::size_t current_failure_call = 1U;
             current_failure_call <= successful_allocation_count;
             ++current_failure_call) {
            auto document = make_destination_document();
            const auto before = serialize(document);
            bool succeeded = true;
            {
                pugi_allocator_guard guard;
                failure_call = current_failure_call;
                const auto root = document.document_element();
                succeeded =
                    use_set_or_append
                        ? featherdoc::detail::checked_set_xml_attribute_value(
                              root, "large-value", large_value)
                        : featherdoc::detail::checked_append_xml_attribute(
                              root, "large-value", large_value);
                CAPTURE(current_failure_call);
                CAPTURE(successful_allocation_count);
                CHECK_FALSE(succeeded);
            }
            CHECK_EQ(serialize(document), before);
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "checked XML element creation removes its shell on every pugixml "
    "name allocation failure") {
    const std::string large_name(70U * 1024U, 'n');
    std::size_t successful_allocation_count = 0U;
    {
        auto baseline = make_destination_document();
        pugi_allocator_guard guard;
        REQUIRE(featherdoc::detail::checked_append_xml_element(
                    baseline.document_element(), large_name.c_str()) !=
                pugi::xml_node{});
        successful_allocation_count = allocation_calls;
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t current_failure_call = 1U;
         current_failure_call <= successful_allocation_count;
         ++current_failure_call) {
        auto document = make_destination_document();
        const auto before = serialize(document);
        auto child = pugi::xml_node{};
        {
            pugi_allocator_guard guard;
            failure_call = current_failure_call;
            child = featherdoc::detail::checked_append_xml_element(
                document.document_element(), large_name.c_str());
            CAPTURE(current_failure_call);
            CAPTURE(successful_allocation_count);
            CHECK(child == pugi::xml_node{});
        }
        CHECK_EQ(serialize(document), before);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "fixed XML initialization preserves the target for every parser "
    "allocation failure") {
    constexpr auto fixed_relationships_xml = std::string_view{
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"/>)"};

    auto baseline_destination = make_destination_document();
    std::size_t successful_initialization_allocation_count = 0U;
    {
        pugi_allocator_guard guard;
        const auto result = featherdoc::detail::initialize_fixed_xml_document(
            baseline_destination, fixed_relationships_xml);
        REQUIRE(static_cast<bool>(result));
        REQUIRE_EQ(
            result.status,
            featherdoc::detail::fixed_xml_initialization_status::success);
        successful_initialization_allocation_count = allocation_calls;
    }
    REQUIRE_GT(successful_initialization_allocation_count, 0U);

    for (std::size_t current_failure_call = 1U;
         current_failure_call <= successful_initialization_allocation_count;
         ++current_failure_call) {
        auto destination = make_destination_document();
        const auto destination_before = serialize(destination);
        featherdoc::detail::fixed_xml_initialization_result result;
        {
            pugi_allocator_guard guard;
            failure_call = current_failure_call;
            result = featherdoc::detail::initialize_fixed_xml_document(
                destination, fixed_relationships_xml);
            CAPTURE(current_failure_call);
            CAPTURE(successful_initialization_allocation_count);
            CAPTURE(allocation_calls);
            CHECK_FALSE(static_cast<bool>(result));
            CHECK_EQ(result.status,
                     featherdoc::detail::fixed_xml_initialization_status::
                         allocation_failure);
            REQUIRE(result.parse_result.has_value());
            CHECK_EQ(result.parse_result->status, pugi::status_out_of_memory);
        }
        CHECK_EQ(serialize(destination), destination_before);
    }
}

TEST_CASE("empty Relationships initialization preserves and reports an invalid "
          "existing DOM without a synthetic parser diagnostic") {
    auto destination = make_destination_document();
    const auto destination_before = serialize(destination);

    const auto result =
        featherdoc::detail::initialize_empty_relationships_document(
            destination);
    CHECK_FALSE(static_cast<bool>(result));
    CHECK_EQ(
        result.status,
        featherdoc::detail::fixed_xml_initialization_status::parse_failure);
    CHECK_FALSE(result.parse_result.has_value());
    CHECK_EQ(serialize(destination), destination_before);

    featherdoc::document_error_info error_info;
    const auto error = featherdoc::detail::set_fixed_xml_initialization_failure(
        error_info, result,
        featherdoc::document_errc::relationships_xml_parse_failed,
        "word/_rels/header1.xml.rels");
    CHECK_EQ(error, featherdoc::document_errc::relationships_xml_parse_failed);
    CHECK_EQ(error_info.code, error);
    CHECK_EQ(error_info.detail,
             "fixed Relationships XML initialization was refused for an "
             "invalid existing DOM");
    CHECK_EQ(error_info.entry_name, "word/_rels/header1.xml.rels");
    CHECK_FALSE(error_info.xml_offset.has_value());
}

TEST_CASE("singleton part attachment rejects an invalid Relationships source "
          "without modifying source or work documents") {
    constexpr auto invalid_relationships_documents = std::array{
        std::string_view{
            R"(<NotRelationships xmlns="urn:invalid"><child/></NotRelationships>)"},
        std::string_view{
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="urn:missing-target"/></Relationships>)"},
        std::string_view{
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="urn:first" Target="first.xml"/><Relationship Id="rId1" Type="urn:second" Target="second.xml"/></Relationships>)"},
    };

    for (const auto invalid_relationships_xml :
         invalid_relationships_documents) {
        CAPTURE(invalid_relationships_xml);
        pugi::xml_document relationships;
        REQUIRE(relationships.load_buffer(invalid_relationships_xml.data(),
                                          invalid_relationships_xml.size()));
        pugi::xml_document content_types;
        REQUIRE(content_types.load_string(
            R"(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Override PartName="/word/document.xml" ContentType="application/example+xml"/></Types>)"));

        featherdoc::detail::singleton_part_attachment_work work;
        work.relationships = make_destination_document();
        work.content_types = make_destination_document();
        work.relationships_changed = true;
        work.content_types_changed = true;
        const auto relationships_before = serialize(relationships);
        const auto content_types_before = serialize(content_types);
        const auto work_relationships_before = serialize(work.relationships);
        const auto work_content_types_before = serialize(work.content_types);

        const auto options =
            featherdoc::detail::singleton_part_attachment_options{
                "http://schemas.openxmlformats.org/officeDocument/2006/"
                "relationships/settings",
                "settings.xml",
                "/word/settings.xml",
                "application/vnd.openxmlformats-officedocument."
                "wordprocessingml.settings+xml",
                "word/_rels/document.xml.rels",
                "[Content_Types].xml",
                featherdoc::document_errc::relationships_xml_parse_failed,
                featherdoc::document_errc::content_types_xml_parse_failed};
        featherdoc::document_error_info error_info;

        const auto error =
            featherdoc::detail::prepare_checked_singleton_part_attachment(
                relationships, content_types, options, error_info, work);

        CHECK_EQ(error,
                 featherdoc::document_errc::relationships_xml_parse_failed);
        CHECK_EQ(error_info.code, error);
        CHECK_EQ(error_info.entry_name, "word/_rels/document.xml.rels");
        CHECK_EQ(serialize(relationships), relationships_before);
        CHECK_EQ(serialize(content_types), content_types_before);
        CHECK_EQ(serialize(work.relationships), work_relationships_before);
        CHECK_EQ(serialize(work.content_types), work_content_types_before);
        CHECK(work.relationships_changed);
        CHECK(work.content_types_changed);
    }
}

TEST_CASE("singleton part attachment rejects structurally invalid Content "
          "Types without modifying source or work documents") {
    const auto check_invalid_content_types = [](pugi::xml_document
                                                    &content_types) {
        pugi::xml_document relationships;
        REQUIRE(relationships.load_string(
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"/>)"));

        featherdoc::detail::singleton_part_attachment_work work;
        work.relationships = make_destination_document();
        work.content_types = make_destination_document();
        work.relationships_changed = true;
        work.content_types_changed = true;
        const auto relationships_before = serialize(relationships);
        const auto content_types_before = serialize(content_types);
        const auto work_relationships_before = serialize(work.relationships);
        const auto work_content_types_before = serialize(work.content_types);

        const auto options =
            featherdoc::detail::singleton_part_attachment_options{
                "http://schemas.openxmlformats.org/officeDocument/2006/"
                "relationships/settings",
                "settings.xml",
                "/word/settings.xml",
                "application/vnd.openxmlformats-officedocument."
                "wordprocessingml.settings+xml",
                "word/_rels/document.xml.rels",
                "[Content_Types].xml",
                featherdoc::document_errc::relationships_xml_parse_failed,
                featherdoc::document_errc::content_types_xml_parse_failed};
        featherdoc::document_error_info error_info;

        const auto error =
            featherdoc::detail::prepare_checked_singleton_part_attachment(
                relationships, content_types, options, error_info, work);

        CHECK_EQ(error,
                 featherdoc::document_errc::content_types_xml_parse_failed);
        CHECK_EQ(error_info.code, error);
        CHECK_EQ(error_info.entry_name, "[Content_Types].xml");
        CHECK_EQ(serialize(relationships), relationships_before);
        CHECK_EQ(serialize(content_types), content_types_before);
        CHECK_EQ(serialize(work.relationships), work_relationships_before);
        CHECK_EQ(serialize(work.content_types), work_content_types_before);
        CHECK(work.relationships_changed);
        CHECK(work.content_types_changed);
    };

    pugi::xml_document unexpected_child;
    REQUIRE(unexpected_child.load_string(
        R"(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Unexpected/></Types>)"));
    check_invalid_content_types(unexpected_child);

    pugi::xml_document duplicate_attribute;
    REQUIRE(duplicate_attribute.load_string(
        R"(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Override PartName="/word/document.xml" ContentType="application/example+xml"/></Types>)"));
    auto override_node =
        duplicate_attribute.document_element().child("Override");
    REQUIRE(override_node != pugi::xml_node{});
    REQUIRE(override_node.append_attribute("PartName")
                .set_value("/word/duplicate.xml"));
    check_invalid_content_types(duplicate_attribute);
}
