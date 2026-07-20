#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "allocation_failure_test_case.hpp"
#include "basic_docx_archive_test_support.hpp"

#include <featherdoc.hpp>
#include <featherdoc/detail/xml_handle.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <pugixml.hpp>

namespace {

struct retirement_fixture final {
    pugi::xml_document document;
    std::shared_ptr<featherdoc::detail::xml_handle_lifetime> lifetime{
        std::make_shared<featherdoc::detail::xml_handle_lifetime>()};

    retirement_fixture() {
        const auto result = this->document.load_string(
            "<root><left><leaf/></left><right><leaf/></right><keep/></root>");
        if (!result) {
            throw std::bad_alloc{};
        }
    }

    [[nodiscard]] auto root() const -> pugi::xml_node {
        return this->document.child("root");
    }

    [[nodiscard]] auto tracked(pugi::xml_node node) const
        -> featherdoc::detail::tracked_xml_node {
        return {node, this->lifetime};
    }
};

std::atomic_bool allocation_tracking_enabled{false};
std::atomic_size_t observed_allocation_calls{0U};
std::atomic_size_t allocation_failure_call{0U};

pugi::allocation_function delegated_pugi_allocate = nullptr;
std::size_t pugi_allocation_calls = 0U;
std::size_t pugi_failure_call = 0U;

auto controlled_pugi_allocate(std::size_t size) -> void * {
    ++pugi_allocation_calls;
    if (pugi_failure_call != 0U &&
        pugi_allocation_calls == pugi_failure_call) {
        return nullptr;
    }
    return delegated_pugi_allocate(size);
}

class pugi_allocator_guard final {
  public:
    pugi_allocator_guard()
        : previous_allocate_(pugi::get_memory_allocation_function()),
          previous_deallocate_(pugi::get_memory_deallocation_function()) {
        delegated_pugi_allocate = this->previous_allocate_;
        pugi_allocation_calls = 0U;
        pugi_failure_call = 0U;
        pugi::set_memory_management_functions(controlled_pugi_allocate,
                                              this->previous_deallocate_);
    }

    pugi_allocator_guard(const pugi_allocator_guard &) = delete;
    auto operator=(const pugi_allocator_guard &)
        -> pugi_allocator_guard & = delete;

    ~pugi_allocator_guard() {
        pugi::set_memory_management_functions(this->previous_allocate_,
                                              this->previous_deallocate_);
        delegated_pugi_allocate = nullptr;
        pugi_allocation_calls = 0U;
        pugi_failure_call = 0U;
    }

  private:
    pugi::allocation_function previous_allocate_;
    pugi::deallocation_function previous_deallocate_;
};

class scoped_test_path final {
  public:
    explicit scoped_test_path(std::filesystem::path path)
        : path_(std::move(path)) {
        std::error_code ignored;
        std::filesystem::remove(this->path_, ignored);
    }

    scoped_test_path(const scoped_test_path &) = delete;
    auto operator=(const scoped_test_path &) -> scoped_test_path & = delete;

    ~scoped_test_path() {
        std::error_code ignored;
        std::filesystem::remove(this->path_, ignored);
    }

    [[nodiscard]] auto path() const -> const std::filesystem::path & {
        return this->path_;
    }

  private:
    std::filesystem::path path_;
};

[[nodiscard]] auto make_test_path(std::string_view scenario,
                                  std::size_t failure_call)
    -> std::filesystem::path {
    auto file_name = std::string{"featherdoc-表格事务-"};
    file_name.append(scenario);
    file_name.push_back('-');
    file_name.append(std::to_string(failure_call));
    file_name.append("-😀.docx");
    auto utf8_file_name = std::u8string{};
    utf8_file_name.reserve(file_name.size());
    for (const auto byte : file_name) {
        utf8_file_name.push_back(
            static_cast<char8_t>(static_cast<unsigned char>(byte)));
    }
    return std::filesystem::temp_directory_path() /
           std::filesystem::path{utf8_file_name};
}

[[nodiscard]] auto large_cell_property_value() -> std::string {
    constexpr auto alphabet =
        std::string_view{"abcdefghijklmnopqrstuvwxyz"
                         "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"};
    auto value = std::string(70U * 1024U, 'a');
    auto state = std::uint32_t{0x6D2B79F5U};
    for (auto &character : value) {
        state = state * 1664525U + 1013904223U;
        character = alphabet[(state >> 24U) % alphabet.size()];
    }
    return value;
}

[[nodiscard]] auto table_fixture_xml(std::size_t rows, std::size_t columns,
                                      bool vertical_merge_chain = false)
    -> std::string {
    auto xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl><w:tblGrid>)"};
    for (std::size_t column = 0U; column < columns; ++column) {
        xml += R"(<w:gridCol w:w="1200"/>)";
    }
    xml += "</w:tblGrid>";

    const auto large_value = large_cell_property_value();
    for (std::size_t row = 0U; row < rows; ++row) {
        xml += "<w:tr>";
        for (std::size_t column = 0U; column < columns; ++column) {
            xml += "<w:tc><w:tcPr data-large=\"";
            xml += large_value;
            xml += "\">";
            if (vertical_merge_chain && column == 0U) {
                xml += row == 0U ? R"(<w:vMerge w:val="restart"/>)"
                                 : R"(<w:vMerge w:val="continue"/>)";
            }
            xml += "</w:tcPr><w:p><w:r><w:t>";
            if (row == 0U && column == 0U) {
                xml += "锚点中文";
                if (vertical_merge_chain) {
                    xml += large_value;
                }
            } else if (column == 0U) {
                xml += "待替换中文";
            } else {
                xml += "不受影响";
            }
            xml += "</w:t></w:r></w:p></w:tc>";
        }
        xml += "</w:tr>";
    }
    xml += "</w:tbl></w:body></w:document>";
    return xml;
}

[[nodiscard]] auto malformed_fixed_table_fixture_xml(std::size_t rows,
                                                      std::size_t columns)
    -> std::string {
    auto xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl><w:tblPr><w:tblLayout w:type="fixed"/></w:tblPr>
  <w:tblGrid><w:gridCol w:w="1200"/></w:tblGrid>)"};
    for (std::size_t row = 0U; row < rows; ++row) {
        xml += "<w:tr>";
        for (std::size_t column = 0U; column < columns; ++column) {
            xml += "<w:tc><w:tcPr></w:tcPr><w:p><w:r><w:t>";
            xml += column == 0U ? "锚点中文" : "不受影响";
            xml += "</w:t></w:r></w:p></w:tc>";
        }
        xml += "</w:tr>";
    }
    xml += "</w:tbl></w:body></w:document>";
    return xml;
}

[[nodiscard]] auto allocation_heavy_table_fixture_xml(std::size_t rows,
                                                       std::size_t columns)
    -> std::string {
    const auto large_value = large_cell_property_value();
    auto xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl><w:tblPr data-large=")"};
    xml += large_value;
    xml += R"("><w:tblW w:w="0" w:type="auto"/><w:tblLayout w:type="fixed"/>
    <w:tblLook w:val="04A0" w:firstRow="1" w:firstColumn="1" w:lastRow="0"
      w:lastColumn="0" w:noHBand="0" w:noVBand="1"/></w:tblPr>
  <w:tblGrid data-large=")";
    xml += large_value;
    xml += R"(">)";
    for (std::size_t column = 0U; column < columns; ++column) {
        xml += R"(<w:gridCol w:w="1200"/>)";
    }
    xml += "</w:tblGrid>";
    for (std::size_t row = 0U; row < rows; ++row) {
        xml += "<w:tr>";
        for (std::size_t column = 0U; column < columns; ++column) {
            xml += R"(<w:tc><w:tcPr><w:tcW w:w="1200" w:type="dxa"/></w:tcPr>)";
            xml += "<w:p><w:r><w:t>原始中文";
            xml += std::to_string(row);
            xml += "-";
            xml += std::to_string(column);
            xml += "</w:t></w:r></w:p></w:tc>";
        }
        xml += "</w:tr>";
    }
    xml += "</w:tbl></w:body></w:document>";
    return xml;
}

[[nodiscard]] auto separated_tables_fixture_xml() -> std::string {
    return R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl><w:tblGrid><w:gridCol w:w="1200"/></w:tblGrid><w:tr><w:tc>
      <w:tcPr/><w:p><w:r><w:t>anchor-table</w:t></w:r></w:p>
    </w:tc></w:tr></w:tbl>
    <w:p><w:r><w:t>middle-paragraph</w:t></w:r></w:p>
    <w:tbl><w:tblGrid><w:gridCol w:w="1200"/></w:tblGrid><w:tr><w:tc>
      <w:tcPr/><w:p><w:r><w:t>tail-table</w:t></w:r></w:p>
    </w:tc></w:tr></w:tbl>
    <w:sectPr/>
  </w:body>
</w:document>)";
}

[[nodiscard]] auto separated_cells_fixture_xml() -> std::string {
    return R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl><w:tblGrid>
    <w:gridCol w:w="1200"/><w:gridCol w:w="1200"/>
  </w:tblGrid><w:tr>
    <w:tc><w:tcPr/><w:p><w:r><w:t>anchor-cell</w:t></w:r></w:p></w:tc>
    <w:bookmarkStart w:id="7" w:name="between_cells"/>
    <w:bookmarkEnd w:id="7"/>
    <w:tc><w:tcPr/><w:p><w:r><w:t>tail-cell</w:t></w:r></w:p></w:tc>
  </w:tr></w:tbl></w:body>
</w:document>)";
}

[[nodiscard]] auto horizontally_merged_marker_fixture_xml() -> std::string {
    return R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl><w:tblGrid>
    <w:gridCol w:w="1200"/><w:gridCol w:w="1200"/>
    <w:gridCol w:w="1200"/><w:gridCol w:w="1200"/>
  </w:tblGrid><w:tr>
    <w:tc><w:tcPr/><w:p><w:r><w:t>anchor-cell</w:t></w:r></w:p></w:tc>
    <w:bookmarkStart w:id="8" w:name="after_merged_cell"/>
    <w:bookmarkEnd w:id="8"/>
    <w:tc><w:tcPr/><w:p><w:r><w:t>merge-one</w:t></w:r></w:p></w:tc>
    <w:tc><w:tcPr/><w:p><w:r><w:t>merge-two</w:t></w:r></w:p></w:tc>
    <w:tc><w:tcPr/><w:p><w:r><w:t>tail-cell</w:t></w:r></w:p></w:tc>
  </w:tr></w:tbl></w:body>
</w:document>)";
}

