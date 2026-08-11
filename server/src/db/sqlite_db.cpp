#include "db/sqlite_db.hpp"

#include <cstring>

namespace oj {
namespace db {

SqliteDb::~SqliteDb() { close(); }

bool SqliteDb::open(const std::string& path) {
  if (db_) close();
  int rc = sqlite3_open(path.c_str(), &db_);
  if (rc != SQLITE_OK) {
    sqlite3_close(db_);
    db_ = nullptr;
    return false;
  }
  return initSchema();
}

void SqliteDb::close() {
  if (db_) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
}

bool SqliteDb::initSchema() {
  const char* sql =
      "CREATE TABLE IF NOT EXISTS users ("
      "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  username      TEXT NOT NULL UNIQUE,"
      "  password_hash TEXT NOT NULL,"
      "  salt          TEXT NOT NULL,"
      "  role          TEXT NOT NULL DEFAULT 'user',"
      "  created_at    TEXT NOT NULL DEFAULT (datetime('now'))"
      ");";
  char* err = nullptr;
  if (sqlite3_exec(db_, sql, nullptr, nullptr, &err) != SQLITE_OK) {
    if (err) sqlite3_free(err);
    return false;
  }
  return true;
}

// ---------- 用户 DAO ----------

bool SqliteDb::createUser(const User& u) {
  const char* sql =
      "INSERT INTO users (username, password_hash, salt, role) VALUES (?, ?, ?, ?);";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
  sqlite3_bind_text(stmt, 1, u.username.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, u.password_hash.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, u.salt.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 4, u.role.c_str(), -1, SQLITE_TRANSIENT);
  bool ok = sqlite3_step(stmt) == SQLITE_DONE;
  sqlite3_finalize(stmt);
  return ok;
}

std::optional<User> SqliteDb::findByUsername(const std::string& username) {
  const char* sql =
      "SELECT id, username, password_hash, salt, role FROM users WHERE username = ?;";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;
  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
  std::optional<User> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    User u;
    u.id = sqlite3_column_int64(stmt, 0);
    u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    u.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    u.salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    u.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    out = std::move(u);
  }
  sqlite3_finalize(stmt);
  return out;
}

std::vector<User> SqliteDb::listUsers() {
  const char* sql =
      "SELECT id, username, password_hash, salt, role FROM users ORDER BY id;";
  sqlite3_stmt* stmt = nullptr;
  std::vector<User> users;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return users;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    User u;
    u.id = sqlite3_column_int64(stmt, 0);
    u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    u.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    u.salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    u.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    users.push_back(std::move(u));
  }
  sqlite3_finalize(stmt);
  return users;
}

bool SqliteDb::deleteUser(long long id) {
  const char* sql = "DELETE FROM users WHERE id = ?;";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
  sqlite3_bind_int64(stmt, 1, id);
  bool ok = sqlite3_step(stmt) == SQLITE_DONE;
  sqlite3_finalize(stmt);
  return ok;
}

bool SqliteDb::updatePassword(long long id, const std::string& hash,
                              const std::string& salt) {
  const char* sql = "UPDATE users SET password_hash = ?, salt = ? WHERE id = ?;";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
  sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, salt.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 3, id);
  bool ok = sqlite3_step(stmt) == SQLITE_DONE;
  sqlite3_finalize(stmt);
  return ok;
}

long long SqliteDb::countUsers() {
  const char* sql = "SELECT COUNT(*) FROM users;";
  sqlite3_stmt* stmt = nullptr;
  long long n = 0;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
  }
  return n;
}

}  // namespace db
}  // namespace oj