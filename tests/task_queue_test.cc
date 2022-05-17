#include <catch2/catch_test_macros.hpp>
#include <task_queue.h>

TEST_CASE("task_queue") {
  using namespace near_mem;

  auto [tx, rx] = make_task_queue<int>(1);
  REQUIRE(tx.full() == false);
  REQUIRE(rx.empty() == true);
  tx.push(1);
  REQUIRE(tx.full() == true);
  REQUIRE(rx.empty() == false);
  REQUIRE(rx.get() == 1);
  REQUIRE(rx.pop() == 1);
  REQUIRE(tx.full() == false);
  REQUIRE(rx.empty() == true);
}