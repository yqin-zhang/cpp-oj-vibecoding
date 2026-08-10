#pragma once

#include <string>

namespace judge {

enum class OutputVerdict { AC, PE, WA };

OutputVerdict compareOutput(const std::string& actual, const std::string& expected);

std::string outputVerdictToString(OutputVerdict v);

}  // namespace judge