function(osf_configure_presentation target)
  if(OPENSHADOWFLARE_PRESENTATION_BACKEND STREQUAL "lgl")
    target_sources(
      ${target}
      PRIVATE
        runtime/presentation/lgl_surface_presenter.cpp
    )
    target_link_libraries(${target} PRIVATE Lgl::Lgl)
  elseif(OPENSHADOWFLARE_PRESENTATION_BACKEND STREQUAL "n3ds")
    target_sources(
      ${target}
      PRIVATE
        runtime/presentation/n3ds_surface_presenter.cpp
    )
  elseif(OPENSHADOWFLARE_PRESENTATION_BACKEND STREQUAL "nxfb")
    target_sources(
      ${target}
      PRIVATE
        runtime/presentation/switch_surface_presenter.cpp
    )
  else()
    message(FATAL_ERROR
      "Unsupported presentation backend: "
      "${OPENSHADOWFLARE_PRESENTATION_BACKEND}")
  endif()
endfunction()
