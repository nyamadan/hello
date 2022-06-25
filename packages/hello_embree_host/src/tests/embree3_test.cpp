#include <gtest/gtest.h>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
#include <embree3/rtcore.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

class EMBREE3_Test : public ::testing::Test {
protected:
  virtual void SetUp() {
#if defined(__EMSCRIPTEN__)
    GTEST_SKIP() << "Not work for Emscripten";
#endif
  }
  virtual void TearDown() {}
};

TEST_F(EMBREE3_Test, TestInit) {
  RTCDevice device = rtcNewDevice(nullptr);
  ASSERT_NE(nullptr, device);
  rtcReleaseDevice(device);
}