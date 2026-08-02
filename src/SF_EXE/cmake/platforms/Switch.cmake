function(osf_configure_switch_platform target)
  target_sources(
    ${target}
    PRIVATE
      runtime/platform/switch/application_loop.cpp
  )
  if(OPENSHADOWFLARE_ENABLE_DEBUG_TOOLS)
    target_sources(
      ${target} PRIVATE runtime/platform/switch/memory_usage.cpp)
  endif()
  target_compile_definitions(${target} PRIVATE OSF_PLATFORM_SWITCH)

  if(COMMAND nx_create_nro)
    nx_create_nro(
      ${target}
      ICON "${PROJECT_SOURCE_DIR}/readme/sf-logo-sm.jpg"
    )
  else()
    message(
      WARNING
      "nx_create_nro is unavailable: no .nro will be produced. "
      "Configure with -DCMAKE_TOOLCHAIN_FILE=$ENV{DEVKITPRO}/cmake/Switch.cmake.")
  endif()
endfunction()
