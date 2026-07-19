#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <new>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "basic_document_xml_test_support.hpp"
#include "basic_docx_archive_test_support.hpp"
#include "basic_image_fixture_test_support.hpp"
#include "doctest.h"
#include "allocation_failure_test_case.hpp"

#include <featherdoc.hpp>

namespace {

pugi::allocation_function custom_xml_delegated_allocate = nullptr;
std::size_t custom_xml_allocation_calls = 0U;
std::size_t custom_xml_failure_call = 0U;

std::atomic_bool global_allocation_tracking_enabled{false};
std::atomic_size_t global_allocation_calls{0U};
std::atomic_size_t global_allocation_failure_call{0U};

auto controlled_custom_xml_allocate(std::size_t size) -> void * {
    ++custom_xml_allocation_calls;
    if (custom_xml_failure_call != 0U &&
        custom_xml_allocation_calls == custom_xml_failure_call) {
        return nullptr;
    }
    return custom_xml_delegated_allocate(size);
}

class custom_xml_pugi_allocator_guard final {
  public:
    custom_xml_pugi_allocator_guard()
        : previous_allocate_(pugi::get_memory_allocation_function()),
          previous_deallocate_(pugi::get_memory_deallocation_function()) {
        custom_xml_delegated_allocate = this->previous_allocate_;
        custom_xml_allocation_calls = 0U;
        custom_xml_failure_call = 0U;
        pugi::set_memory_management_functions(controlled_custom_xml_allocate,
                                              this->previous_deallocate_);
    }

    custom_xml_pugi_allocator_guard(const custom_xml_pugi_allocator_guard &) =
        delete;
    auto operator=(const custom_xml_pugi_allocator_guard &)
        -> custom_xml_pugi_allocator_guard & = delete;

    ~custom_xml_pugi_allocator_guard() {
        pugi::set_memory_management_functions(this->previous_allocate_,
                                              this->previous_deallocate_);
        custom_xml_delegated_allocate = nullptr;
        custom_xml_allocation_calls = 0U;
        custom_xml_failure_call = 0U;
    }

  private:
    pugi::allocation_function previous_allocate_;
    pugi::deallocation_function previous_deallocate_;
};

void record_global_allocation() {
    if (!global_allocation_tracking_enabled.load(std::memory_order_relaxed)) {
        return;
    }

    const auto allocation_call =
        global_allocation_calls.fetch_add(1U, std::memory_order_relaxed) + 1U;
    if (allocation_call ==
        global_allocation_failure_call.load(std::memory_order_relaxed)) {
        throw std::bad_alloc{};
    }
}

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY
[[nodiscard]] void *allocate_controlled(std::size_t size) {
    record_global_allocation();
    if (void *memory = std::malloc(size == 0U ? 1U : size)) {
        return memory;
    }
    throw std::bad_alloc{};
}

[[nodiscard]] void *allocate_controlled_aligned(std::size_t size,
                                                std::size_t alignment) {
    record_global_allocation();
    void *memory = nullptr;
    if (posix_memalign(&memory, alignment, size == 0U ? 1U : size) == 0) {
        return memory;
    }
    throw std::bad_alloc{};
}
#endif

class global_allocation_window final {
  public:
    explicit global_allocation_window(std::size_t failure_call) noexcept {
        global_allocation_calls.store(0U, std::memory_order_relaxed);
        global_allocation_failure_call.store(failure_call,
                                             std::memory_order_relaxed);
        global_allocation_tracking_enabled.store(true,
                                                  std::memory_order_relaxed);
    }

    global_allocation_window(const global_allocation_window &) = delete;
    auto operator=(const global_allocation_window &)
        -> global_allocation_window & = delete;

    ~global_allocation_window() {
        global_allocation_tracking_enabled.store(false,
                                                  std::memory_order_relaxed);
        global_allocation_failure_call.store(0U, std::memory_order_relaxed);
    }
};

auto wrap_in_deep_xml(std::string leaf, std::size_t depth) -> std::string {
    std::string xml;
    xml.reserve(leaf.size() + depth * 7U);
    for (std::size_t index = 0U; index < depth; ++index) {
        xml += "<n>";
    }
    xml += leaf;
    for (std::size_t index = 0U; index < depth; ++index) {
        xml += "</n>";
    }
    return xml;
}

} // namespace

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY
void *operator new(std::size_t size) { return allocate_controlled(size); }
void *operator new[](std::size_t size) { return allocate_controlled(size); }
void *operator new(std::size_t size, std::align_val_t alignment) {
    return allocate_controlled_aligned(size,
                                       static_cast<std::size_t>(alignment));
}
void *operator new[](std::size_t size, std::align_val_t alignment) {
    return allocate_controlled_aligned(size,
                                       static_cast<std::size_t>(alignment));
}
void operator delete(void *memory) noexcept { std::free(memory); }
void operator delete[](void *memory) noexcept { std::free(memory); }
void operator delete(void *memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void *memory, std::size_t) noexcept {
    std::free(memory);
}
void operator delete(void *memory, std::align_val_t) noexcept {
    std::free(memory);
}
void operator delete[](void *memory, std::align_val_t) noexcept {
    std::free(memory);
}
void operator delete(void *memory, std::size_t,
                     std::align_val_t) noexcept {
    std::free(memory);
}
void operator delete[](void *memory, std::size_t,
                       std::align_val_t) noexcept {
    std::free(memory);
}
#endif

TEST_CASE("content controls can be listed and filtered by tag or alias") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "content_controls_inspect.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Customer Name"/>
        <w:tag w:val="customer_name"/>
        <w:id w:val="42"/>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>Ada Lovelace</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
    <w:p>
      <w:r><w:t>Order: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Order Number"/>
          <w:tag w:val="order_no"/>
          <w:id w:val="43"/>
          <w:showingPlcHdr/>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>INV-001</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:tbl>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Line Items"/>
          <w:tag w:val="line_items"/>
          <w:id w:val="44"/>
        </w:sdtPr>
        <w:sdtContent>
          <w:tr>
            <w:tc><w:p><w:r><w:t>SKU-1</w:t></w:r></w:p></w:tc>
          </w:tr>
        </w:sdtContent>
      </w:sdt>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto content_controls = doc.list_content_controls();
    CHECK_FALSE(doc.last_error());
    REQUIRE(content_controls.size() == 3U);
    CHECK_EQ(content_controls[0].index, 0U);
    CHECK_EQ(content_controls[0].kind, featherdoc::content_control_kind::block);
    REQUIRE(content_controls[0].tag.has_value());
    CHECK_EQ(*content_controls[0].tag, "customer_name");
    REQUIRE(content_controls[0].alias.has_value());
    CHECK_EQ(*content_controls[0].alias, "Customer Name");
    REQUIRE(content_controls[0].id.has_value());
    CHECK_EQ(*content_controls[0].id, "42");
    CHECK_FALSE(content_controls[0].showing_placeholder);
    CHECK_EQ(content_controls[0].text, "Ada Lovelace");
    CHECK(content_controls[0].has_tag());
    CHECK(content_controls[0].has_alias());

    CHECK_EQ(content_controls[1].kind, featherdoc::content_control_kind::run);
    CHECK(content_controls[1].showing_placeholder);
    CHECK_EQ(content_controls[1].text, "INV-001");
    CHECK_EQ(content_controls[2].kind,
             featherdoc::content_control_kind::table_row);
    CHECK_EQ(content_controls[2].text, "SKU-1");

    const auto order_controls = doc.find_content_controls_by_tag("order_no");
    CHECK_FALSE(doc.last_error());
    REQUIRE(order_controls.size() == 1U);
    CHECK_EQ(order_controls.front().index, 1U);
    REQUIRE(order_controls.front().alias.has_value());
    CHECK_EQ(*order_controls.front().alias, "Order Number");

    const auto line_item_controls =
        doc.body_template().find_content_controls_by_alias("Line Items");
    CHECK_FALSE(doc.last_error());
    REQUIRE(line_item_controls.size() == 1U);
    CHECK_EQ(line_item_controls.front().kind,
             featherdoc::content_control_kind::table_row);

    const auto missing_controls = doc.find_content_controls_by_tag("missing");
    CHECK_FALSE(doc.last_error());
    CHECK(missing_controls.empty());

    const auto invalid_controls = doc.find_content_controls_by_tag("");
    CHECK(invalid_controls.empty());
    CHECK_EQ(doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "content control tag must not be empty");
    CHECK_EQ(doc.last_error().entry_name, test_document_xml_entry);

    fs::remove(target);
}

