#ifndef STRUCT_H
#define STRUCT_H

#include <component.h>
#include <string>
#include <task_queue.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Log.h>
#include <efanna2e/exceptions.h>
#include <efanna2e/index_nsg.h>
#include <efanna2e/parameters.h>
#include <efanna2e/util.h>
#include "PrimitiveBloomFilter.h"
#include <bitset>
#include <boost/dynamic_bitset.hpp>

#define TOML_HEADER_ONLY 1
#define CACHELINE_SIZE 64

namespace near_mem {
enum SearchState {
  query_init,
  query_init_finish,
  query_get_data,
  query_pending_edge,
  query_pending_distance,
  query_finish
};

enum RequestState {
  NOT_SEND,
  INFLIGHT,
  FINISHED
};

enum DC_unit_State {
  MINUS_PENDING,
  MULTI_PENDING,
  PLUS_PENDING,
  FINISHED_PENDING
};


enum WhileRequestState {
  READ_NOT_SEND,
  READ_INFLIGHT,
  READ_FINISHED,
  DC_INFLIGHT,
  DC_FINISHED,
  SORT_FINISHED
};

struct NsgTask {
  unsigned query_id;
};

struct DistanceTask {
  uint64_t qe_name;
  unsigned query_id;
  unsigned node_id;
};

struct MemReadTask {
  uint64_t addr;
  bool is_read;
  uint64_t qe_name;
  unsigned node_id;
  bool is_edge_table;
};

struct QE_MEM_State{
  uint64_t mem_read_addr;
  int state;
};

struct QE_DC_Task {
  uint64_t qe_name;
  unsigned query_id;
  unsigned node_id;
  std::vector<QE_MEM_State> mem_read_state;
  int remaining_compare_cycle;
  int state;
};

struct DC_Task_state{
  unsigned state;
  unsigned remaining_cycle;
};

} // namespace near_mem

extern unsigned dimension;
extern unsigned L, K;
extern float* data_load;
extern float* query_load;
extern unsigned points_num, dim, query_num, query_dim;
extern unsigned num_compute_unit;
extern unsigned latency_compute_unit;
extern std::vector<efanna2e::Neighbor> global_retset;
extern std::vector<unsigned> global_init_ids;
extern boost::dynamic_bitset<> global_flags;
extern PrimitiveBloomFilter<unsigned,80000> global_BF;
extern efanna2e::IndexNSG* global_index;
extern uint64_t stall_qe; //query start -> end
extern uint64_t stall_dc; //mem ready -> dc finish
extern uint64_t stall_dc1;
extern uint64_t stall_mem; //send mem -> mem ready
extern uint64_t stall_mem1;
extern uint64_t stall_mem2;
extern uint64_t stall_mem3;
extern uint64_t stall_mem4;
extern uint64_t stall_qe_stage1;
extern uint64_t stall_qe_stage2;
extern uint64_t stall_qe_stage3;
extern uint64_t stall_qe_stage4;
extern uint64_t stall_qe_stage5;
extern uint64_t stall_qe_stage6;
extern uint64_t stall_qe_stage7;
extern uint64_t stall_qe_stage1_qe0;
extern uint64_t stall_qe_stage2_qe0;
extern uint64_t stall_qe_stage3_qe0;
extern uint64_t stall_qe_stage4_qe0;
extern uint64_t stall_qe_stage5_qe0;
extern uint64_t stall_qe_stage6_qe0;
extern uint64_t stall_qe_stage7_qe0;
extern uint64_t mem_receive_delta;
extern uint64_t mem_receive_count;
extern uint64_t sub_mem_receive_delta;
extern uint64_t sub_mem_receive_count;
extern uint64_t unfold_num;
extern uint64_t exe_time;

extern unsigned global_sizeof;

#endif // STRUCT_H