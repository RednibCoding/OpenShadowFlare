function(osf_configure_android_platform target)
  target_sources(
    ${target}
    PRIVATE
      runtime/platform/android/application_loop.cpp
  )
  target_compile_definitions(${target} PRIVATE OSF_PLATFORM_ANDROID)

  target_sources(
    ${target}
    PRIVATE
      "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c"
  )
  target_include_directories(
    ${target}
    PRIVATE
      "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue"
  )
  target_link_libraries(
    ${target}
    PRIVATE
      android
      log
  )

  set_target_properties(${target} PROPERTIES OUTPUT_NAME main)
endfunction()
