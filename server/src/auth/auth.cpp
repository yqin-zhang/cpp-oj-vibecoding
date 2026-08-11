#include "auth/auth.hpp"

#include <utility>

namespace oj {
namespace auth {

namespace {

// 建用户 → 建会话，返回用户；失败置 err 返回 nullopt。
std::optional<db::User> finalizeUser(db::SqliteDb* db, SessionStore* sessions,
                                     db::User u, std::string* session_token,
                                     AuthError* err) {
  if (!db->createUser(u)) {
    *err = AuthError::kDatabaseError;
    return std::nullopt;
  }
  const std::string token = sessions->create(u.id, u.username, u.role);
  if (token.empty()) {
    *err = AuthError::kDatabaseError;
    return std::nullopt;
  }
  if (session_token) *session_token = token;
  *err = AuthError::kOk;
  return u;
}

}  // namespace

std::optional<db::User> AuthService::registerUser(
    const std::string& username, const std::string& password,
    std::string* session_token, AuthError* err) {
  if (!validUsername(username)) {
    *err = AuthError::kInvalidUsername;
    return std::nullopt;
  }
  if (!validPassword(password)) {
    *err = AuthError::kInvalidPassword;
    return std::nullopt;
  }
  if (db_->findByUsername(username)) {
    *err = AuthError::kUsernameTaken;
    return std::nullopt;
  }

  db::User u;
  u.username = username;
  u.role = kRoleUser;
  u.salt = randomHex(16);  // 128-bit 随机盐
  if (u.salt.empty()) {
    *err = AuthError::kDatabaseError;
    return std::nullopt;
  }
  u.password_hash = hashPassword(password, u.salt);
  return finalizeUser(db_, sessions_, std::move(u), session_token, err);
}

std::optional<db::User> AuthService::login(const std::string& username,
                                           const std::string& password,
                                           std::string* session_token,
                                           AuthError* err) {
  std::optional<db::User> found = db_->findByUsername(username);
  if (!found || !verifyPassword(password, found->salt, found->password_hash)) {
    *err = AuthError::kBadCredentials;
    return std::nullopt;
  }
  const std::string token = sessions_->create(found->id, found->username,
                                              found->role);
  if (token.empty()) {
    *err = AuthError::kDatabaseError;
    return std::nullopt;
  }
  if (session_token) *session_token = token;
  *err = AuthError::kOk;
  return found;
}

bool AuthService::seedAdminIfNeeded(const std::string& password,
                                    AuthError* err) {
  if (db_->findByUsername("admin")) {
    *err = AuthError::kOk;
    return true;
  }

  db::User u;
  u.username = "admin";
  u.role = kRoleAdmin;
  u.salt = randomHex(16);
  if (u.salt.empty()) {
    *err = AuthError::kDatabaseError;
    return false;
  }
  u.password_hash = hashPassword(password, u.salt);

  AuthError ignored = AuthError::kOk;
  if (!db_->createUser(u)) {
    *err = AuthError::kDatabaseError;
    return false;
  }
  *err = AuthError::kOk;
  return true;
}

}  // namespace auth
}  // namespace oj