add_executable(
  ShadowFlare_rebuilt
  src/main.cpp
)

target_compile_features(ShadowFlare_rebuilt PRIVATE cxx_std_17)
set_target_properties(
  ShadowFlare_rebuilt
  PROPERTIES
    CXX_EXTENSIONS OFF
)

target_link_libraries(
  ShadowFlare_rebuilt
  PRIVATE
    Lwl::Lwl
    Lal::Lal
    Lgl::Lgl
)

if(MINGW)
  target_link_options(
    ShadowFlare_rebuilt
    PRIVATE
      -static-libgcc
      -static-libstdc++
  )
endif()

if(MSVC)
  target_compile_options(ShadowFlare_rebuilt PRIVATE /W4 /permissive-)
else()
  target_compile_options(
    ShadowFlare_rebuilt
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
  )
endif()
