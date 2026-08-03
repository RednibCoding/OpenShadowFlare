function(osf_configure_ps2_platform target)
  target_sources(
    ${target}
    PRIVATE
      runtime/platform/ps2/application_loop.cpp
      runtime/platform/ps2/ps2_data_backend.cpp
      runtime/platform/ps2/surface_presenter.cpp
  )
  target_link_libraries(${target} PRIVATE gskit dmakit)
endfunction()
