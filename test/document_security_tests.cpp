#include "document_core_unit_test_support.hpp"

#include <algorithm>
#include <atomic>
#include <barrier>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "zip_failure_test_support.hpp"

namespace {

constexpr auto valid_document_xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p><w:r><w:t>secure</w:t></w:r></w:p></w:body>
</w:document>
)";

auto read_file_text(const std::filesystem::path &path) -> std::string {
    std::ifstream stream(path, std::ios::binary);
    return {std::istreambuf_iterator<char>{stream},
            std::istreambuf_iterator<char>{}};
}

void write_file_text(const std::filesystem::path &path, std::string_view text) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    REQUIRE(stream.good());
    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    REQUIRE(stream.good());
}

void write_tiny_png(const std::filesystem::path &path) {
    constexpr unsigned char png[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U,
        0x00U, 0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x00U, 0x00U, 0x00U, 0x01U, 0x08U, 0x06U, 0x00U,
        0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U, 0x60U,
        0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U,
        0xA2U, 0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U,
        0xAEU, 0x42U, 0x60U, 0x82U,
    };
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    REQUIRE(stream.good());
    stream.write(reinterpret_cast<const char *>(png), sizeof(png));
    REQUIRE(stream.good());
}

auto unique_temp_files_for(const std::filesystem::path &target) -> std::size_t {
    const auto prefix = target.filename().native() +
                        std::filesystem::path{L".featherdoc-"}.native();
    std::size_t count = 0U;
    for (const auto &entry :
         std::filesystem::directory_iterator(target.parent_path())) {
        if (entry.path().filename().native().starts_with(prefix)) {
            ++count;
        }
    }
    return count;
}

} // namespace

TEST_CASE("open and save support Unicode document and image paths") {
    namespace fs = std::filesystem;

    const auto directory = fs::current_path() /
                           fs::path{u8"Unicode-中文-日本語-🙂-space dir"};
    const auto source = directory / fs::path{u8"源 文档-🙂.docx"};
    const auto image = directory / fs::path{u8"图片-日本語-🙂.png"};
    fs::remove_all(directory);
    REQUIRE(fs::create_directories(directory));

    write_test_docx(source, valid_document_xml);
    write_tiny_png(image);

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.append_image(image));
    REQUIRE(document.paragraphs().add_run(" Unicode edit").has_next());
    REQUIRE_FALSE(document.save());

    featherdoc::Document reopened(source);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(reopened.inline_images().size(), 1U);
    CHECK_NE(collect_document_text(reopened).find("Unicode edit"),
             std::string::npos);

    fs::remove_all(directory);
}

TEST_CASE("strict open rejects malformed OPC structure and tolerant open is explicit") {
    namespace fs = std::filesystem;

    const auto check_modes = [](const fs::path &path,
                                featherdoc::document_errc strict_error) {
        featherdoc::Document strict_document(path);
        CHECK_EQ(strict_document.open(), strict_error);
        CHECK_FALSE(strict_document.is_open());

        featherdoc::document_open_options options;
        options.validation = featherdoc::package_validation_mode::tolerant;
        featherdoc::Document tolerant_document(path);
        CHECK_FALSE(tolerant_document.open(options));
        CHECK(tolerant_document.is_open());
    };

    SUBCASE("missing body") {
        const auto path = fs::current_path() / "strict_missing_body.docx";
        write_test_docx(
            path,
            R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"/>)");
        check_modes(path, featherdoc::document_errc::invalid_package_structure);
        fs::remove(path);
    }

    SUBCASE("wrong WordprocessingML namespace") {
        const auto path = fs::current_path() / "strict_wrong_namespace.docx";
        write_test_docx(
            path,
            R"(<w:document xmlns:w="urn:not-wordprocessingml"><w:body/></w:document>)");
        check_modes(path, featherdoc::document_errc::invalid_package_structure);
        fs::remove(path);
    }

    SUBCASE("missing content types") {
        const auto path = fs::current_path() / "strict_missing_content_types.docx";
        write_test_archive_entries(
            path, {{test_relationships_xml_entry, test_relationships_xml},
                   {test_document_xml_entry, valid_document_xml}});
        check_modes(path, featherdoc::document_errc::content_types_xml_read_failed);
        fs::remove(path);
    }

    SUBCASE("missing root relationships") {
        const auto path = fs::current_path() / "strict_missing_root_rels.docx";
        write_test_archive_entries(
            path, {{test_content_types_xml_entry, test_content_types_xml},
                   {test_document_xml_entry, valid_document_xml}});
        check_modes(path, featherdoc::document_errc::invalid_package_structure);
        fs::remove(path);
    }

    SUBCASE("wrong main document MIME") {
        const auto path = fs::current_path() / "strict_wrong_mime.docx";
        auto wrong_content_types = std::string{test_content_types_xml};
        const auto expected = std::string{
            "application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"};
        const auto position = wrong_content_types.find(expected);
        REQUIRE_NE(position, std::string::npos);
        wrong_content_types.replace(position, expected.size(), "application/xml");
        write_test_archive_entries(
            path, {{test_content_types_xml_entry, wrong_content_types},
                   {test_relationships_xml_entry, test_relationships_xml},
                   {test_document_xml_entry, valid_document_xml}});
        check_modes(path, featherdoc::document_errc::invalid_package_structure);
        fs::remove(path);
    }

    SUBCASE("malformed content types XML") {
        const auto path = fs::current_path() / "strict_bad_content_types.docx";
        write_test_archive_entries(
            path, {{test_content_types_xml_entry, "<Types>"},
                   {test_relationships_xml_entry, test_relationships_xml},
                   {test_document_xml_entry, valid_document_xml}});
        check_modes(path, featherdoc::document_errc::content_types_xml_parse_failed);
        fs::remove(path);
    }
}

