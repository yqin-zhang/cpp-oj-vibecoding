#include "judge/output_compare.hpp"

#include <string>

#include "support/minitest.hpp"

using judge::OutputVerdict;
using judge::compareOutput;
using judge::outputVerdictToString;

namespace {

TEST(output_compare, identical_output_is_ac) {
  CHECK_EQ(compareOutput("1 2\n3 4\n", "1 2\n3 4\n"), OutputVerdict::AC);
}

TEST(output_compare, trailing_whitespace_is_ignored) {
  CHECK_EQ(compareOutput("1 2  \t\n3 4\n", "1 2\n3 4\n"), OutputVerdict::AC);
}

TEST(output_compare, leading_and_trailing_blank_lines_ignored) {
  CHECK_EQ(compareOutput("\n\n1 2\n3 4\n\n", "1 2\n3 4\n"), OutputVerdict::AC);
}

TEST(output_compare, only_trailing_blank_lines_ignored) {
  CHECK_EQ(compareOutput("1 2\n3 4\n\n\n", "1 2\n3 4\n"), OutputVerdict::AC);
}

TEST(output_compare, empty_and_blank_only_both_ac) {
  CHECK_EQ(compareOutput("", ""), OutputVerdict::AC);
  CHECK_EQ(compareOutput("  \n\n", "\n"), OutputVerdict::AC);
}

TEST(output_compare, missing_newline_at_end_is_ac) {
  CHECK_EQ(compareOutput("1 2", "1 2\n"), OutputVerdict::AC);
}

TEST(output_compare, same_tokens_different_spacing_is_pe) {
  CHECK_EQ(compareOutput("1 2 3", "1\n2\n3"), OutputVerdict::PE);
}

TEST(output_compare, same_tokens_different_blank_lines_is_pe) {
  CHECK_EQ(compareOutput("a\n\nb\n\n", "a b"), OutputVerdict::PE);
}

TEST(output_compare, different_tokens_is_wa) {
  CHECK_EQ(compareOutput("1 2\n", "1 3\n"), OutputVerdict::WA);
}

TEST(output_compare, wrong_token_order_is_wa) {
  CHECK_EQ(compareOutput("3 2 1", "1 2 3"), OutputVerdict::WA);
}

TEST(output_compare, partial_match_right_side_only_is_wa) {
  CHECK_EQ(compareOutput("1 2\n", "1 2 3\n"), OutputVerdict::WA);
}

TEST(output_compare, verdict_to_string) {
  CHECK_EQ(outputVerdictToString(OutputVerdict::AC), "AC");
  CHECK_EQ(outputVerdictToString(OutputVerdict::PE), "PE");
  CHECK_EQ(outputVerdictToString(OutputVerdict::WA), "WA");
}

}  // namespace