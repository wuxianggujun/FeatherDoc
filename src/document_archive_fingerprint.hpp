#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace featherdoc::detail {

// The source package fingerprint deliberately contains only ZIP central
// directory metadata. It identifies package parts without retaining or logging
// document contents and is stored in Document's ABI-neutral extension sidecar.
struct archive_entry_fingerprint final {
    std::string physical_name;
    std::string canonical_key;
    std::string identity_key;
    std::uint32_t crc32{0U};
    std::uint64_t compressed_size{0U};
    std::uint64_t uncompressed_size{0U};
    bool is_directory{false};

    [[nodiscard]] auto
    operator==(const archive_entry_fingerprint &) const noexcept
        -> bool = default;
};

using source_archive_fingerprint = std::vector<archive_entry_fingerprint>;

} // namespace featherdoc::detail
