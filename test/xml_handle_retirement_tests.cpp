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

[[nodiscard]] auto allocation_heavy_table_fixture_xml(
    std::size_t rows, std::size_t columns,
    std::string_view layout_type = "fixed",
    std::string_view cell_width = "1200", std::string_view style_id = {},
    bool include_style_look = true) -> std::string {
    const auto large_value = large_cell_property_value();
    auto xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl><w:tblPr data-large=")"};
    xml += large_value;
    xml += R"(">)";
    if (!style_id.empty()) {
        xml += R"(<w:tblStyle w:val=")";
        xml += style_id;
        xml += R"("/>)";
    }
    xml += R"(<w:tblW w:w="0" w:type="auto"/><w:tblLayout w:type=")";
    xml += layout_type;
    xml += R"("/>)";
    if (include_style_look) {
        xml += R"(<w:tblLook w:val="04A0" w:firstRow="1" w:firstColumn="1"
      w:lastRow="0" w:lastColumn="0" w:noHBand="0" w:noVBand="1"/>)";
    }
    xml += R"(</w:tblPr>
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
            xml += R"(<w:tc><w:tcPr><w:tcW w:w=")";
            xml += cell_width;
            xml += R"(" w:type="dxa"/></w:tcPr>)";
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

[[nodiscard]] auto table_style_id_fixture_styles_xml() -> std::string {
    return R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="table" w:styleId="OldTableStyle">
    <w:name w:val="Old Table Style"/>
  </w:style>
  <w:style w:type="table" w:styleId="AtomicTableStyle">
    <w:name w:val="Atomic Table Style"/>
  </w:style>
</w:styles>
)";
}

[[nodiscard]] auto replacement_table_style_look()
    -> featherdoc::table_style_look {
    auto style_look = featherdoc::table_style_look{};
    style_look.first_row = false;
    style_look.last_row = true;
    style_look.first_column = false;
    style_look.last_column = true;
    style_look.banded_rows = false;
    style_look.banded_columns = true;
    return style_look;
}

[[nodiscard]] auto replacement_table_border(std::string_view color = "12AB34")
    -> featherdoc::border_definition {
    return {featherdoc::border_style::double_line, 16U, color, 2U};
}

enum class table_position_fixture_state {
    properties_without_position = 0,
    existing_position,
    no_properties,
    empty_properties,
    properties_without_width,
};

[[nodiscard]] auto table_position_allocation_fixture_xml(
    std::size_t padding_paragraph_count,
    table_position_fixture_state state =
        table_position_fixture_state::properties_without_position)
    -> std::string {
    auto xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl>)"};
    if (state != table_position_fixture_state::no_properties) {
        xml += "<w:tblPr>";
        if (state != table_position_fixture_state::empty_properties) {
            xml += R"(<w:tblStyle w:val="TableGrid"/>)";
            if (state == table_position_fixture_state::existing_position) {
                xml += R"(<w:tblpPr data-custom="preserved"
      w:horzAnchor="margin" w:tblpX="12" w:tblpXSpec="left"
      w:vertAnchor="page" w:tblpY="34" w:tblpYSpec="top"
      w:leftFromText="1" w:rightFromText="2" w:topFromText="3"
      w:bottomFromText="4" w:tblOverlap="never"><w:custom data-child="preserved"><w:nested>payload</w:nested></w:custom></w:tblpPr>
    <w:tblOverlap w:val="overlap" data-overlap-custom="preserved"><w:overlapCustom data-child="preserved"><w:nested>overlap-payload</w:nested></w:overlapCustom></w:tblOverlap>)";
            }
            if (state !=
                table_position_fixture_state::properties_without_width) {
                xml += R"(<w:tblW w:w="0" w:type="auto"/>)";
            }
            xml +=
                R"(<w:tblLayout w:type="fixed"/><w:tblLook w:val="04A0"/>)";
        }
        xml += "</w:tblPr>";
    }
    xml += R"(<w:tblGrid><w:gridCol w:w="1200"/></w:tblGrid>
  <w:tr><w:tc><w:tcPr><w:tcW w:w="1200" w:type="dxa"/></w:tcPr>
  <w:p><w:r><w:t>原始中文0-0</w:t></w:r></w:p></w:tc></w:tr></w:tbl>)";
    for (std::size_t index = 0U; index < padding_paragraph_count; ++index) {
        xml += "<w:p><w:r><w:t>allocation-padding</w:t></w:r></w:p>";
    }
    xml += "</w:body></w:document>";
    return xml;
}

[[nodiscard]] auto full_table_position_replacement()
    -> featherdoc::table_position {
    auto replacement = featherdoc::table_position{};
    replacement.horizontal_reference =
        featherdoc::table_position_horizontal_reference::page;
    replacement.horizontal_offset_twips = 720;
    replacement.horizontal_spec =
        featherdoc::table_position_horizontal_spec::center;
    replacement.vertical_reference =
        featherdoc::table_position_vertical_reference::paragraph;
    replacement.vertical_offset_twips = -120;
    replacement.vertical_spec =
        featherdoc::table_position_vertical_spec::bottom;
    replacement.left_from_text_twips = 144U;
    replacement.right_from_text_twips = 288U;
    replacement.top_from_text_twips = 72U;
    replacement.bottom_from_text_twips = 216U;
    replacement.overlap = featherdoc::table_overlap::never;
    return replacement;
}

[[nodiscard]] auto table_position_revision_tail_fixture_xml() -> std::string {
    return R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl><w:tblPr><w:tblPrChange w:id="1"/></w:tblPr>
  <w:tblGrid><w:gridCol w:w="1200"/></w:tblGrid>
  <w:tr><w:tc><w:tcPr/><w:p/></w:tc></w:tr></w:tbl>
  </w:body>
</w:document>)";
}

[[nodiscard]] auto legacy_table_position_fixture_xml() -> std::string {
    return R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl><w:tblPr><w:tblStyle w:val="TableGrid"/>
  <w:tblW w:w="0" w:type="auto"/><w:tblpPr w:horzAnchor="margin"
    w:tblpX="12" w:vertAnchor="page" w:tblpY="34"
    w:tblOverlap="overlap"/></w:tblPr>
  <w:tblGrid><w:gridCol w:w="1200"/></w:tblGrid>
  <w:tr><w:tc><w:tcPr/><w:p/></w:tc></w:tr></w:tbl>
  </w:body>
</w:document>)";
}

[[nodiscard]] auto duplicate_table_position_fixture_xml() -> std::string {
    return R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:tbl><w:tblPr>
  <w:tblpPr w:horzAnchor="margin" w:tblpX="12"
    w:vertAnchor="page" w:tblpY="34"/>
  <w:tblpPr w:horzAnchor="page" w:tblpX="56"
    w:vertAnchor="text" w:tblpY="78"/>
  <w:tblOverlap w:val="overlap"/><w:tblOverlap w:val="never"/>
  <w:tblW w:w="0" w:type="auto"/></w:tblPr>
  <w:tblGrid><w:gridCol w:w="1200"/></w:tblGrid>
  <w:tr><w:tc><w:tcPr/><w:p/></w:tc></w:tr></w:tbl>
  </w:body>
</w:document>)";
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

TEST_CASE(
    "table floating position uses schema order before property revisions") {
    scoped_test_path path{make_test_path("浮动表格位置顺序", 0U)};
    write_test_docx(path.path(), table_position_revision_tail_fixture_xml());
    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());
    auto table = document.tables();
    REQUIRE(table.set_position(full_table_position_replacement()));
    REQUIRE_FALSE(document.save());

    const auto saved_xml =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_xml.c_str()));
    const auto table_properties = saved_document.child("w:document")
                                      .child("w:body")
                                      .child("w:tbl")
                                      .child("w:tblPr");
    REQUIRE(table_properties != pugi::xml_node{});
    const auto position = table_properties.first_child();
    REQUIRE(position != pugi::xml_node{});
    CHECK_EQ(std::string_view{position.name()}, "w:tblpPr");
    CHECK_EQ(position.attribute("w:tblOverlap"), pugi::xml_attribute{});
    const auto overlap = position.next_sibling();
    REQUIRE(overlap != pugi::xml_node{});
    CHECK_EQ(std::string_view{overlap.name()}, "w:tblOverlap");
    CHECK_EQ(std::string_view{overlap.attribute("w:val").value()}, "never");
    CHECK_EQ(std::string_view{overlap.next_sibling().name()}, "w:tblPrChange");
}

TEST_CASE("table floating position migrates legacy overlap into schema order") {
    scoped_test_path path{make_test_path("legacy-table-position", 0U)};
    write_test_docx(path.path(), legacy_table_position_fixture_xml());
    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());
    auto table = document.tables();

    const auto legacy_position = table.position();
    REQUIRE(legacy_position.has_value());
    REQUIRE(legacy_position->overlap.has_value());
    CHECK_EQ(*legacy_position->overlap, featherdoc::table_overlap::allow);

    REQUIRE(table.set_position(full_table_position_replacement()));
    REQUIRE_FALSE(document.save());

    const auto saved_xml =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_xml.c_str()));
    const auto table_properties = saved_document.child("w:document")
                                      .child("w:body")
                                      .child("w:tbl")
                                      .child("w:tblPr");
    REQUIRE(table_properties != pugi::xml_node{});
    const auto style = table_properties.first_child();
    REQUIRE(style != pugi::xml_node{});
    CHECK_EQ(std::string_view{style.name()}, "w:tblStyle");
    const auto position = style.next_sibling();
    REQUIRE(position != pugi::xml_node{});
    CHECK_EQ(std::string_view{position.name()}, "w:tblpPr");
    CHECK_EQ(position.attribute("w:tblOverlap"), pugi::xml_attribute{});
    const auto overlap = position.next_sibling();
    REQUIRE(overlap != pugi::xml_node{});
    CHECK_EQ(std::string_view{overlap.name()}, "w:tblOverlap");
    CHECK_EQ(std::string_view{overlap.attribute("w:val").value()}, "never");
    CHECK_EQ(std::string_view{overlap.next_sibling().name()}, "w:tblW");
}

TEST_CASE("table floating position prefers standard overlap and preserves "
          "extensions") {
    scoped_test_path path{make_test_path("standard-table-overlap", 0U)};
    write_test_docx(path.path(),
                    table_position_allocation_fixture_xml(
                        0U, table_position_fixture_state::existing_position));
    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());
    auto table = document.tables();

    const auto original_position = table.position();
    REQUIRE(original_position.has_value());
    REQUIRE(original_position->overlap.has_value());
    CHECK_EQ(*original_position->overlap, featherdoc::table_overlap::allow);

    REQUIRE(table.set_position(full_table_position_replacement()));
    REQUIRE_FALSE(document.save());

    const auto saved_xml =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_xml.c_str()));
    const auto table_properties = saved_document.child("w:document")
                                      .child("w:body")
                                      .child("w:tbl")
                                      .child("w:tblPr");
    REQUIRE(table_properties != pugi::xml_node{});
    const auto position = table_properties.child("w:tblpPr");
    REQUIRE(position != pugi::xml_node{});
    CHECK_EQ(position.attribute("w:tblOverlap"), pugi::xml_attribute{});
    const auto overlap = table_properties.child("w:tblOverlap");
    REQUIRE(overlap != pugi::xml_node{});
    CHECK_EQ(std::string_view{overlap.attribute("w:val").value()}, "never");
    CHECK_EQ(std::string_view{overlap.attribute("data-overlap-custom").value()},
             "preserved");
    const auto overlap_custom = overlap.child("w:overlapCustom");
    REQUIRE(overlap_custom != pugi::xml_node{});
    CHECK_EQ(std::string_view{overlap_custom.attribute("data-child").value()},
             "preserved");
    const auto nested = overlap_custom.child("w:nested");
    REQUIRE(nested != pugi::xml_node{});
    CHECK_EQ(std::string_view{nested.text().get()}, "overlap-payload");
}

