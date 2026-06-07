auto omml_node_xml(pugi::xml_node node) -> std::string {
    string_xml_writer writer;
    node.print(writer, "", pugi::format_raw);
    return writer.text;
}

auto omml_root_is_supported(std::string_view name) -> bool {
    return name == "m:oMath" || name == "m:oMathPara";
}

void collect_omml_text_from_node(pugi::xml_node node, std::string &text) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto name = std::string_view{child.name()};
        if (name == "m:t" || name == "w:t") {
            text += child.text().get();
            continue;
        }
        collect_omml_text_from_node(child, text);
    }
}

void collect_omml_nodes_in_order(pugi::xml_node node,
                                 std::vector<pugi::xml_node> &nodes) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto name = std::string_view{child.name()};
        if (name == "m:oMathPara" || name == "m:oMath") {
            nodes.push_back(child);
            continue;
        }
        collect_omml_nodes_in_order(child, nodes);
    }
}

auto summarize_omml_node(pugi::xml_node node, std::size_t index)
    -> featherdoc::omml_summary {
    featherdoc::omml_summary summary;
    summary.index = index;
    summary.display = std::string_view{node.name()} == "m:oMathPara";
    collect_omml_text_from_node(node, summary.text);
    summary.xml = omml_node_xml(node);
    return summary;
}

auto summarize_omml_in_part(featherdoc::document_error_info &last_error_info,
                            pugi::xml_document &document)
    -> std::vector<featherdoc::omml_summary> {
    std::vector<pugi::xml_node> omml_nodes;
    collect_omml_nodes_in_order(document, omml_nodes);

    std::vector<featherdoc::omml_summary> summaries;
    summaries.reserve(omml_nodes.size());
    for (std::size_t index = 0; index < omml_nodes.size(); ++index) {
        summaries.push_back(summarize_omml_node(omml_nodes[index], index));
    }

    last_error_info.clear();
    return summaries;
}

auto parse_omml_fragment(featherdoc::document_error_info &last_error_info,
                         std::string_view entry_name,
                         std::string_view omml_xml,
                         pugi::xml_document &fragment) -> pugi::xml_node {
    if (omml_xml.empty()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "OMML XML must not be empty", std::string{entry_name});
        return {};
    }

    const auto parse_result = fragment.load_buffer(omml_xml.data(), omml_xml.size());
    if (!parse_result) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       parse_result.description(), std::string{entry_name},
                       parse_result.offset >= 0
                           ? std::optional<std::ptrdiff_t>{parse_result.offset}
                           : std::nullopt);
        return {};
    }

    const auto root = fragment.document_element();
    if (root == pugi::xml_node{} || !omml_root_is_supported(root.name())) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "OMML XML root must be m:oMath or m:oMathPara",
                       std::string{entry_name});
        return {};
    }

    return root;
}

void ensure_omml_namespace(pugi::xml_document &document) {
    auto root = document.document_element();
    if (root == pugi::xml_node{}) {
        return;
    }

    auto math_namespace = root.attribute("xmlns:m");
    if (math_namespace == pugi::xml_attribute{}) {
        math_namespace = root.append_attribute("xmlns:m");
    }
    if (math_namespace != pugi::xml_attribute{}) {
        math_namespace.set_value(std::string{math_namespace_uri}.c_str());
    }
}

auto append_omml_to_part(featherdoc::document_error_info &last_error_info,
                         pugi::xml_document &document,
                         std::string_view entry_name,
                         std::string_view omml_xml) -> bool {
    pugi::xml_document fragment;
    const auto omml_root = parse_omml_fragment(last_error_info, entry_name,
                                               omml_xml, fragment);
    if (omml_root == pugi::xml_node{}) {
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

    ensure_omml_namespace(document);
    auto paragraph = featherdoc::detail::append_paragraph_node(container);
    if (paragraph == pugi::xml_node{} ||
        paragraph.append_copy(omml_root) == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       std::string{std::string_view{omml_root.name()} == "m:oMathPara"
                                       ? "failed to append display OMML to template part"
                                       : "failed to append inline OMML to template part"},
                       std::string{entry_name});
        return false;
    }

    last_error_info.clear();
    return true;
}

