#include <fmt/format.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Log.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("simple_test") {
  // 100k per file, max 3 files
  plog::init(plog::debug, "test.log", 100 * 1024, 3);
  PLOG_VERBOSE << fmt::format(
      "{}", "hello, this is a verbose message, should record every thing that "
            "can log the program, should be an addition to debug info");
  PLOG_DEBUG << fmt::format("this is a debug message, only appear when this is "
                            "usefull to find a bug");
  PLOG_INFO << fmt::format("this is an info message, only appear when this is "
                           "usefull to get the runtime status of the program");
  PLOG_WARNING << fmt::format("this is a warning message, only appear when "
                              "Something that is very dangerous is happening");
  PLOG_ERROR << fmt::format("this is an error message, only appear when "
                            "something is wrong, but we can still continue");
  PLOG_FATAL << fmt::format("this is a fatal message, only appear when "
                            "something is wrong and we can't continue");
}