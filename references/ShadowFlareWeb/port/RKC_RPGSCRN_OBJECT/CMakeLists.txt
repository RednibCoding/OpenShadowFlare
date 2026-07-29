add_library(RKC_RPGSCRN_OBJECT STATIC
    src/RKC_RPGSCRN_OBJECT.c
)

target_include_directories(RKC_RPGSCRN_OBJECT PUBLIC include)
target_link_libraries(RKC_RPGSCRN_OBJECT PUBLIC RKC_UPDIB RKC_FILE)

if(SFDE_BUILD_TESTS)
    add_executable(RKC_RPGSCRN_OBJECT_test tests/test_RKC_RPGSCRN_OBJECT.c)
    target_link_libraries(RKC_RPGSCRN_OBJECT_test PRIVATE RKC_RPGSCRN_OBJECT)
    add_test(NAME RKC_RPGSCRN_OBJECT_test COMMAND RKC_RPGSCRN_OBJECT_test)
endif()
