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

enum WhileRequestState {
  READ_NOT_SEND,
  READ_INFLIGHT,
  READ_FINISHED,
  DC_INFLIGHT,
  DC_FINISHED
};

struct NsgTask {
  unsigned query_id;
};

struct DistanceTask {
  uint64_t qe_name;
  unsigned query_id;
  unsigned node_id;
  // unsigned query_dimension;
  // std::vector<std::vector<float>> query_data;
  // unsigned target_id;
  // unsigned target_dimension;
  // std::vector<std::vector<float>> target_data;
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
  int state;
};



struct PeTask {
  /*
      todo the pe task that send to PE
  */
};
struct RetTask {
  /*
      todo the ret task that send from PE
  */
};

} // namespace near_mem

extern unsigned dimension;
extern unsigned L, K;
extern float* data_load;
extern float* query_load;
extern unsigned points_num, dim, query_num, query_dim;
extern std::vector<efanna2e::Neighbor> global_retset;
extern std::vector<unsigned> global_init_ids;
extern boost::dynamic_bitset<> global_flags;
extern PrimitiveBloomFilter<unsigned,80000> global_BF;
extern efanna2e::IndexNSG* global_index;

#endif // STRUCT_H