auto replacement_omml_node(featherdoc::document_error_info &last_error_info,
                           std::string_view entry_name,
                           pugi::xml_node target,
                           pugi::xml_node replacement_root,
                           pugi::xml_document &wrapped_document)
    -> pugi::xml_node {
    const auto target_name = std::string_view{target.name()};
    const auto replacement_name = std::string_view{replacement_root.name()};
    if (target_name == replacement_name) {
        return replacement_root;
    }

    if (target_name == "m:oMathPara" && replacement_name == "m:oMath") {
        auto wrapper = wrapped_document.append_child("m:oMathPara");
        if (wrapper == pugi::xml_node{} ||
            wrapper.append_copy(replacement_root) == pugi::xml_node{}) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to wrap inline OMML for display replacement",
                           std::string{entry_name});
            return {};
        }
        return wrapper;
    }

    set_last_error(last_error_info,
                   std::make_error_code(std::errc::invalid_argument),
                   "display OMML cannot replace an inline OMML target",
                   std::string{entry_name});
    return {};
}

auto replace_omml_in_part(featherdoc::document_error_info &last_error_info,
                          pugi::xml_document &document,
                          std::string_view entry_name,
                          std::size_t omml_index,
                          std::string_view omml_xml) -> bool {
    std::vector<pugi::xml_node> omml_nodes;
    collect_omml_nodes_in_order(document, omml_nodes);
    if (omml_index >= omml_nodes.size()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "OMML index is out of range", std::string{entry_name});
        return false;
    }

    pugi::xml_document fragment;
    const auto omml_root = parse_omml_fragment(last_error_info, entry_name,
                                               omml_xml, fragment);
    if (omml_root == pugi::xml_node{}) {
        return false;
    }

    pugi::xml_document wrapped_document;
    auto target = omml_nodes[omml_index];
    const auto replacement = replacement_omml_node(
        last_error_info, entry_name, target, omml_root, wrapped_document);
    if (replacement == pugi::xml_node{}) {
        return false;
    }

    auto parent = target.parent();
    if (parent == pugi::xml_node{}) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to replace OMML node", std::string{entry_name});
        return false;
    }

    if (std::string_view{target.name()} == "m:oMathPara" &&
        std::string_view{parent.name()} != "w:p") {
        auto paragraph = featherdoc::detail::insert_paragraph_node(parent, target);
        if (paragraph == pugi::xml_node{} ||
            paragraph.append_copy(replacement) == pugi::xml_node{} ||
            !parent.remove_child(target)) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to replace OMML node", std::string{entry_name});
            return false;
        }
    } else if (parent.insert_copy_before(replacement, target) == pugi::xml_node{} ||
               !parent.remove_child(target)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to replace OMML node", std::string{entry_name});
        return false;
    }

    ensure_omml_namespace(document);
    last_error_info.clear();
    return true;
}

auto remove_omml_in_part(featherdoc::document_error_info &last_error_info,
                         pugi::xml_document &document,
                         std::string_view entry_name,
                         std::size_t omml_index) -> bool {
    std::vector<pugi::xml_node> omml_nodes;
    collect_omml_nodes_in_order(document, omml_nodes);
    if (omml_index >= omml_nodes.size()) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "OMML index is out of range", std::string{entry_name});
        return false;
    }

    auto target = omml_nodes[omml_index];
    auto parent = target.parent();
    if (parent == pugi::xml_node{} || !parent.remove_child(target)) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to remove OMML node", std::string{entry_name});
        return false;
    }

    last_error_info.clear();
    return true;
}

bool xml_text_looks_like_omml(std::string_view text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    return first != std::string_view::npos && text.substr(first).starts_with("<m:");
}

std::string escape_xml_text(std::string_view text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (const char ch : text) {
        switch (ch) {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&apos;"; break;
        default: escaped.push_back(ch); break;
        }
    }
    return escaped;
}

std::string omml_run_xml(std::string_view text) {
    return "<m:r><m:t>" + escape_xml_text(text) + "</m:t></m:r>";
}

std::string omml_expression_xml(std::string_view text) {
    if (xml_text_looks_like_omml(text)) {
        return std::string{text};
    }
    return omml_run_xml(text);
}

std::string wrap_omml_root(std::string inner_xml, bool display) {
    constexpr auto xmlns =
        " xmlns:m=\"http://schemas.openxmlformats.org/officeDocument/2006/math\"";
    if (display) {
        return std::string{"<m:oMathPara"} + xmlns + "><m:oMath>" +
               std::move(inner_xml) + "</m:oMath></m:oMathPara>";
    }
    return std::string{"<m:oMath"} + xmlns + ">" + std::move(inner_xml) +
           "</m:oMath>";
}