TEST_CASE("tolerant open reports repairable and unsafe package diagnostics") {
    namespace fs = std::filesystem;

    SUBCASE("missing deterministic OPC parts are repairable") {
        const auto path = fs::current_path() / "tolerant_repairable_diagnostics.docx";
        write_test_archive_entries(
            path,
            {{test_document_xml_entry,
              R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"/>)"}});

        featherdoc::document_open_options options;
        options.validation = featherdoc::package_validation_mode::tolerant;
        featherdoc::Document document(path);
        REQUIRE_FALSE(document.open(options));

        const auto &diagnostics = document.package_diagnostics();
        REQUIRE_EQ(diagnostics.size(), 3U);
        CHECK(std::ranges::all_of(diagnostics, [](const auto &diagnostic) {
            return diagnostic.repairable;
        }));
        CHECK(std::ranges::any_of(diagnostics, [](const auto &diagnostic) {
            return diagnostic.code ==
                   featherdoc::package_diagnostic_code::missing_document_body;
        }));
        CHECK(std::ranges::any_of(diagnostics, [](const auto &diagnostic) {
            return diagnostic.code ==
                   featherdoc::package_diagnostic_code::missing_root_relationships;
        }));
        CHECK(std::ranges::any_of(diagnostics, [](const auto &diagnostic) {
            return diagnostic.code ==
                   featherdoc::package_diagnostic_code::missing_content_types;
        }));
        fs::remove(path);
    }

    SUBCASE("invalid namespace and malformed metadata are unsafe") {
        const auto path = fs::current_path() / "tolerant_unsafe_diagnostics.docx";
        write_test_archive_entries(
            path,
            {{test_content_types_xml_entry, "<Types>"},
             {test_relationships_xml_entry, "<Relationships>"},
             {test_document_xml_entry,
              R"(<w:document xmlns:w="urn:not-wordprocessingml"><w:body/></w:document>)"}});

        featherdoc::document_open_options options;
        options.validation = featherdoc::package_validation_mode::tolerant;
        featherdoc::Document document(path);
        REQUIRE_FALSE(document.open(options));

        const auto &diagnostics = document.package_diagnostics();
        REQUIRE_EQ(diagnostics.size(), 3U);
        CHECK(std::ranges::none_of(diagnostics, [](const auto &diagnostic) {
            return diagnostic.repairable;
        }));
        fs::remove(path);
    }
}

TEST_CASE("explicit package repair produces a strict-valid package without data loss") {
    namespace fs = std::filesystem;
    const auto source = fs::current_path() / "repair_source.docx";
    const auto repaired = fs::current_path() / "repair_output.docx";
    constexpr auto custom_entry = "customXml/item1.xml";
    constexpr auto custom_payload = "<custom>preserve-me</custom>";
    write_test_archive_entries(
        source,
        {{test_document_xml_entry,
          R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"/>)"},
         {custom_entry, custom_payload}});

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open(options));

    const auto report = document.repair_package();
    REQUIRE(report.has_value());
    CHECK_EQ(report->diagnostics_before.size(), 3U);
    CHECK_EQ(report->actions.size(), 3U);
    CHECK(report->changed());
    CHECK(document.package_diagnostics().empty());

    const auto second_report = document.repair_package();
    REQUIRE(second_report.has_value());
    CHECK_FALSE(second_report->changed());

    REQUIRE(document.paragraphs().add_run("repaired").has_next());
    REQUIRE_FALSE(document.save_as(repaired));

    featherdoc::Document strict_document(repaired);
    REQUIRE_FALSE(strict_document.open());
    CHECK_NE(collect_document_text(strict_document).find("repaired"),
             std::string::npos);
    CHECK_EQ(read_test_docx_entry(repaired, custom_entry), custom_payload);

    fs::remove(source);
    fs::remove(repaired);
}

