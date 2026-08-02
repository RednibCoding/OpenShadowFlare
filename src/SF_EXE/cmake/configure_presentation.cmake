function(osf_configure_presentation target)
  if(OPENSHADOWFLARE_PRESENTATION_BACKEND STREQUAL "lgl")
    target_sources(
      ${target}
      PRIVATE
        runtime/presentation/lgl_surface_presenter.cpp
    )
    target_link_libraries(${target} PRIVATE Lgl::Lgl)
  elseif(OPENSHADOWFLARE_PRESENTATION_BACKEND STREQUAL "nxfb")
    target_sources(
      ${target}
      PRIVATE
        runtime/presentation/switch_surface_presenter.cpp
    )
  elseif(OPENSHADOWFLARE_PRESENTATION_BACKEND STREQUAL "vita")
    target_sources(
      ${target}
      PRIVATE
        runtime/presentation/vita_surface_presenter.cpp
    )
    target_link_libraries(${target} PRIVATE SceDisplay_stub)
  else()
    message(FATAL_ERROR
      "Unsupported presentation backend: "
      "${OPENSHADOWFLARE_PRESENTATION_BACKEND}")
  endif()
endfunction()
