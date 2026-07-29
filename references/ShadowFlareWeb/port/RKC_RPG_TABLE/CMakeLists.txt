add_library(RKC_RPG_TABLE STATIC
    src/RKC_RPG_TABLE.c
)

target_include_directories(RKC_RPG_TABLE PUBLIC include)
target_link_libraries(RKC_RPG_TABLE PUBLIC RKC_FILE RK_FUNCTION)

if(SFDE_BUILD_TESTS)
    add_executable(RKC_RPG_TABLE_test tests/test_RKC_RPG_TABLE.c)
    target_link_libraries(RKC_RPG_TABLE_test PRIVATE RKC_RPG_TABLE)
    add_test(NAME RKC_RPG_TABLE_test COMMAND RKC_RPG_TABLE_test)
endif()
