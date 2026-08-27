#include "../src/wordprocessingml_namespace_helpers.hpp"

#include <constants.hpp>
#include <featherdoc/document_core.hpp>

#include <array>
#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

namespace {

using featherdoc::detail::canonicalize_wordprocessingml_part;
using featherdoc::detail::wordprocessingml_namespace_limits;
using featherdoc::detail::wordprocessingml_namespace_status;
using featherdoc::detail::wordprocessingml_part_kind;

constexpr auto wml_namespace = std::string_view{
    "http://schemas.openxmlformats.org/wordprocessingml/2006/main"};
constexpr auto strict_wml_namespace =
    std::string_view{"http://purl.oclc.org/ooxml/wordprocessingml/main"};

auto parse_xml(pugi::xml_document &document, std::string_view xml) -> void {
    const auto parsed = document.load_buffer(
        xml.data(), xml.size(),
        pugi::parse_default | pugi::parse_ws_pcdata | pugi::parse_pi |
            pugi::parse_comments | pugi::parse_declaration);
    REQUIRE_MESSAGE(parsed, parsed.description());
}

auto serialize(const pugi::xml_document &document) -> std::string {
    std::ostringstream output;
    document.save(output, "", pugi::format_raw, pugi::encoding_utf8);
    return output.str();
}

struct part_case final {
    wordprocessingml_part_kind kind;
    std::string_view root_local_name;
};

constexpr auto part_cases = std::array{
    part_case{wordprocessingml_part_kind::main_document, "document"},
    part_case{wordprocessingml_part_kind::header, "hdr"},
    part_case{wordprocessingml_part_kind::footer, "ftr"},
    part_case{wordprocessingml_part_kind::styles, "styles"},
    part_case{wordprocessingml_part_kind::numbering, "numbering"},
    part_case{wordprocessingml_part_kind::settings, "settings"},
    part_case{wordprocessingml_part_kind::footnotes, "footnotes"},
    part_case{wordprocessingml_part_kind::endnotes, "endnotes"},
    part_case{wordprocessingml_part_kind::comments, "comments"},
};

} // namespace

TEST_CASE("WordprocessingML canonicalizer recognizes all supported part "
          "roots") {
    for (const auto &[kind, root_local_name] : part_cases) {
        pugi::xml_document document;
        const auto xml = std::string{"<alias:"} + std::string{root_local_name} +
                         " xmlns:alias=\"" + std::string{wml_namespace} +
                         "\"/>";
        parse_xml(document, xml);

        const auto result = canonicalize_wordprocessingml_part(document, kind);
        REQUIRE(result);
        CHECK(result.changed);
        CHECK(result.root_matches);
        CHECK_EQ(result.actual_root_namespace_uri, wml_namespace);
        CHECK_EQ(result.actual_root_local_name, root_local_name);

        const auto expected_name =
            std::string{"w:"} + std::string{root_local_name};
        const auto root = document.document_element();
        CHECK_EQ(std::string_view{root.name()}, expected_name);
        CHECK_EQ(std::string_view{root.attribute("xmlns:alias").value()},
                 wml_namespace);
        CHECK_EQ(std::string_view{root.attribute("xmlns:w").value()},
                 wml_namespace);
    }
}

TEST_CASE("WordprocessingML root mismatches are reported for every supported "
          "part kind") {
    for (const auto &part : part_cases) {
        pugi::xml_document document;
        parse_xml(document, std::string{"<alias:unexpected xmlns:alias=\""} +
                                std::string{wml_namespace} + "\"/>");

        const auto result =
            canonicalize_wordprocessingml_part(document, part.kind);
        REQUIRE(result);
        CHECK_FALSE(result.root_matches);
        CHECK_EQ(result.actual_root_namespace_uri, wml_namespace);
        CHECK_EQ(result.actual_root_local_name, "unexpected");

        featherdoc::document_error_info error_info;
        CHECK_EQ(
            featherdoc::detail::set_wordprocessingml_root_mismatch_last_error(
                error_info, result, part.kind, "word/part.xml"),
            featherdoc::document_errc::invalid_package_structure);
        CHECK_EQ(error_info.entry_name, "word/part.xml");
        CHECK_NE(error_info.detail.find(part.root_local_name),
                 std::string::npos);
        CHECK_NE(error_info.detail.find("unexpected"), std::string::npos);
        CHECK_NE(error_info.detail.find("entry_name="), std::string::npos);
        CHECK_NE(error_info.detail.find("part_role="), std::string::npos);
        CHECK_NE(error_info.detail.find("expected_local_name="),
                 std::string::npos);
        CHECK_NE(error_info.detail.find("actual_local_name="),
                 std::string::npos);
    }
}

