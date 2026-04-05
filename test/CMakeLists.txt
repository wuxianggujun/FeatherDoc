add_executable(
    unit_tests
    basic_tests.cpp
)

target_link_libraries(
    unit_tests
    PRIVATE FeatherDoc::FeatherDoc
)

add_test(
    NAME
    unit
    COMMAND
    unit_tests
)

add_executable(
    iterator_tests
    iterator_tests.cpp
)

target_link_libraries(
    iterator_tests
    PRIVATE FeatherDoc::FeatherDoc
)

add_test(
    NAME
    iterator
    COMMAND
    iterator_tests
)

configure_file(my_test.docx ${CMAKE_CURRENT_BINARY_DIR}/my_test.docx COPYONLY)
