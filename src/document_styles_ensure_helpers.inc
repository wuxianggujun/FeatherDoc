auto ensure_paragraph_properties_node(pugi::xml_node paragraph)
    -> pugi::xml_node {
    if (paragraph == pugi::xml_node{}) {
        return {};
    }

    auto paragraph_properties = paragraph.child("w:pPr");
    if (paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }

    if (const auto first_child = paragraph.first_child();
        first_child != pugi::xml_node{}) {
        return paragraph.insert_child_before("w:pPr", first_child);
    }

    return paragraph.append_child("w:pPr");
}

auto ensure_run_properties_node(pugi::xml_node run) -> pugi::xml_node {
    if (run == pugi::xml_node{}) {
        return {};
    }

    auto run_properties = run.child("w:rPr");
    if (run_properties != pugi::xml_node{}) {
        return run_properties;
    }

    if (const auto first_child = run.first_child();
        first_child != pugi::xml_node{}) {
        return run.insert_child_before("w:rPr", first_child);
    }

    return run.append_child("w:rPr");
}

auto ensure_style_run_properties_node(pugi::xml_node style) -> pugi::xml_node {
    if (style == pugi::xml_node{}) {
        return {};
    }

    auto run_properties = style.child("w:rPr");
    if (run_properties != pugi::xml_node{}) {
        return run_properties;
    }

    if (const auto paragraph_properties = style.child("w:pPr");
        paragraph_properties != pugi::xml_node{}) {
        return style.insert_child_after("w:rPr", paragraph_properties);
    }

    return style.append_child("w:rPr");
}

auto ensure_style_paragraph_properties_node(pugi::xml_node style)
    -> pugi::xml_node {
    if (style == pugi::xml_node{}) {
        return {};
    }

    auto paragraph_properties = style.child("w:pPr");
    if (paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }

    if (const auto run_properties = style.child("w:rPr");
        run_properties != pugi::xml_node{}) {
        return style.insert_child_before("w:pPr", run_properties);
    }

    return style.append_child("w:pPr");
}

auto ensure_run_fonts_node(pugi::xml_node run_properties) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto run_fonts = run_properties.child("w:rFonts");
    if (run_fonts != pugi::xml_node{}) {
        return run_fonts;
    }

    if (const auto run_style = run_properties.child("w:rStyle");
        run_style != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:rFonts", run_style);
    }

    if (const auto first_child = run_properties.first_child();
        first_child != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:rFonts", first_child);
    }

    return run_properties.append_child("w:rFonts");
}

auto ensure_run_language_node(pugi::xml_node run_properties) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto run_language = run_properties.child("w:lang");
    if (run_language != pugi::xml_node{}) {
        return run_language;
    }

    if (const auto run_fonts = run_properties.child("w:rFonts");
        run_fonts != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:lang", run_fonts);
    }

    if (const auto run_style = run_properties.child("w:rStyle");
        run_style != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:lang", run_style);
    }

    if (const auto first_child = run_properties.first_child();
        first_child != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:lang", first_child);
    }

    return run_properties.append_child("w:lang");
}

auto ensure_run_rtl_node(pugi::xml_node run_properties) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto rtl = run_properties.child("w:rtl");
    if (rtl != pugi::xml_node{}) {
        return rtl;
    }

    if (const auto run_language = run_properties.child("w:lang");
        run_language != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:rtl", run_language);
    }

    if (const auto run_fonts = run_properties.child("w:rFonts");
        run_fonts != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:rtl", run_fonts);
    }

    if (const auto run_style = run_properties.child("w:rStyle");
        run_style != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:rtl", run_style);
    }

    if (const auto first_child = run_properties.first_child();
        first_child != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:rtl", first_child);
    }

    return run_properties.append_child("w:rtl");
}

auto ensure_run_text_color_node(pugi::xml_node run_properties)
    -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto color = run_properties.child("w:color");
    if (color != pugi::xml_node{}) {
        return color;
    }

    return run_properties.append_child("w:color");
}

