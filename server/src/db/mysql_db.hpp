#pragma once

#include <mysql.h>

#include <optional>
#include <string>
#include <vector>

#include "db/db_types.hpp"

namespace oj {
namespace db {

// MySQL 封装：题目 / 用例（oj_problems），见 SPEC §4.7。
// 本机认证走 unix socket + auth_socket（zyq 免密），故 connect 接受 socket 路径。
class MySqlDb {
 public:
  MySqlDb() = default;
  ~MySqlDb();

  MySqlDb(const MySqlDb&) = delete;
  MySqlDb& operator=(const MySqlDb&) = delete;

  bool connect(const DbConfig& cfg);
  void close();
  bool isConnected() const { return conn_ != nullptr; }
  std::string error() const { return last_error_; }

  bool ensureDatabaseAndSchema();  // 建库 + 建表（幂等）

  // 题目 DAO
  bool createProblem(const Problem& p, long long* out_id = nullptr);
  std::optional<Problem> getProblem(long long id);
  std::vector<Problem> listProblems();
  bool updateProblem(const Problem& p);
  bool deleteProblem(long long id);

  // 用例 DAO
  bool addTestCase(const TestCase& tc);
  std::vector<TestCase> listCases(long long problem_id, bool only_samples);
  bool deleteTestCasesByProblem(long long problem_id);

 private:
  MYSQL* conn_ = nullptr;
  std::string last_error_;
  bool exec(const std::string& sql);
};

}  // namespace db
}  // namespace oj