[[nodiscard]] auto first_table_cell_text(pugi::xml_node table)
    -> std::string_view {
    return table.child("w:tr")
        .child("w:tc")
        .child("w:p")
        .child("w:r")
        .child("w:t")
        .text()
        .get();
}

[[nodiscard]] auto tolerant_open_options()
    -> featherdoc::document_open_options {
    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    return options;
}

void record_controlled_allocation() {
    if (!allocation_tracking_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    const auto call =
        observed_allocation_calls.fetch_add(1U, std::memory_order_relaxed) +
        1U;
    if (call == allocation_failure_call.load(std::memory_order_relaxed)) {
        throw std::bad_alloc{};
    }
}

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY
[[nodiscard]] void *allocate_controlled(std::size_t size) {
    record_controlled_allocation();
    if (void *memory = std::malloc(size == 0U ? 1U : size)) {
        return memory;
    }
    throw std::bad_alloc{};
}
[[nodiscard]] void *allocate_controlled_aligned(std::size_t size,
                                                std::size_t alignment) {
    record_controlled_allocation();
    void *memory = nullptr;
    if (posix_memalign(&memory, alignment, size == 0U ? 1U : size) == 0) {
        return memory;
    }
    throw std::bad_alloc{};
}
#endif

void begin_allocation_tracking(std::size_t failure_call) noexcept {
    observed_allocation_calls.store(0U, std::memory_order_relaxed);
    allocation_failure_call.store(failure_call, std::memory_order_relaxed);
    allocation_tracking_enabled.store(true, std::memory_order_relaxed);
}

void end_allocation_tracking() noexcept {
    allocation_tracking_enabled.store(false, std::memory_order_relaxed);
    allocation_failure_call.store(0U, std::memory_order_relaxed);
}

class global_allocation_guard final {
  public:
    explicit global_allocation_guard(std::size_t failure_call) noexcept {
        begin_allocation_tracking(failure_call);
    }

    global_allocation_guard(const global_allocation_guard &) = delete;
    auto operator=(const global_allocation_guard &)
        -> global_allocation_guard & = delete;

    ~global_allocation_guard() { end_allocation_tracking(); }
};

[[nodiscard]] auto replacement_cell_text() -> std::string {
    auto text = std::string{"  中文😀\t第一行\r\n第二行  "};
    text += large_cell_property_value();
    text += "\n尾行中文  ";
    return text;
}

[[nodiscard]] auto normalized_replacement_cell_text() -> std::string {
    auto text = replacement_cell_text();
    const auto carriage_return = text.find("\r\n");
    if (carriage_return != std::string::npos) {
        text.erase(carriage_return, 1U);
    }
    return text;
}

enum class table_batch_text_operation {
    row = 0,
    rows,
    cell_block,
};

struct table_batch_text_inputs final {
    std::vector<std::string> row_texts;
    std::vector<std::vector<std::string>> rows_texts;
    std::vector<std::vector<std::string>> cell_block_texts;
    std::string normalized_large_text;
};

[[nodiscard]] auto make_table_batch_text_inputs()
    -> table_batch_text_inputs {
    const auto large_text = replacement_cell_text();
    return table_batch_text_inputs{
        {"行批量中文😀", large_text},
        {{"多行中文00", "多行中文01"}, {"多行中文10", large_text}},
        {{"区块中文01"}, {large_text}},
        normalized_replacement_cell_text(),
    };
}

[[nodiscard]] auto
table_batch_operation_name(table_batch_text_operation operation) -> const char * {
    switch (operation) {
    case table_batch_text_operation::row:
        return "TableRow::set_texts";
    case table_batch_text_operation::rows:
        return "Table::set_rows_texts";
    case table_batch_text_operation::cell_block:
        return "Table::set_cell_block_texts";
    }
    return "unknown";
}

[[nodiscard]] auto apply_table_batch_text_operation(
    featherdoc::Table &table, table_batch_text_operation operation,
    const table_batch_text_inputs &inputs) -> bool {
    switch (operation) {
    case table_batch_text_operation::row: {
        auto row = table.find_row(0U);
        return row.has_value() && row->set_texts(inputs.row_texts);
    }
    case table_batch_text_operation::rows:
        return table.set_rows_texts(0U, inputs.rows_texts);
    case table_batch_text_operation::cell_block:
        return table.set_cell_block_texts(0U, 1U,
                                          inputs.cell_block_texts);
    }
    return false;
}

struct table_cell_coordinate final {
    std::size_t row{};
    std::size_t column{};
};

[[nodiscard]] auto batch_target_coordinates(
    table_batch_text_operation operation)
    -> std::vector<table_cell_coordinate> {
    switch (operation) {
    case table_batch_text_operation::row:
        return {{0U, 0U}, {0U, 1U}};
    case table_batch_text_operation::rows:
        return {{0U, 0U}, {0U, 1U}, {1U, 0U}, {1U, 1U}};
    case table_batch_text_operation::cell_block:
        return {{0U, 1U}, {1U, 1U}};
    }
    return {};
}

struct table_cell_handle_snapshot final {
    featherdoc::TableCell cell;
    featherdoc::Paragraph paragraph;
    featherdoc::Run run;
};

[[nodiscard]] auto capture_table_cell_handle_snapshots(
    featherdoc::Table &table,
    const std::vector<table_cell_coordinate> &coordinates)
    -> std::vector<table_cell_handle_snapshot> {
    auto snapshots = std::vector<table_cell_handle_snapshot>{};
    snapshots.reserve(coordinates.size());

    for (const auto coordinate : coordinates) {
        auto cell = table.find_cell(coordinate.row, coordinate.column);
        REQUIRE(cell.has_value());
        auto paragraph = cell->paragraphs();
        auto run = paragraph.runs();
        snapshots.push_back({*cell, paragraph, run});
    }

    return snapshots;
}

void check_table_cell_handle_snapshots_valid(
    const std::vector<table_cell_handle_snapshot> &snapshots) {
    for (const auto &snapshot : snapshots) {
        CHECK(snapshot.cell.valid());
        CHECK(snapshot.paragraph.valid());
        CHECK(snapshot.run.valid());
    }
}

void check_table_cell_body_handles_retired(
    const std::vector<table_cell_handle_snapshot> &snapshots) {
    for (const auto &snapshot : snapshots) {
        CHECK(snapshot.cell.valid());
        CHECK_FALSE(snapshot.paragraph.valid());
        CHECK_FALSE(snapshot.run.valid());
    }
}

[[nodiscard]] auto expected_table_batch_texts(
    table_batch_text_operation operation,
    const table_batch_text_inputs &inputs)
    -> std::array<std::string_view, 4U> {
    switch (operation) {
    case table_batch_text_operation::row:
        return {inputs.row_texts[0U], inputs.normalized_large_text,
                "原始中文1-0", "原始中文1-1"};
    case table_batch_text_operation::rows:
        return {inputs.rows_texts[0U][0U], inputs.rows_texts[0U][1U],
                inputs.rows_texts[1U][0U], inputs.normalized_large_text};
    case table_batch_text_operation::cell_block:
        return {"原始中文0-0", inputs.cell_block_texts[0U][0U],
                "原始中文1-0", inputs.normalized_large_text};
    }
    return {};
}

void check_table_cell_texts(
    featherdoc::Table &table,
    const std::array<std::string_view, 4U> &expected_texts) {
    for (std::size_t row = 0U; row < 2U; ++row) {
        for (std::size_t column = 0U; column < 2U; ++column) {
            auto cell = table.find_cell(row, column);
            REQUIRE(cell.has_value());
            CHECK_EQ(cell->get_text(), expected_texts[row * 2U + column]);
        }
    }
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

TEST_CASE("batch XML retirement ignores empty and overlapping roots") {
    retirement_fixture fixture;
    const auto root_node = fixture.root();
    const auto left_node = root_node.child("left");
    const auto left_leaf_node = left_node.child("leaf");
    const auto right_node = root_node.child("right");
    const auto right_leaf_node = right_node.child("leaf");
    const auto keep_node = root_node.child("keep");

    auto root = fixture.tracked(root_node);
    auto left = fixture.tracked(left_node);
    auto left_leaf = fixture.tracked(left_leaf_node);
    auto right = fixture.tracked(right_node);
    auto right_leaf = fixture.tracked(right_leaf_node);
    auto keep = fixture.tracked(keep_node);

    const auto empty_roots = std::array<pugi::xml_node, 2U>{};
    CHECK(root.retire_subtrees(empty_roots));
    CHECK(root.alive());
    CHECK(left.alive());
    CHECK(left_leaf.alive());
    CHECK(right.alive());
    CHECK(right_leaf.alive());
    CHECK(keep.alive());

    const auto roots = std::array{
        left_node, pugi::xml_node{}, left_leaf_node, left_node, right_node};
    CHECK(root.retire_subtrees(roots));

    CHECK(root.alive());
    CHECK_FALSE(left.alive());
    CHECK_FALSE(left_leaf.alive());
    CHECK_FALSE(right.alive());
    CHECK_FALSE(right_leaf.alive());
    CHECK(keep.alive());
    CHECK_EQ(fixture.lifetime->node_epoch(root_node), 0U);
    CHECK_EQ(fixture.lifetime->node_epoch(left_node), 1U);
    CHECK_EQ(fixture.lifetime->node_epoch(left_leaf_node), 1U);
    CHECK_EQ(fixture.lifetime->node_epoch(right_node), 1U);
    CHECK_EQ(fixture.lifetime->node_epoch(right_leaf_node), 1U);
    CHECK_EQ(fixture.lifetime->node_epoch(keep_node), 0U);
}

TEST_CASE("batch XML retirement rejects an expired tracking anchor") {
    const auto roots = std::array{pugi::xml_node{}};
    const auto expired = featherdoc::detail::tracked_xml_node{};
    CHECK_FALSE(expired.retire_subtrees(roots));
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "batch XML retirement preserves every handle for every allocation failure") {
    auto successful_allocation_count = std::size_t{0U};
    {
        retirement_fixture baseline;
        const auto root_node = baseline.root();
        const auto left_node = root_node.child("left");
        const auto roots = std::array{
            left_node, left_node.child("leaf"), root_node.child("right")};
        const auto anchor = baseline.tracked(root_node);

        begin_allocation_tracking(0U);
        const auto retired = anchor.retire_subtrees(roots);
        end_allocation_tracking();
        REQUIRE(retired);
        successful_allocation_count =
            observed_allocation_calls.load(std::memory_order_relaxed);
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);

        retirement_fixture fixture;
        const auto root_node = fixture.root();
        const auto left_node = root_node.child("left");
        const auto left_leaf_node = left_node.child("leaf");
        const auto right_node = root_node.child("right");
        const auto right_leaf_node = right_node.child("leaf");
        const auto keep_node = root_node.child("keep");
        const auto roots =
            std::array{left_node, left_leaf_node, left_node, right_node};

        auto root = fixture.tracked(root_node);
        auto left = fixture.tracked(left_node);
        auto left_leaf = fixture.tracked(left_leaf_node);
        auto right = fixture.tracked(right_node);
        auto right_leaf = fixture.tracked(right_leaf_node);
        auto keep = fixture.tracked(keep_node);

        auto observed_bad_alloc = false;
        begin_allocation_tracking(failure_call);
        try {
            (void)root.retire_subtrees(roots);
        } catch (const std::bad_alloc &) {
            observed_bad_alloc = true;
        } catch (...) {
            end_allocation_tracking();
            throw;
        }
        end_allocation_tracking();

        REQUIRE(observed_bad_alloc);
        CHECK(root.alive());
        CHECK(left.alive());
        CHECK(left_leaf.alive());
        CHECK(right.alive());
        CHECK(right_leaf.alive());
        CHECK(keep.alive());
        CHECK_EQ(fixture.lifetime->node_epoch(root_node), 0U);
        CHECK_EQ(fixture.lifetime->node_epoch(left_node), 0U);
        CHECK_EQ(fixture.lifetime->node_epoch(left_leaf_node), 0U);
        CHECK_EQ(fixture.lifetime->node_epoch(right_node), 0U);
        CHECK_EQ(fixture.lifetime->node_epoch(right_leaf_node), 0U);
        CHECK_EQ(fixture.lifetime->node_epoch(keep_node), 0U);

        REQUIRE(root.retire_subtrees(roots));
        CHECK(root.alive());
        CHECK_FALSE(left.alive());
        CHECK_FALSE(left_leaf.alive());
        CHECK_FALSE(right.alive());
        CHECK_FALSE(right_leaf.alive());
        CHECK(keep.alive());
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "horizontal merge leaves DOM and handles unchanged for every batch "
    "retirement allocation failure") {
    auto successful_allocation_count = std::size_t{0U};
    {
        featherdoc::Document baseline;
        REQUIRE_FALSE(baseline.create_empty());
        auto anchor = baseline.append_table(1U, 4U).rows().cells();
        REQUIRE(anchor.valid());

        begin_allocation_tracking(0U);
        const auto merged = anchor.merge_right(2U);
        end_allocation_tracking();
        REQUIRE(merged);
        successful_allocation_count =
            observed_allocation_calls.load(std::memory_order_relaxed);
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);

        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());
        auto table = document.append_table(1U, 4U);
        REQUIRE(table.valid());
        auto row = table.rows();
        REQUIRE(row.valid());
        auto anchor = row.cells();
        REQUIRE(anchor.set_text("anchor"));
        auto removed_first = anchor;
        removed_first.next();
        REQUIRE(removed_first.set_text("removed first"));
        auto removed_first_paragraph = removed_first.paragraphs();
        auto removed_first_run = removed_first_paragraph.runs();
        auto removed_second = removed_first;
        removed_second.next();
        REQUIRE(removed_second.set_text("removed second"));
        auto removed_second_paragraph = removed_second.paragraphs();
        auto removed_second_run = removed_second_paragraph.runs();
        auto unaffected = removed_second;
        unaffected.next();
        REQUIRE(unaffected.set_text("unaffected"));

        auto observed_bad_alloc = false;
        begin_allocation_tracking(failure_call);
        try {
            (void)anchor.merge_right(2U);
        } catch (const std::bad_alloc &) {
            observed_bad_alloc = true;
        } catch (...) {
            end_allocation_tracking();
            throw;
        }
        end_allocation_tracking();

        REQUIRE(observed_bad_alloc);
        CHECK_EQ(anchor.column_span(), 1U);
        CHECK_EQ(anchor.get_text(), "anchor");
        CHECK(removed_first.valid());
        CHECK(removed_first_paragraph.valid());
        CHECK(removed_first_run.valid());
        CHECK_EQ(removed_first.get_text(), "removed first");
        CHECK(removed_second.valid());
        CHECK(removed_second_paragraph.valid());
        CHECK(removed_second_run.valid());
        CHECK_EQ(removed_second.get_text(), "removed second");
        CHECK(unaffected.valid());
        CHECK_EQ(unaffected.get_text(), "unaffected");
        auto remaining_cell_count = std::size_t{0U};
        for (auto cell = row.cells(); cell.valid(); cell.next()) {
            ++remaining_cell_count;
        }
        CHECK_EQ(remaining_cell_count, 4U);

        REQUIRE(anchor.merge_right(2U));
        CHECK_FALSE(removed_first.valid());
        CHECK_FALSE(removed_first_paragraph.valid());
        CHECK_FALSE(removed_first_run.valid());
        CHECK_FALSE(removed_second.valid());
        CHECK_FALSE(removed_second_paragraph.valid());
        CHECK_FALSE(removed_second_run.valid());
        CHECK(unaffected.valid());
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "vertical merge leaves DOM and handles unchanged for every batch "
    "retirement allocation failure") {
    auto successful_allocation_count = std::size_t{0U};
    {
        featherdoc::Document baseline;
        REQUIRE_FALSE(baseline.create_empty());
        auto anchor = baseline.append_table(3U, 2U).rows().cells();
        REQUIRE(anchor.valid());

        begin_allocation_tracking(0U);
        const auto merged = anchor.merge_down(2U);
        end_allocation_tracking();
        REQUIRE(merged);
        successful_allocation_count =
            observed_allocation_calls.load(std::memory_order_relaxed);
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);

        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());
        auto table = document.append_table(3U, 2U);
        REQUIRE(table.valid());
        auto first_row = table.rows();
        auto second_row = first_row;
        second_row.next();
        auto third_row = second_row;
        third_row.next();
        REQUIRE(first_row.valid());
        REQUIRE(second_row.valid());
        REQUIRE(third_row.valid());

        auto anchor = first_row.cells();
        REQUIRE(anchor.set_text("anchor"));
        auto second_target = second_row.cells();
        REQUIRE(second_target.set_text("second target"));
        auto second_paragraph = second_target.paragraphs();
        auto second_run = second_paragraph.runs();
        auto third_target = third_row.cells();
        REQUIRE(third_target.set_text("third target"));
        auto third_paragraph = third_target.paragraphs();
        auto third_run = third_paragraph.runs();

        auto observed_bad_alloc = false;
        begin_allocation_tracking(failure_call);
        try {
            (void)anchor.merge_down(2U);
        } catch (const std::bad_alloc &) {
            observed_bad_alloc = true;
        } catch (...) {
            end_allocation_tracking();
            throw;
        }
        end_allocation_tracking();

        REQUIRE(observed_bad_alloc);
        CHECK_EQ(anchor.vertical_merge(), featherdoc::cell_vertical_merge::none);
        CHECK_EQ(anchor.get_text(), "anchor");
        CHECK_EQ(second_target.vertical_merge(),
                 featherdoc::cell_vertical_merge::none);
        CHECK(second_target.valid());
        CHECK(second_paragraph.valid());
        CHECK(second_run.valid());
        CHECK_EQ(second_target.get_text(), "second target");
        CHECK_EQ(third_target.vertical_merge(),
                 featherdoc::cell_vertical_merge::none);
        CHECK(third_target.valid());
        CHECK(third_paragraph.valid());
        CHECK(third_run.valid());
        CHECK_EQ(third_target.get_text(), "third target");

        REQUIRE(anchor.merge_down(2U));
        CHECK_FALSE(second_paragraph.valid());
        CHECK_FALSE(second_run.valid());
        CHECK_FALSE(third_paragraph.valid());
        CHECK_FALSE(third_run.valid());
        CHECK_EQ(anchor.vertical_merge(),
                 featherdoc::cell_vertical_merge::restart);
        CHECK_EQ(second_target.vertical_merge(),
                 featherdoc::cell_vertical_merge::continue_merge);
        CHECK_EQ(third_target.vertical_merge(),
                 featherdoc::cell_vertical_merge::continue_merge);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "horizontal merge preserves serialized XML for every pugixml allocation "
    "failure") {
    const auto fixture_xml = table_fixture_xml(1U, 4U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("横向基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto anchor = document.tables().rows().cells();
        REQUIRE(anchor.valid());

        auto merged = false;
        {
            pugi_allocator_guard guard;
            merged = anchor.merge_right(2U);
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(merged);
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t current_failure_call = 1U;
         current_failure_call <= successful_allocation_count;
         ++current_failure_call) {
        CAPTURE(current_failure_call);
        CAPTURE(successful_allocation_count);

        scoped_test_path path{
            make_test_path("横向失败", current_failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto row = document.tables().rows();
        REQUIRE(row.valid());
        auto anchor = row.cells();
        auto removed_first = anchor;
        removed_first.next();
        auto removed_first_paragraph = removed_first.paragraphs();
        auto removed_first_run = removed_first_paragraph.runs();
        auto removed_second = removed_first;
        removed_second.next();
        auto removed_second_paragraph = removed_second.paragraphs();
        auto removed_second_run = removed_second_paragraph.runs();

        auto merged = true;
        {
            pugi_allocator_guard guard;
            pugi_failure_call = current_failure_call;
            merged = anchor.merge_right(2U);
        }

        REQUIRE_FALSE(merged);
        CHECK_EQ(anchor.column_span(), 1U);
        CHECK_EQ(anchor.get_text(), "锚点中文");
        CHECK(removed_first.valid());
        CHECK(removed_first_paragraph.valid());
        CHECK(removed_first_run.valid());
        CHECK_EQ(removed_first.get_text(), "不受影响");
        CHECK(removed_second.valid());
        CHECK(removed_second_paragraph.valid());
        CHECK(removed_second_run.valid());
        CHECK_EQ(removed_second.get_text(), "不受影响");
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(anchor.merge_right(2U));
        CHECK_FALSE(removed_first.valid());
        CHECK_FALSE(removed_first_paragraph.valid());
        CHECK_FALSE(removed_first_run.valid());
        CHECK_FALSE(removed_second.valid());
        CHECK_FALSE(removed_second_paragraph.valid());
        CHECK_FALSE(removed_second_run.valid());
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "vertical merge preserves serialized XML for every pugixml allocation "
    "failure") {
    const auto fixture_xml = table_fixture_xml(3U, 2U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("纵向基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto anchor = document.tables().rows().cells();
        REQUIRE(anchor.valid());

        auto merged = false;
        {
            pugi_allocator_guard guard;
            merged = anchor.merge_down(2U);
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(merged);
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t current_failure_call = 1U;
         current_failure_call <= successful_allocation_count;
         ++current_failure_call) {
        CAPTURE(current_failure_call);
        CAPTURE(successful_allocation_count);

        scoped_test_path path{
            make_test_path("纵向失败", current_failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto first_row = document.tables().rows();
        auto second_row = first_row;
        second_row.next();
        auto third_row = second_row;
        third_row.next();
        auto anchor = first_row.cells();
        auto second_target = second_row.cells();
        auto second_paragraph = second_target.paragraphs();
        auto second_run = second_paragraph.runs();
        auto third_target = third_row.cells();
        auto third_paragraph = third_target.paragraphs();
        auto third_run = third_paragraph.runs();

        auto merged = true;
        {
            pugi_allocator_guard guard;
            pugi_failure_call = current_failure_call;
            merged = anchor.merge_down(2U);
        }

        REQUIRE_FALSE(merged);
        CHECK_EQ(anchor.vertical_merge(), featherdoc::cell_vertical_merge::none);
        CHECK_EQ(anchor.get_text(), "锚点中文");
        CHECK_EQ(second_target.vertical_merge(),
                 featherdoc::cell_vertical_merge::none);
        CHECK(second_target.valid());
        CHECK(second_paragraph.valid());
        CHECK(second_run.valid());
        CHECK_EQ(second_target.get_text(), "待替换中文");
        CHECK_EQ(third_target.vertical_merge(),
                 featherdoc::cell_vertical_merge::none);
        CHECK(third_target.valid());
        CHECK(third_paragraph.valid());
        CHECK(third_run.valid());
        CHECK_EQ(third_target.get_text(), "待替换中文");
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(anchor.merge_down(2U));
        CHECK_FALSE(second_paragraph.valid());
        CHECK_FALSE(second_run.valid());
        CHECK_FALSE(third_paragraph.valid());
        CHECK_FALSE(third_run.valid());
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "row removal promotion preserves serialized XML for every pugixml "
    "allocation failure") {
    const auto fixture_xml = table_fixture_xml(3U, 2U, true);
    const auto promoted_text =
        std::string{"锚点中文"} + large_cell_property_value();
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("删行基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto first_row = document.tables().rows();
        REQUIRE(first_row.valid());

        auto removed = false;
        {
            pugi_allocator_guard guard;
            removed = first_row.remove();
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(removed);
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t current_failure_call = 1U;
         current_failure_call <= successful_allocation_count;
         ++current_failure_call) {
        CAPTURE(current_failure_call);
        CAPTURE(successful_allocation_count);

        scoped_test_path path{
            make_test_path("删行失败", current_failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto first_row = table.rows();
        auto removed_row = first_row;
        auto source_cell = first_row.cells();
        auto source_paragraph = source_cell.paragraphs();
        auto source_run = source_paragraph.runs();
        auto promoted_row = first_row;
        promoted_row.next();
        auto promoted_cell = promoted_row.cells();
        auto promoted_paragraph = promoted_cell.paragraphs();
        auto promoted_run = promoted_paragraph.runs();

        auto removed = true;
        {
            pugi_allocator_guard guard;
            pugi_failure_call = current_failure_call;
            removed = first_row.remove();
        }

        REQUIRE_FALSE(removed);
        CHECK(removed_row.valid());
        CHECK(source_cell.valid());
        CHECK(source_paragraph.valid());
        CHECK(source_run.valid());
        CHECK_EQ(source_cell.get_text(), promoted_text);
        CHECK(promoted_row.valid());
        CHECK(promoted_cell.valid());
        CHECK(promoted_paragraph.valid());
        CHECK(promoted_run.valid());
        CHECK_EQ(promoted_cell.get_text(), "待替换中文");
        CHECK_EQ(promoted_cell.vertical_merge(),
                 featherdoc::cell_vertical_merge::continue_merge);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(first_row.remove());
        CHECK_FALSE(removed_row.valid());
        CHECK_FALSE(source_cell.valid());
        CHECK_FALSE(source_paragraph.valid());
        CHECK_FALSE(source_run.valid());
        CHECK(promoted_row.valid());
        CHECK(promoted_cell.valid());
        CHECK_FALSE(promoted_paragraph.valid());
        CHECK_FALSE(promoted_run.valid());
        CHECK_EQ(promoted_cell.get_text(), promoted_text);
        CHECK_EQ(promoted_cell.vertical_merge(),
                 featherdoc::cell_vertical_merge::restart);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "tolerant horizontal merge preserves malformed fixed table state for "
    "every global allocation failure") {
    const auto fixture_xml = malformed_fixed_table_fixture_xml(1U, 4U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("容错横向全局基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open(tolerant_open_options()));
        auto anchor = document.tables().rows().cells();
        REQUIRE(anchor.valid());

        begin_allocation_tracking(0U);
        const auto merged = anchor.merge_right(2U);
        end_allocation_tracking();
        REQUIRE(merged);
        successful_allocation_count =
            observed_allocation_calls.load(std::memory_order_relaxed);
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);

        scoped_test_path path{
            make_test_path("容错横向全局失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open(tolerant_open_options()));
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto row = document.tables().rows();
        auto anchor = row.cells();
        auto removed_first = anchor;
        removed_first.next();
        auto removed_first_paragraph = removed_first.paragraphs();
        auto removed_first_run = removed_first_paragraph.runs();
        auto removed_second = removed_first;
        removed_second.next();
        auto removed_second_paragraph = removed_second.paragraphs();
        auto removed_second_run = removed_second_paragraph.runs();
        auto unaffected = removed_second;
        unaffected.next();
        auto unaffected_paragraph = unaffected.paragraphs();
        auto unaffected_run = unaffected_paragraph.runs();

        auto observed_bad_alloc = false;
        begin_allocation_tracking(failure_call);
        try {
            (void)anchor.merge_right(2U);
        } catch (const std::bad_alloc &) {
            observed_bad_alloc = true;
        } catch (...) {
            end_allocation_tracking();
            throw;
        }
        end_allocation_tracking();

        REQUIRE(observed_bad_alloc);
        CHECK_EQ(anchor.column_span(), 1U);
        CHECK(removed_first.valid());
        CHECK(removed_first_paragraph.valid());
        CHECK(removed_first_run.valid());
        CHECK(removed_second.valid());
        CHECK(removed_second_paragraph.valid());
        CHECK(removed_second_run.valid());
        CHECK(unaffected.valid());
        CHECK(unaffected_paragraph.valid());
        CHECK(unaffected_run.valid());
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(anchor.merge_right(2U));
        CHECK_FALSE(removed_first.valid());
        CHECK_FALSE(removed_first_paragraph.valid());
        CHECK_FALSE(removed_first_run.valid());
        CHECK_FALSE(removed_second.valid());
        CHECK_FALSE(removed_second_paragraph.valid());
        CHECK_FALSE(removed_second_run.valid());
        CHECK(unaffected.valid());
        CHECK(unaffected_paragraph.valid());
        CHECK(unaffected_run.valid());
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "tolerant column removal preserves malformed fixed table state for every "
    "global allocation failure") {
    const auto fixture_xml = malformed_fixed_table_fixture_xml(2U, 3U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("容错删列全局基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open(tolerant_open_options()));
        auto target = document.tables().rows().cells();
        REQUIRE(target.valid());

        begin_allocation_tracking(0U);
        const auto removed = target.remove();
        end_allocation_tracking();
        REQUIRE(removed);
        successful_allocation_count =
            observed_allocation_calls.load(std::memory_order_relaxed);
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);

        scoped_test_path path{
            make_test_path("容错删列全局失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open(tolerant_open_options()));
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto first_row = table.rows();
        auto target = first_row.cells();
        auto removed_current = target;
        auto removed_current_paragraph = removed_current.paragraphs();
        auto removed_current_run = removed_current_paragraph.runs();
        auto first_survivor = removed_current;
        first_survivor.next();
        auto first_survivor_paragraph = first_survivor.paragraphs();
        auto second_row = first_row;
        second_row.next();
        auto other_removed = second_row.cells();
        auto other_removed_paragraph = other_removed.paragraphs();
        auto other_removed_run = other_removed_paragraph.runs();
        auto other_survivor = other_removed;
        other_survivor.next();
        auto other_survivor_paragraph = other_survivor.paragraphs();

        auto observed_bad_alloc = false;
        begin_allocation_tracking(failure_call);
        try {
            (void)target.remove();
        } catch (const std::bad_alloc &) {
            observed_bad_alloc = true;
        } catch (...) {
            end_allocation_tracking();
            throw;
        }
        end_allocation_tracking();

        REQUIRE(observed_bad_alloc);
        CHECK(removed_current.valid());
        CHECK(removed_current_paragraph.valid());
        CHECK(removed_current_run.valid());
        CHECK(other_removed.valid());
        CHECK(other_removed_paragraph.valid());
        CHECK(other_removed_run.valid());
        CHECK(first_survivor.valid());
        CHECK(first_survivor_paragraph.valid());
        CHECK(other_survivor.valid());
        CHECK(other_survivor_paragraph.valid());
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(target.remove());
        CHECK_FALSE(removed_current.valid());
        CHECK_FALSE(removed_current_paragraph.valid());
        CHECK_FALSE(removed_current_run.valid());
        CHECK_FALSE(other_removed.valid());
        CHECK_FALSE(other_removed_paragraph.valid());
        CHECK_FALSE(other_removed_run.valid());
        CHECK(target.valid());
        CHECK(first_survivor.valid());
        CHECK(first_survivor_paragraph.valid());
        CHECK(other_survivor.valid());
        CHECK(other_survivor_paragraph.valid());
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "tolerant horizontal merge rolls back malformed fixed table for every "
    "pugixml allocation failure") {
    const auto fixture_xml = malformed_fixed_table_fixture_xml(1U, 63U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("容错横向XML基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open(tolerant_open_options()));
        auto anchor = document.tables().rows().cells();
        REQUIRE(anchor.valid());
        auto merged = false;
        {
            pugi_allocator_guard guard;
            merged = anchor.merge_right(2U);
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(merged);
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);

        scoped_test_path path{
            make_test_path("容错横向XML失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open(tolerant_open_options()));
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto row = document.tables().rows();
        auto anchor = row.cells();
        auto removed_first = anchor;
        removed_first.next();
        auto removed_first_paragraph = removed_first.paragraphs();
        auto removed_first_run = removed_first_paragraph.runs();
        auto removed_second = removed_first;
        removed_second.next();
        auto removed_second_paragraph = removed_second.paragraphs();
        auto removed_second_run = removed_second_paragraph.runs();
        auto unaffected = removed_second;
        unaffected.next();
        auto unaffected_paragraph = unaffected.paragraphs();

        auto merged = true;
        {
            pugi_allocator_guard guard;
            pugi_failure_call = failure_call;
            merged = anchor.merge_right(2U);
        }

        REQUIRE_FALSE(merged);
        CHECK_EQ(anchor.column_span(), 1U);
        CHECK(removed_first.valid());
        CHECK(removed_first_paragraph.valid());
        CHECK(removed_first_run.valid());
        CHECK(removed_second.valid());
        CHECK(removed_second_paragraph.valid());
        CHECK(removed_second_run.valid());
        CHECK(unaffected.valid());
        CHECK(unaffected_paragraph.valid());
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(anchor.merge_right(2U));
        CHECK_FALSE(removed_first.valid());
        CHECK_FALSE(removed_first_paragraph.valid());
        CHECK_FALSE(removed_first_run.valid());
        CHECK_FALSE(removed_second.valid());
        CHECK_FALSE(removed_second_paragraph.valid());
        CHECK_FALSE(removed_second_run.valid());
        CHECK(unaffected.valid());
        CHECK(unaffected_paragraph.valid());
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "tolerant column removal rolls back malformed fixed table for every "
    "pugixml allocation failure") {
    const auto fixture_xml = malformed_fixed_table_fixture_xml(2U, 63U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("容错删列XML基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open(tolerant_open_options()));
        auto target = document.tables().rows().cells();
        REQUIRE(target.valid());
        auto removed = false;
        {
            pugi_allocator_guard guard;
            removed = target.remove();
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(removed);
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);

        scoped_test_path path{
            make_test_path("容错删列XML失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open(tolerant_open_options()));
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto first_row = table.rows();
        auto target = first_row.cells();
        auto removed_current = target;
        auto removed_current_paragraph = removed_current.paragraphs();
        auto removed_current_run = removed_current_paragraph.runs();
        auto first_survivor = removed_current;
        first_survivor.next();
        auto first_survivor_paragraph = first_survivor.paragraphs();
        auto second_row = first_row;
        second_row.next();
        auto other_removed = second_row.cells();
        auto other_removed_paragraph = other_removed.paragraphs();
        auto other_removed_run = other_removed_paragraph.runs();
        auto other_survivor = other_removed;
        other_survivor.next();
        auto other_survivor_paragraph = other_survivor.paragraphs();

        auto removed = true;
        {
            pugi_allocator_guard guard;
            pugi_failure_call = failure_call;
            removed = target.remove();
        }

        REQUIRE_FALSE(removed);
        CHECK(removed_current.valid());
        CHECK(removed_current_paragraph.valid());
        CHECK(removed_current_run.valid());
        CHECK(other_removed.valid());
        CHECK(other_removed_paragraph.valid());
        CHECK(other_removed_run.valid());
        CHECK(first_survivor.valid());
        CHECK(first_survivor_paragraph.valid());
        CHECK(other_survivor.valid());
        CHECK(other_survivor_paragraph.valid());
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(target.remove());
        CHECK_FALSE(removed_current.valid());
        CHECK_FALSE(removed_current_paragraph.valid());
        CHECK_FALSE(removed_current_run.valid());
        CHECK_FALSE(other_removed.valid());
        CHECK_FALSE(other_removed_paragraph.valid());
        CHECK_FALSE(other_removed_run.valid());
        CHECK(target.valid());
        CHECK(first_survivor.valid());
        CHECK(first_survivor_paragraph.valid());
        CHECK(other_survivor.valid());
        CHECK(other_survivor_paragraph.valid());
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "append cell preserves table DOM and existing handles for every pugixml "
    "allocation failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("追加单元格XML基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto row = document.tables().rows();
        auto appended = featherdoc::TableCell{};
        {
            pugi_allocator_guard guard;
            appended = row.append_cell();
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(appended.valid());
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{
            make_test_path("追加单元格XML失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto old_cell = row.cells();
        auto old_paragraph = old_cell.paragraphs();
        auto old_run = old_paragraph.runs();
        auto appended = featherdoc::TableCell{};
        {
            pugi_allocator_guard guard;
            pugi_failure_call = failure_call;
            appended = row.append_cell();
        }

        REQUIRE_FALSE(appended.valid());
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(old_cell.valid());
        CHECK(old_paragraph.valid());
        CHECK(old_run.valid());
        CHECK_EQ(old_cell.get_text(), "原始中文0-0");
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        appended = row.append_cell();
        REQUIRE(appended.valid());
        CHECK(old_cell.valid());
        CHECK(old_paragraph.valid());
        CHECK(old_run.valid());
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "append row preserves table DOM and existing handles for every pugixml "
    "allocation failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("追加表格行XML基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto appended = featherdoc::TableRow{};
        {
            pugi_allocator_guard guard;
            appended = table.append_row(3U);
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(appended.valid());
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{
            make_test_path("追加表格行XML失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto old_row = table.rows();
        auto old_cell = old_row.cells();
        auto old_paragraph = old_cell.paragraphs();
        auto old_run = old_paragraph.runs();
        auto appended = featherdoc::TableRow{};
        {
            pugi_allocator_guard guard;
            pugi_failure_call = failure_call;
            appended = table.append_row(3U);
        }

        REQUIRE_FALSE(appended.valid());
        CHECK(table.valid());
        CHECK(old_row.valid());
        CHECK(old_cell.valid());
        CHECK(old_paragraph.valid());
        CHECK(old_run.valid());
        CHECK_EQ(old_cell.get_text(), "原始中文0-0");
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        appended = table.append_row(3U);
        REQUIRE(appended.valid());
        CHECK(old_row.valid());
        CHECK(old_cell.valid());
        CHECK(old_paragraph.valid());
        CHECK(old_run.valid());
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "cell insertion before and after preserves DOM and old handles for every "
    "pugixml allocation failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 3U);
    for (const auto insert_after : {false, true}) {
        CAPTURE(insert_after);
        auto successful_allocation_count = std::size_t{0U};
        {
            scoped_test_path path{make_test_path(
                insert_after ? "后插单元格XML基线" : "前插单元格XML基线",
                0U)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            auto anchor = document.tables().rows().cells();
            auto inserted = featherdoc::TableCell{};
            {
                pugi_allocator_guard guard;
                inserted = insert_after ? anchor.insert_cell_after()
                                        : anchor.insert_cell_before();
                successful_allocation_count = pugi_allocation_calls;
            }
            REQUIRE(inserted.valid());
            REQUIRE_GT(successful_allocation_count, 0U);
        }

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            scoped_test_path path{make_test_path(
                insert_after ? "后插单元格XML失败" : "前插单元格XML失败",
                failure_call)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            REQUIRE_FALSE(document.save());
            const auto xml_before =
                read_test_docx_entry(path.path(), test_document_xml_entry);

            auto row = document.tables().rows();
            auto anchor = row.cells();
            auto old_anchor = anchor;
            auto old_paragraph = old_anchor.paragraphs();
            auto old_run = old_paragraph.runs();
            auto unaffected = old_anchor;
            unaffected.next();
            auto unaffected_paragraph = unaffected.paragraphs();
            auto inserted = featherdoc::TableCell{};
            {
                pugi_allocator_guard guard;
                pugi_failure_call = failure_call;
                inserted = insert_after ? anchor.insert_cell_after()
                                        : anchor.insert_cell_before();
            }

            REQUIRE_FALSE(inserted.valid());
            CHECK(anchor.valid());
            CHECK(old_anchor.valid());
            CHECK(old_paragraph.valid());
            CHECK(old_run.valid());
            CHECK(unaffected.valid());
            CHECK(unaffected_paragraph.valid());
            CHECK_EQ(old_anchor.get_text(), "原始中文0-0");
            REQUIRE_FALSE(document.save());
            CHECK_EQ(read_test_docx_entry(path.path(),
                                          test_document_xml_entry),
                     xml_before);

            inserted = insert_after ? anchor.insert_cell_after()
                                    : anchor.insert_cell_before();
            REQUIRE(inserted.valid());
            CHECK(old_anchor.valid());
            CHECK(old_paragraph.valid());
            CHECK(old_run.valid());
            CHECK(unaffected.valid());
            CHECK(unaffected_paragraph.valid());
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table insertion before and after preserves DOM and old handles for every "
    "pugixml allocation failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    for (const auto insert_after : {false, true}) {
        CAPTURE(insert_after);
        auto successful_allocation_count = std::size_t{0U};
        {
            scoped_test_path path{make_test_path(
                insert_after ? "后插表格XML基线" : "前插表格XML基线", 0U)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            auto table = document.tables();
            auto inserted = featherdoc::Table{};
            {
                pugi_allocator_guard guard;
                inserted = insert_after ? table.insert_table_after(8U, 8U)
                                        : table.insert_table_before(8U, 8U);
                successful_allocation_count = pugi_allocation_calls;
            }
            REQUIRE(inserted.valid());
            REQUIRE_GT(successful_allocation_count, 0U);
        }

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            scoped_test_path path{make_test_path(
                insert_after ? "后插表格XML失败" : "前插表格XML失败",
                failure_call)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            REQUIRE_FALSE(document.save());
            const auto xml_before =
                read_test_docx_entry(path.path(), test_document_xml_entry);

            auto table = document.tables();
            auto old_table = table;
            auto old_row = old_table.rows();
            auto old_cell = old_row.cells();
            auto old_paragraph = old_cell.paragraphs();
            auto old_run = old_paragraph.runs();
            auto inserted = featherdoc::Table{};
            {
                pugi_allocator_guard guard;
                pugi_failure_call = failure_call;
                inserted = insert_after ? table.insert_table_after(8U, 8U)
                                        : table.insert_table_before(8U, 8U);
            }

            REQUIRE_FALSE(inserted.valid());
            CHECK(table.valid());
            CHECK(old_table.valid());
            CHECK(old_row.valid());
            CHECK(old_cell.valid());
            CHECK(old_paragraph.valid());
            CHECK(old_run.valid());
            CHECK_EQ(old_cell.get_text(), "原始中文0-0");
            REQUIRE_FALSE(document.save());
            CHECK_EQ(read_test_docx_entry(path.path(),
                                          test_document_xml_entry),
                     xml_before);

            inserted = insert_after ? table.insert_table_after(8U, 8U)
                                    : table.insert_table_before(8U, 8U);
            REQUIRE(inserted.valid());
            CHECK(old_table.valid());
            CHECK(old_row.valid());
            CHECK(old_cell.valid());
            CHECK(old_paragraph.valid());
            CHECK(old_run.valid());
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table paragraph insertion rolls back for every pugixml allocation "
    "failure and preserves empty text semantics") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    const auto inserted_text = replacement_cell_text();
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("表后段落XML基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto inserted = featherdoc::Paragraph{};
        {
            pugi_allocator_guard guard;
            inserted = table.insert_paragraph_after(
                inserted_text, featherdoc::formatting_flag::bold);
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(inserted.valid());
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{make_test_path("表后段落XML失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto old_table = table;
        auto old_row = old_table.rows();
        auto old_cell = old_row.cells();
        auto old_paragraph = old_cell.paragraphs();
        auto old_run = old_paragraph.runs();
        auto inserted = featherdoc::Paragraph{};
        {
            pugi_allocator_guard guard;
            pugi_failure_call = failure_call;
            inserted = table.insert_paragraph_after(
                inserted_text, featherdoc::formatting_flag::bold);
        }

        REQUIRE_FALSE(inserted.valid());
        CHECK(table.valid());
        CHECK(old_table.valid());
        CHECK(old_row.valid());
        CHECK(old_cell.valid());
        CHECK(old_paragraph.valid());
        CHECK(old_run.valid());
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        inserted = table.insert_paragraph_after(
            inserted_text, featherdoc::formatting_flag::bold);
        REQUIRE(inserted.valid());
        CHECK(old_table.valid());
        CHECK(old_row.valid());
        CHECK(old_cell.valid());
        CHECK(old_paragraph.valid());
        CHECK(old_run.valid());
    }

    scoped_test_path empty_path{make_test_path("表后空段落语义", 0U)};
    write_test_docx(empty_path.path(), fixture_xml);
    featherdoc::Document empty_document(empty_path.path());
    REQUIRE_FALSE(empty_document.open());
    auto empty_paragraph =
        empty_document.tables().insert_paragraph_after(std::string{});
    REQUIRE(empty_paragraph.valid());
    CHECK_FALSE(empty_paragraph.runs().has_next());
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table paragraph insertion rolls back for every global allocation "
    "failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    const auto inserted_text = replacement_cell_text();
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("表后段落全局基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto inserted = featherdoc::Paragraph{};
        {
            global_allocation_guard guard{0U};
            inserted = table.insert_paragraph_after(
                inserted_text, featherdoc::formatting_flag::bold);
        }
        REQUIRE(inserted.valid());
        successful_allocation_count =
            observed_allocation_calls.load(std::memory_order_relaxed);
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{
            make_test_path("表后段落全局失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto old_row = table.rows();
        auto old_cell = old_row.cells();
        auto old_paragraph = old_cell.paragraphs();
        auto old_run = old_paragraph.runs();
        auto inserted = featherdoc::Paragraph{};
        auto observed_bad_alloc = false;
        try {
            global_allocation_guard guard{failure_call};
            inserted = table.insert_paragraph_after(
                inserted_text, featherdoc::formatting_flag::bold);
        } catch (const std::bad_alloc &) {
            observed_bad_alloc = true;
        }

        CHECK_FALSE(inserted.valid());
        CHECK((observed_bad_alloc || !inserted.valid()));
        CHECK(table.valid());
        CHECK(old_row.valid());
        CHECK(old_cell.valid());
        CHECK(old_paragraph.valid());
        CHECK(old_run.valid());
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table cell text replacement is atomic for every pugixml allocation "
    "failure and preserves UTF-8 whitespace and newlines") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    const auto replacement_text = replacement_cell_text();
    const auto expected_text = normalized_replacement_cell_text();
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("单元格文本XML基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto cell = document.tables().rows().cells();
        auto replaced = false;
        {
            pugi_allocator_guard guard;
            replaced = cell.set_text(replacement_text);
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(replaced);
        CHECK_EQ(cell.get_text(), expected_text);
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{make_test_path("单元格文本XML失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto cell = row.cells();
        auto old_paragraph = cell.paragraphs();
        auto old_run = old_paragraph.runs();
        auto replaced = true;
        {
            pugi_allocator_guard guard;
            pugi_failure_call = failure_call;
            replaced = cell.set_text(replacement_text);
        }

        REQUIRE_FALSE(replaced);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(cell.valid());
        CHECK(old_paragraph.valid());
        CHECK(old_run.valid());
        CHECK_EQ(cell.get_text(), "原始中文0-0");
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(cell.set_text(replacement_text));
        CHECK(cell.valid());
        CHECK_FALSE(old_paragraph.valid());
        CHECK_FALSE(old_run.valid());
        CHECK_EQ(cell.get_text(), expected_text);
    }

    scoped_test_path empty_path{make_test_path("单元格空文本语义", 0U)};
    write_test_docx(empty_path.path(), fixture_xml);
    featherdoc::Document empty_document(empty_path.path());
    REQUIRE_FALSE(empty_document.open());
    auto empty_cell = empty_document.tables().rows().cells();
    auto retired_paragraph = empty_cell.paragraphs();
    auto retired_run = retired_paragraph.runs();
    REQUIRE(empty_cell.set_text(std::string{}));
    CHECK_EQ(empty_cell.get_text(), "");
    CHECK_FALSE(retired_paragraph.valid());
    CHECK_FALSE(retired_run.valid());
    CHECK(empty_cell.paragraphs().valid());
    CHECK_FALSE(empty_cell.paragraphs().runs().has_next());
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table cell text replacement is atomic for every global allocation "
    "failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    const auto replacement_text = replacement_cell_text();
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("单元格文本全局基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto cell = document.tables().rows().cells();
        auto replaced = false;
        {
            global_allocation_guard guard{0U};
            replaced = cell.set_text(replacement_text);
        }
        REQUIRE(replaced);
        successful_allocation_count =
            observed_allocation_calls.load(std::memory_order_relaxed);
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{
            make_test_path("单元格文本全局失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto cell = document.tables().rows().cells();
        auto old_paragraph = cell.paragraphs();
        auto old_run = old_paragraph.runs();
        auto replaced = true;
        try {
            global_allocation_guard guard{failure_call};
            replaced = cell.set_text(replacement_text);
        } catch (const std::bad_alloc &) {
            replaced = false;
        }

        REQUIRE_FALSE(replaced);
        CHECK(cell.valid());
        CHECK(old_paragraph.valid());
        CHECK(old_run.valid());
        CHECK_EQ(cell.get_text(), "原始中文0-0");
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table batch text replacement APIs are atomic for every pugixml "
    "allocation failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    const auto inputs = make_table_batch_text_inputs();
    constexpr auto original_texts = std::array<std::string_view, 4U>{
        "原始中文0-0", "原始中文0-1", "原始中文1-0", "原始中文1-1"};
    constexpr auto operations = std::array{
        table_batch_text_operation::row, table_batch_text_operation::rows,
        table_batch_text_operation::cell_block};

    for (const auto operation : operations) {
        CAPTURE(table_batch_operation_name(operation));
        const auto scenario_suffix =
            std::to_string(static_cast<std::size_t>(operation));
        auto successful_allocation_count = std::size_t{0U};
        {
            scoped_test_path path{make_test_path(
                "批量文本XML基线" + scenario_suffix, 0U)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            auto table = document.tables();
            auto replaced = false;
            {
                pugi_allocator_guard guard;
                replaced =
                    apply_table_batch_text_operation(table, operation, inputs);
                successful_allocation_count = pugi_allocation_calls;
            }
            REQUIRE(replaced);
            REQUIRE_GT(successful_allocation_count, 0U);
            check_table_cell_texts(
                table, expected_table_batch_texts(operation, inputs));
        }

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            scoped_test_path path{make_test_path(
                "批量文本XML失败" + scenario_suffix, failure_call)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            REQUIRE_FALSE(document.save());
            const auto xml_before =
                read_test_docx_entry(path.path(), test_document_xml_entry);

            auto table = document.tables();
            auto row = table.rows();
            const auto coordinates = batch_target_coordinates(operation);
            const auto target_snapshots =
                capture_table_cell_handle_snapshots(table, coordinates);

            auto replaced = true;
            {
                pugi_allocator_guard guard;
                pugi_failure_call = failure_call;
                replaced =
                    apply_table_batch_text_operation(table, operation, inputs);
            }

            REQUIRE_FALSE(replaced);
            CHECK(table.valid());
            CHECK(row.valid());
            check_table_cell_handle_snapshots_valid(target_snapshots);
            check_table_cell_texts(table, original_texts);
            REQUIRE_FALSE(document.save());
            CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                     xml_before);

            REQUIRE(
                apply_table_batch_text_operation(table, operation, inputs));
            check_table_cell_body_handles_retired(target_snapshots);
            check_table_cell_texts(
                table, expected_table_batch_texts(operation, inputs));
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table batch text replacement APIs are atomic for every global "
    "allocation failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    const auto inputs = make_table_batch_text_inputs();
    constexpr auto original_texts = std::array<std::string_view, 4U>{
        "原始中文0-0", "原始中文0-1", "原始中文1-0", "原始中文1-1"};
    constexpr auto operations = std::array{
        table_batch_text_operation::row, table_batch_text_operation::rows,
        table_batch_text_operation::cell_block};

    for (const auto operation : operations) {
        CAPTURE(table_batch_operation_name(operation));
        const auto scenario_suffix =
            std::to_string(static_cast<std::size_t>(operation));
        auto successful_allocation_count = std::size_t{0U};
        {
            scoped_test_path path{make_test_path(
                "批量文本全局基线" + scenario_suffix, 0U)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            auto table = document.tables();
            auto replaced = false;
            {
                global_allocation_guard guard{0U};
                replaced =
                    apply_table_batch_text_operation(table, operation, inputs);
            }
            successful_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
            REQUIRE(replaced);
            REQUIRE_GT(successful_allocation_count, 0U);
            check_table_cell_texts(
                table, expected_table_batch_texts(operation, inputs));
        }

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            scoped_test_path path{make_test_path(
                "批量文本全局失败" + scenario_suffix, failure_call)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            REQUIRE_FALSE(document.save());
            const auto xml_before =
                read_test_docx_entry(path.path(), test_document_xml_entry);

            auto table = document.tables();
            auto row = table.rows();
            const auto coordinates = batch_target_coordinates(operation);
            const auto target_snapshots =
                capture_table_cell_handle_snapshots(table, coordinates);

            auto replaced = true;
            try {
                global_allocation_guard guard{failure_call};
                replaced =
                    apply_table_batch_text_operation(table, operation, inputs);
            } catch (const std::bad_alloc &) {
                replaced = false;
            }

            REQUIRE_FALSE(replaced);
            CHECK(table.valid());
            CHECK(row.valid());
            check_table_cell_handle_snapshots_valid(target_snapshots);
            check_table_cell_texts(table, original_texts);
            REQUIRE_FALSE(document.save());
            CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                     xml_before);

            REQUIRE(
                apply_table_batch_text_operation(table, operation, inputs));
            check_table_cell_body_handles_retired(target_snapshots);
            check_table_cell_texts(
                table, expected_table_batch_texts(operation, inputs));
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "document and template table append roll back whole tables for every "
    "pugixml allocation failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    for (const auto through_template_part : {false, true}) {
        CAPTURE(through_template_part);
        auto successful_allocation_count = std::size_t{0U};
        {
            scoped_test_path path{make_test_path(
                through_template_part ? "模板整表XML基线" : "文档整表XML基线",
                0U)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            auto template_part = document.body_template();
            auto appended = featherdoc::Table{};
            {
                pugi_allocator_guard guard;
                appended = through_template_part
                               ? template_part.append_table(8U, 8U)
                               : document.append_table(8U, 8U);
                successful_allocation_count = pugi_allocation_calls;
            }
            REQUIRE(appended.valid());
            REQUIRE_GT(successful_allocation_count, 0U);
        }

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            scoped_test_path path{make_test_path(
                through_template_part ? "模板整表XML失败" : "文档整表XML失败",
                failure_call)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            REQUIRE_FALSE(document.save());
            const auto xml_before =
                read_test_docx_entry(path.path(), test_document_xml_entry);

            auto old_table = document.tables();
            auto old_row = old_table.rows();
            auto old_cell = old_row.cells();
            auto old_paragraph = old_cell.paragraphs();
            auto old_run = old_paragraph.runs();
            auto template_part = document.body_template();
            auto appended = featherdoc::Table{};
            {
                pugi_allocator_guard guard;
                pugi_failure_call = failure_call;
                appended = through_template_part
                               ? template_part.append_table(8U, 8U)
                               : document.append_table(8U, 8U);
            }

            REQUIRE_FALSE(appended.valid());
            CHECK(old_table.valid());
            CHECK(old_row.valid());
            CHECK(old_cell.valid());
            CHECK(old_paragraph.valid());
            CHECK(old_run.valid());
            CHECK(document.last_error());
            CHECK_EQ(document.last_error().code,
                     std::make_error_code(std::errc::not_enough_memory));
            CHECK_NE(document.last_error().detail.find("table"),
                     std::string::npos);
            CHECK_EQ(document.last_error().entry_name,
                     "word/document.xml");
            REQUIRE_FALSE(document.save());
            CHECK_EQ(read_test_docx_entry(path.path(),
                                          test_document_xml_entry),
                     xml_before);

            appended = through_template_part
                           ? template_part.append_table(8U, 8U)
                           : document.append_table(8U, 8U);
            REQUIRE(appended.valid());
            CHECK_FALSE(document.last_error());
            CHECK(old_table.valid());
            CHECK(old_row.valid());
            CHECK(old_cell.valid());
            CHECK(old_paragraph.valid());
            CHECK(old_run.valid());
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "document and template table append publish without a fallible global "
    "allocation after setup") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    for (const auto through_template_part : {false, true}) {
        CAPTURE(through_template_part);
        scoped_test_path path{make_test_path(
            through_template_part ? "模板整表无全局分配"
                                  : "文档整表无全局分配",
            0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto template_part = document.body_template();
        auto appended = featherdoc::Table{};
        {
            // All table-building storage belongs to pugixml. Keeping this
            // operation free of global-new allocations makes the pugi
            // fail-Nth sweep above exhaustive for its mutation phase.
            global_allocation_guard guard{1U};
            appended = through_template_part
                           ? template_part.append_table(8U, 8U)
                           : document.append_table(8U, 8U);
        }
        REQUIRE(appended.valid());
        CHECK_FALSE(document.last_error());
        CHECK_EQ(observed_allocation_calls.load(std::memory_order_relaxed),
                 0U);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "row and table clone insertion rolls back partial deep copies for every "
    "pugixml allocation failure") {
    const auto fixture_xml = table_fixture_xml(2U, 2U);
    for (std::size_t operation = 0U; operation < 4U; ++operation) {
        const auto clone_table = operation >= 2U;
        const auto insert_after = (operation % 2U) != 0U;
        CAPTURE(operation);
        CAPTURE(clone_table);
        CAPTURE(insert_after);

        auto successful_allocation_count = std::size_t{0U};
        {
            scoped_test_path path{
                make_test_path("checked-deep-clone-baseline", operation)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            auto table = document.tables();
            auto row = table.rows();
            auto inserted_table = featherdoc::Table{};
            auto inserted_row = featherdoc::TableRow{};
            {
                pugi_allocator_guard guard;
                if (clone_table) {
                    inserted_table =
                        insert_after ? table.insert_table_like_after()
                                     : table.insert_table_like_before();
                } else {
                    inserted_row = insert_after ? row.insert_row_after()
                                                : row.insert_row_before();
                }
                successful_allocation_count = pugi_allocation_calls;
            }
            if (clone_table) {
                REQUIRE(inserted_table.valid());
                CHECK_EQ(inserted_table.rows().cells().get_text(), "");
            } else {
                REQUIRE(inserted_row.valid());
                CHECK_EQ(inserted_row.cells().get_text(), "");
            }
            REQUIRE_GT(successful_allocation_count, 0U);
        }

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            scoped_test_path path{
                make_test_path("checked-deep-clone-failure",
                               operation * 1000U + failure_call)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            REQUIRE_FALSE(document.save());
            const auto xml_before =
                read_test_docx_entry(path.path(), test_document_xml_entry);

            auto table = document.tables();
            auto old_table = table;
            auto row = table.rows();
            auto old_row = row;
            auto old_cell = old_row.cells();
            auto old_paragraph = old_cell.paragraphs();
            auto old_run = old_paragraph.runs();
            auto unaffected_row = old_row;
            unaffected_row.next();
            auto unaffected_cell = unaffected_row.cells();
            auto inserted_table = featherdoc::Table{};
            auto inserted_row = featherdoc::TableRow{};
            {
                pugi_allocator_guard guard;
                pugi_failure_call = failure_call;
                if (clone_table) {
                    inserted_table =
                        insert_after ? table.insert_table_like_after()
                                     : table.insert_table_like_before();
                } else {
                    inserted_row = insert_after ? row.insert_row_after()
                                                : row.insert_row_before();
                }
            }

            CHECK_FALSE(inserted_table.valid());
            CHECK_FALSE(inserted_row.valid());
            CHECK(table.valid());
            CHECK(row.valid());
            CHECK(old_table.valid());
            CHECK(old_row.valid());
            CHECK(old_cell.valid());
            CHECK(old_paragraph.valid());
            CHECK(old_run.valid());
            CHECK(unaffected_row.valid());
            CHECK(unaffected_cell.valid());
            REQUIRE_FALSE(document.save());
            CHECK_EQ(read_test_docx_entry(path.path(),
                                          test_document_xml_entry),
                     xml_before);

            if (clone_table) {
                inserted_table =
                    insert_after ? table.insert_table_like_after()
                                 : table.insert_table_like_before();
                REQUIRE(inserted_table.valid());
                CHECK_EQ(inserted_table.rows().cells().get_text(), "");
            } else {
                inserted_row = insert_after ? row.insert_row_after()
                                            : row.insert_row_before();
                REQUIRE(inserted_row.valid());
                CHECK_EQ(inserted_row.cells().get_text(), "");
            }
            CHECK(old_table.valid());
            CHECK(old_row.valid());
            CHECK(old_cell.valid());
            CHECK(old_paragraph.valid());
            CHECK(old_run.valid());
            CHECK(unaffected_row.valid());
            CHECK(unaffected_cell.valid());
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table insertion after an anchor preserves immediate block order") {
    const auto fixture_xml = separated_tables_fixture_xml();
    for (const auto clone_like : {false, true}) {
        CAPTURE(clone_like);
        scoped_test_path path{
            make_test_path(clone_like ? "clone-after-block-order"
                                      : "new-after-block-order",
                           0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto anchor = document.tables();
        auto inserted = clone_like ? anchor.insert_table_like_after()
                                   : anchor.insert_table_after(1U, 1U);
        REQUIRE(inserted.valid());
        REQUIRE(inserted.rows().cells().set_text("inserted-table"));
        REQUIRE_FALSE(document.save());

        const auto xml =
            read_test_docx_entry(path.path(), test_document_xml_entry);
        pugi::xml_document parsed;
        REQUIRE(parsed.load_string(xml.c_str()));
        const auto body = parsed.child("w:document").child("w:body");
        const auto first = body.child("w:tbl");
        const auto second = first.next_sibling();
        const auto middle = second.next_sibling();
        const auto tail = middle.next_sibling();
        REQUIRE(first != pugi::xml_node{});
        REQUIRE(second != pugi::xml_node{});
        REQUIRE(middle != pugi::xml_node{});
        REQUIRE(tail != pugi::xml_node{});
        CHECK_EQ(std::string_view{first.name()}, "w:tbl");
        CHECK_EQ(std::string_view{second.name()}, "w:tbl");
        CHECK_EQ(std::string_view{middle.name()}, "w:p");
        CHECK_EQ(std::string_view{tail.name()}, "w:tbl");
        CHECK_EQ(first_table_cell_text(first), "anchor-table");
        CHECK_EQ(first_table_cell_text(second), "inserted-table");
        CHECK_EQ(middle.child("w:r").child("w:t").text().get(),
                 std::string_view{"middle-paragraph"});
        CHECK_EQ(first_table_cell_text(tail), "tail-table");
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "cell insertion after an anchor does not cross intervening row markers") {
    scoped_test_path path{make_test_path("cell-after-marker-order", 0U)};
    write_test_docx(path.path(), separated_cells_fixture_xml());
    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());
    auto anchor = document.tables().rows().cells();
    auto old_anchor = anchor;
    auto tail = anchor;
    tail.next();
    REQUIRE(tail.valid());
    auto inserted = anchor.insert_cell_after();
    REQUIRE(inserted.valid());
    REQUIRE(inserted.set_text("inserted-cell"));
    CHECK(old_anchor.valid());
    CHECK(tail.valid());
    CHECK_EQ(old_anchor.get_text(), "anchor-cell");
    CHECK_EQ(tail.get_text(), "tail-cell");
    REQUIRE_FALSE(document.save());

    const auto xml =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    pugi::xml_document parsed;
    REQUIRE(parsed.load_string(xml.c_str()));
    const auto row = parsed.child("w:document")
                         .child("w:body")
                         .child("w:tbl")
                         .child("w:tr");
    auto first = row.first_child();
    auto second = first.next_sibling();
    auto bookmark_start = second.next_sibling();
    auto bookmark_end = bookmark_start.next_sibling();
    auto last = bookmark_end.next_sibling();
    REQUIRE(first != pugi::xml_node{});
    REQUIRE(second != pugi::xml_node{});
    REQUIRE(bookmark_start != pugi::xml_node{});
    REQUIRE(bookmark_end != pugi::xml_node{});
    REQUIRE(last != pugi::xml_node{});
    CHECK_EQ(std::string_view{first.name()}, "w:tc");
    CHECK_EQ(std::string_view{second.name()}, "w:tc");
    CHECK_EQ(std::string_view{bookmark_start.name()}, "w:bookmarkStart");
    CHECK_EQ(std::string_view{bookmark_end.name()}, "w:bookmarkEnd");
    CHECK_EQ(std::string_view{last.name()}, "w:tc");
    CHECK_EQ(std::string_view{first.child("w:p")
                                  .child("w:r")
                                  .child("w:t")
                                  .text()
                                  .get()},
             "anchor-cell");
    CHECK_EQ(std::string_view{second.child("w:p")
                                  .child("w:r")
                                  .child("w:t")
                                  .text()
                                  .get()},
             "inserted-cell");
    CHECK_EQ(std::string_view{last.child("w:p")
                                  .child("w:r")
                                  .child("w:t")
                                  .text()
                                  .get()},
             "tail-cell");
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "horizontal unmerge restores adjacent cells before intervening markers") {
    scoped_test_path path{make_test_path("unmerge-right-marker-order", 0U)};
    write_test_docx(path.path(), horizontally_merged_marker_fixture_xml());
    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());
    auto anchor = document.tables().rows().cells();
    REQUIRE(anchor.merge_right(2U));
    CHECK_EQ(anchor.column_span(), 3U);
    REQUIRE(anchor.unmerge_right());
    CHECK_EQ(anchor.column_span(), 1U);

    auto first_restored = anchor;
    first_restored.next();
    REQUIRE(first_restored.valid());
    CHECK_EQ(first_restored.get_text(), "");
    auto second_restored = first_restored;
    second_restored.next();
    REQUIRE(second_restored.valid());
    CHECK_EQ(second_restored.get_text(), "");
    auto tail = second_restored;
    tail.next();
    REQUIRE(tail.valid());
    CHECK_EQ(tail.get_text(), "tail-cell");
    REQUIRE_FALSE(document.save());

    const auto xml =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    pugi::xml_document parsed;
    REQUIRE(parsed.load_string(xml.c_str()));
    const auto row = parsed.child("w:document")
                         .child("w:body")
                         .child("w:tbl")
                         .child("w:tr");
    const auto first = row.first_child();
    const auto second = first.next_sibling();
    const auto third = second.next_sibling();
    const auto bookmark_start = third.next_sibling();
    const auto bookmark_end = bookmark_start.next_sibling();
    const auto last = bookmark_end.next_sibling();
    REQUIRE(last != pugi::xml_node{});
    CHECK_EQ(std::string_view{first.name()}, "w:tc");
    CHECK_EQ(std::string_view{second.name()}, "w:tc");
    CHECK_EQ(std::string_view{third.name()}, "w:tc");
    CHECK_EQ(std::string_view{bookmark_start.name()}, "w:bookmarkStart");
    CHECK_EQ(std::string_view{bookmark_end.name()}, "w:bookmarkEnd");
    CHECK_EQ(std::string_view{last.name()}, "w:tc");
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "zero-dimensional table and row requests are rejected atomically") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    constexpr auto invalid_dimensions =
        std::array<std::pair<std::size_t, std::size_t>, 3U>{
            std::pair{0U, 1U}, std::pair{1U, 0U}, std::pair{0U, 0U}};

    for (const auto through_template_part : {false, true}) {
        for (std::size_t index = 0U; index < invalid_dimensions.size();
             ++index) {
            CAPTURE(through_template_part);
            CAPTURE(index);
            const auto [row_count, column_count] = invalid_dimensions[index];
            scoped_test_path path{make_test_path(
                through_template_part ? "template-zero-dimension"
                                      : "document-zero-dimension",
                index)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            REQUIRE_FALSE(document.save());
            const auto xml_before =
                read_test_docx_entry(path.path(), test_document_xml_entry);
            auto template_part = document.body_template();

            const auto rejected =
                through_template_part
                    ? template_part.append_table(row_count, column_count)
                    : document.append_table(row_count, column_count);
            CHECK_FALSE(rejected.valid());
            REQUIRE(document.last_error());
            CHECK_EQ(document.last_error().code,
                     std::make_error_code(std::errc::invalid_argument));
            CHECK_NE(document.last_error().detail.find("greater than zero"),
                     std::string::npos);
            CHECK_EQ(document.last_error().entry_name,
                     test_document_xml_entry);
            REQUIRE_FALSE(document.save());
            CHECK_EQ(read_test_docx_entry(path.path(),
                                          test_document_xml_entry),
                     xml_before);
        }
    }

    for (std::size_t operation = 0U; operation < 5U; ++operation) {
        CAPTURE(operation);
        scoped_test_path path{
            make_test_path("table-cursor-zero-dimension", operation)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto old_table = table;
        auto old_row = old_table.rows();
        auto old_cell = old_row.cells();
        auto old_paragraph = old_cell.paragraphs();
        auto old_run = old_paragraph.runs();
        if (operation == 4U) {
            CHECK_FALSE(table.append_row(0U).valid());
        } else {
            const auto row_count = operation % 2U == 0U ? 0U : 1U;
            const auto column_count = operation % 2U == 0U ? 1U : 0U;
            const auto rejected = operation < 2U
                                      ? table.insert_table_before(
                                            row_count, column_count)
                                      : table.insert_table_after(
                                            row_count, column_count);
            CHECK_FALSE(rejected.valid());
        }

        CHECK(table.valid());
        CHECK(old_table.valid());
        CHECK(old_row.valid());
        CHECK(old_cell.valid());
        CHECK(old_paragraph.valid());
        CHECK(old_run.valid());
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);
    }
}
