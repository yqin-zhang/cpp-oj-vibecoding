#include "db/mysql_db.hpp"

#include <cstdio>

namespace oj {
namespace db {

MySqlDb::~MySqlDb() { close(); }

bool MySqlDb::exec(const std::string& sql) {
  if (mysql_query(conn_, sql.c_str()) != 0) {
    last_error_ = mysql_error(conn_);
    return false;
  }
  return true;
}

bool MySqlDb::connect(const DbConfig& cfg) {
  if (conn_) close();
  conn_ = mysql_init(nullptr);
  if (!conn_) {
    last_error_ = "mysql_init failed";
    return false;
  }
  // host=localhost 且提供 socket 时走 unix socket（auth_socket 免密依赖此路径）
  const char* host = cfg.mysql_host.c_str();
  const char* sock = cfg.mysql_unix_socket.empty() ? nullptr : cfg.mysql_unix_socket.c_str();
  if (!mysql_real_connect(conn_, host, cfg.mysql_user.c_str(), cfg.mysql_password.c_str(),
                          nullptr, cfg.mysql_port, sock, 0)) {
    last_error_ = mysql_error(conn_);
    mysql_close(conn_);
    conn_ = nullptr;
    return false;
  }
  mysql_set_character_set(conn_, "utf8mb4");
  return true;
}

void MySqlDb::close() {
  if (conn_) {
    mysql_close(conn_);
    conn_ = nullptr;
  }
}

bool MySqlDb::ensureDatabaseAndSchema() {
  if (!exec("CREATE DATABASE IF NOT EXISTS oj_problems "
            "DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci")) {
    return false;
  }
  if (!exec("USE oj_problems")) return false;

  const char* problems_sql =
      "CREATE TABLE IF NOT EXISTS problems ("
      "  id              BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,"
      "  title           VARCHAR(255) NOT NULL,"
      "  difficulty      ENUM('easy','medium','hard') NOT NULL DEFAULT 'easy',"
      "  content_html    MEDIUMTEXT   NOT NULL,"
      "  time_limit_ms   INT UNSIGNED NOT NULL DEFAULT 2000,"
      "  memory_limit_mb INT UNSIGNED NOT NULL DEFAULT 256,"
      "  created_at      TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
      "  updated_at      TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP"
      "                    ON UPDATE CURRENT_TIMESTAMP,"
      "  KEY idx_difficulty (difficulty)"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
  if (!exec(problems_sql)) return false;

  const char* cases_sql =
      "CREATE TABLE IF NOT EXISTS test_cases ("
      "  id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,"
      "  problem_id   BIGINT UNSIGNED NOT NULL,"
      "  input_text   MEDIUMTEXT NOT NULL,"
      "  output_text  MEDIUMTEXT NOT NULL,"
      "  is_sample    TINYINT(1) NOT NULL DEFAULT 0,"
      "  sort_order   INT NOT NULL DEFAULT 0,"
      "  created_at   TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
      "  CONSTRAINT fk_tc_problem FOREIGN KEY (problem_id)"
      "    REFERENCES problems(id) ON DELETE CASCADE,"
      "  KEY idx_problem (problem_id, sort_order)"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
  return exec(cases_sql);
}

// ---------- 题目 DAO ----------

bool MySqlDb::createProblem(const Problem& p, long long* out_id) {
  std::string sql =
      "INSERT INTO problems (title, difficulty, content_html, time_limit_ms, memory_limit_mb) "
      "VALUES ('" + p.title + "','" + p.difficulty + "','" + p.content_html + "'," +
      std::to_string(p.time_limit_ms) + "," + std::to_string(p.memory_limit_mb) + ")";
  if (!exec(sql)) return false;
  if (out_id) *out_id = static_cast<long long>(mysql_insert_id(conn_));
  return true;
}

std::optional<Problem> MySqlDb::getProblem(long long id) {
  std::string sql =
      "SELECT id, title, difficulty, content_html, time_limit_ms, memory_limit_mb "
      "FROM problems WHERE id = " + std::to_string(id);
  if (mysql_query(conn_, sql.c_str()) != 0) {
    last_error_ = mysql_error(conn_);
    return std::nullopt;
  }
  MYSQL_RES* res = mysql_store_result(conn_);
  if (!res) return std::nullopt;
  std::optional<Problem> out;
  if (MYSQL_ROW row = mysql_fetch_row(res)) {
    out = Problem{};
    out->id = id;
    out->title = row[1] ? row[1] : "";
    out->difficulty = row[2] ? row[2] : "easy";
    out->content_html = row[3] ? row[3] : "";
    out->time_limit_ms = row[4] ? std::stoll(row[4]) : 2000;
    out->memory_limit_mb = row[5] ? std::stoll(row[5]) : 256;
  }
  mysql_free_result(res);
  return out;
}

std::vector<Problem> MySqlDb::listProblems() {
  std::vector<Problem> out;
  const char* sql =
      "SELECT id, title, difficulty, content_html, time_limit_ms, memory_limit_mb "
      "FROM problems ORDER BY id";
  if (mysql_query(conn_, sql) != 0) {
    last_error_ = mysql_error(conn_);
    return out;
  }
  MYSQL_RES* res = mysql_store_result(conn_);
  if (!res) return out;
  while (MYSQL_ROW row = mysql_fetch_row(res)) {
    Problem p;
    p.id = row[0] ? std::stoll(row[0]) : 0;
    p.title = row[1] ? row[1] : "";
    p.difficulty = row[2] ? row[2] : "easy";
    p.content_html = row[3] ? row[3] : "";
    p.time_limit_ms = row[4] ? std::stoll(row[4]) : 2000;
    p.memory_limit_mb = row[5] ? std::stoll(row[5]) : 256;
    out.push_back(std::move(p));
  }
  mysql_free_result(res);
  return out;
}

bool MySqlDb::updateProblem(const Problem& p) {
  std::string sql =
      "UPDATE problems SET title='" + p.title + "', difficulty='" + p.difficulty +
      "', content_html='" + p.content_html + "', time_limit_ms=" +
      std::to_string(p.time_limit_ms) + ", memory_limit_mb=" +
      std::to_string(p.memory_limit_mb) + " WHERE id=" + std::to_string(p.id);
  return exec(sql);
}

bool MySqlDb::deleteProblem(long long id) {
  return exec("DELETE FROM problems WHERE id=" + std::to_string(id));
}

// ---------- 用例 DAO ----------

bool MySqlDb::addTestCase(const TestCase& tc) {
  std::string sql =
      "INSERT INTO test_cases (problem_id, input_text, output_text, is_sample, sort_order) "
      "VALUES (" + std::to_string(tc.problem_id) + ",'" + tc.input_text + "','" +
      tc.output_text + "'," + std::to_string(tc.is_sample ? 1 : 0) + "," +
      std::to_string(tc.sort_order) + ")";
  return exec(sql);
}

std::vector<TestCase> MySqlDb::listCases(long long problem_id, bool only_samples) {
  std::vector<TestCase> out;
  std::string sql =
      "SELECT id, problem_id, input_text, output_text, is_sample, sort_order "
      "FROM test_cases WHERE problem_id = " + std::to_string(problem_id);
  if (only_samples) sql += " AND is_sample = 1";
  sql += " ORDER BY sort_order, id";
  if (mysql_query(conn_, sql.c_str()) != 0) {
    last_error_ = mysql_error(conn_);
    return out;
  }
  MYSQL_RES* res = mysql_store_result(conn_);
  if (!res) return out;
  while (MYSQL_ROW row = mysql_fetch_row(res)) {
    TestCase tc;
    tc.id = row[0] ? std::stoll(row[0]) : 0;
    tc.problem_id = row[1] ? std::stoll(row[1]) : 0;
    tc.input_text = row[2] ? row[2] : "";
    tc.output_text = row[3] ? row[3] : "";
    tc.is_sample = row[4] && (std::stoi(row[4]) == 1);
    tc.sort_order = row[5] ? std::stoi(row[5]) : 0;
    out.push_back(std::move(tc));
  }
  mysql_free_result(res);
  return out;
}

bool MySqlDb::deleteTestCasesByProblem(long long problem_id) {
  return exec("DELETE FROM test_cases WHERE problem_id=" + std::to_string(problem_id));
}

}  // namespace db
}  // namespace oj