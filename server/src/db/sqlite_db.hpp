#pragma once

#include <sqlite3.h>

#include <optional>
#include <string>
#include <vector>

#include "db/db_types.hpp"

namespace oj {
namespace db {

// SQLite 封装：用户表（users），见 SPEC §4.7。
class SqliteDb {
 public:
  SqliteDb() = default;
  ~SqliteDb();

  SqliteDb(const SqliteDb&) = delete;
  SqliteDb& operator=(const SqliteDb&) = delete;

  // 打开库文件并建表（幂等）。
  bool open(const std::string& path);
  void close();
  bool isOpen() const { return db_ != nullptr; }

  // 用户 DAO
  bool createUser(const User& u);                      // 写入新用户
  std::optional<User> findByUsername(const std::string& username);
  std::vector<User> listUsers();                       // 全部用户（不含密码敏感项可选）
  bool deleteUser(long long id);
  bool updatePassword(long long id, const std::string& hash, const std::string& salt);
  long long countUsers();

 private:
  sqlite3* db_ = nullptr;
  bool initSchema();  // CREATE TABLE IF NOT EXISTS users
};

}  // namespace db
}  // namespace oj