auto ensure_run_bold_node(pugi::xml_node run_properties) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto bold = run_properties.child("w:b");
    if (bold != pugi::xml_node{}) {
        return bold;
    }

    if (const auto italic = run_properties.child("w:i");
        italic != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:b", italic);
    }
    if (const auto color = run_properties.child("w:color");
        color != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:b", color);
    }

    return run_properties.append_child("w:b");
}

auto ensure_run_italic_node(pugi::xml_node run_properties) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto italic = run_properties.child("w:i");
    if (italic != pugi::xml_node{}) {
        return italic;
    }

    if (const auto bold = run_properties.child("w:b");
        bold != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:i", bold);
    }
    if (const auto color = run_properties.child("w:color");
        color != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:i", color);
    }
    if (const auto size = run_properties.child("w:sz");
        size != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:i", size);
    }

    return run_properties.append_child("w:i");
}

auto ensure_run_underline_node(pugi::xml_node run_properties)
    -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto underline = run_properties.child("w:u");
    if (underline != pugi::xml_node{}) {
        return underline;
    }

    if (const auto size = run_properties.child("w:sz");
        size != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:u", size);
    }
    if (const auto italic = run_properties.child("w:i");
        italic != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:u", italic);
    }
    if (const auto bold = run_properties.child("w:b");
        bold != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:u", bold);
    }
    if (const auto color = run_properties.child("w:color");
        color != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:u", color);
    }

    return run_properties.append_child("w:u");
}

auto ensure_run_strikethrough_node(pugi::xml_node run_properties)
    -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto strike = run_properties.child("w:strike");
    if (strike != pugi::xml_node{}) {
        return strike;
    }

    if (const auto underline = run_properties.child("w:u");
        underline != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:strike", underline);
    }
    if (const auto size = run_properties.child("w:sz");
        size != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:strike", size);
    }
    if (const auto italic = run_properties.child("w:i");
        italic != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:strike", italic);
    }
    if (const auto bold = run_properties.child("w:b");
        bold != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:strike", bold);
    }

    return run_properties.append_child("w:strike");
}

auto ensure_run_vertical_align_node(pugi::xml_node run_properties)
    -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto vertical_align = run_properties.child("w:vertAlign");
    if (vertical_align != pugi::xml_node{}) {
        return vertical_align;
    }

    if (const auto font_size = run_properties.child("w:szCs");
        font_size != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:vertAlign", font_size);
    }
    if (const auto font_size = run_properties.child("w:sz");
        font_size != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:vertAlign", font_size);
    }
    if (const auto underline = run_properties.child("w:u");
        underline != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:vertAlign", underline);
    }

    return run_properties.append_child("w:vertAlign");
}

auto ensure_run_font_size_node(pugi::xml_node run_properties,
                               const char *child_name) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto font_size = run_properties.child(child_name);
    if (font_size != pugi::xml_node{}) {
        return font_size;
    }

    const auto name = std::string_view{child_name};
    if (name == "w:szCs") {
        if (const auto primary_size = run_properties.child("w:sz");
            primary_size != pugi::xml_node{}) {
            return run_properties.insert_child_after(child_name, primary_size);
        }
    } else if (const auto complex_size = run_properties.child("w:szCs");
               complex_size != pugi::xml_node{}) {
        return run_properties.insert_child_before(child_name, complex_size);
    }

    if (const auto underline = run_properties.child("w:u");
        underline != pugi::xml_node{}) {
        return run_properties.insert_child_after(child_name, underline);
    }
    if (const auto color = run_properties.child("w:color");
        color != pugi::xml_node{}) {
        return run_properties.insert_child_after(child_name, color);
    }
    if (const auto italic = run_properties.child("w:i");
        italic != pugi::xml_node{}) {
        return run_properties.insert_child_after(child_name, italic);
    }
    if (const auto bold = run_properties.child("w:b");
        bold != pugi::xml_node{}) {
        return run_properties.insert_child_after(child_name, bold);
    }

    return run_properties.append_child(child_name);
}