TEST_CASE("table floating position rejects duplicate schema nodes atomically") {
    scoped_test_path path{make_test_path("duplicate-table-position", 0U)};
    write_test_docx(path.path(), duplicate_table_position_fixture_xml());
    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());
    REQUIRE_FALSE(document.save());
    const auto xml_before =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    auto table = document.tables();

    CHECK_FALSE(table.set_position(full_table_position_replacement()));
    CHECK_FALSE(table.clear_position());
    REQUIRE_FALSE(document.save());
    CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
             xml_before);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table floating position is atomic for every pugixml allocation failure") {
    constexpr auto padding_paragraph_count = std::size_t{99U};
    const auto fixture_xml =
        table_position_allocation_fixture_xml(padding_paragraph_count);
    const auto replacement = full_table_position_replacement();

    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("浮动表格位置XML基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto positioned = false;
        {
            pugi_allocator_guard guard;
            positioned = table.set_position(replacement);
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(positioned);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{
            make_test_path("浮动表格位置XML失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto cell = row.cells();
        auto paragraph = cell.paragraphs();
        auto run = paragraph.runs();
        auto positioned = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            pugi_allocator_guard guard;
            pugi_failure_call = failure_call;
            positioned = table.set_position(replacement);
            observed_failure_allocation_count = pugi_allocation_calls;
        }

        REQUIRE_FALSE(positioned);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_FALSE(table.position().has_value());
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.set_position(replacement));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        const auto applied = table.position();
        REQUIRE(applied.has_value());
        CHECK_EQ(applied->horizontal_reference,
                 replacement.horizontal_reference);
        CHECK_EQ(applied->horizontal_offset_twips,
                 replacement.horizontal_offset_twips);
        CHECK_EQ(applied->horizontal_spec, replacement.horizontal_spec);
        CHECK_EQ(applied->vertical_reference, replacement.vertical_reference);
        CHECK_EQ(applied->vertical_offset_twips,
                 replacement.vertical_offset_twips);
        CHECK_EQ(applied->vertical_spec, replacement.vertical_spec);
        CHECK_EQ(applied->left_from_text_twips,
                 replacement.left_from_text_twips);
        CHECK_EQ(applied->right_from_text_twips,
                 replacement.right_from_text_twips);
        CHECK_EQ(applied->top_from_text_twips, replacement.top_from_text_twips);
        CHECK_EQ(applied->bottom_from_text_twips,
                 replacement.bottom_from_text_twips);
        CHECK_EQ(applied->overlap, replacement.overlap);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table floating position replacement and property creation roll back for "
    "every pugixml allocation failure") {
    constexpr auto scenarios = std::array{
        std::pair{table_position_fixture_state::existing_position,
                  std::size_t{90U}},
        std::pair{table_position_fixture_state::no_properties,
                  std::size_t{101U}},
        std::pair{table_position_fixture_state::empty_properties,
                  std::size_t{101U}},
    };
    const auto replacement = full_table_position_replacement();

    for (const auto &[state, padding_paragraph_count] : scenarios) {
        CAPTURE(static_cast<int>(state));
        const auto scenario_suffix =
            state == table_position_fixture_state::existing_position
                ? std::string{"已有位置"}
            : state == table_position_fixture_state::no_properties
                ? std::string{"无表格属性"}
                : std::string{"空表格属性"};
        auto scenario_replacement = replacement;
        if (state == table_position_fixture_state::existing_position) {
            scenario_replacement.horizontal_spec.reset();
            scenario_replacement.vertical_spec.reset();
            scenario_replacement.left_from_text_twips.reset();
            scenario_replacement.right_from_text_twips.reset();
            scenario_replacement.top_from_text_twips.reset();
            scenario_replacement.bottom_from_text_twips.reset();
        }

        const auto fixture_xml = table_position_allocation_fixture_xml(
            padding_paragraph_count, state);
        auto successful_allocation_count = std::size_t{0U};
        {
            scoped_test_path path{
                make_test_path("浮动表格位置边界基线" + scenario_suffix, 0U)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            auto table = document.tables();
            auto positioned = false;
            {
                pugi_allocator_guard guard;
                positioned = table.set_position(scenario_replacement);
                successful_allocation_count = pugi_allocation_calls;
            }
            REQUIRE(positioned);
        }
        REQUIRE_GT(successful_allocation_count, 0U);

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            scoped_test_path path{make_test_path(
                "浮动表格位置边界失败" + scenario_suffix, failure_call)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            REQUIRE_FALSE(document.save());
            const auto xml_before =
                read_test_docx_entry(path.path(), test_document_xml_entry);

            auto table = document.tables();
            auto row = table.rows();
            auto cell = row.cells();
            auto paragraph = cell.paragraphs();
            auto run = paragraph.runs();
            const auto original_position = table.position();
            if (state == table_position_fixture_state::existing_position) {
                REQUIRE(original_position.has_value());
                CHECK_EQ(
                    original_position->horizontal_reference,
                    featherdoc::table_position_horizontal_reference::margin);
                CHECK_EQ(original_position->horizontal_offset_twips, 12);
                CHECK_EQ(
                    original_position->vertical_reference,
                    featherdoc::table_position_vertical_reference::page);
                CHECK_EQ(original_position->vertical_offset_twips, 34);
                CHECK_EQ(original_position->overlap,
                         featherdoc::table_overlap::allow);
            } else {
                REQUIRE_FALSE(original_position.has_value());
            }

            auto positioned = true;
            auto observed_failure_allocation_count = std::size_t{0U};
            {
                pugi_allocator_guard guard;
                pugi_failure_call = failure_call;
                positioned = table.set_position(scenario_replacement);
                observed_failure_allocation_count = pugi_allocation_calls;
            }

            REQUIRE_FALSE(positioned);
            REQUIRE_GE(observed_failure_allocation_count, failure_call);
            CHECK(table.valid());
            CHECK(row.valid());
            CHECK(cell.valid());
            CHECK(paragraph.valid());
            CHECK(run.valid());
            const auto position_after_failure = table.position();
            if (state == table_position_fixture_state::existing_position) {
                REQUIRE(position_after_failure.has_value());
                CHECK_EQ(
                    position_after_failure->horizontal_reference,
                    featherdoc::table_position_horizontal_reference::margin);
                CHECK_EQ(position_after_failure->horizontal_offset_twips, 12);
                CHECK_EQ(
                    position_after_failure->vertical_reference,
                    featherdoc::table_position_vertical_reference::page);
                CHECK_EQ(position_after_failure->vertical_offset_twips, 34);
                CHECK_EQ(position_after_failure->overlap,
                         featherdoc::table_overlap::allow);
            } else {
                CHECK_FALSE(position_after_failure.has_value());
            }
            REQUIRE_FALSE(document.save());
            CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                     xml_before);

            REQUIRE(table.set_position(scenario_replacement));
            CHECK(table.valid());
            CHECK(row.valid());
            CHECK(cell.valid());
            CHECK(paragraph.valid());
            CHECK(run.valid());
            const auto applied = table.position();
            REQUIRE(applied.has_value());
            CHECK_EQ(applied->horizontal_reference,
                     scenario_replacement.horizontal_reference);
            CHECK_EQ(applied->horizontal_offset_twips,
                     scenario_replacement.horizontal_offset_twips);
            CHECK_EQ(applied->vertical_reference,
                     scenario_replacement.vertical_reference);
            CHECK_EQ(applied->vertical_offset_twips,
                     scenario_replacement.vertical_offset_twips);
            CHECK_EQ(applied->horizontal_spec,
                     scenario_replacement.horizontal_spec);
            CHECK_EQ(applied->vertical_spec,
                     scenario_replacement.vertical_spec);
            CHECK_EQ(applied->left_from_text_twips,
                     scenario_replacement.left_from_text_twips);
            CHECK_EQ(applied->right_from_text_twips,
                     scenario_replacement.right_from_text_twips);
            CHECK_EQ(applied->top_from_text_twips,
                     scenario_replacement.top_from_text_twips);
            CHECK_EQ(applied->bottom_from_text_twips,
                     scenario_replacement.bottom_from_text_twips);
            CHECK_EQ(applied->overlap, scenario_replacement.overlap);

            REQUIRE_FALSE(document.save());
            const auto saved_xml =
                read_test_docx_entry(path.path(), test_document_xml_entry);
            pugi::xml_document saved_document;
            REQUIRE(saved_document.load_string(saved_xml.c_str()));
            const auto table_properties = saved_document.child("w:document")
                                              .child("w:body")
                                              .child("w:tbl")
                                              .child("w:tblPr");
            REQUIRE(table_properties != pugi::xml_node{});
            const auto position_node = table_properties.child("w:tblpPr");
            REQUIRE(position_node != pugi::xml_node{});
            CHECK_EQ(position_node.next_sibling("w:tblpPr"), pugi::xml_node{});
            if (state == table_position_fixture_state::existing_position) {
                CHECK_EQ(
                    std::string_view{
                        position_node.attribute("data-custom").value()},
                    "preserved");
                const auto custom = position_node.child("w:custom");
                REQUIRE(custom != pugi::xml_node{});
                CHECK_EQ(
                    std::string_view{custom.attribute("data-child").value()},
                    "preserved");
                CHECK_EQ(
                    std::string_view{custom.child("w:nested").text().get()},
                    "payload");
                CHECK_EQ(position_node.attribute("w:tblpXSpec"),
                         pugi::xml_attribute{});
                CHECK_EQ(position_node.attribute("w:tblpYSpec"),
                         pugi::xml_attribute{});
                CHECK_EQ(position_node.attribute("w:leftFromText"),
                         pugi::xml_attribute{});
                CHECK_EQ(position_node.attribute("w:rightFromText"),
                         pugi::xml_attribute{});
                CHECK_EQ(position_node.attribute("w:topFromText"),
                         pugi::xml_attribute{});
                CHECK_EQ(position_node.attribute("w:bottomFromText"),
                         pugi::xml_attribute{});
                CHECK_EQ(position_node.attribute("w:tblOverlap"),
                         pugi::xml_attribute{});
                const auto overlap_node =
                    table_properties.child("w:tblOverlap");
                REQUIRE(overlap_node != pugi::xml_node{});
                CHECK_EQ(
                    std::string_view{overlap_node.attribute("w:val").value()},
                    "never");
                CHECK_EQ(std::string_view{
                             overlap_node.attribute("data-overlap-custom")
                                 .value()},
                         "preserved");
                const auto overlap_custom =
                    overlap_node.child("w:overlapCustom");
                REQUIRE(overlap_custom != pugi::xml_node{});
                CHECK_EQ(std::string_view{
                             overlap_custom.attribute("data-child").value()},
                         "preserved");
                const auto nested = overlap_custom.child("w:nested");
                REQUIRE(nested != pugi::xml_node{});
                CHECK_EQ(std::string_view{nested.text().get()},
                         "overlap-payload");
            }
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table floating position replacement is atomic for every global "
    "allocation failure") {
    const auto fixture_xml = table_position_allocation_fixture_xml(
        0U, table_position_fixture_state::existing_position);
    const auto replacement = full_table_position_replacement();

    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("浮动表格位置全局基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto positioned = false;
        {
            global_allocation_guard guard{0U};
            positioned = table.set_position(replacement);
            successful_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }
        REQUIRE(positioned);
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{
            make_test_path("浮动表格位置全局失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto cell = row.cells();
        auto paragraph = cell.paragraphs();
        auto run = paragraph.runs();
        auto positioned = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            global_allocation_guard guard{failure_call};
            positioned = table.set_position(replacement);
            observed_failure_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }

        REQUIRE_FALSE(positioned);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        const auto position_after_failure = table.position();
        REQUIRE(position_after_failure.has_value());
        CHECK_EQ(position_after_failure->horizontal_offset_twips, 12);
        CHECK_EQ(position_after_failure->vertical_offset_twips, 34);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.set_position(replacement));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        const auto applied = table.position();
        REQUIRE(applied.has_value());
        CHECK_EQ(applied->horizontal_offset_twips,
                 replacement.horizontal_offset_twips);
        CHECK_EQ(applied->vertical_offset_twips,
                 replacement.vertical_offset_twips);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "clearing table floating position is atomic for every global allocation "
    "failure") {
    const auto fixture_xml = table_position_allocation_fixture_xml(
        0U, table_position_fixture_state::existing_position);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{
            make_test_path("clear-table-position-baseline", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto cleared = false;
        {
            global_allocation_guard guard{0U};
            cleared = table.clear_position();
            successful_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }
        REQUIRE(cleared);
        REQUIRE_GT(successful_allocation_count, 0U);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{
            make_test_path("clear-table-position-failure", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto cell = row.cells();
        auto paragraph = cell.paragraphs();
        auto run = paragraph.runs();
        auto cleared = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            global_allocation_guard guard{failure_call};
            cleared = table.clear_position();
            observed_failure_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }

        REQUIRE_FALSE(cleared);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        const auto position_after_failure = table.position();
        REQUIRE(position_after_failure.has_value());
        CHECK_EQ(position_after_failure->overlap,
                 featherdoc::table_overlap::allow);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.clear_position());
        CHECK_FALSE(table.position().has_value());
        REQUIRE_FALSE(document.save());
        const auto saved_xml =
            read_test_docx_entry(path.path(), test_document_xml_entry);
        pugi::xml_document saved_document;
        REQUIRE(saved_document.load_string(saved_xml.c_str()));
        const auto table_properties = saved_document.child("w:document")
                                          .child("w:body")
                                          .child("w:tbl")
                                          .child("w:tblPr");
        REQUIRE(table_properties != pugi::xml_node{});
        CHECK_EQ(table_properties.child("w:tblpPr"), pugi::xml_node{});
        CHECK_EQ(table_properties.child("w:tblOverlap"), pugi::xml_node{});
    }
}

TEST_CASE("layout mode updates preserve handles extensions and fixed widths") {
    const auto fixture_xml =
        allocation_heavy_table_fixture_xml(2U, 2U, "autofit", "600");
    scoped_test_path path{make_test_path("布局模式句柄与扩展保留", 0U)};
    write_test_docx(path.path(), fixture_xml);
    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());

    auto table = document.tables();
    auto row = table.rows();
    auto first_cell = row.cells();
    auto second_cell = first_cell;
    second_cell.next();
    auto paragraph = first_cell.paragraphs();
    auto run = paragraph.runs();

    REQUIRE(table.set_layout_mode(featherdoc::table_layout_mode::fixed));
    CHECK(table.valid());
    CHECK(row.valid());
    CHECK(first_cell.valid());
    CHECK(second_cell.valid());
    CHECK(paragraph.valid());
    CHECK(run.valid());
    REQUIRE(table.layout_mode().has_value());
    CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
    REQUIRE(first_cell.width_twips().has_value());
    CHECK_EQ(*first_cell.width_twips(), 1200U);
    REQUIRE(second_cell.width_twips().has_value());
    CHECK_EQ(*second_cell.width_twips(), 1200U);

    REQUIRE_FALSE(document.save());
    const auto saved_xml =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_xml.c_str()));
    const auto saved_table =
        saved_document.child("w:document").child("w:body").child("w:tbl");
    const auto saved_properties = saved_table.child("w:tblPr");
    const auto saved_grid = saved_table.child("w:tblGrid");
    REQUIRE(saved_properties != pugi::xml_node{});
    REQUIRE(saved_grid != pugi::xml_node{});
    CHECK_FALSE(
        std::string_view{saved_properties.attribute("data-large").value()}
            .empty());
    CHECK_FALSE(
        std::string_view{saved_grid.attribute("data-large").value()}.empty());
    const auto saved_layout = saved_properties.child("w:tblLayout");
    REQUIRE(saved_layout != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_layout.attribute("w:type").value()},
             "fixed");
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "layout mode updates are atomic for every pugixml allocation failure") {
    for (const auto target_layout : {featherdoc::table_layout_mode::fixed,
                                     featherdoc::table_layout_mode::autofit}) {
        const auto target_fixed =
            target_layout == featherdoc::table_layout_mode::fixed;
        const auto initial_layout = target_fixed
                                        ? featherdoc::table_layout_mode::autofit
                                        : featherdoc::table_layout_mode::fixed;
        const auto initial_width = target_fixed ? 600U : 1200U;
        const auto scenario_suffix =
            target_fixed ? std::string{"固定"} : std::string{"自动适应"};
        const auto fixture_xml = allocation_heavy_table_fixture_xml(
            2U, 2U, target_fixed ? "autofit" : "fixed",
            target_fixed ? "600" : "1200");
        CAPTURE(scenario_suffix);

        auto successful_allocation_count = std::size_t{0U};
        {
            scoped_test_path path{make_test_path(
                "布局模式XML基线" + scenario_suffix, 0U)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            auto table = document.tables();
            auto updated = false;
            {
                pugi_allocator_guard guard;
                updated = table.set_layout_mode(target_layout);
                successful_allocation_count = pugi_allocation_calls;
            }
            REQUIRE(updated);
        }
        REQUIRE_GT(successful_allocation_count, 0U);

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            scoped_test_path path{make_test_path(
                "布局模式XML失败" + scenario_suffix, failure_call)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            REQUIRE_FALSE(document.save());
            const auto xml_before =
                read_test_docx_entry(path.path(), test_document_xml_entry);

            auto table = document.tables();
            auto row = table.rows();
            auto first_cell = row.cells();
            auto second_cell = first_cell;
            second_cell.next();
            auto paragraph = first_cell.paragraphs();
            auto run = paragraph.runs();
            auto updated = true;
            auto observed_failure_allocation_count = std::size_t{0U};
            {
                pugi_allocator_guard guard;
                pugi_failure_call = failure_call;
                updated = table.set_layout_mode(target_layout);
                observed_failure_allocation_count = pugi_allocation_calls;
            }

            REQUIRE_FALSE(updated);
            REQUIRE_GE(observed_failure_allocation_count, failure_call);
            CHECK(table.valid());
            CHECK(row.valid());
            CHECK(first_cell.valid());
            CHECK(second_cell.valid());
            CHECK(paragraph.valid());
            CHECK(run.valid());
            REQUIRE(table.layout_mode().has_value());
            CHECK_EQ(*table.layout_mode(), initial_layout);
            REQUIRE(first_cell.width_twips().has_value());
            CHECK_EQ(*first_cell.width_twips(), initial_width);
            REQUIRE(second_cell.width_twips().has_value());
            CHECK_EQ(*second_cell.width_twips(), initial_width);
            REQUIRE_FALSE(document.save());
            CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                     xml_before);

            REQUIRE(table.set_layout_mode(target_layout));
            CHECK(table.valid());
            CHECK(row.valid());
            CHECK(first_cell.valid());
            CHECK(second_cell.valid());
            CHECK(paragraph.valid());
            CHECK(run.valid());
            REQUIRE(table.layout_mode().has_value());
            CHECK_EQ(*table.layout_mode(), target_layout);
            REQUIRE(first_cell.width_twips().has_value());
            CHECK_EQ(*first_cell.width_twips(), 1200U);
            REQUIRE(second_cell.width_twips().has_value());
            CHECK_EQ(*second_cell.width_twips(), 1200U);
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "layout mode updates are atomic for every global allocation failure") {
    for (const auto target_layout : {featherdoc::table_layout_mode::fixed,
                                     featherdoc::table_layout_mode::autofit}) {
        const auto target_fixed =
            target_layout == featherdoc::table_layout_mode::fixed;
        const auto initial_layout = target_fixed
                                        ? featherdoc::table_layout_mode::autofit
                                        : featherdoc::table_layout_mode::fixed;
        const auto initial_width = target_fixed ? 600U : 1200U;
        const auto scenario_suffix =
            target_fixed ? std::string{"固定"} : std::string{"自动适应"};
        const auto fixture_xml = allocation_heavy_table_fixture_xml(
            2U, 2U, target_fixed ? "autofit" : "fixed",
            target_fixed ? "600" : "1200");
        CAPTURE(scenario_suffix);

        auto successful_allocation_count = std::size_t{0U};
        {
            scoped_test_path path{make_test_path(
                "布局模式全局基线" + scenario_suffix, 0U)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            auto table = document.tables();
            auto updated = false;
            {
                global_allocation_guard guard{0U};
                updated = table.set_layout_mode(target_layout);
                successful_allocation_count =
                    observed_allocation_calls.load(std::memory_order_relaxed);
            }
            REQUIRE(updated);
        }
        REQUIRE_GT(successful_allocation_count, 0U);

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            scoped_test_path path{make_test_path(
                "布局模式全局失败" + scenario_suffix, failure_call)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            REQUIRE_FALSE(document.save());
            const auto xml_before =
                read_test_docx_entry(path.path(), test_document_xml_entry);

            auto table = document.tables();
            auto row = table.rows();
            auto first_cell = row.cells();
            auto second_cell = first_cell;
            second_cell.next();
            auto paragraph = first_cell.paragraphs();
            auto run = paragraph.runs();
            auto updated = true;
            auto observed_failure_allocation_count = std::size_t{0U};
            {
                global_allocation_guard guard{failure_call};
                updated = table.set_layout_mode(target_layout);
                observed_failure_allocation_count =
                    observed_allocation_calls.load(std::memory_order_relaxed);
            }

            REQUIRE_FALSE(updated);
            REQUIRE_GE(observed_failure_allocation_count, failure_call);
            CHECK(table.valid());
            CHECK(row.valid());
            CHECK(first_cell.valid());
            CHECK(second_cell.valid());
            CHECK(paragraph.valid());
            CHECK(run.valid());
            REQUIRE(table.layout_mode().has_value());
            CHECK_EQ(*table.layout_mode(), initial_layout);
            REQUIRE(first_cell.width_twips().has_value());
            CHECK_EQ(*first_cell.width_twips(), initial_width);
            REQUIRE(second_cell.width_twips().has_value());
            CHECK_EQ(*second_cell.width_twips(), initial_width);
            REQUIRE_FALSE(document.save());
            CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                     xml_before);

            REQUIRE(table.set_layout_mode(target_layout));
            CHECK(table.valid());
            CHECK(row.valid());
            CHECK(first_cell.valid());
            CHECK(second_cell.valid());
            CHECK(paragraph.valid());
            CHECK(run.valid());
            REQUIRE(table.layout_mode().has_value());
            CHECK_EQ(*table.layout_mode(), target_layout);
            REQUIRE(first_cell.width_twips().has_value());
            CHECK_EQ(*first_cell.width_twips(), 1200U);
            REQUIRE(second_cell.width_twips().has_value());
            CHECK_EQ(*second_cell.width_twips(), 1200U);
        }
    }
}

TEST_CASE("table width updates preserve handles and unrelated XML content") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    scoped_test_path path{make_test_path("表宽句柄与扩展保留", 0U)};
    write_test_docx(path.path(), fixture_xml);
    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());

    auto table = document.tables();
    auto row = table.rows();
    auto first_cell = row.cells();
    auto second_cell = first_cell;
    second_cell.next();
    auto paragraph = first_cell.paragraphs();
    auto run = paragraph.runs();
    CHECK_FALSE(table.width_twips().has_value());

    REQUIRE(table.set_width_twips(7200U));
    CHECK(table.valid());
    CHECK(row.valid());
    CHECK(first_cell.valid());
    CHECK(second_cell.valid());
    CHECK(paragraph.valid());
    CHECK(run.valid());
    REQUIRE(table.width_twips().has_value());
    CHECK_EQ(*table.width_twips(), 7200U);
    REQUIRE(table.layout_mode().has_value());
    CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
    REQUIRE(first_cell.width_twips().has_value());
    CHECK_EQ(*first_cell.width_twips(), 1200U);
    REQUIRE(second_cell.width_twips().has_value());
    CHECK_EQ(*second_cell.width_twips(), 1200U);

    REQUIRE_FALSE(document.save());
    const auto saved_xml =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_xml.c_str()));
    const auto saved_table =
        saved_document.child("w:document").child("w:body").child("w:tbl");
    const auto saved_properties = saved_table.child("w:tblPr");
    const auto saved_grid = saved_table.child("w:tblGrid");
    REQUIRE(saved_properties != pugi::xml_node{});
    REQUIRE(saved_grid != pugi::xml_node{});
    CHECK_FALSE(
        std::string_view{saved_properties.attribute("data-large").value()}
            .empty());
    CHECK_FALSE(
        std::string_view{saved_grid.attribute("data-large").value()}.empty());
    const auto saved_width = saved_properties.child("w:tblW");
    REQUIRE(saved_width != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_width.attribute("w:w").value()}, "7200");
    CHECK_EQ(std::string_view{saved_width.attribute("w:type").value()}, "dxa");
    const auto saved_layout = saved_properties.child("w:tblLayout");
    REQUIRE(saved_layout != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_layout.attribute("w:type").value()},
             "fixed");
    const auto saved_look = saved_properties.child("w:tblLook");
    REQUIRE(saved_look != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_look.attribute("w:val").value()}, "04A0");
    CHECK_EQ(std::string_view{saved_look.attribute("w:firstRow").value()}, "1");
    CHECK_EQ(std::string_view{saved_look.attribute("w:noVBand").value()}, "1");
}

