#include "featherdoc_cli_page_setup.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_text.hpp"

#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_json_page_margins(std::ostream &stream,
                             const featherdoc::page_margins &margins) {
    stream << "{\"top_twips\":" << margins.top_twips
           << ",\"bottom_twips\":" << margins.bottom_twips
           << ",\"left_twips\":" << margins.left_twips
           << ",\"right_twips\":" << margins.right_twips
           << ",\"header_twips\":" << margins.header_twips
           << ",\"footer_twips\":" << margins.footer_twips << '}';
}

void print_section_page_setup_summary(
    std::ostream &stream, const featherdoc::section_page_setup &page_setup) {
    stream << "orientation=" << page_orientation_name(page_setup.orientation)
           << " width_twips=" << page_setup.width_twips
           << " height_twips=" << page_setup.height_twips
           << " margins(top=" << page_setup.margins.top_twips
           << ", bottom=" << page_setup.margins.bottom_twips
           << ", left=" << page_setup.margins.left_twips
           << ", right=" << page_setup.margins.right_twips
           << ", header=" << page_setup.margins.header_twips
           << ", footer=" << page_setup.margins.footer_twips
           << ") page_number_start=";
    if (page_setup.page_number_start.has_value()) {
        stream << *page_setup.page_number_start;
    } else {
        stream << "none";
    }
}

void print_section_page_setup_details(
    std::ostream &stream, const featherdoc::section_page_setup &page_setup) {
    stream << "orientation: " << page_orientation_name(page_setup.orientation)
           << '\n';
    stream << "width_twips: " << page_setup.width_twips << '\n';
    stream << "height_twips: " << page_setup.height_twips << '\n';
    stream << "top_twips: " << page_setup.margins.top_twips << '\n';
    stream << "bottom_twips: " << page_setup.margins.bottom_twips << '\n';
    stream << "left_twips: " << page_setup.margins.left_twips << '\n';
    stream << "right_twips: " << page_setup.margins.right_twips << '\n';
    stream << "header_twips: " << page_setup.margins.header_twips << '\n';
    stream << "footer_twips: " << page_setup.margins.footer_twips << '\n';
    stream << "page_number_start: ";
    if (page_setup.page_number_start.has_value()) {
        stream << *page_setup.page_number_start;
    } else {
        stream << "none";
    }
    stream << '\n';
}

} // namespace

void write_json_section_page_setup(
    std::ostream &stream, const featherdoc::section_page_setup &page_setup) {
    stream << "{\"orientation\":";
    write_json_string(stream, page_orientation_name(page_setup.orientation));
    stream << ",\"width_twips\":" << page_setup.width_twips
           << ",\"height_twips\":" << page_setup.height_twips
           << ",\"margins\":";
    write_json_page_margins(stream, page_setup.margins);
    stream << ",\"page_number_start\":";
    if (page_setup.page_number_start.has_value()) {
        stream << *page_setup.page_number_start;
    } else {
        stream << "null";
    }
    stream << '}';
}

auto inspect_page_setup(featherdoc::Document &doc, std::size_t section_index,
                        std::string_view command, bool json_output) -> bool {
    const auto page_setup = doc.get_section_page_setup(section_index);
    if (!page_setup.has_value() && doc.last_error().code) {
        return report_document_error(command, "inspect", doc.last_error(),
                                     json_output);
    }

    if (json_output) {
        std::cout << "{\"section\":" << section_index
                  << ",\"present\":" << json_bool(page_setup.has_value())
                  << ",\"page_setup\":";
        if (page_setup.has_value()) {
            write_json_section_page_setup(std::cout, *page_setup);
        } else {
            std::cout << "null";
        }
        std::cout << "}\n";
        return true;
    }

    std::cout << "section: " << section_index << '\n';
    std::cout << "present: " << yes_no(page_setup.has_value()) << '\n';
    if (page_setup.has_value()) {
        print_section_page_setup_details(std::cout, *page_setup);
    }
    return true;
}

