# 单元测试：unit_tests，接入 ctest
add_executable(unit_tests
  unit/test_main.cpp
  unit/test_output_compare.cpp
)

target_link_libraries(unit_tests PRIVATE judge)

target_compile_options(unit_tests PRIVATE -Wall -Wextra)

find_package(Threads REQUIRED)
target_link_libraries(unit_tests PRIVATE Threads::Threads)

add_test(NAME unit_tests COMMAND unit_tests)