# C++ OJ（仿 LeetCode）— 需求规格说明书 SPEC.md

> 版本：v1.0 ｜ 状态：已定稿（经 6 轮深度访谈确认）
> 后端：C++ (cpp-httplib) ｜ 前端：原生 HTML/CSS/JS

---

## 1. 需求总览

搭建一个仿 LeetCode 的在线判题（OJ）项目。用户浏览题目、在线编写 C/C++ 代码、提交判
题并查看结果；管理员管理题目、测试用例与用户。

### 1.1 业务目标与成功标准

| 项 | 结论 |
| --- | --- |
| 定位 | 个人学习 / 练手项目 |
| 成功标准 | 在单机上提供**媲美 LeetCode 的完整体验**：题目浏览、在线编码、即时判题、结果状态、排行榜、角色权限 |
| 规模目标 | 个位数并发提交重复进，**几十人同时判题**也能稳定排队、不卡死 |

### 1.2 V1 功能范围（已锁定）

**用户侧**
- 题目浏览（列表 + 详情）
- 在线代码编辑（原生 textarea）+ 提交判题
- 用户注册 / 登录，角色分普通用户、管理员
- 我的提交列表 / 判题结果状态查看（**仅本次会话内**，见 §4.2）
- 提交结果状态：八态（AC / WA / TLE / MLE / CE / RE / PE / OLE）
- 全站排行榜页（按解决数 / 通过率，**仅进程内**，见 §4.2）

**管理员侧**
- 题目 CRUD（题面、难度、时间/内存限制）
- 测试用例管理（标准 IO 用例组）
- 用户管理（删除、重置密码、查看用户）
- 提交日志 / 诊断（会话内提交与判题错误日志）

### 1.3 非功能需求

| 维度 | 要求 |
| --- | --- |
| 性能 | 单机、几十人并提交；判题全局闸 4 并发，其余等待前端轮询；HTTP 线程池须大于判题闸避免死锁 |
| 安全 | 用户提交流程 fork 隔离 + setrlimit 硬限制（时间/内存/输出）；严格权限隔离（普通用户只见自己） |
| 可用性 | 判题进程崩溃不拖垮服务进程，崩溃用例标记为系统错误（SE 语义并入 RE/诊断日志） |
| 成本 | 零云依赖，本机运行；依赖全为系统库（gcc、libsqlite3、libmysqlclient、libcrypto） |
| 可维护性 | server/ 后端 + web/ 前端双目录；CMake 构建；种子数据脚本 |

---

## 2. 技术栈与架构选型（含理由）

### 2.1 技术栈

| 层 | 选型 | 理由 |
| --- | --- | --- |
| HTTP 服务 | cpp-httplib（header-only，异步线程池） | 单头文件集成简单，个人项目零部署 |
| 判题语言 | 仅 C / C++（本机 g++ 编译运行） | 最小复杂度，一套编译参数 |
| 判题模型 | 本地即时判题 + **全局判题闸（并发 4）** | 避免线程池被 fork+编译 阻塞拖垮 |
| 沙箱 | 轻量 fork 隔离：`fork()` + `setrlimit()`，超时/内存/输出硬截断 | 个人项目接受轻量风险，能力足以保证限时限存正确工作 |
| 认证 | Session/Cookie（服务端内存 session + HTTP-Only Cookie） | 符合 AJAX 无刷新、区分角色；个人项目不引入 JWT 复杂度 |
| 用户数据 | **SQLite**（用户、session、会话内统计） | 嵌入式零部署 |
| 题目数据 | **MySQL**（题目、用例、题面 HTML） | 用户明确指定；题目为管理类型数据，独立存储 |
| 密码 | SHA-256 + 随机盐（系统 libcrypto） | 原生 C++，无额外第三方 |
| 构建 | CMake + 系统库 | 见规格工具链 |
| 协议 | REST + JSON，前端 `fetch` 消费 | 前后端清晰分离 |
| 前端 | 原生 HTML/CSS/JS，**无第三方 JS 库**；编辑器为原生 textarea | 用户明确要求纯原生 |