TEST_CASE("content control inspection handles deeply nested XML iteratively") {
    namespace fs = std::filesystem;

    constexpr std::size_t nesting_depth = 25'000U;
    const auto target =
        fs::current_path() / "content_controls_deep_nesting.docx";
    fs::remove(target);

    const auto nested_text =
        wrap_in_deep_xml("<w:r><w:t>deep value</w:t></w:r>", nesting_depth);
    const auto content_control =
        std::string{"<w:sdt><w:sdtPr><w:tag w:val=\"deep\"/></w:sdtPr>"
                    "<w:sdtContent>"} +
        nested_text + "</w:sdtContent></w:sdt>";
    const auto document_xml =
        std::string{"<w:document xmlns:w=\"http://schemas.openxmlformats.org/"
                    "wordprocessingml/2006/main\"><w:body>"} +
        wrap_in_deep_xml(content_control, nesting_depth) +
        "</w:body></w:document>";
    write_test_docx(target, document_xml);

    featherdoc::document_open_options options;
    options.limits.max_compression_ratio = 100'000U;
    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open(options));

    const auto controls = document.list_content_controls();
    REQUIRE_EQ(controls.size(), 1U);
    CHECK_EQ(controls.front().text, "deep value");
    CHECK_FALSE(document.last_error());

    fs::remove(target);
}

TEST_CASE("content control mutations handle deeply nested XML iteratively") {
    namespace fs = std::filesystem;

    constexpr std::size_t nesting_depth = 25'000U;
    const auto target =
        fs::current_path() / "content_controls_deep_mutation.docx";
    fs::remove(target);

    const auto controls =
        std::string{"<w:sdt><w:sdtPr><w:tag w:val=\"deep_text\"/></w:sdtPr>"
                    "<w:sdtContent><w:p><w:r><w:t>old</w:t></w:r></w:p>"
                    "</w:sdtContent></w:sdt>"
                    "<w:sdt><w:sdtPr><w:tag w:val=\"deep_checkbox\"/>"
                    "<w14:checkbox><w14:checked w14:val=\"1\"/></w14:checkbox>"
                    "</w:sdtPr><w:sdtContent><w:p><w:r><w:t>☒</w:t></w:r></w:p>"
                    "</w:sdtContent></w:sdt>"
                    "<w:sdt><w:sdtPr><w:tag w:val=\"deep_rich\"/></w:sdtPr>"
                    "<w:sdtContent><w:p><w:r><w:t>old rich</w:t></w:r></w:p>"
                    "</w:sdtContent></w:sdt>"};
    const auto document_xml =
        std::string{"<w:document xmlns:w=\"http://schemas.openxmlformats.org/"
                    "wordprocessingml/2006/main\" "
                    "xmlns:w14=\"http://schemas.microsoft.com/office/word/"
                    "2010/wordml\"><w:body>"} +
        wrap_in_deep_xml(controls, nesting_depth) + "</w:body></w:document>";
    write_test_docx(target, document_xml);

    featherdoc::document_open_options options;
    options.limits.max_compression_ratio = 100'000U;
    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open(options));

    CHECK_EQ(
        document.replace_content_control_text_by_tag("deep_text", "updated"),
        1U);
    featherdoc::content_control_form_state_options checkbox_options;
    checkbox_options.checked = false;
    CHECK_EQ(document.set_content_control_form_state_by_tag("deep_checkbox",
                                                            checkbox_options),
             1U);
    CHECK_EQ(document.replace_content_control_with_paragraphs_by_tag(
                 "deep_rich", {"first", "second"}),
             1U);

    const auto summaries = document.list_content_controls();
    REQUIRE_EQ(summaries.size(), 3U);
    CHECK_EQ(summaries[0].text, "updated");
    CHECK_EQ(summaries[1].text, "☐");
    CHECK_EQ(summaries[2].text, "firstsecond");
    CHECK_FALSE(document.last_error());

    fs::remove(target);
}

TEST_CASE("content control inspection reports form state metadata") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "content_controls_form_state.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:w14="http://schemas.microsoft.com/office/word/2010/wordml">
  <w:body>
    <w:p>
      <w:r><w:t>Approved: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Approved"/>
          <w:tag w:val="approved"/>
          <w:id w:val="101"/>
          <w:lock w:val="sdtContentLocked"/>
          <w14:checkbox><w14:checked w14:val="1"/></w14:checkbox>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>☒</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:p>
      <w:r><w:t>Status: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Status"/>
          <w:tag w:val="status"/>
          <w:id w:val="102"/>
          <w:dropDownList>
            <w:listItem w:displayText="Draft" w:value="draft"/>
            <w:listItem w:displayText="Approved" w:value="approved"/>
          </w:dropDownList>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>Approved</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Due Date"/>
        <w:tag w:val="due_date"/>
        <w:id w:val="103"/>
        <w:dataBinding w:storeItemID="{11111111-1111-1111-1111-111111111111}" w:xpath="/invoice/dueDate" w:prefixMappings="xmlns:fd=&quot;urn:featherdoc&quot;"/>
        <w:date><w:dateFormat w:val="yyyy-MM-dd"/><w:lid w:val="en-US"/></w:date>
      </w:sdtPr>
      <w:sdtContent><w:p><w:r><w:t>2026-05-01</w:t></w:r></w:p></w:sdtContent>
    </w:sdt>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto controls = doc.list_content_controls();
    REQUIRE(controls.size() == 3U);
    CHECK_EQ(controls[0].form_kind,
             featherdoc::content_control_form_kind::checkbox);
    REQUIRE(controls[0].lock.has_value());
    CHECK_EQ(*controls[0].lock, "sdtContentLocked");
    REQUIRE(controls[0].checked.has_value());
    CHECK(*controls[0].checked);

    CHECK_EQ(controls[1].form_kind,
             featherdoc::content_control_form_kind::drop_down_list);
    REQUIRE(controls[1].selected_list_item.has_value());
    CHECK_EQ(*controls[1].selected_list_item, 1U);
    REQUIRE(controls[1].list_items.size() == 2U);
    CHECK_EQ(controls[1].list_items[0].display_text, "Draft");
    CHECK_EQ(controls[1].list_items[0].value, "draft");
    CHECK_EQ(controls[1].list_items[1].display_text, "Approved");
    CHECK_EQ(controls[1].list_items[1].value, "approved");

    CHECK_EQ(controls[2].form_kind,
             featherdoc::content_control_form_kind::date);
    REQUIRE(controls[2].date_format.has_value());
    CHECK_EQ(*controls[2].date_format, "yyyy-MM-dd");
    REQUIRE(controls[2].date_locale.has_value());
    CHECK_EQ(*controls[2].date_locale, "en-US");
    REQUIRE(controls[2].data_binding_store_item_id.has_value());
    CHECK_EQ(*controls[2].data_binding_store_item_id,
             "{11111111-1111-1111-1111-111111111111}");
    REQUIRE(controls[2].data_binding_xpath.has_value());
    CHECK_EQ(*controls[2].data_binding_xpath, "/invoice/dueDate");
    REQUIRE(controls[2].data_binding_prefix_mappings.has_value());
    CHECK_EQ(*controls[2].data_binding_prefix_mappings,
             "xmlns:fd=\"urn:featherdoc\"");

    fs::remove(target);
}

