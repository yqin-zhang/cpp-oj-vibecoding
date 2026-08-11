#include "auth/password.hpp"

#include <cstdio>

using oj::auth::hashPassword;
using oj::auth::randomHex;
using oj::auth::validPassword;
using oj::auth::validUsername;
using oj::auth::verifyPassword;

int runPasswordTests() {
  int failed = 0;

  // SHA-256 已知向量：SHA256("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
  if (oj::auth::sha256Hex("abc") !=
      "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") {
    std::printf("FAIL sha256(\"abc\")\n");
    ++failed;
  }

  // 同口令同名盐 → 同哈希；不同盐 → 不同哈希
  const std::string salt = randomHex(16);
  if (salt.size() != 32) {
    std::printf("FAIL randomHex length\n");
    ++failed;
  }
  if (randomHex(16) == randomHex(16)) {
    std::printf("FAIL salt 非随机\n");
    ++failed;
  }
  if (hashPassword("Passw0rd", salt) != hashPassword("Passw0rd", salt)) {
    std::printf("FAIL 确定性哈希\n");
    ++failed;
  }
  if (hashPassword("Passw0rd", salt) == hashPassword("Passw0rd", randomHex(16))) {
    std::printf("FAIL 盐应改变哈希\n");
    ++failed;
  }

  // 校验
  const std::string h = hashPassword("Passw0rd", salt);
  if (!verifyPassword("Passw0rd", salt, h)) {
    std::printf("FAIL 正确口令未通过\n");
    ++failed;
  }
  if (verifyPassword("Passw0rdX", salt, h)) {
    std::printf("FAIL 错误口令通过了校验\n");
    ++failed;
  }

  // 用户名规则
  if (!validUsername("abc")) {
    std::printf("FAIL validUsername(\"abc\")\n");
    ++failed;
  }
  if (validUsername("ab")) {
    std::printf("FAIL 2 位用户名不应合法\n");
    ++failed;
  }
  if (validUsername("a b")) {
    std::printf("FAIL 含空格用户名不应合法\n");
    ++failed;
  }
  if (!validUsername("_ab1C90")) {
    std::printf("FAIL 下划线用户名应合法\n");
    ++failed;
  }

  // 密码规则
  if (!validPassword("Passw0rd")) {
    std::printf("FAIL validPassword(\"Passw0rd\")\n");
    ++failed;
  }
  if (validPassword("abcdefgh")) {
    std::printf("FAIL 纯字母密码不应合法\n");
    ++failed;
  }
  if (validPassword("12345678")) {
    std::printf("FAIL 纯数字密码不应合法\n");
    ++failed;
  }
  if (validPassword("Pa1")) {
    std::printf("FAIL 过短密码不应合法\n");
    ++failed;
  }

  return failed;
}