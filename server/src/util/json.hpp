#pragma once

// 极简 JSON（RFC 8259 子集）：对象 / 数组 / 字符串 / 数值 / bool / null。
// 零依赖、header-only，供后端 REST API 使用（SPEC §6，错误体统一 JSON）。
//
// 用法：
//   Json o = Json::object();
//   o["error"] = "用户已存在"; o["code"] = 400;
//   std::string s = o.dump();          // {"error":"...","code":400}
//   Json j = Json::parse(text, &ok);   // ok=false 表示解析失败
//   j.get("username").asString();

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace oj {
namespace util {

class Json {
 public:
  enum class Type { Null, Bool, Number, String, Array, Object };

  Json() : type_(Type::Null) {}
  Json(std::nullptr_t) : type_(Type::Null) {}
  Json(bool b) : type_(Type::Bool), bool_(b) {}
  Json(int value) : type_(Type::Number), number_(static_cast<double>(value)) {}
  Json(long long value)
      : type_(Type::Number), number_(static_cast<double>(value)) {}
  Json(double value) : type_(Type::Number), number_(value) {}
  Json(const char* s) : type_(Type::String), string_(s) {}
  Json(const std::string& s) : type_(Type::String), string_(s) {}
  Json(std::string&& s) : type_(Type::String), string_(std::move(s)) {}

  static Json array() {
    Json j;
    j.type_ = Type::Array;
    return j;
  }
  static Json object() {
    Json j;
    j.type_ = Type::Object;
    return j;
  }

  Type type() const { return type_; }
  bool isNull() const { return type_ == Type::Null; }
  bool asBool() const { return bool_; }
  double asNumber() const { return number_; }
  long long asInt() const { return static_cast<long long>(number_); }
  const std::string& asString() const { return string_; }

  void setBool(bool b) {
    type_ = Type::Bool;
    bool_ = b;
  }
  void setNumber(double d) {
    type_ = Type::Number;
    number_ = d;
  }
  void setString(const std::string& s) {
    type_ = Type::String;
    string_ = s;
  }

  std::vector<Json>& array() { return array_; }
  std::map<std::string, Json>& object() { return object_; }
  const std::vector<Json>& array() const { return array_; }
  const std::map<std::string, Json>& object() const { return object_; }

  Json& operator[](const std::string& key) {
    type_ = Type::Object;
    return object_[key];
  }
  void push_back(const Json& v) { array_.push_back(v); }

  bool has(const std::string& key) const {
    return object_.find(key) != object_.end();
  }
  const Json& get(const std::string& key) const {
    static const Json kNull;
    auto it = object_.find(key);
    return it == object_.end() ? kNull : it->second;
  }

  std::string dump() const;

  // 解析失败返回 Null 值；可通过 ok 判断是否成功。
  static Json parse(const std::string& text, bool* ok = nullptr);

 private:
  Type type_;
  bool bool_ = false;
  double number_ = 0;
  std::string string_;
  std::vector<Json> array_;
  std::map<std::string, Json> object_;
};

namespace detail {

inline std::string jsonQuote(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 2);
  out.push_back('"');
  for (char c : s) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof(buf), "\\u%04x",
                        static_cast<unsigned char>(c));
          out += buf;
        } else {
          out += c;
        }
    }
  }
  out.push_back('"');
  return out;
}

struct JsonParser {
  const std::string& s;
  size_t i = 0;
  bool ok = true;

  explicit JsonParser(const std::string& text) : s(text) {}

