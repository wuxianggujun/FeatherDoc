auto last_block_paragraph(pugi::xml_node container) -> pugi::xml_node {
    pugi::xml_node paragraph;
    for (auto child = container.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:p") {
            paragraph = child;
        }
    }

    return paragraph;
}

auto paragraph_is_effectively_empty(pugi::xml_node paragraph) -> bool {
    if (paragraph == pugi::xml_node{} ||
        std::string_view{paragraph.name()} != "w:p") {
        return false;
    }

    for (auto child = paragraph.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:pPr") {
            return false;
        }
    }

    return true;
}

auto select_field_paragraph(pugi::xml_node container) -> pugi::xml_node {
    if (container == pugi::xml_node{}) {
        return {};
    }

    if (auto paragraph = last_block_paragraph(container);
        paragraph_is_effectively_empty(paragraph)) {
        return paragraph;
    }

    return featherdoc::detail::append_paragraph_node(container);
}

auto append_field_result_run(pugi::xml_node field, std::string_view result_text)
    -> bool {
    if (field == pugi::xml_node{}) {
        return false;
    }

    auto run = field.append_child("w:r");
    if (run == pugi::xml_node{}) {
        return false;
    }

    auto run_properties = run.append_child("w:rPr");
    if (run_properties == pugi::xml_node{}) {
        return false;
    }
    if (run_properties.append_child("w:noProof") == pugi::xml_node{}) {
        return false;
    }

    if (result_text.empty()) {
        return true;
    }

    auto text = run.append_child("w:t");
    if (text == pugi::xml_node{}) {
        return false;
    }

    return text.text().set(std::string{result_text}.c_str());
}

bool set_simple_field_on_off_attribute(pugi::xml_node field,
                                       const char *attribute_name,
                                       bool enabled) {
    if (!enabled) {
        return true;
    }

    auto attribute = field.attribute(attribute_name);
    if (attribute == pugi::xml_attribute{}) {
        attribute = field.append_attribute(attribute_name);
    }
    return attribute != pugi::xml_attribute{} && attribute.set_value("true");
}

bool apply_simple_field_state(pugi::xml_node field,
                              featherdoc::field_state_options state) {
    return set_simple_field_on_off_attribute(field, "w:dirty", state.dirty) &&
           set_simple_field_on_off_attribute(field, "w:fldLock", state.locked);
}

auto append_complex_field_char_run(pugi::xml_node paragraph,
                                   const char *field_char_type,
                                   featherdoc::field_state_options state = {},
                                   bool apply_state = false) -> bool {
    if (paragraph == pugi::xml_node{}) {
        return false;
    }

    auto run = paragraph.append_child("w:r");
    if (run == pugi::xml_node{}) {
        return false;
    }

    auto field_char = run.append_child("w:fldChar");
    if (field_char == pugi::xml_node{}) {
        return false;
    }

    auto type_attribute = field_char.append_attribute("w:fldCharType");
    if (type_attribute == pugi::xml_attribute{} ||
        !type_attribute.set_value(field_char_type)) {
        return false;
    }

    return !apply_state || apply_simple_field_state(field_char, state);
}

auto append_complex_field_instruction_run(pugi::xml_node paragraph,
                                          std::string_view instruction) -> bool {
    if (paragraph == pugi::xml_node{} || instruction.empty()) {
        return paragraph != pugi::xml_node{};
    }

    auto run = paragraph.append_child("w:r");
    if (run == pugi::xml_node{}) {
        return false;
    }

    auto instruction_text = run.append_child("w:instrText");
    if (instruction_text == pugi::xml_node{}) {
        return false;
    }

    auto space_attribute = instruction_text.append_attribute("xml:space");
    if (space_attribute == pugi::xml_attribute{} ||
        !space_attribute.set_value("preserve")) {
        return false;
    }

    return instruction_text.text().set(std::string{instruction}.c_str());
}

auto append_complex_field_result_run(pugi::xml_node paragraph,
                                     std::string_view result_text) -> bool {
    if (paragraph == pugi::xml_node{}) {
        return false;
    }
    if (result_text.empty()) {
        return true;
    }

    auto run = paragraph.append_child("w:r");
    if (run == pugi::xml_node{}) {
        return false;
    }

    auto run_properties = run.append_child("w:rPr");
    if (run_properties == pugi::xml_node{} ||
        run_properties.append_child("w:noProof") == pugi::xml_node{}) {
        return false;
    }

    auto text = run.append_child("w:t");
    if (text == pugi::xml_node{}) {
        return false;
    }

    return text.text().set(std::string{result_text}.c_str());
}