TEST_CASE("package repair preserves valid OPC metadata while correcting main entries") {
    namespace fs = std::filesystem;
    const auto source = fs::current_path() / "repair_metadata_source.docx";
    const auto repaired = fs::current_path() / "repair_metadata_output.docx";
    const auto content_types = std::string{R"(<?xml version="1.0" encoding="UTF-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/xml"/>
  <Override PartName="/customXml/item1.xml" ContentType="application/vnd.example.custom+xml"/>
</Types>)"};
    const auto relationships = std::string{R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/wrong.xml"/>
  <Relationship Id="rId9" Type="urn:example:custom" Target="customXml/item1.xml"/>
</Relationships>)"};
    write_test_archive_entries(
        source, {{test_content_types_xml_entry, content_types},
                 {test_relationships_xml_entry, relationships},
                 {test_document_xml_entry, valid_document_xml},
                 {"customXml/item1.xml", "<custom/>"}});

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open(options));
    const auto paragraph_before_repair = document.paragraphs();
    REQUIRE(paragraph_before_repair.valid());
    const auto report = document.repair_package();
    REQUIRE(report.has_value());
    CHECK_EQ(report->actions.size(), 2U);
    CHECK_FALSE(paragraph_before_repair.valid());
    CHECK_FALSE(paragraph_before_repair.set_text("stale repair handle"));
    CHECK(document.paragraphs().valid());
    REQUIRE_FALSE(document.save_as(repaired));

    featherdoc::Document strict_document(repaired);
    REQUIRE_FALSE(strict_document.open());
    const auto repaired_content_types =
        read_test_docx_entry(repaired, test_content_types_xml_entry);
    const auto repaired_relationships =
        read_test_docx_entry(repaired, test_relationships_xml_entry);
    CHECK_NE(repaired_content_types.find("application/vnd.example.custom+xml"),
             std::string::npos);
    CHECK_NE(repaired_relationships.find("urn:example:custom"), std::string::npos);

    fs::remove(source);
    fs::remove(repaired);
}

TEST_CASE("unsafe package repair is rejected atomically") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "repair_unsafe_atomic.docx";
    write_test_archive_entries(
        path,
        {{test_content_types_xml_entry, "<Types>"},
         {test_relationships_xml_entry, test_relationships_xml},
         {test_document_xml_entry,
          R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"/>)"}});

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open(options));
    const auto diagnostics_before = document.package_diagnostics();

    const auto report = document.repair_package();
    CHECK_FALSE(report.has_value());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::package_repair_not_possible);
    CHECK_EQ(document.package_diagnostics().size(), diagnostics_before.size());
    CHECK_FALSE(document.paragraphs().add_run("must-not-be-added").has_next());

    fs::remove(path);
}

TEST_CASE("package repair options reject disabled changes atomically") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "repair_options_atomic.docx";
    write_test_archive_entries(
        path, {{test_document_xml_entry, valid_document_xml}});

    featherdoc::document_open_options open_options;
    open_options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open(open_options));
    REQUIRE_EQ(document.package_diagnostics().size(), 2U);

    featherdoc::document_repair_options repair_options;
    repair_options.repair_root_relationships = false;
    const auto report = document.repair_package(repair_options);
    CHECK_FALSE(report.has_value());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::package_repair_not_possible);
    CHECK_EQ(document.package_diagnostics().size(), 2U);

    fs::remove(path);
}

TEST_CASE("duplicate main OPC declarations are diagnosed as unsafe") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "repair_ambiguous_main_parts.docx";
    const auto duplicate_relationships = std::string{R"(
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/other.xml"/>
</Relationships>)"};
    const auto duplicate_content_types = std::string{R"(
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/document.xml" ContentType="application/xml"/>
</Types>)"};
    write_test_archive_entries(
        path, {{test_content_types_xml_entry, duplicate_content_types},
               {test_relationships_xml_entry, duplicate_relationships},
               {test_document_xml_entry, valid_document_xml}});

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open(options));
    REQUIRE_EQ(document.package_diagnostics().size(), 2U);
    CHECK(std::ranges::none_of(document.package_diagnostics(),
                               [](const auto &diagnostic) {
                                   return diagnostic.repairable;
                               }));
    CHECK_FALSE(document.repair_package().has_value());

    fs::remove(path);
}

