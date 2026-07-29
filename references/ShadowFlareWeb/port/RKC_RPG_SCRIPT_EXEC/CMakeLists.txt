add_library(RKC_RPG_SCRIPT_EXEC STATIC
    src/RKC_RPG_SCRIPT_EXEC.c
)

target_include_directories(RKC_RPG_SCRIPT_EXEC PUBLIC include)
target_link_libraries(RKC_RPG_SCRIPT_EXEC PUBLIC RKC_RPG_SCRIPT)

if(SFDE_BUILD_TESTS)
    add_executable(RKC_RPG_SCRIPT_EXEC_test tests/test_RKC_RPG_SCRIPT_EXEC.c)
    target_link_libraries(RKC_RPG_SCRIPT_EXEC_test PRIVATE RKC_RPG_SCRIPT_EXEC)
    add_test(NAME RKC_RPG_SCRIPT_EXEC_test COMMAND RKC_RPG_SCRIPT_EXEC_test)
endif()
