add_library(RK_FUNCTION STATIC
    src/RK_FUNCTION_Lz.c
    src/RK_FUNCTION_Huffman.c
    src/RK_FUNCTION_HuffmanTree.c
)

target_include_directories(RK_FUNCTION PUBLIC include)

if(SFDE_BUILD_TESTS)
    add_executable(RK_FUNCTION_test tests/test_RK_FUNCTION.c)
    target_link_libraries(RK_FUNCTION_test PRIVATE RK_FUNCTION)
    add_test(NAME RK_FUNCTION_test COMMAND RK_FUNCTION_test)
endif()