  void skipWs() {
    while (i < s.size() &&
           (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) {
      ++i;
    }
  }

  bool expect(const char* word) {
    for (const char* p = word; *p; ++p) {
      if (i >= s.size() || s[i] != *p) {
        ok = false;
        return false;
      }
      ++i;
    }
    return true;
  }

  std::string parseString() {
    // 调用前保证 s[i] == '"'，返回值仅在 ok==true 时有效。
    ++i;
    std::string out;
    while (i < s.size()) {
      char c = s[i++];
      if (c == '"') return out;
      if (c == '\\') {
        if (i >= s.size()) {
          ok = false;
          return out;
        }
        char e = s[i++];
        switch (e) {
          case '"': out += '"'; break;
          case '\\': out += '\\'; break;
          case '/': out += '/'; break;
          case 'b': out += '\b'; break;
          case 'f': out += '\f'; break;
          case 'n': out += '\n'; break;
          case 'r': out += '\r'; break;
          case 't': out += '\t'; break;
          case 'u':
            if (i + 4 <= s.size()) {
              i += 4;  // 非 ASCII 转义：省略，后续如需中文再扩展
              out += "?";
            } else {
              ok = false;
            }
            break;
          default:
            ok = false;
            return out;
        }
      } else if (static_cast<unsigned char>(c) < 0x20) {
        ok = false;
        return out;
      } else {
        out += c;
      }
    }
    ok = false;
    return out;
  }

  Json parseValue() {
    skipWs();
    if (i >= s.size()) {
      ok = false;
      return Json();
    }
    char c = s[i];
    switch (c) {
      case '{': return parseObject();
      case '[': return parseArray();
      case '"': return Json(parseString());
      case 't':
        if (expect("true")) return Json(true);
        return Json();
      case 'f':
        if (expect("false")) return Json(false);
        return Json();
      case 'n':
        if (expect("null")) return Json();
        return Json();
      default: return parseNumber();
    }
  }

  Json parseObject() {
    Json obj = Json::object();
    ++i;  // consume '{'
    skipWs();
    if (i < s.size() && s[i] == '}') {
      ++i;
      return obj;
    }
    while (true) {
      skipWs();
      if (i >= s.size() || s[i] != '"') {
        ok = false;
        return obj;
      }
      std::string key = parseString();
      skipWs();
      if (i >= s.size() || s[i] != ':') {
        ok = false;
        return obj;
      }
      ++i;
      Json val = parseValue();
      if (!ok) return obj;
      obj.object()[key] = std::move(val);
      skipWs();
      if (i >= s.size()) {
        ok = false;
        return obj;
      }
      if (s[i] == ',') {
        ++i;
        continue;
      }
      if (s[i] == '}') {
        ++i;
        return obj;
      }
      ok = false;
      return obj;
    }
  }

  Json parseArray() {
    Json arr = Json::array();
    ++i;  // consume '['
    skipWs();
    if (i < s.size() && s[i] == ']') {
      ++i;
      return arr;
    }
    while (true) {
      Json val = parseValue();
      if (!ok) return arr;
      arr.push_back(std::move(val));
      skipWs();
      if (i >= s.size()) {
        ok = false;
        return arr;
      }
      if (s[i] == ',') {
        ++i;
        continue;
      }
      if (s[i] == ']') {
        ++i;
        return arr;
      }
      ok = false;
      return arr;
    }
  }

  Json parseNumber() {
    size_t start = i;
    bool integral = true;
    while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) ||
                            s[i] == '-' || s[i] == '+' || s[i] == '.' ||
                            s[i] == 'e' || s[i] == 'E')) {
      if (s[i] == '.' || s[i] == 'e' || s[i] == 'E') integral = false;
      ++i;
    }
    if (start == i) {
      ok = false;
      return Json();
    }
    std::string num = s.substr(start, i - start);
    try {
      if (integral) return Json(std::stoll(num));
      return Json(std::stod(num));
    } catch (...) {
      ok = false;
      return Json();
    }
  }
};

}  // namespace detail

inline std::string Json::dump() const {
  switch (type_) {
    case Type::Null: return "null";
    case Type::Bool: return bool_ ? "true" : "false";
    case Type::Number: {
      if (number_ == static_cast<long long>(number_)) {
        return std::to_string(static_cast<long long>(number_));
      }
      char buf[64];
      std::snprintf(buf, sizeof(buf), "%.10g", number_);
      return buf;
    }
    case Type::String: return detail::jsonQuote(string_);
    case Type::Array: {
      std::string out = "[";
      for (size_t k = 0; k < array_.size(); ++k) {
        if (k) out += ",";
        out += array_[k].dump();
      }
      out += "]";
      return out;
    }
    case Type::Object: {
      std::string out = "{";
      bool first = true;
      for (const auto& kv : object_) {
        if (!first) out += ",";
        first = false;
        out += detail::jsonQuote(kv.first);
        out += ":";
        out += kv.second.dump();
      }
      out += "}";
      return out;
    }
  }
  return "null";
}

inline Json Json::parse(const std::string& text, bool* ok) {
  detail::JsonParser p(text);
  Json out = p.parseValue();
  p.skipWs();
  if (p.ok && p.i != text.size()) p.ok = false;
  if (ok) *ok = p.ok;
  return p.ok ? out : Json();
}

}  // namespace util
}  // namespace oj