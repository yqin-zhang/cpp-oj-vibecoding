#!/usr/bin/env bash
# 一键初始化 MySQL：建库 + 建表 + 种子题目。
# admin 账号由服务端首次启动时写入 SQLite（盐随机生成）。
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

MYSQL_ARGS=()
if [[ -n "${MYSQL_USER:-}" ]]; then MYSQL_ARGS+=( -u "$MYSQL_USER" ); fi
if [[ -n "${MYSQL_PASS:-}" ]]; then MYSQL_ARGS+=( -p"$MYSQL_PASS" ); fi
if [[ -n "${MYSQL_HOST:-}" ]]; then MYSQL_ARGS+=( -h "$MYSQL_HOST" ); fi
if [[ -n "${MYSQL_PORT:-}" ]]; then MYSQL_ARGS+=( -P "$MYSQL_PORT" ); fi

echo "==> 执行 MySQL 建表 + 种子数据：${SCRIPT_DIR}/seed.sql"
mysql "${MYSQL_ARGS[@]}" < "${SCRIPT_DIR}/seed.sql"
echo "==> 完成：库 oj_problems，题目 3 道 + 标准/示例用例已灌入。"