TEST_CASE("WordprocessingML canonicalizer handles default, alias, and Unicode "
          "prefixes by expanded QName") {
    SUBCASE("the default namespace applies to elements but not attributes") {
        pugi::xml_document document;
        parse_xml(
            document,
            R"(<document xmlns="http://schemas.openxmlformats.org/wordprocessingml/2006/main" plain="keep"><body><p val="unqualified"/></body></document>)");

        const auto result = canonicalize_wordprocessingml_part(
            document, wordprocessingml_part_kind::main_document);
        REQUIRE(result);
        CHECK(result.changed);
        CHECK(result.root_matches);

        const auto root = document.child("w:document");
        REQUIRE(root != pugi::xml_node{});
        CHECK_EQ(std::string_view{root.attribute("plain").value()}, "keep");
        CHECK(root.attribute("w:plain") == pugi::xml_attribute{});
        const auto paragraph = root.child("w:body").child("w:p");
        REQUIRE(paragraph != pugi::xml_node{});
        CHECK_EQ(std::string_view{paragraph.attribute("val").value()},
                 "unqualified");
        CHECK_EQ(std::string_view{root.attribute("xmlns").value()},
                 wml_namespace);
    }

    SUBCASE("a Unicode namespace prefix is accepted and retained") {
        pugi::xml_document document;
        parse_xml(
            document,
            R"(<文字:document xmlns:文字="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><文字:body 文字:属性="中文🙂"/></文字:document>)");

        const auto result = canonicalize_wordprocessingml_part(
            document, wordprocessingml_part_kind::main_document);
        REQUIRE(result);
        CHECK(result.root_matches);
        const auto root = document.child("w:document");
        REQUIRE(root != pugi::xml_node{});
        CHECK_EQ(std::string_view{root.attribute("xmlns:文字").value()},
                 wml_namespace);
        CHECK_EQ(
            std::string_view{root.child("w:body").attribute("w:属性").value()},
            "中文🙂");
    }
}

TEST_CASE("WordprocessingML canonicalizer preserves extension markup and "
          "QName-valued attribute strings") {
    pugi::xml_document document;
    parse_xml(
        document,
        R"(<?xml version="1.0" encoding="UTF-8"?><a:document xmlns:a="http://schemas.openxmlformats.org/wordprocessingml/2006/main" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension" xmlns:map="urn:mapping" mc:Ignorable="x" mc:PreserveElements="x:Keep"><a:body><mc:AlternateContent><mc:Choice Requires="x"><x:Keep a:custom="word-value"><a:dataBinding a:prefixMappings="xmlns:map='urn:mapping'" a:xpath="/map:根/map:值" a:storeItemID="{ABC}"/></x:Keep></mc:Choice></mc:AlternateContent><![CDATA[中文🙂]]></a:body></a:document>)");

    const auto result = canonicalize_wordprocessingml_part(
        document, wordprocessingml_part_kind::main_document);
    REQUIRE(result);
    CHECK(result.root_matches);

    const auto root = document.child("w:document");
    REQUIRE(root != pugi::xml_node{});
    CHECK_EQ(std::string_view{root.attribute("mc:Ignorable").value()}, "x");
    CHECK_EQ(std::string_view{root.attribute("mc:PreserveElements").value()},
             "x:Keep");
    CHECK_EQ(std::string_view{root.attribute("xmlns:a").value()},
             wml_namespace);
    CHECK_EQ(std::string_view{root.attribute("xmlns:x").value()},
             "urn:extension");
    CHECK_EQ(std::string_view{root.attribute("xmlns:map").value()},
             "urn:mapping");

    const auto choice =
        root.child("w:body").child("mc:AlternateContent").child("mc:Choice");
    REQUIRE(choice != pugi::xml_node{});
    CHECK_EQ(std::string_view{choice.attribute("Requires").value()}, "x");
    const auto extension = choice.child("x:Keep");
    REQUIRE(extension != pugi::xml_node{});
    CHECK_EQ(std::string_view{extension.attribute("w:custom").value()},
             "word-value");
    const auto binding = extension.child("w:dataBinding");
    REQUIRE(binding != pugi::xml_node{});
    CHECK_EQ(std::string_view{binding.attribute("w:prefixMappings").value()},
             "xmlns:map='urn:mapping'");
    CHECK_EQ(std::string_view{binding.attribute("w:xpath").value()},
             "/map:根/map:值");
    CHECK_EQ(std::string_view{binding.attribute("w:storeItemID").value()},
             "{ABC}");
    CHECK_EQ(root.child("w:body").last_child().type(), pugi::node_cdata);
    CHECK_EQ(std::string_view{root.child("w:body").last_child().value()},
             "中文🙂");
}

TEST_CASE("WordprocessingML canonicalizer rejects unsafe namespace markup") {
    SUBCASE("the internal w prefix cannot target another namespace") {
        pugi::xml_document document;
        parse_xml(document,
                  R"(<w:document xmlns:w="urn:wrong"><w:body/></w:document>)");
        const auto result = canonicalize_wordprocessingml_part(
            document, wordprocessingml_part_kind::main_document);
        CHECK_EQ(result.status,
                 wordprocessingml_namespace_status::wrong_w_binding);
    }

    SUBCASE("an unused descendant w rebinding is still rejected") {
        pugi::xml_document document;
        parse_xml(
            document,
            R"(<a:document xmlns:a="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><a:body><x:item xmlns:x="urn:item" xmlns:w="urn:wrong"/></a:body></a:document>)");
        const auto before = serialize(document);
        const auto result = canonicalize_wordprocessingml_part(
            document, wordprocessingml_part_kind::main_document);
        CHECK_EQ(result.status,
                 wordprocessingml_namespace_status::wrong_w_binding);
        CHECK_EQ(serialize(document), before);
    }

    SUBCASE("unbound element and attribute prefixes are rejected") {
        pugi::xml_document element_document;
        parse_xml(
            element_document,
            R"(<a:document xmlns:a="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><missing:body/></a:document>)");
        CHECK_EQ(
            canonicalize_wordprocessingml_part(
                element_document, wordprocessingml_part_kind::main_document)
                .status,
            wordprocessingml_namespace_status::invalid_namespace_markup);

        pugi::xml_document attribute_document;
        parse_xml(
            attribute_document,
            R"(<a:document xmlns:a="http://schemas.openxmlformats.org/wordprocessingml/2006/main" missing:value="x"/>)");
        CHECK_EQ(
            canonicalize_wordprocessingml_part(
                attribute_document, wordprocessingml_part_kind::main_document)
                .status,
            wordprocessingml_namespace_status::invalid_namespace_markup);
    }

    SUBCASE("attributes with the same expanded name are rejected") {
        pugi::xml_document document;
        parse_xml(
            document,
            R"(<a:document xmlns:a="http://schemas.openxmlformats.org/wordprocessingml/2006/main" xmlns:b="http://schemas.openxmlformats.org/wordprocessingml/2006/main" a:value="one" b:value="two"/>)");
        const auto result = canonicalize_wordprocessingml_part(
            document, wordprocessingml_part_kind::main_document);
        INFO(result.detail);
        CHECK_EQ(
            result.status,
            wordprocessingml_namespace_status::duplicate_expanded_attribute);
    }
}

TEST_CASE("WordprocessingML canonicalizer permits unrelated local rebinding "
          "and default namespace reset") {
    pugi::xml_document document;
    parse_xml(
        document,
        R"(<a:document xmlns:a="http://schemas.openxmlformats.org/wordprocessingml/2006/main" xmlns:x="urn:outer" xmlns="urn:default"><a:body><x:item xmlns:x="urn:inner"><plain xmlns=""><x:child/></plain></x:item></a:body></a:document>)");

    const auto result = canonicalize_wordprocessingml_part(
        document, wordprocessingml_part_kind::main_document);
    REQUIRE(result);
    CHECK(result.root_matches);
    const auto root = document.child("w:document");
    const auto item = root.child("w:body").child("x:item");
    REQUIRE(item != pugi::xml_node{});
    CHECK_EQ(std::string_view{item.attribute("xmlns:x").value()}, "urn:inner");
    const auto plain = item.child("plain");
    REQUIRE(plain != pugi::xml_node{});
    CHECK(plain.attribute("xmlns") != pugi::xml_attribute{});
    CHECK_EQ(std::string_view{plain.attribute("xmlns").value()}, "");
    CHECK(plain.child("x:child") != pugi::xml_node{});
}

TEST_CASE("WordprocessingML canonicalization failures leave the original DOM "
          "unchanged") {
    pugi::xml_document document;
    parse_xml(
        document,
        R"(<?xml version="1.0"?><a:document xmlns:a="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><a:body a:value="keep"><a:p/></a:body></a:document>)");
    const auto before = serialize(document);

    wordprocessingml_namespace_limits limits;
    limits.maximum_elements = 2U;
    const auto result = canonicalize_wordprocessingml_part(
        document, wordprocessingml_part_kind::main_document, limits);
    CHECK_EQ(result.status, wordprocessingml_namespace_status::resource_limit);
    CHECK_EQ(serialize(document), before);

    limits.maximum_elements = 100U;
    limits.maximum_attributes = 1U;
    const auto attribute_result = canonicalize_wordprocessingml_part(
        document, wordprocessingml_part_kind::main_document, limits);
    CHECK_EQ(attribute_result.status,
             wordprocessingml_namespace_status::resource_limit);
    CHECK_EQ(serialize(document), before);

    limits.maximum_attributes = 100U;
    limits.maximum_depth = 2U;
    const auto depth_result = canonicalize_wordprocessingml_part(
        document, wordprocessingml_part_kind::main_document, limits);
    CHECK_EQ(depth_result.status,
             wordprocessingml_namespace_status::resource_limit);
    CHECK_EQ(serialize(document), before);
}

TEST_CASE("WordprocessingML synthetic bindings are complete and obey the "
          "attribute limit") {
    SUBCASE("each top-level subtree receives a required w binding") {
        pugi::xml_document document;
        auto first = document.append_child("a:document");
        REQUIRE(
            first.append_attribute("xmlns:a").set_value(wml_namespace.data()));
        auto second = document.append_child("b:document");
        REQUIRE(
            second.append_attribute("xmlns:b").set_value(wml_namespace.data()));

        const auto result = canonicalize_wordprocessingml_part(
            document, wordprocessingml_part_kind::main_document);
        REQUIRE(result);
        CHECK_FALSE(result.root_matches);
        first = document.first_child();
        second = first.next_sibling();
        CHECK_EQ(std::string_view{first.name()}, "w:document");
        CHECK_EQ(std::string_view{second.name()}, "w:document");
        CHECK_EQ(std::string_view{first.attribute("xmlns:w").value()},
                 wml_namespace);
        CHECK_EQ(std::string_view{second.attribute("xmlns:w").value()},
                 wml_namespace);
    }

    SUBCASE("a synthesized declaration counts toward the attribute cap") {
        pugi::xml_document document;
        parse_xml(
            document,
            R"(<a:document xmlns:a="http://schemas.openxmlformats.org/wordprocessingml/2006/main"/>)");
        const auto before = serialize(document);
        wordprocessingml_namespace_limits limits;
        limits.maximum_attributes = 1U;

        const auto result = canonicalize_wordprocessingml_part(
            document, wordprocessingml_part_kind::main_document, limits);
        CHECK_EQ(result.status,
                 wordprocessingml_namespace_status::resource_limit);
        CHECK_EQ(serialize(document), before);
    }

    SUBCASE("XML declaration attributes cannot bypass the attribute cap") {
        pugi::xml_document document;
        parse_xml(
            document,
            R"(<?xml version="1.0" encoding="UTF-8"?><document xmlns="urn:not-wml"/>)");
        const auto before = serialize(document);
        wordprocessingml_namespace_limits limits;
        limits.maximum_attributes = 1U;

        const auto result = canonicalize_wordprocessingml_part(
            document, wordprocessingml_part_kind::main_document, limits);
        CHECK_EQ(result.status,
                 wordprocessingml_namespace_status::resource_limit);
        CHECK_EQ(serialize(document), before);
    }

    SUBCASE("a non-element xmlns:w attribute cannot bypass binding checks") {
        pugi::xml_document document;
        auto declaration = document.append_child(pugi::node_declaration);
        REQUIRE(declaration != pugi::xml_node{});
        REQUIRE(declaration.append_attribute("version").set_value("1.0"));
        REQUIRE(declaration.append_attribute("xmlns:w").set_value("urn:wrong"));
        auto root = document.append_child("a:document");
        REQUIRE(
            root.append_attribute("xmlns:a").set_value(wml_namespace.data()));
        const auto before = serialize(document);

        const auto result = canonicalize_wordprocessingml_part(
            document, wordprocessingml_part_kind::main_document);
        CHECK_EQ(result.status,
                 wordprocessingml_namespace_status::wrong_w_binding);
        CHECK_EQ(serialize(document), before);
    }
}

TEST_CASE("WordprocessingML canonicalizer is iterative for deeply nested XML") {
    constexpr std::size_t nested_element_count = 25'000U;
    pugi::xml_document document;
    auto root = document.append_child("alias:document");
    REQUIRE(root != pugi::xml_node{});
    REQUIRE(
        root.append_attribute("xmlns:alias").set_value(wml_namespace.data()));
    auto current = root.append_child("alias:body");
    REQUIRE(current != pugi::xml_node{});
    for (std::size_t index = 0U; index < nested_element_count; ++index) {
        current = current.append_child("alias:p");
        REQUIRE(current != pugi::xml_node{});
    }

    const auto result = canonicalize_wordprocessingml_part(
        document, wordprocessingml_part_kind::main_document);
    REQUIRE(result);
    CHECK(result.root_matches);
    current = document.child("w:document").child("w:body");
    for (std::size_t index = 0U; index < nested_element_count; ++index) {
        current = current.child("w:p");
        REQUIRE(current != pugi::xml_node{});
    }
}

TEST_CASE("WordprocessingML strict namespace is unsupported") {
    SUBCASE("a strict default namespace is not mistaken for transitional WML") {
        pugi::xml_document document;
        const auto xml = std::string{"<document xmlns=\""} +
                         std::string{strict_wml_namespace} + "\"/>";
        parse_xml(document, xml);
        const auto result = canonicalize_wordprocessingml_part(
            document, wordprocessingml_part_kind::main_document);
        REQUIRE(result);
        CHECK_FALSE(result.changed);
        CHECK_FALSE(result.root_matches);
        CHECK_EQ(result.actual_root_namespace_uri, strict_wml_namespace);
        CHECK_EQ(std::string_view{document.document_element().name()},
                 "document");
    }

    SUBCASE("the w prefix cannot be rebound to the strict namespace") {
        pugi::xml_document document;
        const auto xml = std::string{"<w:document xmlns:w=\""} +
                         std::string{strict_wml_namespace} + "\"/>";
        parse_xml(document, xml);
        CHECK_EQ(canonicalize_wordprocessingml_part(
                     document, wordprocessingml_part_kind::main_document)
                     .status,
                 wordprocessingml_namespace_status::wrong_w_binding);
    }
}

TEST_CASE("WordprocessingML namespace statuses map to stable public errors") {
    using featherdoc::detail::wordprocessingml_namespace_error_code;

    CHECK_FALSE(wordprocessingml_namespace_error_code(
        wordprocessingml_namespace_status::success));
    CHECK_EQ(wordprocessingml_namespace_error_code(
                 wordprocessingml_namespace_status::invalid_namespace_markup),
             featherdoc::make_error_code(
                 featherdoc::document_errc::invalid_package_structure));
    CHECK_EQ(wordprocessingml_namespace_error_code(
                 wordprocessingml_namespace_status::wrong_w_binding),
             featherdoc::make_error_code(
                 featherdoc::document_errc::invalid_package_structure));
    CHECK_EQ(
        wordprocessingml_namespace_error_code(
            wordprocessingml_namespace_status::duplicate_expanded_attribute),
        featherdoc::make_error_code(
            featherdoc::document_errc::invalid_package_structure));
    CHECK_EQ(wordprocessingml_namespace_error_code(
                 wordprocessingml_namespace_status::resource_limit),
             featherdoc::make_error_code(
                 featherdoc::document_errc::archive_limit_exceeded));
    CHECK_EQ(wordprocessingml_namespace_error_code(
                 wordprocessingml_namespace_status::allocation_failure),
             std::make_error_code(std::errc::not_enough_memory));

    featherdoc::document_error_info allocation_error;
    allocation_error.detail = "stale detail";
    allocation_error.entry_name = "stale entry";
    featherdoc::detail::wordprocessingml_namespace_result allocation_result;
    allocation_result.status =
        wordprocessingml_namespace_status::allocation_failure;
    CHECK_EQ(featherdoc::detail::set_wordprocessingml_namespace_last_error(
                 allocation_error, allocation_result, "word/document.xml"),
             std::make_error_code(std::errc::not_enough_memory));
    CHECK(allocation_error.detail.empty());
    CHECK(allocation_error.entry_name.empty());
}
