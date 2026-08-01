function(osf_configure_presentation target)
  if(PLATFORM_PS2)
  elseif(OPENSHADOWFLARE_PRESENTATION_BACKEND STREQUAL "lgl")
    target_sources(
      ${target}
      PRIVATE
        runtime/presentation/lgl_surface_presenter.cpp
    )
    target_link_libraries(${target} PRIVATE Lgl::Lgl)
  else()
    message(FATAL_ERROR
      "Unsupported presentation backend: "
      "${OPENSHADOWFLARE_PRESENTATION_BACKEND}")
  endif()
endfunction()
