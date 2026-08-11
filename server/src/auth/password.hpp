#pragma once

// 密码哈希：SHA-256(salt + password) 十六进制 + 随机盐（libcrypto），
// 规则见 SPEC §2.1 / §5.1 / §4.7。

#include <cstddef>
#include <string>

namespace oj {
namespace auth {

// 生成随机十六进制串（bytes 为随机字节数，输出为 2*bytes 个 hex 字符）。
// 失败返回空串。
std::string randomHex(std::size_t bytes);

// SHA-256 十六进制串（小写）。失败返回空串。
std::string sha256Hex(const std::string& data);

// 哈希口令：sha256(salt + password) 的十六进制串。
std::string hashPassword(const std::string& password, const std::string& salt);

// 校验口令（常量时间比较，避免时序攻击）。
bool verifyPassword(const std::string& password, const std::string& salt,
                    const std::string& expected_hex);

// 用户名规则：3~20 位，字母 / 数字 / 下划线（SPEC §5.1）。
bool validUsername(const std::string& username);

// 密码规则：≥8 位且同时含字母与数字（SPEC §5.1）。
bool validPassword(const std::string& password);

}  // namespace auth
}  // namespace oj