#pragma once

// 内存 Session 存储 + HttpOnly Cookie 令牌（SPEC §2.1 / §4.2）。
// 会话驻内存，进程重启即清空，符合「提交/排行等会话内数据不落库」约定。

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace oj {
namespace auth {

struct Session {
  std::string token;
  long long user_id = 0;
  std::string username;
  std::string role;
  std::chrono::steady_clock::time_point expires_at;
  std::chrono::steady_clock::time_point created_at;

  bool expired() const {
    return std::chrono::steady_clock::now() >= expires_at;
  }
};

// Cookie 名（HttpOnly，通过 Set-Cookie 下发）。
inline constexpr const char* kSessionCookie = "oj_session";

class SessionStore {
 public:
  static constexpr std::chrono::hours kSessionTtl = std::chrono::hours(24);

  // 创建会话，返回随机令牌（hex）。
  std::string create(long long user_id, const std::string& username,
                     const std::string& role);
  // 取回会话；不存在或已过期返回 nullopt（过期则清理）。
  std::optional<Session> get(const std::string& token);
  // 注销会话。
  bool destroy(const std::string& token);
  // 清理过期会话，返回清除数量。
  std::size_t pruneExpired();

 private:
  std::mutex mu_;
  std::unordered_map<std::string, Session> sessions_;
};

}  // namespace auth
}  // namespace oj