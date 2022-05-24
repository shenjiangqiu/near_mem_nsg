#ifndef STRUCT_H
#define STRUCT_H

#include <component.h>
#include <string>
#include <task_queue.h>

namespace near_mem {
enum SearchState{
    query_init,
    query_init_finish,
    query_processed,
    query_finished
};
enum DataTypeName{
    uint8,
    int8,
    float32,
};
struct DataType{
    std::vector<uint8_t> uint8data;
    std::vector<int8_t> int8data;
    std::vector<float> float32data;
};
struct NsgData{
    DataTypeName name;
    uint64_t Query_ID;
    uint64_t Query_Dimension;
    uint64_t Query_Vector;
    std::vector<std::vector<DataType>> Query_Data;
    uint64_t State;
    uint64_t Add_cycle;
};

struct NsgTask{
  unsigned query_id;
};
// int test(){
//   NsgTask mytask;
//   if(mytask.data->name==uint8){
//     auto data=mytask.data->Query_Data[0][0].uint8data;
//   }
//   return 0;
// }

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

uint64_t dimension;

}

#endif // STRUCT_H