TEST_CASE("content control form state can be mutated by tag or alias") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "content_controls_form_state_mutation.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:w14="http://schemas.microsoft.com/office/word/2010/wordml">
  <w:body>
    <w:p>
      <w:r><w:t>Approved: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Approved"/>
          <w:tag w:val="approved"/>
          <w:id w:val="101"/>
          <w:lock w:val="sdtContentLocked"/>
          <w14:checkbox><w14:checked w14:val="1"/></w14:checkbox>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>☒</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:p>
      <w:r><w:t>Status: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Status"/>
          <w:tag w:val="status"/>
          <w:id w:val="102"/>
          <w:dropDownList>
            <w:listItem w:displayText="Draft" w:value="draft"/>
            <w:listItem w:displayText="Approved" w:value="approved"/>
          </w:dropDownList>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>Approved</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Due Date"/>
        <w:tag w:val="due_date"/>
        <w:id w:val="103"/>
        <w:date><w:dateFormat w:val="yyyy-MM-dd"/><w:lid w:val="en-US"/></w:date>
      </w:sdtPr>
      <w:sdtContent><w:p><w:r><w:t>2026-05-01</w:t></w:r></w:p></w:sdtContent>
    </w:sdt>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    featherdoc::content_control_form_state_options checkbox_options;
    checkbox_options.clear_lock = true;
    checkbox_options.checked = false;
    CHECK_EQ(
        doc.set_content_control_form_state_by_tag("approved", checkbox_options),
        1U);
    CHECK_FALSE(doc.last_error());

    featherdoc::content_control_form_state_options list_options;
    list_options.lock = "sdtLocked";
    list_options.selected_list_item = "draft";
    CHECK_EQ(
        doc.set_content_control_form_state_by_alias("Status", list_options),
        1U);
    CHECK_FALSE(doc.last_error());

    featherdoc::content_control_form_state_options date_options;
    date_options.date_text = "2026-06-01";
    date_options.date_format = "yyyy/MM/dd";
    date_options.date_locale = "zh-CN";
    date_options.data_binding_store_item_id =
        "{22222222-2222-2222-2222-222222222222}";
    date_options.data_binding_xpath = "/invoice/dueDate";
    date_options.data_binding_prefix_mappings = "xmlns:fd=\"urn:featherdoc\"";
    CHECK_EQ(doc.body_template().set_content_control_form_state_by_tag(
                 "due_date", date_options),
             1U);
    CHECK_FALSE(doc.last_error());

    const auto controls = doc.list_content_controls();
    REQUIRE(controls.size() == 3U);
    CHECK_EQ(controls[0].form_kind,
             featherdoc::content_control_form_kind::checkbox);
    REQUIRE(controls[0].checked.has_value());
    CHECK_FALSE(*controls[0].checked);
    CHECK_FALSE(controls[0].lock.has_value());
    CHECK_EQ(controls[0].text, "☐");

    CHECK_EQ(controls[1].form_kind,
             featherdoc::content_control_form_kind::drop_down_list);
    REQUIRE(controls[1].lock.has_value());
    CHECK_EQ(*controls[1].lock, "sdtLocked");
    REQUIRE(controls[1].selected_list_item.has_value());
    CHECK_EQ(*controls[1].selected_list_item, 0U);
    CHECK_EQ(controls[1].text, "Draft");

    CHECK_EQ(controls[2].form_kind,
             featherdoc::content_control_form_kind::date);
    REQUIRE(controls[2].date_format.has_value());
    CHECK_EQ(*controls[2].date_format, "yyyy/MM/dd");
    REQUIRE(controls[2].date_locale.has_value());
    CHECK_EQ(*controls[2].date_locale, "zh-CN");
    CHECK_EQ(controls[2].text, "2026-06-01");
    REQUIRE(controls[2].data_binding_store_item_id.has_value());
    CHECK_EQ(*controls[2].data_binding_store_item_id,
             "{22222222-2222-2222-2222-222222222222}");
    REQUIRE(controls[2].data_binding_xpath.has_value());
    CHECK_EQ(*controls[2].data_binding_xpath, "/invoice/dueDate");
    REQUIRE(controls[2].data_binding_prefix_mappings.has_value());
    CHECK_EQ(*controls[2].data_binding_prefix_mappings,
             "xmlns:fd=\"urn:featherdoc\"");

    featherdoc::content_control_form_state_options clear_binding_options;
    clear_binding_options.clear_data_binding = true;
    CHECK_EQ(doc.set_content_control_form_state_by_tag("due_date",
                                                       clear_binding_options),
             1U);
    CHECK_FALSE(doc.last_error());
    const auto cleared_controls = doc.list_content_controls();
    REQUIRE(cleared_controls.size() == 3U);
    CHECK_FALSE(cleared_controls[2].data_binding_store_item_id.has_value());
    CHECK_FALSE(cleared_controls[2].data_binding_xpath.has_value());
    CHECK_FALSE(cleared_controls[2].data_binding_prefix_mappings.has_value());

    featherdoc::content_control_form_state_options empty_options;
    CHECK_EQ(
        doc.set_content_control_form_state_by_tag("approved", empty_options),
        0U);
    CHECK_EQ(doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(
        doc.last_error().detail,
        "content control form-state options must include at least one change");

    fs::remove(target);
}

TEST_CASE("content controls can sync text from Custom XML data bindings") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "content_controls_custom_xml_sync.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Due Date"/>
        <w:tag w:val="due_date"/>
        <w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/invoice/dueDate"/>
      </w:sdtPr>
      <w:sdtContent><w:p><w:r><w:t>Pending date</w:t></w:r></w:p></w:sdtContent>
    </w:sdt>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Total"/>
        <w:tag w:val="total"/>
        <w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/fd:invoice/fd:total" w:prefixMappings="xmlns:fd=&quot;urn:featherdoc&quot;"/>
      </w:sdtPr>
      <w:sdtContent><w:p><w:r><w:t>Pending total</w:t></w:r></w:p></w:sdtContent>
    </w:sdt>
    <w:sdt>
      <w:sdtPr>
        <w:tag w:val="missing"/>
        <w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/invoice/missing"/>
      </w:sdtPr>
      <w:sdtContent><w:p><w:r><w:t>Missing value</w:t></w:r></w:p></w:sdtContent>
    </w:sdt>
  </w:body>
</w:document>
)";
    const std::string custom_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<invoice xmlns="urn:featherdoc">
  <dueDate>2026-07-15</dueDate>
  <total currency="USD">123.45</total>
</invoice>
)";
    const std::string item_props =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<ds:datastoreItem ds:itemID="{55555555-5555-5555-5555-555555555555}"
                  xmlns:ds="http://schemas.openxmlformats.org/officeDocument/2006/customXml">
  <ds:schemaRefs/>
</ds:datastoreItem>
)";
    const std::string item_relationships =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<pkg:Relationships xmlns:pkg="http://schemas.openxmlformats.org/package/2006/relationships"
                   xmlns:item="http://schemas.openxmlformats.org/package/2006/relationships">
  <item:Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps"
                Target="itemProps1.xml"/>
</pkg:Relationships>
)";

    write_test_archive_entries(
        target, {{test_content_types_xml_entry, test_content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, document_xml},
                 {"customXml/item1.xml", custom_xml},
                 {"customXml/itemProps1.xml", item_props},
                 {"customXml/_rels/item1.xml.rels", item_relationships}});

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.sync_content_controls_from_custom_xml();
    REQUIRE(result.has_value());
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(result->scanned_content_controls, 3U);
    CHECK_EQ(result->bound_content_controls, 3U);
    CHECK_EQ(result->synced_content_controls, 2U);
    REQUIRE(result->synced_items.size() == 2U);
    CHECK_EQ(result->synced_items[0].previous_text, "Pending date");
    CHECK_EQ(result->synced_items[0].value, "2026-07-15");
    CHECK_EQ(result->synced_items[1].value, "123.45");
    REQUIRE(result->issues.size() == 1U);
    CHECK_EQ(result->issues[0].reason, "custom_xml_value_not_found");
    CHECK_EQ(result->issues[0].xpath, "/invoice/missing");

    const auto controls = doc.list_content_controls();
    REQUIRE(controls.size() == 3U);
    CHECK_EQ(controls[0].text, "2026-07-15");
    CHECK_EQ(controls[1].text, "123.45");
    CHECK_EQ(controls[2].text, "Missing value");

    CHECK_FALSE(doc.save());
    const auto saved_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_xml.find("2026-07-15"), std::string::npos);
    CHECK_NE(saved_xml.find("123.45"), std::string::npos);
    CHECK_NE(saved_xml.find("Missing value"), std::string::npos);

    fs::remove(target);
}

