#pragma once

// 认证业务：注册 / 登录 / 注销 / 会话，仅依赖 SqliteDb + SessionStore，
// 不依赖 httplib，便于单元测试。角色模型见 SPEC §2.1 / §5.1。

#include <optional>
#include <string>

#include "auth/password.hpp"
#include "auth/session.hpp"
#include "db/db_types.hpp"

namespace oj {
namespace auth {

// 角色常量。
inline constexpr const char* kRoleUser = "user";
inline constexpr const char* kRoleAdmin = "admin";

// 认证失败原因（对应 HTTP 状态码语义）。
enum class AuthError {
  kOk = 0,
  kInvalidUsername,  // 400：用户名不符合规则
  kInvalidPassword,  // 400：密码不符合规则
  kUsernameTaken,    // 400：用户名已存在
  kBadCredentials,   // 401/400：登录凭据错误
  kDatabaseError,    // 500
};

inline bool isAdmin(const std::string& role) { return role == kRoleAdmin; }

// 认证服务：注册成功后自动登录并派发会话令牌。
class AuthService {
 public:
  AuthService(db::SqliteDb* db, SessionStore* sessions)
      : db_(db), sessions_(sessions) {}

  // 注册：校验 → 写入用户（SHA-256+salt）→ 建会话。成功返回用户 + 会话令牌。
  std::optional<db::User> registerUser(const std::string& username,
                                       const std::string& password,
                                       std::string* session_token,
                                       AuthError* err);

  // 登录：校验凭据，成功建立会话。
  std::optional<db::User> login(const std::string& username,
                                const std::string& password,
                                std::string* session_token, AuthError* err);

  // 确保 admin 账号存在（首次启动种子，见 SPEC §4.7）。
  // 若库中已无任何用户，以给定口令创建 admin；已有用户则跳过。
  bool seedAdminIfNeeded(const std::string& password, AuthError* err);

 private:
  db::SqliteDb* db_;
  SessionStore* sessions_;
};

}  // namespace auth
}  // namespace oj