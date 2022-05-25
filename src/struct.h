#ifndef STRUCT_H
#define STRUCT_H

#include <component.h>
#include <string>
#include <task_queue.h>

namespace near_mem {
enum SearchState {
  query_init,
  query_init_finish,
  query_processed,
  query_finished
};
// enum DataTypeName {
//   uint8,
//   int8,
//   float32,
// };
// struct DataType {
//   std::vector<uint8_t> uint8data;
//   std::vector<int8_t> int8data;
//   std::vector<float> float32data;
// };
// struct NsgData {
//   uint64_t Query_ID;
//   uint64_t Query_Dimension;
//   std::vector<std::vector<float>> Query_Data;
//   uint64_t State;
//   uint64_t Add_cycle;
// };

struct NsgTask {
  unsigned query_id;
};

struct DistanceTask {
  uint64_t qe_name;
  unsigned query_id;
  // unsigned query_dimension;
  // std::vector<std::vector<float>> query_data;
  // unsigned target_id;
  // unsigned target_dimension;
  // std::vector<std::vector<float>> target_data;
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

extern uint64_t dimension;

} // namespace near_mem

#endif // STRUCT_H