function(osf_configure_presentation target)
  if(OPENSHADOWFLARE_PRESENTATION_BACKEND STREQUAL "lgl")
    target_sources(
      ${target}
      PRIVATE
        runtime/presentation/lgl_surface_presenter.cpp
    )
    target_link_libraries(${target} PRIVATE Lgl::Lgl)
  elseif(OPENSHADOWFLARE_PRESENTATION_BACKEND STREQUAL "gu")
    if(NOT PSP)
      message(FATAL_ERROR
        "The gu presentation backend is supported only by the PSP target")
    endif()
    # PSP-only: blit the software surface with sceGu instead of OpenGL.
    target_sources(
      ${target}
      PRIVATE
        runtime/presentation/gu_surface_presenter.cpp
    )
    target_link_libraries(
      ${target}
      PRIVATE
        pspgum
        pspgu
        pspge
        pspdisplay
    )
  else()
    message(FATAL_ERROR
      "Unsupported presentation backend: "
      "${OPENSHADOWFLARE_PRESENTATION_BACKEND}")
  endif()
endfunction()
