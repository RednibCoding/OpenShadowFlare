add_library(RKC_RPG_ITEMDATA STATIC
    src/RKC_RPG_ITEMDATA.c
)

target_include_directories(RKC_RPG_ITEMDATA PUBLIC include)
target_link_libraries(RKC_RPG_ITEMDATA PUBLIC RKC_FILE RK_FUNCTION)

if(SFDE_BUILD_TESTS)
    add_executable(RKC_RPG_ITEMDATA_test tests/test_RKC_RPG_ITEMDATA.c)
    target_link_libraries(RKC_RPG_ITEMDATA_test PRIVATE RKC_RPG_ITEMDATA)
    add_test(NAME RKC_RPG_ITEMDATA_test COMMAND RKC_RPG_ITEMDATA_test)
endif()
