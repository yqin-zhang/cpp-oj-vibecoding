#pragma once

#include <string>
#include <vector>

namespace oj {
namespace db {

struct DbConfig {
    std::string mysql_user = "zyq";
    std::string mysql_password = "";
    std::string mysql_host = "localhost";
    unsigned mysql_port = 3306;
    std::string mysql_unix_socket = "/var/run/mysqld/mysqld.sock";
    std::string mysql_database = "oj_problems";
    std::string sqlite_path = "oj.db";
};

struct User {
    long long id = 0;
    std::string username;
    std::string password_hash;
    std::string salt;
    std::string role = "user";
};

struct Problem {
    long long id = 0;
    std::string title;
    std::string difficulty = "easy";
    std::string content_html;
    long long time_limit_ms = 2000;
    long long memory_limit_mb = 256;
};

struct TestCase {
    long long id = 0;
    long long problem_id = 0;
    std::string input_text;
    std::string output_text;
    bool is_sample = false;
    int sort_order = 0;
};

}  // namespace db
}  // namespace oj