TEST_CASE("Custom XML synchronization commits and saves body header and "
          "footer together") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "content_controls_custom_xml_all_wml_parts.docx";
    fs::remove(target);

    constexpr auto content_types_xml =
        R"(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="xml" ContentType="application/xml"/><Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/><Override PartName="/word/header1.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/><Override PartName="/word/footer1.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/></Types>)";
    constexpr auto document_relationships_xml =
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rIdHeader" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header" Target="header1.xml"/><Relationship Id="rIdFooter" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer" Target="footer1.xml"/></Relationships>)";
    constexpr auto document_xml =
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"><w:body><w:sdt><w:sdtPr><w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/invoice/body"/></w:sdtPr><w:sdtContent><w:p><w:r><w:t>old body</w:t></w:r></w:p></w:sdtContent></w:sdt><w:sectPr><w:headerReference w:type="default" r:id="rIdHeader"/><w:footerReference w:type="default" r:id="rIdFooter"/></w:sectPr></w:body></w:document>)";
    constexpr auto header_xml =
        R"(<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:sdt><w:sdtPr><w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/invoice/header"/></w:sdtPr><w:sdtContent><w:p><w:r><w:t>old header</w:t></w:r></w:p></w:sdtContent></w:sdt></w:hdr>)";
    constexpr auto footer_xml =
        R"(<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:sdt><w:sdtPr><w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/invoice/footer"/></w:sdtPr><w:sdtContent><w:p><w:r><w:t>old footer</w:t></w:r></w:p></w:sdtContent></w:sdt></w:ftr>)";
    constexpr auto item_xml =
        R"(<invoice><body>new body</body><header>new header</header><footer>new footer</footer></invoice>)";
    constexpr auto item_props =
        R"(<ds:datastoreItem ds:itemID="{55555555-5555-5555-5555-555555555555}" xmlns:ds="http://schemas.openxmlformats.org/officeDocument/2006/customXml"><ds:schemaRefs/></ds:datastoreItem>)";
    constexpr auto item_relationships =
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps" Target="itemProps1.xml"/></Relationships>)";
    write_test_archive_entries(
        target, {{test_content_types_xml_entry, content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, document_xml},
                 {"word/_rels/document.xml.rels", document_relationships_xml},
                 {"word/header1.xml", header_xml},
                 {"word/footer1.xml", footer_xml},
                 {"customXml/item1.xml", item_xml},
                 {"customXml/itemProps1.xml", item_props},
                 {"customXml/_rels/item1.xml.rels", item_relationships}});

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    const auto result = document.sync_content_controls_from_custom_xml();
    REQUIRE(result.has_value());
    CHECK_EQ(result->synced_content_controls, 3U);
    REQUIRE_FALSE(document.save());

    const auto saved_document =
        read_test_docx_entry(target, test_document_xml_entry);
    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    const auto saved_footer = read_test_docx_entry(target, "word/footer1.xml");
    CHECK_NE(saved_document.find("new body"), std::string::npos);
    CHECK_EQ(saved_document.find("old body"), std::string::npos);
    CHECK_NE(saved_header.find("new header"), std::string::npos);
    CHECK_EQ(saved_header.find("old header"), std::string::npos);
    CHECK_NE(saved_footer.find("new footer"), std::string::npos);
    CHECK_EQ(saved_footer.find("old footer"), std::string::npos);

    fs::remove(target);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "Custom XML text synchronization publishes atomically across "
    "mutation allocation failures") {
    namespace fs = std::filesystem;

    const auto matching_target =
        fs::current_path() / "content_controls_custom_xml_atomic.docx";
    const auto missing_target =
        fs::current_path() / "content_controls_custom_xml_atomic_missing.docx";
    fs::remove(matching_target);
    fs::remove(missing_target);

    const auto make_document_xml = [](bool values_exist) {
        const auto first_name = values_exist ? "value1" : "absent";
        const auto second_name = values_exist ? "value2" : "absent";
        return std::string{
                   R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:t>anchor</w:t></w:r></w:p><w:sdt><w:sdtPr><w:tag w:val="first"/><w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/invoice/)"} +
               first_name +
               R"("/></w:sdtPr><w:sdtContent><w:p><w:r><w:t>old first</w:t></w:r></w:p></w:sdtContent></w:sdt><w:sdt><w:sdtPr><w:tag w:val="second"/><w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/invoice/)" +
               second_name +
               R"("/></w:sdtPr><w:sdtContent><w:p><w:r><w:t>old second</w:t></w:r></w:p></w:sdtContent></w:sdt></w:body></w:document>)";
    };

    std::string first_value(96U * 1024U, 'a');
    std::string second_value(96U * 1024U, 'b');
    const auto item_xml = std::string{"<invoice><value1>"} + first_value +
                          "</value1><value2>" + second_value +
                          "</value2></invoice>";
    constexpr auto item_props =
        R"(<ds:datastoreItem ds:itemID="{55555555-5555-5555-5555-555555555555}" xmlns:ds="http://schemas.openxmlformats.org/officeDocument/2006/customXml"><ds:schemaRefs/></ds:datastoreItem>)";
    constexpr auto item_relationships =
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps" Target="itemProps1.xml"/></Relationships>)";
    const auto write_fixture = [&](const fs::path &target, bool values_exist) {
        write_test_archive_entries(
            target, {{test_content_types_xml_entry, test_content_types_xml},
                     {test_relationships_xml_entry, test_relationships_xml},
                     {test_document_xml_entry, make_document_xml(values_exist)},
                     {"customXml/item1.xml", item_xml},
                     {"customXml/itemProps1.xml", item_props},
                     {"customXml/_rels/item1.xml.rels", item_relationships}});
    };
    write_fixture(matching_target, true);
    write_fixture(missing_target, false);
    featherdoc::document_open_options open_options;
    open_options.limits.max_compression_ratio = 100'000U;

    std::size_t allocation_calls_without_mutation = 0U;
    {
        featherdoc::Document document(missing_target);
        REQUIRE_FALSE(document.open(open_options));
        auto old_paragraph = document.paragraphs();
        REQUIRE(old_paragraph.has_next());
        {
            custom_xml_pugi_allocator_guard guard;
            const auto result =
                document.sync_content_controls_from_custom_xml();
            REQUIRE(result.has_value());
            CHECK_EQ(result->synced_content_controls, 0U);
            allocation_calls_without_mutation = custom_xml_allocation_calls;
        }
        CHECK(old_paragraph.valid());
    }

    std::size_t successful_sync_allocation_calls = 0U;
    {
        featherdoc::Document document(matching_target);
        REQUIRE_FALSE(document.open(open_options));
        auto old_paragraph = document.paragraphs();
        REQUIRE(old_paragraph.has_next());
        {
            custom_xml_pugi_allocator_guard guard;
            const auto result =
                document.sync_content_controls_from_custom_xml();
            REQUIRE(result.has_value());
            CHECK_EQ(result->synced_content_controls, 2U);
            successful_sync_allocation_calls = custom_xml_allocation_calls;
        }
        CHECK_FALSE(old_paragraph.valid());
        const auto controls = document.list_content_controls();
        REQUIRE_EQ(controls.size(), 2U);
        CHECK_EQ(controls[0].text, first_value);
        CHECK_EQ(controls[1].text, second_value);
    }

    REQUIRE_GE(successful_sync_allocation_calls,
               allocation_calls_without_mutation + 2U);
    for (std::size_t failure_call = allocation_calls_without_mutation + 1U;
         failure_call <= successful_sync_allocation_calls; ++failure_call) {
        featherdoc::Document document(matching_target);
        REQUIRE_FALSE(document.open(open_options));

        auto old_paragraph = document.paragraphs();
        REQUIRE(old_paragraph.has_next());
        auto old_run = old_paragraph.runs();
        REQUIRE(old_run.has_next());
        REQUIRE(old_run.set_text("anchor with unsaved edit"));
        auto old_body = document.body_template();
        REQUIRE(static_cast<bool>(old_body));

        std::optional<featherdoc::custom_xml_data_binding_sync_result> result;
        {
            custom_xml_pugi_allocator_guard guard;
            custom_xml_failure_call = failure_call;
            result = document.sync_content_controls_from_custom_xml();
            CAPTURE(failure_call);
            CAPTURE(allocation_calls_without_mutation);
            CAPTURE(successful_sync_allocation_calls);
            CAPTURE(custom_xml_allocation_calls);
            CHECK_FALSE(result.has_value());
        }

        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK(old_paragraph.valid());
        CHECK(old_run.valid());
        CHECK_EQ(old_run.get_text(), "anchor with unsaved edit");
        CHECK(static_cast<bool>(old_body));
        const auto controls = document.list_content_controls();
        REQUIRE_EQ(controls.size(), 2U);
        CHECK_EQ(controls[0].text, "old first");
        CHECK_EQ(controls[1].text, "old second");
    }

    fs::remove(matching_target);
    fs::remove(missing_target);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "Custom XML loading reports every pugixml parse allocation failure") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "content_controls_custom_xml_parse_oom.docx";
    fs::remove(target);
    constexpr auto document_xml =
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:t>unchanged</w:t></w:r></w:p></w:body></w:document>)";
    constexpr auto item_xml =
        R"(<invoice><value>parsed value</value></invoice>)";
    constexpr auto item_props =
        R"(<ds:datastoreItem ds:itemID="{55555555-5555-5555-5555-555555555555}" xmlns:ds="http://schemas.openxmlformats.org/officeDocument/2006/customXml"><ds:schemaRefs/></ds:datastoreItem>)";
    constexpr auto item_relationships =
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps" Target="itemProps1.xml"/></Relationships>)";
    write_test_archive_entries(
        target, {{test_content_types_xml_entry, test_content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, document_xml},
                 {"customXml/item1.xml", item_xml},
                 {"customXml/itemProps1.xml", item_props},
                 {"customXml/_rels/item1.xml.rels", item_relationships}});

    std::size_t successful_load_allocation_count = 0U;
    {
        featherdoc::Document baseline(target);
        REQUIRE_FALSE(baseline.open());
        custom_xml_pugi_allocator_guard guard;
        const auto result = baseline.sync_content_controls_from_custom_xml();
        REQUIRE(result.has_value());
        successful_load_allocation_count = custom_xml_allocation_calls;
    }
    REQUIRE_GT(successful_load_allocation_count, 2U);

    for (std::size_t current_failure_call = 1U;
         current_failure_call <= successful_load_allocation_count;
         ++current_failure_call) {
        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());

        std::optional<featherdoc::custom_xml_data_binding_sync_result> result;
        {
            custom_xml_pugi_allocator_guard guard;
            custom_xml_failure_call = current_failure_call;
            result = document.sync_content_controls_from_custom_xml();
            CAPTURE(current_failure_call);
            CAPTURE(successful_load_allocation_count);
            CAPTURE(custom_xml_allocation_calls);
            CHECK_FALSE(result.has_value());
        }

        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK_EQ(document.list_content_controls().size(), 0U);
    }

    fs::remove(target);
}

