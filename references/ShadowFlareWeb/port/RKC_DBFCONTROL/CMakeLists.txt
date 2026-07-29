add_library(RKC_DBFCONTROL STATIC
    src/RKC_DBFCONTROL.c
)

target_include_directories(RKC_DBFCONTROL PUBLIC include)
target_link_libraries(RKC_DBFCONTROL PUBLIC RKC_DIB)

if(EMSCRIPTEN)
    target_compile_options(RKC_DBFCONTROL PUBLIC "-sUSE_SDL=2")
    target_link_options(RKC_DBFCONTROL PUBLIC "-sUSE_SDL=2")
else()
    if(WIN32)
        # Hint for the MSYS2/UCRT64 toolchain this project builds with
        # (installed via: pacman -S mingw-w64-ucrt-x86_64-SDL2). Harmless if
        # SDL2 is already discoverable another way.
        list(APPEND CMAKE_PREFIX_PATH "C:/msys64/ucrt64")
    endif()
    find_package(SDL2 REQUIRED CONFIG)
    target_link_libraries(RKC_DBFCONTROL PUBLIC SDL2::SDL2)
endif()

if(SFDE_BUILD_TESTS)
    add_executable(RKC_DBFCONTROL_test tests/test_RKC_DBFCONTROL.c)
    target_link_libraries(RKC_DBFCONTROL_test PRIVATE RKC_DBFCONTROL App)
    add_test(NAME RKC_DBFCONTROL_test COMMAND RKC_DBFCONTROL_test)
endif()
