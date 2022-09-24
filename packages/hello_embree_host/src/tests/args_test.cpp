#include <gtest/gtest.h>

#include "../core/utils/arguments.hpp"

TEST(ArgsTest, GetFile) {
  const auto argc = 3;
  const char *const argv[argc] = {"./test", "-f", "test.txt"};
  auto result =
      hello::utils::arguments::parse(argc, argv, std::cout, std::cerr);
  ASSERT_EQ("test.txt", result.file);
}

TEST(ArgsTest, ShowHelp) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  const auto argc = 2;
  const char *const argv[argc] = {"./test", "-h"};
  ASSERT_THROW(hello::utils::arguments::parse(argc, argv, std::cout, std::cerr),
               hello::utils::arguments::ShowHelp);
}

TEST(ArgsTest, NoArgs) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  const auto argc = 1;
  const char *const argv[argc] = {"./test"};
  ASSERT_THROW(hello::utils::arguments::parse(argc, argv, std::cout, std::cerr),
               hello::utils::arguments::ArgumentsError);
}

TEST(ArgsTest, ArgumentsError) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  const auto argc = 2;
  const char *const argv[argc] = {"./test", "-f"};
  ASSERT_THROW(hello::utils::arguments::parse(argc, argv, std::cout, std::cerr),
               hello::utils::arguments::ArgumentsError);
}