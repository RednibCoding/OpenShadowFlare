function(osf_configure_vita_platform target)
  target_sources(
    ${target}
    PRIVATE
      runtime/platform/vita/application_loop.cpp
      runtime/platform/vita/text_input.cpp
  )
  target_compile_definitions(${target} PRIVATE OSF_PLATFORM_VITA)
  target_link_libraries(
    ${target}
    PRIVATE
      SceCommonDialog_stub
      SceIme_stub
      SceSysmodule_stub
  )

  if(NOT DEFINED ENV{VITASDK})
    message(FATAL_ERROR
      "VITASDK is not set. Configure with "
      "-DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake.")
  endif()

  include("$ENV{VITASDK}/share/vita.cmake")
  vita_create_self(OpenShadowFlare.self ${target})
  vita_create_vpk(
    OpenShadowFlare.vpk
    OSFLR0001
    OpenShadowFlare.self
    VERSION "01.00"
    NAME "OpenShadowFlare"
    FILE
      "${PROJECT_SOURCE_DIR}/assets/vita/sce_sys/icon0.png"
      "sce_sys/icon0.png"
  )
endfunction()
