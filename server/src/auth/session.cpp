#include "auth/session.hpp"

#include <algorithm>

#include "auth/password.hpp"

namespace oj {
namespace auth {

namespace {
constexpr std::size_t kTokenBytes = 32;  // 64 hex chars
}

std::string SessionStore::create(long long user_id, const std::string& username,
                                 const std::string& role) {
  const std::string token = randomHex(kTokenBytes);
  if (token.empty()) return "";

  Session s;
  s.token = token;
  s.user_id = user_id;
  s.username = username;
  s.role = role;
  const auto now = std::chrono::steady_clock::now();
  s.created_at = now;
  s.expires_at = now + kSessionTtl;

  {
    std::lock_guard<std::mutex> lock(mu_);
    sessions_[token] = std::move(s);
  }
  return token;
}

std::optional<Session> SessionStore::get(const std::string& token) {
  if (token.empty()) return std::nullopt;
  std::lock_guard<std::mutex> lock(mu_);
  auto it = sessions_.find(token);
  if (it == sessions_.end()) return std::nullopt;
  if (it->second.expired()) {
    sessions_.erase(it);
    return std::nullopt;
  }
  return it->second;
}

bool SessionStore::destroy(const std::string& token) {
  std::lock_guard<std::mutex> lock(mu_);
  return sessions_.erase(token) > 0;
}

std::size_t SessionStore::pruneExpired() {
  const auto now = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> lock(mu_);
  std::size_t removed = 0;
  for (auto it = sessions_.begin(); it != sessions_.end();) {
    if (it->second.expired()) {
      it = sessions_.erase(it);
      ++removed;
    } else {
      ++it;
    }
  }
  return removed;
}

}  // namespace auth
}  // namespace oj