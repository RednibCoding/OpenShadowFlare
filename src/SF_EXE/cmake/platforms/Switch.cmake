function(osf_configure_switch_platform target)
  target_sources(
    ${target}
    PRIVATE
      runtime/platform/switch/application_loop.cpp
  )
  target_compile_definitions(${target} PRIVATE OSF_PLATFORM_SWITCH)

  if(COMMAND nx_create_nro)
    nx_create_nro(${target})
  else()
    message(
      WARNING
      "nx_create_nro is unavailable: no .nro will be produced. "
      "Configure with -DCMAKE_TOOLCHAIN_FILE=$ENV{DEVKITPRO}/cmake/Switch.cmake.")
  endif()
endfunction()
