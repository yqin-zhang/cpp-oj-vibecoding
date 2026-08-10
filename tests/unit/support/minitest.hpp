#pragma once

#include <cstdio>
#include <functional>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace minitest {

struct TestCase {
  std::string name;
  std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
  static std::vector<TestCase> r;
  return r;
}

inline int& failureCount() {
  static int n = 0;
  return n;
}

inline std::string& currentTest() {
  static std::string n;
  return n;
}

struct Registrar {
  Registrar(const char* name, std::function<void()> fn) {
    registry().push_back({name, std::move(fn)});
  }
};

template <typename T>
std::string repr(const T& v) {
  if constexpr (std::is_enum_v<T>) {
    return std::to_string(static_cast<std::underlying_type_t<T>>(v));
  } else {
    std::ostringstream os;
    os << v;
    return os.str();
  }
}

inline void reportFailure(const char* file, int line, const std::string& msg) {
  ++failureCount();
  std::printf("  [FAIL] %s\n         %s:%d\n         %s\n", currentTest().c_str(), file,
              line, msg.c_str());
}

#define CHECK(cond)                                                             \
  do {                                                                          \
    if (cond) {                                                                 \
      std::printf("  [PASS] CHECK(%s)\n", #cond);                               \
    } else {                                                                    \
      minitest::reportFailure(__FILE__, __LINE__,                              \
                              std::string("CHECK(") + #cond + ") failed");      \
    }                                                                           \
  } while (0)

#define CHECK_EQ(a, b)                                                          \
  do {                                                                          \
    const auto& va_ = (a);                                                      \
    const auto& vb_ = (b);                                                      \
    if (va_ == vb_) {                                                           \
      std::printf("  [PASS] CHECK_EQ(%s, %s)\n", #a, #b);                       \
    } else {                                                                    \
      minitest::reportFailure(                                                  \
          __FILE__, __LINE__, std::string("CHECK_EQ(") + #a + ", " + #b +       \
                                 ") left=" + minitest::repr(va_) +              \
                                 " right=" + minitest::repr(vb_));              \
    }                                                                           \
  } while (0)

#define TEST(suite, name)                                                       \
  static void suite##_##name##_body();                                          \
  static ::minitest::Registrar suite##_##name##_reg(#suite "." #name,           \
                                                    suite##_##name##_body);     \
  static void suite##_##name##_body()

inline int runAll() {
  std::printf("Unit tests: %zu case(s)\n\n", registry().size());
  for (const TestCase& t : registry()) {
    currentTest() = t.name;
    std::printf("[ RUN  ] %s\n", t.name.c_str());
    t.fn();
  }
  const int failed = failureCount();
  std::printf("\n%d failure(s) out of %zu test(s).\n", failed, registry().size());
  return failed == 0 ? 0 : 1;
}

}  // namespace minitest