TEST_CASE("strict open accepts a normalized absolute root relationship target") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "strict_normalized_root_target.docx";
    const auto relationships = std::string{R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="/word/./document.xml"/>
</Relationships>)"};
    write_test_archive_entries(
        path, {{test_content_types_xml_entry, test_content_types_xml},
               {test_relationships_xml_entry, relationships},
               {test_document_xml_entry, valid_document_xml}});

    featherdoc::Document document(path);
    CHECK_FALSE(document.open());
    CHECK(document.is_open());
    fs::remove(path);
}

TEST_CASE("archive metadata limits reject oversized DOCX inputs before extraction") {
    namespace fs = std::filesystem;
    const auto path = fs::current_path() / "archive_limits.docx";
    write_test_archive_entries(
        path, {{test_content_types_xml_entry, test_content_types_xml},
               {test_relationships_xml_entry, test_relationships_xml},
               {test_document_xml_entry, valid_document_xml},
               {"word/media/payload.bin", "12345678"}});

    const auto check_limit = [&](const featherdoc::archive_limits &limits,
                                 std::string_view expected_entry) {
        featherdoc::document_open_options options;
        options.limits = limits;
        featherdoc::Document document(path);
        CHECK_EQ(document.open(options),
                 featherdoc::document_errc::archive_limit_exceeded);
        if (!expected_entry.empty()) {
            CHECK_EQ(document.last_error().entry_name, expected_entry);
        }
        CHECK_FALSE(document.is_open());
    };

    auto limits = featherdoc::archive_limits{};
    limits.max_entries = 3U;
    check_limit(limits, {});

    limits = {};
    limits.max_xml_part_bytes = 1U;
    check_limit(limits, test_content_types_xml_entry);

    limits = {};
    limits.max_binary_part_bytes = 4U;
    check_limit(limits, "word/media/payload.bin");

    limits = {};
    limits.max_total_uncompressed_bytes = 1U;
    check_limit(limits, test_content_types_xml_entry);

    limits = {};
    limits.max_compression_ratio = 0U;
    check_limit(limits, test_content_types_xml_entry);

    fs::remove(path);
}

TEST_CASE("save uses unique temporary files and preserves fixed sibling files") {
    namespace fs = std::filesystem;
    const auto target = fs::current_path() / "save_transaction.docx";
    auto fixed_tmp = target;
    fixed_tmp += ".tmp";
    auto fixed_backup = target;
    fixed_backup += ".bak";
    fs::remove(target);
    write_file_text(fixed_tmp, "user tmp");
    write_file_text(fixed_backup, "user backup");

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.create_empty());
    REQUIRE(document.paragraphs().add_run("transactional save").has_next());
    REQUIRE_FALSE(document.save());

    CHECK_EQ(read_file_text(fixed_tmp), "user tmp");
    CHECK_EQ(read_file_text(fixed_backup), "user backup");
    CHECK_EQ(unique_temp_files_for(target), 0U);

    fs::remove(target);
    fs::remove(fixed_tmp);
    fs::remove(fixed_backup);
}

TEST_CASE("temporary output creation failure is reported before ZIP writing") {
    namespace fs = std::filesystem;
    const auto missing_parent =
        fs::current_path() / "missing_save_transaction_parent";
    const auto target = missing_parent / "output.docx";
    fs::remove_all(missing_parent);

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    const auto save_error = document.save_as(target);
    CHECK_EQ(save_error, featherdoc::document_errc::output_archive_open_failed);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::output_archive_open_failed);
    CHECK_FALSE(fs::exists(missing_parent));
}

TEST_CASE("ZIP entry write failure preserves the original target and allows retry") {
    namespace fs = std::filesystem;
    const auto target = fs::current_path() / "save_write_failure.docx";
    fs::remove(target);
    write_file_text(target, "original bytes");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    REQUIRE(document.paragraphs().add_run("retry after write failure").has_next());

    featherdoc_test::zip_fail_next_write();
    const auto save_error = document.save_as(target);
    CHECK_EQ(save_error,
             featherdoc::document_errc::output_document_xml_write_failed);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::output_document_xml_write_failed);
    CHECK_EQ(read_file_text(target), "original bytes");
    CHECK_EQ(unique_temp_files_for(target), 0U);

    REQUIRE_FALSE(document.save_as(target));
    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "retry after write failure\n");
    CHECK_EQ(unique_temp_files_for(target), 0U);
    fs::remove(target);
}

