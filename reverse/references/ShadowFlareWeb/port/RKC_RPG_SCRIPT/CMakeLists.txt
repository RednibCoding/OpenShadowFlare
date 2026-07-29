add_library(RKC_RPG_SCRIPT STATIC
    src/RKC_RPG_SCRIPT.c
)

target_include_directories(RKC_RPG_SCRIPT PUBLIC include)
target_link_libraries(RKC_RPG_SCRIPT PUBLIC RKC_FILE)

if(SFDE_BUILD_TESTS)
    add_executable(RKC_RPG_SCRIPT_test tests/test_RKC_RPG_SCRIPT.c)
    target_link_libraries(RKC_RPG_SCRIPT_test PRIVATE RKC_RPG_SCRIPT)
    add_test(NAME RKC_RPG_SCRIPT_test COMMAND RKC_RPG_SCRIPT_test)
endif()
