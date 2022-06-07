#ifndef QE_H
#define QE_H

#include <component.h>
#include <struct.h>
#include <task_queue.h>
#include <fmt/format.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Log.h>
#include <efanna2e/index_nsg.h>
#include <efanna2e/parameters.h>
#include <efanna2e/util.h>
#include <efanna2e/neighbor.h>
#include <efanna2e/distance.h>
#include "PrimitiveBloomFilter.h"
#include <bitset>
#include <boost/dynamic_bitset.hpp>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

namespace near_mem {
class Query_Engine : public Component {
  using TaskReceiver = Receiver<NsgTask>;
  using DistanceSender = Sender<DistanceTask>;
  using DistanceReceiver = Receiver<DistanceTask>;
  using MemSender = Sender<MemReadTask>;
  using MemReceiver = Receiver<MemReadTask>;

public:
  Query_Engine(
    uint64_t &current_cycle, 
    uint64_t qe_name,
    TaskReceiver task_receiver, 
    DistanceSender distance_sender,
    DistanceReceiver return_distance_receiver,
    MemSender mem_sender,
    MemReceiver mem_receiver
    ) : 
  Component(current_cycle), 
  qe_name(qe_name),
  task_receiver(task_receiver),
  distance_sender(distance_sender),
  return_distance_receiver(return_distance_receiver),
  mem_sender(mem_sender),
  mem_receiver(mem_receiver) {}

  bool do_cycle() override;
  bool all_end() const override {
    auto end = !working();
    return end;
  }
  std::string get_internal_size() const override {
    return std::string("");
  }; // TODO add the internal status for the controller for debug
  std::string get_line_trace() const override { 
    return std::string(""); 
  }; //
  bool working() const { 
    // return remaining_cycle > 0; 
    return (query_state != query_finish);
  }

private:
  uint64_t qe_name;
  unsigned query_id = 0;
  TaskReceiver task_receiver;
  DistanceSender distance_sender;
  DistanceReceiver return_distance_receiver;
  MemSender mem_sender;
  MemReceiver mem_receiver;
  // TODO: Query state machine
  bool palse = false;
  unsigned palse_id = 0;//TODO: vector<unsigned> palse_list;
  
  unsigned remaining_cycle = 0;
  unsigned query_state = query_finish;

  int dc_test();
  void print_Neighbor(const efanna2e::Neighbor nn);
  unsigned init_loop_iter = 0;
  // std::vector<efanna2e::Neighbor> retset[201];//?
  std::vector<efanna2e::Neighbor> retset = std::vector<efanna2e::Neighbor>(L+1);
  std::vector<unsigned> init_ids = std::vector<unsigned>(L, 0);
  boost::dynamic_bitset<> flags = boost::dynamic_bitset<>{points_num, 0};
  // int query_init_state[200];
  // int init_mem_state[200];
  int init_dc_state[200];
  int while_k = 0;//reserve for the while loop
  unsigned edge_table_id = 0;
  std::vector<QE_DC_Task> init_task_list;
  QE_DC_Task edge_read_task;
  std::vector<QE_DC_Task> dc_task_list;

  void address_mapping(unsigned id, bool is_edge_read, std::vector<uint64_t>& address_list);

  // int while_nk = 0;
  // int while_n = 0;
  // int while_m = 0;
  // int while_r = 0;

  // int temp_flags[200];
  // int while_enter_flag[200];//while_enter_flag[k]  ->  retset[while_k].flag, think again
  // //TODO: maybe all don't need [label]?
  // int while_edge_table_read_state[200][50];
  // int while_vec_read_state[200][50];
  // int while_dc_state[50];
  // int while_for_in_loop_m[200];
  // float xxx = 0;
  // efanna2e::Distance* distance_;
  // efanna2e::IndexNSG index;

};
} // namespace near_mem

#endif