# judge：判题核心库（纯函数输出比对，可独立链接测试）
add_library(judge
  src/judge/output_compare.cpp
)
target_include_directories(judge PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 后续服务端可执行（main.cpp / db / auth）在此扩展
# add_executable(oj_server src/main.cpp)
# target_link_libraries(oj_server PRIVATE judge)