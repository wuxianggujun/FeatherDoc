#pragma once

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"

#include <cstring>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <featherdoc.hpp>
#include <zip.h>

namespace {

auto collect_document_text(featherdoc::Document &doc) -> std::string {
    std::ostringstream stream;
    for (featherdoc::Paragraph paragraph = doc.paragraphs(); paragraph.has_next();
         paragraph.next()) {
        for (featherdoc::Run run = paragraph.runs(); run.has_next(); run.next()) {
            stream << run.get_text();
        }
        stream << '\n';
    }
    return stream.str();
}

auto utf8_from_u8(std::u8string_view text) -> std::string {
    return {reinterpret_cast<const char *>(text.data()), text.size()};
}

auto collect_table_text(featherdoc::Document &doc) -> std::string {
    std::ostringstream stream;
    for (featherdoc::Table table = doc.tables(); table.has_next(); table.next()) {
        for (featherdoc::TableRow row = table.rows(); row.has_next(); row.next()) {
            for (featherdoc::TableCell cell = row.cells(); cell.has_next(); cell.next()) {
                for (featherdoc::Paragraph paragraph = cell.paragraphs();
                     paragraph.has_next(); paragraph.next()) {
                    for (featherdoc::Run run = paragraph.runs(); run.has_next(); run.next()) {
                        stream << run.get_text();
                    }
                    stream << '\n';
                }
            }
        }
    }
    return stream.str();
}

auto collect_template_part_text(featherdoc::TemplatePart part) -> std::string {
    std::ostringstream stream;
    for (featherdoc::Paragraph paragraph = part.paragraphs(); paragraph.has_next();
         paragraph.next()) {
        for (featherdoc::Run run = paragraph.runs(); run.has_next(); run.next()) {
            stream << run.get_text();
        }
        stream << '\n';
    }
    return stream.str();
}

auto collect_template_part_table_text(featherdoc::TemplatePart part) -> std::string {
    std::ostringstream stream;
    for (featherdoc::Table table = part.tables(); table.has_next(); table.next()) {
        for (featherdoc::TableRow row = table.rows(); row.has_next(); row.next()) {
            for (featherdoc::TableCell cell = row.cells(); cell.has_next(); cell.next()) {
                for (featherdoc::Paragraph paragraph = cell.paragraphs();
                     paragraph.has_next(); paragraph.next()) {
                    for (featherdoc::Run run = paragraph.runs(); run.has_next(); run.next()) {
                        stream << run.get_text();
                    }
                    stream << '\n';
                }
            }
        }
    }
    return stream.str();
}

void create_semantic_diff_fixture(const std::filesystem::path &path,
                                  std::string_view title,
                                  std::string_view status,
                                  std::string_view customer,
                                  std::string_view amount) {
    std::filesystem::remove(path);

    const auto document_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>)"} + std::string{title} + R"(</w:t></w:r></w:p>
    <w:p><w:r><w:t>Status: )" + std::string{status} + R"(</w:t></w:r></w:p>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Customer Name"/>
        <w:tag w:val="customer_name"/>
        <w:id w:val="101"/>
        <w:dataBinding w:storeItemID="{77777777-7777-7777-7777-777777777777}" w:xpath="/invoice/customer"/>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>)" + std::string{customer} + R"(</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
    <w:tbl>
      <w:tblPr><w:tblStyle w:val="TableGrid"/></w:tblPr>
      <w:tblGrid><w:gridCol w:w="2400"/><w:gridCol w:w="2400"/></w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Label</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Amount</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Total</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>)" + std::string{amount} + R"(</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_basic_archive_entry(archive, test_relationships_xml_entry,
                                      test_relationships_xml));
    REQUIRE(write_basic_archive_entry(archive, test_content_types_xml_entry,
                                      test_content_types_xml));
    REQUIRE(write_basic_archive_entry(archive, test_document_xml_entry,
                                      document_xml));
    zip_close(archive);
}

auto find_style_summary(const std::vector<featherdoc::style_summary> &styles,
                        std::string_view style_id)
    -> const featherdoc::style_summary * {
    for (const auto &style : styles) {
        if (style.style_id == style_id) {
            return &style;
        }
    }

    return nullptr;
}

auto find_style_xml_node(pugi::xml_node styles_root, std::string_view style_id) -> pugi::xml_node {
    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        if (std::string_view{style.attribute("w:styleId").value()} == style_id) {
            return style;
        }
    }

    return {};
}

auto find_table_style_region_xml_node(pugi::xml_node style, std::string_view type_name)
    -> pugi::xml_node {
    for (auto region = style.child("w:tblStylePr"); region != pugi::xml_node{};
         region = region.next_sibling("w:tblStylePr")) {
        if (std::string_view{region.attribute("w:type").value()} == type_name) {
            return region;
        }
    }

    return {};
}

auto find_numbering_abstract_xml_node(pugi::xml_node numbering_root, std::string_view name)
    -> pugi::xml_node {
    for (auto abstract_num = numbering_root.child("w:abstractNum");
         abstract_num != pugi::xml_node{};
         abstract_num = abstract_num.next_sibling("w:abstractNum")) {
        if (std::string_view{abstract_num.child("w:name").attribute("w:val").value()} == name) {
            return abstract_num;
        }
    }

    return {};
}

