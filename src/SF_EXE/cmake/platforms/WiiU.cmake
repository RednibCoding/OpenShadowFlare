function(osf_configure_wiiu_platform target)
  target_sources(
    ${target}
    PRIVATE
      runtime/platform/wiiu/application_loop.cpp
  )
  target_compile_definitions(${target} PRIVATE OSF_PLATFORM_WIIU)

  set(_osf_wiiu_portlibs "$ENV{DEVKITPRO}/portlibs/wiiu")
  find_path(
    OSF_WIIU_SDL2_INCLUDE_DIR
    NAMES SDL.h
    PATHS "${_osf_wiiu_portlibs}/include/SDL2"
    NO_DEFAULT_PATH
    REQUIRED
  )
  find_library(
    OSF_WIIU_SDL2_LIBRARY
    NAMES SDL2
    PATHS "${_osf_wiiu_portlibs}/lib"
    NO_DEFAULT_PATH
    REQUIRED
  )
  target_include_directories(${target} PRIVATE "${OSF_WIIU_SDL2_INCLUDE_DIR}")
  target_link_libraries(${target} PRIVATE "${OSF_WIIU_SDL2_LIBRARY}")

  # The devkitPro Wii U CMake wrapper supplies WUT's Cafe OS import libraries
  # at the end of the link line.  Do not add gx2/vpad/coreinit manually.

  if(COMMAND wut_create_rpx)
    wut_create_rpx(${target})
  else()
    message(FATAL_ERROR
      "wut_create_rpx is unavailable. Configure with "
      "$ENV{DEVKITPRO}/portlibs/wiiu/bin/powerpc-eabi-cmake.")
  endif()

  if(COMMAND wut_create_wuhb)
    wut_create_wuhb(
      ${target}
      NAME "OpenShadowFlare"
      AUTHOR "OpenShadowFlare contributors"
      ICON "${PROJECT_SOURCE_DIR}/assets/wiiu/icon.png"
    )
  else()
    message(WARNING
      "wut_create_wuhb is unavailable: only OpenShadowFlare.rpx will be produced.")
  endif()
endfunction()
