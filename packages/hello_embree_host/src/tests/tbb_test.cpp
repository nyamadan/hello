#if defined(_MSC_VER)
#include <gtest/gtest.h>

#include <oneapi/tbb.h>

class TBB_Test : public ::testing::Test {
protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST_F(TBB_Test, TestSum) {
  auto sum = oneapi::tbb::parallel_reduce(
      oneapi::tbb::blocked_range<int>(1, 101), 0,
      [](oneapi::tbb::blocked_range<int> const &r, int init) -> int {
        for (int v = r.begin(); v != r.end(); v++) {
          init += v;
        }
        return init;
      },
      [](int lhs, int rhs) -> int { return lhs + rhs; });
  ASSERT_EQ(5050, sum);
}
#endif