auto append_simple_field_run(pugi::xml_node paragraph,
                             std::string_view instruction,
                             std::string_view result_text,
                             featherdoc::field_state_options state) -> bool {
    if (paragraph == pugi::xml_node{}) {
        return false;
    }

    auto field = paragraph.append_child("w:fldSimple");
    if (field == pugi::xml_node{}) {
        return false;
    }

    auto instruction_attribute = field.append_attribute("w:instr");
    if (instruction_attribute == pugi::xml_attribute{}) {
        return false;
    }
    if (!instruction_attribute.set_value(std::string{instruction}.c_str())) {
        return false;
    }
    if (!apply_simple_field_state(field, state)) {
        return false;
    }

    return append_field_result_run(field, result_text);
}

auto trimmed_field_instruction(std::string_view instruction) -> std::string_view {
    auto begin = instruction.begin();
    auto end = instruction.end();
    while (begin != end &&
           std::isspace(static_cast<unsigned char>(*begin)) != 0) {
        ++begin;
    }
    while (begin != end &&
           std::isspace(static_cast<unsigned char>(*(end - 1))) != 0) {
        --end;
    }
    return {begin, end};
}

auto fragment_contains_instruction(
    const featherdoc::complex_field_instruction_fragment &fragment) -> bool {
    if (fragment.kind ==
        featherdoc::complex_field_instruction_fragment_kind::nested_field) {
        return !trimmed_field_instruction(fragment.instruction).empty();
    }
    return !trimmed_field_instruction(fragment.text).empty();
}

auto append_complex_field_sequence(
    pugi::xml_node paragraph,
    std::span<const featherdoc::complex_field_instruction_fragment> fragments,
    std::string_view result_text, featherdoc::field_state_options state) -> bool;

auto append_complex_field_instruction_fragment(
    pugi::xml_node paragraph,
    const featherdoc::complex_field_instruction_fragment &fragment) -> bool {
    if (fragment.kind ==
        featherdoc::complex_field_instruction_fragment_kind::nested_field) {
        featherdoc::complex_field_instruction_fragment nested_text;
        nested_text.kind = featherdoc::complex_field_instruction_fragment_kind::text;
        nested_text.text = fragment.instruction;
        const std::array<featherdoc::complex_field_instruction_fragment, 1U>
            nested_fragments{nested_text};
        return append_complex_field_sequence(
            paragraph, std::span<const featherdoc::complex_field_instruction_fragment>{
                           nested_fragments.data(), nested_fragments.size()},
            fragment.result_text, fragment.state);
    }

    return append_complex_field_instruction_run(paragraph, fragment.text);
}

auto append_complex_field_sequence(
    pugi::xml_node paragraph,
    std::span<const featherdoc::complex_field_instruction_fragment> fragments,
    std::string_view result_text, featherdoc::field_state_options state) -> bool {
    if (!append_complex_field_char_run(paragraph, "begin", state, true)) {
        return false;
    }

    for (const auto &fragment : fragments) {
        if (!append_complex_field_instruction_fragment(paragraph, fragment)) {
            return false;
        }
    }

    return append_complex_field_char_run(paragraph, "separate") &&
           append_complex_field_result_run(paragraph, result_text) &&
           append_complex_field_char_run(paragraph, "end");
}

auto first_field_instruction_token(std::string_view instruction) -> std::string {
    const auto trimmed = trimmed_field_instruction(instruction);
    std::string token;
    for (const auto character : trimmed) {
        if (std::isspace(static_cast<unsigned char>(character)) != 0) {
            break;
        }
        token.push_back(static_cast<char>(
            std::toupper(static_cast<unsigned char>(character))));
    }
    return token;
}