TEST_CASE("Custom XML sync rejects an existing relationships part with an "
          "invalid root or namespace") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "content_controls_custom_xml_invalid_rels.docx";
    constexpr auto document_xml =
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:sdt><w:sdtPr><w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/invoice/value"/></w:sdtPr><w:sdtContent><w:p><w:r><w:t>must remain</w:t></w:r></w:p></w:sdtContent></w:sdt></w:body></w:document>)";
    constexpr auto item_xml =
        R"(<invoice><value>must not be imported</value></invoice>)";
    constexpr auto item_props =
        R"(<ds:datastoreItem ds:itemID="{55555555-5555-5555-5555-555555555555}" xmlns:ds="http://schemas.openxmlformats.org/officeDocument/2006/customXml"><ds:schemaRefs/></ds:datastoreItem>)";
    constexpr auto relationship =
        R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps" Target="itemProps1.xml"/>)";
    const std::vector<std::pair<std::string, featherdoc::document_errc>>
        invalid_relationships_parts{
            {std::string{
                 R"(<Relationships xmlns="urn:invalid:relationships">)"} +
                 relationship + "</Relationships>",
             featherdoc::document_errc::mce_mismatch},
            {std::string{
                 R"(<NotRelationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)"} +
                 relationship + "</NotRelationships>",
             featherdoc::document_errc::invalid_package_structure},
        };

    for (const auto &[relationships_xml, expected_error] :
         invalid_relationships_parts) {
        for (const auto mode :
             {featherdoc::package_validation_mode::strict,
              featherdoc::package_validation_mode::tolerant}) {
            write_test_archive_entries(
                target,
                {{test_content_types_xml_entry, test_content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, document_xml},
                 {"customXml/item1.xml", item_xml},
                 {"customXml/itemProps1.xml", item_props},
                 {"customXml/_rels/item1.xml.rels", relationships_xml}});

            featherdoc::document_open_options options;
            options.validation = mode;
            featherdoc::Document document(target);
            REQUIRE_FALSE(document.open(options));
            CHECK_FALSE(
                document.sync_content_controls_from_custom_xml().has_value());
            CHECK_EQ(document.last_error().code, expected_error);
            CHECK_EQ(document.last_error().entry_name,
                     "customXml/_rels/item1.xml.rels");

            const auto controls = document.list_content_controls();
            REQUIRE_EQ(controls.size(), 1U);
            CHECK_EQ(controls.front().text, "must remain");
        }
    }

    fs::remove(target);
}

TEST_CASE("Custom XML Relationships MCE is sanitized and mismatches fail "
          "closed") {
    namespace fs = std::filesystem;
    const auto target =
        fs::current_path() / "content_controls_custom_xml_mce.docx";
    constexpr auto document_xml =
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:sdt><w:sdtPr><w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/invoice/value"/></w:sdtPr><w:sdtContent><w:p><w:r><w:t>must remain</w:t></w:r></w:p></w:sdtContent></w:sdt></w:body></w:document>)";
    constexpr auto item_xml =
        R"(<invoice><value>来自安全 MCE</value></invoice>)";
    constexpr auto item_props =
        R"(<ds:datastoreItem ds:itemID="{55555555-5555-5555-5555-555555555555}" xmlns:ds="http://schemas.openxmlformats.org/officeDocument/2006/customXml"><ds:schemaRefs/></ds:datastoreItem>)";
    constexpr auto valid_mce_relationships = R"(
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"
               xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"
               xmlns:x="urn:extension" mc:Ignorable="x">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps" Target="itemProps1.xml" x:discard="yes"/>
</Relationships>)";
    constexpr auto mismatched_relationships = R"(
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"
               xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"
               xmlns:x="urn:unsupported" mc:MustUnderstand="x">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps" Target="itemProps1.xml"/>
</Relationships>)";

    const auto write_fixture = [&](std::string_view relationships) {
        write_test_archive_entries(
            target,
            {{test_content_types_xml_entry, test_content_types_xml},
             {test_relationships_xml_entry, test_relationships_xml},
             {test_document_xml_entry, document_xml},
             {"customXml/item1.xml", item_xml},
             {"customXml/itemProps1.xml", item_props},
             {"customXml/_rels/item1.xml.rels", std::string{relationships}}});
    };

    for (const auto mode : {featherdoc::package_validation_mode::strict,
                            featherdoc::package_validation_mode::tolerant}) {
        featherdoc::document_open_options options;
        options.validation = mode;

        write_fixture(valid_mce_relationships);
        featherdoc::Document valid_document(target);
        REQUIRE_FALSE(valid_document.open(options));
        const auto synced =
            valid_document.sync_content_controls_from_custom_xml();
        REQUIRE(synced.has_value());
        CHECK_EQ(synced->synced_content_controls, 1U);
        CHECK_EQ(valid_document.list_content_controls().front().text,
                 "来自安全 MCE");

        write_fixture(mismatched_relationships);
        featherdoc::Document invalid_document(target);
        REQUIRE_FALSE(invalid_document.open(options));
        CHECK_FALSE(invalid_document.sync_content_controls_from_custom_xml()
                        .has_value());
        CHECK_EQ(invalid_document.last_error().code,
                 featherdoc::document_errc::mce_mismatch);
        CHECK_EQ(invalid_document.last_error().entry_name,
                 "customXml/_rels/item1.xml.rels");
        CHECK_EQ(invalid_document.list_content_controls().front().text,
                 "must remain");
    }

    fs::remove(target);
}

TEST_CASE("Custom XML sync does not treat a relationship type suffix as the "
          "Custom XML properties relationship") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "content_controls_custom_xml_fake_type.docx";
    constexpr auto document_xml =
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:sdt><w:sdtPr><w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/invoice/value"/></w:sdtPr><w:sdtContent><w:p><w:r><w:t>must remain</w:t></w:r></w:p></w:sdtContent></w:sdt></w:body></w:document>)";
    constexpr auto item_xml =
        R"(<invoice><value>must not be imported</value></invoice>)";
    constexpr auto unrelated_props =
        R"(<ds:datastoreItem ds:itemID="{55555555-5555-5555-5555-555555555555}" xmlns:ds="http://schemas.openxmlformats.org/officeDocument/2006/customXml"><ds:schemaRefs/></ds:datastoreItem>)";
    constexpr auto relationships_xml =
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="urn:unrelated/customXmlProps" Target="unrelatedProps.xml"/></Relationships>)";
    write_test_archive_entries(
        target, {{test_content_types_xml_entry, test_content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, document_xml},
                 {"customXml/item1.xml", item_xml},
                 {"customXml/unrelatedProps.xml", unrelated_props},
                 {"customXml/_rels/item1.xml.rels", relationships_xml}});

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    const auto result = document.sync_content_controls_from_custom_xml();
    REQUIRE(result.has_value());
    CHECK_EQ(result->synced_content_controls, 0U);
    const auto controls = document.list_content_controls();
    REQUIRE_EQ(controls.size(), 1U);
    CHECK_EQ(controls.front().text, "must remain");

    fs::remove(target);
}

