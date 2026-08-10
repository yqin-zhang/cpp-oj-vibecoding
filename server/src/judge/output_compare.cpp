#include "judge/output_compare.hpp"

#include <cctype>
#include <sstream>
#include <vector>

namespace judge {

namespace {

std::string trimRight(const std::string& s) {
  std::size_t end = s.size();
  while (end > 0 && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
    --end;
  }
  return s.substr(0, end);
}

std::vector<std::string> normalizedLines(const std::string& s) {
  std::vector<std::string> lines;
  std::size_t pos = 0;
  while (pos <= s.size()) {
    std::size_t nl = s.find('\n', pos);
    if (nl == std::string::npos) {
      lines.push_back(trimRight(s.substr(pos)));
      break;
    }
    lines.push_back(trimRight(s.substr(pos, nl - pos)));
    pos = nl + 1;
  }
  std::size_t begin = 0;
  std::size_t end = lines.size();
  while (begin < end && lines[begin].empty()) {
    ++begin;
  }
  while (end > begin && lines[end - 1].empty()) {
    --end;
  }
  return std::vector<std::string>(lines.begin() + static_cast<std::ptrdiff_t>(begin),
                                  lines.begin() + static_cast<std::ptrdiff_t>(end));
}

std::string joinLines(const std::vector<std::string>& lines) {
  std::string joined;
  for (const std::string& line : lines) {
    joined += line;
    joined += '\n';
  }
  return joined;
}

std::vector<std::string> tokens(const std::string& s) {
  std::istringstream in(s);
  std::vector<std::string> result;
  std::string word;
  while (in >> word) {
    result.push_back(word);
  }
  return result;
}

}  // namespace

OutputVerdict compareOutput(const std::string& actual, const std::string& expected) {
  const std::string a = joinLines(normalizedLines(actual));
  const std::string e = joinLines(normalizedLines(expected));
  if (a == e) {
    return OutputVerdict::AC;
  }
  if (tokens(actual) == tokens(expected)) {
    return OutputVerdict::PE;
  }
  return OutputVerdict::WA;
}

std::string outputVerdictToString(OutputVerdict v) {
  switch (v) {
    case OutputVerdict::AC:
      return "AC";
    case OutputVerdict::PE:
      return "PE";
    case OutputVerdict::WA:
      return "WA";
  }
  return "UNKNOWN";
}

}  // namespace judge