TEST_CASE("table width creation supports missing property nodes") {
    constexpr auto scenarios = std::array{
        std::pair{table_position_fixture_state::no_properties,
                  std::string_view{"缺少表格属性"}},
        std::pair{table_position_fixture_state::empty_properties,
                  std::string_view{"缺少表宽节点"}},
        std::pair{table_position_fixture_state::properties_without_width,
                  std::string_view{"保留既有表格属性"}},
    };

    for (const auto &[fixture_state, scenario_suffix] : scenarios) {
        CAPTURE(scenario_suffix);
        const auto fixture_xml =
            table_position_allocation_fixture_xml(0U, fixture_state);
        scoped_test_path path{make_test_path(
            std::string{"表宽创建"} + std::string{scenario_suffix}, 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());

        auto table = document.tables();
        auto row = table.rows();
        auto cell = row.cells();
        auto paragraph = cell.paragraphs();
        auto run = paragraph.runs();
        CHECK_FALSE(table.width_twips().has_value());
        if (fixture_state ==
            table_position_fixture_state::properties_without_width) {
            REQUIRE(table.layout_mode().has_value());
            CHECK_EQ(*table.layout_mode(),
                     featherdoc::table_layout_mode::fixed);
        } else {
            CHECK_FALSE(table.layout_mode().has_value());
        }

        REQUIRE(table.set_width_twips(7200U));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        REQUIRE(table.width_twips().has_value());
        CHECK_EQ(*table.width_twips(), 7200U);
        if (fixture_state ==
            table_position_fixture_state::properties_without_width) {
            REQUIRE(table.layout_mode().has_value());
            CHECK_EQ(*table.layout_mode(),
                     featherdoc::table_layout_mode::fixed);
        } else {
            CHECK_FALSE(table.layout_mode().has_value());
        }

        REQUIRE_FALSE(document.save());
        const auto saved_xml =
            read_test_docx_entry(path.path(), test_document_xml_entry);
        pugi::xml_document saved_document;
        REQUIRE(saved_document.load_string(saved_xml.c_str()));
        const auto saved_table = saved_document.child("w:document")
                                     .child("w:body")
                                     .child("w:tbl");
        const auto saved_properties = saved_table.child("w:tblPr");
        REQUIRE(saved_properties != pugi::xml_node{});
        CHECK_EQ(saved_table.first_child(), saved_properties);
        CHECK_EQ(std::string_view{saved_properties.next_sibling().name()},
                 "w:tblGrid");
        CHECK_EQ(saved_properties.next_sibling("w:tblPr"), pugi::xml_node{});
        const auto saved_width = saved_properties.child("w:tblW");
        REQUIRE(saved_width != pugi::xml_node{});
        CHECK_EQ(saved_width.next_sibling("w:tblW"), pugi::xml_node{});
        CHECK_EQ(std::string_view{saved_width.attribute("w:w").value()},
                 "7200");
        CHECK_EQ(std::string_view{saved_width.attribute("w:type").value()},
                 "dxa");
        if (fixture_state ==
            table_position_fixture_state::properties_without_width) {
            const auto saved_style = saved_properties.first_child();
            REQUIRE(saved_style != pugi::xml_node{});
            CHECK_EQ(std::string_view{saved_style.name()}, "w:tblStyle");
            CHECK_EQ(saved_style.next_sibling(), saved_width);
            CHECK_EQ(std::string_view{saved_width.next_sibling().name()},
                     "w:tblLayout");
        } else {
            CHECK_EQ(saved_properties.first_child(), saved_width);
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table width updates are atomic for every pugixml allocation failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("表宽XML基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto updated = false;
        {
            pugi_allocator_guard guard;
            updated = table.set_width_twips(7200U);
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(updated);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{make_test_path("表宽XML失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto first_cell = row.cells();
        auto second_cell = first_cell;
        second_cell.next();
        auto paragraph = first_cell.paragraphs();
        auto run = paragraph.runs();
        auto updated = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            pugi_allocator_guard guard;
            pugi_failure_call = failure_call;
            updated = table.set_width_twips(7200U);
            observed_failure_allocation_count = pugi_allocation_calls;
        }

        REQUIRE_FALSE(updated);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_FALSE(table.width_twips().has_value());
        REQUIRE(table.layout_mode().has_value());
        CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
        REQUIRE(first_cell.width_twips().has_value());
        CHECK_EQ(*first_cell.width_twips(), 1200U);
        REQUIRE(second_cell.width_twips().has_value());
        CHECK_EQ(*second_cell.width_twips(), 1200U);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.set_width_twips(7200U));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        REQUIRE(table.width_twips().has_value());
        CHECK_EQ(*table.width_twips(), 7200U);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table width node creation rolls back for every pugixml allocation "
    "failure") {
    constexpr auto first_padding_paragraph_count = std::size_t{101U};
    constexpr auto last_padding_paragraph_count = std::size_t{128U};
    constexpr auto scenarios = std::array{
        std::pair{table_position_fixture_state::no_properties,
                  std::string_view{"无表格属性"}},
        std::pair{table_position_fixture_state::properties_without_width,
                  std::string_view{"缺少表宽节点"}},
    };

    for (const auto &[fixture_state, scenario_suffix] : scenarios) {
        CAPTURE(scenario_suffix);
        auto padding_paragraph_count = std::size_t{0U};
        auto successful_allocation_count = std::size_t{0U};
        for (auto candidate = first_padding_paragraph_count;
             candidate <= last_padding_paragraph_count; ++candidate) {
            scoped_test_path path{make_test_path(
                std::string{"表宽节点创建XML基线"} +
                    std::string{scenario_suffix},
                candidate)};
            write_test_docx(
                path.path(),
                table_position_allocation_fixture_xml(candidate, fixture_state));
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            auto table = document.tables();
            auto updated = false;
            {
                pugi_allocator_guard guard;
                updated = table.set_width_twips(7200U);
                successful_allocation_count = pugi_allocation_calls;
            }
            REQUIRE(updated);
            if (successful_allocation_count > 0U) {
                padding_paragraph_count = candidate;
                break;
            }
        }
        REQUIRE_GT(successful_allocation_count, 0U);
        CAPTURE(padding_paragraph_count);
        const auto fixture_xml = table_position_allocation_fixture_xml(
            padding_paragraph_count, fixture_state);

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            scoped_test_path path{make_test_path(
                std::string{"表宽节点创建XML失败"} +
                    std::string{scenario_suffix},
                failure_call)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            REQUIRE_FALSE(document.save());
            const auto xml_before =
                read_test_docx_entry(path.path(), test_document_xml_entry);
            pugi::xml_document before_document;
            REQUIRE(before_document.load_string(xml_before.c_str()));
            const auto properties_before =
                before_document.child("w:document")
                    .child("w:body")
                    .child("w:tbl")
                    .child("w:tblPr");
            if (fixture_state ==
                table_position_fixture_state::properties_without_width) {
                REQUIRE(properties_before != pugi::xml_node{});
                CHECK_EQ(properties_before.child("w:tblW"), pugi::xml_node{});
                CHECK_EQ(
                    std::string_view{properties_before.child("w:tblLayout")
                                         .attribute("w:type")
                                         .value()},
                    "fixed");
            } else {
                CHECK_EQ(properties_before, pugi::xml_node{});
            }

            auto table = document.tables();
            auto row = table.rows();
            auto cell = row.cells();
            auto paragraph = cell.paragraphs();
            auto run = paragraph.runs();
            auto updated = true;
            auto observed_failure_allocation_count = std::size_t{0U};
            {
                pugi_allocator_guard guard;
                pugi_failure_call = failure_call;
                updated = table.set_width_twips(7200U);
                observed_failure_allocation_count = pugi_allocation_calls;
            }

            REQUIRE_FALSE(updated);
            REQUIRE_GE(observed_failure_allocation_count, failure_call);
            CHECK(table.valid());
            CHECK(row.valid());
            CHECK(cell.valid());
            CHECK(paragraph.valid());
            CHECK(run.valid());
            CHECK_FALSE(table.width_twips().has_value());
            if (fixture_state ==
                table_position_fixture_state::properties_without_width) {
                REQUIRE(table.layout_mode().has_value());
                CHECK_EQ(*table.layout_mode(),
                         featherdoc::table_layout_mode::fixed);
            } else {
                CHECK_FALSE(table.layout_mode().has_value());
            }
            REQUIRE(cell.width_twips().has_value());
            CHECK_EQ(*cell.width_twips(), 1200U);
            REQUIRE_FALSE(document.save());
            CHECK_EQ(
                read_test_docx_entry(path.path(), test_document_xml_entry),
                xml_before);

            REQUIRE(table.set_width_twips(7200U));
            CHECK(table.valid());
            CHECK(row.valid());
            CHECK(cell.valid());
            CHECK(paragraph.valid());
            CHECK(run.valid());
            REQUIRE(table.width_twips().has_value());
            CHECK_EQ(*table.width_twips(), 7200U);
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table width updates are atomic for every global allocation failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("表宽全局基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto updated = false;
        {
            global_allocation_guard guard{0U};
            updated = table.set_width_twips(7200U);
            successful_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }
        REQUIRE(updated);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{make_test_path("表宽全局失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto first_cell = row.cells();
        auto second_cell = first_cell;
        second_cell.next();
        auto paragraph = first_cell.paragraphs();
        auto run = paragraph.runs();
        auto updated = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            global_allocation_guard guard{failure_call};
            updated = table.set_width_twips(7200U);
            observed_failure_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }

        REQUIRE_FALSE(updated);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_FALSE(table.width_twips().has_value());
        REQUIRE(table.layout_mode().has_value());
        CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
        REQUIRE(first_cell.width_twips().has_value());
        CHECK_EQ(*first_cell.width_twips(), 1200U);
        REQUIRE(second_cell.width_twips().has_value());
        CHECK_EQ(*second_cell.width_twips(), 1200U);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.set_width_twips(7200U));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        REQUIRE(table.width_twips().has_value());
        CHECK_EQ(*table.width_twips(), 7200U);
    }
}

TEST_CASE(
    "table alignment updates preserve handles and unrelated XML content") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    scoped_test_path path{make_test_path("表格对齐句柄与扩展保留", 0U)};
    write_test_docx(path.path(), fixture_xml);
    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());

    auto table = document.tables();
    auto row = table.rows();
    auto first_cell = row.cells();
    auto second_cell = first_cell;
    second_cell.next();
    auto paragraph = first_cell.paragraphs();
    auto run = paragraph.runs();
    CHECK_FALSE(table.alignment().has_value());

    REQUIRE(table.set_alignment(featherdoc::table_alignment::center));
    CHECK(table.valid());
    CHECK(row.valid());
    CHECK(first_cell.valid());
    CHECK(second_cell.valid());
    CHECK(paragraph.valid());
    CHECK(run.valid());
    REQUIRE(table.alignment().has_value());
    CHECK_EQ(*table.alignment(), featherdoc::table_alignment::center);
    REQUIRE(table.layout_mode().has_value());
    CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
    REQUIRE(first_cell.width_twips().has_value());
    CHECK_EQ(*first_cell.width_twips(), 1200U);
    REQUIRE(second_cell.width_twips().has_value());
    CHECK_EQ(*second_cell.width_twips(), 1200U);

    REQUIRE_FALSE(document.save());
    const auto saved_xml =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_xml.c_str()));
    const auto saved_table =
        saved_document.child("w:document").child("w:body").child("w:tbl");
    const auto saved_properties = saved_table.child("w:tblPr");
    const auto saved_grid = saved_table.child("w:tblGrid");
    REQUIRE(saved_properties != pugi::xml_node{});
    REQUIRE(saved_grid != pugi::xml_node{});
    CHECK_FALSE(
        std::string_view{saved_properties.attribute("data-large").value()}
            .empty());
    CHECK_FALSE(
        std::string_view{saved_grid.attribute("data-large").value()}.empty());

    const auto saved_width = saved_properties.child("w:tblW");
    REQUIRE(saved_width != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_width.attribute("w:w").value()}, "0");
    CHECK_EQ(std::string_view{saved_width.attribute("w:type").value()}, "auto");
    const auto saved_alignment = saved_properties.child("w:jc");
    REQUIRE(saved_alignment != pugi::xml_node{});
    CHECK_EQ(saved_alignment.next_sibling("w:jc"), pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_alignment.attribute("w:val").value()},
             "center");
    const auto saved_layout = saved_properties.child("w:tblLayout");
    REQUIRE(saved_layout != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_layout.attribute("w:type").value()},
             "fixed");
    const auto saved_look = saved_properties.child("w:tblLook");
    REQUIRE(saved_look != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_look.attribute("w:val").value()}, "04A0");
    CHECK_EQ(std::string_view{saved_look.attribute("w:firstRow").value()}, "1");
    CHECK_EQ(std::string_view{saved_look.attribute("w:noVBand").value()}, "1");
    CHECK_EQ(saved_properties.first_child(), saved_width);
    CHECK_EQ(saved_width.next_sibling(), saved_alignment);
    CHECK_EQ(saved_alignment.next_sibling(), saved_layout);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table alignment updates are atomic for every pugixml allocation "
    "failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("表格对齐XML基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto updated = false;
        {
            pugi_allocator_guard guard;
            updated = table.set_alignment(featherdoc::table_alignment::center);
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(updated);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{make_test_path("表格对齐XML失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto first_cell = row.cells();
        auto second_cell = first_cell;
        second_cell.next();
        auto paragraph = first_cell.paragraphs();
        auto run = paragraph.runs();
        auto updated = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            pugi_allocator_guard guard;
            pugi_failure_call = failure_call;
            updated = table.set_alignment(featherdoc::table_alignment::center);
            observed_failure_allocation_count = pugi_allocation_calls;
        }

        REQUIRE_FALSE(updated);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_FALSE(table.alignment().has_value());
        REQUIRE(table.layout_mode().has_value());
        CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
        REQUIRE(first_cell.width_twips().has_value());
        CHECK_EQ(*first_cell.width_twips(), 1200U);
        REQUIRE(second_cell.width_twips().has_value());
        CHECK_EQ(*second_cell.width_twips(), 1200U);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.set_alignment(featherdoc::table_alignment::center));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        REQUIRE(table.alignment().has_value());
        CHECK_EQ(*table.alignment(), featherdoc::table_alignment::center);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table alignment updates are atomic for every global allocation failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("表格对齐全局基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto updated = false;
        {
            global_allocation_guard guard{0U};
            updated = table.set_alignment(featherdoc::table_alignment::center);
            successful_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }
        REQUIRE(updated);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{make_test_path("表格对齐全局失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto first_cell = row.cells();
        auto second_cell = first_cell;
        second_cell.next();
        auto paragraph = first_cell.paragraphs();
        auto run = paragraph.runs();
        auto updated = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            global_allocation_guard guard{failure_call};
            updated = table.set_alignment(featherdoc::table_alignment::center);
            observed_failure_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }

        REQUIRE_FALSE(updated);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_FALSE(table.alignment().has_value());
        REQUIRE(table.layout_mode().has_value());
        CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
        REQUIRE(first_cell.width_twips().has_value());
        CHECK_EQ(*first_cell.width_twips(), 1200U);
        REQUIRE(second_cell.width_twips().has_value());
        CHECK_EQ(*second_cell.width_twips(), 1200U);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.set_alignment(featherdoc::table_alignment::center));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        REQUIRE(table.alignment().has_value());
        CHECK_EQ(*table.alignment(), featherdoc::table_alignment::center);
    }
}

