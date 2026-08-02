function(osf_configure_desktop_platform target)
  target_sources(
    ${target}
    PRIVATE
      runtime/platform/desktop/application_loop.cpp
  )

  if(APPLE)
    option(
      OPENSHADOWFLARE_MACOS_BUNDLE
      "Build the macOS target as a .app bundle instead of a plain executable"
      OFF)
    if(OPENSHADOWFLARE_MACOS_BUNDLE)
      set_target_properties(
        ${target}
        PROPERTIES
          MACOSX_BUNDLE TRUE
          MACOSX_BUNDLE_BUNDLE_NAME "OpenShadowFlare"
          MACOSX_BUNDLE_GUI_IDENTIFIER "org.openshadowflare.game"
          MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
          MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
    endif()
  endif()
endfunction()