auto inspect_page_setups(featherdoc::Document &doc, std::string_view command,
                         bool json_output) -> bool {
    std::vector<std::optional<featherdoc::section_page_setup>> page_setups;
    page_setups.reserve(doc.section_count());
    for (std::size_t section_index = 0; section_index < doc.section_count();
         ++section_index) {
        auto page_setup = doc.get_section_page_setup(section_index);
        if (!page_setup.has_value() && doc.last_error().code) {
            return report_document_error(command, "inspect", doc.last_error(),
                                         json_output);
        }

        page_setups.push_back(std::move(page_setup));
    }

    if (json_output) {
        std::cout << "{\"count\":" << page_setups.size() << ",\"sections\":[";
        for (std::size_t section_index = 0; section_index < page_setups.size();
             ++section_index) {
            if (section_index != 0U) {
                std::cout << ',';
            }

            std::cout << "{\"section\":" << section_index
                      << ",\"present\":"
                      << json_bool(page_setups[section_index].has_value())
                      << ",\"page_setup\":";
            if (page_setups[section_index].has_value()) {
                write_json_section_page_setup(std::cout,
                                              *page_setups[section_index]);
            } else {
                std::cout << "null";
            }
            std::cout << '}';
        }
        std::cout << "]}\n";
        return true;
    }

    std::cout << "sections: " << page_setups.size() << '\n';
    for (std::size_t section_index = 0; section_index < page_setups.size();
         ++section_index) {
        std::cout << "section[" << section_index
                  << "]: present="
                  << yes_no(page_setups[section_index].has_value());
        if (page_setups[section_index].has_value()) {
            std::cout << ' ';
            print_section_page_setup_summary(std::cout,
                                             *page_setups[section_index]);
        }
        std::cout << '\n';
    }

    return true;
}

auto resolve_section_page_setup(featherdoc::Document &doc, std::size_t section_index,
                                const set_section_page_setup_options &options,
                                std::string_view command, bool json_output,
                                featherdoc::section_page_setup &setup) -> bool {
    const auto existing = doc.get_section_page_setup(section_index);
    if (!existing.has_value() && doc.last_error().code) {
        return report_document_error(command, "inspect", doc.last_error(),
                                     json_output);
    }

    std::vector<std::string> missing_options;
    auto use_existing_or_require =
        [&]<typename T>(const std::optional<T> &specified, const T *existing_value,
                        T &target, const char *option_name) {
            if (specified.has_value()) {
                target = *specified;
                return;
            }
            if (existing_value != nullptr) {
                target = *existing_value;
                return;
            }
            missing_options.emplace_back(option_name);
        };

    const auto *existing_setup = existing.has_value() ? &*existing : nullptr;
    use_existing_or_require(options.orientation,
                            existing_setup != nullptr
                                ? &existing_setup->orientation
                                : nullptr,
                            setup.orientation, "--orientation");
    use_existing_or_require(options.width_twips,
                            existing_setup != nullptr ? &existing_setup->width_twips
                                                      : nullptr,
                            setup.width_twips, "--width");
    use_existing_or_require(options.height_twips,
                            existing_setup != nullptr ? &existing_setup->height_twips
                                                      : nullptr,
                            setup.height_twips, "--height");
    use_existing_or_require(options.margin_top_twips,
                            existing_setup != nullptr
                                ? &existing_setup->margins.top_twips
                                : nullptr,
                            setup.margins.top_twips, "--margin-top");
    use_existing_or_require(options.margin_bottom_twips,
                            existing_setup != nullptr
                                ? &existing_setup->margins.bottom_twips
                                : nullptr,
                            setup.margins.bottom_twips, "--margin-bottom");
    use_existing_or_require(options.margin_left_twips,
                            existing_setup != nullptr
                                ? &existing_setup->margins.left_twips
                                : nullptr,
                            setup.margins.left_twips, "--margin-left");
    use_existing_or_require(options.margin_right_twips,
                            existing_setup != nullptr
                                ? &existing_setup->margins.right_twips
                                : nullptr,
                            setup.margins.right_twips, "--margin-right");
    use_existing_or_require(options.margin_header_twips,
                            existing_setup != nullptr
                                ? &existing_setup->margins.header_twips
                                : nullptr,
                            setup.margins.header_twips, "--margin-header");
    use_existing_or_require(options.margin_footer_twips,
                            existing_setup != nullptr
                                ? &existing_setup->margins.footer_twips
                                : nullptr,
                            setup.margins.footer_twips, "--margin-footer");

    if (!missing_options.empty()) {
        std::string detail =
            "section does not expose explicit page setup; supply ";
        for (std::size_t index = 0; index < missing_options.size(); ++index) {
            if (index != 0U) {
                detail += index + 1U == missing_options.size() ? ", and " : ", ";
            }
            detail += missing_options[index];
        }

        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail = std::move(detail);
        return report_operation_failure(command, "input",
                                        "invalid page setup input", error_info,
                                        json_output);
    }

    if (options.clear_page_number_start) {
        setup.page_number_start = std::nullopt;
    } else if (options.page_number_start.has_value()) {
        setup.page_number_start = options.page_number_start;
    } else if (existing_setup != nullptr) {
        setup.page_number_start = existing_setup->page_number_start;
    } else {
        setup.page_number_start = std::nullopt;
    }

    return true;
}

} // namespace featherdoc_cli