TEST_CASE("table indent updates preserve handles and unrelated XML content") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    scoped_test_path path{make_test_path("表格缩进句柄与扩展保留", 0U)};
    write_test_docx(path.path(), fixture_xml);
    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());

    auto table = document.tables();
    auto row = table.rows();
    auto first_cell = row.cells();
    auto second_cell = first_cell;
    second_cell.next();
    auto paragraph = first_cell.paragraphs();
    auto run = paragraph.runs();
    CHECK_FALSE(table.indent_twips().has_value());

    REQUIRE(table.set_indent_twips(720U));
    CHECK(table.valid());
    CHECK(row.valid());
    CHECK(first_cell.valid());
    CHECK(second_cell.valid());
    CHECK(paragraph.valid());
    CHECK(run.valid());
    REQUIRE(table.indent_twips().has_value());
    CHECK_EQ(*table.indent_twips(), 720U);
    REQUIRE(table.layout_mode().has_value());
    CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
    REQUIRE(first_cell.width_twips().has_value());
    CHECK_EQ(*first_cell.width_twips(), 1200U);
    REQUIRE(second_cell.width_twips().has_value());
    CHECK_EQ(*second_cell.width_twips(), 1200U);

    REQUIRE_FALSE(document.save());
    const auto saved_xml =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_xml.c_str()));
    const auto saved_table =
        saved_document.child("w:document").child("w:body").child("w:tbl");
    const auto saved_properties = saved_table.child("w:tblPr");
    const auto saved_grid = saved_table.child("w:tblGrid");
    REQUIRE(saved_properties != pugi::xml_node{});
    REQUIRE(saved_grid != pugi::xml_node{});
    CHECK_FALSE(
        std::string_view{saved_properties.attribute("data-large").value()}
            .empty());
    CHECK_FALSE(
        std::string_view{saved_grid.attribute("data-large").value()}.empty());

    const auto saved_width = saved_properties.child("w:tblW");
    REQUIRE(saved_width != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_width.attribute("w:w").value()}, "0");
    CHECK_EQ(std::string_view{saved_width.attribute("w:type").value()}, "auto");
    const auto saved_indent = saved_properties.child("w:tblInd");
    REQUIRE(saved_indent != pugi::xml_node{});
    CHECK_EQ(saved_indent.next_sibling("w:tblInd"), pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_indent.attribute("w:w").value()}, "720");
    CHECK_EQ(std::string_view{saved_indent.attribute("w:type").value()}, "dxa");
    const auto saved_layout = saved_properties.child("w:tblLayout");
    REQUIRE(saved_layout != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_layout.attribute("w:type").value()},
             "fixed");
    const auto saved_look = saved_properties.child("w:tblLook");
    REQUIRE(saved_look != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_look.attribute("w:val").value()}, "04A0");
    CHECK_EQ(std::string_view{saved_look.attribute("w:firstRow").value()}, "1");
    CHECK_EQ(std::string_view{saved_look.attribute("w:noVBand").value()}, "1");
    CHECK_EQ(saved_properties.first_child(), saved_width);
    CHECK_EQ(saved_width.next_sibling(), saved_indent);
    CHECK_EQ(saved_indent.next_sibling(), saved_layout);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table indent updates are atomic for every pugixml allocation failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("表格缩进XML基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto updated = false;
        {
            pugi_allocator_guard guard;
            updated = table.set_indent_twips(720U);
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(updated);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{make_test_path("表格缩进XML失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto first_cell = row.cells();
        auto second_cell = first_cell;
        second_cell.next();
        auto paragraph = first_cell.paragraphs();
        auto run = paragraph.runs();
        auto updated = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            pugi_allocator_guard guard;
            pugi_failure_call = failure_call;
            updated = table.set_indent_twips(720U);
            observed_failure_allocation_count = pugi_allocation_calls;
        }

        REQUIRE_FALSE(updated);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_FALSE(table.indent_twips().has_value());
        REQUIRE(table.layout_mode().has_value());
        CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
        REQUIRE(first_cell.width_twips().has_value());
        CHECK_EQ(*first_cell.width_twips(), 1200U);
        REQUIRE(second_cell.width_twips().has_value());
        CHECK_EQ(*second_cell.width_twips(), 1200U);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.set_indent_twips(720U));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        REQUIRE(table.indent_twips().has_value());
        CHECK_EQ(*table.indent_twips(), 720U);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table indent updates are atomic for every global allocation failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("表格缩进全局基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto updated = false;
        {
            global_allocation_guard guard{0U};
            updated = table.set_indent_twips(720U);
            successful_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }
        REQUIRE(updated);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{make_test_path("表格缩进全局失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto first_cell = row.cells();
        auto second_cell = first_cell;
        second_cell.next();
        auto paragraph = first_cell.paragraphs();
        auto run = paragraph.runs();
        auto updated = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            global_allocation_guard guard{failure_call};
            updated = table.set_indent_twips(720U);
            observed_failure_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }

        REQUIRE_FALSE(updated);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_FALSE(table.indent_twips().has_value());
        REQUIRE(table.layout_mode().has_value());
        CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
        REQUIRE(first_cell.width_twips().has_value());
        CHECK_EQ(*first_cell.width_twips(), 1200U);
        REQUIRE(second_cell.width_twips().has_value());
        CHECK_EQ(*second_cell.width_twips(), 1200U);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.set_indent_twips(720U));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        REQUIRE(table.indent_twips().has_value());
        CHECK_EQ(*table.indent_twips(), 720U);
    }
}

TEST_CASE(
    "table cell spacing updates preserve handles and unrelated XML content") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    scoped_test_path path{make_test_path("表格间距句柄与扩展保留", 0U)};
    write_test_docx(path.path(), fixture_xml);
    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());

    auto table = document.tables();
    auto row = table.rows();
    auto first_cell = row.cells();
    auto second_cell = first_cell;
    second_cell.next();
    auto paragraph = first_cell.paragraphs();
    auto run = paragraph.runs();
    CHECK_FALSE(table.cell_spacing_twips().has_value());

    REQUIRE(table.set_cell_spacing_twips(180U));
    CHECK(table.valid());
    CHECK(row.valid());
    CHECK(first_cell.valid());
    CHECK(second_cell.valid());
    CHECK(paragraph.valid());
    CHECK(run.valid());
    REQUIRE(table.cell_spacing_twips().has_value());
    CHECK_EQ(*table.cell_spacing_twips(), 180U);
    REQUIRE(table.layout_mode().has_value());
    CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
    REQUIRE(first_cell.width_twips().has_value());
    CHECK_EQ(*first_cell.width_twips(), 1200U);
    REQUIRE(second_cell.width_twips().has_value());
    CHECK_EQ(*second_cell.width_twips(), 1200U);

    REQUIRE_FALSE(document.save());
    const auto saved_xml =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_xml.c_str()));
    const auto saved_table =
        saved_document.child("w:document").child("w:body").child("w:tbl");
    const auto saved_properties = saved_table.child("w:tblPr");
    const auto saved_grid = saved_table.child("w:tblGrid");
    REQUIRE(saved_properties != pugi::xml_node{});
    REQUIRE(saved_grid != pugi::xml_node{});
    CHECK_FALSE(
        std::string_view{saved_properties.attribute("data-large").value()}
            .empty());
    CHECK_FALSE(
        std::string_view{saved_grid.attribute("data-large").value()}.empty());

    const auto saved_width = saved_properties.child("w:tblW");
    REQUIRE(saved_width != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_width.attribute("w:w").value()}, "0");
    CHECK_EQ(std::string_view{saved_width.attribute("w:type").value()}, "auto");
    const auto saved_spacing = saved_properties.child("w:tblCellSpacing");
    REQUIRE(saved_spacing != pugi::xml_node{});
    CHECK_EQ(saved_spacing.next_sibling("w:tblCellSpacing"), pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_spacing.attribute("w:w").value()}, "180");
    CHECK_EQ(std::string_view{saved_spacing.attribute("w:type").value()},
             "dxa");
    const auto saved_layout = saved_properties.child("w:tblLayout");
    REQUIRE(saved_layout != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_layout.attribute("w:type").value()},
             "fixed");
    const auto saved_look = saved_properties.child("w:tblLook");
    REQUIRE(saved_look != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_look.attribute("w:val").value()}, "04A0");
    CHECK_EQ(std::string_view{saved_look.attribute("w:firstRow").value()}, "1");
    CHECK_EQ(std::string_view{saved_look.attribute("w:noVBand").value()}, "1");
    CHECK_EQ(saved_properties.first_child(), saved_width);
    CHECK_EQ(saved_width.next_sibling(), saved_spacing);
    CHECK_EQ(saved_spacing.next_sibling(), saved_layout);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table cell spacing updates are atomic for every pugixml allocation "
    "failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("表格间距XML基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto updated = false;
        {
            pugi_allocator_guard guard;
            updated = table.set_cell_spacing_twips(180U);
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(updated);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{make_test_path("表格间距XML失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto first_cell = row.cells();
        auto second_cell = first_cell;
        second_cell.next();
        auto paragraph = first_cell.paragraphs();
        auto run = paragraph.runs();
        auto updated = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            pugi_allocator_guard guard;
            pugi_failure_call = failure_call;
            updated = table.set_cell_spacing_twips(180U);
            observed_failure_allocation_count = pugi_allocation_calls;
        }

        REQUIRE_FALSE(updated);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_FALSE(table.cell_spacing_twips().has_value());
        REQUIRE(table.layout_mode().has_value());
        CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
        REQUIRE(first_cell.width_twips().has_value());
        CHECK_EQ(*first_cell.width_twips(), 1200U);
        REQUIRE(second_cell.width_twips().has_value());
        CHECK_EQ(*second_cell.width_twips(), 1200U);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.set_cell_spacing_twips(180U));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        REQUIRE(table.cell_spacing_twips().has_value());
        CHECK_EQ(*table.cell_spacing_twips(), 180U);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table cell spacing updates are atomic for every global allocation "
    "failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("表格间距全局基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto updated = false;
        {
            global_allocation_guard guard{0U};
            updated = table.set_cell_spacing_twips(180U);
            successful_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }
        REQUIRE(updated);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{make_test_path("表格间距全局失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto first_cell = row.cells();
        auto second_cell = first_cell;
        second_cell.next();
        auto paragraph = first_cell.paragraphs();
        auto run = paragraph.runs();
        auto updated = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            global_allocation_guard guard{failure_call};
            updated = table.set_cell_spacing_twips(180U);
            observed_failure_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }

        REQUIRE_FALSE(updated);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_FALSE(table.cell_spacing_twips().has_value());
        REQUIRE(table.layout_mode().has_value());
        CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
        REQUIRE(first_cell.width_twips().has_value());
        CHECK_EQ(*first_cell.width_twips(), 1200U);
        REQUIRE(second_cell.width_twips().has_value());
        CHECK_EQ(*second_cell.width_twips(), 1200U);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.set_cell_spacing_twips(180U));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        REQUIRE(table.cell_spacing_twips().has_value());
        CHECK_EQ(*table.cell_spacing_twips(), 180U);
    }
}