auto field_kind_from_instruction(std::string_view instruction)
    -> featherdoc::field_kind {
    const auto token = first_field_instruction_token(instruction);
    if (token == "PAGE") {
        return featherdoc::field_kind::page;
    }
    if (token == "NUMPAGES") {
        return featherdoc::field_kind::total_pages;
    }
    if (token == "TOC") {
        return featherdoc::field_kind::table_of_contents;
    }
    if (token == "REF") {
        return featherdoc::field_kind::reference;
    }
    if (token == "PAGEREF") {
        return featherdoc::field_kind::page_reference;
    }
    if (token == "STYLEREF") {
        return featherdoc::field_kind::style_reference;
    }
    if (token == "DOCPROPERTY") {
        return featherdoc::field_kind::document_property;
    }
    if (token == "DATE") {
        return featherdoc::field_kind::date;
    }
    if (token == "HYPERLINK") {
        return featherdoc::field_kind::hyperlink;
    }
    if (token == "SEQ") {
        return featherdoc::field_kind::sequence;
    }
    if (token == "INDEX") {
        return featherdoc::field_kind::index;
    }
    if (token == "XE") {
        return featherdoc::field_kind::index_entry;
    }
    return featherdoc::field_kind::custom;
}

auto field_attribute_is_on(pugi::xml_node field, const char *attribute_name)
    -> bool {
    const auto value = std::string_view{field.attribute(attribute_name).value()};
    return value == "1" || value == "true" || value == "on";
}

void collect_replaceable_field_nodes_in_order(pugi::xml_node node,
                                              std::vector<pugi::xml_node> &fields) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto name = std::string_view{child.name()};
        if (name == "w:fldSimple") {
            fields.push_back(child);
            continue;
        }
        if (name == "w:r") {
            const auto field_char = child.child("w:fldChar");
            if (field_char != pugi::xml_node{} &&
                std::string_view{field_char.attribute("w:fldCharType").value()} ==
                    "begin") {
                fields.push_back({});
            }
            continue;
        }
        collect_replaceable_field_nodes_in_order(child, fields);
    }
}

auto summarize_field(pugi::xml_node field, std::size_t index,
                     std::size_t depth = 0U) -> featherdoc::field_summary {
    featherdoc::field_summary summary;
    summary.index = index;
    summary.depth = depth;
    summary.instruction = field.attribute("w:instr").value();
    summary.kind = field_kind_from_instruction(summary.instruction);
    summary.result_text = featherdoc::detail::collect_plain_text_from_xml(field);
    summary.dirty = field_attribute_is_on(field, "w:dirty");
    summary.locked = field_attribute_is_on(field, "w:fldLock");
    return summary;
}

struct active_complex_field_summary {
    std::size_t summary_index{};
    bool in_result{false};
};

void append_complex_field_run_text(
    pugi::xml_node run, std::vector<featherdoc::field_summary> &summaries,
    std::vector<active_complex_field_summary> &stack) {
    if (stack.empty()) {
        return;
    }

    auto &active = stack.back();
    auto &summary = summaries[active.summary_index];
    for (auto child = run.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto child_name = std::string_view{child.name()};
        if (child_name == "w:instrText" && !active.in_result) {
            summary.instruction += child.text().get();
        } else if (child_name == "w:t" && active.in_result) {
            summary.result_text += child.text().get();
        }
    }
}

void collect_field_summaries_in_order(
    pugi::xml_node node, std::vector<featherdoc::field_summary> &summaries,
    std::vector<active_complex_field_summary> &stack) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto name = std::string_view{child.name()};
        if (name == "w:fldSimple") {
            summaries.push_back(summarize_field(child, summaries.size(), stack.size()));
            continue;
        }

        if (name == "w:r") {
            const auto field_char = child.child("w:fldChar");
            if (field_char != pugi::xml_node{}) {
                const auto field_char_type =
                    std::string_view{field_char.attribute("w:fldCharType").value()};
                if (field_char_type == "begin") {
                    featherdoc::field_summary summary;
                    summary.index = summaries.size();
                    summary.complex = true;
                    summary.depth = stack.size();
                    summary.dirty = field_attribute_is_on(field_char, "w:dirty");
                    summary.locked = field_attribute_is_on(field_char, "w:fldLock");
                    summaries.push_back(std::move(summary));
                    stack.push_back({summaries.size() - 1U, false});
                } else if (field_char_type == "separate") {
                    if (!stack.empty()) {
                        stack.back().in_result = true;
                    }
                } else if (field_char_type == "end") {
                    if (!stack.empty()) {
                        auto &summary = summaries[stack.back().summary_index];
                        summary.kind = field_kind_from_instruction(summary.instruction);
                        stack.pop_back();
                    }
                }
                continue;
            }

            append_complex_field_run_text(child, summaries, stack);
            continue;
        }

        collect_field_summaries_in_order(child, summaries, stack);
    }
}

