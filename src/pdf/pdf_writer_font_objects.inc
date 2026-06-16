[[nodiscard]] pdfio_obj_t *
create_stream_object_from_bytes(pdfio_file_t *pdf, pdfio_dict_t *dict,
                                const unsigned char *data, std::size_t size,
                                std::string_view label,
                                std::string &error_message) {
    if (dict == nullptr) {
        error_message = "Unable to create PDF stream dictionary: " +
                        std::string{label};
        return nullptr;
    }

    pdfio_obj_t *object = pdfioFileCreateObj(pdf, dict);
    if (object == nullptr) {
        error_message =
            "Unable to create PDF stream object: " + std::string{label};
        return nullptr;
    }

    std::unique_ptr<pdfio_stream_t, PdfioStreamCloser> stream(
        pdfioObjCreateStream(object, PDFIO_FILTER_FLATE));
    if (!stream) {
        error_message =
            "Unable to open PDF stream object: " + std::string{label};
        return nullptr;
    }

    if (size > 0U && !pdfioStreamWrite(stream.get(), data, size)) {
        error_message =
            "Unable to write PDF stream object: " + std::string{label};
        return nullptr;
    }

    if (!pdfioStreamClose(stream.release())) {
        error_message =
            "Unable to close PDF stream object: " + std::string{label};
        return nullptr;
    }

    return object;
}

[[nodiscard]] pdfio_obj_t *
create_stream_object_from_text(pdfio_file_t *pdf, pdfio_dict_t *dict,
                               const std::string &text,
                               std::string_view label,
                               std::string &error_message) {
    return create_stream_object_from_bytes(
        pdf, dict, reinterpret_cast<const unsigned char *>(text.data()),
        text.size(), label, error_message);
}

[[nodiscard]] pdfio_obj_t *
create_font_file_object(pdfio_file_t *pdf, const FontResourceKey &key,
                        std::string &error_message) {
    auto font_data = read_binary_file(key.file_path);
    if (font_data.empty()) {
        error_message = "Unable to read PDF glyph font file: " +
                        key.file_path.string();
        return nullptr;
    }

    pdfio_dict_t *dict = pdfioDictCreate(pdf);
    if (dict != nullptr) {
        pdfioDictSetName(dict, "Filter", "FlateDecode");
    }
    return create_stream_object_from_bytes(pdf, dict, font_data.data(),
                                           font_data.size(), font_label(key),
                                           error_message);
}

[[nodiscard]] pdfio_obj_t *
create_cid_to_gid_map_object(pdfio_file_t *pdf,
                             const GlyphFontResource &resource,
                             std::string_view label,
                             std::string &error_message) {
    const std::size_t byte_count = (resource.entries.size() + 1U) * 2U;
    std::vector<unsigned char> cid_to_gid(byte_count, 0U);
    for (const auto &entry : resource.entries) {
        const auto offset = static_cast<std::size_t>(entry.cid) * 2U;
        cid_to_gid[offset] = static_cast<unsigned char>(entry.glyph_id >> 8U);
        cid_to_gid[offset + 1U] =
            static_cast<unsigned char>(entry.glyph_id & 0xFFU);
    }

    pdfio_dict_t *dict = pdfioDictCreate(pdf);
    if (dict != nullptr) {
        pdfioDictSetName(dict, "Filter", "FlateDecode");
    }
    return create_stream_object_from_bytes(pdf, dict, cid_to_gid.data(),
                                           cid_to_gid.size(), label,
                                           error_message);
}

[[nodiscard]] pdfio_obj_t *
create_to_unicode_object(pdfio_file_t *pdf, const GlyphFontResource &resource,
                         std::string_view label,
                         std::string &error_message) {
    std::string cmap;
    cmap += "/CIDInit /ProcSet findresource begin\n";
    cmap += "12 dict begin\n";
    cmap += "begincmap\n";
    cmap += "/CIDSystemInfo << /Registry (Adobe) /Ordering (UCS) "
            "/Supplement 0 >> def\n";
    cmap += "/CMapName /FeatherDocGlyphToUnicode def\n";
    cmap += "/CMapType 2 def\n";
    cmap += "1 begincodespacerange\n";
    cmap += "<0000> <FFFF>\n";
    cmap += "endcodespacerange\n";

    std::vector<const GlyphCidEntry *> unicode_entries;
    unicode_entries.reserve(resource.entries.size());
    for (const auto &entry : resource.entries) {
        if (entry.unicode_text.empty()) {
            continue;
        }
        unicode_entries.push_back(&entry);
    }

    constexpr std::size_t kEntriesPerChunk = 100U;
    for (std::size_t offset = 0U; offset < unicode_entries.size();
         offset += kEntriesPerChunk) {
        const auto end =
            std::min(unicode_entries.size(), offset + kEntriesPerChunk);
        cmap += std::to_string(end - offset);
        cmap += " beginbfchar\n";

        for (std::size_t index = offset; index < end; ++index) {
            const auto &entry = *unicode_entries[index];
            cmap += "<";
            append_utf16be_hex_unit(cmap, entry.cid);
            cmap += "> <";
            cmap += utf16be_hex_payload_from_utf8(entry.unicode_text);
            cmap += ">\n";
        }
        cmap += "endbfchar\n";
    }

    cmap += "endcmap\n";
    cmap += "CMapName currentdict /CMap defineresource pop\n";
    cmap += "end\n";
    cmap += "end\n";

    pdfio_dict_t *dict = pdfioDictCreate(pdf);
    if (dict != nullptr) {
        pdfioDictSetName(dict, "Type", "CMap");
        pdfioDictSetName(dict, "CMapName", "FeatherDocGlyphToUnicode");
        pdfioDictSetName(dict, "Filter", "FlateDecode");
    }
    return create_stream_object_from_text(pdf, dict, cmap, label,
                                          error_message);
}

