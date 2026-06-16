#pragma once

#include "cli_test_support.hpp"
#include "pdf_import_test_support.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace {
namespace fs = std::filesystem;

auto pdf_fixture_path(std::string_view filename) -> fs::path {
    const auto fixture_filename = std::string(filename);
    const auto copied_fixture =
        test_binary_directory() / "pdf-fixtures" / fixture_filename;
    if (fs::exists(copied_fixture)) {
        return copied_fixture;
    }

    return test_binary_directory() / fixture_filename;
}

void open_imported_document(const fs::path &path,
                            featherdoc::Document &document) {
    document.set_path(path);
    REQUIRE_FALSE(document.open());
}

} // namespace
