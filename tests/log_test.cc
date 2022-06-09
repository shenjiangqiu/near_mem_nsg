#include <fmt/format.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Log.h>
#include <catch2/catch_test_macros.hpp>
#include <task_queue.h>
#include <dc.h>
#include <struct.h>

unsigned num_compute_unit = 64;
unsigned latency_compute_unit = 1;
unsigned dim = 64;

TEST_CASE("dc_test") {
  using namespace near_mem;

  uint64_t cycle = 0;
  auto [dist_sender, dist_receiver] = make_task_queue<DistanceTask>(1);
  auto [return_dist_sender, return_dist_receiver] = make_task_queue<DistanceTask>(1);
  auto dc = Distance_Computer(
      cycle,
      0,
      dist_receiver,
      return_dist_sender
      );
  dist_sender.push(DistanceTask(2, 3, 4));
  int i=0;
  while(++i){
    dc.do_cycle();
    if (!return_dist_receiver.empty()) {
      auto ret = return_dist_receiver.get();
      REQUIRE(ret.qe_name == 2);
      REQUIRE(ret.query_id == 3);
      REQUIRE(ret.node_id == 4);
      return_dist_receiver.pop();
      std::cout << "i=" << i << std::endl;
      break;
    }
  }

}

// TEST_CASE("simple_test") {
//   // 100k per file, max 3 files
//   plog::init(plog::debug, "test.log", 100 * 1024, 3);
//   PLOG_VERBOSE << fmt::format(
//       "{}", "hello, this is a verbose message, should record every thing that "
//             "can log the program, should be an addition to debug info");
//   PLOG_DEBUG << fmt::format("this is a debug message, only appear when this is "
//                             "usefull to find a bug");
//   PLOG_INFO << fmt::format("this is an info message, only appear when this is "
//                            "usefull to get the runtime status of the program");
//   PLOG_WARNING << fmt::format("this is a warning message, only appear when "
//                               "Something that is very dangerous is happening");
//   PLOG_ERROR << fmt::format("this is an error message, only appear when "
//                             "something is wrong, but we can still continue");
//   PLOG_FATAL << fmt::format("this is a fatal message, only appear when "
//                             "something is wrong and we can't continue");
// }