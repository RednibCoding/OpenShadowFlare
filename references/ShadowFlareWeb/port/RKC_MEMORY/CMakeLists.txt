add_library(RKC_MEMORY STATIC
    src/RKC_MEMORY.c
)

target_include_directories(RKC_MEMORY PUBLIC include)

if(SFDE_BUILD_TESTS)
    add_executable(RKC_MEMORY_test tests/test_RKC_MEMORY.c)
    target_link_libraries(RKC_MEMORY_test PRIVATE RKC_MEMORY)
    add_test(NAME RKC_MEMORY_test COMMAND RKC_MEMORY_test)
endif()
