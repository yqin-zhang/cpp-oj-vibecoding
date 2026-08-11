# ---- db 库：SQLite + MySQL 访问封装（DAO）----
add_library(db STATIC
  src/db/sqlite_db.cpp
  src/db/mysql_db.cpp
)

target_include_directories(db
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  PRIVATE
    ${SQLite3_INCLUDE_DIRS}
    ${MYSQL_INCLUDE_DIR}
)

target_link_libraries(db PUBLIC ${SQLite3_LIBRARIES} ${MYSQL_LIBRARIES})
target_compile_options(db PRIVATE -Wall -Wextra)

# ---- auth 库：密码哈希 + 会话 + 角色（libcrypto）----
add_library(auth STATIC
  src/auth/password.cpp
  src/auth/session.cpp
  src/auth/auth.cpp
)

target_include_directories(auth PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(auth PUBLIC db OpenSSL::Crypto)
target_compile_options(auth PRIVATE -Wall -Wextra)

# ---- server 可执行：HTTP 服务入口 ----
add_executable(server
  src/main.cpp
)

target_include_directories(server
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CPP_HTTPLIB_INCLUDE_DIR}
)

target_link_libraries(server PRIVATE auth db OpenSSL::Crypto)
find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)
target_compile_options(server PRIVATE -Wall -Wextra)

# ---- judge 判题核心库（可独立链接测试）----
add_library(judge STATIC
  src/judge/output_compare.cpp
)

target_include_directories(judge
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(judge PRIVATE -Wall -Wextra)