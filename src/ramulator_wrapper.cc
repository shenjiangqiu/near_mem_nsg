#include "DDR3.h"
#include "DDR4.h"
#include "GDDR5.h"
#include "HBM.h"
#include "LPDDR3.h"
#include "LPDDR4.h"
#include "Memory.h"
#include "MemoryFactory.h"
#include "RamulatorConfig.h"
#include "Request.h"
#include "SALP.h"
#include "WideIO.h"
#include "WideIO2.h"
#include <MemoryFactory.h>
#include <fmt/format.h>
#include <map>
#include <ramulator_wrapper.h>
namespace near_mem {
static map<string,
           function<ramulator::MemoryBase *(const ramulator::Config &, int)>>
    name_to_func = {
        {"DDR3", &ramulator::MemoryFactory<ramulator::DDR3>::create},
        {"DDR4", &ramulator::MemoryFactory<ramulator::DDR4>::create},
        {"LPDDR3", &ramulator::MemoryFactory<ramulator::LPDDR3>::create},
        {"LPDDR4", &ramulator::MemoryFactory<ramulator::LPDDR4>::create},
        {"GDDR5", &ramulator::MemoryFactory<ramulator::GDDR5>::create},
        {"WideIO", &ramulator::MemoryFactory<ramulator::WideIO>::create},
        {"WideIO2", &ramulator::MemoryFactory<ramulator::WideIO2>::create},
        {"HBM", &ramulator::MemoryFactory<ramulator::HBM>::create},
        {"SALP-1", &ramulator::MemoryFactory<ramulator::SALP>::create},
        {"SALP-2", &ramulator::MemoryFactory<ramulator::SALP>::create},
        {"SALP-MASA", &ramulator::MemoryFactory<ramulator::SALP>::create},
};

ramulator_wrapper::ramulator_wrapper(const ramulator::Config &configs,
                                     int cacheline, uint64_t &t_current_cycle,
                                     Receiver<MemTask> &&task_rx,
                                     Sender<uint64_t> &&ret_tx)
    : Component(t_current_cycle), in_queue(std::move(task_rx)),
      out_queue(std::move(ret_tx)) {
  const string &std_name = configs["standard"];
  assert(name_to_func.find(std_name) != name_to_func.end() &&
         "unrecognized standard name");
  mem = name_to_func[std_name](configs, cacheline);
  Stats::statlist.output("mem_stats.txt");
  tCK = mem->clk_ns();
}
ramulator_wrapper::~ramulator_wrapper() {
  fmt::print("total_read: {}\ntotal_write: {}\ntotal_cycle: {}\n",
             total_requests_read, total_requests_write, cycle_in_memory);
  Stats::statlist.printall();
  mem->finish();
  delete mem;
}
void ramulator_wrapper::finish() { mem->finish(); }
void ramulator_wrapper::tick() {
  cycle_in_memory++;
  mem->tick();
}
void ramulator_wrapper::call_back(ramulator::Request &req) {
  outgoing_reqs--;
  assert((long long)outgoing_reqs >= 0);
  switch (req.type) {
  case ramulator::Request::Type::READ:
    temp_receive_queue.push(req.addr);
    break;

  default:
    assert(req.type == ramulator::Request::Type::WRITE);
    break;
  }
}
bool ramulator_wrapper::do_cycle() {
  bool busy = false;
  if (!in_queue.empty()) {
    busy = true;
    auto req = in_queue.get();
    auto r_req = ramulator::Request(
        req.addr,
        req.is_read ? ramulator::Request::Type::READ
                    : ramulator::Request::Type::WRITE,
        [this](ramulator::Request &req) { this->call_back(req); });
    if (mem->send(r_req)) {
      if (req.is_read)
        total_requests_read++;
      else
        total_requests_write++;
      outgoing_reqs++;
      in_queue.pop();
    }
  }

  if (!out_queue.full()) {
    if (!temp_receive_queue.empty()) {
      auto addr = temp_receive_queue.front();
      temp_receive_queue.pop();
      out_queue.push(addr);
    }
  }
  this->tick();
  return busy;
}

std::string ramulator_wrapper::get_internal_size() const {
  return fmt::format("name:{} in:{} out:{} outgoing:{} cycle:{}\n", "mem",
                     in_queue.size(), out_queue.size(), outgoing_reqs,
                     current_cycle);
}

std::string ramulator_wrapper::get_line_trace() const {
  return fmt::format("{}", get_busy_percent());
}
} // namespace near_mem