TEST_CASE("ZIP close failures preserve the original target and allow retry") {
    namespace fs = std::filesystem;

    const auto check_failure = [](int failure_stage,
                                  std::string_view stage_name) {
        auto target = fs::current_path() / "save_close_failure.docx";
        target += "." + std::string{stage_name};
        fs::remove(target);
        write_file_text(target, "original bytes");

        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());
        REQUIRE(document.paragraphs().add_run("retry succeeds").has_next());

        featherdoc_test::zip_fail_next_close_stage(failure_stage);
        const auto save_error = document.save_as(target);
        CHECK_EQ(save_error,
                 featherdoc::document_errc::output_archive_finalize_failed);
        CHECK_EQ(document.last_error().code,
                 featherdoc::document_errc::output_archive_finalize_failed);
        CHECK_EQ(read_file_text(target), "original bytes");
        CHECK_EQ(unique_temp_files_for(target), 0U);

        REQUIRE_FALSE(document.save_as(target));
        featherdoc::Document reopened(target);
        REQUIRE_FALSE(reopened.open());
        CHECK_EQ(collect_document_text(reopened), "retry succeeds\n");
        CHECK_EQ(unique_temp_files_for(target), 0U);
        fs::remove(target);
    };

    SUBCASE("archive finalize") {
        check_failure(featherdoc_test::zip_close_failure_finalize, "finalize");
    }
    SUBCASE("archive truncate") {
        check_failure(featherdoc_test::zip_close_failure_truncate, "truncate");
    }
    SUBCASE("writer end") {
        check_failure(featherdoc_test::zip_close_failure_writer_end,
                      "writer-end");
    }
}

TEST_CASE("concurrent saves use independent temporary archives") {
    namespace fs = std::filesystem;
    const auto target = fs::current_path() / "concurrent_save.docx";
    fs::remove(target);

    featherdoc::Document first;
    featherdoc::Document second;
    REQUIRE_FALSE(first.create_empty());
    REQUIRE_FALSE(second.create_empty());
    REQUIRE(first.paragraphs().add_run("first writer").has_next());
    REQUIRE(second.paragraphs().add_run("second writer").has_next());

    std::barrier start{3};
    std::error_code first_error;
    std::error_code second_error;
    std::thread first_thread([&] {
        start.arrive_and_wait();
        first_error = first.save_as(target);
    });
    std::thread second_thread([&] {
        start.arrive_and_wait();
        second_error = second.save_as(target);
    });
    start.arrive_and_wait();
    first_thread.join();
    second_thread.join();

    CHECK_FALSE(first_error);
    CHECK_FALSE(second_error);
    CHECK_EQ(unique_temp_files_for(target), 0U);
    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    const auto text = collect_document_text(reopened);
    CHECK((text.find("first writer") != std::string::npos ||
           text.find("second writer") != std::string::npos));

    fs::remove(target);
}

#if defined(_WIN32)
TEST_CASE("failed Windows replacement preserves the original target") {
    namespace fs = std::filesystem;
    const auto target = fs::current_path() / "replace_failure.docx";
    write_file_text(target, "original bytes");

    const auto locked_file = CreateFileW(
        target.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(locked_file != INVALID_HANDLE_VALUE);

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    const auto save_error = document.save_as(target);
    CHECK_EQ(save_error, featherdoc::document_errc::output_replace_failed);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::output_replace_failed);
    CHECK(CloseHandle(locked_file) != 0);

    CHECK_EQ(read_file_text(target), "original bytes");
    CHECK_EQ(unique_temp_files_for(target), 0U);
    fs::remove(target);
}
#endif