### 2.2 目录结构

```
cpp-oj-vibecoding/
├── CMakeLists.txt         # 顶层构建：add_subdirectory(server tests)
├── server/                # C++ 后端
│   ├── CMakeLists.txt     # 库 target：judge（判题核心，可独立链接测试）
│   ├── src/
│   │   ├── main.cpp       # 服务入口、路由注册、线程池配置
│   │   ├── db/            # SQLite / MySQL 访问封装（原生 C++ SQL，DAO）
│   │   ├── auth/          # session 管理、密码哈希、角色
│   │   └── judge/         # 判题器（编译、fork、setrlimit、比对、闸）
│   │       └── output_compare.*  # 输出比对核心（AC/WA/PE 判定，纯函数）
│   └── static/            # 编译期可选的静态资源路径（若无则指向 ../../web）
├── web/                   # 前端（原生 HTML/CSS/JS）
│   ├── index.html         # 题目列表
│   ├── problem.html?/v1   # 题目详情 + 在线编辑器
│   ├── login.html / register.html
│   ├── submissions.html   # 我的提交
│   ├── leaderboard.html   # 排行榜
│   ├── admin/             # 后台：题目/用例/用户/日志
│   └── css/ js/           # 样式与脚本
├── tests/                 # 自动化测试（原生 C++，零第三方库依赖）
│   ├── CMakeLists.txt     # 生成 unit_tests，接入 ctest
│   └── unit/
│       ├── support/minitest.hpp        # 极简断言/注册框架（无第三方框架）
│       ├── test_main.cpp               # 测试入口（RUN_ALL）
│       └── test_output_compare.cpp     # 判题比对核心单测（AC/WA/PE）
├── scripts/
│   ├── seed.sql           # MySQL 建表 + 种子数据（admin 账号 + 3 道示例题 + 用例）
│   └── seed.sh            # 一键建库、执行 seed.sql（可加 SQLite 建表初始化）
├── SPEC.md
└── README.md
```

Web 页面集合为 V1 用户侧最小集 + 排行榜 + 管理页，由 ADMIN 入口跳转，管理员与非管理员页面严格隔离。

### 2.3 前端页面清单

V1 前端共 **5 个核心页面**（原生 HTML/CSS/JS，无第三方库）：

| 页面 | 文件 | 访问权限 | 功能 |
| --- | --- | --- | --- |
| 登录页 | `login.html` | 公开 | 用户名 + 密码登录，成功后跳转题目列表；未登录 / 会话过期（401）自动跳转至此 |
| 注册页 | `register.html` | 公开 | 新用户注册，成功后自动登录并跳转题目列表（细则见 §5.1） |
| 题目列表页 | `index.html` | 公开可浏览 / 登录可提交 | 全部题目卡片：标题、难度标签、通过状态；点击卡片进入详情 |
| 题目详情页 | `problem.html?/v1` | 公开可浏览 / 登录可提交 | 题面（HTML 渲染）+ 示例用例 + 原生 textarea 编辑器 + 语言（C/C++）选择 + 提交按钮 + 判题结果轮询展示 |
| 后台管理页 | `admin/`（题目 / 用例 / 用户 / 日志 四个子页） | 仅 admin | 题目 CRUD、标准用例组管理、用户管理、判题日志与诊断；非 admin 访问一律 403 |

页面间导航关系：`register.html ⇄ login.html → index.html → problem.html → submissions.html / leaderboard.html`；后台管理入口仅 admin 登录后在导航栏可见。
（另含辅助页 `submissions.html` 我的提交、`leaderboard.html` 排行榜，见 §2.2。）

---

## 3. 架构图

