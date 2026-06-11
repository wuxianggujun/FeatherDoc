#include "featherdoc_cli_dispatch.hpp"
#include "featherdoc_cli_usage.hpp"

#include <iostream>
#include <string_view>
#include <vector>

int featherdoc_cli_main(int argc, char **argv) {
    const std::vector<std::string_view> arguments(argv + 1, argv + argc);
    if (arguments.empty()) {
        featherdoc_cli::print_usage(std::cerr);
        return 2;
    }

    return featherdoc_cli::run_featherdoc_cli_command(arguments);
}