TEST_CASE(
    "table cell margin updates preserve handles and unrelated XML content") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    scoped_test_path path{make_test_path("表格边距句柄与扩展保留", 0U)};
    write_test_docx(path.path(), fixture_xml);
    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());

    auto table = document.tables();
    auto row = table.rows();
    auto first_cell = row.cells();
    auto second_cell = first_cell;
    second_cell.next();
    auto paragraph = first_cell.paragraphs();
    auto run = paragraph.runs();
    CHECK_FALSE(
        table.cell_margin_twips(featherdoc::cell_margin_edge::top).has_value());

    REQUIRE(
        table.set_cell_margin_twips(featherdoc::cell_margin_edge::top, 120U));
    CHECK(table.valid());
    CHECK(row.valid());
    CHECK(first_cell.valid());
    CHECK(second_cell.valid());
    CHECK(paragraph.valid());
    CHECK(run.valid());
    REQUIRE(
        table.cell_margin_twips(featherdoc::cell_margin_edge::top).has_value());
    CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::top), 120U);
    REQUIRE(table.layout_mode().has_value());
    CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
    REQUIRE(first_cell.width_twips().has_value());
    CHECK_EQ(*first_cell.width_twips(), 1200U);
    REQUIRE(second_cell.width_twips().has_value());
    CHECK_EQ(*second_cell.width_twips(), 1200U);

    REQUIRE_FALSE(document.save());
    const auto saved_xml =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_xml.c_str()));
    const auto saved_table =
        saved_document.child("w:document").child("w:body").child("w:tbl");
    const auto saved_properties = saved_table.child("w:tblPr");
    const auto saved_grid = saved_table.child("w:tblGrid");
    REQUIRE(saved_properties != pugi::xml_node{});
    REQUIRE(saved_grid != pugi::xml_node{});
    CHECK_FALSE(
        std::string_view{saved_properties.attribute("data-large").value()}
            .empty());
    CHECK_FALSE(
        std::string_view{saved_grid.attribute("data-large").value()}.empty());

    const auto saved_width = saved_properties.child("w:tblW");
    REQUIRE(saved_width != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_width.attribute("w:w").value()}, "0");
    CHECK_EQ(std::string_view{saved_width.attribute("w:type").value()}, "auto");
    const auto saved_layout = saved_properties.child("w:tblLayout");
    REQUIRE(saved_layout != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_layout.attribute("w:type").value()},
             "fixed");
    const auto saved_margins = saved_properties.child("w:tblCellMar");
    REQUIRE(saved_margins != pugi::xml_node{});
    CHECK_EQ(saved_margins.next_sibling("w:tblCellMar"), pugi::xml_node{});
    const auto saved_top = saved_margins.child("w:top");
    REQUIRE(saved_top != pugi::xml_node{});
    CHECK_EQ(saved_top.next_sibling("w:top"), pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_top.attribute("w:w").value()}, "120");
    CHECK_EQ(std::string_view{saved_top.attribute("w:type").value()}, "dxa");
    const auto saved_look = saved_properties.child("w:tblLook");
    REQUIRE(saved_look != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_look.attribute("w:val").value()}, "04A0");
    CHECK_EQ(std::string_view{saved_look.attribute("w:firstRow").value()}, "1");
    CHECK_EQ(std::string_view{saved_look.attribute("w:noVBand").value()}, "1");
    CHECK_EQ(saved_properties.first_child(), saved_width);
    CHECK_EQ(saved_width.next_sibling(), saved_layout);
    CHECK_EQ(saved_layout.next_sibling(), saved_margins);
    CHECK_EQ(saved_margins.next_sibling(), saved_look);
    CHECK_EQ(saved_margins.first_child(), saved_top);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table cell margin updates are atomic for every pugixml allocation "
    "failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("表格边距XML基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto updated = false;
        {
            pugi_allocator_guard guard;
            updated = table.set_cell_margin_twips(
                featherdoc::cell_margin_edge::top, 120U);
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(updated);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{make_test_path("表格边距XML失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto first_cell = row.cells();
        auto second_cell = first_cell;
        second_cell.next();
        auto paragraph = first_cell.paragraphs();
        auto run = paragraph.runs();
        auto updated = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            pugi_allocator_guard guard;
            pugi_failure_call = failure_call;
            updated = table.set_cell_margin_twips(
                featherdoc::cell_margin_edge::top, 120U);
            observed_failure_allocation_count = pugi_allocation_calls;
        }

        REQUIRE_FALSE(updated);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_FALSE(table.cell_margin_twips(featherdoc::cell_margin_edge::top)
                        .has_value());
        REQUIRE(table.layout_mode().has_value());
        CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
        REQUIRE(first_cell.width_twips().has_value());
        CHECK_EQ(*first_cell.width_twips(), 1200U);
        REQUIRE(second_cell.width_twips().has_value());
        CHECK_EQ(*second_cell.width_twips(), 1200U);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.set_cell_margin_twips(featherdoc::cell_margin_edge::top,
                                            120U));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        REQUIRE(table.cell_margin_twips(featherdoc::cell_margin_edge::top)
                    .has_value());
        CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::top),
                 120U);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table cell margin updates are atomic for every global allocation "
    "failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("表格边距全局基线", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto updated = false;
        {
            global_allocation_guard guard{0U};
            updated = table.set_cell_margin_twips(
                featherdoc::cell_margin_edge::top, 120U);
            successful_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }
        REQUIRE(updated);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{make_test_path("表格边距全局失败", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto first_cell = row.cells();
        auto second_cell = first_cell;
        second_cell.next();
        auto paragraph = first_cell.paragraphs();
        auto run = paragraph.runs();
        auto updated = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            global_allocation_guard guard{failure_call};
            updated = table.set_cell_margin_twips(
                featherdoc::cell_margin_edge::top, 120U);
            observed_failure_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }

        REQUIRE_FALSE(updated);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_FALSE(table.cell_margin_twips(featherdoc::cell_margin_edge::top)
                        .has_value());
        REQUIRE(table.layout_mode().has_value());
        CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
        REQUIRE(first_cell.width_twips().has_value());
        CHECK_EQ(*first_cell.width_twips(), 1200U);
        REQUIRE(second_cell.width_twips().has_value());
        CHECK_EQ(*second_cell.width_twips(), 1200U);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.set_cell_margin_twips(featherdoc::cell_margin_edge::top,
                                            120U));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        REQUIRE(table.cell_margin_twips(featherdoc::cell_margin_edge::top)
                    .has_value());
        CHECK_EQ(*table.cell_margin_twips(featherdoc::cell_margin_edge::top),
                 120U);
    }
}

