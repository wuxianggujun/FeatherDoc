#include "../src/package_path_helpers.hpp"
#include "../src/package_relationships_xml_helpers.hpp"

#include <string>
#include <string_view>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

TEST_CASE("package relationship QName helpers resolve namespace prefixes") {
    pugi::xml_document relationships;
    REQUIRE(relationships.load_string(
        R"(<opc:Relationships xmlns:opc="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:item="http://schemas.openxmlformats.org/package/2006/relationships"><item:Relationship Id="rOne" Type="urn:one" Target="one.bin"/><opc:Relationship Id="rTwo" Type="urn:two" Target="two.bin"/></opc:Relationships>)"));

    const auto root =
        featherdoc::detail::package_relationships_root(relationships);
    REQUIRE(root != pugi::xml_node{});
    CHECK_EQ(std::string_view{root.name()}, "opc:Relationships");
    const auto first = featherdoc::detail::first_package_relationship(root);
    REQUIRE(first != pugi::xml_node{});
    CHECK_EQ(std::string_view{first.name()}, "item:Relationship");
    const auto second = featherdoc::detail::next_package_relationship(first);
    REQUIRE(second != pugi::xml_node{});
    CHECK_EQ(std::string_view{second.name()}, "opc:Relationship");
    CHECK(featherdoc::detail::next_package_relationship(second) ==
          pugi::xml_node{});
    CHECK(featherdoc::detail::inspect_package_relationships_document(
              relationships) ==
          featherdoc::detail::package_relationships_document_state::valid);

    const auto appended = featherdoc::detail::append_package_relationship(root);
    REQUIRE(appended != pugi::xml_node{});
    CHECK_EQ(std::string_view{appended.name()}, "opc:Relationship");
}

TEST_CASE("package relationship QName validation rejects namespace and "
          "attribute escapes") {
    const auto check_invalid = [](std::string_view xml) {
        pugi::xml_document relationships;
        REQUIRE(relationships.load_buffer(xml.data(), xml.size()));
        CHECK(
            featherdoc::detail::inspect_package_relationships_document(
                relationships) ==
            featherdoc::detail::package_relationships_document_state::invalid);
    };

    SUBCASE("unprefixed child has no inherited default namespace") {
        check_invalid(
            R"(<opc:Relationships xmlns:opc="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rOne" Type="urn:one" Target="one.bin"/></opc:Relationships>)");
    }
    SUBCASE("child prefix is rebound") {
        check_invalid(
            R"(<opc:Relationships xmlns:opc="http://schemas.openxmlformats.org/package/2006/relationships"><opc:Relationship xmlns:opc="urn:wrong" Id="rOne" Type="urn:one" Target="one.bin"/></opc:Relationships>)");
    }
    SUBCASE("child prefix is undeclared") {
        check_invalid(
            R"(<opc:Relationships xmlns:opc="http://schemas.openxmlformats.org/package/2006/relationships"><missing:Relationship Id="rOne" Type="urn:one" Target="one.bin"/></opc:Relationships>)");
    }
    SUBCASE("relationship attributes remain unqualified") {
        check_invalid(
            R"(<opc:Relationships xmlns:opc="http://schemas.openxmlformats.org/package/2006/relationships"><opc:Relationship opc:Id="rOne" Type="urn:one" Target="one.bin"/></opc:Relationships>)");
    }
    SUBCASE("unknown root attributes are rejected") {
        check_invalid(
            R"(<opc:Relationships xmlns:opc="http://schemas.openxmlformats.org/package/2006/relationships" Extra="invalid"><opc:Relationship Id="rOne" Type="urn:one" Target="one.bin"/></opc:Relationships>)");
    }
    SUBCASE("duplicate namespace declarations are rejected") {
        check_invalid(
            R"(<opc:Relationships xmlns:opc="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:opc="urn:shadow"><opc:Relationship Id="rOne" Type="urn:one" Target="one.bin"/></opc:Relationships>)");
    }
    SUBCASE("unknown relationship attributes are rejected") {
        check_invalid(
            R"(<opc:Relationships xmlns:opc="http://schemas.openxmlformats.org/package/2006/relationships"><opc:Relationship Id="rOne" Type="urn:one" Target="one.bin" Extra="invalid"/></opc:Relationships>)");
    }
    SUBCASE("duplicate relationship attributes are rejected") {
        check_invalid(
            R"(<opc:Relationships xmlns:opc="http://schemas.openxmlformats.org/package/2006/relationships"><opc:Relationship Id="rOne" Id="rTwo" Type="urn:one" Target="one.bin"/></opc:Relationships>)");
    }
    SUBCASE("reserved XML prefix cannot be rebound") {
        check_invalid(
            R"(<xml:Relationships xmlns:xml="http://schemas.openxmlformats.org/package/2006/relationships"><xml:Relationship Id="rOne" Type="urn:one" Target="one.bin"/></xml:Relationships>)");
    }
    SUBCASE("duplicate identifiers span child prefixes") {
        check_invalid(
            R"(<opc:Relationships xmlns:opc="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:item="http://schemas.openxmlformats.org/package/2006/relationships"><opc:Relationship Id="rOne" Type="urn:one" Target="one.bin"/><item:Relationship Id="rOne" Type="urn:two" Target="two.bin"/></opc:Relationships>)");
    }
}

TEST_CASE("package paths normalize independently of host filesystem encoding") {
    CHECK_EQ(featherdoc::detail::normalize_package_path(
                 "word/目录/../样式-日本語-🙂.xml"),
             "word/样式-日本語-🙂.xml");
    CHECK_EQ(featherdoc::detail::normalize_package_path(
                 "\\word\\media\\中文 图片.png"),
             "word/media/中文 图片.png");
    CHECK_EQ(featherdoc::detail::normalize_package_path(
                 "word//headers/./header1.xml"),
             "word/headers/header1.xml");
    CHECK_EQ(featherdoc::detail::normalize_package_path("word/../../evil.xml"),
             "../evil.xml");
}

TEST_CASE("package relationship targets resolve as OPC part names") {
    CHECK_EQ(featherdoc::detail::resolve_package_relationship_target(
                 "word/document.xml", "media/图片-🙂.png"),
             "word/media/%E5%9B%BE%E7%89%87-%F0%9F%99%82.png");
    CHECK_EQ(featherdoc::detail::resolve_package_relationship_target(
                 "word/header1.xml", "../customXml/项目.xml"),
             "customXml/%E9%A1%B9%E7%9B%AE.xml");
    CHECK_EQ(featherdoc::detail::resolve_package_relationship_target(
                 "word/document.xml", "/word/页眉.xml"),
             "word/%E9%A1%B5%E7%9C%89.xml");
    CHECK_EQ(featherdoc::detail::resolve_package_relationship_target(
                 "word/document.xml", "word/comments.xml"),
             "word/word/comments.xml");
    CHECK_EQ(featherdoc::detail::resolve_package_relationship_target(
                 "word/document.xml", "media/%E4%B8%AD%E6%96%87.png"),
             "word/media/%E4%B8%AD%E6%96%87.png");
    CHECK_EQ(featherdoc::detail::resolve_package_relationship_target(
                 "word/document.xml", "media/%e4%b8%ad%e6%96%87.png"),
             "word/media/%E4%B8%AD%E6%96%87.png");
    CHECK_EQ(featherdoc::detail::resolve_package_relationship_target(
                 "word/document.xml", "media/%252F.png"),
             "word/media/%252F.png");
    CHECK_EQ(featherdoc::detail::resolve_package_relationship_target(
                 "word/document.xml", "media/name%23x%3F.png"),
             "word/media/name%23x%3F.png");
    CHECK_EQ(featherdoc::detail::resolve_package_relationship_target(
                 "word/document.xml", "./C:foo"),
             "word/C:foo");
}

TEST_CASE("untrusted relationship targets reject ambiguous URI syntax") {
    using featherdoc::detail::package_relationship_target_error;
    using featherdoc::detail::resolve_internal_package_relationship_target;

    const auto check_error = [&](std::string_view target,
                                 package_relationship_target_error error) {
        const auto result = resolve_internal_package_relationship_target(
            "word/document.xml", target);
        CHECK_FALSE(static_cast<bool>(result));
        CHECK_EQ(result.error, error);
        CHECK(result.entry_name.empty());
    };

    check_error("//word/document.xml",
                package_relationship_target_error::authority_reference);
    check_error("word//document.xml",
                package_relationship_target_error::empty_path_segment);
    check_error("word/document.xml/",
                package_relationship_target_error::trailing_directory);
    check_error("\\word\\document.xml",
                package_relationship_target_error::backslash_separator);
    check_error("https://example.test/document.xml",
                package_relationship_target_error::absolute_uri);
    check_error("document.xml?revision=1",
                package_relationship_target_error::query_or_fragment);
    check_error("document.xml#bookmark",
                package_relationship_target_error::query_or_fragment);
    check_error("../../../outside.xml",
                package_relationship_target_error::escapes_package_root);
    check_error("media/bad%ZZ.png",
                package_relationship_target_error::invalid_percent_encoding);
    check_error("media/bad%FF.png",
                package_relationship_target_error::invalid_utf8);
    check_error("media/control%C2%80.png",
                package_relationship_target_error::control_character);
    check_error("media/raw space.png",
                package_relationship_target_error::space_character);
    check_error("media/slash%2Fname.png",
                package_relationship_target_error::encoded_path_delimiter);
    check_error("media/%2E%2E/name.png",
                package_relationship_target_error::invalid_percent_encoding);
    check_error("media/name<draft>.png",
                package_relationship_target_error::invalid_path_character);
    check_error("media/name[1].png",
                package_relationship_target_error::invalid_path_character);
    check_error("media/name|draft.png",
                package_relationship_target_error::invalid_path_character);
    check_error("media/name./image.png",
                package_relationship_target_error::dot_terminated_path_segment);
    check_error("media/.../image.png",
                package_relationship_target_error::dot_only_path_segment);
}

TEST_CASE("package relationship targets are emitted without code-page loss") {
    CHECK_EQ(featherdoc::detail::make_package_relationship_target(
                 "word/document.xml", "word/编号-日本語.xml"),
             "%E7%BC%96%E5%8F%B7-%E6%97%A5%E6%9C%AC%E8%AA%9E.xml");
    CHECK_EQ(featherdoc::detail::make_package_relationship_target(
                 "word/header1.xml", "word/media/徽标 🙂.png"),
             "media/%E5%BE%BD%E6%A0%87%20%F0%9F%99%82.png");
    CHECK_EQ(featherdoc::detail::make_package_relationship_target(
                 "word/document.xml", "customXml/数据.xml"),
             "../customXml/%E6%95%B0%E6%8D%AE.xml");
    CHECK_EQ(featherdoc::detail::make_package_relationship_target(
                 "word/document.xml", "word/a:b.xml"),
             "./a:b.xml");
    CHECK_EQ(featherdoc::detail::make_package_relationship_target(
                 "word/document.xml", "word/C:foo"),
             "./C:foo");
    CHECK(featherdoc::detail::make_package_relationship_target(
              "word/document.xml", "word//invalid.xml")
              .empty());
    CHECK(featherdoc::detail::make_package_relationship_target(
              "word/document.xml", "word/a/../invalid.xml")
              .empty());
    CHECK(featherdoc::detail::make_package_relationship_target(
              "word/document.xml", "word/invalid.")
              .empty());
}

TEST_CASE("content type PartName identity follows OPC URI equivalence") {
    using featherdoc::detail::content_type_part_name_identity;
    using featherdoc::detail::content_type_part_names_equivalent;
    using featherdoc::detail::make_package_content_type_part_name;

    const auto raw_identity =
        content_type_part_name_identity("/word/目录/样式.xml");
    const auto encoded_identity = content_type_part_name_identity(
        "/WORD/%e7%9b%ae%e5%bd%95/%E6%A0%B7%E5%BC%8F.XML");
    REQUIRE(raw_identity.has_value());
    REQUIRE(encoded_identity.has_value());
    CHECK_EQ(*raw_identity, *encoded_identity);
    CHECK(content_type_part_names_equivalent(
        "/word/目录/样式.xml",
        "/WORD/%e7%9b%ae%e5%bd%95/%E6%A0%B7%E5%BC%8F.XML"));
    CHECK_FALSE(content_type_part_names_equivalent("/word/目录/样式.xml",
                                                   "/word/目录/other.xml"));
    CHECK_FALSE(
        content_type_part_name_identity("word/document.xml").has_value());
    CHECK_FALSE(
        content_type_part_name_identity("//word/document.xml").has_value());
    CHECK_FALSE(
        content_type_part_name_identity("/word/../document.xml").has_value());
    CHECK_FALSE(
        content_type_part_name_identity("/word/./document.xml").has_value());
    CHECK_FALSE(
        content_type_part_name_identity("/word//document.xml").has_value());
    CHECK_FALSE(
        content_type_part_name_identity("/word/document.xml/").has_value());
    CHECK_FALSE(
        content_type_part_name_identity("/word/%2E/document.xml").has_value());
    CHECK_FALSE(content_type_part_name_identity("/word/name./document.xml")
                    .has_value());
    CHECK_FALSE(
        content_type_part_name_identity("/word/.../document.xml").has_value());
    CHECK_FALSE(
        content_type_part_name_identity("/word/name[1].xml").has_value());
    CHECK(content_type_part_name_identity("/word/name%23x%3F.xml").has_value());
    CHECK_EQ(make_package_content_type_part_name("word/目录/样式 file.xml"),
             "/word/%E7%9B%AE%E5%BD%95/%E6%A0%B7%E5%BC%8F%20file.xml");
}

TEST_CASE("content type media types validate RFC token and parameter syntax") {
    using featherdoc::detail::content_type_media_type_is_valid;

    SUBCASE("valid media types and parameters are accepted") {
        const std::string_view valid_media_types[]{
            "application/xml",
            "Application/Vnd.Example+Json",
            "x!#$%&'*+-.^_`|~/vnd.example+binary",
            "text/plain;charset=utf-8",
            "text/plain ; charset=utf-8 ; level=1",
            R"(text/plain; empty="")",
            R"(text/plain; note="a b; c=1")",
            R"(text/plain; note="a b; c=\"quoted\"\\tail")",
            "text/plain; note=\"tab\tvalue\"",
            R"(text/plain; title="café")",
            "text/plain; value=!#$%&'*+-.^_`|~",
        };
        for (const auto media_type : valid_media_types) {
            INFO(media_type);
            CHECK(content_type_media_type_is_valid(media_type));
        }
    }

    SUBCASE("malformed base types and parameters are rejected") {
        const std::string_view invalid_media_types[]{
            "",
            "application",
            "/xml",
            "application/",
            "application//xml",
            " application/xml",
            "application/xml ",
            "application /xml",
            "application/ xml",
            "application/(xml)",
            "项目/xml",
            "application/项目",
            "text/plain;",
            "text/plain; charset",
            "text/plain; =utf-8",
            "text/plain; charset=",
            "text/plain; charset =utf-8",
            "text/plain; charset= utf-8",
            "text/plain; 项目=utf-8",
            "text/plain; charset=项目",
            "text/plain; charset=utf-8 trailing",
            R"(text/plain; title="中")",
            R"(text/plain; title="🙂")",
            R"(text/plain; title="caf\é")",
            R"(text/plain; note="unterminated)",
            R"(text/plain; note="closed"trailing)",
            R"media(text/plain; note="dangling\)media",
        };
        for (const auto media_type : invalid_media_types) {
            INFO(media_type);
            CHECK_FALSE(content_type_media_type_is_valid(media_type));
        }
    }

    SUBCASE("invalid UTF-8 and controls are rejected") {
        std::string invalid_utf8 = "text/plain; note=\"";
        invalid_utf8.push_back(static_cast<char>(0xC3U));
        invalid_utf8.push_back('(');
        invalid_utf8.push_back('"');
        CHECK_FALSE(content_type_media_type_is_valid(invalid_utf8));

        std::string nul = "text/plain; note=\"a";
        nul.push_back('\0');
        nul += "b\"";
        CHECK_FALSE(content_type_media_type_is_valid(nul));

        std::string del = "text/plain; note=\"a";
        del.push_back(static_cast<char>(0x7FU));
        del += "b\"";
        CHECK_FALSE(content_type_media_type_is_valid(del));

        std::string c1_control = "text/plain; note=\"a";
        c1_control.push_back(static_cast<char>(0xC2U));
        c1_control.push_back(static_cast<char>(0x85U));
        c1_control += "b\"";
        CHECK_FALSE(content_type_media_type_is_valid(c1_control));

        CHECK_FALSE(content_type_media_type_is_valid(
            "text/plain; note=\"line\nbreak\""));
        CHECK_FALSE(content_type_media_type_is_valid(
            "text/plain; note=\"line\rbreak\""));
        CHECK_FALSE(content_type_media_type_is_valid(
            "text/plain; note=\"bad\\\nbreak\""));
    }
}

TEST_CASE("package filename and extension helpers preserve UTF-8") {
    CHECK_EQ(
        featherdoc::detail::package_path_filename("word/media/图像.最终.PNG"),
        "图像.最终.PNG");
    CHECK_EQ(
        featherdoc::detail::package_path_extension("word/media/图像.最终.PNG"),
        ".PNG");
    CHECK(featherdoc::detail::package_path_extension("word/media/.隐藏文件")
              .empty());
}
