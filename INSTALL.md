# 依赖安装指南（Ubuntu 22.04）

> 假定当前系统为**空白的 Ubuntu 22.04**。本文档列出项目所需全部依赖及安装命令，对应 SPEC.md §1.3 / §2.1 / §7 M1。

## 依赖清单

| 依赖 | 用途 | 来源（SPEC.md） |
| --- | --- | --- |
| g++ / gcc / make（build-essential） | 判题编译运行 C/C++ | §1.3、§2.1 |
| cmake | 项目构建 | §2.1、§7 M1 |
| libsqlite3-dev | SQLite：用户表 | §1.3、§2.1 |
| libmysqlclient-dev | MySQL：题目/用例/题面 | §1.3、§2.1 |
| libssl-dev | SHA-256（libcrypto）+ HTTPS | §1.3、§2.1 |
| libcpp-httplib-dev | cpp-httplib HTTP 库（header-only） | §2.1 |
| mysql-server | 运行 MySQL 服务（建库/种子数据） | §4.7、scripts/seed.sh |

## 安装命令

### 1. 构建工具与编译器（g++/gcc，判题用）

```bash
sudo apt update
sudo apt install -y build-essential cmake
```

### 2. SQLite（用户表）

```bash
sudo apt install -y libsqlite3-dev
```

### 3. MySQL 客户端（题目表）

```bash
sudo apt install -y libmysqlclient-dev
```

### 4. libcrypto（SHA-256，来自 OpenSSL）

```bash
sudo apt install -y libssl-dev
```

### 5. cpp-httplib（header-only HTTP 库，系统包已提供）

```bash
sudo apt install -y libcpp-httplib-dev
```

### 6. MySQL 服务端（运行数据库，非编译依赖但运行必需）

```bash
sudo apt install -y mysql-server
sudo systemctl enable --now mysql
```

## 合并后的最小安装命令

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake \
  libsqlite3-dev libmysqlclient-dev libssl-dev \
  libcpp-httplib-dev mysql-server
```

## 构建验证

```bash
cmake -S . -B build && cmake --build build
```

> 当前阶段仅编译 `judge` 库与 `unit_tests`，已覆盖全部 build 依赖。运行期还需 MySQL 服务（步骤 6）；判题本身依赖系统 `g++`（已在步骤 1 覆盖）。