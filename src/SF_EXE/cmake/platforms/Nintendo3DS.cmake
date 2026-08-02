function(osf_configure_nintendo_3ds_platform target)
  target_sources(
    ${target}
    PRIVATE
      runtime/platform/n3ds/application_loop.cpp
  )
  target_compile_definitions(${target} PRIVATE OSF_PLATFORM_N3DS)

  set(smdh "${CMAKE_CURRENT_BINARY_DIR}/OpenShadowFlare.smdh")
  set(icon "${CMAKE_SOURCE_DIR}/assets/n3ds/icon.png")
  ctr_generate_smdh(
    OUTPUT "${smdh}"
    NAME "OpenShadowFlare"
    DESCRIPTION "OpenShadowFlare for Nintendo 3DS"
    AUTHOR "OpenShadowFlare contributors"
    ICON "${icon}"
  )
  ctr_create_3dsx(
    OpenShadowFlare
    TARGET ${target}
    OUTPUT "OpenShadowFlare.3dsx"
    SMDH "${smdh}"
  )
endfunction()
