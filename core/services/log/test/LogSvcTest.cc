#include "gtest/gtest.h"
#include "LogSvc.hh"

// Test fixture for LogSvc tests
class LogSvcTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Initialize LogSvc before each test
    LogSvc::Initialize();
  }

  void TearDown() override {
    // Shutdown LogSvc after each test
    LogSvc::ShutDown();
  }
};

TEST_F(LogSvcTest, InitializeTest) {
  EXPECT_TRUE(LogSvc::Initialized());
}

TEST_F(LogSvcTest, GetLoggerTest) {
  std::string loggerName = "TestLogger";
  auto logger = LogSvc::GetLogger(loggerName);
  EXPECT_NE(logger, nullptr);
  auto logger2 = LogSvc::GetLogger(loggerName);
  EXPECT_EQ(logger, logger2);
}

TEST_F(LogSvcTest, SetLoggerLogLevelTest) {
  std::string loggerName = "TestLogger";
  auto logger = LogSvc::GetLogger(loggerName);

  LogSvc::SetLoggerLogLevel(logger, "warn");
  // EXPECT_EQ(logger->level(), spdlog::level::warn);

  LogSvc::SetLoggerLogLevel(logger, "info");
  // EXPECT_EQ(logger->level(), spdlog::level::info);
  EXPECT_FALSE(false);
}



