add_library(RKC_RPGSCRN_GROUND STATIC
    src/RKC_RPGSCRN_GROUND.c
)

target_include_directories(RKC_RPGSCRN_GROUND PUBLIC include)
target_link_libraries(RKC_RPGSCRN_GROUND PUBLIC RKC_UPDIB RKC_FILE RK_FUNCTION)

if(SFDE_BUILD_TESTS)
    add_executable(RKC_RPGSCRN_GROUND_test tests/test_RKC_RPGSCRN_GROUND.c)
    target_link_libraries(RKC_RPGSCRN_GROUND_test PRIVATE RKC_RPGSCRN_GROUND)
    add_test(NAME RKC_RPGSCRN_GROUND_test COMMAND RKC_RPGSCRN_GROUND_test)
endif()
