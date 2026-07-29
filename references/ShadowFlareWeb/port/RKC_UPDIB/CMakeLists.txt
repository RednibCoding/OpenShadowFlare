add_library(RKC_UPDIB STATIC
    src/RKC_UPDIB.c
    src/RKC_UPDIB_SET.c
)

target_include_directories(RKC_UPDIB PUBLIC include)
target_link_libraries(RKC_UPDIB PUBLIC RKC_DIB RKC_FILE RK_FUNCTION)

if(SFDE_BUILD_TESTS)
    add_executable(RKC_UPDIB_test tests/test_RKC_UPDIB.c)
    target_link_libraries(RKC_UPDIB_test PRIVATE RKC_UPDIB)
    add_test(NAME RKC_UPDIB_test COMMAND RKC_UPDIB_test)

    add_executable(RKC_UPDIB_SET_test tests/test_RKC_UPDIB_SET.c)
    target_link_libraries(RKC_UPDIB_SET_test PRIVATE RKC_UPDIB)
    add_test(NAME RKC_UPDIB_SET_test COMMAND RKC_UPDIB_SET_test)
endif()
