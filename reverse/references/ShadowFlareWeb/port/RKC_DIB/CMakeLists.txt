add_library(RKC_DIB STATIC
    src/RKC_DIB.c
)

target_include_directories(RKC_DIB PUBLIC include)
target_link_libraries(RKC_DIB PUBLIC RKC_FILE)

if(SFDE_BUILD_TESTS)
    add_executable(RKC_DIB_test tests/test_RKC_DIB.c)
    target_link_libraries(RKC_DIB_test PRIVATE RKC_DIB)
    add_test(NAME RKC_DIB_test COMMAND RKC_DIB_test)
endif()