auto ensure_paragraph_bidi_node(pugi::xml_node paragraph_properties)
    -> pugi::xml_node {
    if (paragraph_properties == pugi::xml_node{}) {
        return {};
    }

    auto bidi = paragraph_properties.child("w:bidi");
    if (bidi != pugi::xml_node{}) {
        return bidi;
    }

    return paragraph_properties.append_child("w:bidi");
}

auto ensure_doc_defaults_node(pugi::xml_node styles_root) -> pugi::xml_node {
    if (styles_root == pugi::xml_node{}) {
        return {};
    }

    auto doc_defaults = styles_root.child("w:docDefaults");
    if (doc_defaults != pugi::xml_node{}) {
        return doc_defaults;
    }

    if (const auto first_child = styles_root.first_child();
        first_child != pugi::xml_node{}) {
        return styles_root.insert_child_before("w:docDefaults", first_child);
    }

    return styles_root.append_child("w:docDefaults");
}

auto ensure_doc_defaults_run_properties_node(pugi::xml_node styles_root)
    -> pugi::xml_node {
    auto doc_defaults = ensure_doc_defaults_node(styles_root);
    if (doc_defaults == pugi::xml_node{}) {
        return {};
    }

    auto run_properties_default = doc_defaults.child("w:rPrDefault");
    if (run_properties_default == pugi::xml_node{}) {
        if (const auto first_child = doc_defaults.first_child();
            first_child != pugi::xml_node{}) {
            run_properties_default =
                doc_defaults.insert_child_before("w:rPrDefault", first_child);
        } else {
            run_properties_default = doc_defaults.append_child("w:rPrDefault");
        }
    }
    if (run_properties_default == pugi::xml_node{}) {
        return {};
    }

    auto run_properties = run_properties_default.child("w:rPr");
    if (run_properties != pugi::xml_node{}) {
        return run_properties;
    }

    if (const auto first_child = run_properties_default.first_child();
        first_child != pugi::xml_node{}) {
        return run_properties_default.insert_child_before("w:rPr", first_child);
    }

    return run_properties_default.append_child("w:rPr");
}

auto ensure_doc_defaults_paragraph_properties_node(pugi::xml_node styles_root)
    -> pugi::xml_node {
    auto doc_defaults = ensure_doc_defaults_node(styles_root);
    if (doc_defaults == pugi::xml_node{}) {
        return {};
    }

    auto paragraph_properties_default = doc_defaults.child("w:pPrDefault");
    if (paragraph_properties_default == pugi::xml_node{}) {
        if (const auto run_properties_default =
                doc_defaults.child("w:rPrDefault");
            run_properties_default != pugi::xml_node{}) {
            paragraph_properties_default = doc_defaults.insert_child_after(
                "w:pPrDefault", run_properties_default);
        } else if (const auto first_child = doc_defaults.first_child();
                   first_child != pugi::xml_node{}) {
            paragraph_properties_default =
                doc_defaults.insert_child_before("w:pPrDefault", first_child);
        } else {
            paragraph_properties_default =
                doc_defaults.append_child("w:pPrDefault");
        }
    }
    if (paragraph_properties_default == pugi::xml_node{}) {
        return {};
    }

    auto paragraph_properties = paragraph_properties_default.child("w:pPr");
    if (paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }

    if (const auto first_child = paragraph_properties_default.first_child();
        first_child != pugi::xml_node{}) {
        return paragraph_properties_default.insert_child_before("w:pPr",
                                                                first_child);
    }

    return paragraph_properties_default.append_child("w:pPr");
}

auto find_style_node(pugi::xml_node styles_root,
                     std::string_view style_id) -> pugi::xml_node;