```
┌──────────────────────────── 浏览器 ────────────────────────────┐
│  原生 HTML/CSS/JS   │  题目列表  │  题目详情+编辑器  │  登录/注册  │
│  fetch(REST/JSON)   │  我的提交  │  排行榜 ── 管理后台(Admin Only)  │
└──────────────────────────────┬──────────────────────────────┘
                               │ HTTP(S) + Cookie(会话)
┌──────────────────────────────▼──────────────────────────────┐
│                     server/  (cpp-httplib 线程池)              │
│  路由层：/api/problems   /api/submit   /api/login   /api/admin/* │
│       │                          │                          │
│  ┌────▼─────┐   ┌───────────────▼──────────────────────┐    │
│  │ auth/    │   │  judge/ 判题器                        │    │
│  │ ①session │   │  全局判题闸(信号量 N=4)               │    │
│  │ ②密码哈希│   │  → 写源码临时文件 → g++ 编译           │    │
│  │ ③角色    │   │  → fork() + setrlimit(时间/内存/输出) │    │
│  └────┬─────┘   │      每个用例 stdin → 捕获 stdout      │    │
│       │         │      → 尾随空白容错比对 → 八态判定     │    │
│  ┌────▼─────┐   └───────────────┬──────────────────────┘    │
│  │ SQLite   │   用户/会话/进程内统计                        │
│  │ (系统库) │                                              │
│  └──────────┘   ┌───────────────▼──────────────────────┐    │
│                 │ MySQL：题目 CRUD、用例、题面           │    │
│                 │ 存储：题面 (HTML)、难度、时限、内存     │    │
│                 └──────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────┘
```

**并发模型关键点**：cpp-httplib 线程池大小配置须 **> 判题闸并发数（4）**，例如线程池 8，
判题闸 4。判题在独立于 HTTP 工作线程的方式中排队执行；闸满时接口立即返回
`status: "queued"`，前端按间隔轮询 `/api/submissions/:id`。

---

## 4. 数据模型与边界条件

### 4.1 存储边界（已锁定）

| 数据 | 存储 | 说明 |
| --- | --- | --- |
| 题目 / 用例 / 题面 | MySQL | 题面以 **HTML 片段**存储（前端 `innerHTML` 直出，避免引入 Markdown 库） |
| 用户、session | SQLite | 用户持久化；session 亦可驻内存 |
| 提交明细 / 代码 | **不持久化** | 仅进程内 / 会话内可见（见 §4.2） |

### 4.2 提交记录与排行榜矛盾点的最终裁定

用户此前选「提交历史 / 排行榜」，又明确要求「提交记录不持久化」。裁定如下（以此为准）：

- **提交记录不落库**：`我的提交` 页面仅展示**本次会话内**的提交（前端内存态即可，或由
  会话 cookie 关联的临时索引，不写 DB）。
- **排行榜**：**仅进程内临时聚合**（内存 map 按用户累计 AC / N-已解决），**进程重启排行榜清零**。
- 管理员**提交日志 / 诊断**同样为进程内日志（环形缓冲），重启清空。
- 权衡说明：丧失长期历史与持久排行，换取「个人项目零落盘代码」、避免磁盘增长与隐私扩散；
  与 leetcode 的长期统计体验有差异，属用户取舍。

### 4.3 判题语义（八态）

| 状态 | 含义 |
| --- | --- |
| AC | 全部用例输出正确 |
| WA | 至少一个用例输出与期望不符（尾随空白容错后仍不同） |
| TLE | 单用例超出题目配置时限（**运行时限，不含编译**） |
| MLE | 超出题目配置内存限制 |
| CE | 编译失败（返回编译器信息） |
| RE | 运行异常（非零退出 / 信号，如段错误） |
| PE | 内容正确但格式不符（normalize 后仍不同而逐 token 相同） |
| OLE | 输出超长（超出截断上限立即终止） |

### 4.4 用例与比对规则

- 用例为标准 **IO 用例组**：`(输入文本, 期望输出文本)` 的多对集合。
- 判题时对每个用例：以输入为 stdin 启动，捕获 stdout；**库内标准用例对用户不可见**
  （题面仅展示示例用例）。
