#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "allocation_failure_test_case.hpp"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"
#include "doctest.h"

#include <featherdoc.hpp>

namespace {

pugi::allocation_function review_delegated_allocate = nullptr;
std::size_t review_allocation_calls = 0U;
std::size_t review_failure_call = 0U;

auto controlled_review_allocate(std::size_t size) -> void * {
    ++review_allocation_calls;
    if (review_failure_call != 0U &&
        review_allocation_calls == review_failure_call) {
        return nullptr;
    }
    return review_delegated_allocate(size);
}

class review_pugi_allocator_guard final {
  public:
    review_pugi_allocator_guard()
        : previous_allocate_(pugi::get_memory_allocation_function()),
          previous_deallocate_(pugi::get_memory_deallocation_function()) {
        review_delegated_allocate = this->previous_allocate_;
        review_allocation_calls = 0U;
        review_failure_call = 0U;
        pugi::set_memory_management_functions(controlled_review_allocate,
                                              this->previous_deallocate_);
    }

    review_pugi_allocator_guard(const review_pugi_allocator_guard &) = delete;
    auto operator=(const review_pugi_allocator_guard &)
        -> review_pugi_allocator_guard & = delete;

    ~review_pugi_allocator_guard() {
        pugi::set_memory_management_functions(this->previous_allocate_,
                                              this->previous_deallocate_);
        review_delegated_allocate = nullptr;
        review_allocation_calls = 0U;
        review_failure_call = 0U;
    }

  private:
    pugi::allocation_function previous_allocate_;
    pugi::deallocation_function previous_deallocate_;
};

auto sorted_review_archive_entries(const std::filesystem::path &path)
    -> std::vector<std::pair<std::string, std::string>> {
    auto entries = read_test_archive_entries(path);
    std::sort(entries.begin(), entries.end(),
              [](const auto &left, const auto &right) {
                  return left.first < right.first;
              });
    return entries;
}

void check_review_archives_equal(
    const std::vector<std::pair<std::string, std::string>> &expected,
    const std::filesystem::path &actual_path) {
    const auto actual = sorted_review_archive_entries(actual_path);
    REQUIRE_EQ(actual.size(), expected.size());
    for (std::size_t index = 0U; index < expected.size(); ++index) {
        CAPTURE(index);
        CHECK_EQ(actual[index].first, expected[index].first);
        CHECK_EQ(actual[index].second, expected[index].second);
    }
}

template <typename Operation>
void sweep_review_operation_allocation_failures(
    const std::filesystem::path &source, std::string_view operation_name,
    Operation operation) {
    namespace fs = std::filesystem;

    std::size_t successful_allocation_calls = 0U;
    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        review_pugi_allocator_guard guard;
        REQUIRE(operation(document));
        successful_allocation_calls = review_allocation_calls;
    }
    REQUIRE_GT(successful_allocation_calls, 0U);

    const auto expected_entries = sorted_review_archive_entries(source);
    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_calls; ++failure_call) {
        const auto after = fs::current_path() /
                           ("review_oom_" + std::string{operation_name} + "_" +
                            std::to_string(failure_call) + ".docx");
        fs::remove(after);
        {
            featherdoc::Document document(source);
            REQUIRE_FALSE(document.open());
            auto retained_paragraph = document.paragraphs();
            REQUIRE(retained_paragraph.has_next());
            auto retained_run = retained_paragraph.runs();
            REQUIRE(retained_run.has_next());
            const auto retained_text = retained_run.get_text();

            bool result = false;
            {
                review_pugi_allocator_guard guard;
                review_failure_call = failure_call;
                result = operation(document);
                CAPTURE(operation_name);
                CAPTURE(failure_call);
                CAPTURE(successful_allocation_calls);
                CAPTURE(review_allocation_calls);
                CHECK_FALSE(result);
            }

            CHECK_EQ(document.last_error().code,
                     std::make_error_code(std::errc::not_enough_memory));
            CHECK(retained_paragraph.valid());
            CHECK(retained_run.valid());
            CHECK_EQ(retained_run.get_text(), retained_text);
            REQUIRE_FALSE(document.save_as(after));
        }
        check_review_archives_equal(expected_entries, after);
        fs::remove(after);
    }
}

