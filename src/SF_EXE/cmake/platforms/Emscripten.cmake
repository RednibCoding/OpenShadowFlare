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
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../../../gh-pages")
  set(web_shell_output "${CMAKE_CURRENT_BINARY_DIR}/index.html")
  add_custom_command(
    OUTPUT "${web_shell_output}"
    COMMAND
      ${CMAKE_COMMAND} -E copy
      "${web_shell}/index.html" "${CMAKE_CURRENT_BINARY_DIR}/index.html"
    COMMAND
      ${CMAKE_COMMAND} -E copy
      "${web_shell}/app.js" "${CMAKE_CURRENT_BINARY_DIR}/app.js"
    COMMAND
      ${CMAKE_COMMAND} -E copy
      "${web_shell}/style.css" "${CMAKE_CURRENT_BINARY_DIR}/style.css"
    DEPENDS
      "${web_shell}/index.html"
      "${web_shell}/app.js"
      "${web_shell}/style.css"
    COMMENT "Copying OpenShadowFlare web shell from gh-pages"
  )
  add_custom_target(
    shadowflare_web_shell ALL
    DEPENDS "${web_shell_output}"
  )
  add_dependencies(shadowflare_web_shell ${target})

  set(web_assets_dir "${CMAKE_CURRENT_BINARY_DIR}/assets")
  add_custom_command(
    TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${web_assets_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE:${target}>" "${web_assets_dir}/ShadowFlare_rebuilt.js"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE_DIR:${target}>/ShadowFlare_rebuilt.wasm"
      "${web_assets_dir}/ShadowFlare_rebuilt.wasm"
    COMMENT "Copying WebAssembly artifacts to assets/"
  )
endfunction()