- 比对：**尾随空白容错**——忽略行尾空白与首尾空行；仅格式不同（逐 token 相同）判 PE。
- 输出截断：单用例输出截断上限（如 64KB），触顶判 OLE 并 kill。

### 4.5 题目字段

```
id, title, difficulty(easy/medium/hard), html(content),
time_limit_ms(默认 2000, 运行时限), memory_limit_mb(默认 256),
示例用例(展示用), 标准用例组(隐藏), created_at, updated_at
```

### 4.6 边缘案例 / 异常处理

| 场景 | 处理 |
| --- | --- |
| 用户代码死循环 | 单用例超时限 → SIGKILL → TLE；判题目进程崩溃不影响服务主进程 |
| 用户代码疯狂分配内存 / fork 炸弹 | `setrlimit(RLIMIT_AS / RLIMIT_NPROC)` 硬上限 → MLE / RE |
| 编译失败 | 返回 CE + 编译器 stderr（截断） |
| 判题进程被 kill / 服务重启 | 进行中的提交标记为 **SE/错误（并入诊断日志）**，由管理员人工介入重查；不自动重判 |
| 空白/空输入用例 | 输入可为空，stdin EOF，正常判比 |
| 超大输出 | 截断捕获，判 OLE，不污染内存 |
| 并发 4 闸满 | 接口返回 `queued` + 排队序号，前端轮询；HTTP 线程池 8 保证非判题请求（浏览/登录）不受阻塞 |
| 权限越权 | 普通用户访问后台 → 403；他人提交不可见（严格隔离） |
| 未登录 | 仅可浏览题目；提交/我的提交/排行榜需登录 401 |
| Session 过期 | 前端收到 401 跳转登录 |
| 中文乱码 | 全局 UTF-8，MySQL charset=utf8mb4 |
| 库连接异常 | API 返回 500 + 结构化错误体，判题闸原子释放避免假死 |

### 4.7 数据库建表 SQL

**MySQL（题目 / 用例 / 题面，库名 `oj_problems`）**：全部 `utf8mb4`，见 §4.6「中文乱码」。
标准用例与示例用例合入单表，用 `is_sample` 区分（API 只向用户暴露 `is_sample=1` 的示例）。

