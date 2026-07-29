add_library(RKC_RPGSCRN_CAF STATIC
    src/RKC_RPGSCRN_CAF.c
)

target_include_directories(RKC_RPGSCRN_CAF PUBLIC include)
target_link_libraries(RKC_RPGSCRN_CAF PUBLIC RKC_UPDIB RKC_FILE)

if(SFDE_BUILD_TESTS)
    add_executable(RKC_RPGSCRN_CAF_test tests/test_RKC_RPGSCRN_CAF.c)
    target_link_libraries(RKC_RPGSCRN_CAF_test PRIVATE RKC_RPGSCRN_CAF)
    add_test(NAME RKC_RPGSCRN_CAF_test COMMAND RKC_RPGSCRN_CAF_test)
endif()
