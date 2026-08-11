cmake_minimum_required(VERSION 3.16)
project(cpp_oj_vibecoding VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall -Wextra")

message(STATUS "=== C++ OJ (cpp-oj-vibecoding) 依赖探测 ===")

# ---- 编译器探测：本机 g++ ----
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message(FATAL_ERROR "需要 GNU g++，当前编译器: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
endif()
message(STATUS "[OK] g++: ${CMAKE_CXX_COMPILER_VERSION}")

# ---- 1. cpp-httplib（header-only，校验 httplib.h）----
find_path(CPP_HTTPLIB_INCLUDE_DIR httplib.h)
if(NOT CPP_HTTPLIB_INCLUDE_DIR)
  message(FATAL_ERROR "未找到 httplib.h（cpp-httplib）。请安装: sudo apt install cpp-httplib-dev")
endif()
message(STATUS "[OK] cpp-httplib: ${CPP_HTTPLIB_INCLUDE_DIR}/httplib.h")

# ---- 2. libsqlite3（用户 / SQLite）----
find_package(SQLite3)
if(NOT SQLite3_FOUND)
  message(WARNING "未找到 SQLite3，尝试手工查找 libsqlite3")
  find_path(SQLITE3_INCLUDE_DIR sqlite3.h)
  find_library(SQLITE3_LIBRARY NAMES sqlite3)
  if(SQLITE3_INCLUDE_DIR AND SQLITE3_LIBRARY)
    set(SQLite3_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIR})
    set(SQLite3_LIBRARIES ${SQLITE3_LIBRARY})
    set(SQLite3_FOUND TRUE)
  endif()
endif()
if(NOT SQLite3_FOUND)
  message(FATAL_ERROR "未找到 SQLite3。请安装: sudo apt install libsqlite3-dev")
endif()
message(STATUS "[OK] sqlite3: ${SQLite3_VERSION} @ ${SQLite3_LIBRARIES}")

# ---- 3. libmysqlclient（题目 / MySQL）----
# 优先用 mysql_config 解析标记，退化为 find_path/find_library
find_program(MYSQL_CONFIG_EXECUTABLE mysql_config)
if(MYSQL_CONFIG_EXECUTABLE)
  execute_process(
    COMMAND ${MYSQL_CONFIG_EXECUTABLE} --include
    OUTPUT_VARIABLE MYSQL_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REPLACE " " ";" MYSQL_CFLAGS_LIST ${MYSQL_CFLAGS})
  foreach(_flag ${MYSQL_CFLAGS_LIST})
    if(IS_DIRECTORY "${_flag}")
      list(APPEND MYSQL_INCLUDE_DIRS_FROM_FLAG "${_flag}")
    elseif(_flag MATCHES "^-I(.*)")
      list(APPEND MYSQL_INCLUDE_DIRS_FROM_FLAG "${CMAKE_MATCH_1}")
    endif()
  endforeach()
  execute_process(
    COMMAND ${MYSQL_CONFIG_EXECUTABLE} --libs
    OUTPUT_VARIABLE MYSQL_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REPLACE " " ";" MYSQL_LIBS_LIST ${MYSQL_LIBS})
endif()

if(NOT MYSQL_INCLUDE_DIR)
  find_path(MYSQL_INCLUDE_DIR mysql.h PATHS ${MYSQL_INCLUDE_DIRS_FROM_FLAG} /usr/include/mysql)
endif()
if(NOT MYSQL_LIBRARIES)
  find_library(MYSQL_LIBRARY NAMES mysqlclient PATHS /usr/lib /usr/lib/x86_64-linux-gnu)
  if(MYSQL_LIBRARY)
    set(MYSQL_LIBRARIES ${MYSQL_LIBRARY})
  endif()
endif()
if(NOT MYSQL_INCLUDE_DIR OR NOT MYSQL_LIBRARIES)
  message(FATAL_ERROR "未找到 libmysqlclient / mysql.h。请安装: sudo apt install libmysqlclient-dev")
endif()
message(STATUS "[OK] mysqlclient: ${MYSQL_INCLUDE_DIR} ${MYSQL_LIBRARIES}")

# ---- 4. libcrypto（SHA-256 密码哈希）----
find_package(OpenSSL REQUIRED COMPONENTS Crypto)
if(NOT OPENSSL_CRYPTO_LIBRARY)
  message(FATAL_ERROR "未找到 libcrypto。请安装: sudo apt install libssl-dev")
endif()
message(STATUS "[OK] libcrypto: ${OPENSSL_CRYPTO_LIBRARY}")

message(STATUS "=== 依赖探测完成，进入子目录构建 ===")

enable_testing()

add_subdirectory(server)
add_subdirectory(tests)