TEST_CASE("Custom XML sync rejects multiple properties relationships for one "
          "item") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "content_controls_custom_xml_duplicate_props.docx";
    constexpr auto document_xml =
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:sdt><w:sdtPr><w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/invoice/value"/></w:sdtPr><w:sdtContent><w:p><w:r><w:t>must remain</w:t></w:r></w:p></w:sdtContent></w:sdt></w:body></w:document>)";
    constexpr auto item_xml =
        R"(<invoice><value>must not be imported</value></invoice>)";
    constexpr auto item_props =
        R"(<ds:datastoreItem ds:itemID="{55555555-5555-5555-5555-555555555555}" xmlns:ds="http://schemas.openxmlformats.org/officeDocument/2006/customXml"><ds:schemaRefs/></ds:datastoreItem>)";
    constexpr auto relationships_xml =
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps" Target="itemProps1.xml"/><Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps" Target="itemProps2.xml"/></Relationships>)";
    write_test_archive_entries(
        target, {{test_content_types_xml_entry, test_content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, document_xml},
                 {"customXml/item1.xml", item_xml},
                 {"customXml/itemProps1.xml", item_props},
                 {"customXml/itemProps2.xml", item_props},
                 {"customXml/_rels/item1.xml.rels", relationships_xml}});

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    CHECK_FALSE(document.sync_content_controls_from_custom_xml().has_value());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::invalid_package_structure);
    CHECK_EQ(document.last_error().entry_name,
             "customXml/_rels/item1.xml.rels");
    const auto controls = document.list_content_controls();
    REQUIRE_EQ(controls.size(), 1U);
    CHECK_EQ(controls.front().text, "must remain");

    fs::remove(target);
}

TEST_CASE("tolerant Custom XML sync reopens raw Unicode package entries by "
          "canonical identity") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "content_controls_custom_xml_unicode_entry.docx";
    fs::remove(target);

    constexpr auto document_xml =
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:sdt><w:sdtPr><w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/invoice/value"/></w:sdtPr><w:sdtContent><w:p><w:r><w:t>old</w:t></w:r></w:p></w:sdtContent></w:sdt></w:body></w:document>)";
    constexpr auto item_xml =
        R"(<invoice><value>来自中文包路径</value></invoice>)";
    constexpr auto item_props =
        R"(<ds:datastoreItem ds:itemID="{55555555-5555-5555-5555-555555555555}" xmlns:ds="http://schemas.openxmlformats.org/officeDocument/2006/customXml"><ds:schemaRefs/></ds:datastoreItem>)";
    constexpr auto item_relationships =
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps" Target="itemProps项目.xml"/></Relationships>)";

    write_test_archive_entries(
        target, {{test_content_types_xml_entry, test_content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, document_xml},
                 {"CUSTOMXML/ITEM项目.XML", item_xml},
                 {"CUSTOMXML/ITEMPROPS项目.XML", item_props},
                 {"CUSTOMXML/_rels/ITEM项目.XML.rels", item_relationships}});

    featherdoc::Document strict_document(target);
    CHECK_EQ(strict_document.open(),
             featherdoc::document_errc::invalid_package_structure);

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document tolerant_document(target);
    REQUIRE_FALSE(tolerant_document.open(options));

    const auto result =
        tolerant_document.sync_content_controls_from_custom_xml();
    REQUIRE(result.has_value());
    REQUIRE_EQ(result->synced_items.size(), 1U);
    CHECK_EQ(result->synced_items.front().value, "来自中文包路径");
    CHECK_FALSE(tolerant_document.last_error());

    fs::remove(target);
}

TEST_CASE(
    "Custom XML sync enforces semantic XML limits on binary-named props") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "content_controls_custom_xml_props_limit.docx";
    fs::remove(target);

    constexpr auto document_xml =
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:sdt><w:sdtPr><w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/invoice/value"/></w:sdtPr><w:sdtContent><w:p><w:r><w:t>old</w:t></w:r></w:p></w:sdtContent></w:sdt></w:body></w:document>)";
    constexpr auto item_xml = R"(<invoice><value>new</value></invoice>)";
    constexpr auto item_relationships =
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps" Target="itemProps1.bin"/></Relationships>)";
    const auto make_item_props = [](std::size_t filler_size) {
        std::string xml =
            R"(<ds:datastoreItem ds:itemID="{55555555-5555-5555-5555-555555555555}" xmlns:ds="http://schemas.openxmlformats.org/officeDocument/2006/customXml"><ds:schemaRefs/><!--)";
        xml.append(filler_size, 'x');
        xml += "--></ds:datastoreItem>";
        return xml;
    };
    const auto write_fixture = [&](std::string item_props) {
        write_test_archive_entries(
            target, {{test_content_types_xml_entry, test_content_types_xml},
                     {test_relationships_xml_entry, test_relationships_xml},
                     {test_document_xml_entry, document_xml},
                     {"customXml/item1.xml", item_xml},
                     {"customXml/itemProps1.bin", std::move(item_props)},
                     {"customXml/_rels/item1.xml.rels", item_relationships}});
    };

    featherdoc::document_open_options options;
    options.limits.max_xml_part_bytes = 1024U;
    options.limits.max_binary_part_bytes = 8192U;
    options.limits.max_total_uncompressed_bytes = 64U * 1024U;
    options.limits.max_compression_ratio = 10'000U;
    write_fixture(make_item_props(2048U));
    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open(options));
    CHECK_FALSE(document.sync_content_controls_from_custom_xml().has_value());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::archive_limit_exceeded);
    CHECK_EQ(document.last_error().entry_name, "customXml/itemProps1.bin");

    fs::remove(target);
}

TEST_CASE("Custom XML sync handles deeply nested bound values iteratively") {
    namespace fs = std::filesystem;

    constexpr std::size_t nesting_depth = 25'000U;
    const auto target =
        fs::current_path() / "content_controls_custom_xml_deep_nesting.docx";
    fs::remove(target);

    constexpr auto document_xml =
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:sdt><w:sdtPr><w:tag w:val="deep"/><w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/invoice/value"/></w:sdtPr><w:sdtContent><w:p><w:r><w:t>old</w:t></w:r></w:p></w:sdtContent></w:sdt></w:body></w:document>)";
    const auto item_xml = std::string{"<invoice><value>"} +
                          wrap_in_deep_xml("deep custom value", nesting_depth) +
                          "</value></invoice>";
    constexpr auto item_props =
        R"(<ds:datastoreItem ds:itemID="{55555555-5555-5555-5555-555555555555}" xmlns:ds="http://schemas.openxmlformats.org/officeDocument/2006/customXml"><ds:schemaRefs/></ds:datastoreItem>)";
    constexpr auto item_relationships =
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps" Target="itemProps1.xml"/></Relationships>)";

    write_test_archive_entries(
        target, {{test_content_types_xml_entry, test_content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, document_xml},
                 {"customXml/item1.xml", item_xml},
                 {"customXml/itemProps1.xml", item_props},
                 {"customXml/_rels/item1.xml.rels", item_relationships}});

    featherdoc::document_open_options options;
    options.limits.max_compression_ratio = 100'000U;
    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open(options));

    const auto result = document.sync_content_controls_from_custom_xml();
    REQUIRE(result.has_value());
    CHECK_EQ(result->synced_content_controls, 1U);
    const auto controls = document.list_content_controls();
    REQUIRE_EQ(controls.size(), 1U);
    CHECK_EQ(controls.front().text, "deep custom value");
    CHECK_FALSE(document.last_error());

    fs::remove(target);
}

TEST_CASE("Custom XML sync validates a malicious path after create_empty") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() /
        "content_controls_custom_xml_create_empty_bypass.docx";
    fs::remove(target);

    constexpr auto item_props =
        R"(<ds:datastoreItem ds:itemID="{55555555-5555-5555-5555-555555555555}" xmlns:ds="http://schemas.openxmlformats.org/officeDocument/2006/customXml"><ds:schemaRefs/></ds:datastoreItem>)";
    constexpr auto item_relationships =
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps" Target="itemProps1.xml"/></Relationships>)";
    std::string highly_compressible_item = "<invoice><value>";
    highly_compressible_item.append(256U * 1024U, 'a');
    highly_compressible_item += "</value></invoice>";
    write_test_archive_entries(
        target, {{"customXml/item1.xml", highly_compressible_item},
                 {"customXml/itemProps1.xml", item_props},
                 {"customXml/_rels/item1.xml.rels", item_relationships}});

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.create_empty());
    CHECK_FALSE(document.sync_content_controls_from_custom_xml().has_value());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::archive_limit_exceeded);
    CHECK_EQ(document.last_error().entry_name, "customXml/item1.xml");

    fs::remove(target);
}

