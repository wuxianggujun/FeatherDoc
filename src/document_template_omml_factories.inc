std::string make_omml_text(std::string_view text, bool display) {
    return wrap_omml_root(omml_expression_xml(text), display);
}

std::string make_omml_fraction(std::string_view numerator,
                               std::string_view denominator,
                               bool display) {
    return wrap_omml_root("<m:f><m:num>" + omml_expression_xml(numerator) +
                              "</m:num><m:den>" + omml_expression_xml(denominator) +
                              "</m:den></m:f>",
                          display);
}

std::string make_omml_superscript(std::string_view base,
                                  std::string_view superscript,
                                  bool display) {
    return wrap_omml_root("<m:sSup><m:e>" + omml_expression_xml(base) +
                              "</m:e><m:sup>" + omml_expression_xml(superscript) +
                              "</m:sup></m:sSup>",
                          display);
}

std::string make_omml_subscript(std::string_view base,
                                std::string_view subscript,
                                bool display) {
    return wrap_omml_root("<m:sSub><m:e>" + omml_expression_xml(base) +
                              "</m:e><m:sub>" + omml_expression_xml(subscript) +
                              "</m:sub></m:sSub>",
                          display);
}

std::string make_omml_radical(std::string_view radicand,
                              std::string_view degree,
                              bool display) {
    std::string inner = "<m:rad>";
    if (degree.empty()) {
        inner += "<m:radPr><m:degHide m:val=\"1\"/></m:radPr>";
    } else {
        inner += "<m:deg>" + omml_expression_xml(degree) + "</m:deg>";
    }
    inner += "<m:e>" + omml_expression_xml(radicand) + "</m:e></m:rad>";
    return wrap_omml_root(std::move(inner), display);
}

std::string make_omml_delimiter(std::string_view expression,
                                std::string_view begin,
                                std::string_view end,
                                bool display) {
    auto inner = std::string{"<m:d><m:dPr><m:begChr m:val=\""} +
                 escape_xml_text(begin) + "\"/><m:endChr m:val=\"" +
                 escape_xml_text(end) + "\"/></m:dPr><m:e>" +
                 omml_expression_xml(expression) + "</m:e></m:d>";
    return wrap_omml_root(std::move(inner), display);
}

std::string make_omml_nary(std::string_view operator_character,
                           std::string_view expression,
                           std::string_view lower_limit,
                           std::string_view upper_limit,
                           bool display) {
    auto inner = std::string{"<m:nary><m:naryPr><m:chr m:val=\""} +
                 escape_xml_text(operator_character) +
                 "\"/><m:limLoc m:val=\"undOvr\"/>";
    if (lower_limit.empty()) {
        inner += "<m:subHide m:val=\"1\"/>";
    }
    if (upper_limit.empty()) {
        inner += "<m:supHide m:val=\"1\"/>";
    }
    inner += "</m:naryPr>";
    if (!lower_limit.empty()) {
        inner += "<m:sub>" + omml_expression_xml(lower_limit) + "</m:sub>";
    }
    if (!upper_limit.empty()) {
        inner += "<m:sup>" + omml_expression_xml(upper_limit) + "</m:sup>";
    }
    inner += "<m:e>" + omml_expression_xml(expression) + "</m:e></m:nary>";
    return wrap_omml_root(std::move(inner), display);
}
