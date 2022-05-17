#include <RamulatorConfig.h>
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <ramulator_wrapper.h>
#include <task_queue.h>
using namespace ramulator;
TEST_CASE("ramulator_test") {
  using namespace near_mem;
  uint64_t cycle = 0;
  Config m_config("DDR4-config.cfg");
  m_config.set_core_num(1);
  auto [task_tx, task_rx] = make_task_queue<MemTask>(10);
  auto [ret_tx, ret_rx] = make_task_queue<uint64_t>(10);
  auto mem = ramulator_wrapper(m_config, 64, cycle, task_rx, ret_tx);

  for (unsigned i = 0; i < 100; i++) {
    cycle++;
    task_tx.push({i * 1000, true});
    std::cout << mem.get_internal_size() << std::endl;
    mem.cycle();
    if (!ret_rx.empty()) {
      std::cout << mem.get_internal_size() << std::endl;

      auto ret = ret_rx.pop();
      //   std::cout << "out: " << ret << " " << cycle << std::endl;
    }
  }
  while (mem.have_outgoing()) {
    cycle++;

    // std::cout << mem.get_internal_size() << std::endl;
    mem.cycle();
    if (!ret_rx.empty()) {
      std::cout << mem.get_internal_size() << std::endl;
      auto ret = ret_rx.pop();
      //   std::cout << "out: " << ret << " " << cycle << std::endl;
    }
  }
}