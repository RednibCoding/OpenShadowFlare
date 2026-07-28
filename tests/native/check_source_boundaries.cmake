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
  RKC_RPGSCRN/rkc_rpgscrn.hpp
  RKC_UPDIB/rkc_updib.hpp
)

foreach(public_api IN LISTS implemented_public_apis)
  if(NOT EXISTS "${portable_libraries_root}/${public_api}")
    message(FATAL_ERROR
      "Portable DLL API must remain under src/SF_EXE/libs: ${public_api}")
  endif()
endforeach()

set(dll_implementation_names
  bitmap.cpp
  caf.cpp
  coordinates.cpp
  ground_map.cpp
  judgement.cpp
  njp.cpp
  object_map.cpp
  rclib_lz.cpp
  software_backend.cpp
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
