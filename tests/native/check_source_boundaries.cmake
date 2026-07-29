cmake_minimum_required(VERSION 3.20)

set(reconstructed_root "${PROJECT_ROOT}/src/reconstructed")
set(portable_root "${PROJECT_ROOT}/src/SF_EXE")
set(portable_libraries_root "${portable_root}/libs")

set(dll_names
  RK_FUNCTION
  RKC_DBFCONTROL
  RKC_DIB
  RKC_DSOUND
  RKC_FILE
  RKC_FONTMAKER
  RKC_MEMORY
  RKC_NETWORK
  RKC_RPG_AICONTROL
  RKC_RPG_SCRIPT
  RKC_RPG_TABLE
  RKC_RPGSCRN
  RKC_UPDIB
  RKC_WINDOW
)

foreach(shared_file IN ITEMS README.md debug.h utils.h)
  if(NOT EXISTS "${reconstructed_root}/${shared_file}")
    message(FATAL_ERROR
      "Reconstructed DLL support file is misplaced: ${shared_file}")
  endif()
endforeach()

foreach(legacy_file IN ITEMS debug.h utils.h)
  if(EXISTS "${PROJECT_ROOT}/src/${legacy_file}")
    message(FATAL_ERROR
      "Legacy DLL support file must not be restored: src/${legacy_file}")
  endif()
endforeach()

foreach(dll_name IN LISTS dll_names)
  if(NOT EXISTS "${reconstructed_root}/${dll_name}/src/core.cpp")
    message(FATAL_ERROR
      "${dll_name} implementation must live under src/reconstructed")
  endif()
  if(NOT EXISTS "${reconstructed_root}/${dll_name}/dll.def")
    message(FATAL_ERROR
      "${dll_name} exports must live under src/reconstructed")
  endif()
  if(EXISTS "${PROJECT_ROOT}/src/${dll_name}")
    message(FATAL_ERROR
      "Legacy source directory src/${dll_name} must not be restored")
  endif()
  if(NOT IS_DIRECTORY "${portable_libraries_root}/${dll_name}")
    message(FATAL_ERROR
      "${dll_name} needs a matching portable boundary under src/SF_EXE/libs")
  endif()
endforeach()

set(implemented_public_apis
  RK_FUNCTION/rk_function.hpp
  RKC_DBFCONTROL/rkc_dbfcontrol.hpp
  RKC_DIB/rkc_dib.hpp
  RKC_DSOUND/rkc_dsound.hpp
  RKC_RPG_TABLE/rkc_rpg_table.hpp
  RKC_RPGSCRN/rkc_rpgscrn.hpp
  RKC_RPG_SCRIPT/rkc_rpg_script.hpp
  RKC_UPDIB/rkc_updib.hpp
)

foreach(public_api IN LISTS implemented_public_apis)
  if(NOT EXISTS "${portable_libraries_root}/${public_api}")
    message(FATAL_ERROR
      "Portable DLL API must remain under src/SF_EXE/libs: ${public_api}")
  endif()

  get_filename_component(api_directory "${public_api}" DIRECTORY)
  file(
    GLOB api_headers
    LIST_DIRECTORIES false
    "${portable_libraries_root}/${api_directory}/*.hpp"
  )
  list(LENGTH api_headers api_header_count)
  if(NOT api_header_count EQUAL 1)
    message(FATAL_ERROR
      "${api_directory} must expose exactly one top-level public API header")
  endif()
endforeach()

set(dll_implementation_names
  bitmap.cpp
  caf.cpp
  coordinates.cpp
  display_hit_test.cpp
  display_order.cpp
  ground_map.cpp
  judgement.cpp
  njp.cpp
  object_map.cpp
  rclib_lz.cpp
  software_backend.cpp
  script_data.cpp
  script_engine.cpp
  table_database.cpp
  voc.cpp
  voc_player.cpp
)

file(
  GLOB_RECURSE portable_sources
  LIST_DIRECTORIES false
  "${portable_root}/*.cpp"
)

foreach(source_file IN LISTS portable_sources)
  get_filename_component(source_name "${source_file}" NAME)
  if(source_name IN_LIST dll_implementation_names)
    file(RELATIVE_PATH relative_source "${portable_libraries_root}" "${source_file}")
    if(relative_source MATCHES "^\\.\\.")
      message(FATAL_ERROR
        "DLL-derived implementation escaped src/SF_EXE/libs: ${source_file}")
    endif()
  endif()
endforeach()

set(portable_game_directories
  core
  gapi
  items
  render
  resources
  states
  ui
  world
)

