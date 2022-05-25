#ifndef RAMULATOR_WRAPPER_H
#define RAMULATOR_WRAPPER_H
#include "Cache.h"
#include "Memory.h"
#include "RamulatorConfig.h"
#include "Request.h"
#include "Statistics.h"
#include <component.h>
#include <ctype.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <task_queue.h>
#include <tuple>
#include <vector>

namespace near_mem {
struct MemTask {
  uint64_t addr;
  bool is_read;
};

class ramulator_wrapper : public Component {
public:
  void send(uint64_t addr, bool is_read);
  void tick();
  void finish();
  ramulator_wrapper(const ramulator::Config &configs, int cacheline,
                    uint64_t &t_current_cycle, Receiver<MemTask>&& task_rx,
                    Sender<uint64_t> &&ret_tx);
  ~ramulator_wrapper();
  void call_back(ramulator::Request &req);
  std::string get_internal_size() const override;
  std::string get_line_trace() const override;
  bool all_end() const override{
    return true;
  }
  [[nodiscard]] unsigned long long int getTotalRequestsRead() const {
    return total_requests_read;
  }
  [[nodiscard]] unsigned long long int getTotalRequestsWrite() const {
    return total_requests_write;
  }
  [[nodiscard]] unsigned long long int getCycleInMemory() const {
    return cycle_in_memory;
  }
  bool have_outgoing() const { return outgoing_reqs != 0; }

protected:
  bool do_cycle() override;

private:
  unsigned long long total_requests_read = 0;
  unsigned long long total_requests_write = 0;
  unsigned long long cycle_in_memory = 0;
  double tCK;
  unsigned long long outgoing_reqs = 0;
  Receiver<MemTask> in_queue;
  std::queue<uint64_t> temp_receive_queue;
  Sender<uint64_t> out_queue;
  ramulator::MemoryBase *mem;
};
} // namespace near_mem
#endif