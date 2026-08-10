-- ============================================================
-- C++ OJ 种子数据：MySQL（题目 / 标准用例 / 示例用例）
-- 与 SPEC §4.7 建表 SQL 保持一致；admin 账号由服务端首次启动写入 SQLite。
-- 用法：mysql -u <user> -p < scripts/seed.sql
-- ============================================================

CREATE DATABASE IF NOT EXISTS oj_problems
  DEFAULT CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;
USE oj_problems;

CREATE TABLE IF NOT EXISTS problems (
  id              BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  title           VARCHAR(255) NOT NULL,
  difficulty      ENUM('easy','medium','hard') NOT NULL DEFAULT 'easy',
  content_html    MEDIUMTEXT   NOT NULL,
  time_limit_ms   INT UNSIGNED NOT NULL DEFAULT 2000,
  memory_limit_mb INT UNSIGNED NOT NULL DEFAULT 256,
  created_at      TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at      TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
                    ON UPDATE CURRENT_TIMESTAMP,
  KEY idx_difficulty (difficulty)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS test_cases (
  id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  problem_id   BIGINT UNSIGNED NOT NULL,
  input_text   MEDIUMTEXT NOT NULL,
  output_text  MEDIUMTEXT NOT NULL,
  is_sample    TINYINT(1) NOT NULL DEFAULT 0,
  sort_order   INT NOT NULL DEFAULT 0,
  created_at   TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT fk_tc_problem FOREIGN KEY (problem_id)
    REFERENCES problems(id) ON DELETE CASCADE,
  KEY idx_problem (problem_id, sort_order)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ------------------------------------------------------------
-- 种子题目（3 道，均可 AC）
-- ------------------------------------------------------------

INSERT INTO problems (id, title, difficulty, content_html, time_limit_ms, memory_limit_mb) VALUES
(1, 'A + B', 'easy',
 '<h3>题目描述</h3><p>给定两个整数 a 和 b，输出它们的和 a + b。</p>'
 '<h3>输入格式</h3><p>一行两个整数 a b，以空格分隔。</p>'
 '<h3>输出格式</h3><p>输出一个整数，表示 a + b 的结果。</p>',
 1000, 256),
(2, '逆序输出', 'easy',
 '<h3>题目描述</h3><p>给定 n 个整数，按输入的顺序逆序输出。</p>'
 '<h3>输入格式</h3><p>第一行为整数 n；第二行为 n 个整数，以空格分隔。</p>'
 '<h3>输出格式</h3><p>逆序输出 n 个整数，每个占一行。</p>',
 1000, 256),
(3, '斐波那契数列', 'medium',
 '<h3>题目描述</h3><p>定义 F(0)=0，F(1)=1，F(n)=F(n-1)+F(n-2)。'
 '给定非负整数 n，输出 F(n)。</p>'
 '<h3>输入格式</h3><p>单行一个非负整数 n（0 ≤ n ≤ 20）。</p>'
 '<h3>输出格式</h3><p>输出 F(n)。</p>',
 1000, 256);

-- ------------------------------------------------------------
-- 用例：is_sample=1 展示给用户，is_sample=0 为隐藏标准用例
-- ------------------------------------------------------------

INSERT INTO test_cases (problem_id, input_text, output_text, is_sample, sort_order) VALUES
-- 1. A + B
(1, '1 2',      '3',          1, 0),
(1, '3 5',      '8',          0, 1),
(1, '-1 1',     '0',          0, 2),
(1, '1000000000 1000000000', '2000000000', 0, 3),
(1, '',         '0',          0, 4),
-- 2. 逆序输出
(2, '3
1 2 3', '3
2
1
', 1, 0),
(2, '1
42', '42
', 0, 1),
(2, '5
5 4 3 2 1', '1
2
3
4
5
', 0, 2),
(2, '0', '', 0, 3),
-- 3. 斐波那契数列
(3, '6',  '8',    1, 0),
(3, '0',  '0',    0, 1),
(3, '1',  '1',    0, 2),
(3, '20', '6765', 0, 3);