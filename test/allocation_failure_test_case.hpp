#pragma once

#include "doctest.h"

#define FEATHERDOC_DETAIL_JOIN_ALLOCATION_TEST_NAMES_IMPL(left, right)         \
    left##right
#define FEATHERDOC_DETAIL_JOIN_ALLOCATION_TEST_NAMES(left, right)              \
    FEATHERDOC_DETAIL_JOIN_ALLOCATION_TEST_NAMES_IMPL(left, right)

#if defined(FEATHERDOC_ENABLE_ALLOCATION_FAILURE_TESTS) &&                     \
    FEATHERDOC_ENABLE_ALLOCATION_FAILURE_TESTS
#define FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(test_name)                     \
    TEST_CASE(test_name * doctest::test_suite("allocation-failure"))
#else
// Keep the body type-checked in ordinary builds, but do not register a
// doctest case. In particular, doctest's --no-skip option cannot discover or
// execute these functions in Windows test binaries.
#define FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(test_name)                     \
    [[maybe_unused]] static void                                               \
        FEATHERDOC_DETAIL_JOIN_ALLOCATION_TEST_NAMES(                          \
            featherdoc_disabled_allocation_failure_test_, __LINE__)()
#endif
