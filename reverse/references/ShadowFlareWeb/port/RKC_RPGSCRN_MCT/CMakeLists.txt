add_library(RKC_RPGSCRN_MCT STATIC
    src/RKC_RPGSCRN_MCT.c
)

target_include_directories(RKC_RPGSCRN_MCT PUBLIC include)
target_link_libraries(RKC_RPGSCRN_MCT PUBLIC RKC_FILE)

if(SFDE_BUILD_TESTS)
    add_executable(RKC_RPGSCRN_MCT_test tests/test_RKC_RPGSCRN_MCT.c)
    target_link_libraries(RKC_RPGSCRN_MCT_test PRIVATE RKC_RPGSCRN_MCT)
    add_test(NAME RKC_RPGSCRN_MCT_test COMMAND RKC_RPGSCRN_MCT_test)
endif()