auto style_properties_anchor(pugi::xml_node style) -> pugi::xml_node {
    if (const auto paragraph_properties = style.child("w:pPr");
        paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }
    if (const auto run_properties = style.child("w:rPr");
        run_properties != pugi::xml_node{}) {
        return run_properties;
    }
    if (const auto table_properties = style.child("w:tblPr");
        table_properties != pugi::xml_node{}) {
        return table_properties;
    }
    if (const auto table_cell_properties = style.child("w:tcPr");
        table_cell_properties != pugi::xml_node{}) {
        return table_cell_properties;
    }
    return style.child("w:tblStylePr");
}

auto ensure_style_child_before_properties(
    pugi::xml_node style, const char *child_name) -> pugi::xml_node {
    if (style == pugi::xml_node{}) {
        return {};
    }

    auto child = style.child(child_name);
    if (child != pugi::xml_node{}) {
        return child;
    }

    if (const auto anchor = style_properties_anchor(style);
        anchor != pugi::xml_node{}) {
        return style.insert_child_before(child_name, anchor);
    }

    return style.append_child(child_name);
}

auto ensure_style_name_node(pugi::xml_node style) -> pugi::xml_node {
    return ensure_style_child_before_properties(style, "w:name");
}

auto ensure_style_based_on_node(pugi::xml_node style) -> pugi::xml_node {
    if (style == pugi::xml_node{}) {
        return {};
    }

    auto based_on = style.child("w:basedOn");
    if (based_on != pugi::xml_node{}) {
        return based_on;
    }

    if (const auto name = style.child("w:name"); name != pugi::xml_node{}) {
        return style.insert_child_after("w:basedOn", name);
    }

    return ensure_style_child_before_properties(style, "w:basedOn");
}

auto ensure_style_next_node(pugi::xml_node style) -> pugi::xml_node {
    if (style == pugi::xml_node{}) {
        return {};
    }

    auto next = style.child("w:next");
    if (next != pugi::xml_node{}) {
        return next;
    }

    if (const auto based_on = style.child("w:basedOn");
        based_on != pugi::xml_node{}) {
        return style.insert_child_after("w:next", based_on);
    }
    if (const auto name = style.child("w:name"); name != pugi::xml_node{}) {
        return style.insert_child_after("w:next", name);
    }

    return ensure_style_child_before_properties(style, "w:next");
}

auto ensure_style_outline_level_node(pugi::xml_node paragraph_properties)
    -> pugi::xml_node {
    if (paragraph_properties == pugi::xml_node{}) {
        return {};
    }

    auto outline_level = paragraph_properties.child("w:outlineLvl");
    if (outline_level != pugi::xml_node{}) {
        return outline_level;
    }

    if (const auto bidi = paragraph_properties.child("w:bidi");
        bidi != pugi::xml_node{}) {
        return paragraph_properties.insert_child_before("w:outlineLvl", bidi);
    }

    if (const auto first_child = paragraph_properties.first_child();
        first_child != pugi::xml_node{}) {
        return paragraph_properties.insert_child_before("w:outlineLvl",
                                                        first_child);
    }

    return paragraph_properties.append_child("w:outlineLvl");
}

auto ensure_style_node(pugi::xml_node styles_root,
                       std::string_view style_id) -> pugi::xml_node {
    if (styles_root == pugi::xml_node{}) {
        return {};
    }

    if (auto style = find_style_node(styles_root, style_id);
        style != pugi::xml_node{}) {
        return style;
    }

    auto style = styles_root.append_child("w:style");
    if (style != pugi::xml_node{}) {
        ensure_attribute_value(style, "w:styleId", style_id);
    }
    return style;
}

auto find_style_node(pugi::xml_node styles_root,
                     std::string_view style_id) -> pugi::xml_node {
    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        if (std::string_view{style.attribute("w:styleId").value()} ==
            style_id) {
            return style;
        }
    }

    return {};
}
