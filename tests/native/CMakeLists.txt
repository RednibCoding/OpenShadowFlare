add_executable(
  portable_libraries_test
  portable_libraries_test.c
)

target_link_libraries(
  portable_libraries_test
  PRIVATE
    Lal::Lal
    Lgl::Lgl
)
target_include_directories(
  portable_libraries_test
  PRIVATE
    "${PROJECT_SOURCE_DIR}/thirdparty/lwl"
)

add_test(NAME portable_libraries COMMAND portable_libraries_test)
