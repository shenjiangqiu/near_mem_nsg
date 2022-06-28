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
  void print_Neighbor(const efanna2e::Neighbor nn){
    std::cout << "Neighbor: " << nn.id << " " << nn.distance << " " << nn.flag << " ";
  }
  unsigned init_loop_iter = 0;
  std::vector<efanna2e::Neighbor> retset = std::vector<efanna2e::Neighbor>(L+1);
  std::vector<unsigned> init_ids = std::vector<unsigned>(L, 0);
  boost::dynamic_bitset<> flags = boost::dynamic_bitset<>{points_num, 0};
  int init_dc_state[200];
  int while_k = 0;//reserve for the while loop
  unsigned edge_table_id = 0;
  std::vector<QE_DC_Task> init_task_list;
  QE_DC_Task edge_read_task;
  std::vector<QE_DC_Task> dc_task_list;
  uint64_t first_return = 0;//mem receive first return
  uint64_t last_return = 0;//mem receive last return
  bool is_first = true;//mem receive first return flag
  uint64_t sub_first_return = 0;//mem receive first return
  uint64_t sub_last_return = 0;//mem receive last return
  bool sub_is_first = true;//mem receive first return flag
  uint64_t exe_time_start = 0;
  uint64_t time_edge_start = 0;
  uint64_t time_vec_start = 0;
  int offset_edge = 0;
  int offset_vec = 0;
  int send_edge = 0;
  int send_vec = 0;
  int send_dc = 0;
  int rcv_edge = 0;
  int rcv_vec = 0;
  int rcv_dc = 0;
  int total_edge = 0;
  int total_vec = 0;
  int64_t total_edge_time = 0;
  int64_t total_vec_time = 0;
  int64_t total_dc_time = 0;
  int64_t qe0_edge_time = 0;
  int64_t qe0_vec_time = 0;
  int qe0_edge = 0;
  int qe0_vec = 0;
  void address_mapping(unsigned id, bool is_edge_read, std::vector<uint64_t>& address_list, int& accumulate_offset);


};
} // namespace near_mem

#endif