TEST_CASE("content control text can be replaced by tag or alias") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "content_controls_replace_text.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Customer Name"/>
        <w:tag w:val="customer_name"/>
        <w:id w:val="42"/>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>Ada Lovelace</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
    <w:p>
      <w:r><w:t>Order: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Order Number"/>
          <w:tag w:val="order_no"/>
          <w:id w:val="43"/>
          <w:showingPlcHdr/>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>INV-001</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:tbl>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Line Items"/>
          <w:tag w:val="line_items"/>
          <w:id w:val="44"/>
        </w:sdtPr>
        <w:sdtContent>
          <w:tr>
            <w:tc><w:p><w:r><w:t>SKU-1</w:t></w:r></w:p></w:tc>
          </w:tr>
        </w:sdtContent>
      </w:sdt>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(
        doc.replace_content_control_text_by_tag("order_no", "INV-002\nready"),
        1U);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(doc.body_template().replace_content_control_text_by_alias(
                 "Line Items", "SKU-2"),
             1U);
    CHECK_FALSE(doc.last_error());

    const auto content_controls = doc.list_content_controls();
    CHECK_FALSE(doc.last_error());
    REQUIRE(content_controls.size() == 3U);
    CHECK_EQ(content_controls[1].text, "INV-002\nready");
    CHECK_FALSE(content_controls[1].showing_placeholder);
    CHECK_EQ(content_controls[2].kind,
             featherdoc::content_control_kind::table_row);
    CHECK_EQ(content_controls[2].text, "SKU-2");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_document_xml.c_str()));

    const auto body = saved_document.child("w:document").child("w:body");
    const auto order_control = body.child("w:p").child("w:sdt");
    REQUIRE(order_control != pugi::xml_node{});
    CHECK(order_control.child("w:sdtPr").child("w:showingPlcHdr") ==
          pugi::xml_node{});
    const auto order_run = order_control.child("w:sdtContent").child("w:r");
    REQUIRE(order_run != pugi::xml_node{});
    CHECK_EQ(std::string{order_run.child("w:t").text().get()}, "INV-002");
    REQUIRE(order_run.child("w:br") != pugi::xml_node{});
    CHECK_EQ(std::string{order_run.last_child().text().get()}, "ready");

    const auto line_item_cell = body.child("w:tbl")
                                    .child("w:sdt")
                                    .child("w:sdtContent")
                                    .child("w:tr")
                                    .child("w:tc");
    REQUIRE(line_item_cell != pugi::xml_node{});
    CHECK_EQ(
        std::string{
            line_item_cell.child("w:p").child("w:r").child("w:t").text().get()},
        "SKU-2");
    CHECK_EQ(saved_document_xml.find("INV-001"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("SKU-1"), std::string::npos);

    CHECK_EQ(doc.replace_content_control_text_by_alias("missing", "noop"), 0U);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(doc.replace_content_control_text_by_tag("", "noop"), 0U);
    CHECK_EQ(doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "content control tag must not be empty");
    CHECK_EQ(doc.last_error().entry_name, test_document_xml_entry);

    fs::remove(target);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "content control text replacement is atomic for every pugixml "
    "allocation failure") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "content_controls_replace_text_atomic.docx";
    fs::remove(target);

    constexpr auto document_xml =
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:t>anchor</w:t></w:r></w:p><w:sdt><w:sdtPr><w:tag w:val="atomic-text"/><w:showingPlcHdr/></w:sdtPr><w:sdtContent><w:p><w:r><w:t>old first</w:t></w:r></w:p></w:sdtContent></w:sdt><w:sdt><w:sdtPr><w:tag w:val="atomic-text"/><w:showingPlcHdr/></w:sdtPr><w:sdtContent><w:p><w:r><w:t>old second</w:t></w:r></w:p></w:sdtContent></w:sdt></w:body></w:document>)";
    write_test_docx(target, document_xml);

    auto replacement = utf8_from_u8(u8"事务替换中文😀-");
    replacement.append(96U * 1024U, 'x');
    replacement += utf8_from_u8(u8"\n重试完成🪶");

    std::size_t successful_allocation_calls = 0U;
    {
        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        auto body = document.body_template();
        REQUIRE(static_cast<bool>(body));

        {
            custom_xml_pugi_allocator_guard guard;
            REQUIRE_EQ(body.replace_content_control_text_by_tag(
                           "atomic-text", replacement),
                       2U);
            successful_allocation_calls = custom_xml_allocation_calls;
        }

        const auto controls = document.list_content_controls();
        REQUIRE_EQ(controls.size(), 2U);
        CHECK_EQ(controls[0].text, replacement);
        CHECK_EQ(controls[1].text, replacement);
        CHECK_FALSE(controls[0].showing_placeholder);
        CHECK_FALSE(controls[1].showing_placeholder);
    }
    REQUIRE_GT(successful_allocation_calls, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_calls; ++failure_call) {
        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());

        auto anchor = document.paragraphs();
        REQUIRE(anchor.valid());
        auto anchor_run = anchor.runs();
        REQUIRE(anchor_run.valid());
        const auto unsaved_anchor = utf8_from_u8(u8"未保存锚点编辑😀");
        REQUIRE(anchor_run.set_text(unsaved_anchor));
        auto body = document.body_template();
        REQUIRE(static_cast<bool>(body));

        std::size_t replaced = 0U;
        std::size_t observed_allocation_calls = 0U;
        {
            custom_xml_pugi_allocator_guard guard;
            custom_xml_failure_call = failure_call;
            replaced = body.replace_content_control_text_by_tag(
                "atomic-text", replacement);
            observed_allocation_calls = custom_xml_allocation_calls;
        }

        CAPTURE(failure_call);
        CAPTURE(successful_allocation_calls);
        CAPTURE(observed_allocation_calls);
        CHECK_EQ(replaced, 0U);
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK(anchor.valid());
        CHECK(anchor_run.valid());
        CHECK_EQ(anchor_run.get_text(), unsaved_anchor);
        CHECK(static_cast<bool>(body));

        const auto unchanged_controls = document.list_content_controls();
        REQUIRE_EQ(unchanged_controls.size(), 2U);
        CHECK_EQ(unchanged_controls[0].text, "old first");
        CHECK_EQ(unchanged_controls[1].text, "old second");
        CHECK(unchanged_controls[0].showing_placeholder);
        CHECK(unchanged_controls[1].showing_placeholder);

        REQUIRE_EQ(body.replace_content_control_text_by_tag("atomic-text",
                                                            replacement),
                   2U);
        CHECK_FALSE(document.last_error());
        CHECK(anchor.valid());
        CHECK(anchor_run.valid());
        CHECK_EQ(anchor_run.get_text(), unsaved_anchor);
        CHECK(static_cast<bool>(body));

        const auto replaced_controls = document.list_content_controls();
        REQUIRE_EQ(replaced_controls.size(), 2U);
        CHECK_EQ(replaced_controls[0].text, replacement);
        CHECK_EQ(replaced_controls[1].text, replacement);
        CHECK_FALSE(replaced_controls[0].showing_placeholder);
        CHECK_FALSE(replaced_controls[1].showing_placeholder);
    }

    fs::remove(target);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "content control text replacement is atomic for every global allocation "
    "failure") {
    namespace fs = std::filesystem;

    const auto target = fs::current_path() /
                        "content_controls_replace_text_global_atomic.docx";
    fs::remove(target);

    constexpr auto document_xml =
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:t>anchor</w:t></w:r></w:p><w:sdt><w:sdtPr><w:tag w:val="atomic-text"/><w:showingPlcHdr/></w:sdtPr><w:sdtContent><w:p><w:r><w:t>old first</w:t></w:r></w:p></w:sdtContent></w:sdt><w:sdt><w:sdtPr><w:tag w:val="atomic-text"/><w:showingPlcHdr/></w:sdtPr><w:sdtContent><w:p><w:r><w:t>old second</w:t></w:r></w:p></w:sdtContent></w:sdt></w:body></w:document>)";
    write_test_docx(target, document_xml);

    auto replacement = utf8_from_u8(u8"全局事务替换中文😀-");
    replacement.append(96U * 1024U, 'y');
    replacement += utf8_from_u8(u8"\n全局重试完成🪶");

    std::size_t successful_allocation_calls = 0U;
    {
        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        auto body = document.body_template();
        REQUIRE(static_cast<bool>(body));

        std::size_t replaced = 0U;
        {
            global_allocation_window window{0U};
            replaced = body.replace_content_control_text_by_tag(
                "atomic-text", replacement);
        }
        REQUIRE_EQ(replaced, 2U);
        successful_allocation_calls =
            global_allocation_calls.load(std::memory_order_relaxed);
    }
    REQUIRE_GT(successful_allocation_calls, 0U);

    std::size_t retirement_failure_cases = 0U;
    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_calls; ++failure_call) {
        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());

        auto anchor = document.paragraphs();
        REQUIRE(anchor.valid());
        auto anchor_run = anchor.runs();
        REQUIRE(anchor_run.valid());
        const auto unsaved_anchor = utf8_from_u8(u8"未保存全局锚点编辑😀");
        REQUIRE(anchor_run.set_text(unsaved_anchor));
        auto body = document.body_template();
        REQUIRE(static_cast<bool>(body));

        std::size_t replaced = 0U;
        bool allocation_failure_escaped = false;
        try {
            global_allocation_window window{failure_call};
            replaced = body.replace_content_control_text_by_tag(
                "atomic-text", replacement);
        } catch (const std::bad_alloc &) {
            allocation_failure_escaped = true;
        }
        const auto observed_allocation_calls =
            global_allocation_calls.load(std::memory_order_relaxed);

        CAPTURE(failure_call);
        CAPTURE(successful_allocation_calls);
        CAPTURE(observed_allocation_calls);
        CHECK_FALSE(allocation_failure_escaped);
        CHECK_GE(observed_allocation_calls, failure_call);
        CHECK_EQ(replaced, 0U);
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK_EQ(document.last_error().entry_name, test_document_xml_entry);
        if (document.last_error().detail ==
            "content control XML handle retirement ran out of memory") {
            ++retirement_failure_cases;
        }
        CHECK(anchor.valid());
        CHECK(anchor_run.valid());
        CHECK_EQ(anchor_run.get_text(), unsaved_anchor);
        CHECK(static_cast<bool>(body));

        const auto unchanged_controls = document.list_content_controls();
        REQUIRE_EQ(unchanged_controls.size(), 2U);
        CHECK_EQ(unchanged_controls[0].text, "old first");
        CHECK_EQ(unchanged_controls[1].text, "old second");
        CHECK(unchanged_controls[0].showing_placeholder);
        CHECK(unchanged_controls[1].showing_placeholder);

        REQUIRE_EQ(body.replace_content_control_text_by_tag("atomic-text",
                                                            replacement),
                   2U);
        CHECK_FALSE(document.last_error());
        CHECK(anchor.valid());
        CHECK(anchor_run.valid());
        CHECK_EQ(anchor_run.get_text(), unsaved_anchor);
        CHECK(static_cast<bool>(body));

        const auto replaced_controls = document.list_content_controls();
        REQUIRE_EQ(replaced_controls.size(), 2U);
        CHECK_EQ(replaced_controls[0].text, replacement);
        CHECK_EQ(replaced_controls[1].text, replacement);
        CHECK_FALSE(replaced_controls[0].showing_placeholder);
        CHECK_FALSE(replaced_controls[1].showing_placeholder);
    }

    CHECK_GT(retirement_failure_cases, 0U);
    fs::remove(target);
}

