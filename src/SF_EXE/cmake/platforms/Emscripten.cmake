function(osf_configure_emscripten_platform target)
  target_sources(
    ${target}
    PRIVATE
      runtime/platform/web/application_loop.cpp
      runtime/platform/web/text_input.cpp
  )

  target_link_options(
    ${target}
    PRIVATE
      "-sMODULARIZE=1"
      "-sEXPORT_NAME=ShadowFlareModule"
      "-sEXPORTED_RUNTIME_METHODS=[FS,callMain]"
      "-sEXPORTED_FUNCTIONS=[_main]"
      "-sFORCE_FILESYSTEM=1"
      "-sALLOW_MEMORY_GROWTH=1"
      "-sINITIAL_MEMORY=268435456"
      "-sSTACK_SIZE=1048576"
      "-sEXIT_RUNTIME=0"
      "-sENVIRONMENT=web"
  )

  set(web_shell
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../web/index.html")
  set(web_shell_output "${CMAKE_CURRENT_BINARY_DIR}/index.html")
  add_custom_command(
    OUTPUT "${web_shell_output}"
    COMMAND
      ${CMAKE_COMMAND} -E copy
      "${web_shell}" "${web_shell_output}"
    DEPENDS "${web_shell}"
    COMMENT "Copying OpenShadowFlare web shell"
  )
  add_custom_target(
    shadowflare_web_shell ALL
    DEPENDS "${web_shell_output}"
  )
  add_dependencies(shadowflare_web_shell ${target})
endfunction()
