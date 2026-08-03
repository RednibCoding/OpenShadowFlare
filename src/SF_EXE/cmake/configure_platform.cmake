function(osf_configure_platform target)
  if(EMSCRIPTEN)
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platforms/Emscripten.cmake")
    osf_configure_emscripten_platform(${target})
  elseif(PSP)
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platforms/Psp.cmake")
    osf_configure_psp_platform(${target})
  elseif(NINTENDO_SWITCH)
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platforms/Switch.cmake")
    osf_configure_switch_platform(${target})
  elseif(
      WIN32
      OR APPLE
      OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platforms/Desktop.cmake")
    osf_configure_desktop_platform(${target})
  else()
    message(FATAL_ERROR
      "OpenShadowFlare has no application host for "
      "${CMAKE_SYSTEM_NAME}")
  endif()
endfunction()