TEST_CASE("content controls can be replaced with rich paragraphs table rows "
          "and tables") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "content_controls_replace_rich.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:sdt>
      <w:sdtPr>
        <w:tag w:val="summary"/>
        <w:showingPlcHdr/>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>old summary</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
    <w:tbl>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Name</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Qty</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Line Items"/>
          <w:tag w:val="line_items"/>
        </w:sdtPr>
        <w:sdtContent>
          <w:tr>
            <w:trPr><w:cantSplit/></w:trPr>
            <w:tc>
              <w:tcPr><w:shd w:fill="AAAAAA"/></w:tcPr>
              <w:p><w:r><w:rPr><w:b/></w:rPr><w:t>template item</w:t></w:r></w:p>
            </w:tc>
            <w:tc>
              <w:tcPr><w:shd w:fill="BBBBBB"/></w:tcPr>
              <w:p><w:r><w:rPr><w:i/></w:rPr><w:t>template qty</w:t></w:r></w:p>
            </w:tc>
          </w:tr>
        </w:sdtContent>
      </w:sdt>
    </w:tbl>
    <w:sdt>
      <w:sdtPr><w:tag w:val="metrics"/></w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>old metrics</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_content_control_with_paragraphs_by_tag(
                 "summary", {"Executive summary", "Second paragraph"}),
             1U);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(
        doc.body_template().replace_content_control_with_table_rows_by_alias(
            "Line Items", {{"Apple", "2"}, {"Pear", "5"}}),
        1U);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(doc.replace_content_control_with_table_by_tag(
                 "metrics", {{"Metric", "Value"}, {"Quality", "Green"}}),
             1U);
    CHECK_FALSE(doc.last_error());

    const auto controls = doc.list_content_controls();
    REQUIRE(controls.size() == 3U);
    CHECK_EQ(controls[0].text, "Executive summarySecond paragraph");
    CHECK_FALSE(controls[0].showing_placeholder);
    CHECK_EQ(controls[1].kind, featherdoc::content_control_kind::table_row);
    CHECK_EQ(controls[1].text, "Apple2Pear5");
    CHECK_EQ(controls[2].text, "MetricValueQualityGreen");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("old summary"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("template item"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("template qty"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("old metrics"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:showingPlcHdr"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Executive summary"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Second paragraph"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Apple"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Pear"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Metric"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Green"), std::string::npos);
    CHECK_EQ(
        count_substring_occurrences(saved_document_xml, "w:fill=\"AAAAAA\""),
        2U);
    CHECK_EQ(
        count_substring_occurrences(saved_document_xml, "w:fill=\"BBBBBB\""),
        2U);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_document_xml.c_str()));
    const auto body = saved_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});
    const auto summary_content = body.child("w:sdt").child("w:sdtContent");
    REQUIRE(summary_content != pugi::xml_node{});
    CHECK_EQ(count_named_children(summary_content, "w:p"), 2U);
    const auto metrics_content = body.last_child().child("w:sdtContent");
    REQUIRE(metrics_content != pugi::xml_node{});
    CHECK_EQ(count_named_children(metrics_content, "w:tbl"), 1U);
    const auto row_control_content =
        body.child("w:tbl").child("w:sdt").child("w:sdtContent");
    REQUIRE(row_control_content != pugi::xml_node{});
    CHECK_EQ(count_named_children(row_control_content, "w:tr"), 2U);
    CHECK_EQ(count_named_descendants(row_control_content, "w:cantSplit"), 2U);
    CHECK_EQ(count_named_descendants(row_control_content, "w:b"), 2U);
    CHECK_EQ(count_named_descendants(row_control_content, "w:i"), 2U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_controls = reopened.list_content_controls();
    REQUIRE(reopened_controls.size() == 3U);
    CHECK_EQ(reopened_controls[0].text, "Executive summarySecond paragraph");
    CHECK_EQ(reopened_controls[1].text, "Apple2Pear5");
    CHECK_EQ(reopened_controls[2].text, "MetricValueQualityGreen");

    fs::remove(target);
}

TEST_CASE(
    "content control rich replacement rejects incompatible run controls") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "content_controls_replace_rich_invalid.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>Before </w:t></w:r>
      <w:sdt>
        <w:sdtPr><w:tag w:val="inline"/></w:sdtPr>
        <w:sdtContent><w:r><w:t>inline value</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(
        doc.replace_content_control_with_paragraphs_by_tag("inline", {"A"}),
        0U);
    CHECK_EQ(doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("paragraph replacement"),
             std::string::npos);
    CHECK_EQ(doc.last_error().entry_name, test_document_xml_entry);

    CHECK_EQ(doc.replace_content_control_with_table_by_tag("inline", {{"A"}}),
             0U);
    CHECK_EQ(doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("table replacement"),
             std::string::npos);

    CHECK_EQ(
        doc.replace_content_control_with_table_rows_by_tag("inline", {{"A"}}),
        0U);
    CHECK_EQ(doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("table-row content control"),
             std::string::npos);
    const auto controls = doc.list_content_controls();
    REQUIRE(controls.size() == 1U);
    CHECK_EQ(controls.front().text, "inline value");
    CHECK_EQ(collect_document_text(doc), "Before \n");

    fs::remove(target);
}