template <typename Operation, typename Verify>
void check_review_operation_retries_after_part_load_failure(
    const std::filesystem::path &source, std::string_view operation_name,
    Operation operation, Verify verify) {
    namespace fs = std::filesystem;

    const auto after = fs::current_path() /
                       ("review_retry_" + std::string{operation_name} +
                        ".docx");
    fs::remove(after);

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());

    {
        review_pugi_allocator_guard guard;
        review_failure_call = 1U;
        CAPTURE(operation_name);
        CHECK_FALSE(operation(document));
    }

    CHECK_EQ(document.last_error().code,
             std::make_error_code(std::errc::not_enough_memory));

    {
        review_pugi_allocator_guard guard;
        CAPTURE(operation_name);
        REQUIRE(operation(document));
    }
    CHECK_FALSE(document.last_error());

    REQUIRE_FALSE(document.save_as(after));
    verify(after);
    fs::remove(after);
}

} // namespace

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "first review-part attachment rolls back every allocation failure") {
    namespace fs = std::filesystem;

    const auto source = fs::current_path() / "review_first_part_oom.docx";
    fs::remove(source);
    {
        featherdoc::Document seed(source);
        REQUIRE_FALSE(seed.create_empty());
        REQUIRE(seed.paragraphs().set_text("stable anchor"));
        REQUIRE_FALSE(seed.save());
    }

    sweep_review_operation_allocation_failures(
        source, "append_comment", [](auto &document) {
            return document.append_comment("comment anchor", "comment body",
                                           "Reviewer", "RV") != 0U;
        });
    sweep_review_operation_allocation_failures(
        source, "append_footnote", [](auto &document) {
            return document.append_footnote("footnote anchor", "footnote body") !=
                   0U;
        });
    sweep_review_operation_allocation_failures(
        source, "append_endnote", [](auto &document) {
            return document.append_endnote("endnote anchor", "endnote body") !=
                   0U;
        });
    sweep_review_operation_allocation_failures(
        source, "append_text_range_comment", [](auto &document) {
            return document.append_text_range_comment(
                       0U, 0U, 0U, 6U, "range body", "Reviewer", "RV") !=
                   0U;
        });

    fs::remove(source);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "existing review part load allocation failures can be retried") {
    namespace fs = std::filesystem;

    const auto source = fs::current_path() / "review_part_load_retry_oom.docx";
    fs::remove(source);
    {
        featherdoc::Document seed(source);
        REQUIRE_FALSE(seed.create_empty());
        REQUIRE(seed.paragraphs().set_text("stable anchor"));
        REQUIRE_EQ(seed.append_comment("comment anchor", "original comment",
                                       "Reviewer", "RV"),
                   1U);
        REQUIRE_EQ(seed.append_footnote("footnote anchor", "original footnote"),
                   1U);
        REQUIRE_EQ(seed.append_endnote("endnote anchor", "original endnote"),
                   1U);
        REQUIRE_FALSE(seed.save());
    }

    check_review_operation_retries_after_part_load_failure(
        source, "replace_footnote_retry",
        [](auto &document) {
            return document.replace_footnote(0U, "retry footnote");
        },
        [](const auto &path) {
            const auto footnotes_xml =
                read_test_docx_entry(path, "word/footnotes.xml");
            CHECK_NE(footnotes_xml.find("retry footnote"), std::string::npos);
        });

    check_review_operation_retries_after_part_load_failure(
        source, "replace_endnote_retry",
        [](auto &document) {
            return document.replace_endnote(0U, "retry endnote");
        },
        [](const auto &path) {
            const auto endnotes_xml =
                read_test_docx_entry(path, "word/endnotes.xml");
            CHECK_NE(endnotes_xml.find("retry endnote"), std::string::npos);
        });

    check_review_operation_retries_after_part_load_failure(
        source, "replace_comment_retry",
        [](auto &document) {
            return document.replace_comment(0U, "retry comment");
        },
        [](const auto &path) {
            const auto comments_xml =
                read_test_docx_entry(path, "word/comments.xml");
            CHECK_NE(comments_xml.find("retry comment"), std::string::npos);
        });

    check_review_operation_retries_after_part_load_failure(
        source, "append_revision_retry",
        [](auto &document) {
            return document.append_insertion_revision(
                       "retry revision", "Reviewer",
                       "2026-07-17T12:00:00Z") == 1U;
        },
        [](const auto &path) {
            const auto document_xml =
                read_test_docx_entry(path, test_document_xml_entry);
            CHECK_NE(document_xml.find("retry revision"), std::string::npos);
            CHECK_NE(document_xml.find("<w:ins "), std::string::npos);
        });

    fs::remove(source);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "existing comments and notes publish atomically across allocation failures") {
    namespace fs = std::filesystem;

    const auto source = fs::current_path() / "review_existing_part_oom.docx";
    fs::remove(source);
    {
        featherdoc::Document seed(source);
        REQUIRE_FALSE(seed.create_empty());
        REQUIRE(seed.paragraphs().set_text("stable anchor"));
        REQUIRE_EQ(seed.append_comment("comment anchor", "original comment",
                                       "Reviewer", "RV"),
                   1U);
        REQUIRE_EQ(seed.append_footnote("footnote anchor", "original footnote"),
                   1U);
        REQUIRE_EQ(seed.append_endnote("endnote anchor", "original endnote"),
                   1U);
        REQUIRE_FALSE(seed.save());
    }

    sweep_review_operation_allocation_failures(
        source, "replace_comment", [](auto &document) {
            return document.replace_comment(0U, "replacement comment");
        });
    sweep_review_operation_allocation_failures(
        source, "comment_metadata", [](auto &document) {
            featherdoc::comment_metadata_update metadata;
            metadata.author = "Updated Reviewer";
            metadata.clear_initials = true;
            metadata.date = "2026-07-17T12:00:00Z";
            return document.set_comment_metadata(0U, metadata);
        });
    sweep_review_operation_allocation_failures(
        source, "comment_resolved", [](auto &document) {
            return document.set_comment_resolved(0U, true);
        });
    sweep_review_operation_allocation_failures(
        source, "comment_reply", [](auto &document) {
            return document.append_comment_reply(0U, "reply body", "Responder",
                                                 "RS") != 0U;
        });
    sweep_review_operation_allocation_failures(
        source, "move_comment_range", [](auto &document) {
            return document.set_text_range_comment_range(0U, 0U, 0U, 0U, 6U);
        });
    sweep_review_operation_allocation_failures(
        source, "remove_comment", [](auto &document) {
            return document.remove_comment(0U);
        });
    sweep_review_operation_allocation_failures(
        source, "replace_footnote", [](auto &document) {
            return document.replace_footnote(0U, "replacement footnote");
        });
    sweep_review_operation_allocation_failures(
        source, "remove_footnote", [](auto &document) {
            return document.remove_footnote(0U);
        });
    sweep_review_operation_allocation_failures(
        source, "replace_endnote", [](auto &document) {
            return document.replace_endnote(0U, "replacement endnote");
        });
    sweep_review_operation_allocation_failures(
        source, "remove_endnote", [](auto &document) {
            return document.remove_endnote(0U);
        });

    fs::remove(source);
}
