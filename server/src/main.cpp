#include "httplib.h"

#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>

#include "auth/auth.hpp"
#include "auth/password.hpp"
#include "auth/session.hpp"
#include "db/db_types.hpp"
#include "db/mysql_db.hpp"
#include "db/sqlite_db.hpp"
#include "util/json.hpp"

using oj::db::DbConfig;
using oj::db::SqliteDb;
using oj::db::MySqlDb;
using oj::auth::AuthService;
using oj::auth::AuthError;
using oj::auth::SessionStore;
using oj::auth::Session;
using oj::util::Json;

namespace {

constexpr int kHttpOk = 200;
constexpr int kHttpBadRequest = 400;
constexpr int kHttpUnauthorized = 401;
constexpr int kHttpForbidden = 403;
constexpr int kHttpServerError = 500;

// ---- JSON 响应辅助 ----

void sendJson(httplib::Response& res, int status, const Json& body) {
  res.status = status;
  res.set_content(body.dump(), "application/json; charset=utf-8");
}

void sendError(httplib::Response& res, int status, const char* message) {
  Json body = Json::object();
  body["error"] = message;
  body["code"] = status;
  sendJson(res, status, body);
}

// 解析请求体为 JSON 对象；失败 / 缺字段返回 false 并写 400。
bool parseBodyObject(const httplib::Request& req, httplib::Response& res,
                     Json& out) {
  bool ok = false;
  Json j = Json::parse(req.body, &ok);
  if (!ok || j.type() != Json::Type::Object) {
    sendError(res, kHttpBadRequest, "请求体必须是 JSON 对象");
    return false;
  }
  out = std::move(j);
  return true;
}

// 读取字段为字符串，缺失返回空串。
std::string field(const Json& obj, const char* key) {
  const Json& v = obj.get(key);
  return v.type() == Json::Type::String ? v.asString() : std::string();
}

// ---- Cookie 辅助 ----

const char* kCookiePath = "/";

// 从 Set-Cookie 头构造 HttpOnly 会话 Cookie。
std::string sessionCookieHeader(const std::string& token, bool expired) {
  std::string h = std::string(oj::auth::kSessionCookie) + "=";
  if (expired) {
    // 注销：清 cookie（Max-Age=0，立即过期）
    return h + "; Max-Age=0; Path=" + kCookiePath + "; HttpOnly; SameSite=Lax";
  }
  return h + token + "; Path=" + kCookiePath + "; HttpOnly; SameSite=Lax";
}

// 从请求 Cookie 头解析会话令牌。失败返回空串。
std::string extractCookieToken(const httplib::Request& req) {
  const std::string cookie = req.get_header_value("Cookie");
  const std::string needle = std::string(oj::auth::kSessionCookie) + "=";
  std::size_t pos = cookie.find(needle);
  if (pos == std::string::npos) return "";
  std::size_t start = pos + needle.size();
  std::size_t end = cookie.find(';', start);
  if (end == std::string::npos) end = cookie.size();
  return cookie.substr(start, end - start);
}

// 认证中间件：未登录写 401 并返回 nullopt。
std::optional<Session> requireLogin(const httplib::Request& req,
                                    httplib::Response& res,
                                    SessionStore& sessions) {
  const std::string token = extractCookieToken(req);
  std::optional<Session> s = sessions.get(token);
  if (!s) {
    sendError(res, kHttpUnauthorized, "未登录或会话已过期");
    return std::nullopt;
  }
  return s;
}

// 角色中间件：非 admin 写 403 并返回 false。
bool requireAdmin(const std::optional<Session>& s, httplib::Response& res) {
  if (!s || !oj::auth::isAdmin(s->role)) {
    sendError(res, kHttpForbidden, "无管理员权限");
    return false;
  }
  return true;
}

void userJson(const oj::db::User& u, Json& out) {
  out["id"] = u.id;
  out["username"] = u.username;
  out["role"] = u.role;
}

}  // namespace

