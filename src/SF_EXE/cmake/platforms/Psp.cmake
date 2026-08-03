function(osf_configure_psp_platform target)
  if(NOT TARGET osf_psp_sdk)
    add_library(osf_psp_sdk INTERFACE)
    target_link_libraries(
      osf_psp_sdk
      INTERFACE
        pspgum
        pspgu
        pspaudio
        pspdebug
        pspdisplay
        pspge
        pspctrl
        pspnet
        pspnet_apctl
        psputility
    )
  endif()

  target_sources(
    ${target}
    PRIVATE
      runtime/platform/psp/application_loop.cpp
      runtime/platform/psp/psp_main.c
      runtime/platform/psp/text_input.cpp
  )
  target_compile_definitions(${target} PRIVATE OSF_PLATFORM_PSP)
  target_link_libraries(${target} PRIVATE osf_psp_sdk)

  if(COMMAND create_pbp_file)
    create_pbp_file(
      TARGET ${target}
      TITLE "OpenShadowFlare"
    )
  else()
    message(FATAL_ERROR
      "create_pbp_file is unavailable. Configure with "
      "-DCMAKE_TOOLCHAIN_FILE=$ENV{PSPDEV}/psp/share/pspdev.cmake.")
  endif()
endfunction()
