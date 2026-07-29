add_library(RKC_FILE STATIC
    src/RKC_FILE.c
)

target_include_directories(RKC_FILE PUBLIC include)

if(SFDE_BUILD_TESTS)
    add_executable(RKC_FILE_test tests/test_RKC_FILE.c)
    target_link_libraries(RKC_FILE_test PRIVATE RKC_FILE)
    add_test(NAME RKC_FILE_test COMMAND RKC_FILE_test)
endif()
