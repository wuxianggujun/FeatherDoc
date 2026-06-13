#pragma once

#include "featherdoc_cli_style_numbering_audit.hpp"

namespace featherdoc_cli {

void inspect_style_numbering_audit(
    const style_numbering_audit_result &result, bool json_output);

void inspect_style_numbering_repair(
    const style_numbering_repair_result &result, bool json_output);

} // namespace featherdoc_cli
