add_library(RKC_RPG_AICONTROL STATIC
    src/RKC_RPG_AICONTROL.c
)

target_include_directories(RKC_RPG_AICONTROL PUBLIC include)
target_link_libraries(RKC_RPG_AICONTROL PUBLIC RKC_FILE)

if(SFDE_BUILD_TESTS)
    add_executable(RKC_RPG_AICONTROL_test tests/test_RKC_RPG_AICONTROL.c)
    target_link_libraries(RKC_RPG_AICONTROL_test PRIVATE RKC_RPG_AICONTROL)
    add_test(NAME RKC_RPG_AICONTROL_test COMMAND RKC_RPG_AICONTROL_test)
endif()