TEST_CASE("table style id updates preserve handles and unrelated XML content") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    const auto styles_xml = table_style_id_fixture_styles_xml();
    scoped_test_path path{make_test_path("table-style-id-handles", 0U)};
    write_test_docx_with_styles(path.path(), fixture_xml, styles_xml);
    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());
    REQUIRE_GE(document.list_styles().size(), 3U);

    auto table = document.tables();
    auto row = table.rows();
    auto first_cell = row.cells();
    auto second_cell = first_cell;
    second_cell.next();
    auto paragraph = first_cell.paragraphs();
    auto run = paragraph.runs();
    CHECK_FALSE(table.style_id().has_value());

    REQUIRE(table.set_style_id("AtomicTableStyle"));
    CHECK(table.valid());
    CHECK(row.valid());
    CHECK(first_cell.valid());
    CHECK(second_cell.valid());
    CHECK(paragraph.valid());
    CHECK(run.valid());
    REQUIRE(table.style_id().has_value());
    CHECK_EQ(*table.style_id(), "AtomicTableStyle");
    REQUIRE(table.layout_mode().has_value());
    CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
    REQUIRE(first_cell.width_twips().has_value());
    CHECK_EQ(*first_cell.width_twips(), 1200U);
    REQUIRE(second_cell.width_twips().has_value());
    CHECK_EQ(*second_cell.width_twips(), 1200U);

    REQUIRE_FALSE(document.save());
    const auto saved_xml =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_xml.c_str()));
    const auto saved_table =
        saved_document.child("w:document").child("w:body").child("w:tbl");
    const auto saved_properties = saved_table.child("w:tblPr");
    const auto saved_grid = saved_table.child("w:tblGrid");
    REQUIRE(saved_properties != pugi::xml_node{});
    REQUIRE(saved_grid != pugi::xml_node{});
    CHECK_FALSE(
        std::string_view{saved_properties.attribute("data-large").value()}
            .empty());
    CHECK_FALSE(
        std::string_view{saved_grid.attribute("data-large").value()}.empty());

    const auto saved_style = saved_properties.child("w:tblStyle");
    REQUIRE(saved_style != pugi::xml_node{});
    CHECK_EQ(saved_style.next_sibling("w:tblStyle"), pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_style.attribute("w:val").value()},
             "AtomicTableStyle");
    const auto saved_width = saved_properties.child("w:tblW");
    REQUIRE(saved_width != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_width.attribute("w:w").value()}, "0");
    CHECK_EQ(std::string_view{saved_width.attribute("w:type").value()}, "auto");
    const auto saved_layout = saved_properties.child("w:tblLayout");
    REQUIRE(saved_layout != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_layout.attribute("w:type").value()},
             "fixed");
    const auto saved_look = saved_properties.child("w:tblLook");
    REQUIRE(saved_look != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_look.attribute("w:val").value()}, "04A0");
    CHECK_EQ(std::string_view{saved_look.attribute("w:firstRow").value()}, "1");
    CHECK_EQ(std::string_view{saved_look.attribute("w:noVBand").value()}, "1");
    CHECK_EQ(saved_properties.first_child(), saved_style);
    CHECK_EQ(saved_style.next_sibling(), saved_width);
    CHECK_EQ(saved_width.next_sibling(), saved_layout);
    CHECK_EQ(saved_layout.next_sibling(), saved_look);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table style id updates are atomic for every pugixml allocation failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(
        2U, 2U, "fixed", "1200", "OldTableStyle");
    const auto styles_xml = table_style_id_fixture_styles_xml();
    auto updated_style_id = std::string{"AtomicTableStyle-"};
    updated_style_id += large_cell_property_value();
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{
            make_test_path("table-style-id-xml-baseline", 0U)};
        write_test_docx_with_styles(path.path(), fixture_xml, styles_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_GE(document.list_styles().size(), 3U);
        auto table = document.tables();
        auto updated = false;
        {
            pugi_allocator_guard guard;
            updated = table.set_style_id(updated_style_id);
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(updated);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{
            make_test_path("table-style-id-xml-failure", failure_call)};
        write_test_docx_with_styles(path.path(), fixture_xml, styles_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_GE(document.list_styles().size(), 3U);
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto first_cell = row.cells();
        auto second_cell = first_cell;
        second_cell.next();
        auto paragraph = first_cell.paragraphs();
        auto run = paragraph.runs();
        auto updated = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            pugi_allocator_guard guard;
            pugi_failure_call = failure_call;
            updated = table.set_style_id(updated_style_id);
            observed_failure_allocation_count = pugi_allocation_calls;
        }

        REQUIRE_FALSE(updated);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        REQUIRE(table.style_id().has_value());
        CHECK_EQ(*table.style_id(), "OldTableStyle");
        REQUIRE(table.layout_mode().has_value());
        CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
        REQUIRE(first_cell.width_twips().has_value());
        CHECK_EQ(*first_cell.width_twips(), 1200U);
        REQUIRE(second_cell.width_twips().has_value());
        CHECK_EQ(*second_cell.width_twips(), 1200U);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.set_style_id(updated_style_id));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        REQUIRE(table.style_id().has_value());
        CHECK_EQ(*table.style_id(), updated_style_id);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table style id updates are atomic for every global allocation failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(
        2U, 2U, "fixed", "1200", "OldTableStyle");
    const auto styles_xml = table_style_id_fixture_styles_xml();
    auto updated_style_id = std::string{"AtomicTableStyle-"};
    updated_style_id += large_cell_property_value();
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{
            make_test_path("table-style-id-global-baseline", 0U)};
        write_test_docx_with_styles(path.path(), fixture_xml, styles_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_GE(document.list_styles().size(), 3U);
        auto table = document.tables();
        auto updated = false;
        {
            global_allocation_guard guard{0U};
            updated = table.set_style_id(updated_style_id);
            successful_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }
        REQUIRE(updated);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{
            make_test_path("table-style-id-global-failure", failure_call)};
        write_test_docx_with_styles(path.path(), fixture_xml, styles_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_GE(document.list_styles().size(), 3U);
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto first_cell = row.cells();
        auto second_cell = first_cell;
        second_cell.next();
        auto paragraph = first_cell.paragraphs();
        auto run = paragraph.runs();
        auto updated = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            global_allocation_guard guard{failure_call};
            updated = table.set_style_id(updated_style_id);
            observed_failure_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }

        REQUIRE_FALSE(updated);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        REQUIRE(table.style_id().has_value());
        CHECK_EQ(*table.style_id(), "OldTableStyle");
        REQUIRE(table.layout_mode().has_value());
        CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
        REQUIRE(first_cell.width_twips().has_value());
        CHECK_EQ(*first_cell.width_twips(), 1200U);
        REQUIRE(second_cell.width_twips().has_value());
        CHECK_EQ(*second_cell.width_twips(), 1200U);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.set_style_id(updated_style_id));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        REQUIRE(table.style_id().has_value());
        CHECK_EQ(*table.style_id(), updated_style_id);
    }
}