```sql
-- 0) 建库（utf8mb4，支持中文 / emoji）
CREATE DATABASE IF NOT EXISTS oj_problems
  DEFAULT CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;
USE oj_problems;

-- 1) 题目表（对应 §4.5 字段；题面 HTML 以 MEDIUMTEXT 存储）
CREATE TABLE IF NOT EXISTS problems (
  id              BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  title           VARCHAR(255) NOT NULL,
  difficulty      ENUM('easy','medium','hard') NOT NULL DEFAULT 'easy',
  content_html    MEDIUMTEXT   NOT NULL,                -- 题面 HTML 片段
  time_limit_ms   INT UNSIGNED NOT NULL DEFAULT 2000,   -- 运行时限（不含编译）
  memory_limit_mb INT UNSIGNED NOT NULL DEFAULT 256,    -- 内存限制 (MB)
  created_at      TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at      TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
                    ON UPDATE CURRENT_TIMESTAMP,
  KEY idx_difficulty (difficulty)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 2) 用例表（标准 IO 用例组，题目∷用例 一对多；删题级联删用例）
CREATE TABLE IF NOT EXISTS test_cases (
  id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  problem_id   BIGINT UNSIGNED NOT NULL,
  input_text   MEDIUMTEXT NOT NULL,     -- 可为空字符串（空 stdin，EOF）
  output_text  MEDIUMTEXT NOT NULL,     -- 期望输出
  is_sample    TINYINT(1) NOT NULL DEFAULT 0,   -- 1=示例(展示)，0=标准(隐藏)
  sort_order   INT NOT NULL DEFAULT 0,  -- 判题执行顺序
  created_at   TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT fk_tc_problem FOREIGN KEY (problem_id)
    REFERENCES problems(id) ON DELETE CASCADE,
  KEY idx_problem (problem_id, sort_order)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

**SQLite（用户，库文件 `oj.db`）**：用户持久化；session 驻内存、提交/排行不落库（§4.2）。

```sql
-- 用户表（password_hash = SHA-256(salt + password) 十六进制串）
CREATE TABLE IF NOT EXISTS users (
  id            INTEGER PRIMARY KEY AUTOINCREMENT,
  username      TEXT NOT NULL UNIQUE,        -- 3~20 位字母/数字/下划线
  password_hash TEXT NOT NULL,
  salt          TEXT NOT NULL,
  role          TEXT NOT NULL DEFAULT 'user',-- role: user | admin
  created_at    TEXT NOT NULL DEFAULT (datetime('now'))
);
```

> 说明：`.sql` DDL 与种子数据同步维护于 `scripts/seed.sql`（主板本以此处为准）；admin
> 账号由 `seed.sh` + 服务端首次启动时写入 SQLite（盐随机生成，避免明文入库）。

---

## 5. 用户流程

### 5.1 注册功能细则

| 项 | 规则 |
| --- | --- |
| 入口 | 登录页底部「还没有账号？立即注册」→ `register.html` |
| 输入字段 | 用户名、密码、确认密码（前端校验两次输入一致） |
| 前端校验 | 用户名 3~20 位（字母 / 数字 / 下划线）；密码 ≥ 8 位且同时含字母与数字；两次密码一致；不满足则本地提示、不发送请求 |
| 后端校验 | 以服务端为准复用同一套规则；用户名唯一（已存在返回 400「用户名已存在」） |
| 密码存储 | SHA-256 + 随机盐哈希入库，明文不落库、不打印日志（见 §2.1） |
| 成功处理 | 注册成功即自动登录（种 Session Cookie），前端跳转题目列表页 |
| 失败处理 | 返回结构化错误体 `{"error":"...", "code":400}`，表单区内联红字提示，不清空已填用户名 |
| 防滥用 | 可选：注册接口做简单限流（如每 IP 每分钟 ≤ 10 次） |
| 对应 API | `POST /api/register`（见 §6） |

**普通用户**
```
注册/登录
   ↓
题目列表（浏览难度/标题）
   ↓
题目详情页：题面(HTML) + textarea 编写 C/C++ + 示例用例展示
   ↓
点击提交 → POST /api/submit
   ↓
后端：闸满？→返回 queued(轮询) ；否则进判题（编译→逐用例运行比对）
   ↓
GET /api/submissions/:id 得八态结果 + 耗时/内存 → 页面展示 AC🎉 / WA / CE(编译器信息)
   ↓
「我的提交」查看本次会话提交列表；「排行榜」查看进程内排名
```

**管理员**
```
Admin 登录（种子账号）→ 后台入口（仅 admin 可见）
   ↓
题目 CRUD：录入题面 HTML、难度、时限、内存、示例用例
   ↓
用例管理：批量录入/编辑标准 IO 用例组（隐藏）
   ↓
用户管理：禁用/删除、重置密码、查看用户列表
   ↓