auto find_numbering_level_xml_node(pugi::xml_node abstract_num, std::uint32_t level)
    -> pugi::xml_node {
    const auto level_text = std::to_string(level);
    for (auto level_node = abstract_num.child("w:lvl"); level_node != pugi::xml_node{};
         level_node = level_node.next_sibling("w:lvl")) {
        if (std::string_view{level_node.attribute("w:ilvl").value()} == level_text) {
            return level_node;
        }
    }

    return {};
}

auto collect_table_grid_width_values(pugi::xml_node table_node) -> std::vector<std::string> {
    auto widths = std::vector<std::string>{};
    const auto table_grid = table_node.child("w:tblGrid");
    for (auto grid_column = table_grid.child("w:gridCol"); grid_column != pugi::xml_node{};
         grid_column = grid_column.next_sibling("w:gridCol")) {
        const auto width_attribute = grid_column.attribute("w:w");
        widths.emplace_back(width_attribute == pugi::xml_attribute{} ? "" : width_attribute.value());
    }
    return widths;
}

auto collect_row_cell_width_values(pugi::xml_node row_node) -> std::vector<std::string> {
    auto widths = std::vector<std::string>{};
    for (auto cell = row_node.child("w:tc"); cell != pugi::xml_node{};
         cell = cell.next_sibling("w:tc")) {
        const auto width_attribute = cell.child("w:tcPr").child("w:tcW").attribute("w:w");
        widths.emplace_back(width_attribute == pugi::xml_attribute{} ? "" : width_attribute.value());
    }
    return widths;
}

auto set_two_cell_row_text(featherdoc::TableRow row, std::string_view left,
                           std::string_view right) -> bool {
    auto cell = row.cells();
    if (!cell.has_next()) {
        return false;
    }
    if (!cell.set_text(std::string{left})) {
        return false;
    }

    cell.next();
    if (!cell.has_next()) {
        return false;
    }

    return cell.set_text(std::string{right});
}

auto configure_clone_template_table(featherdoc::Table table, std::string_view prefix) -> bool {
    if (!table.has_next()) {
        return false;
    }

    if (!table.set_width_twips(7200U) || !table.set_style_id("TableGrid") ||
        !table.set_layout_mode(featherdoc::table_layout_mode::fixed) ||
        !table.set_alignment(featherdoc::table_alignment::center) ||
        !table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 120U) ||
        !table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 120U)) {
        return false;
    }

    auto header_row = table.rows();
    if (!header_row.has_next() ||
        !header_row.set_height_twips(360U, featherdoc::row_height_rule::exact) ||
        !header_row.set_repeats_header()) {
        return false;
    }

    auto header_cell = header_row.cells();
    if (!header_cell.has_next() ||
        !header_cell.set_fill_color("D9EAF7") ||
        !header_cell.set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center) ||
        !header_cell.set_text(std::string{prefix} + "-h1")) {
        return false;
    }

    header_cell.next();
    if (!header_cell.has_next() ||
        !header_cell.set_fill_color("D9EAF7") ||
        !header_cell.set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center) ||
        !header_cell.set_text(std::string{prefix} + "-h2")) {
        return false;
    }

    header_row.next();
    if (!header_row.has_next() ||
        !header_row.set_height_twips(420U, featherdoc::row_height_rule::at_least)) {
        return false;
    }

    auto body_cell = header_row.cells();
    if (!body_cell.has_next() ||
        !body_cell.set_fill_color("FCE4D6") ||
        !body_cell.set_margin_twips(featherdoc::cell_margin_edge::left, 160U) ||
        !body_cell.set_text(std::string{prefix} + "-b1")) {
        return false;
    }

    body_cell.next();
    if (!body_cell.has_next() ||
        !body_cell.set_fill_color("FFF2CC") ||
        !body_cell.set_margin_twips(featherdoc::cell_margin_edge::right, 160U) ||
        !body_cell.set_text(std::string{prefix} + "-b2")) {
        return false;
    }

    return true;
}

auto count_named_children(pugi::xml_node parent, const char *child_name) -> std::size_t {
    std::size_t count = 0;
    for (auto child = parent.child(child_name); child != pugi::xml_node{};
         child = child.next_sibling(child_name)) {
        ++count;
    }
    return count;
}

auto count_named_descendants(pugi::xml_node parent, const char *child_name) -> std::size_t {
    if (parent == pugi::xml_node{}) {
        return 0U;
    }

    std::size_t count = 0U;
    for (auto child = parent.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::strcmp(child.name(), child_name) == 0) {
            ++count;
        }
        count += count_named_descendants(child, child_name);
    }
    return count;
}

auto count_substring_occurrences(std::string_view text, std::string_view needle)
    -> std::size_t {
    if (needle.empty()) {
        return 0U;
    }

    std::size_t count = 0U;
    std::size_t position = 0U;
    while ((position = text.find(needle, position)) != std::string_view::npos) {
        ++count;
        position += needle.size();
    }
    return count;
}


} // namespace