TEST_CASE(
    "table style look updates preserve handles and unrelated XML content") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    scoped_test_path path{make_test_path("table-style-look-handles", 0U)};
    write_test_docx(path.path(), fixture_xml);
    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());

    auto table = document.tables();
    auto row = table.rows();
    auto first_cell = row.cells();
    auto second_cell = first_cell;
    second_cell.next();
    auto paragraph = first_cell.paragraphs();
    auto run = paragraph.runs();
    const auto initial_style_look = table.style_look();
    REQUIRE(initial_style_look.has_value());
    CHECK(initial_style_look->first_row);
    CHECK_FALSE(initial_style_look->last_row);
    CHECK(initial_style_look->first_column);
    CHECK_FALSE(initial_style_look->last_column);
    CHECK(initial_style_look->banded_rows);
    CHECK_FALSE(initial_style_look->banded_columns);

    const auto replacement = replacement_table_style_look();
    REQUIRE(table.set_style_look(replacement));
    CHECK(table.valid());
    CHECK(row.valid());
    CHECK(first_cell.valid());
    CHECK(second_cell.valid());
    CHECK(paragraph.valid());
    CHECK(run.valid());
    const auto updated_style_look = table.style_look();
    REQUIRE(updated_style_look.has_value());
    CHECK_FALSE(updated_style_look->first_row);
    CHECK(updated_style_look->last_row);
    CHECK_FALSE(updated_style_look->first_column);
    CHECK(updated_style_look->last_column);
    CHECK_FALSE(updated_style_look->banded_rows);
    CHECK(updated_style_look->banded_columns);
    REQUIRE(table.layout_mode().has_value());
    CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
    REQUIRE(first_cell.width_twips().has_value());
    CHECK_EQ(*first_cell.width_twips(), 1200U);
    REQUIRE(second_cell.width_twips().has_value());
    CHECK_EQ(*second_cell.width_twips(), 1200U);

    REQUIRE_FALSE(document.save());
    const auto saved_xml =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_xml.c_str()));
    const auto saved_table =
        saved_document.child("w:document").child("w:body").child("w:tbl");
    const auto saved_properties = saved_table.child("w:tblPr");
    const auto saved_grid = saved_table.child("w:tblGrid");
    REQUIRE(saved_properties != pugi::xml_node{});
    REQUIRE(saved_grid != pugi::xml_node{});
    CHECK_FALSE(
        std::string_view{saved_properties.attribute("data-large").value()}
            .empty());
    CHECK_FALSE(
        std::string_view{saved_grid.attribute("data-large").value()}.empty());

    const auto saved_width = saved_properties.child("w:tblW");
    REQUIRE(saved_width != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_width.attribute("w:w").value()}, "0");
    CHECK_EQ(std::string_view{saved_width.attribute("w:type").value()}, "auto");
    const auto saved_layout = saved_properties.child("w:tblLayout");
    REQUIRE(saved_layout != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_layout.attribute("w:type").value()},
             "fixed");
    const auto saved_look = saved_properties.child("w:tblLook");
    REQUIRE(saved_look != pugi::xml_node{});
    CHECK_EQ(saved_look.next_sibling("w:tblLook"), pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_look.attribute("w:val").value()}, "0340");
    CHECK_EQ(std::string_view{saved_look.attribute("w:firstRow").value()}, "0");
    CHECK_EQ(std::string_view{saved_look.attribute("w:lastRow").value()}, "1");
    CHECK_EQ(std::string_view{saved_look.attribute("w:firstColumn").value()},
             "0");
    CHECK_EQ(std::string_view{saved_look.attribute("w:lastColumn").value()},
             "1");
    CHECK_EQ(std::string_view{saved_look.attribute("w:noHBand").value()}, "1");
    CHECK_EQ(std::string_view{saved_look.attribute("w:noVBand").value()}, "0");
    CHECK_EQ(saved_properties.first_child(), saved_width);
    CHECK_EQ(saved_width.next_sibling(), saved_layout);
    CHECK_EQ(saved_layout.next_sibling(), saved_look);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table style look updates are atomic for every pugixml allocation "
    "failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(
        2U, 2U, "fixed", "1200", std::string_view{}, false);
    const auto replacement = replacement_table_style_look();
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{
            make_test_path("table-style-look-xml-baseline", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto updated = false;
        {
            pugi_allocator_guard guard;
            updated = table.set_style_look(replacement);
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(updated);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{
            make_test_path("table-style-look-xml-failure", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto first_cell = row.cells();
        auto second_cell = first_cell;
        second_cell.next();
        auto paragraph = first_cell.paragraphs();
        auto run = paragraph.runs();
        auto updated = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            pugi_allocator_guard guard;
            pugi_failure_call = failure_call;
            updated = table.set_style_look(replacement);
            observed_failure_allocation_count = pugi_allocation_calls;
        }

        REQUIRE_FALSE(updated);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_FALSE(table.style_look().has_value());
        REQUIRE(table.layout_mode().has_value());
        CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
        REQUIRE(first_cell.width_twips().has_value());
        CHECK_EQ(*first_cell.width_twips(), 1200U);
        REQUIRE(second_cell.width_twips().has_value());
        CHECK_EQ(*second_cell.width_twips(), 1200U);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.set_style_look(replacement));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        const auto updated_style_look = table.style_look();
        REQUIRE(updated_style_look.has_value());
        CHECK_FALSE(updated_style_look->first_row);
        CHECK(updated_style_look->last_row);
        CHECK_FALSE(updated_style_look->first_column);
        CHECK(updated_style_look->last_column);
        CHECK_FALSE(updated_style_look->banded_rows);
        CHECK(updated_style_look->banded_columns);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table style look updates are atomic for every global allocation "
    "failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(
        2U, 2U, "fixed", "1200", std::string_view{}, false);
    const auto replacement = replacement_table_style_look();
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{
            make_test_path("table-style-look-global-baseline", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto updated = false;
        {
            global_allocation_guard guard{0U};
            updated = table.set_style_look(replacement);
            successful_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }
        REQUIRE(updated);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{
            make_test_path("table-style-look-global-failure", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto first_cell = row.cells();
        auto second_cell = first_cell;
        second_cell.next();
        auto paragraph = first_cell.paragraphs();
        auto run = paragraph.runs();
        auto updated = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            global_allocation_guard guard{failure_call};
            updated = table.set_style_look(replacement);
            observed_failure_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }

        REQUIRE_FALSE(updated);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_FALSE(table.style_look().has_value());
        REQUIRE(table.layout_mode().has_value());
        CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
        REQUIRE(first_cell.width_twips().has_value());
        CHECK_EQ(*first_cell.width_twips(), 1200U);
        REQUIRE(second_cell.width_twips().has_value());
        CHECK_EQ(*second_cell.width_twips(), 1200U);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(table.set_style_look(replacement));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        const auto updated_style_look = table.style_look();
        REQUIRE(updated_style_look.has_value());
        CHECK_FALSE(updated_style_look->first_row);
        CHECK(updated_style_look->last_row);
        CHECK_FALSE(updated_style_look->first_column);
        CHECK(updated_style_look->last_column);
        CHECK_FALSE(updated_style_look->banded_rows);
        CHECK(updated_style_look->banded_columns);
    }
}

TEST_CASE("table border updates preserve handles and unrelated XML content") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    scoped_test_path path{make_test_path("table-border-handles", 0U)};
    write_test_docx(path.path(), fixture_xml);
    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());

    auto table = document.tables();
    auto row = table.rows();
    auto first_cell = row.cells();
    auto second_cell = first_cell;
    second_cell.next();
    auto paragraph = first_cell.paragraphs();
    auto run = paragraph.runs();
    CHECK_FALSE(table.border(featherdoc::table_border_edge::top).has_value());

    const auto replacement = replacement_table_border();
    REQUIRE(table.set_border(featherdoc::table_border_edge::top, replacement));
    CHECK(table.valid());
    CHECK(row.valid());
    CHECK(first_cell.valid());
    CHECK(second_cell.valid());
    CHECK(paragraph.valid());
    CHECK(run.valid());
    const auto updated_border =
        table.border(featherdoc::table_border_edge::top);
    REQUIRE(updated_border.has_value());
    CHECK_EQ(updated_border->style, featherdoc::border_style::double_line);
    CHECK_EQ(updated_border->size_eighth_points, 16U);
    CHECK_EQ(updated_border->color, "12AB34");
    CHECK_EQ(updated_border->space_points, 2U);
    REQUIRE(table.layout_mode().has_value());
    CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
    REQUIRE(first_cell.width_twips().has_value());
    CHECK_EQ(*first_cell.width_twips(), 1200U);
    REQUIRE(second_cell.width_twips().has_value());
    CHECK_EQ(*second_cell.width_twips(), 1200U);

    REQUIRE_FALSE(document.save());
    const auto saved_xml =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_xml.c_str()));
    const auto saved_table =
        saved_document.child("w:document").child("w:body").child("w:tbl");
    const auto saved_properties = saved_table.child("w:tblPr");
    const auto saved_grid = saved_table.child("w:tblGrid");
    REQUIRE(saved_properties != pugi::xml_node{});
    REQUIRE(saved_grid != pugi::xml_node{});
    CHECK_FALSE(
        std::string_view{saved_properties.attribute("data-large").value()}
            .empty());
    CHECK_FALSE(
        std::string_view{saved_grid.attribute("data-large").value()}.empty());

    const auto saved_layout = saved_properties.child("w:tblLayout");
    const auto saved_look = saved_properties.child("w:tblLook");
    const auto saved_borders = saved_properties.child("w:tblBorders");
    REQUIRE(saved_layout != pugi::xml_node{});
    REQUIRE(saved_look != pugi::xml_node{});
    REQUIRE(saved_borders != pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_layout.attribute("w:type").value()},
             "fixed");
    CHECK_EQ(saved_look.next_sibling(), saved_borders);
    const auto saved_top_border = saved_borders.child("w:top");
    REQUIRE(saved_top_border != pugi::xml_node{});
    CHECK_EQ(saved_top_border.next_sibling("w:top"), pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_top_border.attribute("w:val").value()},
             "double");
    CHECK_EQ(std::string_view{saved_top_border.attribute("w:sz").value()},
             "16");
    CHECK_EQ(std::string_view{saved_top_border.attribute("w:space").value()},
             "2");
    CHECK_EQ(std::string_view{saved_top_border.attribute("w:color").value()},
             "12AB34");
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table border updates are atomic for every pugixml allocation failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    const auto replacement = replacement_table_border();
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{make_test_path("table-border-xml-baseline", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto updated = false;
        {
            pugi_allocator_guard guard;
            updated = table.set_border(featherdoc::table_border_edge::top,
                                       replacement);
            successful_allocation_count = pugi_allocation_calls;
        }
        REQUIRE(updated);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{
            make_test_path("table-border-xml-failure", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto first_cell = row.cells();
        auto second_cell = first_cell;
        second_cell.next();
        auto paragraph = first_cell.paragraphs();
        auto run = paragraph.runs();
        auto updated = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            pugi_allocator_guard guard;
            pugi_failure_call = failure_call;
            updated = table.set_border(featherdoc::table_border_edge::top,
                                       replacement);
            observed_failure_allocation_count = pugi_allocation_calls;
        }

        REQUIRE_FALSE(updated);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_FALSE(
            table.border(featherdoc::table_border_edge::top).has_value());
        REQUIRE(table.layout_mode().has_value());
        CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
        REQUIRE(first_cell.width_twips().has_value());
        CHECK_EQ(*first_cell.width_twips(), 1200U);
        REQUIRE(second_cell.width_twips().has_value());
        CHECK_EQ(*second_cell.width_twips(), 1200U);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(
            table.set_border(featherdoc::table_border_edge::top, replacement));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        const auto updated_border =
            table.border(featherdoc::table_border_edge::top);
        REQUIRE(updated_border.has_value());
        CHECK_EQ(updated_border->style, featherdoc::border_style::double_line);
        CHECK_EQ(updated_border->size_eighth_points, 16U);
        CHECK_EQ(updated_border->color, "12AB34");
        CHECK_EQ(updated_border->space_points, 2U);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "table border updates are atomic for every global allocation failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    const auto updated_color = std::string(4096U, 'A');
    const auto replacement = replacement_table_border(updated_color);
    auto successful_allocation_count = std::size_t{0U};
    {
        scoped_test_path path{
            make_test_path("table-border-global-baseline", 0U)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        auto table = document.tables();
        auto updated = false;
        {
            global_allocation_guard guard{0U};
            updated = table.set_border(featherdoc::table_border_edge::top,
                                       replacement);
            successful_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }
        REQUIRE(updated);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);
        scoped_test_path path{
            make_test_path("table-border-global-failure", failure_call)};
        write_test_docx(path.path(), fixture_xml);
        featherdoc::Document document(path.path());
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save());
        const auto xml_before =
            read_test_docx_entry(path.path(), test_document_xml_entry);

        auto table = document.tables();
        auto row = table.rows();
        auto first_cell = row.cells();
        auto second_cell = first_cell;
        second_cell.next();
        auto paragraph = first_cell.paragraphs();
        auto run = paragraph.runs();
        auto updated = true;
        auto observed_failure_allocation_count = std::size_t{0U};
        {
            global_allocation_guard guard{failure_call};
            updated = table.set_border(featherdoc::table_border_edge::top,
                                       replacement);
            observed_failure_allocation_count =
                observed_allocation_calls.load(std::memory_order_relaxed);
        }

        REQUIRE_FALSE(updated);
        REQUIRE_GE(observed_failure_allocation_count, failure_call);
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        CHECK_FALSE(
            table.border(featherdoc::table_border_edge::top).has_value());
        REQUIRE(table.layout_mode().has_value());
        CHECK_EQ(*table.layout_mode(), featherdoc::table_layout_mode::fixed);
        REQUIRE(first_cell.width_twips().has_value());
        CHECK_EQ(*first_cell.width_twips(), 1200U);
        REQUIRE(second_cell.width_twips().has_value());
        CHECK_EQ(*second_cell.width_twips(), 1200U);
        REQUIRE_FALSE(document.save());
        CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                 xml_before);

        REQUIRE(
            table.set_border(featherdoc::table_border_edge::top, replacement));
        CHECK(table.valid());
        CHECK(row.valid());
        CHECK(first_cell.valid());
        CHECK(second_cell.valid());
        CHECK(paragraph.valid());
        CHECK(run.valid());
        const auto updated_border =
            table.border(featherdoc::table_border_edge::top);
        REQUIRE(updated_border.has_value());
        CHECK_EQ(updated_border->style, featherdoc::border_style::double_line);
        CHECK_EQ(updated_border->size_eighth_points, 16U);
        CHECK_EQ(updated_border->color, updated_color);
        CHECK_EQ(updated_border->space_points, 2U);
    }
}

