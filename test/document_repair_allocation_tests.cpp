#include "document_core_unit_test_support.hpp"
#include "allocation_failure_test_case.hpp"

#include <filesystem>

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "package repair is atomic for every pugixml clone and mutation "
    "allocation failure") {
    namespace fs = std::filesystem;

    const auto source =
        fs::current_path() / "repair_pugixml_allocation_failure.docx";
    fs::remove(source);
    write_test_archive_entries(
        source,
        {{test_document_xml_entry,
          R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"/>)"}});

    featherdoc::document_open_options open_options;
    open_options.validation = featherdoc::package_validation_mode::tolerant;

    pugi_memory_management_guard guard;
    delegated_pugi_allocate = guard.allocation;
    pugi::set_memory_management_functions(controlled_pugi_allocate,
                                          guard.deallocation);

    controlled_pugi_allocation_calls = 0U;
    controlled_pugi_failure_call = 0U;
    {
        featherdoc::Document baseline(source);
        REQUIRE_FALSE(baseline.open(open_options));
        REQUIRE_EQ(baseline.package_diagnostics().size(), 3U);

        controlled_pugi_allocation_calls = 0U;
        const auto report = baseline.repair_package();
        REQUIRE(report.has_value());
        REQUIRE(report->changed());
    }
    const auto successful_repair_allocation_count =
        controlled_pugi_allocation_calls;
    // The final allocations initialize the missing relationships and Content
    // Types work copies after the initial document clone has completed.
    REQUIRE_GT(successful_repair_allocation_count, 2U);

    for (std::size_t current_failure_call = 1U;
         current_failure_call <= successful_repair_allocation_count;
         ++current_failure_call) {
        controlled_pugi_failure_call = 0U;
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open(open_options));
        const auto diagnostics_before = document.package_diagnostics();
        REQUIRE_EQ(diagnostics_before.size(), 3U);

        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = current_failure_call;
        const auto failed_report = document.repair_package();
        controlled_pugi_failure_call = 0U;

        CAPTURE(current_failure_call);
        CAPTURE(successful_repair_allocation_count);
        CAPTURE(controlled_pugi_allocation_calls);
        CHECK_FALSE(failed_report.has_value());
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK_EQ(document.package_diagnostics().size(),
                 diagnostics_before.size());
        for (std::size_t index = 0U; index < diagnostics_before.size();
             ++index) {
            CHECK_EQ(document.package_diagnostics()[index].code,
                     diagnostics_before[index].code);
        }

        // A successful retry proves that no partial work copy was committed.
        controlled_pugi_allocation_calls = 0U;
        const auto retry_report = document.repair_package();
        REQUIRE(retry_report.has_value());
        CHECK(retry_report->changed());
        CHECK(document.package_diagnostics().empty());
    }

    controlled_pugi_failure_call = 0U;
    fs::remove(source);
}
