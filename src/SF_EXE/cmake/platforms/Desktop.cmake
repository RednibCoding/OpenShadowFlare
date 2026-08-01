function(osf_configure_desktop_platform target)
  target_sources(
    ${target}
    PRIVATE
      runtime/platform/desktop/application_loop.cpp
      runtime/platform/desktop/text_input.cpp
  )
endfunction()