TEST_CASE("column width updates preserve handles and unrelated XML content") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    scoped_test_path path{make_test_path("列宽句柄与扩展保留", 0U)};
    write_test_docx(path.path(), fixture_xml);
    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());

    auto table = document.tables();
    auto row = table.rows();
    auto first_cell = row.cells();
    auto second_cell = first_cell;
    second_cell.next();
    auto paragraph = first_cell.paragraphs();
    auto run = paragraph.runs();

    REQUIRE(table.set_column_width_twips(0U, 1800U));
    CHECK(table.valid());
    CHECK(row.valid());
    CHECK(first_cell.valid());
    CHECK(second_cell.valid());
    CHECK(paragraph.valid());
    CHECK(run.valid());
    REQUIRE(table.column_width_twips(0U).has_value());
    CHECK_EQ(*table.column_width_twips(0U), 1800U);
    REQUIRE(first_cell.width_twips().has_value());
    CHECK_EQ(*first_cell.width_twips(), 1800U);
    REQUIRE(second_cell.width_twips().has_value());
    CHECK_EQ(*second_cell.width_twips(), 1200U);

    REQUIRE(table.clear_column_width(0U));
    CHECK(table.valid());
    CHECK(row.valid());
    CHECK(first_cell.valid());
    CHECK(second_cell.valid());
    CHECK(paragraph.valid());
    CHECK(run.valid());
    CHECK_FALSE(table.column_width_twips(0U).has_value());
    CHECK_FALSE(first_cell.width_twips().has_value());
    REQUIRE(second_cell.width_twips().has_value());
    CHECK_EQ(*second_cell.width_twips(), 1200U);

    REQUIRE_FALSE(document.save());
    const auto saved_xml =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_xml.c_str()));
    const auto saved_table =
        saved_document.child("w:document").child("w:body").child("w:tbl");
    const auto saved_properties = saved_table.child("w:tblPr");
    const auto saved_grid = saved_table.child("w:tblGrid");
    REQUIRE(saved_properties != pugi::xml_node{});
    REQUIRE(saved_grid != pugi::xml_node{});
    CHECK_FALSE(
        std::string_view{saved_properties.attribute("data-large").value()}
            .empty());
    CHECK_FALSE(
        std::string_view{saved_grid.attribute("data-large").value()}.empty());
    const auto first_grid_column = saved_grid.child("w:gridCol");
    const auto second_grid_column = first_grid_column.next_sibling("w:gridCol");
    REQUIRE(first_grid_column != pugi::xml_node{});
    REQUIRE(second_grid_column != pugi::xml_node{});
    CHECK_EQ(first_grid_column.attribute("w:w"), pugi::xml_attribute{});
    CHECK_EQ(std::string_view{second_grid_column.attribute("w:w").value()},
             "1200");
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "column width set and clear are atomic for every pugixml allocation "
    "failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    for (const auto clear_width : {false, true}) {
        CAPTURE(clear_width);
        const auto apply_update = [clear_width](featherdoc::Table &table) {
            return clear_width ? table.clear_column_width(0U)
                               : table.set_column_width_twips(0U, 1800U);
        };

        auto successful_allocation_count = std::size_t{0U};
        {
            scoped_test_path path{make_test_path(
                clear_width ? "清除列宽XML基线" : "设置列宽XML基线", 0U)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            auto table = document.tables();
            auto updated = false;
            {
                pugi_allocator_guard guard;
                updated = apply_update(table);
                successful_allocation_count = pugi_allocation_calls;
            }
            REQUIRE(updated);
        }
        REQUIRE_GT(successful_allocation_count, 0U);

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            scoped_test_path path{make_test_path(
                clear_width ? "清除列宽XML失败" : "设置列宽XML失败",
                failure_call)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            REQUIRE_FALSE(document.save());
            const auto xml_before =
                read_test_docx_entry(path.path(), test_document_xml_entry);

            auto table = document.tables();
            auto row = table.rows();
            auto first_cell = row.cells();
            auto second_cell = first_cell;
            second_cell.next();
            auto paragraph = first_cell.paragraphs();
            auto run = paragraph.runs();
            auto updated = true;
            auto observed_failure_allocation_count = std::size_t{0U};
            {
                pugi_allocator_guard guard;
                pugi_failure_call = failure_call;
                updated = apply_update(table);
                observed_failure_allocation_count = pugi_allocation_calls;
            }

            REQUIRE_FALSE(updated);
            REQUIRE_GE(observed_failure_allocation_count, failure_call);
            CHECK(table.valid());
            CHECK(row.valid());
            CHECK(first_cell.valid());
            CHECK(second_cell.valid());
            CHECK(paragraph.valid());
            CHECK(run.valid());
            REQUIRE(table.column_width_twips(0U).has_value());
            CHECK_EQ(*table.column_width_twips(0U), 1200U);
            REQUIRE(first_cell.width_twips().has_value());
            CHECK_EQ(*first_cell.width_twips(), 1200U);
            REQUIRE(second_cell.width_twips().has_value());
            CHECK_EQ(*second_cell.width_twips(), 1200U);
            REQUIRE_FALSE(document.save());
            CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                     xml_before);

            REQUIRE(apply_update(table));
            CHECK(table.valid());
            CHECK(row.valid());
            CHECK(first_cell.valid());
            CHECK(second_cell.valid());
            CHECK(paragraph.valid());
            CHECK(run.valid());
            if (clear_width) {
                CHECK_FALSE(table.column_width_twips(0U).has_value());
                CHECK_FALSE(first_cell.width_twips().has_value());
            } else {
                REQUIRE(table.column_width_twips(0U).has_value());
                CHECK_EQ(*table.column_width_twips(0U), 1800U);
                REQUIRE(first_cell.width_twips().has_value());
                CHECK_EQ(*first_cell.width_twips(), 1800U);
            }
            REQUIRE(second_cell.width_twips().has_value());
            CHECK_EQ(*second_cell.width_twips(), 1200U);
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "column width set and clear are atomic for every global allocation "
    "failure") {
    const auto fixture_xml = allocation_heavy_table_fixture_xml(2U, 2U);
    for (const auto clear_width : {false, true}) {
        CAPTURE(clear_width);
        const auto apply_update = [clear_width](featherdoc::Table &table) {
            return clear_width ? table.clear_column_width(0U)
                               : table.set_column_width_twips(0U, 1800U);
        };

        auto successful_allocation_count = std::size_t{0U};
        {
            scoped_test_path path{make_test_path(
                clear_width ? "清除列宽全局基线" : "设置列宽全局基线", 0U)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            auto table = document.tables();
            auto updated = false;
            {
                global_allocation_guard guard{0U};
                updated = apply_update(table);
                successful_allocation_count =
                    observed_allocation_calls.load(std::memory_order_relaxed);
            }
            REQUIRE(updated);
        }
        REQUIRE_GT(successful_allocation_count, 0U);

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            scoped_test_path path{make_test_path(
                clear_width ? "清除列宽全局失败" : "设置列宽全局失败",
                failure_call)};
            write_test_docx(path.path(), fixture_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            REQUIRE_FALSE(document.save());
            const auto xml_before =
                read_test_docx_entry(path.path(), test_document_xml_entry);

            auto table = document.tables();
            auto row = table.rows();
            auto first_cell = row.cells();
            auto second_cell = first_cell;
            second_cell.next();
            auto paragraph = first_cell.paragraphs();
            auto run = paragraph.runs();
            auto updated = true;
            auto observed_failure_allocation_count = std::size_t{0U};
            {
                global_allocation_guard guard{failure_call};
                updated = apply_update(table);
                observed_failure_allocation_count =
                    observed_allocation_calls.load(std::memory_order_relaxed);
            }

            REQUIRE_FALSE(updated);
            REQUIRE_GE(observed_failure_allocation_count, failure_call);
            CHECK(table.valid());
            CHECK(row.valid());
            CHECK(first_cell.valid());
            CHECK(second_cell.valid());
            CHECK(paragraph.valid());
            CHECK(run.valid());
            REQUIRE(table.column_width_twips(0U).has_value());
            CHECK_EQ(*table.column_width_twips(0U), 1200U);
            REQUIRE(first_cell.width_twips().has_value());
            CHECK_EQ(*first_cell.width_twips(), 1200U);
            REQUIRE(second_cell.width_twips().has_value());
            CHECK_EQ(*second_cell.width_twips(), 1200U);
            REQUIRE_FALSE(document.save());
            CHECK_EQ(read_test_docx_entry(path.path(), test_document_xml_entry),
                     xml_before);

            REQUIRE(apply_update(table));
            if (clear_width) {
                CHECK_FALSE(table.column_width_twips(0U).has_value());
                CHECK_FALSE(first_cell.width_twips().has_value());
            } else {
                REQUIRE(table.column_width_twips(0U).has_value());
                CHECK_EQ(*table.column_width_twips(0U), 1800U);
                REQUIRE(first_cell.width_twips().has_value());
                CHECK_EQ(*first_cell.width_twips(), 1800U);
            }
            REQUIRE(second_cell.width_twips().has_value());
            CHECK_EQ(*second_cell.width_twips(), 1200U);
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
