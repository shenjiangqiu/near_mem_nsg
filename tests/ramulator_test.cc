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
  auto [task_tx, task_rx] = make_task_queue<MemReadTask>(10);
  auto [ret_tx, ret_rx] = make_task_queue<MemReadTask>(10);
  std::cout << "start" << std::endl;
  std::string str = "mem_test.txt";
  auto mem = ramulator_wrapper(m_config, 64, cycle, std::move(task_rx),
                               std::move(ret_tx), str);

  for (unsigned i = 0; i < 1; i++) {
    cycle++;
    task_tx.push({123456789, true, 0, 0, 0});
    std::cout << mem.get_internal_size() << std::endl;
    mem.cycle();
    if (!ret_rx.empty()) {
      std::cout << mem.get_internal_size() << std::endl;
      auto addr = ret_rx.get().addr;
      std::cout << "success" << std::endl;
      std::cout << "cycle: " << cycle << std::endl;
      std::cout << "addr: " << addr << std::endl;
      std::cout << "size: " << ret_rx.get().qe_name << std::endl;
      std::cout << "size: " << ret_rx.get().node_id << std::endl;
      ret_rx.pop();
      //   std::cout << "out: " << ret << " " << cycle << std::endl;
    }
  }
  while (mem.have_outgoing()) {
    cycle++;
    // std::cout << mem.get_internal_size() << std::endl;
    mem.cycle();
    if (!ret_rx.empty()) {
      std::cout << mem.get_internal_size() << std::endl;
      auto addr = ret_rx.get().addr;
      std::cout << "success" << std::endl;
      std::cout << "cycle: " << cycle << std::endl;
      std::cout << "addr: " << addr << std::endl;
      std::cout << "size: " << ret_rx.get().qe_name << std::endl;
      std::cout << "size: " << ret_rx.get().node_id << std::endl;
      ret_rx.pop();
      //   std::cout << "out: " << ret << " " << cycle << std::endl;
    }
  }
}