set(portable_game_sources)
foreach(directory IN LISTS portable_game_directories)
  file(
    GLOB_RECURSE directory_sources
    LIST_DIRECTORIES false
    "${portable_root}/${directory}/*.cpp"
    "${portable_root}/${directory}/*.hpp"
  )
  list(APPEND portable_game_sources ${directory_sources})
endforeach()

foreach(source_file IN LISTS portable_game_sources)
  file(READ "${source_file}" source_text)
  if(source_text MATCHES
      "#[ \t]*include[ \t]*[<\"](windows\\.h|lwl\\.h|lal\\.h|lgl\\.h)[>\"]")
    message(FATAL_ERROR
      "Platform integration escaped src/SF_EXE/runtime or libs: ${source_file}")
  endif()
  if(source_text MATCHES
      "(^|[^A-Za-z0-9_])(_WIN32|WINAPI|HWND|lwl_|lal_|lgl_)")
    message(FATAL_ERROR
      "Platform-specific code escaped src/SF_EXE/runtime or libs: ${source_file}")
  endif()
  if(source_text MATCHES "src/reconstructed")
    message(FATAL_ERROR
      "Portable code must not include reconstructed Win32 sources: ${source_file}")
  endif()
endforeach()

function(osf_reject_layer_includes directory)
  set(forbidden_layers ${ARGN})
  file(
    GLOB_RECURSE layer_sources
    LIST_DIRECTORIES false
    "${portable_root}/${directory}/*.cpp"
    "${portable_root}/${directory}/*.hpp"
  )
  foreach(source_file IN LISTS layer_sources)
    file(READ "${source_file}" source_text)
    foreach(forbidden_layer IN LISTS forbidden_layers)
      if(source_text MATCHES
          "#[ \t]*include[ \t]*\"${forbidden_layer}/")
        message(FATAL_ERROR
          "${directory} must not depend on ${forbidden_layer}: ${source_file}")
      endif()
    endforeach()
  endforeach()
endfunction()

# These are source-level rules as well as CMake target rules. Keeping them here
# catches a direct include even when a broad transitive link would otherwise let
# the build succeed.
osf_reject_layer_includes(
  core gapi items resources states ui world render runtime libs)
osf_reject_layer_includes(
  gapi core items resources states ui world render runtime libs)
osf_reject_layer_includes(
  items core gapi resources states ui world render runtime)
osf_reject_layer_includes(
  resources core gapi items states ui world render runtime)
osf_reject_layer_includes(
  states gapi resources ui world render runtime libs)
osf_reject_layer_includes(
  world states ui render runtime)
osf_reject_layer_includes(
  ui render runtime)
osf_reject_layer_includes(
  render runtime)

file(
  GLOB_RECURSE runtime_sources
  LIST_DIRECTORIES false
  "${portable_root}/runtime/*.cpp"
  "${portable_root}/runtime/*.hpp"
)

foreach(source_file IN LISTS runtime_sources)
  file(RELATIVE_PATH relative_source "${portable_root}" "${source_file}")
  file(READ "${source_file}" source_text)

  if(NOT relative_source MATCHES "^runtime/platform/")
    if(source_text MATCHES
        "#[ \t]*include[ \t]*[<\"](emscripten[^>\"]*|jni\\.h|android/[^>\"]*)[>\"]")
      message(FATAL_ERROR
        "Platform SDK header escaped runtime/platform: ${source_file}")
    endif()
    if(source_text MATCHES
        "(^|[^A-Za-z0-9_])(__EMSCRIPTEN__|__ANDROID__|ANDROID|JNIEnv|_WIN32|WINAPI|HWND|__ORBIS__|__PROSPERO__|__NX__|NN_NINTENDO_SDK)([^A-Za-z0-9_]|$)")
      message(FATAL_ERROR
        "Platform-specific code escaped runtime/platform: ${source_file}")
    endif()
  endif()

  if(NOT relative_source MATCHES "^runtime/presentation/")
    if(source_text MATCHES
        "#[ \t]*include[ \t]*[<\"]lgl\\.h[>\"]|(^|[^A-Za-z0-9_])(LglSurfacePresenter|LwlGlContext|lgl_)")
      message(FATAL_ERROR
        "Presentation backend escaped runtime/presentation: ${source_file}")
    endif()
  endif()
endforeach()

file(READ "${portable_root}/CMakeLists.txt" executable_cmake)
if(executable_cmake MATCHES
    "(_lal_web_mix|-sEXPORTED_|-sMODULARIZE|-sENVIRONMENT)")
  message(FATAL_ERROR
    "Platform or library linker policy escaped the CMake adapters")
endif()

file(READ "${portable_root}/runtime/main.cpp" main_source)
if(main_source MATCHES "class[ \t\r\n]+Runtime")
  message(FATAL_ERROR
    "runtime/main.cpp must remain a thin startup entry point")
endif()
