#ifndef RAMULATOR_WRAPPER_RUST_H
#define RAMULATOR_WRAPPER_RUST_H

#include "Cache.h"
#include "Memory.h"
#include "RamulatorConfig.h"
#include "Request.h"
#include "Statistics.h"
#include <cctype>
#include <fstream>
#include <functional>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

#include <map>
#include <queue>
#include <set>
#include <tuple>
extern "C" {

void *get_ramulator(const void *config_name, const void *stats_name);
void ramulator_send(void *ramulator, unsigned long long addr, bool is_write);
unsigned long long ramulator_get(const void *ramulator);
unsigned long long ramulator_pop(void *ramulator);
void ramulator_cycle(void *ramulator);
bool ramulator_ret_available(void *);
bool ramulator_available(void *, unsigned long long addr);
void delete_ramulator(void *);
}
namespace rust_bridge {

class ramulator_wrapper {
public:
  void send(uint64_t addr, bool is_write);
  bool available(uint64_t addr) const {
    unsigned channel_id = get_channel_id(addr);
    return in_queue[channel_id].size() <= 512;
  }

  void finish();

  ramulator_wrapper(ramulator::Config configs, int cacheLine,
                    const char *config_file);

  ~ramulator_wrapper();

  void call_back(ramulator::Request &req);

  [[nodiscard]] bool empty() const;

  [[nodiscard]] std::string get_internal_size() const;

  [[nodiscard]] std::string get_line_trace() const;

  [[nodiscard]] uint64_t get() const { return out_queue.front(); }

  uint64_t pop() {
    auto ret = out_queue.front();
    out_queue.pop();
    return ret;
  }

  [[nodiscard]] bool return_available() const { return !out_queue.empty(); }

  void cycle();
  [[nodiscard]] unsigned get_channel_num() const;
  [[nodiscard]] unsigned get_channel_id(uint64_t addr) const;

private:
  void tick();

  double tCK;
  uint64_t outgoing_reqs = 0;
  uint64_t sum_rd_latency = 0;

  // addr,iswrite
  std::vector<std::queue<std::pair<uint64_t, bool>>> in_queue;
  std::queue<uint64_t> out_queue;
  ramulator::MemoryBase *mem;

  u_int64_t my_cycles;
  u_int64_t active_cycles;
  u_int64_t inflight_req_cnt;
  u_int64_t sum_inflight_req;
  u_int64_t finished_read_req;
  u_int64_t finished_write_req;

  // BLP at the interface
  int bank_req_cnt[512];
  int bank_infligt_req_cnt;
  u_int64_t sum_inflight_bank_req;
#define Channel_LEVEL 0
#define Rank_LEVEL 1
#define BankGroup_LEVEL 2
#define Bank_LEVEL 3
};

} // namespace rust_bridge
#endif