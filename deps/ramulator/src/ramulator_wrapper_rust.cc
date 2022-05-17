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
#include "queue"
#include <MemoryFactory.h>
#include <functional>
#include <iostream>
#include <map>
#include <ramulator_wrapper_rust.h>
#include <string>
namespace rust_bridge
{


// this function will be automatically in rust, do not call it mannully
using namespace ramulator;
void *get_ramulator(const void *config_name, const void *stats_name) {
  auto ramu = new ramulator_wrapper(Config((const char *)config_name), 64,
                                    (const char *)stats_name);
  return ramu;
}
// this function will be automatically called in rust, do not call it mannully
void delete_ramulator(void *ramu) {
  auto m_wrapper = (ramulator_wrapper *)ramu;
  m_wrapper->finish();
  delete m_wrapper;
}

void ramulator_send(void *ramu, unsigned long long addr, bool is_write) {
  auto m_wrapper = (ramulator_wrapper *)ramu;
  m_wrapper->send(addr, is_write);
}

unsigned long long ramulator_get(const void *ramu) {
  const ramulator_wrapper *m_wrapper = (const ramulator_wrapper *)ramu;
  return m_wrapper->get();
}
unsigned long long ramulator_pop(void *ramu) {
  ramulator_wrapper *m_wrapper = (ramulator_wrapper *)ramu;
  return m_wrapper->pop();
}

void ramulator_cycle(void *ramu) {
  auto m_wrapper = (ramulator_wrapper *)ramu;

  m_wrapper->cycle();
}

bool ramulator_ret_available(void *ramu) {
  auto m_wrapper = (ramulator_wrapper *)ramu;
  return m_wrapper->return_available();
}

bool ramulator_available(void *ramu, unsigned long long addr) {
  auto m_wrapper = (ramulator_wrapper *)ramu;
  return m_wrapper->available(addr);
}

using namespace ramulator;

static map<string, function<MemoryBase *(const Config &, int)>> name_to_func = {
    {"DDR3", &MemoryFactory<DDR3>::create},
    {"DDR4", &MemoryFactory<DDR4>::create},
    {"LPDDR3", &MemoryFactory<LPDDR3>::create},
    {"LPDDR4", &MemoryFactory<LPDDR4>::create},
    {"GDDR5", &MemoryFactory<GDDR5>::create},
    {"WideIO", &MemoryFactory<WideIO>::create},
    {"WideIO2", &MemoryFactory<WideIO2>::create},
    {"HBM", &MemoryFactory<HBM>::create},
    {"SALP-1", &MemoryFactory<SALP>::create},
    {"SALP-2", &MemoryFactory<SALP>::create},
    {"SALP-MASA", &MemoryFactory<SALP>::create},
};

ramulator_wrapper::ramulator_wrapper(const ramulator::Config configs,
                                     int cacheLine, const char *stats_name) {
  const string &std_name = configs["standard"];
  assert(name_to_func.find(std_name) != name_to_func.end() &&
         "unrecognized standard name");
  mem = name_to_func[std_name](configs, cacheLine);
  Stats::statlist.output(stats_name);
  tCK = mem->clk_ns();
  in_queue.resize(get_channel_num());
  inflight_req_cnt = 0;
  sum_inflight_req = 0;
  my_cycles = 0;
  active_cycles = 0;
  finished_read_req = 0;
  finished_write_req = 0;
  sum_rd_latency = 0;
}
ramulator_wrapper::~ramulator_wrapper() {

  float mlp = (float)sum_inflight_req / active_cycles;
  float activeRate = (float)active_cycles / my_cycles;
  float blp = (float)sum_inflight_bank_req / active_cycles;
  double avgLatency = (double)sum_rd_latency / finished_read_req;
  ostream &out = Stats::statlist.stat_output;
  out << "Interface MLP " << mlp << " BLP  " << blp << " memoy activeRate "
      << activeRate;
  out << " BW "
      << (finished_read_req + finished_write_req) * 64.0 / active_cycles;
  out << " lat " << avgLatency;
  out << " readRqt " << finished_read_req << "  writeRqt " << finished_write_req
      << "\n";

  finish();

  delete mem;
}

void ramulator_wrapper::finish() {
  mem->finish();
  Stats::statlist.printall();
}

void ramulator_wrapper::tick() { mem->tick(); }

void ramulator_wrapper::send(uint64_t addr, bool is_write) {
  auto channel_id = get_channel_id(addr);
  this->in_queue[channel_id].push({addr, is_write});
}

void ramulator_wrapper::call_back(ramulator::Request &req) {
  outgoing_reqs--;
  inflight_req_cnt--;

  assert((long long)outgoing_reqs >= 0);
  switch (req.type) {
  case Request::Type::READ:
    out_queue.push(req.addr);
    finished_read_req++;
    sum_rd_latency += (req.depart - req.arrive);
    break;

  case Request::Type::WRITE:
    finished_write_req++;
    break;

  default:

    break;
  }

  auto bank = req.addr_vec[Bank_LEVEL];
  auto channel = req.addr_vec[Channel_LEVEL];
  int bank_id = channel * 16 + bank;

  if (bank_req_cnt[bank_id]) {
    bank_req_cnt[bank_id]--;
    if (bank_req_cnt[bank_id] == 0)
      bank_infligt_req_cnt--;
  }
}

void ramulator_wrapper::cycle() {

  my_cycles++;
  if (inflight_req_cnt) {
    active_cycles++;
    sum_inflight_req += inflight_req_cnt;
    sum_inflight_bank_req += bank_infligt_req_cnt;
  }
  for (unsigned i = 0; i < get_channel_num(); i++) {
    if (!in_queue[i].empty()) {
      auto &req = in_queue[i].front();
      // first addr, second: is_write
      auto r_req = Request(
          req.first, req.second ? Request::Type::WRITE : Request::Type::READ,
          [this](Request &req) { this->call_back(req); });
      if (mem->send(r_req)) {
        inflight_req_cnt++;
        outgoing_reqs++;
        in_queue[i].pop();

        int bank_id = mem->getBankID();
        bank_req_cnt[bank_id]++;
        if (bank_req_cnt[bank_id] == 1)
          bank_infligt_req_cnt++;
      }
    }
  }
  this->tick();
}

bool ramulator_wrapper::empty() const {
  return in_queue.empty() and out_queue.empty() and outgoing_reqs == 0;
}

std::string ramulator_wrapper::get_line_trace() const { return ""; }
unsigned ramulator_wrapper::get_channel_num() const { return 8; }
unsigned ramulator_wrapper::get_channel_id(uint64_t addr) const {
  return mem->get_channel_id(addr);
}
  
} // namespace rust_bridge