auto summarize_fields_in_part(featherdoc::document_error_info &last_error_info,
                              pugi::xml_document &document)
    -> std::vector<featherdoc::field_summary> {
    std::vector<featherdoc::field_summary> summaries;
    std::vector<active_complex_field_summary> stack;
    collect_field_summaries_in_order(document, summaries, stack);
    for (std::size_t index = 0U; index < summaries.size(); ++index) {
        summaries[index].index = index;
        summaries[index].kind = field_kind_from_instruction(summaries[index].instruction);
    }

    last_error_info.clear();
    return summaries;
}

auto append_field_to_part(featherdoc::document_error_info &last_error_info,
                          pugi::xml_document &document,
                          std::string_view entry_name,
                          std::string_view instruction,
                          std::string_view result_text,
                          featherdoc::field_state_options state) -> bool {
    if (trimmed_field_instruction(instruction).empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "field instruction must not be empty",
                       std::string{entry_name});
        return false;
    }

    const auto container = template_part_block_container(document);
    if (container == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       std::string{entry_name});
        return false;
    }

    const auto paragraph = select_field_paragraph(container);
    if (paragraph == pugi::xml_node{} ||
        !append_simple_field_run(paragraph, instruction, result_text, state)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append field to template part",
                       std::string{entry_name});
        return false;
    }

    last_error_info.clear();
    return true;
}

auto append_complex_field_to_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name,
    std::span<const featherdoc::complex_field_instruction_fragment> fragments,
    std::string_view result_text, featherdoc::complex_field_options options) -> bool {
    if (fragments.empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "complex field instruction must not be empty",
                       std::string{entry_name});
        return false;
    }

    const auto has_instruction = std::any_of(
        fragments.begin(), fragments.end(), fragment_contains_instruction);
    if (!has_instruction) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "complex field instruction must not be empty",
                       std::string{entry_name});
        return false;
    }
    for (const auto &fragment : fragments) {
        if (fragment.kind ==
                featherdoc::complex_field_instruction_fragment_kind::nested_field &&
            trimmed_field_instruction(fragment.instruction).empty()) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "nested field instruction must not be empty",
                           std::string{entry_name});
            return false;
        }
    }

    const auto container = template_part_block_container(document);
    if (container == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       std::string{entry_name});
        return false;
    }

    const auto paragraph = select_field_paragraph(container);
    if (paragraph == pugi::xml_node{} ||
        !append_complex_field_sequence(paragraph, fragments, result_text,
                                       options.state)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append complex field to template part",
                       std::string{entry_name});
        return false;
    }

    last_error_info.clear();
    return true;
}

auto replace_field_in_part(featherdoc::document_error_info &last_error_info,
                           pugi::xml_document &document,
                           std::string_view entry_name,
                           std::size_t field_index,
                           std::string_view instruction,
                           std::string_view result_text) -> bool {
    if (trimmed_field_instruction(instruction).empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "field instruction must not be empty",
                       std::string{entry_name});
        return false;
    }

    std::vector<pugi::xml_node> field_nodes;
    collect_replaceable_field_nodes_in_order(document, field_nodes);
    if (field_index >= field_nodes.size()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "field index is out of range",
                       std::string{entry_name});
        return false;
    }

    auto field = field_nodes[field_index];
    if (field == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "field index refers to a complex field",
                       std::string{entry_name});
        return false;
    }
    auto instruction_attribute = field.attribute("w:instr");
    if (instruction_attribute == pugi::xml_attribute{}) {
        instruction_attribute = field.append_attribute("w:instr");
    }
    if (instruction_attribute == pugi::xml_attribute{} ||
        !instruction_attribute.set_value(std::string{instruction}.c_str())) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to rewrite field instruction",
                       std::string{entry_name});
        return false;
    }

    for (auto child = field.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        field.remove_child(child);
        child = next;
    }
    if (!append_field_result_run(field, result_text)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to rewrite field result text",
                       std::string{entry_name});
        return false;
    }

    last_error_info.clear();
    return true;
}

#include "document_template_field_instruction_helpers.inc"

