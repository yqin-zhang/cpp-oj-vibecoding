#include "auth/password.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <cctype>
#include <cstring>
#include <vector>

namespace oj {
namespace auth {

namespace {

const char* kHex = "0123456789abcdef";

std::string hexEncode(const unsigned char* data, std::size_t len) {
  std::string out;
  out.reserve(len * 2);
  for (std::size_t i = 0; i < len; ++i) {
    out.push_back(kHex[(data[i] >> 4) & 0xF]);
    out.push_back(kHex[data[i] & 0xF]);
  }
  return out;
}

}  // namespace

std::string randomHex(std::size_t bytes) {
  if (bytes == 0) return "";
  std::vector<unsigned char> buf(bytes);
  if (RAND_bytes(buf.data(), static_cast<int>(bytes)) != 1) return "";
  return hexEncode(buf.data(), bytes);
}

std::string sha256Hex(const std::string& data) {
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) return "";
  int ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
           EVP_DigestUpdate(ctx, data.data(), data.size()) == 1 &&
           EVP_DigestFinal_ex(ctx, digest, &digest_len) == 1;
  EVP_MD_CTX_free(ctx);
  if (ok) return hexEncode(digest, digest_len);
  return "";
}

std::string hashPassword(const std::string& password, const std::string& salt) {
  return sha256Hex(salt + password);
}

bool verifyPassword(const std::string& password, const std::string& salt,
                    const std::string& expected_hex) {
  const std::string actual = hashPassword(password, salt);
  if (actual.size() != expected_hex.size()) return false;
  unsigned char diff = 0;
  for (std::size_t i = 0; i < actual.size(); ++i) {
    diff |= static_cast<unsigned char>(actual[i] ^ expected_hex[i]);
  }
  return diff == 0;
}

bool validUsername(const std::string& username) {
  if (username.size() < 3 || username.size() > 20) return false;
  for (char c : username) {
    if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
  }
  return true;
}

bool validPassword(const std::string& password) {
  if (password.size() < 8) return false;
  bool has_letter = false;
  bool has_digit = false;
  for (char c : password) {
    if (std::isalpha(static_cast<unsigned char>(c))) has_letter = true;
    if (std::isdigit(static_cast<unsigned char>(c))) has_digit = true;
  }
  return has_letter && has_digit;
}

}  // namespace auth
}  // namespace oj