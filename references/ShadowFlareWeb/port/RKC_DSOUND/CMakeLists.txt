add_library(RKC_DSOUND STATIC
    src/RKC_DSOUND.c
)

target_include_directories(RKC_DSOUND PUBLIC include)
target_link_libraries(RKC_DSOUND PUBLIC RKC_FILE)

if(EMSCRIPTEN)
    # -pthread is not optional here: Mix_OpenAudio itself (even for plain
    # Mix_Chunk playback, no Mix_Music/streaming involved) creates a
    # background thread and a semaphore internally, and both SDL_CreateThread
    # and SDL_CreateSemaphore are no-ops that fail outright under Emscripten
    # without pthreads -- confirmed empirically: Mix_OpenAudio returns -1
    # with Mix_GetError() == "Couldn't create audio thread startup semaphore"
    # without this flag, and 0/no error with it. Because this propagates to
    # the final link, anything that links RKC_DSOUND under Emscripten inherits
    # the pthreads requirement -- which in a real browser means the page must
    # be served with the COOP/COEP headers SharedArrayBuffer needs
    # (Cross-Origin-Opener-Policy: same-origin, Cross-Origin-Embedder-Policy:
    # require-corp); a plain `python -m http.server` does not set these.
    target_compile_options(RKC_DSOUND PUBLIC "-sUSE_SDL=2" "-sUSE_SDL_MIXER=2" "-pthread")
    target_link_options(RKC_DSOUND PUBLIC "-sUSE_SDL=2" "-sUSE_SDL_MIXER=2" "-pthread" "-sPTHREAD_POOL_SIZE=2")
else()
    if(WIN32)
        # Same MSYS2/UCRT64 hint as port/App's SDL2 lookup (installed via:
        # pacman -S mingw-w64-ucrt-x86_64-SDL2_mixer). Harmless if SDL2_mixer
        # is already discoverable another way.
        list(APPEND CMAKE_PREFIX_PATH "C:/msys64/ucrt64")
    endif()
    find_package(SDL2 REQUIRED CONFIG)
    find_package(SDL2_mixer REQUIRED CONFIG)
    target_link_libraries(RKC_DSOUND PUBLIC SDL2::SDL2 SDL2_mixer::SDL2_mixer)
endif()

if(SFDE_BUILD_TESTS)
    add_executable(RKC_DSOUND_test tests/test_RKC_DSOUND.c)
    target_link_libraries(RKC_DSOUND_test PRIVATE RKC_DSOUND)
    add_test(NAME RKC_DSOUND_test COMMAND RKC_DSOUND_test)
endif()