TEST_CASE("reloading a document requires reacquiring XML-backed handles") {
    namespace fs = std::filesystem;
    const auto first_path = fs::current_path() / "handle_generation_first.docx";
    const auto second_path = fs::current_path() / "handle_generation_second.docx";
    const auto output_path = fs::current_path() / "handle_generation_output.docx";
    write_test_docx(first_path, valid_document_xml);
    write_test_docx(
        second_path,
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:t>second</w:t></w:r></w:p></w:body></w:document>)");

    featherdoc::Document document(first_path);
    REQUIRE_FALSE(document.open());
    auto old_paragraph = document.paragraphs();
    REQUIRE(old_paragraph.has_next());
    auto old_run = old_paragraph.runs();
    REQUIRE(old_run.has_next());
    auto old_template_part = document.body_template();
    REQUIRE(static_cast<bool>(old_template_part));

    document.set_path(second_path);
    CHECK_FALSE(old_paragraph.valid());
    CHECK_FALSE(old_paragraph.has_next());
    CHECK_FALSE(old_paragraph.set_text("stale paragraph"));
    CHECK_FALSE(old_run.valid());
    CHECK_FALSE(old_run.has_next());
    CHECK_FALSE(old_run.set_text("stale run"));
    CHECK_FALSE(static_cast<bool>(old_template_part));
    CHECK_FALSE(old_template_part.append_paragraph("stale template").valid());
    REQUIRE_FALSE(document.open());
    auto current_paragraph = document.paragraphs();
    REQUIRE(current_paragraph.has_next());
    REQUIRE(current_paragraph.add_run(" current handle").has_next());
    REQUIRE_FALSE(document.save_as(output_path));

    featherdoc::Document reopened(output_path);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "second current handle\n");

    fs::remove(first_path);
    fs::remove(second_path);
    fs::remove(output_path);
}

TEST_CASE("all XML-backed handles become inert after Document destruction") {
    featherdoc::Paragraph stale_paragraph;
    featherdoc::Run stale_run;
    featherdoc::Table stale_table;
    featherdoc::TableRow stale_row;
    featherdoc::TableCell stale_cell;
    featherdoc::TemplatePart stale_template_part;

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.valid());
        stale_paragraph = paragraph;
        stale_run = paragraph.add_run("owned text");
        REQUIRE(stale_run.valid());

        stale_table = document.append_table(1U, 1U);
        REQUIRE(stale_table.valid());
        stale_row = stale_table.rows();
        REQUIRE(stale_row.valid());
        stale_cell = stale_row.cells();
        REQUIRE(stale_cell.valid());

        stale_template_part = document.body_template();
        REQUIRE(static_cast<bool>(stale_template_part));
    }

    CHECK_FALSE(stale_paragraph.valid());
    CHECK_FALSE(stale_paragraph.set_text("after destruction"));
    CHECK_FALSE(stale_run.valid());
    CHECK_FALSE(stale_run.set_text("after destruction"));
    CHECK_FALSE(stale_table.valid());
    CHECK_FALSE(stale_table.set_width_twips(1000U));
    CHECK_FALSE(stale_row.valid());
    CHECK_FALSE(stale_row.set_cant_split());
    CHECK_FALSE(stale_cell.valid());
    CHECK_FALSE(stale_cell.set_text("after destruction"));
    CHECK_FALSE(static_cast<bool>(stale_template_part));
    CHECK_FALSE(stale_template_part.append_paragraph("after destruction").valid());
    CHECK_FALSE(stale_template_part.append_table(1U, 1U).valid());
}

TEST_CASE("removed XML subtrees invalidate affected handle copies") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    auto removed_run = paragraph.add_run("old run");
    REQUIRE(removed_run.valid());
    REQUIRE(paragraph.set_text("replacement"));
    CHECK_FALSE(removed_run.valid());
    CHECK_FALSE(removed_run.set_text("stale"));
    CHECK(paragraph.valid());
    CHECK_EQ(paragraph.runs().get_text(), "replacement");

    auto removed_paragraph_copy = paragraph;
    REQUIRE(paragraph.insert_paragraph_after("next paragraph").valid());
    REQUIRE(paragraph.remove());
    CHECK_FALSE(removed_paragraph_copy.valid());
    CHECK(paragraph.valid());
    CHECK_EQ(paragraph.runs().get_text(), "next paragraph");

    auto table = document.append_table(1U, 2U);
    REQUIRE(table.valid());
    auto first_cell = table.rows().cells();
    auto removed_cell = first_cell;
    removed_cell.next();
    REQUIRE(removed_cell.valid());
    REQUIRE(first_cell.merge_right());
    CHECK_FALSE(removed_cell.valid());
    CHECK_FALSE(removed_cell.set_text("stale cell"));
    CHECK(first_cell.valid());
    CHECK(table.valid());

    REQUIRE(table.insert_table_after(1U, 1U).valid());
    auto removed_table_copy = table;
    REQUIRE(table.remove());
    CHECK_FALSE(removed_table_copy.valid());
    CHECK(table.valid());
}
