#include <featherdoc.hpp>

#include <filesystem>
#include <system_error>
#include <type_traits>

namespace {

using document_open_signature =
    std::error_code (featherdoc::Document::*)();
using document_save_signature =
    std::error_code (featherdoc::Document::*)() const;
using document_save_as_signature =
    std::error_code (featherdoc::Document::*)(std::filesystem::path) const;

static_assert(std::is_same_v<
              decltype(static_cast<document_open_signature>(
                  &featherdoc::Document::open)),
              document_open_signature>);
static_assert(std::is_same_v<decltype(&featherdoc::Document::save),
                             document_save_signature>);
static_assert(std::is_same_v<decltype(&featherdoc::Document::save_as),
                             document_save_as_signature>);

} // namespace

// Frozen source-level consumer for the v1.13.2 public API. Extend this fixture
// only when preserving an existing 1.13.x call pattern.
int main() {
    featherdoc::Document document;
    if (document.create_empty()) {
        return 1;
    }

    auto paragraph = document.paragraphs();
    if (!paragraph.has_next()) {
        return 2;
    }

    auto run = paragraph.add_run("FeatherDoc v1.13.2 source compatibility");
    if (!run.has_next()) {
        return 3;
    }

    return document.is_open() ? 0 : 4;
}
