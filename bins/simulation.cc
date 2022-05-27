#include <PrimitiveBloomFilter.h>
#include <bitset>
#include <boost/dynamic_bitset.hpp>
#include <chrono>
#include <component.h>
#include <controller.h>
#include <ctime>
#include <dc.h>
#include <efanna2e/exceptions.h>
#include <efanna2e/index_nsg.h>
#include <efanna2e/parameters.h>
#include <efanna2e/util.h>
#include <fmt/format.h>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Log.h>
#include <qe.h>
#include <struct.h>
#include <task_queue.h>
#include <toml.hpp>
#include <vector>

using namespace std;
using namespace std::chrono;
using namespace near_mem;

const uint64_t L=200;

int main() {
  uint64_t current_cycle = 0;
  plog::init(plog::debug, "test_0526.log", 100 * 1024 * 1024, 3);
  unsigned num_qe = 8;
  unsigned num_dc = 8;
  auto [mem_sender, mem_receiver] = make_task_queue<MemReadTask>(64);
  auto [mem_ret_sender, mem_ret_receiver] = make_task_queue<MemReadTask>(64);
  auto [nsg_sender, nsg_receiver] = make_task_queue<NsgTask>(4);
  
  auto controller = Controller(
    current_cycle, 
    nsg_sender,
    mem_sender, 
    mem_ret_receiver, 
    100, 
    1,
    efanna2e::Metric::L2, 
    nullptr, 
    nullptr, 
    10, 
    100);

  auto [dist_sender, dist_receiver] = make_task_queue<DistanceTask>(4);
  auto [return_dist_sender, return_dist_receiver] = make_task_queue<DistanceTask>(4);
  std::vector<Query_Engine> qes;
  for (auto i = 0u; i < num_qe; ++i) {
    qes.push_back(Query_Engine(
      current_cycle, 
      i,
      nsg_receiver, 
      dist_sender, 
      return_dist_receiver,
      mem_sender,
      mem_ret_receiver
      ));
  }
  std::vector<Distance_Computer> dcs;
  for (auto i = 0u; i < num_dc; ++i) {
    dcs.push_back(Distance_Computer(
      current_cycle,
      i,
      dist_receiver, 
      return_dist_sender
      ));
  }

  std::vector<Component *> components;
  components.push_back(&controller);
  for (auto &qe : qes) {
    components.push_back(&qe);
  }
  for (auto &dc : dcs) {
    components.push_back(&dc);
  }
  int DEAD_LOCK = 0;
  while (true) {
    DEAD_LOCK++;
    if (DEAD_LOCK > 100000) {
      PLOG_DEBUG << "dead lock";
      break;
    }
    for (auto component : components) {
      component->cycle();
      current_cycle++;
    }
    bool all_end = true;

    for (auto component : components) {
      if (!component->all_end()) {
        all_end = false;
      }
    }
    if (all_end) {
      break;
    }
  }
  PLOG_DEBUG << fmt::format("Simulation Finished at cycle: {}", current_cycle);

  return 0;
}