int main(int argc, char** argv) {
  DbConfig cfg;
  int port = 8080;
  std::string admin_password = "admin123";
  std::string web_root;

  for (int i = 1; i < argc; i += 2) {
    std::string key = argv[i];
    std::string val = (i + 1 < argc) ? argv[i + 1] : "";
    if (key == "--port") {
      port = std::atoi(val.c_str());
      if (port <= 0) port = 8080;
    } else if (key == "--sqlite") {
      cfg.sqlite_path = val;
    } else if (key == "--admin-password") {
      admin_password = val;
    } else if (key == "--web") {
      web_root = val;
    }
  }

  std::printf("[init] sqlite=%s, port=%d\n", cfg.sqlite_path.c_str(), port);

  SqliteDb sqlite;
  if (!sqlite.open(cfg.sqlite_path)) {
    std::fprintf(stderr, "[init] 打开 SQLite 失败: %s\n",
                 cfg.sqlite_path.c_str());
    return 1;
  }

  // MySQL 题目库此时仅探测连通性，M2 起承载题目/用例。
  MySqlDb mysql;
  if (mysql.connect(cfg)) {
    if (!mysql.ensureDatabaseAndSchema()) {
      std::fprintf(stderr, "[warn] MySQL schema 初始化失败: %s\n",
                   mysql.error().c_str());
    } else {
      std::printf("[init] MySQL ok (%s)\n", cfg.mysql_database.c_str());
    }
  } else {
    std::fprintf(stderr, "[warn] MySQL 未连接: %s（M2 题目库前可忽略）\n",
                 mysql.error().c_str());
  }

  SessionStore sessions;
  AuthService auth(&sqlite, &sessions);

  AuthError seed_err = AuthError::kOk;
  if (!auth.seedAdminIfNeeded(admin_password, &seed_err)) {
    std::fprintf(stderr, "[init] 创建 admin 失败\n");
    return 1;
  }
  std::printf("[init] admin 账号已就绪（默认口令 %s，请在登录后台后重置）\n",
              admin_password.c_str());

  httplib::Server svr;

  // ---- 静态资源（web/）----
  if (!web_root.empty() && !svr.set_mount_point("/", web_root)) {
    std::fprintf(stderr, "[warn] 静态目录不可用: %s\n", web_root.c_str());
  }

  // ---- 认证路由 ----

  svr.Post("/api/register", [&](const httplib::Request& req,
                                httplib::Response& res) {
    Json body;
    if (!parseBodyObject(req, res, body)) return;

    const std::string username = field(body, "username");
    const std::string password = field(body, "password");

    std::string token;
    AuthError err = AuthError::kOk;
    std::optional<oj::db::User> u = auth.registerUser(username, password,
                                                      &token, &err);
    if (!u) {
      switch (err) {
        case AuthError::kInvalidUsername:
          sendError(res, kHttpBadRequest, "用户名需 3~20 位字母/数字/下划线");
          break;
        case AuthError::kInvalidPassword:
          sendError(res, kHttpBadRequest,
                    "密码至少 8 位且同时包含字母与数字");
          break;
        case AuthError::kUsernameTaken:
          sendError(res, kHttpBadRequest, "用户名已存在");
          break;
        default:
          sendError(res, kHttpServerError, "内部错误");
          break;
      }
      return;
    }

    res.set_header("Set-Cookie", sessionCookieHeader(token, false));
    Json out = Json::object();
    userJson(*u, out);
    sendJson(res, kHttpOk, out);
  });

  svr.Post("/api/login", [&](const httplib::Request& req,
                             httplib::Response& res) {
    Json body;
    if (!parseBodyObject(req, res, body)) return;

    const std::string username = field(body, "username");
    const std::string password = field(body, "password");

    std::string token;
    AuthError err = AuthError::kOk;
    std::optional<oj::db::User> u = auth.login(username, password, &token,
                                               &err);
    if (!u) {
      sendError(res, kHttpUnauthorized, "用户名或密码错误");
      return;
    }

    res.set_header("Set-Cookie", sessionCookieHeader(token, false));
    Json out = Json::object();
    userJson(*u, out);
    sendJson(res, kHttpOk, out);
  });

  svr.Post("/api/logout", [&](const httplib::Request& req,
                              httplib::Response& res) {
    const std::string token = extractCookieToken(req);
    if (!token.empty()) sessions.destroy(token);
    res.set_header("Set-Cookie",
                   sessionCookieHeader("", true /* 清除 */));
    Json out = Json::object();
    out["ok"] = true;
    sendJson(res, kHttpOk, out);
  });

  svr.Get("/api/me", [&](const httplib::Request& req, httplib::Response& res) {
    std::optional<Session> s = requireLogin(req, res, sessions);
    if (!s) return;
    Json out = Json::object();
    out["id"] = s->user_id;
    out["username"] = s->username;
    out["role"] = s->role;
    sendJson(res, kHttpOk, out);
  });

  // ---- 角色中间件示范：admin 示例端 /api/ping-admin ----

  svr.Get("/api/admin/ping", [&](const httplib::Request& req,
                                 httplib::Response& res) {
    std::optional<Session> s = requireLogin(req, res, sessions);
    if (!s) return;
    if (!requireAdmin(s, res)) return;
    Json out = Json::object();
    out["ok"] = true;
    out["admin"] = true;
    sendJson(res, kHttpOk, out);
  });

  if (!svr.listen("0.0.0.0", port)) {
    std::fprintf(stderr, "[fatal] 监听 %d 失败\n", port);
    return 1;
  }
  return 0;
}