add_executable(unit_tests
  unit/test_main.cpp
  unit/test_output_compare.cpp
)
target_link_libraries(unit_tests PRIVATE judge)
target_include_directories(unit_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/unit
)

add_test(NAME unit_tests COMMAND unit_tests)