日志/诊断：进程内提交日志与判题错误查看
```

---

## 6. API 边界（REST + JSON, 前缀 /api）

| Method | Path | 权限 | 说明 |
| --- | --- | --- | --- |
| POST | /api/register | 公开 | 注册（用户名唯一、密码哈希入库） |
| POST | /api/login | 公开 | 登录，种 Session Cookie |
| POST | /api/logout | 登录 | 注销 |
| GET | /api/me | 登录 | 当前用户 + role |
| GET | /api/problems | 公开 | 题目列表（不含隐藏用例） |
| GET | /api/problems/:id | 公开 | 题目详情（含示例，**不含标准用例**） |
| POST | /api/submit | 登录 | 提交代码：题 id + 源码；返回 submission id / queued |
| GET | /api/submissions/:id | 登录 | 轮询判题结果（八态；CE 附编译器信息） |
| GET | /api/submissions/my | 登录 | 本次会话我的提交（进程内） |
| GET | /api/leaderboard | 登录 | 进程内排行（AC 数 / 通过率排序） |
| POST | /api/admin/problems | admin | 新建 / 更新题目（字段含时限内存） |
| (DELETE|PUT) | /api/admin/problems/:id | admin | 改题 / 删题 |
| POST | /api/admin/problems/:id/cases | admin | 批量增删改标准用例 |
| GET | /api/admin/cases/:pid | admin | 查看/导出隐藏用例 |
| GET | /api/admin/users | admin | 用户列表 |
| (DELETE|PUT) | /api/admin/users/:id | admin | 删除 / 重置密码 |
| GET | /api/admin/logs | admin | 进程内判题日志 / 诊断 |

错误体统一形如：`{"error":"message", "code": xxx}`，认证失败 401、越权 403、参数错 400、内部错 500。

---

## 7. TODO 清单（实施顺序，按里程碑）

### M1 基建与认证
- [ ] 仓库结构，CMake（cpp-httplib + libsqlite3 + libmysqlclient + libcrypto + g++ 探测）
- [ ] SQLite 封装（用户表）、MySQL 封装（题目表）
- [ ] 密码哈希（SHA-256 + salt）与 Session/Cookie 登录注册
- [ ] 角色模型：普通用户 / admin 中间件

### M2 题目与用例管理
- [ ] 题目 CRUD API + 题面 HTML 存取（utf8mb4）
- [ ] 用例组 CRUD API（标准 IO 用例组）
- [ ] 种子脚本：admin 账号 + 2~3 道示例题（含用例与示例）

### M3 判题器（核心）
- [ ] 源码临时文件 → 本机 g++ 编译（收集 CE）
- [ ] fork + setrlimit（时间 clip / 内存 / NPROC / 输出 fd 截断）
- [ ] 全局判题闸（信号量 N=4）+ HTTP 线程池 8
- [ ] 逐用例 stdin/stdout 运行 + 尾随空白容错比对 + 八态判定
- [ ] 判题进程崩溃/超时 kill 的健壮处理，诊断日志
- [ ] 提交 API + 轮询 API（queued/状态流转）

### M4 前端（原生）
- [ ] 登录/注册页 → 题目列表页 → 题目详情页（HTML 题面 + textarea 编辑器）
- [ ] 提交结果展示与轮询、我的提交（会话内）
- [ ] 排行榜页（进程内数据）
- [ ] 管理后台：题目/用例/用户/日志 四个子页，严格 admin 隔离
- [ ] 全局样式（类 LeetCode 简洁布局）、错误提示、401 跳转

### M5 测试与验收
- [ ] **测试框架**：自研极简框架 `tests/unit/support/minitest.hpp`（断言 + 用例注册，零第三方依赖），经 `tests/CMakeLists.txt` 接入 `ctest`
- [ ] **判题核心单测**（`test_output_compare.cpp`，纯函数无需外部进程）：
  - AC：完全相同 / 行尾尾随空白 / 首尾空行 / 全文仅尾随空白
  - PE：逐 token 相同但空白/换行位置不同
  - WA：token 顺序或内容不同
- [ ] **判题器集成测试**（起真实 g++ + fork 沙箱，覆盖八态用例）：
  - AC：正确程序 → AC
  - WA：输出值不同 → WA；PE：输出 token 相同格式不同 → PE
  - CE：语法错误源码 → CE（附编译器 stderr）
  - TLE：`while(true);` → TLE；MLE：`malloc(1GB)` → MLE
  - RE：`*(int*)0 = 1;`（段错误）→ RE；OLE：死循环打印 → OLE
- [ ] **前后端联调**：注册/登录 → 提交 → 轮询全过程冒烟
- [ ] **并发压测**：脚本模拟 ≥30 并发提交，确认判题排队、浏览/登录不阻塞（验收 8）
- [ ] **文档**：README（构建 / 运行 / 种子数据 / 测试命令）

---

## 8. 验收标准

**功能**
1. 注册/登录成功，未登录访问受保护 API 返回 401，普通用户访问后台返回 403。
2. 可正常浏览题目；题面 HTML 正确渲染；标准用例对普通用户不可见（API 响应不含之）。
3. 提交 AC/WA 题解分别得到正确八态结果；CE 返回编译器信息；死循环样例在时限内被 kill 判 TLE。
4. 内存超限题解被 kill 判 MLE；超大输出判 OLE。
5. `我的提交` 返回本次会话提交；排行榜按 AC 数/通过率正确排序。
6. 管理员可增删改题目、增删改用例、删除用户、重置密码、查看日志；均需 admin 角色。
7. 种子脚本运行后可登录 admin，并存在 2~3 道可 AC 的示例题。

**性能 / 健壮性**
8. 客户端数并发测试：模拟 **≥30 个同时提交**，51 分钟内全部完成判题；过程中题目浏览/登录仍即时响应（证明线程池 8 > 闸 4 不阻塞）。
9. 提交一个 `while(1);` 死循环 + 一个 `malloc(1GB)` 程序，服务进程 CPU/内存保持稳定，仅判题目进程被 kill。
10. 服务重启后服务正常；重启前的会话内提交/排行榜按约定清空。

**安全**
11. 普通用户无法读取他人提交或隐藏用例；SQL 全部参数化（无注入风险）。
12. 会话 Cookie 设 HttpOnly；越权路径逐一返回 403。

**代码质量**
13. `cmake --build` 零 warning 构建成功；判题器核心逻辑有自动化测试覆盖八态。

**测试运行（统一命令）**
14. 顶层构建 + 全部单测：`cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`，一次零 warning、全绿。
15. 判题比对单测（`unit_tests`)覆盖 §4.4：AC（尾随空白/首尾空行容错）、PE（token 相同空白不同）、WA（token 不同）；集成测试逐条构造八态源码并断言对应状态。

---

## 9. 风险与权衡（访谈中确认决策）

| 风险 / 权衡 | 裁定 | 说明 |
| --- | --- | --- |
| 「轻量 fork 隔离」安全不彻底 | 接受：个人项目信任自我代码；不暴露公网或加 DB + 网 络访问限制即可 | 若要公网部署，后续替换 Docker 容器（接口保持判题目工具可换） |
| cpp-httplib 阻塞模型 vs 即时判题 | 用「全局闸 4 + 线程池 8 + queued 轮询」解耦 | 牺牲一点首判延迟换稳定 |
| 提交不持久化 vs 历史/排行 | 裁定为进程内临时数据 | 见 §4.2；若后续要长期统计，给提交表加开关即可 |
| 双存储（MySQL+SQLite） | 用户明确选定 | 封装统一 DAO 接口，后续可合并/拆分 |
| 原生 textarea 无语法高亮 | 接受：保持纯原生；若体验不足再单独引入 Ace/Monaco（不影响后端） | 最小集优先 |
| 题面存 HTML 片段 | 接受：XSS 风险仅在管理员录入，默认信任 admin | 后端做基本标签过滤（剔除 `<script>`/`on*`） |
| 仅 C/C++ | 用户确认：V1 最小化；语言插槽预留（Docker 化时扩展） | 判题目控制器与语言配置解耦 |

---

## 10. 开放项（后续可选）

- 长期提交历史 / 持久排行榜开关
- Docker 判题目替换轻量沙箱
- 多语言（Java/Python）支持
- 语法高亮编辑器（Ace/Monaco）
- Markdown 题面渲染

---

*规格经由 6 轮访谈收敛，所有决策以此文档为准。*