[[nodiscard]] pdfio_array_t *
create_shaped_width_array(pdfio_file_t *pdf,
                          const GlyphFontResource &resource) {
    pdfio_array_t *widths = pdfioArrayCreate(pdf);
    if (widths == nullptr) {
        return nullptr;
    }

    constexpr std::size_t kWidthsPerChunk = 256U;
    for (std::size_t offset = 0U; offset < resource.entries.size();
         offset += kWidthsPerChunk) {
        pdfioArrayAppendNumber(widths, resource.entries[offset].cid);

        pdfio_array_t *chunk = pdfioArrayCreate(pdf);
        if (chunk == nullptr) {
            return nullptr;
        }

        const auto end =
            std::min(resource.entries.size(), offset + kWidthsPerChunk);
        for (std::size_t index = offset; index < end; ++index) {
            pdfioArrayAppendNumber(chunk, resource.entries[index].width_1000);
        }
        pdfioArrayAppendArray(widths, chunk);
    }

    return widths;
}

[[nodiscard]] pdfio_obj_t *
create_shaped_glyph_font(pdfio_file_t *pdf, const FontResourceKey &key,
                         const GlyphFontResource &resource,
                         bool subset_unicode_fonts,
                         std::string &error_message) {
    if (resource.entries.empty()) {
        error_message = "Shaped PDF glyph font has no glyphs: " +
                        font_label(key);
        return nullptr;
    }

    FreeTypeFaceOwner face_owner(key.file_path);
    if (!face_owner.valid()) {
        error_message =
            "Unable to load shaped PDF glyph font: " + key.file_path.string();
        return nullptr;
    }

    pdfio_obj_t *font_file = nullptr;
    std::vector<unsigned char> subset_font_data;
#if defined(FEATHERDOC_ENABLE_PDF_FONT_SUBSET)
    if (subset_unicode_fonts && !resource.glyph_ids.empty()) {
        const auto subset_result =
            subset_font_file_for_glyph_ids(key.file_path, resource.glyph_ids);
        if (subset_result.success && !subset_result.font_data.empty()) {
            subset_font_data = std::move(subset_result.font_data);
            pdfio_dict_t *dict = pdfioDictCreate(pdf);
            if (dict != nullptr) {
                pdfioDictSetName(dict, "Filter", "FlateDecode");
            }
            font_file = create_stream_object_from_bytes(
                pdf, dict, subset_font_data.data(), subset_font_data.size(),
                font_label(key), error_message);
        }
    }
#else
    (void)subset_unicode_fonts;
#endif
    if (font_file == nullptr) {
        font_file = create_font_file_object(pdf, key, error_message);
    }
    if (font_file == nullptr) {
        return nullptr;
    }

    const auto base_font = base_font_name_for(face_owner.face, key);
    pdfio_array_t *bbox = pdfioArrayCreate(pdf);
    if (bbox == nullptr) {
        error_message = "Unable to create shaped PDF font bounding box";
        return nullptr;
    }
    pdfioArrayAppendNumber(bbox, font_unit_to_1000(face_owner.face,
                                                   face_owner.face->bbox.xMin));
    pdfioArrayAppendNumber(bbox, font_unit_to_1000(face_owner.face,
                                                   face_owner.face->bbox.yMin));
    pdfioArrayAppendNumber(bbox, font_unit_to_1000(face_owner.face,
                                                   face_owner.face->bbox.xMax));
    pdfioArrayAppendNumber(bbox, font_unit_to_1000(face_owner.face,
                                                   face_owner.face->bbox.yMax));

    pdfio_dict_t *descriptor = pdfioDictCreate(pdf);
    if (descriptor == nullptr) {
        error_message = "Unable to create shaped PDF font descriptor";
        return nullptr;
    }
    pdfioDictSetName(descriptor, "Type", "FontDescriptor");
    pdfioDictSetName(descriptor, "FontName", base_font.c_str());
    pdfioDictSetObj(descriptor, "FontFile2", font_file);
    pdfioDictSetNumber(descriptor, "Flags", key.italic ? 0x60 : 0x20);
    pdfioDictSetArray(descriptor, "FontBBox", bbox);
    pdfioDictSetNumber(descriptor, "ItalicAngle", key.italic ? -12.0 : 0.0);
    pdfioDictSetNumber(descriptor, "Ascent",
                       font_unit_to_1000(face_owner.face,
                                         face_owner.face->ascender));
    pdfioDictSetNumber(descriptor, "Descent",
                       font_unit_to_1000(face_owner.face,
                                         face_owner.face->descender));
    pdfioDictSetNumber(descriptor, "CapHeight",
                       font_unit_to_1000(face_owner.face,
                                         face_owner.face->ascender));
    pdfioDictSetNumber(descriptor, "StemV", key.bold ? 120.0 : 80.0);

    pdfio_obj_t *descriptor_obj = pdfioFileCreateObj(pdf, descriptor);
    if (descriptor_obj == nullptr || !pdfioObjClose(descriptor_obj)) {
        error_message = "Unable to create shaped PDF font descriptor object";
        return nullptr;
    }

    pdfio_obj_t *cid_to_gid =
        create_cid_to_gid_map_object(pdf, resource, font_label(key),
                                     error_message);
    if (cid_to_gid == nullptr) {
        return nullptr;
    }

    pdfio_obj_t *to_unicode =
        create_to_unicode_object(pdf, resource, font_label(key), error_message);
    if (to_unicode == nullptr) {
        return nullptr;
    }

    pdfio_array_t *widths = create_shaped_width_array(pdf, resource);
    if (widths == nullptr) {
        error_message = "Unable to create shaped PDF glyph widths";
        return nullptr;
    }

    pdfio_dict_t *system_info = pdfioDictCreate(pdf);
    if (system_info == nullptr) {
        error_message = "Unable to create shaped PDF CID system info";
        return nullptr;
    }
    pdfioDictSetString(system_info, "Registry", "Adobe");
    pdfioDictSetString(system_info, "Ordering", "Identity");
    pdfioDictSetNumber(system_info, "Supplement", 0.0);

    pdfio_dict_t *cid_font = pdfioDictCreate(pdf);
    if (cid_font == nullptr) {
        error_message = "Unable to create shaped PDF CID font";
        return nullptr;
    }
    pdfioDictSetName(cid_font, "Type", "Font");
    pdfioDictSetName(cid_font, "Subtype", "CIDFontType2");
    pdfioDictSetName(cid_font, "BaseFont", base_font.c_str());
    pdfioDictSetDict(cid_font, "CIDSystemInfo", system_info);
    pdfioDictSetObj(cid_font, "CIDToGIDMap", cid_to_gid);
    pdfioDictSetObj(cid_font, "FontDescriptor", descriptor_obj);
    pdfioDictSetArray(cid_font, "W", widths);

    pdfio_obj_t *cid_font_obj = pdfioFileCreateObj(pdf, cid_font);
    if (cid_font_obj == nullptr || !pdfioObjClose(cid_font_obj)) {
        error_message = "Unable to create shaped PDF CID font object";
        return nullptr;
    }

    pdfio_array_t *descendants = pdfioArrayCreate(pdf);
    if (descendants == nullptr ||
        !pdfioArrayAppendObj(descendants, cid_font_obj)) {
        error_message = "Unable to create shaped PDF descendant font list";
        return nullptr;
    }

    pdfio_dict_t *type0 = pdfioDictCreate(pdf);
    if (type0 == nullptr) {
        error_message = "Unable to create shaped PDF Type0 font";
        return nullptr;
    }
    pdfioDictSetName(type0, "Type", "Font");
    pdfioDictSetName(type0, "Subtype", "Type0");
    pdfioDictSetName(type0, "BaseFont", base_font.c_str());
    pdfioDictSetArray(type0, "DescendantFonts", descendants);
    pdfioDictSetName(type0, "Encoding", "Identity-H");
    pdfioDictSetObj(type0, "ToUnicode", to_unicode);

    pdfio_obj_t *font = pdfioFileCreateObj(pdf, type0);
    if (font != nullptr && pdfioObjClose(font)) {
        return font;
    }

    error_message = "Unable to create shaped PDF Type0 font object";
    return nullptr;
}
